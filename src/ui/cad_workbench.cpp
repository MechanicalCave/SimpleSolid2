#include "cad_workbench.hpp"
#include "cad_workbench_shell.hpp"
#include "part_document_tree_controller.hpp"
#include "part_sketch_interaction_controller.hpp"
#include "part_viewport_controller.hpp"

#include <simplesolid2/application/cad_input_semantics.hpp>
#include <simplesolid2/application/precision_input.hpp>

#include <QCheckBox>
#include <QComboBox>
#include <QEvent>
#include <QFormLayout>
#include <QGridLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

#include <cmath>
#include <numbers>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace simplesolid2::ui {
namespace {

std::string toUtf8(const QString& value) {
    const auto bytes = value.toUtf8();
    return std::string{
        bytes.constData(),
        static_cast<std::size_t>(bytes.size())};
}

QString fromUtf8(std::string_view value) {
    return QString::fromUtf8(
        value.data(),
        static_cast<qsizetype>(value.size()));
}

[[nodiscard]] int lengthUnitIndex(
    core::LengthUnit unit) noexcept {
    switch (unit) {
    case core::LengthUnit::millimetre: return 0;
    case core::LengthUnit::centimetre: return 1;
    case core::LengthUnit::metre: return 2;
    case core::LengthUnit::inch: return 3;
    case core::LengthUnit::foot: return 4;
    }
    return 0;
}

[[nodiscard]] std::optional<core::LengthUnit>
lengthUnitForIndex(int index) noexcept {
    switch (index) {
    case 0: return core::LengthUnit::millimetre;
    case 1: return core::LengthUnit::centimetre;
    case 2: return core::LengthUnit::metre;
    case 3: return core::LengthUnit::inch;
    case 4: return core::LengthUnit::foot;
    default: return std::nullopt;
    }
}

[[nodiscard]] double normalizePolarAngle(
    double angle) noexcept {
    constexpr double full_turn =
        2.0 * std::numbers::pi_v<double>;
    double result =
        std::fmod(angle, full_turn);
    if (result < 0.0) {
        result += full_turn;
    }
    return result == full_turn
        ? 0.0
        : result;
}

[[nodiscard]] bool equivalentPolarAngle(
    double first,
    double second) noexcept {
    constexpr double full_turn =
        2.0 * std::numbers::pi_v<double>;
    const double delta =
        std::abs(
            normalizePolarAngle(
                first - second));
    return std::min(
               delta,
               full_turn - delta) <=
           1.0e-12;
}

[[nodiscard]] QString polarAngleText(
    double radians) {
    return QStringLiteral("%1°")
        .arg(
            QString::number(
                radians * 180.0 /
                    std::numbers::pi_v<double>,
                'g',
                12));
}

[[nodiscard]] QString polarAdditionalSummary(
    const std::vector<double>& angles) {
    if (angles.empty()) {
        return QStringLiteral("none");
    }

    QString result;
    for (const double angle : angles) {
        if (!result.isEmpty()) {
            result += QStringLiteral(", ");
        }
        result += polarAngleText(angle);
    }
    return result;
}

[[nodiscard]] QString polarSpacingSummary(
    double radians) {
    if (!std::isfinite(radians) ||
        radians <= 0.0) {
        return QStringLiteral("Invalid");
    }

    const double degrees =
        radians * 180.0 /
        std::numbers::pi_v<double>;
    const double divisions =
        2.0 * std::numbers::pi_v<double> /
        radians;
    const double rounded =
        std::round(divisions);

    if (std::isfinite(divisions) &&
        rounded >= 1.0 &&
        std::abs(divisions - rounded) <
            1.0e-10) {
        return QStringLiteral("360/%1 = %2°")
            .arg(
                QString::number(
                    static_cast<qlonglong>(
                        rounded)))
            .arg(
                QString::number(
                    degrees,
                    'g',
                    12));
    }

    return QStringLiteral("%1°")
        .arg(
            QString::number(
                degrees,
                'g',
                12));
}

QString fromFilesystemPath(const std::filesystem::path& value) {
#if defined(_WIN32)
    return QString::fromStdWString(value.wstring());
#else
    const auto utf8 = value.generic_u8string();
    return QString::fromUtf8(
        reinterpret_cast<const char*>(utf8.data()),
        static_cast<qsizetype>(utf8.size()));
#endif
}

QString directEditModeText(
    sketch::DirectEditMode mode) {
    switch (mode) {
    case sketch::DirectEditMode::reshape:
        return QStringLiteral("Reshape");
    case sketch::DirectEditMode::move:
        return QStringLiteral("Move");
    case sketch::DirectEditMode::rotate:
        return QStringLiteral("Rotate");
    case sketch::DirectEditMode::scale:
        return QStringLiteral("Scale");
    case sketch::DirectEditMode::mirror:
        return QStringLiteral("Mirror");
    }
    return QStringLiteral("Unknown");
}

QString measurementRoleText(
    sketch::EntityRole role) {
    return role == sketch::EntityRole::construction
        ? QStringLiteral("Construction")
        : QStringLiteral("Regular");
}

QString formatLengthMeasurement(
    double canonical_millimetres,
    core::LengthUnit unit) {
    const auto value =
        core::fromCanonicalLength(
            core::LengthValue{
                canonical_millimetres},
            unit);
    return QStringLiteral("%1 %2")
        .arg(
            QString::number(value, 'g', 12),
            fromUtf8(
                core::lengthUnitSuffix(unit)));
}

QString formatAreaMeasurement(
    double canonical_square_millimetres,
    core::LengthUnit unit) {
    const double scale =
        core::millimetresPerUnit(unit);
    const double value =
        canonical_square_millimetres /
        (scale * scale);
    return QStringLiteral("%1 %2²")
        .arg(
            QString::number(value, 'g', 12),
            fromUtf8(
                core::lengthUnitSuffix(unit)));
}

QString formatMeasurement(
    const sketch::EntityMeasurement& measurement,
    core::LengthUnit unit) {
    const auto number = [](double value) {
        return QString::number(value, 'g', 12);
    };
    const auto degrees = [&number](double radians) {
        return number(
            radians * 180.0 /
            std::numbers::pi_v<double>) +
            QStringLiteral("°");
    };

    return std::visit(
        [&](const auto& value) -> QString {
            using Value =
                std::decay_t<decltype(value)>;
            const auto id =
                fromUtf8(value.entity_id.serialized());
            const auto role =
                measurementRoleText(value.role);

            if constexpr (
                std::is_same_v<
                    Value,
                    sketch::LineMeasurement>) {
                return QStringLiteral(
                           "Measure — Line [%1]\n"
                           "Role: %2\n"
                           "Length: %3\n"
                           "Delta U: %4\n"
                           "Delta V: %5\n"
                           "Angle +U: %6")
                    .arg(
                        id,
                        role,
                        formatLengthMeasurement(
                            value.length,
                            unit),
                        formatLengthMeasurement(
                            value.delta_u,
                            unit),
                        formatLengthMeasurement(
                            value.delta_v,
                            unit),
                        degrees(
                            value.angle_from_positive_u));
            } else if constexpr (
                std::is_same_v<
                    Value,
                    sketch::CircleMeasurement>) {
                return QStringLiteral(
                           "Measure — Circle [%1]\n"
                           "Role: %2\n"
                           "Radius: %3\n"
                           "Diameter: %4\n"
                           "Circumference: %5\n"
                           "Area: %6")
                    .arg(
                        id,
                        role,
                        formatLengthMeasurement(
                            value.radius,
                            unit),
                        formatLengthMeasurement(
                            value.diameter,
                            unit),
                        formatLengthMeasurement(
                            value.circumference,
                            unit),
                        formatAreaMeasurement(
                            value.area,
                            unit));
            } else {
                return QStringLiteral(
                           "Measure — Arc [%1]\n"
                           "Role: %2\n"
                           "Radius: %3\n"
                           "Start angle: %4\n"
                           "End angle: %5\n"
                           "Signed sweep: %6\n"
                           "Arc length: %7")
                    .arg(
                        id,
                        role,
                        formatLengthMeasurement(
                            value.radius,
                            unit),
                        degrees(value.start_angle),
                        degrees(value.end_angle),
                        degrees(
                            value.signed_sweep_angle),
                        formatLengthMeasurement(
                            value.arc_length,
                            unit));
            }
        },
        measurement);
}

QString measurePointRoleText(
    sketch::MeasurePointRole role) {
    switch (role) {
    case sketch::MeasurePointRole::line_start:
        return QStringLiteral("Line Start");
    case sketch::MeasurePointRole::line_midpoint:
        return QStringLiteral("Line Midpoint");
    case sketch::MeasurePointRole::line_end:
        return QStringLiteral("Line End");
    case sketch::MeasurePointRole::circle_center:
        return QStringLiteral("Circle Center");
    case sketch::MeasurePointRole::circle_quadrant_pos_u:
        return QStringLiteral("Circle +U Quadrant");
    case sketch::MeasurePointRole::circle_quadrant_pos_v:
        return QStringLiteral("Circle +V Quadrant");
    case sketch::MeasurePointRole::circle_quadrant_neg_u:
        return QStringLiteral("Circle -U Quadrant");
    case sketch::MeasurePointRole::circle_quadrant_neg_v:
        return QStringLiteral("Circle -V Quadrant");
    case sketch::MeasurePointRole::arc_center:
        return QStringLiteral("Arc Center");
    case sketch::MeasurePointRole::arc_start:
        return QStringLiteral("Arc Start");
    case sketch::MeasurePointRole::arc_end:
        return QStringLiteral("Arc End");
    case sketch::MeasurePointRole::arc_midpoint:
        return QStringLiteral("Arc Midpoint");
    }
    return QStringLiteral("Point");
}

QString formatMeasureRelationTarget(
    const sketch::MeasureRelationTarget& target) {
    return std::visit(
        [](const auto& value) -> QString {
            using Value =
                std::decay_t<decltype(value)>;
            if constexpr (
                std::is_same_v<
                    Value,
                    sketch::MeasurePointRef>) {
                return QStringLiteral("Point [%1] — %2")
                    .arg(
                        fromUtf8(
                            value.entity_id.serialized()),
                        measurePointRoleText(value.role));
            } else {
                return QStringLiteral("Line [%1]")
                    .arg(
                        fromUtf8(
                            value.entity_id.serialized()));
            }
        },
        target);
}

QString formatRelationalMeasurement(
    const sketch::RelationalMeasurement& measurement,
    const sketch::MeasureRelationTarget& first,
    const sketch::MeasureRelationTarget& second,
    core::LengthUnit unit) {
    const auto number = [](double value) {
        return QString::number(value, 'g', 12);
    };
    const auto degrees = [&number](double radians) {
        return number(
            radians * 180.0 /
            std::numbers::pi_v<double>) +
            QStringLiteral("°");
    };

    const auto first_text =
        formatMeasureRelationTarget(first);
    const auto second_text =
        formatMeasureRelationTarget(second);

    return std::visit(
        [&](const auto& value) -> QString {
            using Value =
                std::decay_t<decltype(value)>;
            if constexpr (
                std::is_same_v<
                    Value,
                    sketch::PointPointMeasurement>) {
                return QStringLiteral(
                           "Measure Between\n"
                           "Target A: %1\n"
                           "Target B: %2\n"
                           "Distance: %3\n"
                           "Delta U: %4\n"
                           "Delta V: %5\n"
                           "Angle +U: %6")
                    .arg(first_text)
                    .arg(second_text)
                    .arg(
                        formatLengthMeasurement(
                            value.distance,
                            unit))
                    .arg(
                        formatLengthMeasurement(
                            value.delta_u,
                            unit))
                    .arg(
                        formatLengthMeasurement(
                            value.delta_v,
                            unit))
                    .arg(degrees(
                        value.angle_from_positive_u));
            } else if constexpr (
                std::is_same_v<
                    Value,
                    sketch::PointLineMeasurement>) {
                return QStringLiteral(
                           "Measure Between\n"
                           "Target A: %1\n"
                           "Target B: %2\n"
                           "Perpendicular distance: %3\n"
                           "Foot U: %4\n"
                           "Foot V: %5\n"
                           "Line semantics: infinite supporting line")
                    .arg(first_text)
                    .arg(second_text)
                    .arg(
                        formatLengthMeasurement(
                            value.distance,
                            unit))
                    .arg(
                        formatLengthMeasurement(
                            value.perpendicular_foot.u,
                            unit))
                    .arg(
                        formatLengthMeasurement(
                            value.perpendicular_foot.v,
                            unit));
            } else {
                return QStringLiteral(
                           "Measure Between\n"
                           "Target A: %1\n"
                           "Target B: %2\n"
                           "Smaller undirected angle: %3")
                    .arg(first_text)
                    .arg(second_text)
                    .arg(degrees(
                        value.smaller_undirected_angle));
            }
        },
        measurement);
}

std::optional<viewer::StandardView>
standardViewForSketchSupport(
    core::BuiltinReferenceRole role) noexcept {
    switch (role) {
    case core::BuiltinReferenceRole::xy_plane:
        return viewer::StandardView::top;
    case core::BuiltinReferenceRole::xz_plane:
        return viewer::StandardView::front;
    case core::BuiltinReferenceRole::yz_plane:
        return viewer::StandardView::right;
    default:
        return std::nullopt;
    }
}

} // namespace

CadWorkbench::CadWorkbench(QWidget* parent)
    : CadWorkbench{ViewportFactory{}, parent} {}

CadWorkbench::~CadWorkbench() = default;

CadWorkbench::CadWorkbench(
    ViewportFactory viewport_factory,
    QWidget* parent)
    : QWidget{parent},
      viewport_factory_{std::move(viewport_factory)} {
    buildUi();
    deactivateDocument();
    setStatusText(
        QStringLiteral("No Part is active."));
}

