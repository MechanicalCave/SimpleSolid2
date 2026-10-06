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
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QStringList>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>
#include <cctype>
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

QString fromUtf8(std::string_view value);

[[nodiscard]] std::string upperAsciiTrimmed(
    std::string_view value) {
    std::size_t first = 0U;
    while (first < value.size() &&
           std::isspace(
               static_cast<unsigned char>(
                   value[first])) != 0) {
        ++first;
    }

    std::size_t last = value.size();
    while (last > first &&
           std::isspace(
               static_cast<unsigned char>(
                   value[last - 1U])) != 0) {
        --last;
    }

    std::string result;
    result.reserve(last - first);
    for (std::size_t index = first;
         index < last;
         ++index) {
        result.push_back(
            static_cast<char>(
                std::toupper(
                    static_cast<unsigned char>(
                        value[index]))));
    }
    return result;
}

[[nodiscard]] QString
extrudeEvaluationText(
    const application::ExtrudeDraftEvaluationResult&
        evaluation) {
    using Status =
        application::ExtrudeDraftEvaluationStatus;

    if (evaluation.status == Status::ok) {
        return evaluation.previewSolidAvailable()
            ? QStringLiteral("Preview ready.")
            : QStringLiteral(
                  "Feature is valid; final Body preview is unavailable.");
    }

    if (evaluation.status ==
        Status::target_failed) {
        if (evaluation.evaluation_diagnostic) {
            switch (*evaluation.evaluation_diagnostic) {
            case part::FeatureEvaluationDiagnosticCode::
                missing_upstream_body:
                return QStringLiteral(
                    "Cut requires a valid upstream Body.");
            case part::FeatureEvaluationDiagnosticCode::
                unresolved_profile:
                return QStringLiteral(
                    "Source Profile is unresolved.");
            case part::FeatureEvaluationDiagnosticCode::
                sketch_support_missing:
                return QStringLiteral(
                    "Sketch support is missing at its declared Body stage.");
            case part::FeatureEvaluationDiagnosticCode::
                sketch_support_ambiguous:
                return QStringLiteral(
                    "Sketch support is ambiguous at its declared Body stage.");
            case part::FeatureEvaluationDiagnosticCode::
                sketch_support_unsupported:
                return QStringLiteral(
                    "Sketch support is unsupported for modeling.");
            case part::FeatureEvaluationDiagnosticCode::
                detached_add:
                return QStringLiteral(
                    "Add is detached from the current Body.");
            case part::FeatureEvaluationDiagnosticCode::
                no_effect:
                return QStringLiteral(
                    "Extrude has no modeling effect.");
            case part::FeatureEvaluationDiagnosticCode::
                empty_result:
                return QStringLiteral(
                    "Extrude would produce an empty Body.");
            case part::FeatureEvaluationDiagnosticCode::
                multi_solid:
                return QStringLiteral(
                    "Extrude would produce multiple solids.");
            case part::FeatureEvaluationDiagnosticCode::
                kernel_invalid_input:
            case part::FeatureEvaluationDiagnosticCode::
                kernel_provider_mismatch:
            case part::FeatureEvaluationDiagnosticCode::
                kernel_provider_failure:
            case part::FeatureEvaluationDiagnosticCode::
                invalid_brep:
                return QStringLiteral(
                    "Kernel rejected the current Extrude.");
            case part::FeatureEvaluationDiagnosticCode::
                topology_integrity_failure:
                return QStringLiteral(
                    "Body topology accounting failed; Extrude was not accepted.");
            case part::FeatureEvaluationDiagnosticCode::
                missing_profile:
                return QStringLiteral(
                    "Source Profile is missing.");
            case part::FeatureEvaluationDiagnosticCode::
                upstream_unavailable:
                return QStringLiteral(
                    "Upstream Body is unavailable.");
            case part::FeatureEvaluationDiagnosticCode::none:
                break;
            }
        }
        return QStringLiteral(
            "Current Extrude cannot be finished.");
    }

    switch (evaluation.status) {
    case Status::stale_document:
    case Status::stale_revision:
        return QStringLiteral(
            "Extrude context is stale; cancel and restart.");
    case Status::invalid_draft:
        return QStringLiteral(
            "Enter a positive extrusion distance.");
    case Status::missing_profile:
        return QStringLiteral(
            "Source Profile is missing.");
    case Status::missing_feature:
        return QStringLiteral(
            "Edited Feature is missing.");
    case Status::suppressed_feature:
        return QStringLiteral(
            "Suppressed Feature cannot be edited.");
    case Status::feature_id_exhausted:
        return QStringLiteral(
            "FeatureId allocation is exhausted.");
    case Status::invalid_candidate:
        return QStringLiteral(
            "Current Extrude candidate is invalid.");
    case Status::target_failed:
    case Status::ok:
        break;
    }
    return QStringLiteral(
        "Extrude preview is unavailable.");
}


QString featureEvaluationStatusText(
    part::FeatureEvaluationStatus status) {
    switch (status) {
    case part::FeatureEvaluationStatus::up_to_date:
        return QStringLiteral("UpToDate");
    case part::FeatureEvaluationStatus::failed:
        return QStringLiteral("Failed");
    case part::FeatureEvaluationStatus::blocked:
        return QStringLiteral("Blocked");
    case part::FeatureEvaluationStatus::suppressed:
        return QStringLiteral("Suppressed");
    }
    return QStringLiteral("Unknown");
}

QString bodyEvaluationStatusText(
    part::BodyEvaluationStatus status) {
    switch (status) {
    case part::BodyEvaluationStatus::empty:
        return QStringLiteral("Empty");
    case part::BodyEvaluationStatus::up_to_date:
        return QStringLiteral("UpToDate");
    case part::BodyEvaluationStatus::unavailable:
        return QStringLiteral("Unavailable");
    }
    return QStringLiteral("Unknown");
}

QString featureEvaluationDiagnosticText(
    part::FeatureEvaluationDiagnosticCode diagnostic) {
    switch (diagnostic) {
    case part::FeatureEvaluationDiagnosticCode::none:
        return QStringLiteral("—");
    case part::FeatureEvaluationDiagnosticCode::missing_profile:
        return QStringLiteral("Missing Profile");
    case part::FeatureEvaluationDiagnosticCode::unresolved_profile:
        return QStringLiteral("Unresolved Profile");
    case part::FeatureEvaluationDiagnosticCode::sketch_support_missing:
        return QStringLiteral("Sketch support missing");
    case part::FeatureEvaluationDiagnosticCode::sketch_support_ambiguous:
        return QStringLiteral("Sketch support ambiguous");
    case part::FeatureEvaluationDiagnosticCode::sketch_support_unsupported:
        return QStringLiteral("Sketch support unsupported");
    case part::FeatureEvaluationDiagnosticCode::missing_upstream_body:
        return QStringLiteral("Missing upstream Body");
    case part::FeatureEvaluationDiagnosticCode::upstream_unavailable:
        return QStringLiteral("Upstream Body unavailable");
    case part::FeatureEvaluationDiagnosticCode::kernel_invalid_input:
        return QStringLiteral("Kernel invalid input");
    case part::FeatureEvaluationDiagnosticCode::kernel_provider_mismatch:
        return QStringLiteral("Kernel provider mismatch");
    case part::FeatureEvaluationDiagnosticCode::kernel_provider_failure:
        return QStringLiteral("Kernel provider failure");
    case part::FeatureEvaluationDiagnosticCode::invalid_brep:
        return QStringLiteral("Invalid B-Rep");
    case part::FeatureEvaluationDiagnosticCode::detached_add:
        return QStringLiteral("Detached Add");
    case part::FeatureEvaluationDiagnosticCode::no_effect:
        return QStringLiteral("No effect");
    case part::FeatureEvaluationDiagnosticCode::empty_result:
        return QStringLiteral("Empty result");
    case part::FeatureEvaluationDiagnosticCode::multi_solid:
        return QStringLiteral("Multi-solid result");
    case part::FeatureEvaluationDiagnosticCode::topology_integrity_failure:
        return QStringLiteral("Topology integrity failure");
    }
    return QStringLiteral("Unknown");
}

QString axisEvaluationStatusText(
    part::AxisEvaluationStatus status) {
    switch (status) {
    case part::AxisEvaluationStatus::resolved:
        return QStringLiteral("Resolved");
    case part::AxisEvaluationStatus::missing:
        return QStringLiteral("Missing");
    case part::AxisEvaluationStatus::ambiguous:
        return QStringLiteral("Ambiguous");
    case part::AxisEvaluationStatus::unsupported:
        return QStringLiteral("Unsupported");
    case part::AxisEvaluationStatus::blocked:
        return QStringLiteral("Blocked");
    }
    return QStringLiteral("Unknown");
}

QString axisEvaluationDiagnosticText(
    part::AxisEvaluationDiagnostic diagnostic) {
    switch (diagnostic) {
    case part::AxisEvaluationDiagnostic::none:
        return QStringLiteral("—");
    case part::AxisEvaluationDiagnostic::invalid_reference:
        return QStringLiteral("Invalid reference");
    case part::AxisEvaluationDiagnostic::missing_axis:
        return QStringLiteral("Missing Axis");
    case part::AxisEvaluationDiagnostic::missing_sketch:
        return QStringLiteral("Missing source Sketch");
    case part::AxisEvaluationDiagnostic::missing_line:
        return QStringLiteral("Missing source Line");
    case part::AxisEvaluationDiagnostic::source_not_line:
        return QStringLiteral("Source is not a Line");
    case part::AxisEvaluationDiagnostic::sketch_support_missing:
        return QStringLiteral("Sketch support missing");
    case part::AxisEvaluationDiagnostic::sketch_support_ambiguous:
        return QStringLiteral("Sketch support ambiguous");
    case part::AxisEvaluationDiagnostic::sketch_support_unsupported:
        return QStringLiteral("Sketch support unsupported");
    case part::AxisEvaluationDiagnostic::sketch_support_blocked:
        return QStringLiteral("Sketch support blocked");
    case part::AxisEvaluationDiagnostic::stale_part_evaluation:
        return QStringLiteral("Stale Part evaluation");
    case part::AxisEvaluationDiagnostic::stale_datum_evaluation:
        return QStringLiteral("Stale Datum evaluation");
    case part::AxisEvaluationDiagnostic::support_stage_unavailable:
        return QStringLiteral("Support stage unavailable");
    case part::AxisEvaluationDiagnostic::invalid_frame:
        return QStringLiteral("Invalid frame");
    }
    return QStringLiteral("Unknown");
}

QString datumEvaluationStatusText(
    part::DatumPlaneEvaluationStatus status) {
    switch (status) {
    case part::DatumPlaneEvaluationStatus::resolved:
        return QStringLiteral("Resolved");
    case part::DatumPlaneEvaluationStatus::missing:
        return QStringLiteral("Missing");
    case part::DatumPlaneEvaluationStatus::ambiguous:
        return QStringLiteral("Ambiguous");
    case part::DatumPlaneEvaluationStatus::unsupported:
        return QStringLiteral("Unsupported");
    case part::DatumPlaneEvaluationStatus::blocked:
        return QStringLiteral("Blocked");
    }
    return QStringLiteral("Unknown");
}

QString datumEvaluationDiagnosticText(
    part::DatumPlaneEvaluationDiagnostic diagnostic) {
    switch (diagnostic) {
    case part::DatumPlaneEvaluationDiagnostic::none:
        return QStringLiteral("—");
    case part::DatumPlaneEvaluationDiagnostic::invalid_datum:
        return QStringLiteral("Invalid Datum");
    case part::DatumPlaneEvaluationDiagnostic::stale_part_evaluation:
        return QStringLiteral("Stale Part evaluation");
    case part::DatumPlaneEvaluationDiagnostic::body_stage_unavailable:
        return QStringLiteral("Body stage unavailable");
    case part::DatumPlaneEvaluationDiagnostic::missing_surface:
        return QStringLiteral("Missing Surface");
    case part::DatumPlaneEvaluationDiagnostic::ambiguous_surface:
        return QStringLiteral("Ambiguous Surface");
    case part::DatumPlaneEvaluationDiagnostic::unsupported_surface:
        return QStringLiteral("Unsupported Surface");
    case part::DatumPlaneEvaluationDiagnostic::unsupported_non_planar:
        return QStringLiteral("Non-planar Surface");
    case part::DatumPlaneEvaluationDiagnostic::missing_datum:
        return QStringLiteral("Missing Datum");
    case part::DatumPlaneEvaluationDiagnostic::cyclic_dependency:
        return QStringLiteral("Cyclic dependency");
    case part::DatumPlaneEvaluationDiagnostic::upstream_datum_unavailable:
        return QStringLiteral("Upstream Datum unavailable");
    case part::DatumPlaneEvaluationDiagnostic::invalid_frame:
        return QStringLiteral("Invalid frame");
    }
    return QStringLiteral("Unknown");
}

QString datumPlaneSourceText(
    const part::PlaneReference& source) {
    if (const auto origin =
            part::builtinOriginPlaneForPlaneReference(
                source)) {
        switch (*origin) {
        case core::BuiltinReferenceRole::xy_plane:
            return QStringLiteral("XY Plane");
        case core::BuiltinReferenceRole::xz_plane:
            return QStringLiteral("XZ Plane");
        case core::BuiltinReferenceRole::yz_plane:
            return QStringLiteral("YZ Plane");
        default:
            return QStringLiteral("<invalid Origin plane>");
        }
    }
    if (const auto* surface =
            part::bodyPlanarSurfaceForPlaneReference(
                source)) {
        return QStringLiteral("Body Surface @ Feature %1")
            .arg(
                fromUtf8(
                    surface->surface
                        .producer_feature_id
                        .serialized()));
    }
    if (const auto datum =
            part::datumPlaneIdForPlaneReference(
                source)) {
        return QStringLiteral("Datum Plane %1")
            .arg(
                fromUtf8(
                    datum->serialized()));
    }
    return QStringLiteral("<invalid source>");
}


QString formatLengthForPart(
    core::LengthValue value,
    core::LengthUnit unit) {
    return QStringLiteral("%1 %2")
        .arg(
            QString::number(
                core::fromCanonicalLength(
                    value,
                    unit),
                'g',
                12),
            QString::fromLatin1(
            core::lengthUnitSuffix(unit).data(),
            static_cast<qsizetype>(
                core::lengthUnitSuffix(unit).size())));
}

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

[[nodiscard]] part::ProfileVisibilityPolicy
profileVisibilityForCheckState(
    Qt::CheckState state) noexcept {
    switch (state) {
    case Qt::Checked:
        return part::ProfileVisibilityPolicy::force_shown;
    case Qt::Unchecked:
        return part::ProfileVisibilityPolicy::force_hidden;
    case Qt::PartiallyChecked:
        return part::ProfileVisibilityPolicy::automatic;
    }
    return part::ProfileVisibilityPolicy::automatic;
}

[[nodiscard]] Qt::CheckState
checkStateForProfileVisibility(
    part::ProfileVisibilityPolicy policy) noexcept {
    switch (policy) {
    case part::ProfileVisibilityPolicy::automatic:
        return Qt::PartiallyChecked;
    case part::ProfileVisibilityPolicy::force_shown:
        return Qt::Checked;
    case part::ProfileVisibilityPolicy::force_hidden:
        return Qt::Unchecked;
    }
    return Qt::PartiallyChecked;
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

QString referenceStatusText(
    kernel::ReferenceStatus status) {
    switch (status) {
    case kernel::ReferenceStatus::resolved:
        return QStringLiteral("Resolved");
    case kernel::ReferenceStatus::missing:
        return QStringLiteral("Missing");
    case kernel::ReferenceStatus::ambiguous:
        return QStringLiteral("Ambiguous");
    case kernel::ReferenceStatus::unsupported:
        return QStringLiteral("Unsupported");
    }
    return QStringLiteral("Unknown");
}

QString topologyAccountingText(
    part::TopologyAccountingClass value) {
    switch (value) {
    case part::TopologyAccountingClass::referenceable:
        return QStringLiteral("Referenceable");
    case part::TopologyAccountingClass::
        known_representation_artifact:
        return QStringLiteral("Representation Artifact");
    case part::TopologyAccountingClass::
        semantically_unsupported:
        return QStringLiteral("Semantically Unsupported");
    case part::TopologyAccountingClass::
        integrity_failure:
        return QStringLiteral("Integrity Failure");
    }
    return QStringLiteral("Unknown");
}

QString topologyKindText(
    viewer::BodyTopologyPresentationKind kind) {
    switch (kind) {
    case viewer::BodyTopologyPresentationKind::face:
        return QStringLiteral("Face");
    case viewer::BodyTopologyPresentationKind::edge:
        return QStringLiteral("Edge");
    case viewer::BodyTopologyPresentationKind::vertex:
        return QStringLiteral("Vertex");
    }
    return QStringLiteral("Unknown");
}

QString surfaceKindText(
    kernel::SurfaceKind kind) {
    switch (kind) {
    case kernel::SurfaceKind::plane:
        return QStringLiteral("Plane");
    case kernel::SurfaceKind::cylinder:
        return QStringLiteral("Cylinder");
    case kernel::SurfaceKind::cone:
        return QStringLiteral("Cone");
    case kernel::SurfaceKind::sphere:
        return QStringLiteral("Sphere");
    case kernel::SurfaceKind::torus:
        return QStringLiteral("Torus");
    case kernel::SurfaceKind::other:
        return QStringLiteral("Other");
    }
    return QStringLiteral("Unknown");
}

QString curveKindText(
    kernel::CurveKind kind) {
    switch (kind) {
    case kernel::CurveKind::line:
        return QStringLiteral("Line");
    case kernel::CurveKind::circle:
        return QStringLiteral("Circle");
    case kernel::CurveKind::other:
        return QStringLiteral("Other");
    }
    return QStringLiteral("Unknown");
}

QString stageText(
    const part::BodyStageRef& stage) {
    if (!stage.valid()) {
        return QStringLiteral("Invalid");
    }
    switch (stage.kind) {
    case part::BodyStageKind::empty_body:
        return QStringLiteral("Empty Body");
    case part::BodyStageKind::after_feature:
        return stage.feature_id
            ? QStringLiteral("After Feature %1")
                  .arg(QString::fromStdString(
                      stage.feature_id->serialized()))
            : QStringLiteral("After Feature ?");
    }
    return QStringLiteral("Unknown");
}

QString surfaceRoleText(
    part::FeatureSurfaceRoleKind role) {
    switch (role) {
    case part::FeatureSurfaceRoleKind::profile_cap:
        return QStringLiteral("Profile Cap");
    case part::FeatureSurfaceRoleKind::extent_cap:
        return QStringLiteral("Extent Cap");
    case part::FeatureSurfaceRoleKind::negative_cap:
        return QStringLiteral("Negative Cap");
    case part::FeatureSurfaceRoleKind::positive_cap:
        return QStringLiteral("Positive Cap");
    case part::FeatureSurfaceRoleKind::side:
        return QStringLiteral("Side");
    }
    return QStringLiteral("Unknown");
}

QString surfaceAddressText(
    const part::FeatureSurfaceAddress& address) {
    QString result =
        QStringLiteral("Feature %1 / %2")
            .arg(
                QString::fromStdString(
                    address.producer_feature_id
                        .serialized()),
                surfaceRoleText(address.role));
    if (address.source_entity) {
        result +=
            QStringLiteral(" / Entity %1")
                .arg(QString::fromStdString(
                    address.source_entity->serialized()));
    }
    return result;
}

QString adjacentSurfaceSummary(
    const std::vector<
        part::FeatureSurfaceAddress>& surfaces) {
    if (surfaces.empty()) {
        return QStringLiteral("—");
    }
    QStringList items;
    items.reserve(
        static_cast<qsizetype>(
            surfaces.size()));
    for (const auto& surface : surfaces) {
        items.push_back(
            surfaceAddressText(surface));
    }
    return items.join(
        QStringLiteral("; "));
}

QString sketchSupportInspectionText(
    SketchSupportInspectionCapability value) {
    switch (value) {
    case SketchSupportInspectionCapability::
        not_applicable:
        return QStringLiteral("Not applicable");
    case SketchSupportInspectionCapability::
        supported:
        return QStringLiteral("Supported");
    case SketchSupportInspectionCapability::
        unsupported_non_planar:
        return QStringLiteral(
            "Unsupported — non-planar");
    case SketchSupportInspectionCapability::
        missing:
        return QStringLiteral("Missing");
    case SketchSupportInspectionCapability::
        ambiguous:
        return QStringLiteral("Ambiguous");
    case SketchSupportInspectionCapability::
        unsupported:
        return QStringLiteral("Unsupported");
    }
    return QStringLiteral("Unknown");
}

QString topologySummaryText(
    const TopologyKindSummary& summary) {
    return QStringLiteral(
               "%1 total; %2 referenceable; "
               "%3 artifacts; %4 unsupported; "
               "%5 integrity failures")
        .arg(
            static_cast<qulonglong>(summary.total))
        .arg(
            static_cast<qulonglong>(
                summary.referenceable))
        .arg(
            static_cast<qulonglong>(
                summary.representation_artifact))
        .arg(
            static_cast<qulonglong>(
                summary.semantically_unsupported))
        .arg(
            static_cast<qulonglong>(
                summary.integrity_failure));
}

QString providerPointText(
    const std::optional<kernel::Point3>& point) {
    if (!point) {
        return QStringLiteral("—");
    }
    return QStringLiteral("XYZ = (%1, %2, %3)")
        .arg(QString::number(point->x, 'g', 12))
        .arg(QString::number(point->y, 'g', 12))
        .arg(QString::number(point->z, 'g', 12));
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
    : CadWorkbench{
          std::move(viewport_factory),
          nullptr,
          parent} {}

CadWorkbench::CadWorkbench(
    ViewportFactory viewport_factory,
    kernel::ISolidModelingKernel*
        solid_modeling_kernel,
    QWidget* parent)
    : QWidget{parent},
      viewport_factory_{std::move(viewport_factory)},
      solid_modeling_kernel_{
          solid_modeling_kernel} {
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
                                 "Selected references shown.")
                           : QStringLiteral(
                                 "Selected references hidden."))
                    : QStringLiteral(
                          "No reference visibility change."));
        });

    tree_controller_->setSketchEditHandler(
        [this](const sketch::SketchId& sketch_id) {
            requestEditSketch(sketch_id);
        });
    tree_controller_->setSketchSupportChangeHandler(
        [this](const sketch::SketchId& sketch_id) {
            startSketchResupport(sketch_id);
        });
    tree_controller_->setAxisEditHandler(
        [this](part::AxisId axis_id) {
            static_cast<void>(
                startAxisEdit(axis_id));
        });
    tree_controller_->setAxisDeleteHandler(
        [this](part::AxisId axis_id) {
            deleteAxis(axis_id);
        });
    tree_controller_->setDatumEditHandler(
        [this](part::DatumId datum_id) {
            static_cast<void>(
                startDatumPlaneEdit(datum_id));
        });
    tree_controller_->setDatumDeleteHandler(
        [this](part::DatumId datum_id) {
            deleteDatumPlane(datum_id);
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

    extrude_button_ =
        new QPushButton(
            QStringLiteral("Extrude"),
            shell_);
    extrude_button_->setObjectName(
        QStringLiteral("extrudeToolButton"));
    extrude_button_->setCheckable(true);
    shell_->editorToolsLayout().insertWidget(
        1,
        extrude_button_);

    datum_plane_button_ =
        new QPushButton(
            QStringLiteral("Datum Plane"),
            shell_);
    datum_plane_button_->setObjectName(
        QStringLiteral("datumPlaneToolButton"));
    datum_plane_button_->setCheckable(true);
    shell_->editorToolsLayout().insertWidget(
        1,
        datum_plane_button_);

    axis_button_ =
        new QPushButton(
            QStringLiteral("Axis"),
            shell_);
    axis_button_->setObjectName(
        QStringLiteral("axisToolButton"));
    axis_button_->setCheckable(true);
    shell_->editorToolsLayout().insertWidget(
        1,
        axis_button_);

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

    trim_sketch_button_ =
        new QPushButton(
            QStringLiteral("Trim"),
            shell_);
    trim_sketch_button_->setObjectName(
        QStringLiteral("trimSketchToolButton"));
    trim_sketch_button_->setCheckable(true);
    shell_->editorToolsLayout().insertWidget(
        9,
        trim_sketch_button_);

    extend_sketch_button_ =
        new QPushButton(
            QStringLiteral("Extend"),
            shell_);
    extend_sketch_button_->setObjectName(
        QStringLiteral("extendSketchToolButton"));
    extend_sketch_button_->setCheckable(true);
    shell_->editorToolsLayout().insertWidget(
        10,
        extend_sketch_button_);

    extend_both_sketch_button_ =
        new QPushButton(
            QStringLiteral("Extend Both"),
            shell_);
    extend_both_sketch_button_->setObjectName(
        QStringLiteral("extendBothSketchToolButton"));
    extend_both_sketch_button_->setCheckable(true);
    shell_->editorToolsLayout().insertWidget(
        11,
        extend_both_sketch_button_);

    move_sketch_button_ =
        new QPushButton(
            QStringLiteral("Move"),
            shell_);
    move_sketch_button_->setObjectName(
        QStringLiteral("moveSketchToolButton"));
    move_sketch_button_->setCheckable(true);
    shell_->editorToolsLayout().insertWidget(
        12,
        move_sketch_button_);

    copy_sketch_button_ =
        new QPushButton(
            QStringLiteral("Copy"),
            shell_);
    copy_sketch_button_->setObjectName(
        QStringLiteral("copySketchToolButton"));
    copy_sketch_button_->setCheckable(true);
    shell_->editorToolsLayout().insertWidget(
        13,
        copy_sketch_button_);

    rotate_sketch_button_ =
        new QPushButton(
            QStringLiteral("Rotate"),
            shell_);
    rotate_sketch_button_->setObjectName(
        QStringLiteral("rotateSketchToolButton"));
    rotate_sketch_button_->setCheckable(true);
    shell_->editorToolsLayout().insertWidget(
        14,
        rotate_sketch_button_);

    scale_sketch_button_ =
        new QPushButton(
            QStringLiteral("Scale"),
            shell_);
    scale_sketch_button_->setObjectName(
        QStringLiteral("scaleSketchToolButton"));
    scale_sketch_button_->setCheckable(true);
    shell_->editorToolsLayout().insertWidget(
        15,
        scale_sketch_button_);

    mirror_sketch_button_ =
        new QPushButton(
            QStringLiteral("Mirror"),
            shell_);
    mirror_sketch_button_->setObjectName(
        QStringLiteral("mirrorSketchToolButton"));
    mirror_sketch_button_->setCheckable(true);
    shell_->editorToolsLayout().insertWidget(
        16,
        mirror_sketch_button_);

    inspect_tools_label_ =
        new QLabel(
            QStringLiteral("Inspect:"),
            shell_);
    inspect_tools_label_->setObjectName(
        QStringLiteral("sketchInspectToolsLabel"));
    shell_->editorToolsLayout().insertWidget(
        17,
        inspect_tools_label_);

    measure_sketch_button_ =
        new QPushButton(
            QStringLiteral("Measure"),
            shell_);
    measure_sketch_button_->setObjectName(
        QStringLiteral("measureSketchToolButton"));
    measure_sketch_button_->setCheckable(true);
    shell_->editorToolsLayout().insertWidget(
        18,
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
    viewport_controller_->
        setSolidModelingKernel(
            solid_modeling_kernel_);
    viewport_controller_->setSelectionChangedHandler(
        [this](
            const std::vector<core::BuiltinReferenceRole>&,
            std::optional<core::BuiltinReferenceRole> primary) {
            refreshPropertiesContext(primary);
            tryCreateSketchFromSupport(primary);
            tryStageDatumPlaneFromSupport(primary);
        });

    viewport_controller_->setBodyTopologySelectionChangedHandler(
        [this](std::optional<BodyTopologyInspection> inspection) {
            if (!inspection) {
                syncActionState();
                return;
            }
            selected_profile_id_.reset();
            selected_datum_id_.reset();
            selected_feature_id_.reset();
            selected_body_id_.reset();
            if (document_tree_ != nullptr) {
                const QSignalBlocker blocked{
                    document_tree_};
                document_tree_->clearSelection();
                document_tree_->setCurrentItem(nullptr);
            }
            if (viewport_controller_) {
                viewport_controller_->
                    setFeatureContributionSelection(
                        std::nullopt);
            }
            refreshTopologyProperties(*inspection);
            if (sketch_support_pick_active_) {
                tryCreateSketchFromBodyTopology(
                    *inspection);
            }
            tryStageDatumPlaneFromBodyTopology(
                *inspection);
            syncActionState();
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
            tryStageAxisFromSketchSelection();
            if (document_session_ != nullptr) {
                const auto revision =
                    document_session_->document()
                        .revision();
                if (!part_evaluation_revision_ ||
                    *part_evaluation_revision_ !=
                        revision) {
                    refreshPartFeatureEvaluationSnapshot();
                }
            }
            syncActionState();
            notifyDocumentStateChanged();
        });
    sketch_interaction_controller_->setStatusHandler(
        [this](const std::string& message) {
            setStatusText(fromUtf8(message));
        });

    tree_controller_->setAxisSelectionHandler(
        [this](
            const std::vector<part::AxisId>& selected,
            std::optional<part::AxisId> primary) {
            if (viewport_controller_ != nullptr) {
                viewport_controller_->
                    setAxisSelectionFromTree(
                        selected,
                        primary);
            }
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
                selected_axis_id_.reset();
                selected_datum_id_.reset();
                selected_feature_id_.reset();
                selected_body_id_.reset();
                refreshProfileProperties(*primary);
            } else if (selected_profile_id_) {
                selected_profile_id_.reset();
                if (properties_stack_ != nullptr) {
                    properties_stack_->setCurrentWidget(
                        document_properties_page_);
                }
            }
            syncSketchInteractionUi();
            if (semantic) {
                tryCompleteExtrudeProfilePick();
            }
            syncActionState();
        });
    viewport_controller_->setAxisSelectionChangedHandler(
        [this](
            const std::vector<part::AxisId>& selected,
            std::optional<part::AxisId> primary) {
            const auto semantic =
                selected.size() == 1U && primary
                    ? primary
                    : std::nullopt;
            selected_axis_id_ = semantic;
            if (semantic) {
                selected_profile_id_.reset();
                selected_datum_id_.reset();
                selected_feature_id_.reset();
                selected_body_id_.reset();
                refreshAxisProperties(*semantic);
            } else if (properties_stack_ != nullptr &&
                       properties_stack_->currentWidget() ==
                           axis_properties_page_) {
                properties_stack_->setCurrentWidget(
                    document_properties_page_);
            }
            syncActionState();
        });

    viewport_controller_->setDatumSelectionChangedHandler(
        [this](
            const std::vector<part::DatumId>& selected,
            std::optional<part::DatumId> primary) {
            const auto semantic =
                selected.size() == 1U && primary
                    ? primary
                    : std::nullopt;
            selected_datum_id_ = semantic;
            if (semantic) {
                selected_profile_id_.reset();
                selected_axis_id_.reset();
                selected_feature_id_.reset();
                selected_body_id_.reset();
                refreshDatumProperties(*semantic);
                tryCreateSketchFromDatum(semantic);
                tryStageDatumPlaneFromDatum(semantic);
            } else if (properties_stack_ != nullptr &&
                       properties_stack_->currentWidget() ==
                           datum_properties_page_) {
                properties_stack_->setCurrentWidget(
                    document_properties_page_);
            }
            syncActionState();
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
                selected_feature_id_.reset();
                selected_body_id_.reset();
                refreshProfileProperties(*primary);
            } else if (selected_profile_id_) {
                selected_profile_id_.reset();
                if (properties_stack_ != nullptr) {
                    properties_stack_->setCurrentWidget(
                        document_properties_page_);
                }
            }
            syncSketchInteractionUi();
            if (semantic) {
                tryCompleteExtrudeProfilePick();
            }
            syncActionState();
        });
    tree_controller_->setProfileEditHandler(
        [this](part::ProfileId profile_id) {
            requestEditProfile(profile_id);
        });
    tree_controller_->setBodySelectionHandler(
        [this](std::optional<part::BodyId> body_id) {
            selected_body_id_ = body_id;
            if (body_id) {
                selected_axis_id_.reset();
                selected_datum_id_.reset();
                refreshBodyProperties(*body_id);
            }
        });
    tree_controller_->setFeatureSelectionHandler(
        [this](
            const std::vector<part::FeatureId>& selected,
            std::optional<part::FeatureId> primary) {
            const auto semantic =
                selected.size() == 1U && primary
                    ? primary
                    : std::nullopt;
            selected_feature_id_ = semantic;
            if (semantic) {
                selected_axis_id_.reset();
                selected_datum_id_.reset();
            }
            if (viewport_controller_) {
                viewport_controller_->
                    setFeatureContributionSelection(
                        semantic);
            }
            if (semantic) {
                refreshFeatureProperties(*semantic);
            }
            syncActionState();
        });
    tree_controller_->setFeatureHoverHandler(
        [this](std::optional<part::FeatureId> feature_id) {
            if (viewport_controller_) {
                viewport_controller_->
                    setFeatureContributionHover(
                        std::move(feature_id));
            }
        });
    tree_controller_->setFeatureEditHandler(
        [this](part::FeatureId feature_id) {
            static_cast<void>(
                startExtrudeEdit(feature_id));
        });
    tree_controller_->setFeatureSuppressionHandler(
        [this](
            part::FeatureId feature_id,
            bool suppressed) {
            setFeatureSuppressed(
                feature_id,
                suppressed);
        });
    tree_controller_->setFeatureDeleteHandler(
        [this](part::FeatureId feature_id) {
            deleteFeature(feature_id);
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

    axis_properties_page_ =
        new QWidget(properties_stack_);
    axis_properties_page_->setObjectName(
        QStringLiteral("axisPropertiesPage"));
    auto* axis_properties_root =
        new QFormLayout(axis_properties_page_);

    axis_name_ =
        new QLabel(axis_properties_page_);
    axis_name_->setObjectName(
        QStringLiteral("axisPropertyName"));
    axis_identity_ =
        new QLabel(axis_properties_page_);
    axis_identity_->setObjectName(
        QStringLiteral("axisPropertyIdentity"));
    axis_source_sketch_ =
        new QLabel(axis_properties_page_);
    axis_source_sketch_->setObjectName(
        QStringLiteral("axisPropertySourceSketch"));
    axis_source_line_ =
        new QLabel(axis_properties_page_);
    axis_source_line_->setObjectName(
        QStringLiteral("axisPropertySourceLine"));
    axis_visibility_ =
        new QLabel(axis_properties_page_);
    axis_visibility_->setObjectName(
        QStringLiteral("axisPropertyVisibility"));
    axis_status_ =
        new QLabel(axis_properties_page_);
    axis_status_->setObjectName(
        QStringLiteral("axisPropertyStatus"));
    axis_diagnostic_ =
        new QLabel(axis_properties_page_);
    axis_diagnostic_->setObjectName(
        QStringLiteral("axisPropertyDiagnostic"));
    axis_diagnostic_->setWordWrap(true);
    axis_origin_ =
        new QLabel(axis_properties_page_);
    axis_origin_->setObjectName(
        QStringLiteral("axisPropertyOrigin"));
    axis_direction_ =
        new QLabel(axis_properties_page_);
    axis_direction_->setObjectName(
        QStringLiteral("axisPropertyDirection"));

    axis_edit_button_ =
        new QPushButton(
            QStringLiteral("Edit Axis"),
            axis_properties_page_);
    axis_edit_button_->setObjectName(
        QStringLiteral("editAxisPropertyButton"));
    axis_delete_button_ =
        new QPushButton(
            QStringLiteral("Delete Axis"),
            axis_properties_page_);
    axis_delete_button_->setObjectName(
        QStringLiteral("deleteAxisPropertyButton"));

    axis_properties_root->addRow(
        QStringLiteral("Name"),
        axis_name_);
    axis_properties_root->addRow(
        QStringLiteral("AxisId"),
        axis_identity_);
    axis_properties_root->addRow(
        QStringLiteral("Source Sketch"),
        axis_source_sketch_);
    axis_properties_root->addRow(
        QStringLiteral("Source Line"),
        axis_source_line_);
    axis_properties_root->addRow(
        QStringLiteral("Visibility"),
        axis_visibility_);
    axis_properties_root->addRow(
        QStringLiteral("Status"),
        axis_status_);
    axis_properties_root->addRow(
        QStringLiteral("Diagnostic"),
        axis_diagnostic_);
    axis_properties_root->addRow(
        QStringLiteral("Origin"),
        axis_origin_);
    axis_properties_root->addRow(
        QStringLiteral("Direction"),
        axis_direction_);
    axis_properties_root->addRow(
        axis_edit_button_);
    axis_properties_root->addRow(
        axis_delete_button_);

    properties_stack_->addWidget(
        axis_properties_page_);

    datum_properties_page_ =
        new QWidget(properties_stack_);
    datum_properties_page_->setObjectName(
        QStringLiteral("datumPropertiesPage"));
    auto* datum_properties_root =
        new QFormLayout(datum_properties_page_);

    datum_name_ =
        new QLabel(datum_properties_page_);
    datum_name_->setObjectName(
        QStringLiteral("datumPropertyName"));
    datum_identity_ =
        new QLabel(datum_properties_page_);
    datum_identity_->setObjectName(
        QStringLiteral("datumPropertyIdentity"));
    datum_identity_->setWordWrap(true);
    datum_constructor_ =
        new QLabel(
            QStringLiteral("Offset"),
            datum_properties_page_);
    datum_constructor_->setObjectName(
        QStringLiteral("datumPropertyConstructor"));
    datum_source_ =
        new QLabel(datum_properties_page_);
    datum_source_->setObjectName(
        QStringLiteral("datumPropertySource"));
    datum_source_->setWordWrap(true);
    datum_offset_ =
        new QLabel(datum_properties_page_);
    datum_offset_->setObjectName(
        QStringLiteral("datumPropertyOffset"));
    datum_visibility_ =
        new QLabel(datum_properties_page_);
    datum_visibility_->setObjectName(
        QStringLiteral("datumPropertyVisibility"));
    datum_status_ =
        new QLabel(datum_properties_page_);
    datum_status_->setObjectName(
        QStringLiteral("datumPropertyStatus"));
    datum_diagnostic_ =
        new QLabel(datum_properties_page_);
    datum_diagnostic_->setObjectName(
        QStringLiteral("datumPropertyDiagnostic"));
    datum_diagnostic_->setWordWrap(true);

    datum_edit_button_ =
        new QPushButton(
            QStringLiteral("Edit Datum Plane"),
            datum_properties_page_);
    datum_edit_button_->setObjectName(
        QStringLiteral("editDatumPlanePropertyButton"));
    datum_delete_button_ =
        new QPushButton(
            QStringLiteral("Delete Datum Plane"),
            datum_properties_page_);
    datum_delete_button_->setObjectName(
        QStringLiteral("deleteDatumPlanePropertyButton"));

    datum_properties_root->addRow(
        QStringLiteral("Name"),
        datum_name_);
    datum_properties_root->addRow(
        QStringLiteral("DatumId"),
        datum_identity_);
    datum_properties_root->addRow(
        QStringLiteral("Constructor"),
        datum_constructor_);
    datum_properties_root->addRow(
        QStringLiteral("Source"),
        datum_source_);
    datum_properties_root->addRow(
        QStringLiteral("Offset"),
        datum_offset_);
    datum_properties_root->addRow(
        QStringLiteral("Visibility"),
        datum_visibility_);
    datum_properties_root->addRow(
        QStringLiteral("Status"),
        datum_status_);
    datum_properties_root->addRow(
        QStringLiteral("Diagnostic"),
        datum_diagnostic_);
    datum_properties_root->addRow(
        datum_edit_button_);
    datum_properties_root->addRow(
        datum_delete_button_);

    properties_stack_->addWidget(
        datum_properties_page_);

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

    profile_consuming_features_ =
        new QComboBox(
            profile_properties_page_);
    profile_consuming_features_->setObjectName(
        QStringLiteral(
            "profileConsumingFeaturesCombo"));
    profile_go_to_feature_button_ =
        new QPushButton(
            QStringLiteral("Go to Feature"),
            profile_properties_page_);
    profile_go_to_feature_button_->setObjectName(
        QStringLiteral(
            "profileGoToFeatureButton"));

    profile_visible_ =
        new QCheckBox(
            QStringLiteral("Visible"),
            profile_properties_page_);
    profile_visible_->setObjectName(
        QStringLiteral("profilePropertyVisible"));
    profile_visible_->setTristate(true);
    profile_visible_->setCheckState(
        Qt::PartiallyChecked);
    profile_visible_->setToolTip(
        QStringLiteral(
            "Partially checked = Automatic; checked = Force Shown; unchecked = Force Hidden"));

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
        QStringLiteral("Consuming Features"),
        profile_consuming_features_);
    profile_root->addRow(
        profile_go_to_feature_button_);
    profile_root->addRow(
        QStringLiteral("Visibility"),
        profile_visible_);
    profile_root->addRow(
        apply_profile_button_);
    profile_root->addRow(
        delete_profile_button_);

    properties_stack_->addWidget(
        profile_properties_page_);

    body_properties_page_ =
        new QWidget(properties_stack_);
    body_properties_page_->setObjectName(
        QStringLiteral("bodyPropertiesPage"));
    auto* body_root =
        new QFormLayout(body_properties_page_);
    body_root->setContentsMargins(0, 0, 0, 0);

    body_identity_ =
        new QLabel(body_properties_page_);
    body_identity_->setObjectName(
        QStringLiteral("bodyPropertyIdentity"));
    body_status_ =
        new QLabel(body_properties_page_);
    body_status_->setObjectName(
        QStringLiteral("bodyPropertyStatus"));
    body_feature_count_ =
        new QLabel(body_properties_page_);
    body_feature_count_->setObjectName(
        QStringLiteral("bodyPropertyFeatureCount"));
    body_topology_counts_ =
        new QLabel(body_properties_page_);
    body_topology_counts_->setObjectName(
        QStringLiteral("bodyPropertyTopologyCounts"));
    body_topology_counts_->setWordWrap(true);
    body_topology_accounting_ =
        new QLabel(body_properties_page_);
    body_topology_accounting_->setObjectName(
        QStringLiteral("bodyPropertyTopologyAccounting"));
    body_topology_accounting_->setWordWrap(true);

    body_root->addRow(
        QStringLiteral("BodyId"),
        body_identity_);
    body_root->addRow(
        QStringLiteral("Status"),
        body_status_);
    body_root->addRow(
        QStringLiteral("Ordered Features"),
        body_feature_count_);
    body_root->addRow(
        QStringLiteral("Topology"),
        body_topology_counts_);
    body_root->addRow(
        QStringLiteral("Accounting"),
        body_topology_accounting_);
    properties_stack_->addWidget(
        body_properties_page_);

    feature_properties_page_ =
        new QWidget(properties_stack_);
    feature_properties_page_->setObjectName(
        QStringLiteral("featurePropertiesPage"));
    auto* feature_root =
        new QFormLayout(
            feature_properties_page_);
    feature_root->setContentsMargins(
        0, 0, 0, 0);

    feature_name_ =
        new QLabel(feature_properties_page_);
    feature_name_->setObjectName(
        QStringLiteral("featurePropertyName"));
    feature_identity_ =
        new QLabel(feature_properties_page_);
    feature_identity_->setObjectName(
        QStringLiteral("featurePropertyIdentity"));
    feature_status_ =
        new QLabel(feature_properties_page_);
    feature_status_->setObjectName(
        QStringLiteral("featurePropertyStatus"));
    feature_diagnostic_ =
        new QLabel(feature_properties_page_);
    feature_diagnostic_->setObjectName(
        QStringLiteral("featurePropertyDiagnostic"));
    feature_operation_ =
        new QLabel(feature_properties_page_);
    feature_operation_->setObjectName(
        QStringLiteral("featurePropertyOperation"));
    feature_extent_ =
        new QLabel(feature_properties_page_);
    feature_extent_->setObjectName(
        QStringLiteral("featurePropertyExtent"));
    feature_distance_ =
        new QLabel(feature_properties_page_);
    feature_distance_->setObjectName(
        QStringLiteral("featurePropertyDistance"));
    feature_direction_ =
        new QLabel(feature_properties_page_);
    feature_direction_->setObjectName(
        QStringLiteral("featurePropertyDirection"));
    feature_source_profile_ =
        new QLabel(feature_properties_page_);
    feature_source_profile_->setObjectName(
        QStringLiteral("featurePropertySourceProfile"));
    feature_source_sketch_ =
        new QLabel(feature_properties_page_);
    feature_source_sketch_->setObjectName(
        QStringLiteral("featurePropertySourceSketch"));
    feature_contribution_ =
        new QLabel(feature_properties_page_);
    feature_contribution_->setObjectName(
        QStringLiteral("featurePropertyContribution"));
    feature_contribution_->setWordWrap(true);
    feature_contribution_diagnostics_ =
        new QLabel(feature_properties_page_);
    feature_contribution_diagnostics_->setObjectName(
        QStringLiteral("featurePropertyContributionDiagnostics"));
    feature_contribution_diagnostics_->setWordWrap(true);

    feature_go_to_profile_button_ =
        new QPushButton(
            QStringLiteral("Go to Source Profile"),
            feature_properties_page_);
    feature_go_to_profile_button_->setObjectName(
        QStringLiteral(
            "featureGoToProfileButton"));
    feature_edit_button_ =
        new QPushButton(
            QStringLiteral("Edit Extrude"),
            feature_properties_page_);
    feature_edit_button_->setObjectName(
        QStringLiteral("featureEditExtrudeButton"));
    feature_suppress_button_ =
        new QPushButton(
            QStringLiteral("Suppress Feature"),
            feature_properties_page_);
    feature_suppress_button_->setObjectName(
        QStringLiteral("featureSuppressButton"));
    feature_delete_button_ =
        new QPushButton(
            QStringLiteral("Delete Feature"),
            feature_properties_page_);
    feature_delete_button_->setObjectName(
        QStringLiteral("featureDeleteButton"));

    feature_root->addRow(
        QStringLiteral("Name"),
        feature_name_);
    feature_root->addRow(
        QStringLiteral("FeatureId"),
        feature_identity_);
    feature_root->addRow(
        QStringLiteral("Status"),
        feature_status_);
    feature_root->addRow(
        QStringLiteral("Diagnostic"),
        feature_diagnostic_);
    feature_root->addRow(
        QStringLiteral("Operation"),
        feature_operation_);
    feature_root->addRow(
        QStringLiteral("Extent"),
        feature_extent_);
    feature_root->addRow(
        QStringLiteral("Distance"),
        feature_distance_);
    feature_root->addRow(
        QStringLiteral("Direction"),
        feature_direction_);
    feature_root->addRow(
        QStringLiteral("Source Profile"),
        feature_source_profile_);
    feature_root->addRow(
        QStringLiteral("Source Sketch"),
        feature_source_sketch_);
    feature_root->addRow(
        QStringLiteral("Current Contribution"),
        feature_contribution_);
    feature_root->addRow(
        QStringLiteral("Semantic Outputs"),
        feature_contribution_diagnostics_);
    feature_root->addRow(
        feature_go_to_profile_button_);
    feature_root->addRow(
        feature_edit_button_);
    feature_root->addRow(
        feature_suppress_button_);
    feature_root->addRow(
        feature_delete_button_);
    properties_stack_->addWidget(
        feature_properties_page_);

    topology_properties_page_ =
        new QWidget(properties_stack_);
    topology_properties_page_->setObjectName(
        QStringLiteral("topologyPropertiesPage"));
    auto* topology_root =
        new QFormLayout(topology_properties_page_);
    topology_root->setContentsMargins(0, 0, 0, 0);

    const auto make_topology_label =
        [this](const char* object_name) {
            auto* label =
                new QLabel(topology_properties_page_);
            label->setObjectName(
                QString::fromLatin1(object_name));
            label->setWordWrap(true);
            return label;
        };

    topology_kind_ =
        make_topology_label("topologyPropertyKind");
    topology_stage_ =
        make_topology_label("topologyPropertyStage");
    topology_presence_ =
        make_topology_label("topologyPropertyPresence");
    topology_accounting_ =
        make_topology_label("topologyPropertyAccounting");
    topology_strict_reference_ =
        make_topology_label("topologyPropertyStrictReference");
    topology_carrier_reference_ =
        make_topology_label("topologyPropertyCarrierReference");
    topology_carrier_ =
        make_topology_label("topologyPropertyCarrier");
    topology_carrier_type_ =
        make_topology_label("topologyPropertyCarrierType");
    topology_producer_ =
        make_topology_label("topologyPropertyProducer");
    topology_candidates_ =
        make_topology_label("topologyPropertyCandidates");
    topology_adjacency_ =
        make_topology_label("topologyPropertyAdjacency");
    topology_sketch_support_ =
        make_topology_label("topologyPropertySketchSupport");
    topology_geometry_ =
        make_topology_label("topologyPropertyGeometry");

    topology_root->addRow(
        QStringLiteral("Topology"),
        topology_kind_);
    topology_root->addRow(
        QStringLiteral("Stage"),
        topology_stage_);
    topology_root->addRow(
        QStringLiteral("Current State"),
        topology_presence_);
    topology_root->addRow(
        QStringLiteral("Accounting"),
        topology_accounting_);
    topology_root->addRow(
        QStringLiteral("Strict Reference"),
        topology_strict_reference_);
    topology_root->addRow(
        QStringLiteral("Carrier Reference"),
        topology_carrier_reference_);
    topology_root->addRow(
        QStringLiteral("Carrier"),
        topology_carrier_);
    topology_root->addRow(
        QStringLiteral("Carrier Type"),
        topology_carrier_type_);
    topology_root->addRow(
        QStringLiteral("Producer"),
        topology_producer_);
    topology_root->addRow(
        QStringLiteral("Semantic Candidates"),
        topology_candidates_);
    topology_root->addRow(
        QStringLiteral("Adjacency"),
        topology_adjacency_);
    topology_root->addRow(
        QStringLiteral("Sketch Support"),
        topology_sketch_support_);
    topology_root->addRow(
        QStringLiteral("Geometry (diagnostic)"),
        topology_geometry_);

    properties_stack_->addWidget(
        topology_properties_page_);

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

    object_snap_toggle_button_ =
        new QPushButton(
            precision_operations_widget_);
    object_snap_toggle_button_->setObjectName(
        QStringLiteral("objectSnapToggleButton"));
    object_snap_toggle_button_->setCheckable(true);
    precision_form->addRow(
        QStringLiteral("Object Snap"),
        object_snap_toggle_button_);

    auto* object_snap_modes =
        new QWidget(
            precision_operations_widget_);
    object_snap_modes->setObjectName(
        QStringLiteral("objectSnapModes"));
    auto* object_snap_modes_layout =
        new QGridLayout(object_snap_modes);
    object_snap_modes_layout->setContentsMargins(
        0, 0, 0, 0);
    object_snap_modes_layout->setHorizontalSpacing(8);
    object_snap_modes_layout->setVerticalSpacing(2);

    const auto make_snap_mode =
        [object_snap_modes,
         object_snap_modes_layout](
            const QString& text,
            const QString& object_name,
            int row,
            int column) {
            auto* check =
                new QCheckBox(
                    text,
                    object_snap_modes);
            check->setObjectName(object_name);
            object_snap_modes_layout->addWidget(
                check,
                row,
                column);
            return check;
        };

    object_snap_endpoint_check_ =
        make_snap_mode(
            QStringLiteral("END"),
            QStringLiteral("objectSnapEndpointCheck"),
            0, 0);
    object_snap_midpoint_check_ =
        make_snap_mode(
            QStringLiteral("MID"),
            QStringLiteral("objectSnapMidpointCheck"),
            0, 1);
    object_snap_center_check_ =
        make_snap_mode(
            QStringLiteral("CEN"),
            QStringLiteral("objectSnapCenterCheck"),
            0, 2);
    object_snap_quadrant_check_ =
        make_snap_mode(
            QStringLiteral("QUAD"),
            QStringLiteral("objectSnapQuadrantCheck"),
            0, 3);
    object_snap_intersection_check_ =
        make_snap_mode(
            QStringLiteral("INT"),
            QStringLiteral("objectSnapIntersectionCheck"),
            1, 0);
    object_snap_origin_check_ =
        make_snap_mode(
            QStringLiteral("ORG"),
            QStringLiteral("objectSnapOriginCheck"),
            1, 1);
    object_snap_perpendicular_check_ =
        make_snap_mode(
            QStringLiteral("PER"),
            QStringLiteral("objectSnapPerpendicularCheck"),
            1, 2);
    object_snap_tangent_check_ =
        make_snap_mode(
            QStringLiteral("TAN"),
            QStringLiteral("objectSnapTangentCheck"),
            1, 3);
    object_snap_nearest_check_ =
        make_snap_mode(
            QStringLiteral("NEA"),
            QStringLiteral("objectSnapNearestCheck"),
            2, 0);
    object_snap_extension_check_ =
        make_snap_mode(
            QStringLiteral("EXT"),
            QStringLiteral("objectSnapExtensionCheck"),
            2, 1);
    precision_form->addRow(
        QStringLiteral("Modes"),
        object_snap_modes);

    object_snap_override_combo_ =
        new QComboBox(
            precision_operations_widget_);
    object_snap_override_combo_->setObjectName(
        QStringLiteral("objectSnapOverrideCombo"));
    object_snap_override_combo_->addItem(
        QStringLiteral("Persistent"),
        -1);
    const auto add_override =
        [this](
            const QString& text,
            sketch::TemporarySnapOverrideKind value) {
            object_snap_override_combo_->addItem(
                text,
                static_cast<int>(value));
        };
    add_override(
        QStringLiteral("END"),
        sketch::TemporarySnapOverrideKind::endpoint);
    add_override(
        QStringLiteral("MID"),
        sketch::TemporarySnapOverrideKind::midpoint);
    add_override(
        QStringLiteral("CEN"),
        sketch::TemporarySnapOverrideKind::center);
    add_override(
        QStringLiteral("QUAD"),
        sketch::TemporarySnapOverrideKind::quadrant);
    add_override(
        QStringLiteral("INT"),
        sketch::TemporarySnapOverrideKind::intersection);
    add_override(
        QStringLiteral("ORG"),
        sketch::TemporarySnapOverrideKind::origin);
    add_override(
        QStringLiteral("PER"),
        sketch::TemporarySnapOverrideKind::perpendicular);
    add_override(
        QStringLiteral("TAN"),
        sketch::TemporarySnapOverrideKind::tangent);
    add_override(
        QStringLiteral("NEA"),
        sketch::TemporarySnapOverrideKind::nearest);
    add_override(
        QStringLiteral("EXT"),
        sketch::TemporarySnapOverrideKind::extension);
    add_override(
        QStringLiteral("NONE"),
        sketch::TemporarySnapOverrideKind::none);
    precision_form->addRow(
        QStringLiteral("Temporary"),
        object_snap_override_combo_);

    object_tracking_toggle_button_ =
        new QPushButton(
            precision_operations_widget_);
    object_tracking_toggle_button_->setObjectName(
        QStringLiteral("objectTrackingToggleButton"));
    object_tracking_toggle_button_->setCheckable(true);
    precision_form->addRow(
        QStringLiteral("Tracking"),
        object_tracking_toggle_button_);

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


    axis_operations_widget_ =
        new QWidget(operations_content);
    axis_operations_widget_->setObjectName(
        QStringLiteral("axisOperationsWidget"));
    auto* axis_operations_layout =
        new QVBoxLayout(
            axis_operations_widget_);
    axis_operations_layout->setContentsMargins(
        0, 0, 0, 0);

    auto* axis_source_form =
        new QFormLayout;
    axis_source_label_ =
        new QLabel(
            QStringLiteral("<select one Sketch Line>"),
            axis_operations_widget_);
    axis_source_label_->setObjectName(
        QStringLiteral("axisSourceLabel"));
    axis_source_label_->setWordWrap(true);
    axis_source_form->addRow(
        QStringLiteral("Source"),
        axis_source_label_);
    axis_operations_layout->addLayout(
        axis_source_form);

    axis_result_label_ =
        new QLabel(
            QStringLiteral("Select exactly one valid Sketch Line."),
            axis_operations_widget_);
    axis_result_label_->setObjectName(
        QStringLiteral("axisResultLabel"));
    axis_result_label_->setWordWrap(true);
    axis_operations_layout->addWidget(
        axis_result_label_);

    axis_finish_button_ =
        new QPushButton(
            QStringLiteral("Finish Axis"),
            axis_operations_widget_);
    axis_finish_button_->setObjectName(
        QStringLiteral("axisFinishButton"));
    axis_operations_layout->addWidget(
        axis_finish_button_);

    axis_cancel_button_ =
        new QPushButton(
            QStringLiteral("Cancel"),
            axis_operations_widget_);
    axis_cancel_button_->setObjectName(
        QStringLiteral("axisCancelButton"));
    axis_operations_layout->addWidget(
        axis_cancel_button_);

    axis_operations_widget_->setVisible(false);
    operations_layout->addWidget(
        axis_operations_widget_);

    datum_plane_operations_widget_ =
        new QWidget(operations_content);
    datum_plane_operations_widget_->setObjectName(
        QStringLiteral("datumPlaneOperationsWidget"));
    auto* datum_plane_operations_layout =
        new QVBoxLayout(
            datum_plane_operations_widget_);
    datum_plane_operations_layout->setContentsMargins(
        0, 0, 0, 0);

    auto* datum_plane_constructor_form =
        new QFormLayout;
    datum_plane_constructor_combo_ =
        new QComboBox(
            datum_plane_operations_widget_);
    datum_plane_constructor_combo_->setObjectName(
        QStringLiteral("datumPlaneConstructorCombo"));
    datum_plane_constructor_combo_->addItem(
        QStringLiteral("Offset"));
    datum_plane_constructor_form->addRow(
        QStringLiteral("Constructor"),
        datum_plane_constructor_combo_);
    datum_plane_operations_layout->addLayout(
        datum_plane_constructor_form);

    datum_plane_source_label_ =
        new QLabel(
            QStringLiteral("Source: Select XY/XZ/YZ plane or planar Body Face"),
            datum_plane_operations_widget_);
    datum_plane_source_label_->setObjectName(
        QStringLiteral("datumPlaneSourceLabel"));
    datum_plane_source_label_->setWordWrap(true);
    datum_plane_operations_layout->addWidget(
        datum_plane_source_label_);

    auto* datum_plane_offset_form =
        new QFormLayout;
    datum_plane_offset_edit_ =
        new QLineEdit(
            datum_plane_operations_widget_);
    datum_plane_offset_edit_->setObjectName(
        QStringLiteral("datumPlaneOffsetEdit"));
    datum_plane_offset_edit_->setPlaceholderText(
        QStringLiteral("e.g. 10 mm or -5 mm"));
    datum_plane_offset_form->addRow(
        QStringLiteral("Offset"),
        datum_plane_offset_edit_);
    datum_plane_operations_layout->addLayout(
        datum_plane_offset_form);

    datum_plane_reverse_button_ =
        new QPushButton(
            QStringLiteral("Reverse"),
            datum_plane_operations_widget_);
    datum_plane_reverse_button_->setObjectName(
        QStringLiteral("datumPlaneReverseButton"));
    datum_plane_operations_layout->addWidget(
        datum_plane_reverse_button_);

    datum_plane_result_label_ =
        new QLabel(
            QStringLiteral("Select a valid Datum Plane source."),
            datum_plane_operations_widget_);
    datum_plane_result_label_->setObjectName(
        QStringLiteral("datumPlaneResultLabel"));
    datum_plane_result_label_->setWordWrap(true);
    datum_plane_operations_layout->addWidget(
        datum_plane_result_label_);

    datum_plane_finish_button_ =
        new QPushButton(
            QStringLiteral("Finish Datum Plane"),
            datum_plane_operations_widget_);
    datum_plane_finish_button_->setObjectName(
        QStringLiteral("datumPlaneFinishButton"));
    datum_plane_operations_layout->addWidget(
        datum_plane_finish_button_);

    datum_plane_cancel_button_ =
        new QPushButton(
            QStringLiteral("Cancel"),
            datum_plane_operations_widget_);
    datum_plane_cancel_button_->setObjectName(
        QStringLiteral("datumPlaneCancelButton"));
    datum_plane_operations_layout->addWidget(
        datum_plane_cancel_button_);

    datum_plane_preview_timer_ =
        new QTimer(this);
    datum_plane_preview_timer_->setSingleShot(true);
    datum_plane_preview_timer_->setInterval(90);
    QObject::connect(
        datum_plane_preview_timer_,
        &QTimer::timeout,
        this,
        [this] {
            refreshDatumPlaneEvaluation();
        });

    datum_plane_operations_widget_->setVisible(false);
    operations_layout->addWidget(
        datum_plane_operations_widget_);


    extrude_operations_widget_ =
        new QWidget(operations_content);
    extrude_operations_widget_->setObjectName(
        QStringLiteral("extrudeOperationsWidget"));
    auto* extrude_operations_layout =
        new QVBoxLayout(
            extrude_operations_widget_);
    extrude_operations_layout->setContentsMargins(
        0, 0, 0, 0);

    auto* extrude_operation_row =
        new QWidget(
            extrude_operations_widget_);
    auto* extrude_operation_layout =
        new QHBoxLayout(
            extrude_operation_row);
    extrude_operation_layout->setContentsMargins(
        0, 0, 0, 0);
    extrude_add_button_ =
        new QPushButton(
            QStringLiteral("Add"),
            extrude_operation_row);
    extrude_add_button_->setObjectName(
        QStringLiteral("extrudeAddButton"));
    extrude_add_button_->setCheckable(true);
    extrude_cut_button_ =
        new QPushButton(
            QStringLiteral("Cut"),
            extrude_operation_row);
    extrude_cut_button_->setObjectName(
        QStringLiteral("extrudeCutButton"));
    extrude_cut_button_->setCheckable(true);
    extrude_operation_layout->addWidget(
        extrude_add_button_);
    extrude_operation_layout->addWidget(
        extrude_cut_button_);
    extrude_operations_layout->addWidget(
        extrude_operation_row);

    auto* extrude_extent_row =
        new QWidget(
            extrude_operations_widget_);
    auto* extrude_extent_layout =
        new QHBoxLayout(
            extrude_extent_row);
    extrude_extent_layout->setContentsMargins(
        0, 0, 0, 0);
    extrude_one_side_button_ =
        new QPushButton(
            QStringLiteral("One Side"),
            extrude_extent_row);
    extrude_one_side_button_->setObjectName(
        QStringLiteral("extrudeOneSideButton"));
    extrude_one_side_button_->setCheckable(true);
    extrude_midplane_button_ =
        new QPushButton(
            QStringLiteral("Midplane"),
            extrude_extent_row);
    extrude_midplane_button_->setObjectName(
        QStringLiteral("extrudeMidplaneButton"));
    extrude_midplane_button_->setCheckable(true);
    extrude_extent_layout->addWidget(
        extrude_one_side_button_);
    extrude_extent_layout->addWidget(
        extrude_midplane_button_);
    extrude_operations_layout->addWidget(
        extrude_extent_row);

    extrude_reverse_button_ =
        new QPushButton(
            QStringLiteral("Reverse"),
            extrude_operations_widget_);
    extrude_reverse_button_->setObjectName(
        QStringLiteral("extrudeReverseButton"));
    extrude_reverse_button_->setCheckable(true);
    extrude_operations_layout->addWidget(
        extrude_reverse_button_);

    auto* extrude_distance_form =
        new QFormLayout;
    extrude_distance_edit_ =
        new QLineEdit(
            extrude_operations_widget_);
    extrude_distance_edit_->setObjectName(
        QStringLiteral("extrudeDistanceEdit"));
    extrude_distance_edit_->setPlaceholderText(
        QStringLiteral("e.g. 10 mm"));
    extrude_preview_timer_ =
        new QTimer(this);
    extrude_preview_timer_->setSingleShot(true);
    extrude_preview_timer_->setInterval(90);
    QObject::connect(
        extrude_preview_timer_,
        &QTimer::timeout,
        this,
        [this] {
            refreshExtrudePreview();
        });
    extrude_distance_form->addRow(
        QStringLiteral("Distance"),
        extrude_distance_edit_);
    extrude_operations_layout->addLayout(
        extrude_distance_form);

    extrude_result_label_ =
        new QLabel(
            QStringLiteral(
                "Enter a positive extrusion distance."),
            extrude_operations_widget_);
    extrude_result_label_->setObjectName(
        QStringLiteral("extrudeResultLabel"));
    extrude_result_label_->setWordWrap(true);
    extrude_operations_layout->addWidget(
        extrude_result_label_);

    extrude_finish_button_ =
        new QPushButton(
            QStringLiteral("Finish Extrude"),
            extrude_operations_widget_);
    extrude_finish_button_->setObjectName(
        QStringLiteral("extrudeFinishButton"));
    extrude_operations_layout->addWidget(
        extrude_finish_button_);

    extrude_cancel_button_ =
        new QPushButton(
            QStringLiteral("Cancel"),
            extrude_operations_widget_);
    extrude_cancel_button_->setObjectName(
        QStringLiteral("extrudeCancelButton"));
    extrude_operations_layout->addWidget(
        extrude_cancel_button_);

    extrude_operations_widget_->setVisible(false);
    operations_layout->addWidget(
        extrude_operations_widget_);

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

    profile_show_islands_button_ =
        new QPushButton(
            QStringLiteral("Show Islands"),
            profile_operations_widget_);
    profile_show_islands_button_->setObjectName(
        QStringLiteral("profileShowIslandsButton"));
    profile_show_islands_button_->setCheckable(true);
    profile_operations_layout->addWidget(
        profile_show_islands_button_);

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
        object_snap_toggle_button_,
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
            settings.object_snap.master_enabled =
                checked;
            if (!cad_interaction_settings_updater_(
                    std::move(settings))) {
                refreshCadInteractionSettingsUi();
            }
        });

    QObject::connect(
        object_snap_override_combo_,
        &QComboBox::currentIndexChanged,
        this,
        [this](int index) {
            if (syncing_precision_ui_ ||
                sketch_interaction_controller_ ==
                    nullptr) {
                return;
            }

            const auto request =
                sketch_interaction_controller_->
                    activePointRequest();
            if (!request) {
                refreshCadInteractionSettingsUi();
                return;
            }

            const int value =
                object_snap_override_combo_->
                    itemData(index).toInt();
            if (value < 0) {
                if (request->temporary_snap_override) {
                    if (!sketch_interaction_controller_->
                            clearTemporarySnapOverride()) {
                        refreshCadInteractionSettingsUi();
                    }
                }
                return;
            }

            const auto override_kind =
                static_cast<
                    sketch::TemporarySnapOverrideKind>(
                    value);
            if (!sketch_interaction_controller_->
                    setTemporarySnapOverride(
                        override_kind)) {
                refreshCadInteractionSettingsUi();
            }
        });

    QObject::connect(
        object_tracking_toggle_button_,
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
            settings.object_snap.
                object_tracking_enabled =
                checked;
            if (!cad_interaction_settings_updater_(
                    std::move(settings))) {
                refreshCadInteractionSettingsUi();
            }
        });

    const auto connect_snap_mode =
        [this](
            QCheckBox* check,
            bool application::ObjectSnapInputSettings::*
                member) {
            QObject::connect(
                check,
                &QCheckBox::toggled,
                this,
                [this, member](bool checked) {
                    if (syncing_precision_ui_ ||
                        !cad_interaction_settings_provider_ ||
                        !cad_interaction_settings_updater_) {
                        return;
                    }
                    auto settings =
                        cad_interaction_settings_provider_();
                    settings.object_snap.*member =
                        checked;
                    if (!cad_interaction_settings_updater_(
                            std::move(settings))) {
                        refreshCadInteractionSettingsUi();
                    }
                });
        };

    connect_snap_mode(
        object_snap_endpoint_check_,
        &application::ObjectSnapInputSettings::
            endpoint);
    connect_snap_mode(
        object_snap_midpoint_check_,
        &application::ObjectSnapInputSettings::
            midpoint);
    connect_snap_mode(
        object_snap_center_check_,
        &application::ObjectSnapInputSettings::
            center);
    connect_snap_mode(
        object_snap_quadrant_check_,
        &application::ObjectSnapInputSettings::
            quadrant);
    connect_snap_mode(
        object_snap_intersection_check_,
        &application::ObjectSnapInputSettings::
            intersection);
    connect_snap_mode(
        object_snap_origin_check_,
        &application::ObjectSnapInputSettings::
            origin);
    connect_snap_mode(
        object_snap_perpendicular_check_,
        &application::ObjectSnapInputSettings::
            perpendicular);
    connect_snap_mode(
        object_snap_tangent_check_,
        &application::ObjectSnapInputSettings::
            tangent);
    connect_snap_mode(
        object_snap_nearest_check_,
        &application::ObjectSnapInputSettings::
            nearest);
    connect_snap_mode(
        object_snap_extension_check_,
        &application::ObjectSnapInputSettings::
            extension);

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
        axis_edit_button_,
        &QPushButton::clicked,
        this,
        [this] {
            if (selected_axis_id_) {
                static_cast<void>(
                    startAxisEdit(
                        *selected_axis_id_));
            }
        });
    QObject::connect(
        axis_delete_button_,
        &QPushButton::clicked,
        this,
        [this] {
            if (selected_axis_id_) {
                deleteAxis(
                    *selected_axis_id_);
            }
        });
    QObject::connect(
        datum_edit_button_,
        &QPushButton::clicked,
        this,
        [this] {
            if (selected_datum_id_) {
                static_cast<void>(
                    startDatumPlaneEdit(
                        *selected_datum_id_));
            }
        });
    QObject::connect(
        datum_delete_button_,
        &QPushButton::clicked,
        this,
        [this] {
            if (selected_datum_id_) {
                deleteDatumPlane(
                    *selected_datum_id_);
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
        profile_go_to_feature_button_,
        &QPushButton::clicked,
        this,
        [this] {
            if (profile_consuming_features_ ==
                    nullptr ||
                profile_consuming_features_->
                    currentIndex() < 0) {
                return;
            }
            const auto bytes =
                profile_consuming_features_->
                    currentData()
                    .toString()
                    .toUtf8();
            const auto id =
                part::FeatureId::parse(
                    std::string_view{
                        bytes.constData(),
                        static_cast<std::size_t>(
                            bytes.size())});
            if (id) {
                navigateToFeature(*id);
            }
        });
    QObject::connect(
        feature_go_to_profile_button_,
        &QPushButton::clicked,
        this,
        [this] {
            if (selected_feature_id_ &&
                document_session_ != nullptr) {
                const auto* feature =
                    document_session_->document()
                        .findFeature(
                            *selected_feature_id_);
                if (feature != nullptr) {
                    const auto profile =
                        part::sourceProfileId(
                            *feature);
                    if (profile) {
                        navigateToProfile(
                            *profile);
                    }
                }
            }
        });
    QObject::connect(
        feature_edit_button_,
        &QPushButton::clicked,
        this,
        [this] {
            if (selected_feature_id_) {
                static_cast<void>(
                    startExtrudeEdit(
                        *selected_feature_id_));
            }
        });
    QObject::connect(
        feature_suppress_button_,
        &QPushButton::clicked,
        this,
        [this] {
            if (selected_feature_id_ &&
                document_session_ != nullptr) {
                const auto* feature =
                    document_session_->document()
                        .findFeature(
                            *selected_feature_id_);
                if (feature != nullptr) {
                    setFeatureSuppressed(
                        feature->id,
                        !feature->suppressed);
                }
            }
        });
    QObject::connect(
        feature_delete_button_,
        &QPushButton::clicked,
        this,
        [this] {
            if (selected_feature_id_) {
                deleteFeature(
                    *selected_feature_id_);
            }
        });
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
        trim_sketch_button_,
        &QPushButton::clicked,
        this,
        [this] { activateSketchTrim(); });
    QObject::connect(
        extend_sketch_button_,
        &QPushButton::clicked,
        this,
        [this] { activateSketchExtend(); });
    QObject::connect(
        extend_both_sketch_button_,
        &QPushButton::clicked,
        this,
        [this] { activateSketchExtendBoth(); });
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
        axis_button_,
        &QPushButton::clicked,
        this,
        [this] {
            if (axis_draft_) {
                cancelAxis();
                return;
            }
            static_cast<void>(
                startAxisTool());
        });
    QObject::connect(
        axis_finish_button_,
        &QPushButton::clicked,
        this,
        [this] {
            static_cast<void>(
                finishAxis());
        });
    QObject::connect(
        axis_cancel_button_,
        &QPushButton::clicked,
        this,
        [this] {
            cancelAxis();
        });

    QObject::connect(
        datum_plane_button_,
        &QPushButton::clicked,
        this,
        [this] {
            if (datum_plane_draft_) {
                cancelDatumPlane();
                return;
            }
            static_cast<void>(
                startDatumPlaneTool());
        });
    QObject::connect(
        datum_plane_offset_edit_,
        &QLineEdit::textChanged,
        this,
        [this](const QString& text_value) {
            if (syncing_datum_plane_ui_ ||
                !datum_plane_draft_ ||
                document_session_ == nullptr) {
                return;
            }

            const auto quantity =
                application::parseCadQuantity(
                    toUtf8(text_value),
                    {
                        application::CadQuantityDimension::
                            length,
                        document_session_->document()
                            .lengthUnit()});
            if (!quantity) {
                datum_plane_offset_input_valid_ = false;
                datum_plane_evaluation_.reset();
                if (datum_plane_preview_timer_) {
                    datum_plane_preview_timer_->stop();
                }
                if (viewport_controller_ != nullptr) {
                    viewport_controller_->
                        setDatumPlaneDraftPreview(
                            std::nullopt);
                }
                syncDatumPlaneUi();
                return;
            }

            if (!datum_plane_draft_->setOffset(
                    core::LengthValue{
                        quantity->canonical_value})) {
                datum_plane_offset_input_valid_ = false;
                datum_plane_evaluation_.reset();
                if (viewport_controller_ != nullptr) {
                    viewport_controller_->
                        setDatumPlaneDraftPreview(
                            std::nullopt);
                }
                syncDatumPlaneUi();
                return;
            }
            datum_plane_offset_input_valid_ = true;
            datum_plane_evaluation_.reset();
            scheduleDatumPlaneEvaluation();
            notifyCadInputContextChanged();
        });
    QObject::connect(
        datum_plane_offset_edit_,
        &QLineEdit::returnPressed,
        this,
        [this] {
            flushDatumPlaneEvaluation();
            static_cast<void>(
                finishDatumPlane());
        });
    QObject::connect(
        datum_plane_reverse_button_,
        &QPushButton::clicked,
        this,
        [this] {
            if (!datum_plane_draft_) {
                return;
            }
            if (datum_plane_draft_->reverse()) {
                datum_plane_offset_input_valid_ = true;
                if (datum_plane_offset_edit_ != nullptr &&
                    document_session_ != nullptr) {
                    const QSignalBlocker blocked{
                        datum_plane_offset_edit_};
                    datum_plane_offset_edit_->setText(
                        formatLengthForPart(
                            datum_plane_draft_->offset(),
                            document_session_->document()
                                .lengthUnit()));
                }
                datum_plane_evaluation_.reset();
                refreshDatumPlaneEvaluation();
                notifyCadInputContextChanged();
            }
        });
    QObject::connect(
        datum_plane_finish_button_,
        &QPushButton::clicked,
        this,
        [this] {
            flushDatumPlaneEvaluation();
            static_cast<void>(
                finishDatumPlane());
        });
    QObject::connect(
        datum_plane_cancel_button_,
        &QPushButton::clicked,
        this,
        [this] {
            cancelDatumPlane();
        });


    QObject::connect(
        extrude_button_,
        &QPushButton::clicked,
        this,
        [this] {
            if (extrude_profile_pick_active_) {
                cancelExtrudeProfilePick();
                return;
            }
            static_cast<void>(
                startExtrudeTool());
        });
    QObject::connect(
        extrude_add_button_,
        &QPushButton::clicked,
        this,
        [this] {
            if (!extrude_draft_) return;
            if (extrude_draft_->setOperation(
                    part::ExtrudeOperation::add)) {
                refreshExtrudePreview();
                notifyCadInputContextChanged();
            }
        });
    QObject::connect(
        extrude_cut_button_,
        &QPushButton::clicked,
        this,
        [this] {
            if (!extrude_draft_ ||
                document_session_ == nullptr) {
                return;
            }
            if (document_session_->document()
                    .body().features.empty()) {
                setStatusText(
                    QStringLiteral(
                        "The first solid-producing Extrude must be Add."));
                syncExtrudeUi();
                return;
            }
            if (extrude_draft_->setOperation(
                    part::ExtrudeOperation::cut)) {
                refreshExtrudePreview();
                notifyCadInputContextChanged();
            }
        });
    QObject::connect(
        extrude_one_side_button_,
        &QPushButton::clicked,
        this,
        [this] {
            if (!extrude_draft_) return;
            if (extrude_draft_->setExtentMode(
                    application::
                        ExtrudeDraftExtentMode::
                            one_side)) {
                refreshExtrudePreview();
                notifyCadInputContextChanged();
            }
        });
    QObject::connect(
        extrude_midplane_button_,
        &QPushButton::clicked,
        this,
        [this] {
            if (!extrude_draft_) return;
            if (extrude_draft_->setExtentMode(
                    application::
                        ExtrudeDraftExtentMode::
                            midplane)) {
                refreshExtrudePreview();
                notifyCadInputContextChanged();
            }
        });
    QObject::connect(
        extrude_reverse_button_,
        &QPushButton::clicked,
        this,
        [this](bool checked) {
            if (!extrude_draft_) return;
            if (!extrude_draft_->setReversed(
                    checked)) {
                syncExtrudeUi();
                return;
            }
            refreshExtrudePreview();
            notifyCadInputContextChanged();
        });
    QObject::connect(
        extrude_distance_edit_,
        &QLineEdit::textChanged,
        this,
        [this](const QString& text_value) {
            if (syncing_extrude_ui_ ||
                !extrude_draft_ ||
                document_session_ == nullptr) {
                return;
            }

            const auto parsed =
                application::parseBareCadDistance(
                    toUtf8(text_value),
                    application::CadInputNumberFormat{
                        toUtf8(
                            QLocale{}.decimalPoint()),
                        document_session_->document()
                            .lengthUnit()});
            if (!parsed || !(*parsed > 0.0)) {
                extrude_distance_input_valid_ =
                    false;
                extrude_evaluation_.reset();
                if (extrude_preview_timer_ != nullptr) {
                    extrude_preview_timer_->stop();
                }
                if (viewport_controller_) {
                    viewport_controller_->
                        clearSolidPreview();
                    if (extrude_draft_->mode() ==
                        application::ExtrudeDraftMode::edit) {
                        viewport_controller_->
                            setTransientProfilePresentationOverride(
                                extrude_draft_->profileId(),
                                std::nullopt);
                    } else {
                        viewport_controller_->
                            setTransientProfilePresentationOverride(
                                std::nullopt,
                                std::nullopt);
                    }
                }
                syncExtrudeUi();
                return;
            }

            static_cast<void>(
                setExtrudeDistance(
                    core::LengthValue{*parsed},
                    toUtf8(text_value),
                    false));
        });
    QObject::connect(
        extrude_distance_edit_,
        &QLineEdit::returnPressed,
        this,
        [this] {
            flushExtrudePreview();
            static_cast<void>(
                finishExtrude());
        });
    QObject::connect(
        extrude_finish_button_,
        &QPushButton::clicked,
        this,
        [this] {
            static_cast<void>(
                finishExtrude());
        });
    QObject::connect(
        extrude_cancel_button_,
        &QPushButton::clicked,
        this,
        [this] {
            cancelExtrude();
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
            options.show_islands =
                profile_show_islands_button_->
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
             profile_show_islands_button_,
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
    syncExtrudeUi();
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
    clearDatumPlaneRuntimeContext();
    clearExtrudeRuntimeContext();
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

    clearDatumPlaneRuntimeContext();
    clearExtrudeRuntimeContext();
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
                profileVisibilityForCheckState(
                    profile_visible_->checkState())});
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
    if (document_session == nullptr) {
        return;
    }

    std::optional<part::ProfileId> explicit_target =
        selected_profile_id_;

    // SR-01: generic Delete remains owned by the active Sketch context,
    // but the explicitly named Delete Profile action remains a deliberate
    // Part command even while the source Sketch is open for editing.
    //
    // During Edit Profile, the semantic edit target remains authoritative
    // even if Viewer/Tree selection presentation is cleared while entering
    // the Profile tool. This is not a generic Delete fallback.
    if (sketch_interaction_controller_ &&
        sketch_interaction_controller_->active() &&
        sketch_interaction_controller_->profileToolActive()) {
        const auto edited_profile_id =
            sketch_interaction_controller_->
                editedProfileId();
        if (!edited_profile_id) {
            setStatusText(
                QStringLiteral(
                    "Finish or cancel the active Profile creation before deleting a Profile."));
            return;
        }

        if (explicit_target &&
            *explicit_target != *edited_profile_id) {
            setStatusText(
                QStringLiteral(
                    "Finish or cancel the active Profile operation before deleting another Profile."));
            return;
        }

        explicit_target = edited_profile_id;

        // Deleting the Profile currently being edited first discards only
        // its transient draft. The authored deletion below is still one
        // normal DeleteProfileCommand / history entry.
        sketch_interaction_controller_->
            cancelProfile();
    }

    if (!explicit_target) {
        return;
    }
    const auto profile_id = *explicit_target;

    if (sketch_interaction_controller_ &&
        sketch_interaction_controller_->active()) {
        sketch_interaction_controller_->
            setSelectedProfileForCadInput(
                std::nullopt);
    }

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