void CadWorkbench::buildUi() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);

    shell_ = new CadWorkbenchShell(this);
    shell_->setObjectName(QStringLiteral("cadWorkbenchShell"));
    root->addWidget(shell_, 1);

    auto& lifecycle_actions = shell_->documentActionsLayout();

    undo_button_ =
        new QPushButton(QStringLiteral("Undo"), shell_);
    undo_button_->setObjectName(QStringLiteral("undoDocumentButton"));

    redo_button_ =
        new QPushButton(QStringLiteral("Redo"), shell_);
    redo_button_->setObjectName(QStringLiteral("redoDocumentButton"));

    save_button_ =
        new QPushButton(QStringLiteral("Save"), shell_);
    save_button_->setObjectName(QStringLiteral("saveDocumentButton"));

    close_document_button_ =
        new QPushButton(QStringLiteral("Close"), shell_);
    close_document_button_->setObjectName(
        QStringLiteral("closeDocumentButton"));

    lifecycle_actions.addStretch(1);
    lifecycle_actions.addWidget(undo_button_);
    lifecycle_actions.addWidget(redo_button_);
    lifecycle_actions.addWidget(save_button_);
    lifecycle_actions.addWidget(close_document_button_);

    document_tree_ = &shell_->documentTree();
    status_ = &shell_->statusLabel();

    tree_controller_ =
        new PartDocumentTreeController(*document_tree_, this);
    tree_controller_->setResultHandler(
        [this](
            const application::DocumentSessionResult& result,
            bool visible) {
            if (!result.ok()) {
                showFailure(result.diagnostic);
                refreshActiveContext();
                return;
            }

            refreshActiveContext();
            setStatusText(
                result.changed
                    ? (visible
                           ? QStringLiteral(
                                 "Selected Origin references shown.")
                           : QStringLiteral(
                                 "Selected Origin references hidden."))
                    : QStringLiteral(
                          "No Origin visibility change."));
        });

    tree_controller_->setSketchEditHandler(
        [this](const sketch::SketchId& sketch_id) {
            requestEditSketch(sketch_id);
        });

    ViewportSurface viewport_surface;
    if (viewport_factory_) {
        viewport_surface = viewport_factory_(shell_);
    }

    auto* editor_container = new QFrame(shell_);
    editor_container->setObjectName(
        QStringLiteral("editorSurface"));
    editor_container->setFrameShape(QFrame::StyledPanel);
    auto* editor_layout =
        new QGridLayout(editor_container);
    editor_layout->setContentsMargins(0, 0, 0, 0);

    if (viewport_surface.valid()) {
        viewport_ = viewport_surface.viewport;
        auto* viewport_widget =
            viewport_surface.widget;
        viewport_widget_ = viewport_widget;

        viewport_widget->setObjectName(
            QStringLiteral("documentViewport"));
        if (viewport_widget->parentWidget() != editor_container) {
            viewport_widget->setParent(editor_container);
        }

        editor_layout->addWidget(
            viewport_widget,
            0,
            0);

        viewport_->setNavigationCubeActionHandler(
            [this](
                const viewer::NavigationCubeAction& action) {
                if (viewport_ == nullptr) {
                    return;
                }

                const auto current =
                    viewport_->cameraState();
                if (!current) {
                    return;
                }

                const auto navigation =
                    viewer::navigationForCubeAction(
                        *current,
                        action);
                if (!navigation) {
                    return;
                }

                const double duration_seconds =
                    action.kind ==
                            viewer::NavigationCubeActionKind::
                                toggle_projection
                        ? 0.0
                        : 0.28;

                static_cast<void>(
                    viewport_->animateCameraState(
                        navigation->camera,
                        duration_seconds,
                        navigation->fit_all));
            });
    } else {
        if (viewport_surface.widget != nullptr) {
            viewport_surface.widget->deleteLater();
        }

        viewport_ = nullptr;

        auto* editor_label = new QLabel(
            QStringLiteral(
                "3D Document View\n"
                "Native Viewer surface is unavailable."),
            editor_container);
        editor_label->setObjectName(
            QStringLiteral("editorSurfacePlaceholder"));
        editor_label->setAlignment(Qt::AlignCenter);
        editor_layout->addWidget(
            editor_label,
            0,
            0);
    }

    editor_surface_ = editor_container;
    shell_->setEditorSurface(editor_surface_);

    sketch_button_ =
        new QPushButton(
            QStringLiteral("Sketch"),
            shell_);
    sketch_button_->setObjectName(
        QStringLiteral("sketchToolButton"));
    shell_->editorToolsLayout().insertWidget(
        0,
        sketch_button_);

    select_sketch_button_ =
        new QPushButton(
            QStringLiteral("Select"),
            shell_);
    select_sketch_button_->setObjectName(
        QStringLiteral("selectSketchToolButton"));
    select_sketch_button_->setCheckable(true);
    shell_->editorToolsLayout().insertWidget(
        1,
        select_sketch_button_);

    create_tools_label_ =
        new QLabel(
            QStringLiteral("Create:"),
            shell_);
    create_tools_label_->setObjectName(
        QStringLiteral("sketchCreateToolsLabel"));
    shell_->editorToolsLayout().insertWidget(
        2,
        create_tools_label_);

    line_sketch_button_ =
        new QPushButton(
            QStringLiteral("Line"),
            shell_);
    line_sketch_button_->setObjectName(
        QStringLiteral("lineSketchToolButton"));
    line_sketch_button_->setCheckable(true);
    shell_->editorToolsLayout().insertWidget(
        3,
        line_sketch_button_);

    circle_sketch_button_ =
        new QPushButton(
            QStringLiteral("Circle"),
            shell_);
    circle_sketch_button_->setObjectName(
        QStringLiteral("circleSketchToolButton"));
    circle_sketch_button_->setCheckable(true);
    shell_->editorToolsLayout().insertWidget(
        4,
        circle_sketch_button_);

    arc_sketch_button_ =
        new QPushButton(
            QStringLiteral("Arc"),
            shell_);
    arc_sketch_button_->setObjectName(
        QStringLiteral("arcSketchToolButton"));
    arc_sketch_button_->setCheckable(true);
    shell_->editorToolsLayout().insertWidget(
        5,
        arc_sketch_button_);

    profile_tools_label_ =
        new QLabel(
            QStringLiteral("Profile:"),
            shell_);
    profile_tools_label_->setObjectName(
        QStringLiteral("sketchProfileToolsLabel"));
    shell_->editorToolsLayout().insertWidget(
        6,
        profile_tools_label_);

    profile_sketch_button_ =
        new QPushButton(
            QStringLiteral("Profile"),
            shell_);
    profile_sketch_button_->setObjectName(
        QStringLiteral("profileSketchToolButton"));
    profile_sketch_button_->setCheckable(true);
    shell_->editorToolsLayout().insertWidget(
        7,
        profile_sketch_button_);

    modify_tools_label_ =
        new QLabel(
            QStringLiteral("Modify:"),
            shell_);
    modify_tools_label_->setObjectName(
        QStringLiteral("sketchModifyToolsLabel"));
    shell_->editorToolsLayout().insertWidget(
        8,
        modify_tools_label_);

    move_sketch_button_ =
        new QPushButton(
            QStringLiteral("Move"),
            shell_);
    move_sketch_button_->setObjectName(
        QStringLiteral("moveSketchToolButton"));
    move_sketch_button_->setCheckable(true);
    shell_->editorToolsLayout().insertWidget(
        9,
        move_sketch_button_);

    copy_sketch_button_ =
        new QPushButton(
            QStringLiteral("Copy"),
            shell_);
    copy_sketch_button_->setObjectName(
        QStringLiteral("copySketchToolButton"));
    copy_sketch_button_->setCheckable(true);
    shell_->editorToolsLayout().insertWidget(
        10,
        copy_sketch_button_);

    rotate_sketch_button_ =
        new QPushButton(
            QStringLiteral("Rotate"),
            shell_);
    rotate_sketch_button_->setObjectName(
        QStringLiteral("rotateSketchToolButton"));
    rotate_sketch_button_->setCheckable(true);
    shell_->editorToolsLayout().insertWidget(
        11,
        rotate_sketch_button_);

    scale_sketch_button_ =
        new QPushButton(
            QStringLiteral("Scale"),
            shell_);
    scale_sketch_button_->setObjectName(
        QStringLiteral("scaleSketchToolButton"));
    scale_sketch_button_->setCheckable(true);
    shell_->editorToolsLayout().insertWidget(
        12,
        scale_sketch_button_);

    mirror_sketch_button_ =
        new QPushButton(
            QStringLiteral("Mirror"),
            shell_);
    mirror_sketch_button_->setObjectName(
        QStringLiteral("mirrorSketchToolButton"));
    mirror_sketch_button_->setCheckable(true);
    shell_->editorToolsLayout().insertWidget(
        13,
        mirror_sketch_button_);

    inspect_tools_label_ =
        new QLabel(
            QStringLiteral("Inspect:"),
            shell_);
    inspect_tools_label_->setObjectName(
        QStringLiteral("sketchInspectToolsLabel"));
    shell_->editorToolsLayout().insertWidget(
        14,
        inspect_tools_label_);

    measure_sketch_button_ =
        new QPushButton(
            QStringLiteral("Measure"),
            shell_);
    measure_sketch_button_->setObjectName(
        QStringLiteral("measureSketchToolButton"));
    measure_sketch_button_->setCheckable(true);
    shell_->editorToolsLayout().insertWidget(
        15,
        measure_sketch_button_);

    rectangle_sketch_button_ =
        new QPushButton(
            QStringLiteral("Rectangle"),
            shell_);
    rectangle_sketch_button_->setObjectName(
        QStringLiteral("rectangleSketchToolButton"));
    rectangle_sketch_button_->setCheckable(true);
    shell_->editorToolsLayout().insertWidget(
        6,
        rectangle_sketch_button_);

    viewport_controller_ =
        new PartViewportController(
            *tree_controller_,
            viewport_,
            this);
    viewport_controller_->setSelectionChangedHandler(
        [this](
            const std::vector<core::BuiltinReferenceRole>&,
            std::optional<core::BuiltinReferenceRole> primary) {
            refreshPropertiesContext(primary);
            tryCreateSketchFromSupport(primary);
        });

    viewport_controller_->setPresentationStateChangedHandler(
        [this](bool) {
            // Empty normal status means: show only the degradation
            // diagnostic while failed, and clear it after recovery.
            setStatusText(QString{});
        });

    sketch_interaction_controller_ =
        std::make_unique<PartSketchInteractionController>(
            *viewport_controller_);
    if (cad_interaction_settings_provider_) {
        sketch_interaction_controller_->
            setCadInteractionSettingsProvider(
                cad_interaction_settings_provider_);
    }
    sketch_interaction_controller_->setStateChangedHandler(
        [this] {
            syncSketchInteractionUi();
            syncActionState();
            notifyDocumentStateChanged();
        });
    sketch_interaction_controller_->setStatusHandler(
        [this](const std::string& message) {
            setStatusText(fromUtf8(message));
        });

    tree_controller_->setProfileSelectionHandler(
        [this](
            const std::vector<part::ProfileId>& selected,
            std::optional<part::ProfileId> primary) {
            const auto semantic =
                selected.size() == 1U && primary
                    ? primary
                    : std::nullopt;
            if (sketch_interaction_controller_) {
                sketch_interaction_controller_->
                    setSelectedProfileForCadInput(
                        semantic);
            }
            if (primary) {
                refreshProfileProperties(*primary);
            } else if (selected_profile_id_) {
                selected_profile_id_.reset();
                if (properties_stack_ != nullptr) {
                    properties_stack_->setCurrentWidget(
                        document_properties_page_);
                }
            }
            syncSketchInteractionUi();
        });
    viewport_controller_->setProfileSelectionChangedHandler(
        [this](
            const std::vector<part::ProfileId>& selected,
            std::optional<part::ProfileId> primary) {
            const auto semantic =
                selected.size() == 1U && primary
                    ? primary
                    : std::nullopt;
            if (sketch_interaction_controller_) {
                sketch_interaction_controller_->
                    setSelectedProfileForCadInput(
                        semantic);
            }
            if (primary) {
                refreshProfileProperties(*primary);
            } else if (selected_profile_id_) {
                selected_profile_id_.reset();
                if (properties_stack_ != nullptr) {
                    properties_stack_->setCurrentWidget(
                        document_properties_page_);
                }
            }
            syncSketchInteractionUi();
        });
    tree_controller_->setProfileEditHandler(
        [this](part::ProfileId profile_id) {
            requestEditProfile(profile_id);
        });

    viewport_controller_->setSketchPointerHandler(
        [this](const SketchPointerInput& input) {
            if (input.viewport_position.valid()) {
                dynamic_input_anchor_ =
                    input.viewport_position;
            } else {
                dynamic_input_anchor_.reset();
            }
            if (sketch_interaction_controller_) {
                sketch_interaction_controller_->onPointer(
                    input);
            }
            refreshCadDynamicInputOverlay();
        });

    if (viewport_widget_ != nullptr) {
        viewport_widget_->installEventFilter(this);
        setFocusProxy(viewport_widget_);
    }

    auto* properties_content = new QWidget(shell_);
    properties_content->setObjectName(
        QStringLiteral("partPropertiesContent"));
    auto* properties_root =
        new QVBoxLayout(properties_content);
    properties_root->setContentsMargins(0, 0, 0, 0);

    properties_stack_ =
        new QStackedWidget(properties_content);
    properties_stack_->setObjectName(
        QStringLiteral("propertiesContextStack"));
    properties_root->addWidget(properties_stack_, 1);

    document_properties_page_ =
        new QWidget(properties_stack_);
    document_properties_page_->setObjectName(
        QStringLiteral("documentPropertiesPage"));
    auto* document_properties_root =
        new QVBoxLayout(document_properties_page_);
    document_properties_root->setContentsMargins(0, 0, 0, 0);

    active_path_ =
        new QLabel(document_properties_page_);
    active_path_->setObjectName(
        QStringLiteral("activeDocumentPath"));
    active_path_->setWordWrap(true);

    active_id_ =
        new QLabel(document_properties_page_);
    active_id_->setObjectName(
        QStringLiteral("activeDocumentId"));
    active_id_->setWordWrap(true);

    document_properties_root->addWidget(active_path_);
    document_properties_root->addWidget(active_id_);

    auto* form = new QFormLayout;

    number_ =
        new QLineEdit(document_properties_page_);
    number_->setObjectName(
        QStringLiteral("documentNumberEdit"));

    title_ =
        new QLineEdit(document_properties_page_);
    title_->setObjectName(
        QStringLiteral("documentTitleEdit"));

    description_ =
        new QPlainTextEdit(document_properties_page_);
    description_->setObjectName(
        QStringLiteral("documentDescriptionEdit"));
    description_->setMaximumHeight(90);

    engineering_revision_ =
        new QLineEdit(document_properties_page_);
    engineering_revision_->setObjectName(
        QStringLiteral("documentEngineeringRevisionEdit"));

    length_unit_combo_ =
        new QComboBox(document_properties_page_);
    length_unit_combo_->setObjectName(
        QStringLiteral("partLengthUnitCombo"));
    length_unit_combo_->addItems(
        {
            QStringLiteral("mm"),
            QStringLiteral("cm"),
            QStringLiteral("m"),
            QStringLiteral("in"),
            QStringLiteral("ft"),
        });

    form->addRow(QStringLiteral("Number"), number_);
    form->addRow(QStringLiteral("Title"), title_);
    form->addRow(QStringLiteral("Description"), description_);
    form->addRow(
        QStringLiteral("Engineering revision"),
        engineering_revision_);
    form->addRow(
        QStringLiteral("Input/display unit"),
        length_unit_combo_);
    document_properties_root->addLayout(form);

    apply_button_ = new QPushButton(
        QStringLiteral("Apply Properties"),
        document_properties_page_);
    apply_button_->setObjectName(
        QStringLiteral("applyDocumentPropertiesButton"));
    document_properties_root->addWidget(apply_button_);
    document_properties_root->addStretch(1);

    properties_stack_->addWidget(
        document_properties_page_);

    reference_properties_page_ =
        new QWidget(properties_stack_);
    reference_properties_page_->setObjectName(
        QStringLiteral("referencePropertiesPage"));
    auto* reference_root =
        new QFormLayout(reference_properties_page_);

    reference_name_ =
        new QLabel(reference_properties_page_);
    reference_name_->setObjectName(
        QStringLiteral("referencePropertyName"));

    reference_kind_ =
        new QLabel(reference_properties_page_);
    reference_kind_->setObjectName(
        QStringLiteral("referencePropertyKind"));

    reference_identity_ =
        new QLabel(reference_properties_page_);
    reference_identity_->setObjectName(
        QStringLiteral("referencePropertyIdentity"));
    reference_identity_->setWordWrap(true);

    reference_visibility_ =
        new QLabel(reference_properties_page_);
    reference_visibility_->setObjectName(
        QStringLiteral("referencePropertyVisibility"));

    reference_root->addRow(
        QStringLiteral("Reference"),
        reference_name_);
    reference_root->addRow(
        QStringLiteral("Type"),
        reference_kind_);
    reference_root->addRow(
        QStringLiteral("Identity"),
        reference_identity_);
    reference_root->addRow(
        QStringLiteral("Visibility"),
        reference_visibility_);

    properties_stack_->addWidget(
        reference_properties_page_);

    profile_properties_page_ =
        new QWidget(properties_stack_);
    profile_properties_page_->setObjectName(
        QStringLiteral("profilePropertiesPage"));
    auto* profile_root =
        new QFormLayout(profile_properties_page_);

    profile_name_ =
        new QLineEdit(profile_properties_page_);
    profile_name_->setObjectName(
        QStringLiteral("profilePropertyName"));

    profile_identity_ =
        new QLabel(profile_properties_page_);
    profile_identity_->setObjectName(
        QStringLiteral("profilePropertyIdentity"));

    profile_source_ =
        new QLabel(profile_properties_page_);
    profile_source_->setObjectName(
        QStringLiteral("profilePropertySourceSketch"));
    profile_source_->setWordWrap(true);

    profile_status_ =
        new QLabel(profile_properties_page_);
    profile_status_->setObjectName(
        QStringLiteral("profilePropertyStatus"));

    profile_diagnostic_ =
        new QLabel(profile_properties_page_);
    profile_diagnostic_->setObjectName(
        QStringLiteral("profilePropertyDiagnostic"));
    profile_diagnostic_->setWordWrap(true);

    profile_area_ =
        new QLabel(profile_properties_page_);
    profile_area_->setObjectName(
        QStringLiteral("profilePropertyArea"));
    profile_perimeter_ =
        new QLabel(profile_properties_page_);
    profile_perimeter_->setObjectName(
        QStringLiteral("profilePropertyPerimeter"));
    profile_holes_ =
        new QLabel(profile_properties_page_);
    profile_holes_->setObjectName(
        QStringLiteral("profilePropertyHoles"));

    profile_visible_ =
        new QCheckBox(
            QStringLiteral("Visible"),
            profile_properties_page_);
    profile_visible_->setObjectName(
        QStringLiteral("profilePropertyVisible"));

    apply_profile_button_ =
        new QPushButton(
            QStringLiteral("Apply Profile Properties"),
            profile_properties_page_);
    apply_profile_button_->setObjectName(
        QStringLiteral("applyProfilePropertiesButton"));

    delete_profile_button_ =
        new QPushButton(
            QStringLiteral("Delete Profile"),
            profile_properties_page_);
    delete_profile_button_->setObjectName(
        QStringLiteral("deleteProfileButton"));

    profile_root->addRow(
        QStringLiteral("Name"),
        profile_name_);
    profile_root->addRow(
        QStringLiteral("ProfileId"),
        profile_identity_);
    profile_root->addRow(
        QStringLiteral("Source Sketch"),
        profile_source_);
    profile_root->addRow(
        QStringLiteral("Status"),
        profile_status_);
    profile_root->addRow(
        QStringLiteral("Diagnostic"),
        profile_diagnostic_);
    profile_root->addRow(
        QStringLiteral("Area"),
        profile_area_);
    profile_root->addRow(
        QStringLiteral("Perimeter"),
        profile_perimeter_);
    profile_root->addRow(
        QStringLiteral("Holes"),
        profile_holes_);
    profile_root->addRow(
        QStringLiteral("Visibility"),
        profile_visible_);
    profile_root->addRow(
        apply_profile_button_);
    profile_root->addRow(
        delete_profile_button_);

    properties_stack_->addWidget(
        profile_properties_page_);
    properties_stack_->setCurrentWidget(
        document_properties_page_);

    shell_->setPropertiesContent(properties_content);

    auto* operations_content = new QWidget(shell_);
    operations_content->setObjectName(
        QStringLiteral("partOperationsContent"));
    auto* operations_layout = new QVBoxLayout(operations_content);
    operations_layout->setContentsMargins(0, 0, 0, 0);

    operations_placeholder_ = new QLabel(
        QStringLiteral("Part modeling context."),
        operations_content);
    operations_placeholder_->setObjectName(
        QStringLiteral("operationsPlaceholder"));
    operations_placeholder_->setWordWrap(true);
    operations_layout->addWidget(operations_placeholder_);

    precision_operations_widget_ =
        new QWidget(operations_content);
    precision_operations_widget_->setObjectName(
        QStringLiteral("precisionOperationsWidget"));
    auto* precision_layout =
        new QVBoxLayout(
            precision_operations_widget_);
    precision_layout->setContentsMargins(
        0, 0, 0, 0);

    precision_status_label_ =
        new QLabel(
            precision_operations_widget_);
    precision_status_label_->setObjectName(
        QStringLiteral("precisionCadAidStatus"));
    precision_status_label_->setWordWrap(true);
    precision_layout->addWidget(
        precision_status_label_);

    auto* precision_form =
        new QFormLayout;
    polar_toggle_button_ =
        new QPushButton(
            precision_operations_widget_);
    polar_toggle_button_->setObjectName(
        QStringLiteral("polarToggleButton"));
    polar_toggle_button_->setCheckable(true);
    precision_form->addRow(
        QStringLiteral("Polar"),
        polar_toggle_button_);

    polar_step_edit_ =
        new QLineEdit(
            precision_operations_widget_);
    polar_step_edit_->setObjectName(
        QStringLiteral("polarStepEdit"));
    polar_step_edit_->setPlaceholderText(
        QStringLiteral("e.g. 360/8"));
    precision_form->addRow(
        QStringLiteral("Step"),
        polar_step_edit_);

    polar_reference_combo_ =
        new QComboBox(
            precision_operations_widget_);
    polar_reference_combo_->setObjectName(
        QStringLiteral("polarReferenceCombo"));
    polar_reference_combo_->addItems(
        {
            QStringLiteral("Absolute"),
            QStringLiteral("Relative"),
        });
    precision_form->addRow(
        QStringLiteral("Reference"),
        polar_reference_combo_);

    auto* additional_row =
        new QWidget(
            precision_operations_widget_);
    auto* additional_layout =
        new QHBoxLayout(additional_row);
    additional_layout->setContentsMargins(
        0, 0, 0, 0);
    polar_additional_edit_ =
        new QLineEdit(additional_row);
    polar_additional_edit_->setObjectName(
        QStringLiteral("polarAdditionalAngleEdit"));
    polar_additional_edit_->setPlaceholderText(
        QStringLiteral("e.g. 17 or 30deg"));
    polar_additional_add_button_ =
        new QPushButton(
            QStringLiteral("Add"),
            additional_row);
    polar_additional_add_button_->setObjectName(
        QStringLiteral("polarAdditionalAngleAddButton"));
    additional_layout->addWidget(
        polar_additional_edit_,
        1);
    additional_layout->addWidget(
        polar_additional_add_button_);
    precision_form->addRow(
        QStringLiteral("Additional"),
        additional_row);

    polar_additional_label_ =
        new QLabel(
            precision_operations_widget_);
    polar_additional_label_->setObjectName(
        QStringLiteral("polarAdditionalAnglesLabel"));
    polar_additional_label_->setWordWrap(true);
    polar_additional_clear_button_ =
        new QPushButton(
            QStringLiteral("Clear"),
            precision_operations_widget_);
    polar_additional_clear_button_->setObjectName(
        QStringLiteral("polarAdditionalAnglesClearButton"));
    precision_form->addRow(
        polar_additional_label_,
        polar_additional_clear_button_);

    dynamic_input_toggle_button_ =
        new QPushButton(
            precision_operations_widget_);
    dynamic_input_toggle_button_->setObjectName(
        QStringLiteral("dynamicInputToggleButton"));
    dynamic_input_toggle_button_->setCheckable(true);
    precision_form->addRow(
        QStringLiteral("Dynamic Input"),
        dynamic_input_toggle_button_);

    circle_size_mode_combo_ =
        new QComboBox(
            precision_operations_widget_);
    circle_size_mode_combo_->setObjectName(
        QStringLiteral("circleSizeModeCombo"));
    circle_size_mode_combo_->addItems(
        {
            QStringLiteral("Diameter"),
            QStringLiteral("Radius"),
        });
    circle_size_mode_label_ =
        new QLabel(
            QStringLiteral("Circle input"),
            precision_operations_widget_);
    precision_form->addRow(
        circle_size_mode_label_,
        circle_size_mode_combo_);

    precision_layout->addLayout(
        precision_form);
    operations_layout->addWidget(
        precision_operations_widget_);

    create_construction_button_ =
        new QPushButton(
            QStringLiteral("Construction"),
            operations_content);
    create_construction_button_->setObjectName(
        QStringLiteral("sketchCreationConstructionButton"));
    create_construction_button_->setCheckable(true);
    create_construction_button_->setVisible(false);
    create_construction_button_->setToolTip(
        QStringLiteral(
            "Creation role for new Line, Circle, Arc and Rectangle geometry."));
    operations_layout->addWidget(
        create_construction_button_);

    rectangle_diagonals_button_ =
        new QPushButton(
            QStringLiteral("Draw Diagonals"),
            operations_content);
    rectangle_diagonals_button_->setObjectName(
        QStringLiteral("sketchRectangleDiagonalsButton"));
    rectangle_diagonals_button_->setCheckable(true);
    rectangle_diagonals_button_->setVisible(false);
    rectangle_diagonals_button_->setToolTip(
        QStringLiteral(
            "Rectangle only: add two Construction diagonals in the same creation step."));
    operations_layout->addWidget(
        rectangle_diagonals_button_);

    measure_between_button_ =
        new QPushButton(
            QStringLiteral("Between"),
            operations_content);
    measure_between_button_->setObjectName(
        QStringLiteral("measureBetweenButton"));
    measure_between_button_->setCheckable(true);
    measure_between_button_->setVisible(false);
    operations_layout->addWidget(
        measure_between_button_);

    cancel_sketch_button_ =
        new QPushButton(
            QStringLiteral("Cancel"),
            operations_content);
    cancel_sketch_button_->setObjectName(
        QStringLiteral("cancelSketchButton"));
    operations_layout->addWidget(
        cancel_sketch_button_);

    delete_selection_button_ =
        new QPushButton(
            QStringLiteral("Delete Selection"),
            operations_content);
    delete_selection_button_->setObjectName(
        QStringLiteral("deleteSketchSelectionButton"));
    operations_layout->addWidget(
        delete_selection_button_);

    entity_role_label_ =
        new QLabel(
            QStringLiteral("Selected geometry role:"),
            operations_content);
    entity_role_label_->setObjectName(
        QStringLiteral("sketchEntityRoleLabel"));
    entity_role_label_->setVisible(false);
    operations_layout->addWidget(
        entity_role_label_);

    regular_role_button_ =
        new QPushButton(
            QStringLiteral("Regular"),
            operations_content);
    regular_role_button_->setObjectName(
        QStringLiteral("sketchRegularRoleButton"));
    regular_role_button_->setCheckable(true);
    regular_role_button_->setVisible(false);
    operations_layout->addWidget(
        regular_role_button_);

    construction_role_button_ =
        new QPushButton(
            QStringLiteral("Construction"),
            operations_content);
    construction_role_button_->setObjectName(
        QStringLiteral("sketchConstructionRoleButton"));
    construction_role_button_->setCheckable(true);
    construction_role_button_->setVisible(false);
    operations_layout->addWidget(
        construction_role_button_);

    finish_line_button_ =
        new QPushButton(
            QStringLiteral("Finish Line"),
            operations_content);
    finish_line_button_->setObjectName(
        QStringLiteral("finishSketchLineButton"));
    operations_layout->addWidget(
        finish_line_button_);

    cancel_line_button_ =
        new QPushButton(
            QStringLiteral("Cancel Line"),
            operations_content);
    cancel_line_button_->setObjectName(
        QStringLiteral("cancelSketchLineButton"));
    operations_layout->addWidget(
        cancel_line_button_);

    profile_operations_widget_ =
        new QWidget(operations_content);
    profile_operations_widget_->setObjectName(
        QStringLiteral("profileOperationsWidget"));
    auto* profile_operations_layout =
        new QVBoxLayout(profile_operations_widget_);
    profile_operations_layout->setContentsMargins(
        0, 0, 0, 0);

    profile_add_area_button_ =
        new QPushButton(
            QStringLiteral("Add Area"),
            profile_operations_widget_);
    profile_add_area_button_->setObjectName(
        QStringLiteral("profileAddAreaButton"));
    profile_add_area_button_->setCheckable(true);
    profile_operations_layout->addWidget(
        profile_add_area_button_);

    profile_subtract_area_button_ =
        new QPushButton(
            QStringLiteral("Subtract Area"),
            profile_operations_widget_);
    profile_subtract_area_button_->setObjectName(
        QStringLiteral("profileSubtractAreaButton"));
    profile_subtract_area_button_->setCheckable(true);
    profile_operations_layout->addWidget(
        profile_subtract_area_button_);

    profile_detect_islands_button_ =
        new QPushButton(
            QStringLiteral("Detect Islands"),
            profile_operations_widget_);
    profile_detect_islands_button_->setObjectName(
        QStringLiteral("profileDetectIslandsButton"));
    profile_detect_islands_button_->setCheckable(true);
    profile_operations_layout->addWidget(
        profile_detect_islands_button_);

    profile_highlight_hover_button_ =
        new QPushButton(
            QStringLiteral("Highlight on Hover"),
            profile_operations_widget_);
    profile_highlight_hover_button_->setObjectName(
        QStringLiteral("profileHighlightHoverButton"));
    profile_highlight_hover_button_->setCheckable(true);
    profile_operations_layout->addWidget(
        profile_highlight_hover_button_);

    profile_show_boundaries_button_ =
        new QPushButton(
            QStringLiteral("Show Region Boundaries"),
            profile_operations_widget_);
    profile_show_boundaries_button_->setObjectName(
        QStringLiteral("profileShowBoundariesButton"));
    profile_show_boundaries_button_->setCheckable(true);
    profile_operations_layout->addWidget(
        profile_show_boundaries_button_);

    profile_show_problems_button_ =
        new QPushButton(
            QStringLiteral("Show Problems"),
            profile_operations_widget_);
    profile_show_problems_button_->setObjectName(
        QStringLiteral("profileShowProblemsButton"));
    profile_show_problems_button_->setCheckable(true);
    profile_operations_layout->addWidget(
        profile_show_problems_button_);

    profile_result_label_ =
        new QLabel(
            QStringLiteral("Current result: —"),
            profile_operations_widget_);
    profile_result_label_->setObjectName(
        QStringLiteral("profileCurrentResult"));
    profile_result_label_->setWordWrap(true);
    profile_operations_layout->addWidget(
        profile_result_label_);

    profile_find_regions_button_ =
        new QPushButton(
            QStringLiteral("Find All Regions"),
            profile_operations_widget_);
    profile_find_regions_button_->setObjectName(
        QStringLiteral("profileFindRegionsButton"));
    profile_operations_layout->addWidget(
        profile_find_regions_button_);

    profile_finish_button_ =
        new QPushButton(
            QStringLiteral("Finish Profile"),
            profile_operations_widget_);
    profile_finish_button_->setObjectName(
        QStringLiteral("profileFinishButton"));
    profile_operations_layout->addWidget(
        profile_finish_button_);

    profile_cancel_button_ =
        new QPushButton(
            QStringLiteral("Cancel"),
            profile_operations_widget_);
    profile_cancel_button_->setObjectName(
        QStringLiteral("profileCancelButton"));
    profile_operations_layout->addWidget(
        profile_cancel_button_);

    operations_layout->addWidget(
        profile_operations_widget_);

    operations_layout->addStretch(1);

    finish_sketch_button_ =
        new QPushButton(
            QStringLiteral("Finish Sketch"),
            operations_content);
    finish_sketch_button_->setObjectName(
        QStringLiteral("finishSketchButton"));
    operations_layout->addWidget(
        finish_sketch_button_);

    shell_->setOperationsContent(operations_content);

    QObject::connect(
        undo_button_,
        &QPushButton::clicked,
        this,
        [this] { undo(); });
    QObject::connect(
        redo_button_,
        &QPushButton::clicked,
        this,
        [this] { redo(); });
    QObject::connect(
        save_button_,
        &QPushButton::clicked,
        this,
        [this] { save(); });
    QObject::connect(
        close_document_button_,
        &QPushButton::clicked,
        this,
        [this] { closeActiveDocument(); });
    QObject::connect(
        apply_button_,
        &QPushButton::clicked,
        this,
        [this] { applyProperties(); });
    QObject::connect(
        length_unit_combo_,
        &QComboBox::currentIndexChanged,
        this,
        [this](int index) {
            if (syncing_precision_ui_) {
                return;
            }
            const auto unit =
                lengthUnitForIndex(index);
            if (unit) {
                setPartLengthUnit(*unit);
            }
        });
    QObject::connect(
        polar_toggle_button_,
        &QPushButton::clicked,
        this,
        [this](bool checked) {
            if (syncing_precision_ui_ ||
                !cad_interaction_settings_provider_ ||
                !cad_interaction_settings_updater_) {
                return;
            }
            auto settings =
                cad_interaction_settings_provider_();
            settings.polar.enabled = checked;
            if (!cad_interaction_settings_updater_(
                    std::move(settings))) {
                refreshCadInteractionSettingsUi();
            }
        });
    QObject::connect(
        polar_step_edit_,
        &QLineEdit::editingFinished,
        this,
        [this] {
            if (syncing_precision_ui_ ||
                !cad_interaction_settings_provider_ ||
                !cad_interaction_settings_updater_) {
                return;
            }

            const auto parsed =
                application::parseCadQuantity(
                    toUtf8(
                        polar_step_edit_->text()),
                    {
                        application::
                            CadQuantityDimension::angle,
                        core::LengthUnit::millimetre,
                    });
            if (!parsed ||
                parsed->canonical_value <= 0.0 ||
                parsed->canonical_value >
                    std::numbers::pi_v<double>) {
                setStatusText(
                    QStringLiteral(
                        "Polar Step must be a finite Angle greater than 0° and no greater than 180°."));
                const auto current =
                    cad_interaction_settings_provider_();
                syncing_precision_ui_ = true;
                polar_step_edit_->setText(
                    QString::number(
                        current.polar.primary_spacing *
                            180.0 /
                            std::numbers::pi_v<double>,
                        'g',
                        12));
                syncing_precision_ui_ = false;
                return;
            }

            auto settings =
                cad_interaction_settings_provider_();
            settings.polar.primary_spacing =
                parsed->canonical_value;
            if (!settings.valid() ||
                !cad_interaction_settings_updater_(
                    std::move(settings))) {
                refreshCadInteractionSettingsUi();
            }
        });
    QObject::connect(
        polar_reference_combo_,
        &QComboBox::currentIndexChanged,
        this,
        [this](int index) {
            if (syncing_precision_ui_ ||
                !cad_interaction_settings_provider_ ||
                !cad_interaction_settings_updater_) {
                return;
            }
            auto settings =
                cad_interaction_settings_provider_();
            settings.polar.reference_mode =
                index == 1
                    ? application::
                          PolarReferenceMode::relative
                    : application::
                          PolarReferenceMode::absolute;
            if (!cad_interaction_settings_updater_(
                    std::move(settings))) {
                refreshCadInteractionSettingsUi();
            }
        });
    QObject::connect(
        polar_additional_add_button_,
        &QPushButton::clicked,
        this,
        [this] {
            if (syncing_precision_ui_ ||
                !cad_interaction_settings_provider_ ||
                !cad_interaction_settings_updater_) {
                return;
            }

            const auto parsed =
                application::parseCadQuantity(
                    toUtf8(
                        polar_additional_edit_->text()),
                    {
                        application::
                            CadQuantityDimension::angle,
                        core::LengthUnit::millimetre,
                    });
            if (!parsed) {
                setStatusText(
                    QStringLiteral(
                        "Additional Polar Angle must be a finite Angle expression."));
                return;
            }

            auto settings =
                cad_interaction_settings_provider_();
            const double normalized =
                normalizePolarAngle(
                    parsed->canonical_value);

            auto primary_only = settings.polar;
            primary_only.enabled = true;
            primary_only.additional_angles.clear();
            const auto primary_tracks =
                application::generatePolarTrackAngles(
                    primary_only);

            const bool primary_duplicate =
                std::any_of(
                    primary_tracks.begin(),
                    primary_tracks.end(),
                    [normalized](double angle) {
                        return equivalentPolarAngle(
                            angle,
                            normalized);
                    });
            const bool additional_duplicate =
                std::any_of(
                    settings.polar
                        .additional_angles.begin(),
                    settings.polar
                        .additional_angles.end(),
                    [normalized](double angle) {
                        return equivalentPolarAngle(
                            angle,
                            normalized);
                    });

            polar_additional_edit_->clear();
            if (primary_duplicate ||
                additional_duplicate) {
                setStatusText(
                    QStringLiteral(
                        "Additional Polar Angle is already covered by an existing track."));
                refreshCadInteractionSettingsUi();
                return;
            }

            settings.polar.additional_angles.push_back(
                normalized);
            if (!cad_interaction_settings_updater_(
                    std::move(settings))) {
                refreshCadInteractionSettingsUi();
            }
        });
    QObject::connect(
        polar_additional_clear_button_,
        &QPushButton::clicked,
        this,
        [this] {
            if (syncing_precision_ui_ ||
                !cad_interaction_settings_provider_ ||
                !cad_interaction_settings_updater_) {
                return;
            }
            auto settings =
                cad_interaction_settings_provider_();
            settings.polar.additional_angles.clear();
            if (!cad_interaction_settings_updater_(
                    std::move(settings))) {
                refreshCadInteractionSettingsUi();
            }
        });
    QObject::connect(
        dynamic_input_toggle_button_,
        &QPushButton::clicked,
        this,
        [this](bool checked) {
            if (syncing_precision_ui_ ||
                !cad_interaction_settings_provider_ ||
                !cad_interaction_settings_updater_) {
                return;
            }
            auto settings =
                cad_interaction_settings_provider_();
            settings.dynamic_input_enabled =
                checked;
            if (!cad_interaction_settings_updater_(
                    std::move(settings))) {
                refreshCadInteractionSettingsUi();
            }
        });
    QObject::connect(
        circle_size_mode_combo_,
        &QComboBox::currentIndexChanged,
        this,
        [this](int index) {
            if (syncing_precision_ui_ ||
                !sketch_interaction_controller_) {
                return;
            }
            const auto mode =
                index == 1
                    ? application::
                          CircleSizeInputMode::radius
                    : application::
                          CircleSizeInputMode::diameter;
            if (!sketch_interaction_controller_->
                    submitCadInputSemanticCircleSizeMode(
                        mode)) {
                refreshCadInteractionSettingsUi();
            }
        });
    QObject::connect(
        apply_profile_button_,
        &QPushButton::clicked,
        this,
        [this] { applyProfileProperties(); });
    QObject::connect(
        delete_profile_button_,
        &QPushButton::clicked,
        this,
        [this] { deleteSelectedProfile(); });
    QObject::connect(
        sketch_button_,
        &QPushButton::clicked,
        this,
        [this] { startSketchTool(); });
    QObject::connect(
        select_sketch_button_,
        &QPushButton::clicked,
        this,
        [this] { activateSketchSelect(); });
    QObject::connect(
        line_sketch_button_,
        &QPushButton::clicked,
        this,
        [this] { activateSketchLine(); });
    QObject::connect(
        circle_sketch_button_,
        &QPushButton::clicked,
        this,
        [this] { activateSketchCircle(); });
    QObject::connect(
        arc_sketch_button_,
        &QPushButton::clicked,
        this,
        [this] { activateSketchArc(); });
    QObject::connect(
        rectangle_sketch_button_,
        &QPushButton::clicked,
        this,
        [this] { activateSketchRectangle(); });
    QObject::connect(
        create_construction_button_,
        &QPushButton::clicked,
        this,
        [this](bool checked) {
            if (sketch_interaction_controller_) {
                static_cast<void>(
                    sketch_interaction_controller_->
                        setCreationRole(
                            checked
                                ? sketch::EntityRole::construction
                                : sketch::EntityRole::regular));
            }
        });
    QObject::connect(
        rectangle_diagonals_button_,
        &QPushButton::clicked,
        this,
        [this](bool checked) {
            if (sketch_interaction_controller_) {
                static_cast<void>(
                    sketch_interaction_controller_->
                        setRectangleDrawDiagonals(
                            checked));
            }
        });
    QObject::connect(
        profile_sketch_button_,
        &QPushButton::clicked,
        this,
        [this] {
            if (sketch_interaction_controller_ &&
                !sketch_interaction_controller_->
                     activateProfileCreate()) {
                setStatusText(
                    QStringLiteral(
                        "PROFILE could not be activated."));
                return;
            }
            if (viewport_widget_ != nullptr) {
                viewport_widget_->setFocus(
                    Qt::OtherFocusReason);
            }
        });
    QObject::connect(
        move_sketch_button_,
        &QPushButton::clicked,
        this,
        [this] { activateSketchMove(); });
    QObject::connect(
        copy_sketch_button_,
        &QPushButton::clicked,
        this,
        [this] { activateSketchCopy(); });
    QObject::connect(
        rotate_sketch_button_,
        &QPushButton::clicked,
        this,
        [this] { activateSketchRotate(); });
    QObject::connect(
        scale_sketch_button_,
        &QPushButton::clicked,
        this,
        [this] { activateSketchScale(); });
    QObject::connect(
        mirror_sketch_button_,
        &QPushButton::clicked,
        this,
        [this] { activateSketchMirror(); });
    QObject::connect(
        measure_sketch_button_,
        &QPushButton::clicked,
        this,
        [this] { activateSketchMeasure(); });
    QObject::connect(
        measure_between_button_,
        &QPushButton::clicked,
        this,
        [this] { activateSketchMeasureBetween(); });
    QObject::connect(
        cancel_sketch_button_,
        &QPushButton::clicked,
        this,
        [this] { cancelSketchTool(); });
    QObject::connect(
        finish_sketch_button_,
        &QPushButton::clicked,
        this,
        [this] { finishSketch(); });
    QObject::connect(
        finish_line_button_,
        &QPushButton::clicked,
        this,
        [this] { finishSketchLine(); });
    QObject::connect(
        cancel_line_button_,
        &QPushButton::clicked,
        this,
        [this] { cancelSketchLine(); });
    QObject::connect(
        delete_selection_button_,
        &QPushButton::clicked,
        this,
        [this] { deleteSketchSelection(); });
    QObject::connect(
        regular_role_button_,
        &QPushButton::clicked,
        this,
        [this] {
            setSketchSelectionRole(
                sketch::EntityRole::regular);
        });
    QObject::connect(
        construction_role_button_,
        &QPushButton::clicked,
        this,
        [this] {
            setSketchSelectionRole(
                sketch::EntityRole::construction);
        });

    QObject::connect(
        profile_add_area_button_,
        &QPushButton::clicked,
        this,
        [this] {
            if (sketch_interaction_controller_) {
                static_cast<void>(
                    sketch_interaction_controller_->
                        setProfileAreaMode(
                            part::ProfileAreaEditMode::
                                add_area));
            }
        });
    QObject::connect(
        profile_subtract_area_button_,
        &QPushButton::clicked,
        this,
        [this] {
            if (sketch_interaction_controller_) {
                static_cast<void>(
                    sketch_interaction_controller_->
                        setProfileAreaMode(
                            part::ProfileAreaEditMode::
                                subtract_area));
            }
        });

    const auto update_profile_options =
        [this] {
            if (!sketch_interaction_controller_ ||
                !sketch_interaction_controller_->
                     profileToolActive()) {
                return;
            }
            auto options =
                sketch_interaction_controller_->
                    profileToolOptions();
            options.detect_islands =
                profile_detect_islands_button_->
                    isChecked();
            options.highlight_on_hover =
                profile_highlight_hover_button_->
                    isChecked();
            options.show_region_boundaries =
                profile_show_boundaries_button_->
                    isChecked();
            options.show_problems =
                profile_show_problems_button_->
                    isChecked();
            static_cast<void>(
                sketch_interaction_controller_->
                    setProfileToolOptions(options));
        };

    for (auto* button : {
             profile_detect_islands_button_,
             profile_highlight_hover_button_,
             profile_show_boundaries_button_,
             profile_show_problems_button_}) {
        QObject::connect(
            button,
            &QPushButton::clicked,
            this,
            update_profile_options);
    }

    QObject::connect(
        profile_find_regions_button_,
        &QPushButton::clicked,
        this,
        [this] {
            if (!sketch_interaction_controller_) {
                return;
            }
            const auto result =
                sketch_interaction_controller_->
                    submitCadInputSemanticProfileCommand(
                        application::
                            ProfileCadInputCommand{
                                application::
                                    ProfileCadInputCommandKind::
                                        find_all_regions,
                                std::nullopt});
            if (!result.accepted &&
                !result.diagnostic.empty()) {
                setStatusText(
                    fromUtf8(result.diagnostic));
            }
        });
    QObject::connect(
        profile_finish_button_,
        &QPushButton::clicked,
        this,
        [this] {
            if (sketch_interaction_controller_) {
                static_cast<void>(
                    sketch_interaction_controller_->
                        finishProfile());
            }
        });
    QObject::connect(
        profile_cancel_button_,
        &QPushButton::clicked,
        this,
        [this] {
            if (sketch_interaction_controller_) {
                sketch_interaction_controller_->
                    cancelProfile();
            }
        });

    syncSketchInteractionUi();
}