void CadWorkbench::setFeatureSuppressed(
    part::FeatureId feature_id,
    bool suppressed) {
    auto* document_session =
        activeDocumentSession();
    if (document_session == nullptr ||
        datum_plane_draft_ ||
        extrude_draft_ ||
        active_sketch_id_ ||
        sketch_support_pick_active_) {
        setStatusText(
            QStringLiteral(
                "Finish or cancel the active modeling context before changing Feature suppression."));
        return;
    }

    const auto* feature =
        document_session->document()
            .findFeature(feature_id);
    if (feature == nullptr) {
        setStatusText(
            QStringLiteral(
                "Feature is no longer available."));
        return;
    }
    if (feature->suppressed == suppressed) {
        refreshFeatureProperties(
            feature_id);
        return;
    }

    const auto result =
        document_session->execute(
            application::
                SetFeatureSuppressedCommand{
                    feature_id,
                    document_session->document()
                        .revision(),
                    suppressed});
    if (!result.ok()) {
        showFailure(result.diagnostic);
        refreshActiveContext();
        return;
    }

    refreshActiveContext();
    navigateToFeature(feature_id);
    setStatusText(
        suppressed
            ? QStringLiteral(
                  "Feature suppressed — downstream evaluation and automatic Profile visibility updated.")
            : QStringLiteral(
                  "Feature unsuppressed — Body recomputed from current authored state."));
}