bool CadWorkbench::activateDocument(
    application::DocumentSession* session,
    std::filesystem::path workspace_root) {
    if (session == nullptr) {
        deactivateDocument();
        return false;
    }

    if (document_session_ == session) {
        workspace_root_ = std::move(workspace_root);
        refreshActiveContext();
        setStatusText(
            QStringLiteral("Part Document activated."));
        return true;
    }

    captureActiveViewState();
    clearSketchRuntimeContext();
    if (viewport_controller_ != nullptr) {
        viewport_controller_->clear();
    }

    document_session_ = session;
    workspace_root_ = std::move(workspace_root);

    restoreActiveViewState();
    refreshActiveContext();
    setStatusText(
        QStringLiteral("Part Document activated."));
    return true;
}

void CadWorkbench::deactivateDocument() {
    if (document_session_ != nullptr) {
        captureActiveViewState();
    }

    clearSketchRuntimeContext();
    document_session_ = nullptr;
    workspace_root_.clear();

    clearActiveContext();
    setStatusText(
        QStringLiteral("No Part Document is active."));
}

void CadWorkbench::resetRuntimeState() {
    deactivateDocument();
    document_view_states_.clear();
    setStatusText(
        QStringLiteral(
            "No Part Document is active."));
}

void CadWorkbench::forgetDocumentRuntimeState(
    const core::DocumentId& document_id) {
    if (document_session_ != nullptr &&
        document_session_->documentId() ==
            document_id) {
        deactivateDocument();
    }

    document_view_states_.erase(
        std::string{document_id.value()});
}