void CadWorkbench::deleteFeature(
    part::FeatureId feature_id) {
    auto* document_session =
        activeDocumentSession();
    if (document_session == nullptr ||
        datum_plane_draft_ ||
        extrude_draft_ ||
        active_sketch_id_ ||
        sketch_support_pick_active_) {
        setStatusText(
            QStringLiteral(
                "Finish or cancel the active modeling context before deleting a Feature."));
        return;
    }

    const auto* feature =
        document_session->document()
            .findFeature(feature_id);
    if (feature == nullptr) {
        setStatusText(
            QStringLiteral(
                "Feature is no longer available."));
        return;
    }
    const auto source_profile =
        part::sourceProfileId(*feature);

    const auto result =
        document_session->execute(
            application::DeleteFeatureCommand{
                feature_id,
                document_session->document()
                    .revision()});
    if (!result.ok()) {
        showFailure(result.diagnostic);
        refreshActiveContext();
        return;
    }
    if (!result.changed) {
        return;
    }

    selected_feature_id_.reset();
    refreshActiveContext();
    if (source_profile &&
        document_session->document()
                .findProfile(*source_profile) !=
            nullptr) {
        navigateToProfile(
            *source_profile);
    } else {
        properties_stack_->setCurrentWidget(
            document_properties_page_);
    }

    setStatusText(
        QStringLiteral(
            "Feature deleted — dependents remain authored and are reevaluated from current history."));
}

bool CadWorkbench::startAxisTool() {
    auto* document_session =
        activeDocumentSession();
    if (document_session == nullptr ||
        solid_modeling_kernel_ == nullptr ||
        datum_plane_draft_ ||
        extrude_profile_pick_active_ ||
        extrude_draft_ ||
        sketch_support_pick_active_) {
        setStatusText(
            QStringLiteral(
                "Axis requires an active Part and no conflicting Part command."));
        return false;
    }

    axis_draft_ =
        application::AxisDraft::beginCreate(
            *document_session);
    axis_evaluation_.reset();

    if (!active_sketch_id_ &&
        tree_controller_ != nullptr) {
        if (const auto sketch_id =
                tree_controller_->primarySketchId()) {
            requestEditSketch(*sketch_id);
        }
    }

    tryStageAxisFromSketchSelection();
    refreshAxisEvaluation();
    syncAxisUi();
    syncActionState();
    notifyCadInputContextChanged();

    setStatusText(
        axis_draft_->source()
            ? QStringLiteral(
                  "Axis source acquired — Finish to create one authored Axis.")
            : QStringLiteral(
                  "Axis — select exactly one Line in an active Sketch, then Finish."));
    if (viewport_widget_ != nullptr) {
        viewport_widget_->setFocus(
            Qt::OtherFocusReason);
    }
    return true;
}

bool CadWorkbench::startAxisEdit(
    part::AxisId axis_id) {
    auto* document_session =
        activeDocumentSession();
    if (document_session == nullptr ||
        solid_modeling_kernel_ == nullptr ||
        datum_plane_draft_ ||
        extrude_profile_pick_active_ ||
        extrude_draft_ ||
        sketch_support_pick_active_) {
        return false;
    }

    const auto* axis =
        document_session->document()
            .findAxis(axis_id);
    if (axis == nullptr) {
        setStatusText(
            QStringLiteral(
                "Axis is no longer available."));
        return false;
    }

    if (active_sketch_id_ &&
        *active_sketch_id_ !=
            axis->source.sketch_id) {
        setStatusText(
            QStringLiteral(
                "Finish the active Sketch before editing an Axis from another Sketch."));
        return false;
    }

    auto draft =
        application::AxisDraft::beginEdit(
            *document_session,
            axis_id);
    if (!draft) {
        return false;
    }
    axis_draft_ = std::move(*draft);
    axis_evaluation_.reset();

    if (!active_sketch_id_ &&
        document_session->document()
            .findSketch(
                axis->source.sketch_id) != nullptr) {
        requestEditSketch(
            axis->source.sketch_id);
    }

    refreshAxisEvaluation();
    syncAxisUi();
    syncActionState();
    notifyCadInputContextChanged();
    setStatusText(
        QStringLiteral(
            "Edit Axis — select a replacement Line to re-source while preserving AxisId, or Finish to keep the current source."));
    return true;
}

void CadWorkbench::deleteAxis(
    part::AxisId axis_id) {
    auto* document_session =
        activeDocumentSession();
    if (document_session == nullptr) {
        return;
    }
    if (axis_draft_) {
        cancelAxis();
    }

    const auto result =
        document_session->execute(
            application::DeleteAxisCommand{
                axis_id,
                document_session->document()
                    .revision()});
    if (!result.ok()) {
        showFailure(result.diagnostic);
        return;
    }

    if (selected_axis_id_ &&
        *selected_axis_id_ == axis_id) {
        selected_axis_id_.reset();
    }
    refreshActiveContext();
    syncActionState();
    notifyDocumentStateChanged();
    setStatusText(
        result.changed
            ? QStringLiteral(
                  "Axis deleted — downstream authored consumers remain repairable intent.")
            : QStringLiteral(
                  "No Axis delete change."));
}

void CadWorkbench::stageAxisSource(
    part::SketchLineAxisSource source) {
    if (!axis_draft_ ||
        document_session_ == nullptr ||
        !source.valid()) {
        return;
    }

    const auto* sketch =
        document_session_->document()
            .findSketch(source.sketch_id);
    if (sketch == nullptr ||
        sketch->model.findLine(
            source.entity_id) == nullptr) {
        setStatusText(
            QStringLiteral(
                "Axis source must be one existing Sketch Line."));
        return;
    }

    if (!axis_draft_->setSource(
            std::move(source))) {
        setStatusText(
            QStringLiteral(
                "Axis source could not be staged."));
        return;
    }

    axis_evaluation_.reset();
    refreshAxisEvaluation();
    syncAxisUi();
    notifyCadInputContextChanged();
}

void CadWorkbench::tryStageAxisFromSketchSelection() {
    if (!axis_draft_ ||
        !sketch_interaction_controller_ ||
        !sketch_interaction_controller_->active()) {
        return;
    }

    const auto sketch_id =
        sketch_interaction_controller_->
            activeSketchId();
    const auto& selected =
        sketch_interaction_controller_->
            selectedEntities();
    if (!sketch_id ||
        selected.size() != 1U ||
        document_session_ == nullptr) {
        return;
    }

    const auto* sketch =
        document_session_->document()
            .findSketch(*sketch_id);
    if (sketch == nullptr ||
        sketch->model.findLine(
            selected.front()) == nullptr) {
        return;
    }

    stageAxisSource(
        part::SketchLineAxisSource{
            *sketch_id,
            selected.front()});
}

void CadWorkbench::refreshAxisEvaluation() {
    axis_evaluation_.reset();
    if (!axis_draft_ ||
        document_session_ == nullptr ||
        solid_modeling_kernel_ == nullptr) {
        syncAxisUi();
        return;
    }

    axis_evaluation_ =
        document_session_->evaluateAxisDraft(
            *axis_draft_,
            *solid_modeling_kernel_);
    syncAxisUi();
}

void CadWorkbench::syncAxisUi() {
    const bool active =
        axis_draft_.has_value();

    if (axis_operations_widget_ != nullptr) {
        axis_operations_widget_->setVisible(active);
    }
    if (axis_button_ != nullptr) {
        axis_button_->setChecked(active);
    }

    if (axis_source_label_ != nullptr) {
        if (active &&
            axis_draft_->source()) {
            const auto& source =
                *axis_draft_->source();
            axis_source_label_->setText(
                QStringLiteral(
                    "Sketch %1 / Line %2")
                    .arg(
                        fromUtf8(
                            source.sketch_id.value()),
                        fromUtf8(
                            source.entity_id
                                .serialized())));
        } else {
            axis_source_label_->setText(
                QStringLiteral(
                    "<select one Sketch Line>"));
        }
    }

    QString result_text =
        QStringLiteral(
            "Select exactly one valid Sketch Line.");
    if (active &&
        axis_evaluation_) {
        if (axis_evaluation_->committable()) {
            result_text =
                QStringLiteral(
                    "Resolved — ready to Finish.");
        } else if (
            axis_evaluation_->axis_status &&
            axis_evaluation_->axis_diagnostic) {
            result_text =
                axisEvaluationStatusText(
                    *axis_evaluation_->
                         axis_status) +
                QStringLiteral(" — ") +
                axisEvaluationDiagnosticText(
                    *axis_evaluation_->
                         axis_diagnostic);
        } else if (
            axis_evaluation_->status ==
            application::
                AxisDraftEvaluationStatus::
                    stale_revision) {
            result_text =
                QStringLiteral(
                    "Stale draft — restart Axis.");
        } else if (
            axis_draft_->source()) {
            result_text =
                QStringLiteral(
                    "Axis source is not currently resolvable.");
        }
    }

    if (axis_result_label_ != nullptr) {
        axis_result_label_->setText(
            result_text);
    }
    if (axis_finish_button_ != nullptr) {
        axis_finish_button_->setEnabled(
            active &&
            axis_evaluation_ &&
            axis_evaluation_->committable());
    }
    if (axis_cancel_button_ != nullptr) {
        axis_cancel_button_->setEnabled(active);
    }
}

void CadWorkbench::cancelAxis() {
    if (!axis_draft_) {
        return;
    }

    clearAxisRuntimeContext();
    syncActionState();
    notifyCadInputContextChanged();
    setStatusText(
        QStringLiteral(
            "Axis cancelled — no authored change."));
    if (viewport_widget_ != nullptr) {
        viewport_widget_->setFocus(
            Qt::OtherFocusReason);
    }
}

bool CadWorkbench::finishAxis() {
    if (!axis_draft_ ||
        document_session_ == nullptr ||
        solid_modeling_kernel_ == nullptr) {
        return false;
    }

    refreshAxisEvaluation();
    if (!axis_evaluation_ ||
        !axis_evaluation_->committable()) {
        setStatusText(
            QStringLiteral(
                "Axis Finish requires one currently resolved Sketch Line."));
        return false;
    }

    const auto result =
        application::finishAxisDraft(
            *document_session_,
            *axis_draft_,
            *axis_evaluation_,
            *solid_modeling_kernel_);
    if (!result.ok()) {
        setStatusText(
            result.diagnostic.empty()
                ? QStringLiteral(
                      "Axis Finish was rejected.")
                : fromUtf8(
                      result.diagnostic));
        refreshAxisEvaluation();
        return false;
    }

    const auto axis_id =
        result.axis_id;
    clearAxisRuntimeContext();
    refreshActiveContext();
    syncActionState();
    notifyCadInputContextChanged();
    notifyDocumentStateChanged();

    if (axis_id) {
        selected_axis_id_ = axis_id;
        refreshAxisProperties(*axis_id);
    }

    setStatusText(
        QStringLiteral(
            "Axis finished — one semantic transaction committed."));
    if (viewport_widget_ != nullptr) {
        viewport_widget_->setFocus(
            Qt::OtherFocusReason);
    }
    return true;
}

void CadWorkbench::clearAxisRuntimeContext() {
    axis_draft_.reset();
    axis_evaluation_.reset();
    syncAxisUi();
}

application::CadInputSubmitResult
CadWorkbench::submitAxisCadInput(
    std::string_view text) {
    if (!axis_draft_) {
        return {
            false,
            "No active Axis command."};
    }

    auto result =
        application::submitAxisCadInput(
            *axis_draft_,
            text,
            application::CadInputNumberFormat{});

    if (!result.accepted) {
        return {
            false,
            result.diagnostic};
    }

    switch (result.action) {
    case application::AxisCadInputAction::none:
        break;
    case application::AxisCadInputAction::acquire_source:
        tryStageAxisFromSketchSelection();
        if (!axis_draft_->source()) {
            setStatusText(
                QStringLiteral(
                    "Axis SOURCE — select exactly one Line in an active Sketch."));
        }
        break;
    case application::AxisCadInputAction::finish:
        if (!finishAxis()) {
            return {
                false,
                "Axis Finish requires one currently resolved Sketch Line."};
        }
        break;
    case application::AxisCadInputAction::cancel:
        cancelAxis();
        break;
    }

    return {true, {}};
}

bool CadWorkbench::startDatumPlaneTool() {
    auto* document_session =
        activeDocumentSession();
    if (document_session == nullptr ||
        solid_modeling_kernel_ == nullptr) {
        setStatusText(
            QStringLiteral(
                "Datum Plane requires an active Part and modeling Kernel."));
        return false;
    }
    if (datum_plane_draft_) {
        setStatusText(
            QStringLiteral(
                "A Datum Plane operation is already active."));
        return false;
    }
    if (extrude_profile_pick_active_ ||
        extrude_draft_ ||
        active_sketch_id_ ||
        sketch_support_pick_active_) {
        setStatusText(
            QStringLiteral(
                "Finish or cancel the active modeling context before Datum Plane."));
        return false;
    }

    datum_plane_draft_ =
        application::DatumPlaneDraft::beginCreate(
            *document_session);
    datum_plane_evaluation_.reset();
    datum_plane_offset_input_valid_ = true;

    if (datum_plane_offset_edit_ != nullptr) {
        const QSignalBlocker blocked{
            datum_plane_offset_edit_};
        datum_plane_offset_edit_->setText(
            formatLengthForPart(
                datum_plane_draft_->offset(),
                document_session->document()
                    .lengthUnit()));
    }

    if (viewport_controller_ != nullptr) {
        viewport_controller_->
            setBodyTopologyFacePickOnly(true);

        tryStageDatumPlaneFromDatum(
            selected_datum_id_);
        if (datum_plane_draft_ &&
            !datum_plane_draft_->source()) {
            tryStageDatumPlaneFromSupport(
                viewport_controller_->
                    primarySelection());
        }
    }
    if (datum_plane_draft_ &&
        !datum_plane_draft_->source() &&
        viewport_controller_ != nullptr) {
        const auto inspection =
            viewport_controller_->
                primaryBodyTopologyInspection();
        if (inspection) {
            tryStageDatumPlaneFromBodyTopology(
                *inspection);
        }
    }

    refreshDatumPlaneEvaluation();
    syncActionState();
    syncDatumPlaneUi();
    notifyCadInputContextChanged();

    setStatusText(
        datum_plane_draft_->source()
            ? QStringLiteral(
                  "Datum Plane active — Offset constructor, default 10 mm; adjust Offset or Finish.")
            : QStringLiteral(
                  "Datum Plane active — select XY/XZ/YZ Origin plane, planar Body Face or existing Datum Plane."));
    if (viewport_widget_ != nullptr) {
        viewport_widget_->setFocus(
            Qt::OtherFocusReason);
    }
    return true;
}