void CadWorkbench::applyProperties() {
    auto* document_session = activeDocumentSession();
    if (document_session == nullptr) return;

    core::DocumentProperties properties;
    properties.number = toUtf8(number_->text());
    properties.title = toUtf8(title_->text());
    properties.description =
        toUtf8(description_->toPlainText());
    properties.engineering_revision =
        toUtf8(engineering_revision_->text());

    const auto result = document_session->execute(
        application::SetDocumentPropertiesCommand{
            std::move(properties)});
    if (!result.ok()) {
        showFailure(result.diagnostic);
        refreshActiveContext();
        return;
    }

    refreshActiveContext();
    setStatusText(
        result.changed
            ? QStringLiteral(
                  "Properties changed — save is required.")
            : QStringLiteral("No authored property change."));
}

void CadWorkbench::setPartLengthUnit(
    core::LengthUnit unit) {
    auto* document_session =
        activeDocumentSession();
    if (document_session == nullptr ||
        document_session->document().lengthUnit() ==
            unit) {
        return;
    }

    const auto result =
        document_session->execute(
            application::SetPartLengthUnitCommand{
                unit});
    if (!result.ok()) {
        showFailure(result.diagnostic);
        refreshActiveContext();
        return;
    }

    refreshActiveContext();
    setStatusText(
        result.changed
            ? QStringLiteral(
                  "Part input/display unit changed — save is required.")
            : QStringLiteral(
                  "No Part unit change."));
}

void CadWorkbench::applyProfileProperties() {
    auto* document_session = activeDocumentSession();
    if (document_session == nullptr ||
        !selected_profile_id_) {
        return;
    }

    const auto profile_id =
        *selected_profile_id_;
    if (document_session->document()
            .findProfile(profile_id) == nullptr) {
        selected_profile_id_.reset();
        properties_stack_->setCurrentWidget(
            document_properties_page_);
        return;
    }

    const auto result =
        document_session->execute(
            application::SetProfilePropertiesCommand{
                profile_id,
                document_session->document().revision(),
                toUtf8(profile_name_->text()),
                profile_visible_->isChecked()});
    if (!result.ok()) {
        showFailure(result.diagnostic);
        refreshProfileProperties(profile_id);
        return;
    }

    refreshActiveContext();
    refreshProfileProperties(profile_id);
    setStatusText(
        result.changed
            ? QStringLiteral(
                  "Profile properties changed — save is required.")
            : QStringLiteral(
                  "No authored Profile property change."));
}

void CadWorkbench::deleteSelectedProfile() {
    auto* document_session = activeDocumentSession();
    if (document_session == nullptr ||
        !selected_profile_id_ ||
        (sketch_interaction_controller_ &&
         sketch_interaction_controller_->
             profileToolActive())) {
        return;
    }

    const auto profile_id =
        *selected_profile_id_;
    const auto result =
        document_session->execute(
            application::DeleteProfileCommand{
                profile_id,
                document_session->document().revision()});
    if (!result.ok()) {
        showFailure(result.diagnostic);
        return;
    }
    if (!result.changed) {
        setStatusText(
            QStringLiteral("No Profile was deleted."));
        return;
    }

    selected_profile_id_.reset();
    refreshActiveContext();
    properties_stack_->setCurrentWidget(
        document_properties_page_);
    setStatusText(
        QStringLiteral(
            "Profile deleted — source Sketch geometry is unchanged."));
}

void CadWorkbench::setSketchSelectionRole(
    sketch::EntityRole role) {
    if (!sketch_interaction_controller_ ||
        !sketch_interaction_controller_->
             setSelectedEntityRole(role)) {
        return;
    }

    refreshActiveContext();
    setStatusText(
        role == sketch::EntityRole::construction
            ? QStringLiteral(
                  "Selected geometry marked Construction.")
            : QStringLiteral(
                  "Selected geometry marked Regular."));
}

void CadWorkbench::startSketchTool() {
    if (activeDocumentSession() == nullptr ||
        sketch_support_pick_active_) {
        return;
    }

    if (active_sketch_id_) {
        setStatusText(
            QStringLiteral(
                "Finish the active Sketch before creating another one."));
        return;
    }

    sketch_support_pick_active_ = true;
    operations_placeholder_->setText(
        QStringLiteral(
            "Sketch: select XY, XZ or YZ Origin plane in the Tree or 3D Viewport."));
    setStatusText(
        QStringLiteral(
            "Sketch tool active — select an Origin plane."));
    syncActionState();
}

void CadWorkbench::cancelSketchTool() {
    if (!sketch_support_pick_active_) {
        return;
    }

    clearSketchRuntimeContext();
    setStatusText(
        QStringLiteral(
            "Sketch creation cancelled."));
    syncActionState();
}

void CadWorkbench::requestEditSketch(
    const sketch::SketchId& sketch_id) {
    auto* document_session =
        activeDocumentSession();
    if (document_session == nullptr) {
        return;
    }

    if (active_sketch_id_) {
        if (*active_sketch_id_ == sketch_id) {
            setStatusText(
                QStringLiteral(
                    "This Sketch is already being edited."));
        } else {
            setStatusText(
                QStringLiteral(
                    "Finish the active Sketch before editing another one."));
        }
        return;
    }

    if (sketch_support_pick_active_) {
        clearSketchRuntimeContext();
    }

    if (document_session->document()
            .findSketch(sketch_id) == nullptr) {
        setStatusText(
            QStringLiteral(
                "Sketch is no longer available in the active Part."));
        syncActionState();
        return;
    }

    enterSketchEdit(sketch_id);
    setStatusText(
        QStringLiteral(
            "Sketch edit context opened in the 3D Viewport."));
}

void CadWorkbench::requestEditProfile(
    part::ProfileId profile_id) {
    auto* document_session =
        activeDocumentSession();
    if (document_session == nullptr) {
        return;
    }

    const auto* profile =
        document_session->document()
            .findProfile(profile_id);
    if (profile == nullptr) {
        setStatusText(
            QStringLiteral(
                "Profile is no longer available in the active Part."));
        return;
    }

    if (active_sketch_id_ &&
        *active_sketch_id_ !=
            profile->source_sketch_id) {
        setStatusText(
            QStringLiteral(
                "Finish the active Sketch before editing a Profile from another Sketch."));
        return;
    }

    if (!active_sketch_id_) {
        requestEditSketch(
            profile->source_sketch_id);
    }

    if (!active_sketch_id_ ||
        *active_sketch_id_ !=
            profile->source_sketch_id ||
        !sketch_interaction_controller_) {
        return;
    }

    sketch_interaction_controller_->
        setSelectedProfileForCadInput(profile_id);
    if (!sketch_interaction_controller_->
             activateProfileEdit(profile_id)) {
        setStatusText(
            QStringLiteral(
                "Profile edit could not be activated."));
        return;
    }

    if (viewport_widget_ != nullptr) {
        viewport_widget_->setFocus(
            Qt::OtherFocusReason);
    }
    setStatusText(
        QStringLiteral(
            "Profile edit context opened."));
}

void CadWorkbench::tryCreateSketchFromSupport(
    std::optional<core::BuiltinReferenceRole> support) {
    if (!sketch_support_pick_active_) {
        return;
    }

    if (!support) {
        return;
    }

    if (!part::isSketchOriginPlane(*support)) {
        setStatusText(
            QStringLiteral(
                "Sketch support must be XY, XZ or YZ Origin plane."));
        return;
    }

    auto* document_session =
        activeDocumentSession();
    if (document_session == nullptr) {
        clearSketchRuntimeContext();
        return;
    }

    const auto created =
        document_session->execute(
            application::CreatePartSketchCommand{
                *support});
    if (!created.ok()) {
        showFailure(created.diagnostic);
        return;
    }

    if (!created.changed ||
        !created.sketch_id) {
        setStatusText(
            QStringLiteral(
                "Sketch was not created."));
        return;
    }

    sketch_support_pick_active_ = false;

    const auto created_id =
        *created.sketch_id;

    refreshActiveContext();
    enterSketchEdit(created_id);

    setStatusText(
        QStringLiteral(
            "Sketch created — editing in the 3D Viewport. "
            "Pan/Zoom/Orbit remain available."));
}

void CadWorkbench::enterSketchEdit(
    const sketch::SketchId& sketch_id) {
    auto* document_session =
        activeDocumentSession();
    if (document_session == nullptr) {
        clearSketchRuntimeContext();
        return;
    }

    const auto* sketch =
        document_session->document()
            .findSketch(sketch_id);
    if (sketch == nullptr) {
        clearSketchRuntimeContext();
        return;
    }

    active_sketch_id_ = sketch_id;
    sketch_edit_document_id_ =
        document_session->documentId();

    viewport_controller_->setSketchEditSketch(
        sketch_id);

    if (sketch_interaction_controller_) {
        sketch_interaction_controller_->begin(
            *document_session,
            sketch_id);

        const auto selected_profiles =
            tree_controller_->selectedProfileIds();
        sketch_interaction_controller_->
            setSelectedProfileForCadInput(
                selected_profiles.size() == 1U
                    ? tree_controller_->
                          primaryProfileId()
                    : std::nullopt);
    }

    if (viewport_ != nullptr) {
        const auto standard_view =
            standardViewForSketchSupport(
                sketch->support.builtin_plane);
        if (standard_view) {
            static_cast<void>(
                viewport_->setStandardView(
                    *standard_view));
        }
        viewport_->fitAll();
    }

    syncSketchInteractionUi();
    syncActionState();
}

void CadWorkbench::activateSketchSelect() {
    if (sketch_interaction_controller_) {
        sketch_interaction_controller_->activateSelect();
    }
    if (viewport_widget_ != nullptr) {
        viewport_widget_->setFocus(Qt::OtherFocusReason);
    }
}

void CadWorkbench::activateSketchLine() {
    if (sketch_interaction_controller_) {
        sketch_interaction_controller_->activateLine();
    }
    if (viewport_widget_ != nullptr) {
        viewport_widget_->setFocus(Qt::OtherFocusReason);
    }
}

void CadWorkbench::activateSketchCircle() {
    if (sketch_interaction_controller_) {
        sketch_interaction_controller_->activateCircle();
    }
    if (viewport_widget_ != nullptr) {
        viewport_widget_->setFocus(Qt::OtherFocusReason);
    }
}

void CadWorkbench::activateSketchArc() {
    if (sketch_interaction_controller_) {
        sketch_interaction_controller_->activateArc();
    }
    if (viewport_widget_ != nullptr) {
        viewport_widget_->setFocus(Qt::OtherFocusReason);
    }
}

void CadWorkbench::activateSketchRectangle() {
    if (sketch_interaction_controller_) {
        sketch_interaction_controller_->
            activateRectangle();
    }
    if (viewport_widget_ != nullptr) {
        viewport_widget_->setFocus(
            Qt::OtherFocusReason);
    }
}

void CadWorkbench::activateSketchMove() {
    if (sketch_interaction_controller_ &&
        !sketch_interaction_controller_->activateMove()) {
        setStatusText(
            QStringLiteral("MOVE could not be activated."));
        return;
    }
    if (viewport_widget_ != nullptr) {
        viewport_widget_->setFocus(Qt::OtherFocusReason);
    }
}

void CadWorkbench::activateSketchCopy() {
    if (sketch_interaction_controller_ &&
        !sketch_interaction_controller_->activateCopy()) {
        setStatusText(
            QStringLiteral("COPY could not be activated."));
        return;
    }
    if (viewport_widget_ != nullptr) {
        viewport_widget_->setFocus(Qt::OtherFocusReason);
    }
}

void CadWorkbench::activateSketchRotate() {
    if (sketch_interaction_controller_ &&
        !sketch_interaction_controller_->activateRotate()) {
        setStatusText(
            QStringLiteral("ROTATE could not be activated."));
        return;
    }
    if (viewport_widget_ != nullptr) {
        viewport_widget_->setFocus(Qt::OtherFocusReason);
    }
}

void CadWorkbench::activateSketchScale() {
    if (sketch_interaction_controller_ &&
        !sketch_interaction_controller_->activateScale()) {
        setStatusText(
            QStringLiteral("SCALE could not be activated."));
        return;
    }
    if (viewport_widget_ != nullptr) {
        viewport_widget_->setFocus(Qt::OtherFocusReason);
    }
}

void CadWorkbench::activateSketchMirror() {
    if (sketch_interaction_controller_ &&
        !sketch_interaction_controller_->activateMirror()) {
        setStatusText(
            QStringLiteral("MIRROR could not be activated."));
        return;
    }
    if (viewport_widget_ != nullptr) {
        viewport_widget_->setFocus(Qt::OtherFocusReason);
    }
}

void CadWorkbench::activateSketchMeasure() {
    if (sketch_interaction_controller_ &&
        !sketch_interaction_controller_->activateMeasure()) {
        setStatusText(
            QStringLiteral("MEASURE could not be activated."));
        return;
    }
    if (viewport_widget_ != nullptr) {
        viewport_widget_->setFocus(Qt::OtherFocusReason);
    }
}

void CadWorkbench::activateSketchMeasureBetween() {
    if (!sketch_interaction_controller_ ||
        !sketch_interaction_controller_->
            activateMeasureBetween()) {
        setStatusText(
            QStringLiteral(
                "Measure Between could not be activated."));
        return;
    }
    if (viewport_widget_ != nullptr) {
        viewport_widget_->setFocus(
            Qt::OtherFocusReason);
    }
}

void CadWorkbench::finishSketchLine() {
    if (!sketch_interaction_controller_) {
        return;
    }

    const auto tool =
        sketch_interaction_controller_->tool();
    sketch_interaction_controller_->activateSelect();

    switch (tool) {
    case sketch::SketchTool::line:
        setStatusText(
            QStringLiteral("Line finished — Select active."));
        break;
    case sketch::SketchTool::circle:
        setStatusText(
            QStringLiteral("Circle finished — Select active."));
        break;
    case sketch::SketchTool::arc:
        setStatusText(
            QStringLiteral("Arc finished — Select active."));
        break;
    case sketch::SketchTool::rectangle:
        setStatusText(
            QStringLiteral("Rectangle finished — Select active."));
        break;
    case sketch::SketchTool::move:
        setStatusText(
            QStringLiteral("Move finished — Select active."));
        break;
    case sketch::SketchTool::copy:
        setStatusText(
            QStringLiteral("Copy finished — Select active."));
        break;
    case sketch::SketchTool::rotate:
        setStatusText(
            QStringLiteral("Rotate finished — Select active."));
        break;
    case sketch::SketchTool::scale:
        setStatusText(
            QStringLiteral("Scale finished — Select active."));
        break;
    case sketch::SketchTool::mirror:
        setStatusText(
            QStringLiteral("Mirror finished — Select active."));
        break;
    case sketch::SketchTool::measure:
        setStatusText(
            QStringLiteral("Measure finished — Select active."));
        break;
    case sketch::SketchTool::select:
        break;
    }
}

void CadWorkbench::cancelSketchLine() {
    if (!sketch_interaction_controller_) {
        return;
    }

    const auto tool =
        sketch_interaction_controller_->tool();
    sketch_interaction_controller_->activateSelect();

    switch (tool) {
    case sketch::SketchTool::line:
        setStatusText(
            QStringLiteral(
                "Line cancelled — committed segments preserved."));
        break;
    case sketch::SketchTool::circle:
        setStatusText(
            QStringLiteral(
                "Circle cancelled — committed circles preserved."));
        break;
    case sketch::SketchTool::arc:
        setStatusText(
            QStringLiteral(
                "Arc cancelled — committed arcs preserved."));
        break;
    case sketch::SketchTool::rectangle:
        setStatusText(
            QStringLiteral(
                "Rectangle cancelled — committed geometry preserved."));
        break;
    case sketch::SketchTool::move:
        setStatusText(
            QStringLiteral("Move cancelled — selection preserved."));
        break;
    case sketch::SketchTool::copy:
        setStatusText(
            QStringLiteral("Copy cancelled — committed copies preserved; selection preserved."));
        break;
    case sketch::SketchTool::rotate:
        setStatusText(
            QStringLiteral("Rotate cancelled — selection preserved."));
        break;
    case sketch::SketchTool::scale:
        setStatusText(
            QStringLiteral("Scale cancelled — selection preserved."));
        break;
    case sketch::SketchTool::mirror:
        setStatusText(
            QStringLiteral("Mirror cancelled — selection preserved."));
        break;
    case sketch::SketchTool::measure:
        setStatusText(
            QStringLiteral("Measure cancelled — selection preserved."));
        break;
    case sketch::SketchTool::select:
        break;
    }
}

void CadWorkbench::deleteSketchSelection() {
    if (!sketch_interaction_controller_ ||
        !sketch_interaction_controller_->deleteSelection()) {
        return;
    }

    setStatusText(
        QStringLiteral("Sketch selection deleted."));
}

void CadWorkbench::setCadInteractionSettingsProvider(
    CadInteractionSettingsProvider provider) {
    cad_interaction_settings_provider_ =
        std::move(provider);
    if (sketch_interaction_controller_) {
        sketch_interaction_controller_->
            setCadInteractionSettingsProvider(
                cad_interaction_settings_provider_);
    }
    refreshCadInteractionSettingsUi();
}

void CadWorkbench::refreshCadDynamicInputOverlay() {
    if (viewport_controller_ == nullptr) {
        return;
    }

    const auto clear = [this] {
        viewport_controller_->clearSketchDynamicInputOverlay();
    };

    if (!dynamic_input_anchor_ ||
        !dynamic_input_anchor_->valid() ||
        !sketch_interaction_controller_ ||
        !sketch_interaction_controller_->active() ||
        !cad_interaction_settings_provider_ ||
        !cad_dynamic_input_ui_state_provider_) {
        clear();
        return;
    }

    const auto settings =
        cad_interaction_settings_provider_();
    if (!settings.valid() ||
        !settings.dynamic_input_enabled) {
        clear();
        return;
    }

    const auto length_unit =
        document_session_ != nullptr
            ? document_session_->document().lengthUnit()
            : core::LengthUnit::millimetre;
    application::SketchCadInputSemanticEndpoint endpoint{
        *sketch_interaction_controller_,
        application::CadInputNumberFormat{
            toUtf8(QLocale{}.decimalPoint()),
            length_unit}};
    const auto snapshots =
        endpoint.dynamicInputFieldSnapshots();
    if (snapshots.empty()) {
        clear();
        return;
    }

    const auto ui_state =
        cad_dynamic_input_ui_state_provider_();
    const auto focused_index =
        ui_state.focused_index % snapshots.size();

    viewer::SketchDynamicInputOverlay overlay;
    overlay.anchor = *dynamic_input_anchor_;
    overlay.focused_index = focused_index;
    overlay.fields.reserve(snapshots.size());

    for (std::size_t index = 0U;
         index < snapshots.size();
         ++index) {
        const auto& snapshot = snapshots[index];
        viewer::SketchDynamicInputValueState state =
            viewer::SketchDynamicInputValueState::free;
        if (snapshot.value) {
            switch (snapshot.value->state) {
            case application::CadDynamicInputValueState::free:
                state = viewer::SketchDynamicInputValueState::free;
                break;
            case application::CadDynamicInputValueState::assisted:
                state = viewer::SketchDynamicInputValueState::assisted;
                break;
            case application::CadDynamicInputValueState::locked:
                state = viewer::SketchDynamicInputValueState::locked;
                break;
            }
        }

        std::string display = snapshot.display_value;
        if (index == focused_index &&
            !ui_state.buffer.empty()) {
            display = ui_state.buffer;
        }

        overlay.fields.push_back(
            viewer::SketchDynamicInputFieldPresentation{
                snapshot.field.label,
                std::move(display),
                state});
    }

    if (!viewport_controller_->
            setSketchDynamicInputOverlay(overlay)) {
        clear();
    }
}

std::string CadWorkbench::cadInputPrompt() const {
    return toUtf8(cadInputPromptText());
}

application::CadInputContextGeneration
CadWorkbench::cadInputContextGeneration() const noexcept {
    return sketch_interaction_controller_
               ? sketch_interaction_controller_->
                     cadInputContextGeneration()
               : application::CadInputContextGeneration{};
}

std::vector<application::CadDynamicInputField>
CadWorkbench::cadDynamicInputFields() const {
    if (!sketch_interaction_controller_ ||
        !sketch_interaction_controller_->active()) {
        return {};
    }

    const auto length_unit =
        document_session_ != nullptr
            ? document_session_->document().lengthUnit()
            : core::LengthUnit::millimetre;

    application::SketchCadInputSemanticEndpoint endpoint{
        *sketch_interaction_controller_,
        application::CadInputNumberFormat{
            toUtf8(QLocale{}.decimalPoint()),
            length_unit}};
    return endpoint.dynamicInputFields();
}

application::CadInputSubmitResult
CadWorkbench::lockCadDynamicInputField(
    std::size_t index,
    std::string_view text,
    application::CadInputContextGeneration
        expected_context_generation) {
    if (expected_context_generation !=
        cadInputContextGeneration()) {
        return {
            false,
            "CAD input semantic context is stale."};
    }
    if (!sketch_interaction_controller_) {
        return {
            false,
            "No active CAD command context."};
    }

    const auto length_unit =
        document_session_ != nullptr
            ? document_session_->document().lengthUnit()
            : core::LengthUnit::millimetre;

    application::SketchCadInputSemanticEndpoint endpoint{
        *sketch_interaction_controller_,
        application::CadInputNumberFormat{
            toUtf8(QLocale{}.decimalPoint()),
            length_unit}};
    auto result =
        endpoint.lockDynamicInputField(
            index,
            text);
    if (!result.accepted &&
        status_ != nullptr &&
        !result.diagnostic.empty()) {
        setStatusText(
            fromUtf8(result.diagnostic));
    }
    return result;
}

application::CadInputSubmitResult
CadWorkbench::submitCadDynamicInputRequest(
    application::CadInputContextGeneration
        expected_context_generation) {
    if (expected_context_generation !=
        cadInputContextGeneration()) {
        return {
            false,
            "CAD input semantic context is stale."};
    }
    if (!sketch_interaction_controller_ ||
        !sketch_interaction_controller_->active()) {
        return {
            false,
            "No active CAD command context."};
    }

    if (!sketch_interaction_controller_->
            submitCadInputDynamicRequest()) {
        return {
            false,
            "Dynamic Input request is not yet fully placeable."};
    }
    return {true, {}};
}