bool CadWorkbench::startDatumPlaneEdit(
    part::DatumId datum_id) {
    auto* document_session =
        activeDocumentSession();
    if (document_session == nullptr ||
        solid_modeling_kernel_ == nullptr) {
        setStatusText(
            QStringLiteral(
                "Edit Datum Plane requires an active Part and modeling Kernel."));
        return false;
    }
    if (datum_plane_draft_ ||
        extrude_profile_pick_active_ ||
        extrude_draft_ ||
        active_sketch_id_ ||
        sketch_support_pick_active_) {
        setStatusText(
            QStringLiteral(
                "Finish or cancel the active modeling context before Edit Datum Plane."));
        return false;
    }

    auto draft =
        application::DatumPlaneDraft::beginEdit(
            *document_session,
            datum_id);
    if (!draft) {
        setStatusText(
            QStringLiteral(
                "Selected Datum Plane is no longer available."));
        return false;
    }

    datum_plane_draft_ =
        std::move(*draft);
    datum_plane_evaluation_.reset();
    datum_plane_offset_input_valid_ = true;

    if (datum_plane_offset_edit_ != nullptr) {
        const QSignalBlocker blocked{
            datum_plane_offset_edit_};
        datum_plane_offset_edit_->setText(
            formatLengthForPart(
                datum_plane_draft_->offset(),
                document_session->document()
                    .lengthUnit()));
    }
    if (viewport_controller_ != nullptr) {
        viewport_controller_->
            setBodyTopologyFacePickOnly(true);
    }

    refreshDatumPlaneEvaluation();
    syncActionState();
    syncDatumPlaneUi();
    notifyCadInputContextChanged();
    setStatusText(
        QStringLiteral(
            "Edit Datum Plane active — select a new Source or adjust signed Offset; Finish preserves DatumId."));
    if (datum_plane_offset_edit_ != nullptr) {
        datum_plane_offset_edit_->setFocus(
            Qt::OtherFocusReason);
    }
    return true;
}

void CadWorkbench::deleteDatumPlane(
    part::DatumId datum_id) {
    auto* document_session =
        activeDocumentSession();
    if (document_session == nullptr) {
        return;
    }
    if (datum_plane_draft_ ||
        extrude_profile_pick_active_ ||
        extrude_draft_ ||
        active_sketch_id_ ||
        sketch_support_pick_active_) {
        setStatusText(
            QStringLiteral(
                "Finish or cancel the active modeling context before deleting a Datum Plane."));
        return;
    }
    if (document_session->document()
            .findDatumPlane(datum_id) == nullptr) {
        setStatusText(
            QStringLiteral(
                "Datum Plane is no longer available."));
        return;
    }

    const auto result =
        document_session->execute(
            application::DeleteDatumPlaneCommand{
                datum_id,
                document_session->document()
                    .revision()});
    if (!result.ok()) {
        showFailure(result.diagnostic);
        refreshActiveContext();
        return;
    }
    if (!result.changed) {
        return;
    }

    if (selected_datum_id_ &&
        *selected_datum_id_ == datum_id) {
        selected_datum_id_.reset();
    }
    refreshActiveContext();
    if (properties_stack_ != nullptr) {
        properties_stack_->setCurrentWidget(
            document_properties_page_);
    }
    setStatusText(
        QStringLiteral(
            "Datum Plane deleted — dependency-safe command committed."));
}

void CadWorkbench::stageDatumPlaneSource(
    part::PlaneReference source) {
    if (!datum_plane_draft_ ||
        !source.valid()) {
        return;
    }
    if (!datum_plane_draft_->setSource(
            std::move(source))) {
        setStatusText(
            QStringLiteral(
                "Datum Plane source could not be staged."));
        return;
    }
    datum_plane_evaluation_.reset();
    refreshDatumPlaneEvaluation();
    notifyCadInputContextChanged();
}

void CadWorkbench::tryStageDatumPlaneFromSupport(
    std::optional<core::BuiltinReferenceRole> support) {
    if (!datum_plane_draft_ ||
        !support ||
        !part::isDatumOriginPlane(*support)) {
        return;
    }

    stageDatumPlaneSource(
        part::PlaneReference{
            part::BuiltinOriginPlaneReference{
                *support}});
}

void CadWorkbench::tryStageDatumPlaneFromBodyTopology(
    const BodyTopologyInspection& inspection) {
    if (!datum_plane_draft_ ||
        inspection.diagnostic_prefix ||
        inspection.kind !=
            viewer::BodyTopologyPresentationKind::face ||
        inspection.sketch_support !=
            SketchSupportInspectionCapability::supported ||
        !inspection.stage.valid() ||
        !inspection.surface_address ||
        (inspection.surface_kind &&
         *inspection.surface_kind !=
             kernel::SurfaceKind::plane)) {
        return;
    }

    part::SurfaceReference surface{
        inspection.stage,
        *inspection.surface_address};
    if (!surface.valid()) {
        return;
    }

    stageDatumPlaneSource(
        part::PlaneReference{
            part::BodyPlanarSurfacePlaneReference{
                std::move(surface)}});
}

void CadWorkbench::tryStageDatumPlaneFromDatum(
    std::optional<part::DatumId> datum_id) {
    if (!datum_plane_draft_ ||
        !datum_id ||
        !datum_id->valid() ||
        (datum_plane_draft_->datumId() &&
         *datum_plane_draft_->datumId() ==
             *datum_id) ||
        document_session_ == nullptr ||
        document_session_->document()
                .findDatumPlane(*datum_id) ==
            nullptr) {
        return;
    }

    stageDatumPlaneSource(
        part::PlaneReference{
            part::DatumPlaneReference{
                *datum_id}});
}


void CadWorkbench::scheduleDatumPlaneEvaluation() {
    if (datum_plane_preview_timer_ == nullptr) {
        refreshDatumPlaneEvaluation();
        return;
    }
    datum_plane_preview_timer_->start();
}

void CadWorkbench::flushDatumPlaneEvaluation() {
    if (datum_plane_preview_timer_ == nullptr ||
        !datum_plane_preview_timer_->isActive()) {
        return;
    }
    datum_plane_preview_timer_->stop();
    refreshDatumPlaneEvaluation();
}

void CadWorkbench::refreshDatumPlaneEvaluation() {
    if (datum_plane_preview_timer_ != nullptr) {
        datum_plane_preview_timer_->stop();
    }
    datum_plane_evaluation_.reset();

    if (!datum_plane_draft_ ||
        !datum_plane_offset_input_valid_ ||
        !datum_plane_draft_->source() ||
        document_session_ == nullptr ||
        solid_modeling_kernel_ == nullptr) {
        if (viewport_controller_ != nullptr) {
            viewport_controller_->
                setDatumPlaneDraftPreview(
                    std::nullopt);
        }
        syncDatumPlaneUi();
        return;
    }

    datum_plane_evaluation_ =
        document_session_->
            evaluateDatumPlaneDraft(
                *datum_plane_draft_,
                *solid_modeling_kernel_);

    if (viewport_controller_ != nullptr) {
        viewport_controller_->
            setDatumPlaneDraftPreview(
                datum_plane_evaluation_->committable() &&
                        datum_plane_evaluation_->frame
                    ? datum_plane_evaluation_->frame
                    : std::nullopt);
    }
    syncDatumPlaneUi();
}

void CadWorkbench::syncDatumPlaneUi() {
    const bool active =
        datum_plane_draft_.has_value();

    if (datum_plane_operations_widget_ != nullptr) {
        datum_plane_operations_widget_->
            setVisible(active);
    }
    if (datum_plane_button_ != nullptr) {
        datum_plane_button_->setChecked(active);
    }
    if (!active) {
        return;
    }

    syncing_datum_plane_ui_ = true;

    if (datum_plane_constructor_combo_ != nullptr) {
        datum_plane_constructor_combo_->
            setCurrentIndex(0);
    }

    QString source_text =
        QStringLiteral(
            "Select XY/XZ/YZ Origin plane, planar Body Face or existing Datum Plane");
    if (datum_plane_draft_->source()) {
        const auto& source =
            *datum_plane_draft_->source();
        if (const auto origin =
                part::builtinOriginPlaneForPlaneReference(
                    source)) {
            switch (*origin) {
            case core::BuiltinReferenceRole::xy_plane:
                source_text =
                    QStringLiteral("XY Plane");
                break;
            case core::BuiltinReferenceRole::xz_plane:
                source_text =
                    QStringLiteral("XZ Plane");
                break;
            case core::BuiltinReferenceRole::yz_plane:
                source_text =
                    QStringLiteral("YZ Plane");
                break;
            default:
                source_text =
                    QStringLiteral("<invalid Origin plane>");
                break;
            }
        } else if (const auto* surface =
                       part::bodyPlanarSurfaceForPlaneReference(
                           source)) {
            source_text =
                QStringLiteral(
                    "Body Surface @ Feature %1")
                    .arg(
                        fromUtf8(
                            surface->surface
                                .producer_feature_id
                                .serialized()));
        } else if (const auto datum_id =
                       part::datumPlaneIdForPlaneReference(
                           source)) {
            source_text =
                QStringLiteral("Datum Plane %1")
                    .arg(
                        fromUtf8(
                            datum_id->serialized()));
        }
    }

    if (datum_plane_source_label_ != nullptr) {
        datum_plane_source_label_->setText(
            source_text);
    }

    if (datum_plane_offset_edit_ != nullptr &&
        datum_plane_offset_input_valid_ &&
        !datum_plane_offset_edit_->hasFocus() &&
        document_session_ != nullptr) {
        const QSignalBlocker blocked{
            datum_plane_offset_edit_};
        datum_plane_offset_edit_->setText(
            formatLengthForPart(
                datum_plane_draft_->offset(),
                document_session_->document()
                    .lengthUnit()));
    }

    QString result_text;
    if (!datum_plane_offset_input_valid_) {
        result_text =
            QStringLiteral(
                "Offset expects a finite signed Length.");
    } else if (!datum_plane_draft_->source()) {
        result_text =
            QStringLiteral(
                "Select a source plane.");
    } else if (!datum_plane_evaluation_) {
        result_text =
            QStringLiteral(
                "Resolving Datum Plane...");
    } else if (datum_plane_evaluation_->
                   committable()) {
        result_text =
            QStringLiteral(
                "Resolved — current Datum frame is valid.");
    } else {
        switch (datum_plane_evaluation_->status) {
        case application::
            DatumPlaneDraftEvaluationStatus::
                stale_document:
        case application::
            DatumPlaneDraftEvaluationStatus::
                stale_revision:
            result_text =
                QStringLiteral(
                    "Stale context — restart Datum Plane.");
            break;
        case application::
            DatumPlaneDraftEvaluationStatus::
                missing_datum:
            result_text =
                QStringLiteral(
                    "Source Datum is Missing.");
            break;
        case application::
            DatumPlaneDraftEvaluationStatus::
                id_exhausted:
            result_text =
                QStringLiteral(
                    "DatumId allocation is exhausted.");
            break;
        case application::
            DatumPlaneDraftEvaluationStatus::
                source_unresolved:
            result_text =
                QStringLiteral(
                    "Source is not currently Resolved.");
            break;
        case application::
            DatumPlaneDraftEvaluationStatus::
                invalid_candidate:
        case application::
            DatumPlaneDraftEvaluationStatus::
                invalid_draft:
        case application::
            DatumPlaneDraftEvaluationStatus::ok:
            result_text =
                QStringLiteral(
                    "Datum Plane cannot currently Finish.");
            break;
        }
    }

    if (datum_plane_result_label_ != nullptr) {
        datum_plane_result_label_->setText(
            result_text);
    }
    if (datum_plane_reverse_button_ != nullptr) {
        datum_plane_reverse_button_->setEnabled(
            active &&
            datum_plane_offset_input_valid_);
    }
    if (datum_plane_finish_button_ != nullptr) {
        datum_plane_finish_button_->setEnabled(
            datum_plane_offset_input_valid_ &&
            datum_plane_evaluation_ &&
            datum_plane_evaluation_->
                committable());
    }
    if (datum_plane_cancel_button_ != nullptr) {
        datum_plane_cancel_button_->setEnabled(
            active);
    }

    syncing_datum_plane_ui_ = false;
}

void CadWorkbench::cancelDatumPlane() {
    if (!datum_plane_draft_) {
        return;
    }

    clearDatumPlaneRuntimeContext();
    syncActionState();
    notifyCadInputContextChanged();
    setStatusText(
        QStringLiteral(
            "Datum Plane cancelled — no authored change."));
    if (viewport_widget_ != nullptr) {
        viewport_widget_->setFocus(
            Qt::OtherFocusReason);
    }
}

bool CadWorkbench::finishDatumPlane() {
    flushDatumPlaneEvaluation();

    auto* document_session =
        activeDocumentSession();
    if (document_session == nullptr ||
        solid_modeling_kernel_ == nullptr ||
        !datum_plane_draft_ ||
        !datum_plane_offset_input_valid_ ||
        !datum_plane_evaluation_ ||
        !datum_plane_evaluation_->
            committable()) {
        setStatusText(
            QStringLiteral(
                "Datum Plane cannot finish until Source and Offset resolve successfully."));
        return false;
    }

    const auto result =
        application::finishDatumPlaneDraft(
            *document_session,
            *datum_plane_draft_,
            *datum_plane_evaluation_,
            *solid_modeling_kernel_);
    if (!result.ok()) {
        setStatusText(
            result.diagnostic.empty()
                ? QStringLiteral(
                      "Datum Plane Finish was rejected.")
                : fromUtf8(
                      result.diagnostic));
        refreshDatumPlaneEvaluation();
        return false;
    }

    const auto datum_id =
        result.datum_id;
    clearDatumPlaneRuntimeContext();
    refreshActiveContext();
    syncActionState();
    notifyCadInputContextChanged();
    notifyDocumentStateChanged();

    setStatusText(
        datum_id
            ? QStringLiteral(
                  "Datum Plane %1 finished — one transaction committed.")
                  .arg(
                      fromUtf8(
                          datum_id->serialized()))
            : QStringLiteral(
                  "Datum Plane finished — one transaction committed."));
    if (viewport_widget_ != nullptr) {
        viewport_widget_->setFocus(
            Qt::OtherFocusReason);
    }
    return true;
}

void CadWorkbench::clearDatumPlaneRuntimeContext() {
    if (datum_plane_preview_timer_ != nullptr) {
        datum_plane_preview_timer_->stop();
    }
    datum_plane_draft_.reset();
    datum_plane_evaluation_.reset();
    datum_plane_offset_input_valid_ = true;

    if (viewport_controller_ != nullptr) {
        viewport_controller_->
            setDatumPlaneDraftPreview(
                std::nullopt);
        viewport_controller_->
            setBodyTopologyFacePickOnly(false);
    }

    if (datum_plane_offset_edit_ != nullptr) {
        const QSignalBlocker blocked{
            datum_plane_offset_edit_};
        datum_plane_offset_edit_->clear();
    }
    syncDatumPlaneUi();
}

application::CadInputSubmitResult
CadWorkbench::submitDatumPlaneCadInput(
    std::string_view text) {
    if (!datum_plane_draft_ ||
        document_session_ == nullptr) {
        return {
            false,
            "No active Datum Plane command."};
    }

    auto result =
        application::submitDatumPlaneCadInput(
            *datum_plane_draft_,
            text,
            application::CadInputNumberFormat{
                toUtf8(
                    QLocale{}.decimalPoint()),
                document_session_->document()
                    .lengthUnit()});
    if (!result.accepted) {
        return {
            false,
            result.diagnostic};
    }

    switch (result.action) {
    case application::DatumPlaneCadInputAction::finish:
        return finishDatumPlane()
            ? application::CadInputSubmitResult{
                  true, {}}
            : application::CadInputSubmitResult{
                  false,
                  "Datum Plane Finish was rejected."};

    case application::DatumPlaneCadInputAction::cancel:
        cancelDatumPlane();
        return {true, {}};

    case application::DatumPlaneCadInputAction::none:
        datum_plane_offset_input_valid_ = true;
        if (datum_plane_offset_edit_ != nullptr) {
            const QSignalBlocker blocked{
                datum_plane_offset_edit_};
            datum_plane_offset_edit_->setText(
                formatLengthForPart(
                    datum_plane_draft_->offset(),
                    document_session_->document()
                        .lengthUnit()));
        }
        datum_plane_evaluation_.reset();
        refreshDatumPlaneEvaluation();
        notifyCadInputContextChanged();
        return {true, {}};
    }

    return {
        false,
        "Datum Plane input action is invalid."};
}

bool CadWorkbench::startExtrudeTool() {
    auto* document_session =
        activeDocumentSession();
    if (document_session == nullptr ||
        solid_modeling_kernel_ == nullptr) {
        setStatusText(
            QStringLiteral(
                "Extrude requires an active Part and modeling Kernel."));
        return false;
    }
    if (extrude_draft_) {
        setStatusText(
            QStringLiteral(
                "An Extrude operation is already active."));
        return false;
    }
    if (datum_plane_draft_ ||
        active_sketch_id_ ||
        sketch_support_pick_active_) {
        setStatusText(
            QStringLiteral(
                "Finish or cancel the active Datum Plane/Sketch context before Extrude."));
        return false;
    }

    if (selected_profile_id_) {
        const auto evaluation =
            document_session->document()
                .evaluateProfile(
                    *selected_profile_id_);
        if (evaluation &&
            evaluation->valid()) {
            return startExtrudeFromSelectedProfile();
        }
    }

    extrude_profile_pick_active_ = true;
    ++extrude_profile_pick_generation_;
    if (extrude_profile_pick_generation_ == 0U) {
        ++extrude_profile_pick_generation_;
    }
    syncActionState();
    notifyCadInputContextChanged();
    setStatusText(
        QStringLiteral(
            "Extrude active — select one valid Profile in the Tree or viewport; Esc/CANCEL exits."));
    if (viewport_widget_ != nullptr) {
        viewport_widget_->setFocus(
            Qt::OtherFocusReason);
    }
    return true;
}

void CadWorkbench::cancelExtrudeProfilePick() {
    if (!extrude_profile_pick_active_) {
        return;
    }

    extrude_profile_pick_active_ = false;
    ++extrude_profile_pick_generation_;
    if (extrude_profile_pick_generation_ == 0U) {
        ++extrude_profile_pick_generation_;
    }
    syncActionState();
    notifyCadInputContextChanged();
    setStatusText(
        QStringLiteral(
            "Extrude profile selection cancelled — no authored change."));
    if (viewport_widget_ != nullptr) {
        viewport_widget_->setFocus(
            Qt::OtherFocusReason);
    }
}

void CadWorkbench::tryCompleteExtrudeProfilePick() {
    if (!extrude_profile_pick_active_ ||
        !selected_profile_id_) {
        return;
    }

    auto* document_session =
        activeDocumentSession();
    if (document_session == nullptr) {
        cancelExtrudeProfilePick();
        return;
    }

    const auto evaluation =
        document_session->document()
            .evaluateProfile(
                *selected_profile_id_);
    if (!evaluation ||
        !evaluation->valid()) {
        setStatusText(
            QStringLiteral(
                "Selected Profile is Invalid — select one valid Profile for Extrude or cancel."));
        return;
    }

    static_cast<void>(
        startExtrudeFromSelectedProfile());
}

bool CadWorkbench::startExtrudeFromSelectedProfile() {
    auto* document_session =
        activeDocumentSession();
    if (document_session == nullptr ||
        solid_modeling_kernel_ == nullptr) {
        setStatusText(
            QStringLiteral(
                "Extrude requires an active Part and modeling Kernel."));
        return false;
    }
    if (extrude_draft_) {
        setStatusText(
            QStringLiteral(
                "An Extrude operation is already active."));
        return false;
    }
    if (datum_plane_draft_ ||
        active_sketch_id_ ||
        sketch_support_pick_active_) {
        setStatusText(
            QStringLiteral(
                "Finish or cancel the active Datum Plane/Sketch context before Extrude."));
        return false;
    }
    if (!selected_profile_id_) {
        setStatusText(
            QStringLiteral(
                "Select exactly one valid Profile before Extrude."));
        return false;
    }

    const auto profile_evaluation =
        document_session->document()
            .evaluateProfile(
                *selected_profile_id_);
    if (!profile_evaluation ||
        !profile_evaluation->valid()) {
        setStatusText(
            QStringLiteral(
                "Selected Profile is not valid for Extrude."));
        return false;
    }

    auto draft =
        application::ExtrudeDraft::beginCreate(
            *document_session,
            *selected_profile_id_);
    if (!draft) {
        setStatusText(
            QStringLiteral(
                "Extrude draft could not be created."));
        return false;
    }

    constexpr core::LengthValue
        default_extrude_distance{10.0};
    if (!draft->setDistance(
            default_extrude_distance)) {
        setStatusText(
            QStringLiteral(
                "Extrude default distance could not be initialized."));
        return false;
    }

    extrude_profile_pick_active_ = false;
    extrude_draft_ =
        std::move(*draft);
    extrude_evaluation_.reset();
    extrude_distance_input_valid_ = true;

    if (extrude_distance_edit_ != nullptr) {
        const QSignalBlocker blocked{
            extrude_distance_edit_};
        extrude_distance_edit_->setText(
            formatLengthForPart(
                extrude_draft_->distance(),
                document_session->document()
                    .lengthUnit()));
    }
    if (viewport_controller_) {
        viewport_controller_->clearSolidPreview();
        viewport_controller_->
            setTransientProfilePresentationOverride(
                std::nullopt,
                std::nullopt);
    }

    refreshExtrudePreview();
    syncActionState();
    syncExtrudeUi();
    notifyCadInputContextChanged();
    setStatusText(
        QStringLiteral(
            "Extrude active — default distance applied; adjust parameters or Finish."));
    if (extrude_distance_edit_ != nullptr) {
        extrude_distance_edit_->setFocus(
            Qt::OtherFocusReason);
    }
    return true;
}


bool CadWorkbench::startExtrudeEdit(
    part::FeatureId feature_id) {
    auto* document_session =
        activeDocumentSession();
    if (document_session == nullptr ||
        solid_modeling_kernel_ == nullptr) {
        setStatusText(
            QStringLiteral(
                "Edit Extrude requires an active Part and modeling Kernel."));
        return false;
    }
    if (extrude_profile_pick_active_) {
        setStatusText(
            QStringLiteral(
                "Cancel the active Extrude Profile selection before editing a Feature."));
        return false;
    }
    if (extrude_draft_) {
        setStatusText(
            QStringLiteral(
                "Finish or cancel the active Extrude before editing another Feature."));
        return false;
    }
    if (datum_plane_draft_ ||
        active_sketch_id_ ||
        sketch_support_pick_active_) {
        setStatusText(
            QStringLiteral(
                "Finish or cancel the active Datum Plane/Sketch context before Edit Extrude."));
        return false;
    }

    auto draft =
        application::ExtrudeDraft::beginEdit(
            *document_session,
            feature_id);
    if (!draft) {
        setStatusText(
            QStringLiteral(
                "Selected Feature is unavailable or Suppressed."));
        return false;
    }

    extrude_draft_ =
        std::move(*draft);
    extrude_evaluation_.reset();
    extrude_distance_input_valid_ =
        extrude_draft_->valid();

    if (extrude_distance_edit_ != nullptr) {
        const QSignalBlocker blocked{
            extrude_distance_edit_};
        extrude_distance_edit_->setText(
            formatLengthForPart(
                extrude_draft_->distance(),
                document_session->document()
                    .lengthUnit()));
    }

    if (viewport_controller_) {
        viewport_controller_->
            setTransientProfilePresentationOverride(
                extrude_draft_->profileId(),
                std::nullopt);
    }

    refreshExtrudePreview();
    syncActionState();
    syncExtrudeUi();
    notifyCadInputContextChanged();
    setStatusText(
        QStringLiteral(
            "Edit Extrude active — source Profile is hidden while valid preview is shown."));
    if (extrude_distance_edit_ != nullptr) {
        extrude_distance_edit_->setFocus(
            Qt::OtherFocusReason);
    }
    return true;
}

void CadWorkbench::cancelExtrude() {
    if (!extrude_draft_) {
        return;
    }

    if (extrude_preview_timer_ != nullptr) {
        extrude_preview_timer_->stop();
    }
    extrude_draft_.reset();
    extrude_evaluation_.reset();
    extrude_distance_input_valid_ = false;
    if (viewport_controller_) {
        viewport_controller_->clearSolidPreview();
        viewport_controller_->
            setTransientProfilePresentationOverride(
                std::nullopt,
                std::nullopt);
    }

    syncActionState();
    syncExtrudeUi();
    notifyCadInputContextChanged();
    setStatusText(
        QStringLiteral(
            "Extrude cancelled — no authored change."));
    if (viewport_widget_ != nullptr) {
        viewport_widget_->setFocus(
            Qt::OtherFocusReason);
    }
}