application::CadInputSubmitResult
CadWorkbench::submitCadInput(
    std::string_view text,
    application::CadInputContextGeneration
        expected_context_generation) {
    if (expected_context_generation != cadInputContextGeneration()) {
        return {false, "CAD input semantic context is stale."};
    }
    if (!sketch_interaction_controller_) {
        return {false, "No active CAD command context."};
    }

    const bool top_level_command_context =
        !sketch_interaction_controller_->
             activePointRequest()
             .has_value();

    const auto length_unit =
        document_session_ != nullptr
            ? document_session_->document().lengthUnit()
            : core::LengthUnit::millimetre;

    application::SketchCadInputSemanticEndpoint endpoint{
        *sketch_interaction_controller_,
        application::CadInputNumberFormat{
            toUtf8(QLocale{}.decimalPoint()),
            length_unit}};

    auto result = endpoint.submit(text);
    if (result.accepted &&
        top_level_command_context &&
        viewport_widget_ != nullptr) {
        viewport_widget_->setFocus(Qt::OtherFocusReason);
    }
    if (!result.accepted &&
        status_ != nullptr &&
        !result.diagnostic.empty()) {
        setStatusText(fromUtf8(result.diagnostic));
    }
    return result;
}
QString CadWorkbench::cadInputPromptText() const {
    if (!sketch_interaction_controller_ ||
        !sketch_interaction_controller_->active()) {
        return QStringLiteral("Command:");
    }

    if (sketch_interaction_controller_->
            profileToolActive()) {
        const auto kind =
            sketch_interaction_controller_->
                profileToolSessionKind();
        const auto command =
            kind &&
                    *kind ==
                        ProfileToolSessionKind::edit
                ? QStringLiteral("EDITPROFILE")
                : QStringLiteral("PROFILE");
        const auto mode =
            sketch_interaction_controller_->
                    profileAreaMode() ==
                part::ProfileAreaEditMode::add_area
                ? QStringLiteral("Add Area")
                : QStringLiteral("Subtract Area");
        return QStringLiteral(
                   "Command: %1 — %2 — Hover/click bounded region")
            .arg(command, mode);
    }

    const auto tool =
        sketch_interaction_controller_->tool();

    if (tool == sketch::SketchTool::select) {
        if (sketch_interaction_controller_->
                directManipulationActive()) {
            const auto mode =
                sketch_interaction_controller_->
                    directEditMode();
            const auto mode_text =
                mode
                    ? directEditModeText(*mode)
                    : QStringLiteral("Unknown");
            return QStringLiteral(
                       "Command: SELECT — Grip %1")
                .arg(mode_text);
        }
        return QStringLiteral("Command: SELECT");
    }

    if (tool == sketch::SketchTool::measure) {
        if (sketch_interaction_controller_->
                measureBetweenActive()) {
            if (sketch_interaction_controller_->
                    measureRelationalResult()) {
                return QStringLiteral(
                    "Command: MEASURE BETWEEN — Result shown; choose next Target A; Esc returns to Measure");
            }
            if (sketch_interaction_controller_->
                    measureFirstRelationTarget()) {
                return QStringLiteral(
                    "Command: MEASURE BETWEEN — Choose Target B; Esc returns to Measure");
            }
            return QStringLiteral(
                "Command: MEASURE BETWEEN — Choose Target A; Esc returns to Measure");
        }
        return QStringLiteral(
            "Command: MEASURE — Click Line/Circle/Arc; BETWEEN for relational; Esc ends");
    }

    const bool common_transform =
        tool == sketch::SketchTool::move ||
        tool == sketch::SketchTool::copy ||
        tool == sketch::SketchTool::rotate ||
        tool == sketch::SketchTool::scale ||
        tool == sketch::SketchTool::mirror;

    if (common_transform) {
        QString keyword;
        switch (tool) {
        case sketch::SketchTool::move:
            keyword = QStringLiteral("MOVE");
            break;
        case sketch::SketchTool::copy:
            keyword = QStringLiteral("COPY");
            break;
        case sketch::SketchTool::rotate:
            keyword = QStringLiteral("ROTATE");
            break;
        case sketch::SketchTool::scale:
            keyword = QStringLiteral("SCALE");
            break;
        case sketch::SketchTool::mirror:
            keyword = QStringLiteral("MIRROR");
            break;
        default:
            break;
        }

        QString instruction;
        const auto stage =
            sketch_interaction_controller_->
                commonTransformStage();
        if (!stage) {
            instruction =
                QStringLiteral("Transform unavailable");
        } else {
            switch (*stage) {
            case sketch::CommonTransformStage::select_objects:
                instruction =
                    QStringLiteral(
                        "Select objects; Enter/Space/RMB to continue");
                break;
            case sketch::CommonTransformStage::await_base_point:
                instruction =
                    QStringLiteral("Specify Base Point");
                break;
            case sketch::CommonTransformStage::await_reference_point:
                instruction =
                    QStringLiteral("Specify Reference Point");
                break;
            case sketch::CommonTransformStage::await_destination:
                instruction =
                    tool == sketch::SketchTool::copy
                        ? QStringLiteral(
                              "Specify copy placement point")
                        : tool == sketch::SketchTool::rotate
                            ? QStringLiteral(
                                  "Specify destination point or Angle")
                            : tool == sketch::SketchTool::scale
                                ? QStringLiteral(
                                      "Specify destination point or Factor")
                                : QStringLiteral(
                                      "Specify destination point");
                break;
            case sketch::CommonTransformStage::await_axis_start:
                instruction =
                    QStringLiteral(
                        "Specify first axis point");
                break;
            case sketch::CommonTransformStage::await_axis_end:
                instruction =
                    QStringLiteral(
                        "Specify second axis point");
                break;
            }
        }

        return QStringLiteral("Command: ") +
               keyword +
               QStringLiteral(" — ") +
               instruction;
    }

    if (tool == sketch::SketchTool::line) {
        const auto stage =
            sketch_interaction_controller_->
                lineStage();
        return stage &&
                       *stage ==
                           sketch::LineStage::
                               await_next_point
                   ? QStringLiteral(
                         "Command: LINE — Specify next point")
                   : QStringLiteral(
                         "Command: LINE — Specify first point");
    }

    if (tool == sketch::SketchTool::circle) {
        const auto stage =
            sketch_interaction_controller_->
                circleStage();
        if (stage &&
            *stage ==
                sketch::CircleStage::await_radius) {
            return
                sketch_interaction_controller_->
                        circleSizeInputMode() ==
                    application::CircleSizeInputMode::
                        diameter
                    ? QStringLiteral(
                          "Command: CIRCLE — Specify diameter [D]")
                    : QStringLiteral(
                          "Command: CIRCLE — Specify radius [R]");
        }
        return QStringLiteral(
            "Command: CIRCLE — Specify center");
    }

    if (tool == sketch::SketchTool::rectangle) {
        const auto stage =
            sketch_interaction_controller_->
                rectangleStage();
        return stage &&
                       *stage ==
                           sketch::RectangleStage::
                               await_opposite_corner
                   ? QStringLiteral(
                         "Command: RECTANGLE — Specify opposite corner")
                   : QStringLiteral(
                         "Command: RECTANGLE — Specify first corner");
    }

    const auto stage =
        sketch_interaction_controller_->arcStage();
    if (stage &&
        *stage == sketch::ArcStage::await_end) {
        return QStringLiteral(
            "Command: ARC — Specify end point");
    }
    if (stage &&
        *stage == sketch::ArcStage::await_arc_point) {
        return QStringLiteral(
            "Command: ARC — Specify arc point or radius");
    }
    return QStringLiteral(
        "Command: ARC — Specify start point");
}

void CadWorkbench::finishSketch() {
    if (!active_sketch_id_) {
        return;
    }

    clearSketchRuntimeContext();
    setStatusText(
        QStringLiteral(
            "Sketch edit finished. The Sketch remains authored in the Part."));
    syncActionState();
}

void CadWorkbench::clearSketchRuntimeContext() {
    sketch_support_pick_active_ = false;
    dynamic_input_anchor_.reset();
    if (viewport_controller_ != nullptr) {
        viewport_controller_->clearSketchDynamicInputOverlay();
    }

    if (sketch_interaction_controller_) {
        sketch_interaction_controller_->end();
    }

    active_sketch_id_.reset();
    sketch_edit_document_id_.reset();

    if (viewport_controller_ != nullptr) {
        viewport_controller_->setSketchEditSketch(
            std::nullopt);
    }

    syncSketchInteractionUi();
}

void CadWorkbench::reconcileSketchRuntimeContext() {
    if (!active_sketch_id_) {
        return;
    }

    if (document_session_ == nullptr ||
        !sketch_edit_document_id_ ||
        document_session_->documentId() !=
            *sketch_edit_document_id_) {
        clearSketchRuntimeContext();
        return;
    }

    auto* document_session =
        activeDocumentSession();
    if (document_session == nullptr) {
        clearSketchRuntimeContext();
        return;
    }

    const auto* sketch =
        document_session->document()
            .findSketch(*active_sketch_id_);
    if (sketch == nullptr) {
        clearSketchRuntimeContext();
        return;
    }

    viewport_controller_->setSketchEditSketch(
        *active_sketch_id_);

    if (sketch_interaction_controller_) {
        if (!sketch_interaction_controller_->active()) {
            sketch_interaction_controller_->begin(
                *document_session,
                *active_sketch_id_);
        } else {
            static_cast<void>(
                sketch_interaction_controller_->
                    reconcileAfterHistory());
        }
    }

    syncSketchInteractionUi();
}

void CadWorkbench::undo() {
    auto* document_session = activeDocumentSession();
    if (document_session == nullptr) return;

    if (sketch_interaction_controller_ &&
        sketch_interaction_controller_->active()) {
        sketch_interaction_controller_->cancelForHistory();
    }

    const auto result = document_session->undo();
    if (!result.ok()) {
        showFailure(result.diagnostic);
        return;
    }

    refreshActiveContext();
    setStatusText(
        result.changed
            ? QStringLiteral("Undo applied.")
            : QStringLiteral("Nothing to undo."));
}

void CadWorkbench::redo() {
    auto* document_session = activeDocumentSession();
    if (document_session == nullptr) return;

    if (sketch_interaction_controller_ &&
        sketch_interaction_controller_->active()) {
        sketch_interaction_controller_->cancelForHistory();
    }

    const auto result = document_session->redo();
    if (!result.ok()) {
        showFailure(result.diagnostic);
        return;
    }

    refreshActiveContext();
    setStatusText(
        result.changed
            ? QStringLiteral("Redo applied.")
            : QStringLiteral("Nothing to redo."));
}

void CadWorkbench::save() {
    auto* document_session =
        activeDocumentSession();
    if (document_session == nullptr) return;

    const auto result =
        document_session->save();
    if (!result.ok()) {
        showFailure(result.diagnostic);
        return;
    }

    refreshActiveContext();
    setStatusText(
        QStringLiteral("Part saved."));
}

void CadWorkbench::closeActiveDocument() {
    if (document_session_ == nullptr ||
        !close_document_handler_) {
        return;
    }

    close_document_handler_(
        document_session_->documentId());
}

void CadWorkbench::refreshActiveContext() {
    auto* document_session = activeDocumentSession();
    if (document_session == nullptr) {
        clearActiveContext();
        return;
    }

    const auto& properties =
        document_session->document().properties();

    number_->setText(fromUtf8(properties.number));
    title_->setText(fromUtf8(properties.title));
    description_->setPlainText(
        fromUtf8(properties.description));
    engineering_revision_->setText(
        fromUtf8(properties.engineering_revision));

    syncing_precision_ui_ = true;
    length_unit_combo_->setCurrentIndex(
        lengthUnitIndex(
            document_session->document()
                .lengthUnit()));
    length_unit_combo_->setEnabled(true);
    syncing_precision_ui_ = false;

    std::error_code ec;
    const auto relative = std::filesystem::relative(
        document_session->path(),
        workspace_root_,
        ec);

    active_path_->setText(
        QStringLiteral("Path: ") +
        fromFilesystemPath(
            ec ? document_session->path() : relative));

    active_id_->setText(
        QStringLiteral("DocumentId: ") +
        fromUtf8(document_session->documentId().value()));

    number_->setEnabled(true);
    title_->setEnabled(true);
    description_->setEnabled(true);
    engineering_revision_->setEnabled(true);

    viewport_controller_->setDocumentSession(document_session);
    reconcileSketchRuntimeContext();
    syncActionState();
    notifyDocumentStateChanged();
}

void CadWorkbench::clearActiveContext() {
    clearSketchRuntimeContext();
    active_path_->setText(QStringLiteral("No Part is open."));
    active_id_->clear();

    number_->clear();
    title_->clear();
    description_->clear();
    engineering_revision_->clear();
    syncing_precision_ui_ = true;
    length_unit_combo_->setCurrentIndex(0);
    length_unit_combo_->setEnabled(false);
    syncing_precision_ui_ = false;
    selected_profile_id_.reset();
    profile_name_->clear();
    profile_identity_->clear();
    profile_source_->clear();
    profile_status_->clear();
    profile_diagnostic_->clear();
    profile_area_->clear();
    profile_perimeter_->clear();
    profile_holes_->clear();
    profile_visible_->setChecked(false);

    number_->setEnabled(false);
    title_->setEnabled(false);
    description_->setEnabled(false);
    engineering_revision_->setEnabled(false);

    viewport_controller_->clear();
    syncActionState();
}

void CadWorkbench::captureActiveViewState() {
    if (viewport_ == nullptr ||
        document_session_ == nullptr) {
        return;
    }

    const auto state = viewport_->cameraState();
    if (!state) return;

    document_view_states_.insert_or_assign(
        std::string{
            document_session_->documentId().value()},
        *state);
}

void CadWorkbench::restoreActiveViewState() {
    if (viewport_ == nullptr ||
        document_session_ == nullptr) {
        return;
    }

    const auto key =
        std::string{
            document_session_->documentId().value()};
    const auto found =
        document_view_states_.find(key);

    if (found != document_view_states_.end()) {
        static_cast<void>(
            viewport_->setCameraState(found->second));
        return;
    }

    viewer::CameraState initial;
    static_cast<void>(
        viewport_->setCameraState(initial));
}

void CadWorkbench::refreshPropertiesContext(
    std::optional<core::BuiltinReferenceRole> primary) {
    if (properties_stack_ == nullptr) return;

    if (!primary) {
        properties_stack_->setCurrentWidget(
            document_properties_page_);
        return;
    }

    QString name;
    QString kind;
    QString identity;

    switch (*primary) {
    case core::BuiltinReferenceRole::origin_point:
        name = QStringLiteral("Origin Point");
        kind = QStringLiteral("Point");
        identity = QStringLiteral(
            "BuiltinReference::OriginPoint");
        break;
    case core::BuiltinReferenceRole::x_axis:
        name = QStringLiteral("X Axis");
        kind = QStringLiteral("Axis");
        identity = QStringLiteral(
            "BuiltinReference::XAxis");
        break;
    case core::BuiltinReferenceRole::y_axis:
        name = QStringLiteral("Y Axis");
        kind = QStringLiteral("Axis");
        identity = QStringLiteral(
            "BuiltinReference::YAxis");
        break;
    case core::BuiltinReferenceRole::z_axis:
        name = QStringLiteral("Z Axis");
        kind = QStringLiteral("Axis");
        identity = QStringLiteral(
            "BuiltinReference::ZAxis");
        break;
    case core::BuiltinReferenceRole::xy_plane:
        name = QStringLiteral("XY Plane");
        kind = QStringLiteral("Plane");
        identity = QStringLiteral(
            "BuiltinReference::XYPlane");
        break;
    case core::BuiltinReferenceRole::xz_plane:
        name = QStringLiteral("XZ Plane");
        kind = QStringLiteral("Plane");
        identity = QStringLiteral(
            "BuiltinReference::XZPlane");
        break;
    case core::BuiltinReferenceRole::yz_plane:
        name = QStringLiteral("YZ Plane");
        kind = QStringLiteral("Plane");
        identity = QStringLiteral(
            "BuiltinReference::YZPlane");
        break;
    }

    reference_name_->setText(name);
    reference_kind_->setText(kind);
    reference_identity_->setText(identity);

    const auto* document_session =
        activeDocumentSession();
    const bool visible =
        document_session != nullptr &&
        document_session->document()
            .builtinReferenceVisible(*primary);

    reference_visibility_->setText(
        visible
            ? QStringLiteral("Shown")
            : QStringLiteral("Hidden"));

    properties_stack_->setCurrentWidget(
        reference_properties_page_);
}