bool CadWorkbench::finishExtrude() {
    flushExtrudePreview();
    auto* document_session =
        activeDocumentSession();
    if (document_session == nullptr ||
        solid_modeling_kernel_ == nullptr ||
        !extrude_draft_ ||
        !extrude_evaluation_ ||
        !extrude_distance_input_valid_) {
        setStatusText(
            QStringLiteral(
                "Extrude cannot finish until the current preview is valid."));
        return false;
    }

    const auto result =
        application::finishExtrudeDraft(
            *document_session,
            *extrude_draft_,
            *extrude_evaluation_,
            *solid_modeling_kernel_);
    if (!result.ok()) {
        setStatusText(
            result.diagnostic.empty()
                ? QStringLiteral(
                      "Extrude Finish was rejected.")
                : fromUtf8(
                      result.diagnostic));
        refreshExtrudePreview();
        return false;
    }

    const auto committed_feature_id =
        result.feature_id;
    extrude_draft_.reset();
    extrude_evaluation_.reset();
    extrude_distance_input_valid_ = false;
    if (viewport_controller_) {
        viewport_controller_->clearSolidPreview();
        viewport_controller_->
            setTransientProfilePresentationOverride(
                std::nullopt,
                std::nullopt);
    }

    refreshActiveContext();
    if (committed_feature_id) {
        navigateToFeature(
            *committed_feature_id);
    }
    syncExtrudeUi();
    notifyCadInputContextChanged();
    setStatusText(
        QStringLiteral(
            "Extrude finished — Feature committed."));
    if (viewport_widget_ != nullptr) {
        viewport_widget_->setFocus(
            Qt::OtherFocusReason);
    }
    return true;
}

void CadWorkbench::clearExtrudeRuntimeContext() {
    if (extrude_preview_timer_ != nullptr) {
        extrude_preview_timer_->stop();
    }
    extrude_profile_pick_active_ = false;
    extrude_draft_.reset();
    extrude_evaluation_.reset();
    extrude_distance_input_valid_ = false;
    if (viewport_controller_) {
        viewport_controller_->clearSolidPreview();
        viewport_controller_->
            setTransientProfilePresentationOverride(
                std::nullopt,
                std::nullopt);
    }
    if (extrude_distance_edit_ != nullptr) {
        const QSignalBlocker blocked{
            extrude_distance_edit_};
        extrude_distance_edit_->clear();
    }
    syncExtrudeUi();
}

bool CadWorkbench::setExtrudeDistance(
    core::LengthValue distance,
    std::optional<std::string_view>
        display_text,
    bool refresh_now) {
    if (!extrude_draft_ ||
        !distance.finite() ||
        !(distance.millimetres > 0.0)) {
        return false;
    }
    if (!extrude_draft_->setDistance(
            distance)) {
        return false;
    }

    extrude_distance_input_valid_ = true;
    if (display_text &&
        extrude_distance_edit_ != nullptr) {
        const QSignalBlocker blocked{
            extrude_distance_edit_};
        extrude_distance_edit_->setText(
            fromUtf8(*display_text));
    }

    extrude_evaluation_.reset();
    if (refresh_now) {
        refreshExtrudePreview();
    } else {
        scheduleExtrudePreview();
        syncExtrudeUi();
    }
    notifyCadInputContextChanged();
    return true;
}

void CadWorkbench::scheduleExtrudePreview() {
    if (extrude_preview_timer_ == nullptr) {
        refreshExtrudePreview();
        return;
    }
    extrude_preview_timer_->start();
}

void CadWorkbench::flushExtrudePreview() {
    if (extrude_preview_timer_ == nullptr ||
        !extrude_preview_timer_->isActive()) {
        return;
    }
    extrude_preview_timer_->stop();
    refreshExtrudePreview();
}

void CadWorkbench::refreshExtrudePreview() {
    if (extrude_preview_timer_ != nullptr) {
        extrude_preview_timer_->stop();
    }
    extrude_evaluation_.reset();

    if (viewport_controller_) {
        viewport_controller_->clearSolidPreview();
    }

    const auto sync_source_profile =
        [this](bool preview_ready) {
            if (viewport_controller_ == nullptr) {
                return;
            }
            if (!extrude_draft_) {
                viewport_controller_->
                    setTransientProfilePresentationOverride(
                        std::nullopt,
                        std::nullopt);
                return;
            }

            if (preview_ready) {
                viewport_controller_->
                    setTransientProfilePresentationOverride(
                        std::nullopt,
                        extrude_draft_->profileId());
                return;
            }

            if (extrude_draft_->mode() ==
                application::ExtrudeDraftMode::edit) {
                viewport_controller_->
                    setTransientProfilePresentationOverride(
                        extrude_draft_->profileId(),
                        std::nullopt);
            } else {
                viewport_controller_->
                    setTransientProfilePresentationOverride(
                        std::nullopt,
                        std::nullopt);
            }
        };

    if (!extrude_draft_ ||
        !extrude_distance_input_valid_ ||
        document_session_ == nullptr ||
        solid_modeling_kernel_ == nullptr) {
        sync_source_profile(false);
        syncExtrudeUi();
        return;
    }

    auto evaluation =
        document_session_->evaluateExtrudeDraft(
            *extrude_draft_,
            *solid_modeling_kernel_);
    bool preview_ready = false;
    if (evaluation.previewSolidAvailable() &&
        viewport_controller_ != nullptr) {
        const auto tone =
            extrude_draft_->operation() ==
                    part::ExtrudeOperation::cut
                ? viewer::SolidPreviewTone::
                      subtractive
                : viewer::SolidPreviewTone::
                      additive;
        if (viewport_controller_->
                setSolidPreview(
                    *evaluation.preview_delta_mesh,
                    tone)) {
            preview_ready = true;
        } else {
            evaluation.preview_delta_mesh.reset();
        }
    }

    sync_source_profile(preview_ready);
    extrude_evaluation_ =
        std::move(evaluation);
    syncExtrudeUi();
}

void CadWorkbench::syncExtrudeUi() {
    const bool active =
        extrude_draft_.has_value();

    if (extrude_operations_widget_ != nullptr) {
        extrude_operations_widget_->
            setVisible(active);
    }
    if (!active) {
        return;
    }

    syncing_extrude_ui_ = true;

    const bool editing =
        extrude_draft_->mode() ==
        application::ExtrudeDraftMode::edit;
    const bool add =
        extrude_draft_->operation() ==
        part::ExtrudeOperation::add;
    extrude_add_button_->setChecked(add);
    extrude_cut_button_->setChecked(!add);

    const bool one_side =
        extrude_draft_->extentMode() ==
        application::ExtrudeDraftExtentMode::
            one_side;
    extrude_one_side_button_->setChecked(
        one_side);
    extrude_midplane_button_->setChecked(
        !one_side);
    extrude_reverse_button_->setEnabled(
        one_side);
    extrude_reverse_button_->setChecked(
        one_side &&
        extrude_draft_->reversed());

    bool cut_allowed = false;
    if (document_session_ != nullptr) {
        const auto& features =
            document_session_->document()
                .body().features;
        if (!editing) {
            cut_allowed =
                !features.empty();
        } else if (extrude_draft_->featureId()) {
            const auto found =
                std::find_if(
                    features.begin(),
                    features.end(),
                    [this](const part::PartFeature& feature) {
                        return feature.id ==
                            *extrude_draft_->featureId();
                    });
            cut_allowed =
                found != features.end() &&
                found != features.begin();
        }
    }
    extrude_cut_button_->setEnabled(
        cut_allowed);

    const bool committable =
        extrude_distance_input_valid_ &&
        extrude_evaluation_ &&
        extrude_evaluation_->committable();
    extrude_finish_button_->setEnabled(
        committable);

    if (!extrude_distance_input_valid_) {
        extrude_result_label_->setText(
            QStringLiteral(
                "Enter a positive extrusion distance."));
    } else if (extrude_evaluation_) {
        extrude_result_label_->setText(
            extrudeEvaluationText(
                *extrude_evaluation_));
    } else {
        extrude_result_label_->setText(
            QStringLiteral(
                "Preview unavailable."));
    }

    if (extrude_finish_button_ != nullptr) {
        extrude_finish_button_->setText(
            editing
                ? QStringLiteral("Finish Edit")
                : QStringLiteral("Finish Extrude"));
    }

    if (operations_placeholder_ != nullptr) {
        operations_placeholder_->setText(
            QStringLiteral(
                "%1 — %2 · %3%4")
                .arg(
                    editing
                        ? QStringLiteral("Edit Extrude")
                        : QStringLiteral("Extrude"),
                    add
                        ? QStringLiteral("Add")
                        : QStringLiteral("Cut"),
                    one_side
                        ? QStringLiteral("One Side")
                        : QStringLiteral("Midplane"),
                    one_side &&
                            extrude_draft_->reversed()
                        ? QStringLiteral(" · Reverse")
                        : QString{}));
    }

    syncing_extrude_ui_ = false;
}

application::CadInputSubmitResult
CadWorkbench::submitExtrudeCadInput(
    std::string_view text) {
    if (!extrude_draft_ ||
        document_session_ == nullptr) {
        return {
            false,
            "No active Extrude draft."};
    }

    const auto keyword =
        upperAsciiTrimmed(text);
    if (keyword.empty() ||
        keyword == "FINISH") {
        return finishExtrude()
            ? application::CadInputSubmitResult{
                  true, {}}
            : application::CadInputSubmitResult{
                  false,
                  "Extrude Finish was rejected."};
    }
    if (keyword == "CANCEL") {
        cancelExtrude();
        return {true, {}};
    }
    if (keyword == "ADD") {
        if (!extrude_draft_->setOperation(
                part::ExtrudeOperation::add)) {
            return {
                false,
                "ADD could not be applied."};
        }
        refreshExtrudePreview();
        notifyCadInputContextChanged();
        return {true, {}};
    }
    if (keyword == "CUT") {
        const auto& features =
            document_session_->document()
                .body().features;
        bool cut_allowed =
            !features.empty();
        if (extrude_draft_->mode() ==
                application::ExtrudeDraftMode::edit &&
            extrude_draft_->featureId()) {
            const auto found =
                std::find_if(
                    features.begin(),
                    features.end(),
                    [this](const part::PartFeature& feature) {
                        return feature.id ==
                            *extrude_draft_->featureId();
                    });
            cut_allowed =
                found != features.end() &&
                found != features.begin();
        }
        if (!cut_allowed) {
            return {
                false,
                "The first solid-producing Extrude must be ADD."};
        }
        if (!extrude_draft_->setOperation(
                part::ExtrudeOperation::cut)) {
            return {
                false,
                "CUT could not be applied."};
        }
        refreshExtrudePreview();
        notifyCadInputContextChanged();
        return {true, {}};
    }
    if (keyword == "REVERSE") {
        if (extrude_draft_->extentMode() !=
            application::ExtrudeDraftExtentMode::
                one_side) {
            return {
                false,
                "REVERSE is available only for ONESIDE Extrude."};
        }
        if (!extrude_draft_->setReversed(
                !extrude_draft_->reversed())) {
            return {
                false,
                "REVERSE could not be applied."};
        }
        refreshExtrudePreview();
        notifyCadInputContextChanged();
        return {true, {}};
    }
    if (keyword == "MIDPLANE") {
        if (!extrude_draft_->setExtentMode(
                application::
                    ExtrudeDraftExtentMode::
                        midplane)) {
            return {
                false,
                "MIDPLANE could not be applied."};
        }
        refreshExtrudePreview();
        notifyCadInputContextChanged();
        return {true, {}};
    }
    if (keyword == "ONESIDE") {
        if (!extrude_draft_->setExtentMode(
                application::
                    ExtrudeDraftExtentMode::
                        one_side)) {
            return {
                false,
                "ONESIDE could not be applied."};
        }
        refreshExtrudePreview();
        notifyCadInputContextChanged();
        return {true, {}};
    }

    const auto distance =
        application::parseBareCadDistance(
            text,
            application::CadInputNumberFormat{
                toUtf8(
                    QLocale{}.decimalPoint()),
                document_session_->document()
                    .lengthUnit()});
    if (!distance || !(*distance > 0.0)) {
        return {
            false,
            "Extrude expects ADD, CUT, REVERSE, MIDPLANE, ONESIDE, FINISH, CANCEL or a positive Length."};
    }

    if (!setExtrudeDistance(
            core::LengthValue{*distance},
            text)) {
        return {
            false,
            "Extrude distance could not be applied."};
    }
    return {true, {}};
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
        datum_plane_draft_ ||
        sketch_support_pick_active_ ||
        extrude_profile_pick_active_ ||
        extrude_draft_) {
        return;
    }

    if (active_sketch_id_) {
        setStatusText(
            QStringLiteral(
                "Finish the active Sketch before creating another one."));
        return;
    }
    sketch_resupport_target_.reset();
    pending_sketch_support_.reset();
    pending_sketch_support_revision_.reset();
    sketch_support_pick_active_ = true;
    ++sketch_support_pick_generation_;
    if (viewport_controller_ != nullptr) {
        viewport_controller_->
            setBodyTopologyFacePickOnly(true);
    }
    operations_placeholder_->setText(
        QStringLiteral(
            "Sketch: select XY/XZ/YZ Origin plane, a Body Face or a Datum Plane."));
    setStatusText(
        QStringLiteral(
            "Sketch tool active — select an Origin plane, Body Face or Datum Plane."));
    notifyCadInputContextChanged();
    syncActionState();
}

void CadWorkbench::startSketchResupport(
    const sketch::SketchId& sketch_id) {
    auto* document_session =
        activeDocumentSession();
    if (document_session == nullptr ||
        datum_plane_draft_ ||
        sketch_support_pick_active_ ||
        extrude_profile_pick_active_ ||
        extrude_draft_) {
        return;
    }
    if (active_sketch_id_) {
        setStatusText(
            QStringLiteral(
                "Finish the active Sketch edit before changing support."));
        return;
    }
    if (document_session->document()
            .findSketch(sketch_id) == nullptr) {
        setStatusText(
            QStringLiteral(
                "Sketch support change is unavailable."));
        return;
    }

    sketch_resupport_target_ = sketch_id;
    pending_sketch_support_.reset();
    pending_sketch_support_revision_.reset();
    sketch_support_pick_active_ = true;
    ++sketch_support_pick_generation_;
    if (viewport_controller_ != nullptr) {
        viewport_controller_->
            setBodyTopologyFacePickOnly(true);
    }
    operations_placeholder_->setText(
        QStringLiteral(
            "Change Sketch Support: select XY/XZ/YZ Origin plane, a Body Face or a Datum Plane."));
    setStatusText(
        QStringLiteral(
            "Re-support active — select a new Origin plane, Body Face or Datum Plane."));
    notifyCadInputContextChanged();
    syncActionState();
}

void CadWorkbench::cancelSketchTool() {
    if (!sketch_support_pick_active_) {
        return;
    }

    const bool resupport =
        sketch_resupport_target_.has_value();
    sketch_support_pick_active_ = false;
    sketch_resupport_target_.reset();
    pending_sketch_support_.reset();
    pending_sketch_support_revision_.reset();
    ++sketch_support_pick_generation_;
    if (viewport_controller_ != nullptr) {
        viewport_controller_->
            setBodyTopologyFacePickOnly(false);
        viewport_controller_->
            clearSketchDynamicInputOverlay();
    }
    dynamic_input_anchor_.reset();
    setStatusText(
        resupport
            ? QStringLiteral(
                  "Sketch support change cancelled — no authored state changed.")
            : QStringLiteral(
                  "Sketch creation cancelled — no authored state changed."));
    notifyCadInputContextChanged();
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
        cancelSketchTool();
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

    sketch_interaction_controller_->
        setSelectedProfileForCadInput(profile_id);
    refreshProfileProperties(profile_id);

    if (viewport_widget_ != nullptr) {
        viewport_widget_->setFocus(
            Qt::OtherFocusReason);
    }
    setStatusText(
        QStringLiteral(
            "Profile edit context opened."));
}

void CadWorkbench::stageSketchSupport(
    part::PartSketchSupport support) {
    if (!sketch_support_pick_active_) {
        return;
    }

    auto* document_session =
        activeDocumentSession();
    if (document_session == nullptr) {
        cancelSketchTool();
        return;
    }

    pending_sketch_support_ =
        std::move(support);
    pending_sketch_support_revision_ =
        document_session->document().revision();
    ++sketch_support_pick_generation_;

    setStatusText(
        sketch_resupport_target_
            ? QStringLiteral(
                  "New Sketch support selected — Apply Support to commit or Cancel.")
            : QStringLiteral(
                  "Sketch support selected — Create Sketch to commit or Cancel."));
    notifyCadInputContextChanged();
    syncActionState();
}

bool CadWorkbench::finishSketchSupport() {
    if (!sketch_support_pick_active_ ||
        !pending_sketch_support_ ||
        !pending_sketch_support_revision_) {
        setStatusText(
            sketch_resupport_target_
                ? QStringLiteral(
                      "Select an Origin plane, Body Face or Datum Plane before Apply Support.")
                : QStringLiteral(
                      "Select an Origin plane, Body Face or Datum Plane before Create Sketch."));
        return false;
    }

    auto* document_session =
        activeDocumentSession();
    if (document_session == nullptr) {
        cancelSketchTool();
        return false;
    }

    const bool resupport =
        sketch_resupport_target_.has_value();

    application::SketchSupportMutationResult result;
    if (resupport) {
        result =
            document_session->execute(
                application::SetPartSketchSupportCommand{
                    *sketch_resupport_target_,
                    *pending_sketch_support_,
                    *pending_sketch_support_revision_},
                solid_modeling_kernel_);
    } else {
        result =
            document_session->execute(
                application::CreatePartSketchOnSupportCommand{
                    *pending_sketch_support_,
                    *pending_sketch_support_revision_},
                solid_modeling_kernel_);
    }

    if (!result.ok()) {
        if (result.status ==
            application::SketchSupportMutationStatus::
                stale_revision) {
            pending_sketch_support_.reset();
            pending_sketch_support_revision_.reset();
            ++sketch_support_pick_generation_;
            notifyCadInputContextChanged();
            syncActionState();
        }
        setStatusText(
            fromUtf8(
                result.diagnostic.message.empty()
                    ? std::string{
                          "Sketch support Finish was rejected."}
                    : result.diagnostic.message));
        return false;
    }

    if (!result.sketch_id) {
        setStatusText(
            QStringLiteral(
                "Sketch support Finish produced no Sketch target."));
        return false;
    }

    const auto target_id =
        *result.sketch_id;
    sketch_support_pick_active_ = false;
    sketch_resupport_target_.reset();
    pending_sketch_support_.reset();
    pending_sketch_support_revision_.reset();
    ++sketch_support_pick_generation_;
    if (viewport_controller_ != nullptr) {
        viewport_controller_->
            setBodyTopologyFacePickOnly(false);
    }

    refreshActiveContext();
    enterSketchEdit(target_id);
    notifyCadInputContextChanged();

    setStatusText(
        resupport
            ? QStringLiteral(
                  "Sketch support changed — one transaction committed; SketchId and local geometry preserved.")
            : QStringLiteral(
                  "Sketch created — one transaction committed; editing in the 3D Viewport."));
    return true;
}

void CadWorkbench::tryCreateSketchFromSupport(
    std::optional<core::BuiltinReferenceRole> support) {
    if (!sketch_support_pick_active_ ||
        !support) {
        return;
    }

    const auto semantic =
        part::partSketchSupportForBuiltinPlane(
            *support);
    if (!semantic) {
        setStatusText(
            QStringLiteral(
                "Sketch support must be XY, XZ or YZ Origin plane."));
        return;
    }

    stageSketchSupport(*semantic);
}

void CadWorkbench::tryCreateSketchFromDatum(
    std::optional<part::DatumId> datum_id) {
    if (!sketch_support_pick_active_ ||
        !datum_id) {
        return;
    }

    const auto semantic =
        part::partSketchSupportForDatumPlane(
            *datum_id);
    if (!semantic) {
        setStatusText(
            QStringLiteral(
                "Selected Datum Plane produced an invalid Sketch support reference."));
        return;
    }

    stageSketchSupport(*semantic);
}