void CadWorkbench::refreshProfileProperties(
    part::ProfileId profile_id) {
    auto* document_session =
        activeDocumentSession();
    if (properties_stack_ == nullptr ||
        document_session == nullptr) {
        return;
    }

    const auto* profile =
        document_session->document()
            .findProfile(profile_id);
    if (profile == nullptr) {
        selected_profile_id_.reset();
        properties_stack_->setCurrentWidget(
            document_properties_page_);
        return;
    }

    selected_profile_id_ = profile_id;
    profile_name_->setText(
        fromUtf8(profile->name));
    profile_identity_->setText(
        fromUtf8(profile->id.serialized()));
    profile_source_->setText(
        fromUtf8(
            profile->source_sketch_id.value()));
    profile_visible_->setChecked(
        profile->visible);

    const auto evaluation =
        document_session->document()
            .evaluateProfile(profile_id);
    const bool valid =
        evaluation && evaluation->valid();
    profile_status_->setText(
        valid
            ? QStringLiteral("Valid")
            : QStringLiteral("Invalid"));

    QString diagnostic =
        QStringLiteral("—");
    if (evaluation && !evaluation->valid()) {
        switch (evaluation->status) {
        case part::ProfileIntentResolutionStatus::valid:
            break;
        case part::ProfileIntentResolutionStatus::invalid_intent:
            diagnostic =
                QStringLiteral("Invalid RegionIntent");
            break;
        case part::ProfileIntentResolutionStatus::missing_source_entity:
            diagnostic =
                QStringLiteral("Missing source entity");
            break;
        case part::ProfileIntentResolutionStatus::ambiguous_topology:
            diagnostic =
                QStringLiteral("Ambiguous source topology");
            break;
        case part::ProfileIntentResolutionStatus::unresolved_intent:
            diagnostic =
                QStringLiteral("Region intent no longer resolves");
            break;
        }
    }
    profile_diagnostic_->setText(diagnostic);

    if (valid) {
        profile_area_->setText(
            QString::number(
                evaluation->region->area,
                'g',
                12));
        profile_perimeter_->setText(
            QString::number(
                evaluation->region->perimeter,
                'g',
                12));
        profile_holes_->setText(
            QString::number(
                static_cast<qulonglong>(
                    evaluation->region->holes.size())));
    } else {
        profile_area_->setText(
            QStringLiteral("—"));
        profile_perimeter_->setText(
            QStringLiteral("—"));
        profile_holes_->setText(
            QStringLiteral("—"));
    }

    delete_profile_button_->setEnabled(
        !sketch_interaction_controller_ ||
        !sketch_interaction_controller_->
             profileToolActive());

    properties_stack_->setCurrentWidget(
        profile_properties_page_);
}

bool CadWorkbench::eventFilter(
    QObject* watched,
    QEvent* event) {
    if (event != nullptr &&
        event->type() == QEvent::MouseButtonPress &&
        watched == viewport_widget_ &&
        sketch_interaction_controller_ &&
        sketch_interaction_controller_->active()) {
        auto* mouse_event =
            static_cast<QMouseEvent*>(event);
        if (mouse_event->button() == Qt::RightButton &&
            sketch_interaction_controller_->
                commonTransformStage() ==
                sketch::CommonTransformStage::select_objects) {
            if (sketch_interaction_controller_->
                    completeTransformSelection()) {
                setStatusText(
                    QStringLiteral(
                        "Transform objects accepted."));
            }
            return true;
        }
    }

    if (event != nullptr &&
        event->type() == QEvent::KeyPress) {
        auto* key_event =
            static_cast<QKeyEvent*>(event);

        const bool profile_delete_context =
            watched == viewport_widget_ &&
            selected_profile_id_ &&
            properties_stack_ != nullptr &&
            properties_stack_->currentWidget() ==
                profile_properties_page_ &&
            (!sketch_interaction_controller_ ||
             !sketch_interaction_controller_->
                  profileToolActive());
        if (profile_delete_context &&
            key_event->key() == Qt::Key_Delete) {
            deleteSelectedProfile();
            return true;
        }

        if (watched == viewport_widget_ &&
            sketch_interaction_controller_ &&
            sketch_interaction_controller_->active()) {
            if (key_event->key() == Qt::Key_Delete) {
                deleteSketchSelection();
                return true;
            }

            if (key_event->key() == Qt::Key_Escape) {
                if (sketch_interaction_controller_->escape()) {
                    setStatusText(
                        QStringLiteral(
                            "Sketch interaction cancelled."));
                }
                return true;
            }

            if (key_event->key() == Qt::Key_Space &&
                sketch_interaction_controller_->
                    directManipulationActive()) {
                static_cast<void>(
                    sketch_interaction_controller_->
                        cycleDirectEditMode());
                return true;
            }

            if ((key_event->key() == Qt::Key_Return ||
                 key_event->key() == Qt::Key_Enter) &&
                sketch_interaction_controller_->
                    directManipulationActive()) {
                if (sketch_interaction_controller_->
                        commitDirectManipulation()) {
                    setStatusText(
                        QStringLiteral(
                            "Sketch edit committed."));
                }
                return true;
            }

            const auto transform_stage =
                sketch_interaction_controller_->
                    commonTransformStage();
            if (transform_stage) {
                if ((key_event->key() == Qt::Key_Return ||
                     key_event->key() == Qt::Key_Enter ||
                     key_event->key() == Qt::Key_Space) &&
                    *transform_stage ==
                        sketch::CommonTransformStage::
                            select_objects) {
                    if (sketch_interaction_controller_->
                            completeTransformSelection()) {
                        setStatusText(
                            QStringLiteral(
                                "Transform objects accepted."));
                    }
                    return true;
                }

                const bool final_stage =
                    *transform_stage ==
                        sketch::CommonTransformStage::
                            await_destination ||
                    *transform_stage ==
                        sketch::CommonTransformStage::
                            await_axis_end;
                if ((key_event->key() == Qt::Key_Return ||
                     key_event->key() == Qt::Key_Enter) &&
                    final_stage) {
                    static_cast<void>(
                        sketch_interaction_controller_->
                            commitTransform());
                    return true;
                }
            }

            if ((key_event->key() == Qt::Key_Return ||
                 key_event->key() == Qt::Key_Enter ||
                 key_event->key() == Qt::Key_Space) &&
                sketch_interaction_controller_->tool() ==
                    sketch::SketchTool::select &&
                !sketch_interaction_controller_->
                    directManipulationActive()) {
                const auto remembered =
                    sketch_interaction_controller_->
                        lastRepeatableCommand();
                if (!sketch_interaction_controller_->
                        repeatLastCommand()) {
                    setStatusText(
                        remembered
                            ? QStringLiteral(
                                  "Last Sketch command could not be repeated.")
                            : QStringLiteral(
                                  "No repeatable Sketch command."));
                } else {
                    setStatusText(
                        QStringLiteral(
                            "Last Sketch command repeated."));
                }
                return true;
            }
        }
    }

    return QWidget::eventFilter(watched, event);
}

void CadWorkbench::refreshCadInteractionSettingsUi() {
    syncing_precision_ui_ = true;

    const bool editing =
        sketch_interaction_controller_ &&
        sketch_interaction_controller_->active();
    const bool settings_available =
        static_cast<bool>(
            cad_interaction_settings_provider_);

    if (precision_operations_widget_ != nullptr) {
        precision_operations_widget_->setVisible(
            editing && settings_available);
    }

    if (editing && settings_available) {
        const auto settings =
            cad_interaction_settings_provider_();

        polar_toggle_button_->setChecked(
            settings.polar.enabled);
        polar_toggle_button_->setText(
            settings.polar.enabled
                ? QStringLiteral("ON")
                : QStringLiteral("OFF"));

        if (!polar_step_edit_->hasFocus()) {
            polar_step_edit_->setText(
                QString::number(
                    settings.polar.primary_spacing *
                        180.0 /
                        std::numbers::pi_v<double>,
                    'g',
                    12));
        }
        polar_reference_combo_->setCurrentIndex(
            settings.polar.reference_mode ==
                    application::
                        PolarReferenceMode::relative
                ? 1
                : 0);
        polar_additional_label_->setText(
            polarAdditionalSummary(
                settings.polar.additional_angles));
        polar_additional_clear_button_->setEnabled(
            !settings.polar.additional_angles.empty());

        dynamic_input_toggle_button_->setChecked(
            settings.dynamic_input_enabled);
        dynamic_input_toggle_button_->setText(
            settings.dynamic_input_enabled
                ? QStringLiteral("ON")
                : QStringLiteral("OFF"));

        const auto reference =
            settings.polar.reference_mode ==
                    application::
                        PolarReferenceMode::absolute
                ? QStringLiteral("ABS")
                : QStringLiteral("REL");
        precision_status_label_->setText(
            QStringLiteral(
                "POLAR %1   %2   %3   DYN %4")
                .arg(
                    settings.polar.enabled
                        ? QStringLiteral("ON")
                        : QStringLiteral("OFF"),
                    polarSpacingSummary(
                        settings.polar.primary_spacing),
                    reference,
                    settings.dynamic_input_enabled
                        ? QStringLiteral("ON")
                        : QStringLiteral("OFF")));

        const bool circle =
            sketch_interaction_controller_->tool() ==
            sketch::SketchTool::circle;
        circle_size_mode_label_->setVisible(circle);
        circle_size_mode_combo_->setVisible(circle);
        circle_size_mode_combo_->setEnabled(
            circle &&
            sketch_interaction_controller_->
                circleStage() ==
                sketch::CircleStage::await_radius);
        circle_size_mode_combo_->setCurrentIndex(
            sketch_interaction_controller_->
                    circleSizeInputMode() ==
                application::
                    CircleSizeInputMode::radius
                ? 1
                : 0);
    } else {
        if (precision_status_label_ != nullptr) {
            precision_status_label_->clear();
        }
        if (circle_size_mode_label_ != nullptr) {
            circle_size_mode_label_->setVisible(false);
        }
        if (circle_size_mode_combo_ != nullptr) {
            circle_size_mode_combo_->setVisible(false);
        }
    }

    syncing_precision_ui_ = false;
}

void CadWorkbench::syncSketchInteractionUi() {
    notifyCadInputContextChanged();

    if (measure_between_button_ != nullptr) {
        measure_between_button_->setVisible(false);
        measure_between_button_->setChecked(false);
    }

    if (entity_role_label_ != nullptr) {
        entity_role_label_->setVisible(false);
    }
    if (regular_role_button_ != nullptr) {
        regular_role_button_->setVisible(false);
    }
    if (construction_role_button_ != nullptr) {
        construction_role_button_->setVisible(false);
    }
    if (create_construction_button_ != nullptr) {
        create_construction_button_->setVisible(false);
    }
    if (rectangle_diagonals_button_ != nullptr) {
        rectangle_diagonals_button_->setVisible(false);
    }

    const bool editing =
        sketch_interaction_controller_ &&
        sketch_interaction_controller_->active();

    refreshCadInteractionSettingsUi();

    if (sketch_button_ != nullptr) {
        sketch_button_->setVisible(!editing);
    }

    if (select_sketch_button_ != nullptr) {
        select_sketch_button_->setVisible(editing);
        select_sketch_button_->setChecked(
            editing &&
            !sketch_interaction_controller_->
                 profileToolActive() &&
            sketch_interaction_controller_->tool() ==
                sketch::SketchTool::select);
    }

    if (create_tools_label_ != nullptr) {
        create_tools_label_->setVisible(editing);
    }
    if (profile_tools_label_ != nullptr) {
        profile_tools_label_->setVisible(editing);
    }
    if (profile_sketch_button_ != nullptr) {
        profile_sketch_button_->setVisible(editing);
        profile_sketch_button_->setChecked(
            editing &&
            sketch_interaction_controller_->
                profileToolActive());
    }
    if (modify_tools_label_ != nullptr) {
        modify_tools_label_->setVisible(editing);
    }

    if (line_sketch_button_ != nullptr) {
        line_sketch_button_->setVisible(editing);
        line_sketch_button_->setChecked(
            editing &&
            sketch_interaction_controller_->tool() ==
                sketch::SketchTool::line);
    }

    if (circle_sketch_button_ != nullptr) {
        circle_sketch_button_->setVisible(editing);
        circle_sketch_button_->setChecked(
            editing &&
            sketch_interaction_controller_->tool() ==
                sketch::SketchTool::circle);
    }

    if (arc_sketch_button_ != nullptr) {
        arc_sketch_button_->setVisible(editing);
        arc_sketch_button_->setChecked(
            editing &&
            sketch_interaction_controller_->tool() ==
                sketch::SketchTool::arc);
    }

    if (rectangle_sketch_button_ != nullptr) {
        rectangle_sketch_button_->setVisible(editing);
        rectangle_sketch_button_->setChecked(
            editing &&
            sketch_interaction_controller_->tool() ==
                sketch::SketchTool::rectangle);
    }
    if (create_construction_button_ != nullptr) {
        create_construction_button_->setChecked(
            editing &&
            sketch_interaction_controller_->
                creationRole() ==
                    sketch::EntityRole::construction);
    }
    if (rectangle_diagonals_button_ != nullptr) {
        rectangle_diagonals_button_->setChecked(
            editing &&
            sketch_interaction_controller_->
                rectangleDrawDiagonals());
    }

    if (move_sketch_button_ != nullptr) {
        move_sketch_button_->setVisible(editing);
        move_sketch_button_->setChecked(
            editing &&
            sketch_interaction_controller_->tool() ==
                sketch::SketchTool::move);
    }

    if (copy_sketch_button_ != nullptr) {
        copy_sketch_button_->setVisible(editing);
        copy_sketch_button_->setChecked(
            editing &&
            sketch_interaction_controller_->tool() ==
                sketch::SketchTool::copy);
    }

    if (rotate_sketch_button_ != nullptr) {
        rotate_sketch_button_->setVisible(editing);
        rotate_sketch_button_->setChecked(
            editing &&
            sketch_interaction_controller_->tool() ==
                sketch::SketchTool::rotate);
    }

    if (scale_sketch_button_ != nullptr) {
        scale_sketch_button_->setVisible(editing);
        scale_sketch_button_->setChecked(
            editing &&
            sketch_interaction_controller_->tool() ==
                sketch::SketchTool::scale);
    }

    if (mirror_sketch_button_ != nullptr) {
        mirror_sketch_button_->setVisible(editing);
        mirror_sketch_button_->setChecked(
            editing &&
            sketch_interaction_controller_->tool() ==
                sketch::SketchTool::mirror);
    }
    if (inspect_tools_label_ != nullptr) {
        inspect_tools_label_->setVisible(editing);
    }
    if (measure_sketch_button_ != nullptr) {
        measure_sketch_button_->setVisible(editing);
        measure_sketch_button_->setChecked(
            editing &&
            sketch_interaction_controller_->tool() ==
                sketch::SketchTool::measure);
    }

    const bool profile_active =
        editing &&
        sketch_interaction_controller_->
            profileToolActive();
    if (profile_operations_widget_ != nullptr) {
        profile_operations_widget_->setVisible(
            profile_active);
    }

    if (!editing) {
        if (operations_placeholder_ != nullptr &&
            !sketch_support_pick_active_) {
            operations_placeholder_->setText(
                QStringLiteral("Part modeling context."));
        }
        if (delete_selection_button_ != nullptr) {
            delete_selection_button_->setVisible(false);
        }
        if (finish_line_button_ != nullptr) {
            finish_line_button_->setVisible(false);
        }
        if (cancel_line_button_ != nullptr) {
            cancel_line_button_->setVisible(false);
        }
        return;
    }

    if (profile_active) {
        delete_selection_button_->setVisible(false);
        finish_line_button_->setVisible(false);
        cancel_line_button_->setVisible(false);

        const auto kind =
            sketch_interaction_controller_->
                profileToolSessionKind();
        const auto editing_profile =
            kind &&
            *kind ==
                ProfileToolSessionKind::edit;
        QString title =
            editing_profile
                ? QStringLiteral("Profile — Edit")
                : QStringLiteral("Profile — Create");

        if (editing_profile) {
            const auto id =
                sketch_interaction_controller_->
                    editedProfileId();
            if (id && document_session_ != nullptr) {
                const auto* profile =
                    document_session_->document()
                        .findProfile(*id);
                if (profile != nullptr) {
                    title +=
                        QStringLiteral(" — ") +
                        QString::fromUtf8(
                            profile->name.c_str());
                }
            }
        }
        operations_placeholder_->setText(title);

        const bool add_mode =
            sketch_interaction_controller_->
                profileAreaMode() ==
            part::ProfileAreaEditMode::add_area;
        profile_add_area_button_->setChecked(
            add_mode);
        profile_subtract_area_button_->setChecked(
            !add_mode);

        const auto options =
            sketch_interaction_controller_->
                profileToolOptions();
        profile_detect_islands_button_->setChecked(
            options.detect_islands);
        profile_highlight_hover_button_->setChecked(
            options.highlight_on_hover);
        profile_show_boundaries_button_->setChecked(
            options.show_region_boundaries);
        profile_show_problems_button_->setChecked(
            options.show_problems);

        const auto current =
            sketch_interaction_controller_->
                profileCurrentResult();
        QString status_text;
        if (current) {
            status_text =
                QStringLiteral(
                    "Current result\n"
                    "Status: Valid\n"
                    "Area: %1\n"
                    "Perimeter: %2\n"
                    "Holes: %3\n"
                    "Islands: %4\n"
                    "Problems: %5")
                    .arg(
                        QString::number(
                            current->area,
                            'g',
                            12),
                        QString::number(
                            current->perimeter,
                            'g',
                            12))
                    .arg(
                        static_cast<qulonglong>(
                            current->holes.size()))
                    .arg(
                        static_cast<qulonglong>(
                            sketch_interaction_controller_->
                                profileIslandCount()))
                    .arg(
                        static_cast<qulonglong>(
                            sketch_interaction_controller_->
                                profileProblemCount()));
        } else {
            QString status =
                QStringLiteral("Ready");
            const auto hover =
                sketch_interaction_controller_->
                    profileHoverStatus();
            if (hover) {
                switch (*hover) {
                case part::ProfileAreaEditStatus::
                    disconnected_result:
                    status =
                        QStringLiteral(
                            "Rejected — disconnected material");
                    break;
                case part::ProfileAreaEditStatus::
                    ambiguous_topology:
                    status =
                        QStringLiteral(
                            "Rejected — ambiguous topology");
                    break;
                case part::ProfileAreaEditStatus::
                    invalid_selection:
                    status =
                        QStringLiteral(
                            "Invalid selection");
                    break;
                case part::ProfileAreaEditStatus::
                    invalid_draft: {
                    status =
                        QStringLiteral(
                            "Invalid draft");
                    const auto resolution =
                        sketch_interaction_controller_->
                            profileDraftResolutionStatus();
                    if (resolution) {
                        switch (*resolution) {
                        case part::ProfileIntentResolutionStatus::
                            invalid_intent:
                            status +=
                                QStringLiteral(
                                    " — invalid intent");
                            break;
                        case part::ProfileIntentResolutionStatus::
                            missing_source_entity:
                            status +=
                                QStringLiteral(
                                    " — missing source");
                            break;
                        case part::ProfileIntentResolutionStatus::
                            ambiguous_topology:
                            status +=
                                QStringLiteral(
                                    " — ambiguous topology");
                            break;
                        case part::ProfileIntentResolutionStatus::
                            unresolved_intent:
                            status +=
                                QStringLiteral(
                                    " — unresolved intent");
                            break;
                        case part::ProfileIntentResolutionStatus::
                            valid:
                            break;
                        }
                    }
                    break;
                }
                case part::ProfileAreaEditStatus::
                    no_change:
                    status =
                        QStringLiteral("No change");
                    break;
                case part::ProfileAreaEditStatus::
                    changed:
                    status =
                        QStringLiteral("Preview");
                    break;
                }
            }
            status_text =
                QStringLiteral(
                    "Current result\n"
                    "Status: %1\n"
                    "Area: —\n"
                    "Perimeter: —\n"
                    "Holes: —\n"
                    "Islands: %2\n"
                    "Problems: %3")
                    .arg(status)
                    .arg(
                        static_cast<qulonglong>(
                            sketch_interaction_controller_->
                                profileIslandCount()))
                    .arg(
                        static_cast<qulonglong>(
                            sketch_interaction_controller_->
                                profileProblemCount()));
        }
        profile_result_label_->setText(
            status_text);
        profile_finish_button_->setEnabled(
            sketch_interaction_controller_->
                profileDraftValid());
        return;
    }

    const auto tool =
        sketch_interaction_controller_->tool();

    if (tool == sketch::SketchTool::select) {
        const auto selected =
            sketch_interaction_controller_->selectedCount();

        if (sketch_interaction_controller_->
                directManipulationActive()) {
            const auto mode =
                sketch_interaction_controller_->
                    directEditMode();
            const auto mode_text =
                mode
                    ? directEditModeText(*mode)
                    : QStringLiteral("Unknown");
            operations_placeholder_->setText(
                QStringLiteral(
                    "Grip — %1; Space cycles mode; Enter/LMB commits; Esc cancels")
                    .arg(mode_text));
            delete_selection_button_->setVisible(false);
            finish_line_button_->setVisible(false);
            cancel_line_button_->setVisible(false);
                        return;
        }

        operations_placeholder_->setText(
            QStringLiteral("Select — %1 entit%2 selected")
                .arg(static_cast<qulonglong>(selected))
                .arg(selected == 1U
                         ? QStringLiteral("y")
                         : QStringLiteral("ies")));
        delete_selection_button_->setVisible(true);
        delete_selection_button_->setEnabled(
            selected > 0U);

        entity_role_label_->setVisible(
            selected > 0U);
        regular_role_button_->setVisible(
            selected > 0U);
        construction_role_button_->setVisible(
            selected > 0U);
        const auto selected_role =
            sketch_interaction_controller_->
                selectedEntityRole();
        regular_role_button_->setChecked(
            selected_role &&
            *selected_role ==
                sketch::EntityRole::regular);
        construction_role_button_->setChecked(
            selected_role &&
            *selected_role ==
                sketch::EntityRole::construction);

        finish_line_button_->setVisible(false);
        cancel_line_button_->setVisible(false);
                return;
    }

    if (tool == sketch::SketchTool::measure) {
        delete_selection_button_->setVisible(false);
        finish_line_button_->setVisible(false);
        cancel_line_button_->setVisible(false);

        const bool between =
            sketch_interaction_controller_->
                measureBetweenActive();
        if (measure_between_button_ != nullptr) {
            measure_between_button_->setVisible(true);
            measure_between_button_->setChecked(
                between);
            measure_between_button_->setEnabled(
                !between);
        }

        if (between) {
            const auto first =
                sketch_interaction_controller_->
                    measureFirstRelationTarget();
            const auto second =
                sketch_interaction_controller_->
                    measureSecondRelationTarget();
            const auto relation =
                sketch_interaction_controller_->
                    measureRelationalResult();

            if (relation && first && second) {
                operations_placeholder_->setText(
                    formatRelationalMeasurement(
                        *relation,
                        *first,
                        *second,
                        document_session_->document()
                            .lengthUnit()));
            } else if (first) {
                operations_placeholder_->setText(
                    QStringLiteral(
                        "Measure Between\n"
                        "Target A: %1\n"
                        "Target B: Choose visible point marker or Line body")
                        .arg(
                            formatMeasureRelationTarget(
                                *first)));
            } else {
                operations_placeholder_->setText(
                    QStringLiteral(
                        "Measure Between\n"
                        "Target A: Choose visible point marker or Line body\n"
                        "Target B: —"));
            }
            return;
        }

        if (measure_between_button_ != nullptr) {
            measure_between_button_->setEnabled(true);
        }
        const auto result =
            sketch_interaction_controller_->
                measureResult();
        operations_placeholder_->setText(
            result
                ? formatMeasurement(
                      *result,
                      document_session_->document()
                          .lengthUnit())
                : QStringLiteral(
                      "Measure — Click Line/Circle/Arc to inspect; Between measures relations; Esc ends"));
        return;
    }

    const bool common_transform =
        tool == sketch::SketchTool::move ||
        tool == sketch::SketchTool::copy ||
        tool == sketch::SketchTool::rotate ||
        tool == sketch::SketchTool::scale ||
        tool == sketch::SketchTool::mirror;

    if (common_transform) {
        delete_selection_button_->setVisible(false);
        finish_line_button_->setVisible(false);
        cancel_line_button_->setVisible(false);

        QString title;
        QString keyword;
        switch (tool) {
        case sketch::SketchTool::move:
            title = QStringLiteral("Move");
            keyword = QStringLiteral("MOVE");
            break;
        case sketch::SketchTool::copy:
            title = QStringLiteral("Copy");
            keyword = QStringLiteral("COPY");
            break;
        case sketch::SketchTool::rotate:
            title = QStringLiteral("Rotate");
            keyword = QStringLiteral("ROTATE");
            break;
        case sketch::SketchTool::scale:
            title = QStringLiteral("Scale");
            keyword = QStringLiteral("SCALE");
            break;
        case sketch::SketchTool::mirror:
            title = QStringLiteral("Mirror");
            keyword = QStringLiteral("MIRROR");
            break;
        default:
            break;
        }

        QString instruction;
        const auto stage =
            sketch_interaction_controller_->
                commonTransformStage();
        if (!stage) {
            instruction =
                QStringLiteral("Transform unavailable");
        } else {
            switch (*stage) {
            case sketch::CommonTransformStage::select_objects:
                instruction =
                    QStringLiteral(
                        "Select objects; Enter/Space/RMB to continue");
                break;
            case sketch::CommonTransformStage::await_base_point:
                instruction =
                    QStringLiteral("Specify Base Point");
                break;
            case sketch::CommonTransformStage::await_reference_point:
                instruction =
                    QStringLiteral("Specify Reference Point");
                break;
            case sketch::CommonTransformStage::await_destination:
                instruction =
                    tool == sketch::SketchTool::copy
                        ? QStringLiteral("Specify copy placement point")
                        : tool == sketch::SketchTool::rotate
                            ? QStringLiteral(
                                  "Specify destination point or Angle")
                            : tool == sketch::SketchTool::scale
                                ? QStringLiteral(
                                      "Specify destination point or Factor")
                                : QStringLiteral(
                                      "Specify destination point");
                break;
            case sketch::CommonTransformStage::await_axis_start:
                instruction =
                    QStringLiteral("Specify first axis point");
                break;
            case sketch::CommonTransformStage::await_axis_end:
                instruction =
                    QStringLiteral("Specify second axis point");
                break;
            }
        }

        operations_placeholder_->setText(
            title +
            QStringLiteral(" — ") +
            instruction);
                return;
    }

    delete_selection_button_->setVisible(false);
    finish_line_button_->setVisible(true);
    cancel_line_button_->setVisible(true);

    const bool creation_tool =
        tool == sketch::SketchTool::line ||
        tool == sketch::SketchTool::circle ||
        tool == sketch::SketchTool::arc ||
        tool == sketch::SketchTool::rectangle;
    if (create_construction_button_ != nullptr) {
        create_construction_button_->setVisible(
            creation_tool);
    }
    if (rectangle_diagonals_button_ != nullptr) {
        rectangle_diagonals_button_->setVisible(
            tool == sketch::SketchTool::rectangle);
    }

    if (tool == sketch::SketchTool::line) {
        finish_line_button_->setText(
            QStringLiteral("Finish Line"));
        cancel_line_button_->setText(
            QStringLiteral("Cancel Line"));

        const auto stage =
            sketch_interaction_controller_->lineStage();
        const bool next =
            stage &&
            *stage ==
                sketch::LineStage::await_next_point;

        operations_placeholder_->setText(
            next
                ? QStringLiteral("Line — Specify next point")
                : QStringLiteral("Line — Specify first point"));
                return;
    }

    if (tool == sketch::SketchTool::circle) {
        finish_line_button_->setText(
            QStringLiteral("Finish Circle"));
        cancel_line_button_->setText(
            QStringLiteral("Cancel Circle"));

        const auto stage =
            sketch_interaction_controller_->circleStage();
        const bool radius =
            stage &&
            *stage ==
                sketch::CircleStage::await_radius;

        if (radius) {
            operations_placeholder_->setText(
                sketch_interaction_controller_->
                        circleSizeInputMode() ==
                    application::CircleSizeInputMode::
                        diameter
                    ? QStringLiteral(
                          "Circle — Diameter mode [D]; specify size")
                    : QStringLiteral(
                          "Circle — Radius mode [R]; specify size"));
        } else {
            operations_placeholder_->setText(
                QStringLiteral(
                    "Circle — Specify center"));
        }
        return;
    }

    if (tool == sketch::SketchTool::rectangle) {
        finish_line_button_->setText(
            QStringLiteral("Finish Rectangle"));
        cancel_line_button_->setText(
            QStringLiteral("Cancel Rectangle"));

        const auto stage =
            sketch_interaction_controller_->
                rectangleStage();
        const bool opposite =
            stage &&
            *stage ==
                sketch::RectangleStage::
                    await_opposite_corner;
        const auto role =
            sketch_interaction_controller_->
                creationRole() ==
                    sketch::EntityRole::construction
                ? QStringLiteral("Construction")
                : QStringLiteral("Regular");
        const auto diagonals =
            sketch_interaction_controller_->
                rectangleDrawDiagonals()
                ? QStringLiteral("On")
                : QStringLiteral("Off");

        operations_placeholder_->setText(
            QStringLiteral(
                "Rectangle — %1; Role: %2; Draw Diagonals: %3")
                .arg(
                    opposite
                        ? QStringLiteral(
                              "Specify opposite corner")
                        : QStringLiteral(
                              "Specify first corner"),
                    role,
                    diagonals));
        return;
    }

    finish_line_button_->setText(
        QStringLiteral("Finish Arc"));
    cancel_line_button_->setText(
        QStringLiteral("Cancel Arc"));

    const auto stage =
        sketch_interaction_controller_->arcStage();
    if (stage &&
        *stage == sketch::ArcStage::await_end) {
        operations_placeholder_->setText(
            QStringLiteral("Arc — Specify end point"));
    } else if (
        stage &&
        *stage == sketch::ArcStage::await_arc_point) {
        operations_placeholder_->setText(
            QStringLiteral(
                "Arc — Specify arc point or radius"));
    } else {
        operations_placeholder_->setText(
            QStringLiteral("Arc — Specify start point"));
    }
}

void CadWorkbench::syncActionState() {
    const auto* document_session = activeDocumentSession();
    const bool active = document_session != nullptr;

    apply_button_->setEnabled(active);
    undo_button_->setEnabled(
        active && document_session->canUndo());
    redo_button_->setEnabled(
        active && document_session->canRedo());
    // Save remains available for a clean active Document so an explicit
    // Save can revalidate the native-file checkpoint and report an external
    // file conflict as required by ADR-0013.
    save_button_->setEnabled(active);
    close_document_button_->setEnabled(active);

    const bool editing_sketch =
        active &&
        active_sketch_id_.has_value() &&
        sketch_edit_document_id_.has_value() &&
        document_session_ != nullptr &&
        *sketch_edit_document_id_ ==
            document_session_->documentId();

    sketch_button_->setText(
        QStringLiteral("Sketch"));
    sketch_button_->setVisible(!editing_sketch);
    sketch_button_->setEnabled(
        active &&
        !editing_sketch &&
        !sketch_support_pick_active_);

    select_sketch_button_->setVisible(
        editing_sketch);
    create_tools_label_->setVisible(
        editing_sketch);
    line_sketch_button_->setVisible(
        editing_sketch);
    circle_sketch_button_->setVisible(
        editing_sketch);
    arc_sketch_button_->setVisible(
        editing_sketch);
    rectangle_sketch_button_->setVisible(
        editing_sketch);
    modify_tools_label_->setVisible(
        editing_sketch);
    move_sketch_button_->setVisible(
        editing_sketch);
    rotate_sketch_button_->setVisible(
        editing_sketch);
    scale_sketch_button_->setVisible(
        editing_sketch);
    mirror_sketch_button_->setVisible(
        editing_sketch);
    inspect_tools_label_->setVisible(
        editing_sketch);
    measure_sketch_button_->setVisible(
        editing_sketch);
    if (measure_between_button_ != nullptr &&
        !editing_sketch) {
        measure_between_button_->setVisible(false);
    }

    cancel_sketch_button_->setVisible(
        active && sketch_support_pick_active_);
    cancel_sketch_button_->setEnabled(
        active && sketch_support_pick_active_);

    finish_sketch_button_->setVisible(
        editing_sketch);
    finish_sketch_button_->setEnabled(
        editing_sketch);

    syncSketchInteractionUi();
}

void CadWorkbench::notifyCadInputContextChanged() {
    if (cad_input_context_changed_handler_) {
        cad_input_context_changed_handler_();
    }
}

void CadWorkbench::notifyDocumentStateChanged() {
    if (document_session_ != nullptr &&
        document_state_changed_handler_) {
        document_state_changed_handler_(
            document_session_->documentId());
    }
}

void CadWorkbench::setStatusText(
    const QString& message) {
    if (status_ == nullptr) {
        return;
    }

    const bool degraded =
        viewport_controller_ != nullptr &&
        viewport_controller_->presentationDegraded();

    if (!degraded) {
        status_->setText(message);
        return;
    }

    const auto presentation_diagnostic =
        QStringLiteral(
            "3D presentation update failed. Authored CAD state remains "
            "authoritative; the next full refresh will retry.");

    status_->setText(
        message.isEmpty()
            ? presentation_diagnostic
            : message + QStringLiteral(" ") +
                  presentation_diagnostic);
}

void CadWorkbench::showFailure(
    const application::DocumentSessionDiagnostic& diagnostic) {
    auto message = fromUtf8(diagnostic.message);

    if (!diagnostic.path.empty()) {
        message +=
            QStringLiteral("\n\nPath: ") +
            fromFilesystemPath(diagnostic.path);
    }

    QMessageBox::warning(
        this,
        QStringLiteral("Part Document"),
        message);
}

} // namespace simplesolid2::ui