void CadWorkbench::tryCreateSketchFromBodyTopology(
    const BodyTopologyInspection& inspection) {
    if (!sketch_support_pick_active_ ||
        inspection.kind !=
            viewer::BodyTopologyPresentationKind::
                face) {
        return;
    }

    switch (inspection.sketch_support) {
    case SketchSupportInspectionCapability::supported:
        break;
    case SketchSupportInspectionCapability::
        unsupported_non_planar:
        setStatusText(
            QStringLiteral(
                "Unsupported — the selected Body Face is non-planar."));
        return;
    case SketchSupportInspectionCapability::missing:
        setStatusText(
            QStringLiteral(
                "Sketch support is Missing at the current Body stage."));
        return;
    case SketchSupportInspectionCapability::ambiguous:
        setStatusText(
            QStringLiteral(
                "Sketch support is Ambiguous at the current Body stage."));
        return;
    case SketchSupportInspectionCapability::unsupported:
    case SketchSupportInspectionCapability::not_applicable:
        setStatusText(
            QStringLiteral(
                "Selected Face cannot provide standard Sketch support."));
        return;
    }

    if (!inspection.stage.valid() ||
        !inspection.surface_address) {
        setStatusText(
            QStringLiteral(
                "Selected Face has no singular semantic Surface support."));
        return;
    }

    const auto semantic =
        part::partSketchSupportForBodyPlanarSurface(
            part::SurfaceReference{
                inspection.stage,
                *inspection.surface_address});
    if (!semantic) {
        setStatusText(
            QStringLiteral(
                "Selected Face produced an invalid semantic Surface reference."));
        return;
    }

    stageSketchSupport(*semantic);
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
        if (const auto origin_plane =
                part::builtinOriginPlaneForSketchSupport(
                    sketch->support)) {
            const auto standard_view =
                standardViewForSketchSupport(
                    *origin_plane);
            if (standard_view) {
                static_cast<void>(
                    viewport_->setStandardView(
                        *standard_view));
            }
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

void CadWorkbench::activateSketchTrim() {
    if (!sketch_interaction_controller_ ||
        !sketch_interaction_controller_->activateTrim()) {
        setStatusText(
            QStringLiteral(
                "TRIM could not start with the current preselection."));
        syncSketchInteractionUi();
        return;
    }
    if (viewport_widget_ != nullptr) {
        viewport_widget_->setFocus(
            Qt::OtherFocusReason);
    }
}

void CadWorkbench::activateSketchExtend() {
    if (!sketch_interaction_controller_ ||
        !sketch_interaction_controller_->activateExtend()) {
        setStatusText(
            QStringLiteral(
                "EXTEND could not start with the current preselection."));
        syncSketchInteractionUi();
        return;
    }
    if (viewport_widget_ != nullptr) {
        viewport_widget_->setFocus(
            Qt::OtherFocusReason);
    }
}

void CadWorkbench::activateSketchExtendBoth() {
    if (!sketch_interaction_controller_) {
        return;
    }
    sketch_interaction_controller_->activateExtendBoth();
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
    case sketch::SketchTool::trim:
        setStatusText(
            QStringLiteral("Trim finished — Select active."));
        break;
    case sketch::SketchTool::extend:
        setStatusText(
            QStringLiteral("Extend finished — Select active."));
        break;
    case sketch::SketchTool::extend_both:
        setStatusText(
            QStringLiteral("Extend Both finished — Select active."));
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
    case sketch::SketchTool::trim:
        setStatusText(
            QStringLiteral("Trim cancelled — committed edits and boundary selection preserved."));
        break;
    case sketch::SketchTool::extend:
        setStatusText(
            QStringLiteral("Extend cancelled — committed edits and boundary selection preserved."));
        break;
    case sketch::SketchTool::extend_both:
        setStatusText(
            QStringLiteral("Extend Both cancelled — committed edits preserved."));
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
    if (sketch_support_pick_active_) {
        constexpr application::CadInputContextGeneration
            sketch_support_namespace =
                application::CadInputContextGeneration{
                    1ULL << 61U};
        return sketch_support_namespace |
               (sketch_support_pick_generation_ &
                (sketch_support_namespace - 1U));
    }
    if (extrude_profile_pick_active_) {
        constexpr application::CadInputContextGeneration
            extrude_pick_namespace =
                application::CadInputContextGeneration{
                    1ULL << 62U};
        return extrude_pick_namespace |
               (extrude_profile_pick_generation_ &
                (extrude_pick_namespace - 1U));
    }
    if (datum_plane_draft_) {
        constexpr application::CadInputContextGeneration
            datum_plane_namespace =
                application::CadInputContextGeneration{
                    1ULL << 60U};
        return datum_plane_namespace |
               (datum_plane_draft_->generation() &
                (datum_plane_namespace - 1U));
    }
    if (extrude_draft_) {
        constexpr application::CadInputContextGeneration
            extrude_namespace =
                application::CadInputContextGeneration{
                    1ULL << 63U};
        return extrude_namespace |
               (extrude_draft_->generation() &
                (extrude_namespace - 1U));
    }
    return sketch_interaction_controller_
               ? sketch_interaction_controller_->
                     cadInputContextGeneration()
               : application::CadInputContextGeneration{};
}

std::vector<application::CadDynamicInputField>
CadWorkbench::cadDynamicInputFields() const {
    if (sketch_support_pick_active_ ||
        extrude_profile_pick_active_) {
        return {};
    }
    if (datum_plane_draft_) {
        return {
            application::CadDynamicInputField{
                application::
                    CadDynamicInputFieldSemantic::
                        distance,
                "Offset"}};
    }
    if (extrude_draft_) {
        return {
            application::CadDynamicInputField{
                application::
                    CadDynamicInputFieldSemantic::
                        distance,
                "Distance"}};
    }

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
    if (datum_plane_draft_) {
        if (index != 0U ||
            document_session_ == nullptr) {
            return {
                false,
                "Datum Plane has one Offset input field."};
        }

        const auto quantity =
            application::parseCadQuantity(
                text,
                {
                    application::CadQuantityDimension::length,
                    document_session_->document()
                        .lengthUnit()});
        if (!quantity ||
            !datum_plane_draft_->setOffset(
                core::LengthValue{
                    quantity->canonical_value})) {
            return {
                false,
                "Datum Plane Offset expects a finite signed Length."};
        }

        datum_plane_offset_input_valid_ = true;
        datum_plane_evaluation_.reset();
        if (datum_plane_offset_edit_ != nullptr) {
            const QSignalBlocker blocked{
                datum_plane_offset_edit_};
            datum_plane_offset_edit_->setText(
                fromUtf8(text));
        }
        refreshDatumPlaneEvaluation();
        notifyCadInputContextChanged();
        return {true, {}};
    }

    if (extrude_draft_) {
        if (index != 0U ||
            document_session_ == nullptr) {
            return {
                false,
                "Extrude has one Distance input field."};
        }
        const auto distance =
            application::parseBareCadDistance(
                text,
                application::CadInputNumberFormat{
                    toUtf8(
                        QLocale{}.decimalPoint()),
                    document_session_->document()
                        .lengthUnit()});
        if (!distance || !(*distance > 0.0) ||
            !setExtrudeDistance(
                core::LengthValue{*distance},
                text)) {
            return {
                false,
                "Extrude Distance expects a positive Length."};
        }
        return {true, {}};
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
    if (datum_plane_draft_) {
        return finishDatumPlane()
            ? application::CadInputSubmitResult{
                  true, {}}
            : application::CadInputSubmitResult{
                  false,
                  "Datum Plane Finish was rejected."};
    }
    if (extrude_draft_) {
        return finishExtrude()
            ? application::CadInputSubmitResult{
                  true, {}}
            : application::CadInputSubmitResult{
                  false,
                  "Extrude Finish was rejected."};
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

    if (sketch_support_pick_active_) {
        const auto keyword =
            upperAsciiTrimmed(text);
        if (keyword == "CANCEL" ||
            keyword == "ESC") {
            cancelSketchTool();
            return {true, {}};
        }
        if ((keyword == "FINISH" ||
             keyword.empty()) &&
            pending_sketch_support_) {
            return finishSketchSupport()
                ? application::CadInputSubmitResult{
                      true, {}}
                : application::CadInputSubmitResult{
                      false,
                      "Sketch support Finish was rejected."};
        }
        const bool expected_command =
            sketch_resupport_target_
                ? keyword == "RESUPPORT"
                : keyword == "SKETCH";
        if (expected_command) {
            return {true, {}};
        }
        return {
            false,
            pending_sketch_support_
                ? (sketch_resupport_target_
                       ? "RESUPPORT target selected; use FINISH/Enter to commit or CANCEL."
                       : "SKETCH support selected; use FINISH/Enter to create or CANCEL.")
                : (sketch_resupport_target_
                       ? "RESUPPORT is waiting for an Origin plane or Body Face selection; use Tree/viewport or CANCEL."
                       : "SKETCH is waiting for an Origin plane or Body Face selection; use Tree/viewport or CANCEL.")};
    }

    if (extrude_profile_pick_active_) {
        const auto keyword =
            upperAsciiTrimmed(text);
        if (keyword == "CANCEL" ||
            keyword == "ESC") {
            cancelExtrudeProfilePick();
            return {true, {}};
        }
        if (keyword == "EXTRUDE") {
            return {true, {}};
        }
        return {
            false,
            "EXTRUDE is waiting for one valid Profile selection; use Tree/viewport or CANCEL."};
    }

    if (axis_draft_) {
        auto result =
            submitAxisCadInput(text);
        if (!result.accepted &&
            status_ != nullptr &&
            !result.diagnostic.empty()) {
            setStatusText(
                fromUtf8(result.diagnostic));
        }
        return result;
    }

    if (datum_plane_draft_) {
        auto result =
            submitDatumPlaneCadInput(text);
        if (!result.accepted &&
            status_ != nullptr &&
            !result.diagnostic.empty()) {
            setStatusText(
                fromUtf8(result.diagnostic));
        }
        return result;
    }

    if (extrude_draft_) {
        auto result =
            submitExtrudeCadInput(text);
        if (!result.accepted &&
            status_ != nullptr &&
            !result.diagnostic.empty()) {
            setStatusText(
                fromUtf8(result.diagnostic));
        }
        return result;
    }

    const auto top_level_keyword =
        upperAsciiTrimmed(text);
    if (top_level_keyword == "AXIS") {
        return startAxisTool()
            ? application::CadInputSubmitResult{
                  true, {}}
            : application::CadInputSubmitResult{
                  false,
                  "AXIS could not be activated."};
    }
    if (top_level_keyword == "DATUMPLANE" ||
        top_level_keyword == "DATUM PLANE") {
        return startDatumPlaneTool()
            ? application::CadInputSubmitResult{
                  true, {}}
            : application::CadInputSubmitResult{
                  false,
                  "DATUM PLANE could not be activated."};
    }
    if (top_level_keyword == "EXTRUDE") {
        return startExtrudeTool()
            ? application::CadInputSubmitResult{
                  true, {}}
            : application::CadInputSubmitResult{
                  false,
                  "EXTRUDE could not be activated."};
    }
    if (top_level_keyword == "SKETCH") {
        startSketchTool();
        return sketch_support_pick_active_ &&
                       !sketch_resupport_target_
            ? application::CadInputSubmitResult{
                  true, {}}
            : application::CadInputSubmitResult{
                  false,
                  "SKETCH could not be activated."};
    }
    if (top_level_keyword == "RESUPPORT") {
        const auto sketch_id =
            tree_controller_ != nullptr
                ? tree_controller_->primarySketchId()
                : std::nullopt;
        if (!sketch_id) {
            return {
                false,
                "RESUPPORT requires one Sketch selected in the document Tree."};
        }
        startSketchResupport(*sketch_id);
        return sketch_support_pick_active_ &&
                       sketch_resupport_target_ &&
                       *sketch_resupport_target_ ==
                           *sketch_id
            ? application::CadInputSubmitResult{
                  true, {}}
            : application::CadInputSubmitResult{
                  false,
                  "RESUPPORT could not be activated."};
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
    if (sketch_support_pick_active_) {
        if (pending_sketch_support_) {
            return sketch_resupport_target_
                ? QStringLiteral(
                      "Command: RESUPPORT — Support selected · FINISH/Enter or CANCEL/Esc")
                : QStringLiteral(
                      "Command: SKETCH — Support selected · FINISH/Enter or CANCEL/Esc");
        }
        return sketch_resupport_target_
            ? QStringLiteral(
                  "Command: RESUPPORT — Select XY/XZ/YZ Origin plane or Body Face · CANCEL/Esc")
            : QStringLiteral(
                  "Command: SKETCH — Select XY/XZ/YZ Origin plane or Body Face · CANCEL/Esc");
    }
    if (axis_draft_) {
        return axis_draft_->source()
            ? QStringLiteral(
                  "Command: AXIS — SOURCE · FINISH/CANCEL")
            : QStringLiteral(
                  "Command: AXIS — Select exactly one Line in an active Sketch · SOURCE · CANCEL/Esc");
    }
    if (datum_plane_draft_) {
        return datum_plane_draft_->source()
            ? QStringLiteral(
                  "Command: DATUM PLANE — OFFSET · signed Length · REVERSE · FINISH/CANCEL")
            : QStringLiteral(
                  "Command: DATUM PLANE — Select XY/XZ/YZ Origin plane, planar Body Face or existing Datum Plane · CANCEL/Esc");
    }
    if (extrude_profile_pick_active_) {
        return QStringLiteral(
            "Command: EXTRUDE — Select one valid Profile · CANCEL/Esc");
    }
    if (extrude_draft_) {
        return QStringLiteral(
            "Command: EXTRUDE — ADD/CUT · ONESIDE/MIDPLANE · REVERSE · Distance · FINISH/CANCEL");
    }

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

    if (tool == sketch::SketchTool::trim) {
        return sketch_interaction_controller_->
                       structuralBoundarySelectionPending()
                   ? QStringLiteral(
                         "Command: TRIM — Select finite boundaries; Enter/RMB to continue")
                   : QStringLiteral(
                         "Command: TRIM — Click target fragment");
    }

    if (tool == sketch::SketchTool::extend) {
        return sketch_interaction_controller_->
                       structuralBoundarySelectionPending()
                   ? QStringLiteral(
                         "Command: EXTEND — Select finite boundaries; Enter/RMB to continue")
                   : QStringLiteral(
                         "Command: EXTEND — Click target end");
    }

    if (tool == sketch::SketchTool::extend_both) {
        return sketch_interaction_controller_->
                       extendBothFirstLine()
                   ? QStringLiteral(
                         "Command: EXTEND BOTH — Choose second Line")
                   : QStringLiteral(
                         "Command: EXTEND BOTH — Choose first Line");
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
    if (sketch_support_pick_active_) {
        static_cast<void>(
            finishSketchSupport());
        return;
    }
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
    if (sketch_support_pick_active_ ||
        sketch_resupport_target_) {
        ++sketch_support_pick_generation_;
    }
    sketch_support_pick_active_ = false;
    sketch_resupport_target_.reset();
    pending_sketch_support_.reset();
    pending_sketch_support_revision_.reset();
    dynamic_input_anchor_.reset();
    if (viewport_controller_ != nullptr) {
        viewport_controller_->
            setBodyTopologyFacePickOnly(false);
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
    refreshPartFeatureEvaluationSnapshot();
    reconcileSketchRuntimeContext();
    if (axis_draft_) {
        refreshAxisEvaluation();
    }
    if (datum_plane_draft_) {
        refreshDatumPlaneEvaluation();
    }
    if (extrude_draft_) {
        refreshExtrudePreview();
    }
    if (selected_axis_id_) {
        refreshAxisProperties(
            *selected_axis_id_);
    } else if (selected_datum_id_) {
        refreshDatumProperties(
            *selected_datum_id_);
    } else if (selected_feature_id_) {
        refreshFeatureProperties(
            *selected_feature_id_);
    } else if (selected_body_id_) {
        refreshBodyProperties(
            *selected_body_id_);
    }
    syncActionState();
    notifyDocumentStateChanged();
}

void CadWorkbench::clearActiveContext() {
    clearAxisRuntimeContext();
    clearDatumPlaneRuntimeContext();
    clearExtrudeRuntimeContext();
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
    selected_axis_id_.reset();
    selected_datum_id_.reset();
    selected_feature_id_.reset();
    selected_body_id_.reset();
    part_evaluation_revision_.reset();
    body_evaluation_status_.reset();
    feature_evaluation_statuses_.clear();
    axis_evaluation_statuses_.clear();
    datum_evaluation_statuses_.clear();
    axis_name_->clear();
    axis_identity_->clear();
    axis_source_sketch_->clear();
    axis_source_line_->clear();
    axis_visibility_->clear();
    axis_status_->clear();
    axis_diagnostic_->clear();
    axis_origin_->clear();
    axis_direction_->clear();
    axis_edit_button_->setEnabled(false);
    axis_delete_button_->setEnabled(false);
    datum_name_->clear();
    datum_identity_->clear();
    datum_constructor_->setText(
        QStringLiteral("Offset"));
    datum_source_->clear();
    datum_offset_->clear();
    datum_visibility_->clear();
    datum_status_->clear();
    datum_diagnostic_->clear();
    datum_edit_button_->setEnabled(false);
    datum_delete_button_->setEnabled(false);
    profile_name_->clear();
    profile_identity_->clear();
    profile_source_->clear();
    profile_status_->clear();
    profile_diagnostic_->clear();
    profile_area_->clear();
    profile_perimeter_->clear();
    profile_holes_->clear();
    profile_consuming_features_->clear();
    profile_go_to_feature_button_->setEnabled(
        false);
    body_identity_->clear();
    body_status_->clear();
    body_feature_count_->clear();
    feature_name_->clear();
    feature_identity_->clear();
    feature_status_->clear();
    feature_diagnostic_->clear();
    feature_operation_->clear();
    feature_extent_->clear();
    feature_distance_->clear();
    feature_direction_->clear();
    feature_source_profile_->clear();
    feature_source_sketch_->clear();
    feature_suppress_button_->setText(
        QStringLiteral("Suppress Feature"));
    feature_suppress_button_->setEnabled(false);
    feature_delete_button_->setEnabled(false);
    profile_visible_->setCheckState(
        Qt::PartiallyChecked);

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
    profile_visible_->setCheckState(
        checkStateForProfileVisibility(
            profile->visibility));

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

    {
        const QSignalBlocker blocked{
            profile_consuming_features_};
        profile_consuming_features_->clear();
        for (const auto& feature :
             document_session->document()
                 .body().features) {
            const auto source =
                part::sourceProfileId(feature);
            if (!source ||
                *source != profile_id) {
                continue;
            }
            const auto label =
                feature.name.empty()
                    ? QStringLiteral("Feature %1")
                          .arg(fromUtf8(
                              feature.id.serialized()))
                    : fromUtf8(feature.name);
            profile_consuming_features_->
                addItem(
                    label,
                    fromUtf8(
                        feature.id.serialized()));
        }
    }
    profile_go_to_feature_button_->setEnabled(
        profile_consuming_features_->
            count() > 0);

    bool explicit_profile_delete_enabled = true;
    if (sketch_interaction_controller_ &&
        sketch_interaction_controller_->active() &&
        sketch_interaction_controller_->profileToolActive()) {
        const auto edited_profile_id =
            sketch_interaction_controller_->
                editedProfileId();
        explicit_profile_delete_enabled =
            edited_profile_id &&
            *edited_profile_id == profile_id;
    }
    delete_profile_button_->setEnabled(
        explicit_profile_delete_enabled);

    properties_stack_->setCurrentWidget(
        profile_properties_page_);
}


void CadWorkbench::refreshPartFeatureEvaluationSnapshot() {
    if (tree_controller_ == nullptr ||
        document_session_ == nullptr) {
        part_evaluation_revision_.reset();
        body_evaluation_status_.reset();
        feature_evaluation_statuses_.clear();
        axis_evaluation_statuses_.clear();
        datum_evaluation_statuses_.clear();
        return;
    }

    feature_evaluation_statuses_.clear();
    axis_evaluation_statuses_.clear();
    datum_evaluation_statuses_.clear();
    auto body_status =
        document_session_->document()
                .body().features.empty()
            ? part::BodyEvaluationStatus::empty
            : part::BodyEvaluationStatus::
                  unavailable;

    if (solid_modeling_kernel_ != nullptr) {
        const auto evaluation =
            part::evaluatePart(
                document_session_->document(),
                *solid_modeling_kernel_);
        body_status =
            evaluation.body_status;
        feature_evaluation_statuses_.reserve(
            evaluation.features.size());
        for (const auto& feature :
             evaluation.features) {
            feature_evaluation_statuses_.push_back(
                FeatureEvaluationUiState{
                    feature.feature_id,
                    feature.status,
                    feature.diagnostic});
        }
        const auto datums =
            part::evaluateDatums(
                document_session_->document(),
                evaluation);
        if (datums.source_revision ==
                document_session_->document()
                    .revision() &&
            datums.planes.size() ==
                document_session_->document()
                    .datumPlanes()
                    .size()) {
            datum_evaluation_statuses_.reserve(
                datums.planes.size());
            for (const auto& datum :
                 datums.planes) {
                datum_evaluation_statuses_.push_back(
                    DatumEvaluationUiState{
                        datum.datum_id,
                        datum.status,
                        datum.diagnostic});
            }
        }

        axis_evaluation_statuses_.reserve(
            document_session_->document()
                .axes()
                .size());
        for (const auto& axis :
             document_session_->document()
                 .axes()) {
            const part::AxisReference reference{
                part::AuthoredAxisReference{
                    axis.id}};
            const auto axis_evaluation =
                part::resolveAxisReference(
                    document_session_->document(),
                    reference,
                    &evaluation,
                    &datums);
            axis_evaluation_statuses_.push_back(
                AxisEvaluationUiState{
                    axis.id,
                    axis_evaluation.status,
                    axis_evaluation.diagnostic,
                    axis_evaluation.line});
        }
    } else {
        feature_evaluation_statuses_.reserve(
            document_session_->document()
                .body().features.size());
        for (const auto& feature :
             document_session_->document()
                 .body().features) {
            feature_evaluation_statuses_.push_back(
                FeatureEvaluationUiState{
                    feature.id,
                    feature.suppressed
                        ? part::FeatureEvaluationStatus::
                              suppressed
                        : part::FeatureEvaluationStatus::
                              blocked,
                    part::FeatureEvaluationDiagnosticCode::
                        none});
        }
    }

    part_evaluation_revision_ =
        document_session_->document()
            .revision();
    body_evaluation_status_ =
        body_status;

    std::vector<FeatureTreeEvaluationEntry>
        entries;
    entries.reserve(
        feature_evaluation_statuses_.size());
    for (const auto& feature :
         feature_evaluation_statuses_) {
        entries.push_back(
            FeatureTreeEvaluationEntry{
                feature.feature_id,
                feature.status,
                feature.diagnostic});
    }

    std::vector<DatumTreeEvaluationEntry>
        datum_entries;
    datum_entries.reserve(
        datum_evaluation_statuses_.size());
    for (const auto& datum :
         datum_evaluation_statuses_) {
        datum_entries.push_back(
            DatumTreeEvaluationEntry{
                datum.datum_id,
                datum.status,
                datum.diagnostic});
    }

    std::vector<AxisTreeEvaluationEntry>
        axis_entries;
    axis_entries.reserve(
        axis_evaluation_statuses_.size());
    for (const auto& axis :
         axis_evaluation_statuses_) {
        axis_entries.push_back(
            AxisTreeEvaluationEntry{
                axis.axis_id,
                axis.status,
                axis.diagnostic});
    }

    tree_controller_->setEvaluationSnapshot(
        body_status,
        std::move(entries),
        std::move(datum_entries),
        std::move(axis_entries));
}

void CadWorkbench::refreshAxisProperties(
    part::AxisId axis_id) {
    if (properties_stack_ == nullptr ||
        document_session_ == nullptr) {
        return;
    }

    const auto* axis =
        document_session_->document()
            .findAxis(axis_id);
    if (axis == nullptr) {
        selected_axis_id_.reset();
        properties_stack_->setCurrentWidget(
            document_properties_page_);
        return;
    }

    selected_axis_id_ = axis_id;
    axis_name_->setText(
        axis->name.empty()
            ? QStringLiteral("Axis %1")
                  .arg(fromUtf8(
                      axis->id.serialized()))
            : fromUtf8(axis->name));
    axis_identity_->setText(
        fromUtf8(
            axis->id.serialized()));
    axis_source_sketch_->setText(
        fromUtf8(
            axis->source.sketch_id.value()));
    axis_source_line_->setText(
        fromUtf8(
            axis->source.entity_id.serialized()));
    axis_visibility_->setText(
        axis->visible
            ? QStringLiteral("Shown")
            : QStringLiteral("Hidden"));

    const auto evaluation =
        std::find_if(
            axis_evaluation_statuses_.begin(),
            axis_evaluation_statuses_.end(),
            [axis_id](
                const AxisEvaluationUiState& item) {
                return item.axis_id == axis_id;
            });

    if (evaluation ==
        axis_evaluation_statuses_.end()) {
        axis_status_->setText(
            QStringLiteral("Not evaluated"));
        axis_diagnostic_->setText(
            QStringLiteral("—"));
        axis_origin_->setText(
            QStringLiteral("—"));
        axis_direction_->setText(
            QStringLiteral("—"));
    } else {
        axis_status_->setText(
            axisEvaluationStatusText(
                evaluation->status));
        axis_diagnostic_->setText(
            axisEvaluationDiagnosticText(
                evaluation->diagnostic));

        if (evaluation->line &&
            evaluation->line->valid()) {
            const auto& origin =
                evaluation->line->origin;
            const auto& direction =
                evaluation->line->direction;
            axis_origin_->setText(
                QStringLiteral(
                    "(%1, %2, %3)")
                    .arg(
                        QString::number(
                            origin.x, 'g', 12),
                        QString::number(
                            origin.y, 'g', 12),
                        QString::number(
                            origin.z, 'g', 12)));
            axis_direction_->setText(
                QStringLiteral(
                    "(%1, %2, %3)")
                    .arg(
                        QString::number(
                            direction.x, 'g', 12),
                        QString::number(
                            direction.y, 'g', 12),
                        QString::number(
                            direction.z, 'g', 12)));
        } else {
            axis_origin_->setText(
                QStringLiteral("—"));
            axis_direction_->setText(
                QStringLiteral("—"));
        }
    }

    const bool lifecycle_available =
        !axis_draft_ &&
        !datum_plane_draft_ &&
        !extrude_profile_pick_active_ &&
        !extrude_draft_ &&
        !active_sketch_id_ &&
        !sketch_support_pick_active_;
    axis_edit_button_->setEnabled(
        lifecycle_available &&
        solid_modeling_kernel_ != nullptr);
    axis_delete_button_->setEnabled(
        lifecycle_available);

    properties_stack_->setCurrentWidget(
        axis_properties_page_);
}


void CadWorkbench::refreshDatumProperties(
    part::DatumId datum_id) {
    if (properties_stack_ == nullptr ||
        document_session_ == nullptr) {
        return;
    }

    const auto& datums =
        document_session_->document()
            .datumPlanes();
    const auto found =
        std::find_if(
            datums.begin(),
            datums.end(),
            [datum_id](const auto& datum) {
                return datum.id == datum_id;
            });
    if (found == datums.end()) {
        selected_datum_id_.reset();
        properties_stack_->setCurrentWidget(
            document_properties_page_);
        return;
    }

    selected_datum_id_ = datum_id;
    const auto ordinal =
        static_cast<qulonglong>(
            std::distance(
                datums.begin(),
                found) +
            1);
    datum_name_->setText(
        QStringLiteral("Datum Plane %1")
            .arg(ordinal));
    datum_identity_->setText(
        fromUtf8(
            found->id.serialized()));
    datum_constructor_->setText(
        QStringLiteral("Offset"));
    datum_source_->setText(
        datumPlaneSourceText(
            found->source));
    datum_offset_->setText(
        formatLengthForPart(
            found->offset,
            document_session_->document()
                .lengthUnit()));
    datum_visibility_->setText(
        found->visible
            ? QStringLiteral("Shown")
            : QStringLiteral("Hidden"));

    const auto revision =
        document_session_->document()
            .revision();
    const auto evaluation =
        part_evaluation_revision_ &&
                *part_evaluation_revision_ ==
                    revision
            ? std::find_if(
                  datum_evaluation_statuses_
                      .begin(),
                  datum_evaluation_statuses_
                      .end(),
                  [datum_id](
                      const DatumEvaluationUiState&
                          item) {
                      return item.datum_id ==
                             datum_id;
                  })
            : datum_evaluation_statuses_.end();

    if (evaluation !=
        datum_evaluation_statuses_.end()) {
        datum_status_->setText(
            datumEvaluationStatusText(
                evaluation->status));
        datum_diagnostic_->setText(
            datumEvaluationDiagnosticText(
                evaluation->diagnostic));
    } else {
        datum_status_->setText(
            QStringLiteral("Not evaluated"));
        datum_diagnostic_->setText(
            QStringLiteral("—"));
    }

    const bool lifecycle_available =
        !datum_plane_draft_ &&
        !extrude_profile_pick_active_ &&
        !extrude_draft_ &&
        !active_sketch_id_ &&
        !sketch_support_pick_active_;
    datum_edit_button_->setEnabled(
        lifecycle_available &&
        solid_modeling_kernel_ != nullptr);
    datum_delete_button_->setEnabled(
        lifecycle_available);

    properties_stack_->setCurrentWidget(
        datum_properties_page_);
}

void CadWorkbench::refreshBodyProperties(
    part::BodyId body_id) {
    if (properties_stack_ == nullptr ||
        document_session_ == nullptr ||
        document_session_->document()
                .body().id != body_id) {
        return;
    }

    selected_body_id_ = body_id;
    body_identity_->setText(
        fromUtf8(body_id.serialized()));
    body_feature_count_->setText(
        QString::number(
            static_cast<qulonglong>(
                document_session_->document()
                    .body().features.size())));

    const auto current_revision =
        document_session_->document()
            .revision();
    const auto status =
        part_evaluation_revision_ &&
                *part_evaluation_revision_ ==
                    current_revision &&
                body_evaluation_status_
            ? *body_evaluation_status_
            : (document_session_->document()
                       .body().features.empty()
                   ? part::BodyEvaluationStatus::empty
                   : part::BodyEvaluationStatus::
                         unavailable);
    body_status_->setText(
        bodyEvaluationStatusText(status));

    const auto topology =
        status ==
                part::BodyEvaluationStatus::
                    up_to_date &&
                viewport_controller_ != nullptr
            ? viewport_controller_->
                  bodyTopologySummary()
            : std::nullopt;
    if (topology) {
        body_topology_counts_->setText(
            QStringLiteral(
                "Faces %1; Edges %2; Vertices %3")
                .arg(static_cast<qulonglong>(
                    topology->faces.total))
                .arg(static_cast<qulonglong>(
                    topology->edges.total))
                .arg(static_cast<qulonglong>(
                    topology->vertices.total)));
        body_topology_accounting_->setText(
            QStringLiteral(
                "Faces: %1\nEdges: %2\nVertices: %3")
                .arg(topologySummaryText(
                    topology->faces))
                .arg(topologySummaryText(
                    topology->edges))
                .arg(topologySummaryText(
                    topology->vertices)));
    } else {
        body_topology_counts_->setText(
            QStringLiteral("—"));
        body_topology_accounting_->setText(
            QStringLiteral("—"));
    }

    properties_stack_->setCurrentWidget(
        body_properties_page_);
}

void CadWorkbench::refreshFeatureProperties(
    part::FeatureId feature_id) {
    if (properties_stack_ == nullptr ||
        document_session_ == nullptr) {
        return;
    }

    const auto* feature =
        document_session_->document()
            .findFeature(feature_id);
    if (feature == nullptr) {
        selected_feature_id_.reset();
        properties_stack_->setCurrentWidget(
            document_properties_page_);
        return;
    }

    const auto* extrude =
        std::get_if<part::ExtrudeFeature>(
            &feature->definition);
    if (extrude == nullptr) {
        return;
    }

    selected_feature_id_ = feature_id;
    feature_name_->setText(
        fromUtf8(feature->name));
    feature_identity_->setText(
        fromUtf8(feature->id.serialized()));
    feature_operation_->setText(
        extrude->operation ==
                part::ExtrudeOperation::cut
            ? QStringLiteral("Cut")
            : QStringLiteral("Add"));

    const auto unit =
        document_session_->document()
            .lengthUnit();
    if (const auto* one =
            std::get_if<
                part::OneSidedExtrudeExtent>(
                &extrude->extent)) {
        feature_extent_->setText(
            QStringLiteral("One Side"));
        feature_distance_->setText(
            formatLengthForPart(
                one->distance,
                unit));
        feature_direction_->setText(
            one->reversed
                ? QStringLiteral("Reverse")
                : QStringLiteral("Forward"));
    } else if (const auto* midplane =
                   std::get_if<
                       part::MidplaneExtrudeExtent>(
                       &extrude->extent)) {
        feature_extent_->setText(
            QStringLiteral("Midplane"));
        feature_distance_->setText(
            formatLengthForPart(
                midplane->total_distance,
                unit));
        feature_direction_->setText(
            QStringLiteral("Centered"));
    }

    feature_source_profile_->setText(
        fromUtf8(
            extrude->profile_id.serialized()));
    const auto* profile =
        document_session_->document()
            .findProfile(
                extrude->profile_id);
    feature_source_sketch_->setText(
        profile != nullptr
            ? fromUtf8(
                  profile->source_sketch_id
                      .value())
            : QStringLiteral("<missing>"));

    auto status =
        feature->suppressed
            ? part::FeatureEvaluationStatus::
                  suppressed
            : part::FeatureEvaluationStatus::
                  blocked;
    auto diagnostic =
        part::FeatureEvaluationDiagnosticCode::
            none;
    if (part_evaluation_revision_ &&
        *part_evaluation_revision_ ==
            document_session_->document()
                .revision()) {
        const auto found =
            std::find_if(
                feature_evaluation_statuses_
                    .begin(),
                feature_evaluation_statuses_
                    .end(),
                [feature_id](
                    const FeatureEvaluationUiState&
                        item) {
                    return item.feature_id ==
                           feature_id;
                });
        if (found !=
            feature_evaluation_statuses_
                .end()) {
            status = found->status;
            diagnostic =
                found->diagnostic;
        }
    }
    feature_status_->setText(
        featureEvaluationStatusText(status));
    feature_diagnostic_->setText(
        featureEvaluationDiagnosticText(
            diagnostic));
    feature_go_to_profile_button_->setEnabled(
        profile != nullptr);
    const bool lifecycle_available =
        !extrude_profile_pick_active_ &&
        !extrude_draft_ &&
        !active_sketch_id_ &&
        !sketch_support_pick_active_;
    feature_edit_button_->setEnabled(
        !feature->suppressed &&
        solid_modeling_kernel_ != nullptr &&
        lifecycle_available);
    feature_suppress_button_->setText(
        feature->suppressed
            ? QStringLiteral("Unsuppress Feature")
            : QStringLiteral("Suppress Feature"));
    feature_suppress_button_->setEnabled(
        lifecycle_available);
    feature_delete_button_->setEnabled(
        lifecycle_available);

    const auto contribution =
        viewport_controller_ != nullptr
            ? viewport_controller_->
                  featureContributionSummary(
                      feature_id)
            : std::nullopt;
    if (contribution) {
        feature_contribution_->setText(
            QStringLiteral(
                "Faces %1; direct Edges %2; direct Vertices %3; "
                "boundary Edges %4; boundary Vertices %5")
                .arg(static_cast<qulonglong>(
                    contribution->faces))
                .arg(static_cast<qulonglong>(
                    contribution->direct_edges))
                .arg(static_cast<qulonglong>(
                    contribution->direct_vertices))
                .arg(static_cast<qulonglong>(
                    contribution->boundary_edges))
                .arg(static_cast<qulonglong>(
                    contribution->boundary_vertices)));
        feature_contribution_diagnostics_->setText(
            QStringLiteral(
                "Missing %1; Ambiguous %2; %3")
                .arg(static_cast<qulonglong>(
                    contribution->missing_outputs))
                .arg(static_cast<qulonglong>(
                    contribution->ambiguous_outputs))
                .arg(stageText(
                    contribution->current_stage)));
    } else {
        feature_contribution_->setText(
            QStringLiteral("—"));
        feature_contribution_diagnostics_->setText(
            QStringLiteral("—"));
    }

    properties_stack_->setCurrentWidget(
        feature_properties_page_);
}

void CadWorkbench::refreshTopologyProperties(
    const BodyTopologyInspection& inspection) {
    if (properties_stack_ == nullptr ||
        topology_properties_page_ == nullptr ||
        !inspection.valid()) {
        return;
    }

    topology_kind_->setText(
        topologyKindText(inspection.kind));
    topology_stage_->setText(
        stageText(inspection.stage));
    topology_presence_->setText(
        inspection.diagnostic_prefix
            ? QStringLiteral(
                  "Diagnostic Prefix — non-authoritative")
            : QStringLiteral("Present"));
    topology_accounting_->setText(
        topologyAccountingText(
            inspection.accounting_class));
    topology_strict_reference_->setText(
        referenceStatusText(
            inspection.strict_referenceability));
    topology_carrier_reference_->setText(
        referenceStatusText(
            inspection.carrier_referenceability));
    topology_candidates_->setText(
        QString::number(
            static_cast<qulonglong>(
                inspection.semantic_candidate_count)));

    QString carrier = QStringLiteral("—");
    QString carrier_type = QStringLiteral("—");
    switch (inspection.kind) {
    case viewer::BodyTopologyPresentationKind::face:
        carrier = inspection.surface_address
            ? surfaceAddressText(
                  *inspection.surface_address)
            : QStringLiteral("Surface");
        carrier_type = inspection.surface_kind
            ? surfaceKindText(
                  *inspection.surface_kind)
            : QStringLiteral("—");
        break;
    case viewer::BodyTopologyPresentationKind::edge:
        carrier = inspection.curve_address
            ? QStringLiteral("Curve — Feature %1")
                  .arg(QString::fromStdString(
                      inspection.curve_address
                          ->producer_feature_id
                          .serialized()))
            : (inspection.periodic_seam
                   ? QStringLiteral(
                         "Provider representation seam")
                   : QStringLiteral("Curve"));
        carrier_type = inspection.curve_kind
            ? curveKindText(
                  *inspection.curve_kind)
            : QStringLiteral("—");
        break;
    case viewer::BodyTopologyPresentationKind::vertex:
        carrier = inspection.point_address
            ? QStringLiteral("Point — Feature %1")
                  .arg(QString::fromStdString(
                      inspection.point_address
                          ->producer_feature_id
                          .serialized()))
            : QStringLiteral("Point");
        carrier_type = QStringLiteral("Point");
        break;
    }
    topology_carrier_->setText(carrier);
    topology_carrier_type_->setText(
        carrier_type);

    QString producer = QStringLiteral("—");
    if (inspection.producer_feature_id) {
        const auto id =
            *inspection.producer_feature_id;
        const auto* feature =
            document_session_ != nullptr
                ? document_session_->document()
                      .findFeature(id)
                : nullptr;
        producer = feature != nullptr
            ? QStringLiteral("%1 (Feature %2)")
                  .arg(
                      fromUtf8(feature->name),
                      fromUtf8(id.serialized()))
            : QStringLiteral("Feature %1")
                  .arg(fromUtf8(id.serialized()));
    }
    topology_producer_->setText(producer);
    topology_adjacency_->setText(
        adjacentSurfaceSummary(
            inspection.adjacent_surfaces));
    topology_sketch_support_->setText(
        sketchSupportInspectionText(
            inspection.sketch_support));
    topology_geometry_->setText(
        inspection.kind ==
                viewer::BodyTopologyPresentationKind::
                    vertex
            ? providerPointText(
                  inspection.provider_point)
            : QStringLiteral("—"));

    properties_stack_->setCurrentWidget(
        topology_properties_page_);
}

void CadWorkbench::navigateToProfile(
    part::ProfileId profile_id) {
    if (document_session_ == nullptr ||
        document_session_->document()
                .findProfile(profile_id) ==
            nullptr) {
        return;
    }

    selected_feature_id_.reset();
    selected_body_id_.reset();
    selected_profile_id_ = profile_id;
    if (document_tree_ != nullptr) {
        const QSignalBlocker blocked{
            document_tree_};
        document_tree_->clearSelection();
    }
    if (tree_controller_) {
        tree_controller_->setProfileSelection(
            {profile_id},
            profile_id);
    }
    if (viewport_controller_) {
        viewport_controller_->
            setProfileSelectionFromTree(
                {profile_id},
                profile_id);
    }
    refreshProfileProperties(
        profile_id);
}

void CadWorkbench::navigateToFeature(
    part::FeatureId feature_id) {
    if (document_session_ == nullptr ||
        document_session_->document()
                .findFeature(feature_id) ==
            nullptr) {
        return;
    }

    selected_profile_id_.reset();
    selected_body_id_.reset();
    selected_feature_id_ = feature_id;
    if (document_tree_ != nullptr) {
        const QSignalBlocker blocked{
            document_tree_};
        document_tree_->clearSelection();
    }
    if (tree_controller_) {
        tree_controller_->setFeatureSelection(
            {feature_id},
            feature_id);
    }
    if (viewport_controller_) {
        viewport_controller_->
            setProfileSelectionFromTree(
                {},
                std::nullopt);
    }
    refreshFeatureProperties(
        feature_id);
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
                structuralBoundarySelectionPending()) {
            if (sketch_interaction_controller_->
                    completeStructuralBoundarySelection()) {
                setStatusText(
                    QStringLiteral(
                        "Structural boundaries accepted."));
            }
            return true;
        }

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

        if (watched == viewport_widget_ &&
            axis_draft_) {
            if (key_event->key() ==
                Qt::Key_Escape) {
                cancelAxis();
                return true;
            }
            if (key_event->key() ==
                    Qt::Key_Return ||
                key_event->key() ==
                    Qt::Key_Enter) {
                static_cast<void>(
                    finishAxis());
                return true;
            }
        }

        if (watched == viewport_widget_ &&
            datum_plane_draft_) {
            if (key_event->key() ==
                Qt::Key_Escape) {
                cancelDatumPlane();
                return true;
            }
            if (key_event->key() ==
                    Qt::Key_Return ||
                key_event->key() ==
                    Qt::Key_Enter) {
                static_cast<void>(
                    finishDatumPlane());
                return true;
            }
        }

        if (watched == viewport_widget_ &&
            extrude_profile_pick_active_ &&
            key_event->key() == Qt::Key_Escape) {
            cancelExtrudeProfilePick();
            return true;
        }

        if (watched == viewport_widget_ &&
            extrude_draft_) {
            if (key_event->key() ==
                Qt::Key_Escape) {
                cancelExtrude();
                return true;
            }
            if (key_event->key() ==
                    Qt::Key_Return ||
                key_event->key() ==
                    Qt::Key_Enter) {
                static_cast<void>(
                    finishExtrude());
                return true;
            }
        }

        const bool profile_delete_context =
            watched == viewport_widget_ &&
            selected_profile_id_ &&
            properties_stack_ != nullptr &&
            properties_stack_->currentWidget() ==
                profile_properties_page_ &&
            (!sketch_interaction_controller_ ||
             !sketch_interaction_controller_->
                  active());
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

            if ((key_event->key() == Qt::Key_Return ||
                 key_event->key() == Qt::Key_Enter) &&
                sketch_interaction_controller_->
                    structuralBoundarySelectionPending()) {
                if (sketch_interaction_controller_->
                        completeStructuralBoundarySelection()) {
                    setStatusText(
                        QStringLiteral(
                            "Structural boundaries accepted."));
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

        object_snap_toggle_button_->setChecked(
            settings.object_snap.master_enabled);
        object_snap_toggle_button_->setText(
            settings.object_snap.master_enabled
                ? QStringLiteral("ON")
                : QStringLiteral("OFF"));
        object_tracking_toggle_button_->setChecked(
            settings.object_snap.
                object_tracking_enabled);
        object_tracking_toggle_button_->setText(
            settings.object_snap.
                    object_tracking_enabled
                ? QStringLiteral("ON")
                : QStringLiteral("OFF"));

        object_snap_endpoint_check_->setChecked(
            settings.object_snap.endpoint);
        object_snap_midpoint_check_->setChecked(
            settings.object_snap.midpoint);
        object_snap_center_check_->setChecked(
            settings.object_snap.center);
        object_snap_quadrant_check_->setChecked(
            settings.object_snap.quadrant);
        object_snap_intersection_check_->setChecked(
            settings.object_snap.intersection);
        object_snap_origin_check_->setChecked(
            settings.object_snap.origin);
        object_snap_perpendicular_check_->setChecked(
            settings.object_snap.perpendicular);
        object_snap_tangent_check_->setChecked(
            settings.object_snap.tangent);
        object_snap_nearest_check_->setChecked(
            settings.object_snap.nearest);
        object_snap_extension_check_->setChecked(
            settings.object_snap.extension);

        const auto point_request =
            sketch_interaction_controller_->
                activePointRequest();
        object_snap_override_combo_->setEnabled(
            point_request.has_value());
        int override_index = 0;
        if (point_request &&
            point_request->temporary_snap_override) {
            const int found =
                object_snap_override_combo_->
                    findData(
                        static_cast<int>(
                            *point_request->
                                temporary_snap_override));
            if (found >= 0) {
                override_index = found;
            }
        }
        object_snap_override_combo_->
            setCurrentIndex(override_index);

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
        if (object_snap_override_combo_ != nullptr) {
            object_snap_override_combo_->setEnabled(false);
            object_snap_override_combo_->setCurrentIndex(0);
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

    if (trim_sketch_button_ != nullptr) {
        trim_sketch_button_->setVisible(editing);
        trim_sketch_button_->setChecked(
            editing &&
            sketch_interaction_controller_->tool() ==
                sketch::SketchTool::trim);
    }

    if (extend_sketch_button_ != nullptr) {
        extend_sketch_button_->setVisible(editing);
        extend_sketch_button_->setChecked(
            editing &&
            sketch_interaction_controller_->tool() ==
                sketch::SketchTool::extend);
    }

    if (extend_both_sketch_button_ != nullptr) {
        extend_both_sketch_button_->setVisible(editing);
        extend_both_sketch_button_->setChecked(
            editing &&
            sketch_interaction_controller_->tool() ==
                sketch::SketchTool::extend_both);
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
        profile_show_islands_button_->setChecked(
            options.show_islands);
        profile_highlight_hover_button_->setChecked(
            options.highlight_on_hover);
        profile_show_boundaries_button_->setChecked(
            options.show_region_boundaries);
        profile_show_problems_button_->setChecked(
            options.show_problems);

        const auto island_count =
            sketch_interaction_controller_->
                profileIslandCount();
        const auto island_text =
            options.show_islands
                ? QString::number(
                      static_cast<qulonglong>(
                          island_count))
                : QStringLiteral("Hidden");

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
                    .arg(island_text)
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
                    .arg(island_text)
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

    const bool structural_tool =
        tool == sketch::SketchTool::trim ||
        tool == sketch::SketchTool::extend ||
        tool == sketch::SketchTool::extend_both;

    if (structural_tool) {
        delete_selection_button_->setVisible(false);
        finish_line_button_->setVisible(true);
        cancel_line_button_->setVisible(true);

        if (tool == sketch::SketchTool::trim) {
            finish_line_button_->setText(
                QStringLiteral("Finish Trim"));
            cancel_line_button_->setText(
                QStringLiteral("Cancel Trim"));
            operations_placeholder_->setText(
                sketch_interaction_controller_->
                        structuralBoundarySelectionPending()
                    ? QStringLiteral(
                          "Trim — Select boundaries (%1 selected); Enter/RMB to continue")
                          .arg(
                              static_cast<qulonglong>(
                                  sketch_interaction_controller_->
                                      selectedCount()))
                    : QStringLiteral(
                          "Trim — %1 finite boundary entities; click target fragment")
                          .arg(
                              static_cast<qulonglong>(
                                  sketch_interaction_controller_->
                                      structuralBoundaries().size())));
        } else if (
            tool == sketch::SketchTool::extend) {
            finish_line_button_->setText(
                QStringLiteral("Finish Extend"));
            cancel_line_button_->setText(
                QStringLiteral("Cancel Extend"));
            operations_placeholder_->setText(
                sketch_interaction_controller_->
                        structuralBoundarySelectionPending()
                    ? QStringLiteral(
                          "Extend — Select boundaries (%1 selected); Enter/RMB to continue")
                          .arg(
                              static_cast<qulonglong>(
                                  sketch_interaction_controller_->
                                      selectedCount()))
                    : QStringLiteral(
                          "Extend — %1 finite boundary entities; click target end")
                          .arg(
                              static_cast<qulonglong>(
                                  sketch_interaction_controller_->
                                      structuralBoundaries().size())));
        } else {
            finish_line_button_->setText(
                QStringLiteral("Finish Extend Both"));
            cancel_line_button_->setText(
                QStringLiteral("Cancel Extend Both"));
            operations_placeholder_->setText(
                sketch_interaction_controller_->
                        extendBothFirstLine()
                    ? QStringLiteral(
                          "Extend Both — Choose second Line")
                    : QStringLiteral(
                          "Extend Both — Choose first Line"));
        }
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
        active &&
        !sketch_support_pick_active_ &&
        !axis_draft_ &&
        !datum_plane_draft_ &&
        !extrude_profile_pick_active_ &&
        !extrude_draft_ &&
        document_session->canUndo());
    redo_button_->setEnabled(
        active &&
        !sketch_support_pick_active_ &&
        !axis_draft_ &&
        !datum_plane_draft_ &&
        !extrude_profile_pick_active_ &&
        !extrude_draft_ &&
        document_session->canRedo());
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
        !sketch_support_pick_active_ &&
        !datum_plane_draft_ &&
        !extrude_profile_pick_active_ &&
        !extrude_draft_);

    if (axis_button_ != nullptr) {
        axis_button_->setVisible(active);
        axis_button_->setEnabled(
            active &&
            !sketch_support_pick_active_ &&
            !datum_plane_draft_ &&
            !extrude_profile_pick_active_ &&
            !extrude_draft_ &&
            solid_modeling_kernel_ != nullptr);
        axis_button_->setChecked(
            axis_draft_.has_value());
    }

    if (datum_plane_button_ != nullptr) {
        datum_plane_button_->setVisible(
            !editing_sketch);
        datum_plane_button_->setEnabled(
            active &&
            !editing_sketch &&
            !sketch_support_pick_active_ &&
            !axis_draft_ &&
            !extrude_profile_pick_active_ &&
            !extrude_draft_ &&
            solid_modeling_kernel_ != nullptr);
        datum_plane_button_->setChecked(
            datum_plane_draft_.has_value());
    }

    if (extrude_button_ != nullptr) {
        extrude_button_->setVisible(
            !editing_sketch);
        extrude_button_->setEnabled(
            active &&
            !editing_sketch &&
            !sketch_support_pick_active_ &&
            !axis_draft_ &&
            !datum_plane_draft_ &&
            !extrude_draft_ &&
            solid_modeling_kernel_ != nullptr);
        extrude_button_->setChecked(
            extrude_profile_pick_active_ ||
            extrude_draft_.has_value());
    }

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
    trim_sketch_button_->setVisible(
        editing_sketch);
    extend_sketch_button_->setVisible(
        editing_sketch);
    extend_both_sketch_button_->setVisible(
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
        editing_sketch ||
        (active &&
         sketch_support_pick_active_ &&
         pending_sketch_support_.has_value()));
    finish_sketch_button_->setEnabled(
        editing_sketch ||
        (active &&
         sketch_support_pick_active_ &&
         pending_sketch_support_.has_value()));
    if (sketch_support_pick_active_) {
        finish_sketch_button_->setText(
            sketch_resupport_target_
                ? QStringLiteral("Apply Support")
                : QStringLiteral("Create Sketch"));
    } else {
        finish_sketch_button_->setText(
            QStringLiteral("Finish Sketch"));
    }

    const bool axis_lifecycle_available =
        active &&
        selected_axis_id_.has_value() &&
        !axis_draft_ &&
        !datum_plane_draft_ &&
        !extrude_profile_pick_active_ &&
        !extrude_draft_ &&
        !active_sketch_id_ &&
        !sketch_support_pick_active_;
    if (axis_edit_button_ != nullptr) {
        axis_edit_button_->setEnabled(
            axis_lifecycle_available &&
            solid_modeling_kernel_ != nullptr);
    }
    if (axis_delete_button_ != nullptr) {
        axis_delete_button_->setEnabled(
            axis_lifecycle_available);
    }

    const bool datum_lifecycle_available =
        active &&
        selected_datum_id_.has_value() &&
        !axis_draft_ &&
        !datum_plane_draft_ &&
        !extrude_profile_pick_active_ &&
        !extrude_draft_ &&
        !active_sketch_id_ &&
        !sketch_support_pick_active_;
    if (datum_edit_button_ != nullptr) {
        datum_edit_button_->setEnabled(
            datum_lifecycle_available &&
            solid_modeling_kernel_ != nullptr);
    }
    if (datum_delete_button_ != nullptr) {
        datum_delete_button_->setEnabled(
            datum_lifecycle_available);
    }

    syncSketchInteractionUi();
    syncAxisUi();
    syncDatumPlaneUi();
    syncExtrudeUi();
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
