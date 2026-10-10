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
#include <iterator>
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
    case part::FeatureEvaluationDiagnosticCode::profile_unavailable:
        return QStringLiteral("Profile unavailable");
    case part::FeatureEvaluationDiagnosticCode::unresolved_profile:
        return QStringLiteral("Unresolved Profile");
    case part::FeatureEvaluationDiagnosticCode::missing_axis:
        return QStringLiteral("Missing Axis");
    case part::FeatureEvaluationDiagnosticCode::axis_unavailable:
        return QStringLiteral("Axis unavailable");
    case part::FeatureEvaluationDiagnosticCode::axis_not_in_profile_plane:
        return QStringLiteral("Axis not in Profile plane");
    case part::FeatureEvaluationDiagnosticCode::profile_crosses_axis:
        return QStringLiteral("Profile crosses Axis");
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
    case part::FeatureEvaluationDiagnosticCode::edge_reference_missing:
        return QStringLiteral("Edge reference Missing");
    case part::FeatureEvaluationDiagnosticCode::edge_reference_ambiguous:
        return QStringLiteral("Edge reference Ambiguous");
    case part::FeatureEvaluationDiagnosticCode::edge_reference_unsupported:
        return QStringLiteral("Edge reference Unsupported");
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

QString formatAngleForCad(
    core::AngleValue value) {
    return QStringLiteral("%1 deg")
        .arg(
            QString::number(
                value.radians * 180.0 /
                    std::numbers::pi_v<double>,
                'g',
                12));
}


QString revolveAxisText(
    const part::AxisReference& axis) {
    if (const auto role =
            part::builtinOriginAxisForAxisReference(
                axis)) {
        switch (*role) {
        case core::BuiltinReferenceRole::x_axis:
            return QStringLiteral("Origin X Axis");
        case core::BuiltinReferenceRole::y_axis:
            return QStringLiteral("Origin Y Axis");
        case core::BuiltinReferenceRole::z_axis:
            return QStringLiteral("Origin Z Axis");
        default:
            break;
        }
    }
    if (const auto id =
            part::authoredAxisIdForAxisReference(
                axis)) {
        return QStringLiteral("Axis %1")
            .arg(QString::fromStdString(
                id->serialized()));
    }
    return QStringLiteral("<invalid Axis>");
}

QString revolveEvaluationText(
    const application::RevolveDraftEvaluationResult&
        evaluation) {
    using Status =
        application::RevolveDraftEvaluationStatus;
    switch (evaluation.status) {
    case Status::ok:
        return evaluation.previewSolidAvailable()
            ? QStringLiteral("Preview ready.")
            : QStringLiteral(
                  "Candidate valid; preview unavailable.");
    case Status::incomplete_draft:
        return QStringLiteral(
            "Select a Profile and an Origin/Authored Axis.");
    case Status::stale_document:
    case Status::stale_revision:
        return QStringLiteral(
            "Draft is stale; restart Revolve.");
    case Status::missing_profile:
        return QStringLiteral("Source Profile is missing.");
    case Status::missing_feature:
        return QStringLiteral("Edited Feature is missing.");
    case Status::suppressed_feature:
        return QStringLiteral(
            "Suppressed Feature cannot be edited.");
    case Status::feature_id_exhausted:
        return QStringLiteral(
            "Feature identity space is exhausted.");
    case Status::invalid_candidate:
        return QStringLiteral(
            "Revolve candidate is invalid.");
    case Status::target_failed:
        return evaluation.evaluation_diagnostic
            ? QStringLiteral("Revolve rejected: %1")
                  .arg(
                      featureEvaluationDiagnosticText(
                          *evaluation.evaluation_diagnostic))
            : QStringLiteral("Revolve evaluation failed.");
    }
    return QStringLiteral("Revolve unavailable.");
}


QString edgeFeatureEvaluationText(
    const application::EdgeFeatureDraftEvaluationResult&
        evaluation,
    QStringView feature_name) {
    using Status =
        application::EdgeFeatureDraftEvaluationStatus;
    switch (evaluation.status) {
    case Status::ok:
        return evaluation.previewSolidAvailable()
            ? QStringLiteral("Preview ready.")
            : QStringLiteral(
                  "Candidate valid; preview unavailable.");
    case Status::incomplete_draft:
        return QStringLiteral(
            "Select one or more material Edges and enter a positive parameter.");
    case Status::stale_document:
    case Status::stale_revision:
        return QStringLiteral(
            "%1 draft is stale; cancel and restart.")
            .arg(feature_name);
    case Status::feature_id_exhausted:
        return QStringLiteral(
            "Feature identity space is exhausted.");
    case Status::missing_feature:
        return QStringLiteral(
            "Edited Feature is missing.");
    case Status::suppressed_feature:
        return QStringLiteral(
            "Suppressed Feature cannot be edited.");
    case Status::invalid_candidate:
        return QStringLiteral(
            "%1 candidate is invalid.")
            .arg(feature_name);
    case Status::target_failed: {
        QString detail =
            evaluation.evaluation_diagnostic
                ? featureEvaluationDiagnosticText(
                      *evaluation.evaluation_diagnostic)
                : QStringLiteral(
                      "modeling evaluation failed");
        if (evaluation.failing_edge_input_index) {
            detail += QStringLiteral(
                          " · Edge input %1")
                          .arg(
                              static_cast<qulonglong>(
                                  *evaluation
                                       .failing_edge_input_index +
                                  1U));
        }
        if (evaluation.target_status) {
            detail += QStringLiteral(" · %1")
                          .arg(
                              featureEvaluationStatusText(
                                  *evaluation.target_status));
        }
        return QStringLiteral("%1 rejected: %2")
            .arg(feature_name, detail);
    }
    }
    return QStringLiteral("%1 unavailable.")
        .arg(feature_name);
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
    tree_controller_->setProfileEvaluationProvider(
        [this](part::ProfileId profile_id) {
            return evaluateCurrentProfile(profile_id);
        });
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

    part_create_tools_label_ =
        new QLabel(
            QStringLiteral("Create:"),
            shell_);
    part_create_tools_label_->setObjectName(
        QStringLiteral("partCreateToolsLabel"));
    shell_->editorToolsLayout().insertWidget(
        0,
        part_create_tools_label_);

    sketch_button_ =
        new QPushButton(
            QStringLiteral("Sketch"),
            shell_);
    sketch_button_->setObjectName(
        QStringLiteral("sketchToolButton"));
    shell_->editorToolsLayout().insertWidget(
        1,
        sketch_button_);

    datum_plane_button_ =
        new QPushButton(
            QStringLiteral("Datum Plane"),
            shell_);
    datum_plane_button_->setObjectName(
        QStringLiteral("datumPlaneToolButton"));
    datum_plane_button_->setCheckable(true);
    shell_->editorToolsLayout().insertWidget(
        2,
        datum_plane_button_);

    extrude_button_ =
        new QPushButton(
            QStringLiteral("Extrude"),
            shell_);
    extrude_button_->setObjectName(
        QStringLiteral("extrudeToolButton"));
    extrude_button_->setCheckable(true);
    shell_->editorToolsLayout().insertWidget(
        3,
        extrude_button_);

    revolve_button_ =
        new QPushButton(
            QStringLiteral("Revolve"),
            shell_);
    revolve_button_->setObjectName(
        QStringLiteral("revolveToolButton"));
    revolve_button_->setCheckable(true);
    shell_->editorToolsLayout().insertWidget(
        4,
        revolve_button_);

    part_modify_tools_label_ =
        new QLabel(
            QStringLiteral("Modify:"),
            shell_);
    part_modify_tools_label_->setObjectName(
        QStringLiteral("partModifyToolsLabel"));
    shell_->editorToolsLayout().insertWidget(
        5,
        part_modify_tools_label_);

    fillet_button_ =
        new QPushButton(
            QStringLiteral("Fillet"),
            shell_);
    fillet_button_->setObjectName(
        QStringLiteral("filletToolButton"));
    fillet_button_->setCheckable(true);
    shell_->editorToolsLayout().insertWidget(
        6,
        fillet_button_);

    chamfer_button_ =
        new QPushButton(
            QStringLiteral("Chamfer"),
            shell_);
    chamfer_button_->setObjectName(
        QStringLiteral("chamferToolButton"));
    chamfer_button_->setCheckable(true);
    shell_->editorToolsLayout().insertWidget(
        7,
        chamfer_button_);

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

    project_edge_button_ =
        new QPushButton(
            QStringLiteral("Project Geometry"),
            shell_);
    project_edge_button_->setObjectName(
        QStringLiteral("projectEdgeToolButton"));
    project_edge_button_->setCheckable(true);
    // Same Sketch Modify group as Trim/Extend, not a Part-level dialog.
    shell_->editorToolsLayout().insertWidget(
        10, project_edge_button_);

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
            tryStageRevolveAxisFromBuiltin(primary);
        });

    viewport_controller_->setBodyTopologySelectionChangedHandler(
        [this](std::optional<BodyTopologyInspection> inspection) {
            if (!inspection) {
                if (fillet_draft_ ||
                    chamfer_draft_) {
                    tryStageEdgeFeatureSelection();
                }
                if (project_edge_active_) {
                    tryStageProjectEdgeSelection();
                }
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
            if (fillet_draft_ ||
                chamfer_draft_) {
                tryStageEdgeFeatureSelection();
            }
            if (project_edge_active_) {
                tryStageProjectEdgeSelection();
            }
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
    sketch_interaction_controller_->
        setSolidModelingKernel(
            solid_modeling_kernel_);
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
                tryStageRevolveProfile(semantic);
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
            tryStageRevolveAxisFromAuthored(semantic);
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
                tryStageRevolveProfile(semantic);
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
            if (document_session_ != nullptr) {
                const auto* feature =
                    document_session_->document()
                        .findFeature(feature_id);
                if (feature == nullptr) {
                    return;
                }
                if (std::holds_alternative<
                        part::RevolveFeature>(
                        feature->definition)) {
                    static_cast<void>(
                        startRevolveEdit(feature_id));
                } else if (std::holds_alternative<
                               part::ExtrudeFeature>(
                               feature->definition)) {
                    static_cast<void>(
                        startExtrudeEdit(feature_id));
                } else if (
                    std::holds_alternative<
                        part::FilletFeature>(
                        feature->definition) ||
                    std::holds_alternative<
                        part::ChamferFeature>(
                        feature->definition)) {
                    static_cast<void>(
                        startEdgeFeatureEdit(feature_id));
                }
            }
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
        QStringLiteral("Distance / Angle"),
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

    project_link_status_label_ =
        new QLabel(operations_content);
    project_link_status_label_->setObjectName(
        QStringLiteral("projectLinkedEdgeStatusLabel"));
    project_link_status_label_->setWordWrap(true);
    project_link_status_label_->setVisible(false);
    operations_layout->addWidget(project_link_status_label_);

    project_link_break_button_ =
        new QPushButton(
            QStringLiteral("Break Link"),
            operations_content);
    project_link_break_button_->setObjectName(
        QStringLiteral("projectBreakLinkButton"));
    project_link_break_button_->setToolTip(
        QStringLiteral(
            "Detach all selected linked Edges atomically using their current resolved source curves; keep their current geometry (one Undo)."));
    project_link_break_button_->setVisible(false);
    operations_layout->addWidget(project_link_break_button_);

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

    line_part_reference_label_ =
        new QLabel(
            QStringLiteral("Part reference:"),
            operations_content);
    line_part_reference_label_->setObjectName(
        QStringLiteral("sketchPartReferenceLabel"));
    line_part_reference_label_->setVisible(false);
    operations_layout->addWidget(
        line_part_reference_label_);

    line_axis_designation_check_ =
        new QCheckBox(
            QStringLiteral("Axis"),
            operations_content);
    line_axis_designation_check_->setObjectName(
        QStringLiteral("sketchAxisDesignationCheck"));
    line_axis_designation_check_->setVisible(false);
    line_axis_designation_check_->setToolTip(
        QStringLiteral(
            "Create or remove the Part-owned Axis designation for this Line."));
    operations_layout->addWidget(
        line_axis_designation_check_);

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

    revolve_operations_widget_ =
        new QWidget(operations_content);
    revolve_operations_widget_->setObjectName(
        QStringLiteral("revolveOperationsWidget"));
    auto* revolve_operations_layout =
        new QVBoxLayout(
            revolve_operations_widget_);
    revolve_operations_layout->setContentsMargins(
        0, 0, 0, 0);

    auto* revolve_sources_form =
        new QFormLayout;
    revolve_profile_label_ =
        new QLabel(
            QStringLiteral("<select Profile>"),
            revolve_operations_widget_);
    revolve_profile_label_->setObjectName(
        QStringLiteral("revolveProfileLabel"));
    revolve_axis_label_ =
        new QLabel(
            QStringLiteral("<select Axis>"),
            revolve_operations_widget_);
    revolve_axis_label_->setObjectName(
        QStringLiteral("revolveAxisLabel"));
    revolve_sources_form->addRow(
        QStringLiteral("Profile"),
        revolve_profile_label_);
    revolve_sources_form->addRow(
        QStringLiteral("Axis"),
        revolve_axis_label_);
    revolve_operations_layout->addLayout(
        revolve_sources_form);

    auto* revolve_operation_row =
        new QWidget(
            revolve_operations_widget_);
    auto* revolve_operation_layout =
        new QHBoxLayout(
            revolve_operation_row);
    revolve_operation_layout->setContentsMargins(
        0, 0, 0, 0);
    revolve_add_button_ =
        new QPushButton(
            QStringLiteral("Add"),
            revolve_operation_row);
    revolve_add_button_->setObjectName(
        QStringLiteral("revolveAddButton"));
    revolve_add_button_->setCheckable(true);
    revolve_cut_button_ =
        new QPushButton(
            QStringLiteral("Cut"),
            revolve_operation_row);
    revolve_cut_button_->setObjectName(
        QStringLiteral("revolveCutButton"));
    revolve_cut_button_->setCheckable(true);
    revolve_operation_layout->addWidget(
        revolve_add_button_);
    revolve_operation_layout->addWidget(
        revolve_cut_button_);
    revolve_operations_layout->addWidget(
        revolve_operation_row);

    auto* revolve_extent_row =
        new QWidget(
            revolve_operations_widget_);
    auto* revolve_extent_layout =
        new QHBoxLayout(
            revolve_extent_row);
    revolve_extent_layout->setContentsMargins(
        0, 0, 0, 0);
    revolve_one_side_button_ =
        new QPushButton(
            QStringLiteral("One Side"),
            revolve_extent_row);
    revolve_one_side_button_->setObjectName(
        QStringLiteral("revolveOneSideButton"));
    revolve_one_side_button_->setCheckable(true);
    revolve_midplane_button_ =
        new QPushButton(
            QStringLiteral("Midplane"),
            revolve_extent_row);
    revolve_midplane_button_->setObjectName(
        QStringLiteral("revolveMidplaneButton"));
    revolve_midplane_button_->setCheckable(true);
    revolve_extent_layout->addWidget(
        revolve_one_side_button_);
    revolve_extent_layout->addWidget(
        revolve_midplane_button_);
    revolve_operations_layout->addWidget(
        revolve_extent_row);

    revolve_reverse_button_ =
        new QPushButton(
            QStringLiteral("Reverse"),
            revolve_operations_widget_);
    revolve_reverse_button_->setObjectName(
        QStringLiteral("revolveReverseButton"));
    revolve_reverse_button_->setCheckable(true);
    revolve_operations_layout->addWidget(
        revolve_reverse_button_);

    auto* revolve_angle_form =
        new QFormLayout;
    revolve_angle_edit_ =
        new QLineEdit(
            revolve_operations_widget_);
    revolve_angle_edit_->setObjectName(
        QStringLiteral("revolveAngleEdit"));
    revolve_angle_edit_->setPlaceholderText(
        QStringLiteral("0 < angle <= 360 deg"));
    revolve_angle_form->addRow(
        QStringLiteral("Angle"),
        revolve_angle_edit_);
    revolve_operations_layout->addLayout(
        revolve_angle_form);

    revolve_preview_timer_ =
        new QTimer(this);
    revolve_preview_timer_->setSingleShot(true);
    revolve_preview_timer_->setInterval(90);
    QObject::connect(
        revolve_preview_timer_,
        &QTimer::timeout,
        this,
        [this] {
            refreshRevolvePreview();
        });

    revolve_result_label_ =
        new QLabel(
            QStringLiteral(
                "Select a Profile and an Origin/Authored Axis."),
            revolve_operations_widget_);
    revolve_result_label_->setObjectName(
        QStringLiteral("revolveResultLabel"));
    revolve_result_label_->setWordWrap(true);
    revolve_operations_layout->addWidget(
        revolve_result_label_);

    revolve_finish_button_ =
        new QPushButton(
            QStringLiteral("Finish Revolve"),
            revolve_operations_widget_);
    revolve_finish_button_->setObjectName(
        QStringLiteral("revolveFinishButton"));
    revolve_operations_layout->addWidget(
        revolve_finish_button_);

    revolve_cancel_button_ =
        new QPushButton(
            QStringLiteral("Cancel"),
            revolve_operations_widget_);
    revolve_cancel_button_->setObjectName(
        QStringLiteral("revolveCancelButton"));
    revolve_operations_layout->addWidget(
        revolve_cancel_button_);

    revolve_operations_widget_->setVisible(false);
    operations_layout->addWidget(
        revolve_operations_widget_);

    edge_feature_operations_widget_ =
        new QWidget(operations_content);
    edge_feature_operations_widget_->setObjectName(
        QStringLiteral("edgeFeatureOperationsWidget"));
    auto* edge_feature_operations_layout =
        new QVBoxLayout(
            edge_feature_operations_widget_);
    edge_feature_operations_layout->setContentsMargins(
        0, 0, 0, 0);

    edge_feature_title_label_ =
        new QLabel(
            QStringLiteral("FILLET"),
            edge_feature_operations_widget_);
    edge_feature_title_label_->setObjectName(
        QStringLiteral("edgeFeatureTitleLabel"));
    edge_feature_operations_layout->addWidget(
        edge_feature_title_label_);

    auto* edge_feature_selection_row =
        new QWidget(edge_feature_operations_widget_);
    auto* edge_feature_selection_layout =
        new QHBoxLayout(edge_feature_selection_row);
    edge_feature_selection_layout->setContentsMargins(
        0, 0, 0, 0);
    edge_feature_selection_label_ =
        new QLabel(
            QStringLiteral("Selected edges: 0"),
            edge_feature_selection_row);
    edge_feature_selection_label_->setObjectName(
        QStringLiteral("edgeFeatureSelectionLabel"));
    edge_feature_clear_button_ =
        new QPushButton(
            QStringLiteral("Clear"),
            edge_feature_selection_row);
    edge_feature_clear_button_->setObjectName(
        QStringLiteral("edgeFeatureClearButton"));
    edge_feature_selection_layout->addWidget(
        edge_feature_selection_label_);
    edge_feature_selection_layout->addStretch();
    edge_feature_selection_layout->addWidget(
        edge_feature_clear_button_);
    edge_feature_operations_layout->addWidget(
        edge_feature_selection_row);

    auto* edge_feature_parameter_form =
        new QFormLayout;
    edge_feature_parameter_name_label_ =
        new QLabel(
            QStringLiteral("Radius"),
            edge_feature_operations_widget_);
    edge_feature_parameter_edit_ =
        new QLineEdit(
            edge_feature_operations_widget_);
    edge_feature_parameter_edit_->setObjectName(
        QStringLiteral("edgeFeatureParameterEdit"));
    edge_feature_parameter_edit_->setPlaceholderText(
        QStringLiteral("positive length"));
    edge_feature_parameter_form->addRow(
        edge_feature_parameter_name_label_,
        edge_feature_parameter_edit_);
    edge_feature_operations_layout->addLayout(
        edge_feature_parameter_form);

    edge_feature_preview_timer_ =
        new QTimer(this);
    edge_feature_preview_timer_->setSingleShot(true);
    edge_feature_preview_timer_->setInterval(90);
    QObject::connect(
        edge_feature_preview_timer_,
        &QTimer::timeout,
        this,
        [this] {
            refreshEdgeFeaturePreview();
        });

    edge_feature_result_label_ =
        new QLabel(
            QStringLiteral(
                "Select one or more material Edges and enter a positive parameter."),
            edge_feature_operations_widget_);
    edge_feature_result_label_->setObjectName(
        QStringLiteral("edgeFeatureResultLabel"));
    edge_feature_result_label_->setWordWrap(true);
    edge_feature_operations_layout->addWidget(
        edge_feature_result_label_);

    edge_feature_finish_button_ =
        new QPushButton(
            QStringLiteral("Finish"),
            edge_feature_operations_widget_);
    edge_feature_finish_button_->setObjectName(
        QStringLiteral("edgeFeatureFinishButton"));
    edge_feature_operations_layout->addWidget(
        edge_feature_finish_button_);

    edge_feature_cancel_button_ =
        new QPushButton(
            QStringLiteral("Cancel"),
            edge_feature_operations_widget_);
    edge_feature_cancel_button_->setObjectName(
        QStringLiteral("edgeFeatureCancelButton"));
    edge_feature_operations_layout->addWidget(
        edge_feature_cancel_button_);

    edge_feature_operations_widget_->setVisible(false);
    operations_layout->addWidget(
        edge_feature_operations_widget_);

    project_edge_operations_widget_ =
        new QWidget(operations_content);
    project_edge_operations_widget_->setObjectName(
        QStringLiteral("projectEdgeOperationsWidget"));
    auto* project_layout =
        new QVBoxLayout(project_edge_operations_widget_);
    project_layout->setContentsMargins(0, 0, 0, 0);

    auto* project_title =
        new QLabel(QStringLiteral("PROJECT GEOMETRY"),
                   project_edge_operations_widget_);
    project_title->setObjectName(
        QStringLiteral("projectEdgeTitleLabel"));
    project_layout->addWidget(project_title);

    project_edge_stage_label_ =
        new QLabel(project_edge_operations_widget_);
    project_edge_stage_label_->setObjectName(
        QStringLiteral("projectEdgeStageLabel"));
    project_edge_stage_label_->setWordWrap(true);
    project_layout->addWidget(project_edge_stage_label_);

    auto* source_row = new QWidget(project_edge_operations_widget_);
    auto* source_layout = new QHBoxLayout(source_row);
    source_layout->setContentsMargins(0, 0, 0, 0);
    project_edge_edges_button_ =
        new QPushButton(QStringLiteral("Edges"), source_row);
    project_edge_edges_button_->setObjectName(
        QStringLiteral("projectEdgeSourceEdgesButton"));
    project_edge_edges_button_->setCheckable(true);
    project_edge_face_button_ =
        new QPushButton(QStringLiteral("Planar Face"), source_row);
    project_edge_face_button_->setObjectName(
        QStringLiteral("projectEdgeSourceFaceButton"));
    project_edge_face_button_->setCheckable(true);
    source_layout->addWidget(project_edge_edges_button_);
    source_layout->addWidget(project_edge_face_button_);
    project_edge_boundary_button_ =
        new QPushButton(QStringLiteral("Face Boundary"), source_row);
    project_edge_boundary_button_->setObjectName(
        QStringLiteral("projectEdgeSourceBoundaryButton"));
    project_edge_boundary_button_->setCheckable(true);
    source_layout->addWidget(project_edge_boundary_button_);
    project_layout->addWidget(source_row);

    project_edge_selection_label_ =
        new QLabel(project_edge_operations_widget_);
    project_edge_selection_label_->setObjectName(
        QStringLiteral("projectEdgeSelectionLabel"));
    project_layout->addWidget(project_edge_selection_label_);

    auto* role_row = new QWidget(project_edge_operations_widget_);
    auto* role_layout = new QHBoxLayout(role_row);
    role_layout->setContentsMargins(0, 0, 0, 0);
    project_edge_regular_button_ =
        new QPushButton(QStringLiteral("Regular"), role_row);
    project_edge_regular_button_->setObjectName(
        QStringLiteral("projectEdgeRegularButton"));
    project_edge_regular_button_->setCheckable(true);
    project_edge_construction_button_ =
        new QPushButton(QStringLiteral("Construction"), role_row);
    project_edge_construction_button_->setObjectName(
        QStringLiteral("projectEdgeConstructionButton"));
    project_edge_construction_button_->setCheckable(true);
    role_layout->addWidget(project_edge_regular_button_);
    role_layout->addWidget(project_edge_construction_button_);
    project_layout->addWidget(role_row);

    auto* pick_row = new QWidget(project_edge_operations_widget_);
    auto* pick_layout = new QHBoxLayout(pick_row);
    pick_layout->setContentsMargins(0, 0, 0, 0);
    project_edge_remove_button_ =
        new QPushButton(QStringLiteral("Remove"), pick_row);
    project_edge_remove_button_->setObjectName(
        QStringLiteral("projectEdgeRemoveButton"));
    project_edge_clear_button_ =
        new QPushButton(QStringLiteral("Clear"), pick_row);
    project_edge_clear_button_->setObjectName(
        QStringLiteral("projectEdgeClearButton"));
    pick_layout->addWidget(project_edge_remove_button_);
    pick_layout->addWidget(project_edge_clear_button_);
    project_layout->addWidget(pick_row);

    project_edge_result_label_ =
        new QLabel(project_edge_operations_widget_);
    project_edge_result_label_->setObjectName(
        QStringLiteral("projectEdgeResultLabel"));
    project_edge_result_label_->setWordWrap(true);
    project_layout->addWidget(project_edge_result_label_);

    project_edge_finish_button_ =
        new QPushButton(QStringLiteral("Finish Project Geometry"),
                        project_edge_operations_widget_);
    project_edge_finish_button_->setObjectName(
        QStringLiteral("projectEdgeFinishButton"));
    project_layout->addWidget(project_edge_finish_button_);

    project_edge_cancel_button_ =
        new QPushButton(QStringLiteral("Cancel"),
                        project_edge_operations_widget_);
    project_edge_cancel_button_->setObjectName(
        QStringLiteral("projectEdgeCancelButton"));
    project_layout->addWidget(project_edge_cancel_button_);
    project_edge_operations_widget_->setVisible(false);
    operations_layout->addWidget(project_edge_operations_widget_);

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
            if (selected_feature_id_ &&
                document_session_ != nullptr) {
                const auto feature_id =
                    *selected_feature_id_;
                const auto* feature =
                    document_session_->document()
                        .findFeature(feature_id);
                if (feature == nullptr) {
                    return;
                }
                if (std::holds_alternative<
                        part::RevolveFeature>(
                        feature->definition)) {
                    static_cast<void>(
                        startRevolveEdit(feature_id));
                } else if (std::holds_alternative<
                               part::ExtrudeFeature>(
                               feature->definition)) {
                    static_cast<void>(
                        startExtrudeEdit(feature_id));
                } else if (
                    std::holds_alternative<
                        part::FilletFeature>(
                        feature->definition) ||
                    std::holds_alternative<
                        part::ChamferFeature>(
                        feature->definition)) {
                    static_cast<void>(
                        startEdgeFeatureEdit(feature_id));
                }
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
            if (sketch_interaction_controller_ &&
                sketch_interaction_controller_->tool() ==
                    sketch::SketchTool::line) {
                static_cast<void>(
                    sketch_interaction_controller_->
                        setCreationRole(
                            sketch::EntityRole::regular));
                return;
            }
            setSketchSelectionRole(
                sketch::EntityRole::regular);
        });
    QObject::connect(
        construction_role_button_,
        &QPushButton::clicked,
        this,
        [this] {
            if (sketch_interaction_controller_ &&
                sketch_interaction_controller_->tool() ==
                    sketch::SketchTool::line) {
                static_cast<void>(
                    sketch_interaction_controller_->
                        setCreationRole(
                            sketch::EntityRole::construction));
                return;
            }
            setSketchSelectionRole(
                sketch::EntityRole::construction);
        });
    QObject::connect(
        line_axis_designation_check_,
        &QCheckBox::clicked,
        this,
        [this](bool checked) {
            setSketchLineAxisDesignation(
                checked);
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
        revolve_button_,
        &QPushButton::clicked,
        this,
        [this] {
            if (revolve_draft_) {
                cancelRevolve();
                return;
            }
            static_cast<void>(
                startRevolveTool());
        });
    QObject::connect(
        revolve_add_button_,
        &QPushButton::clicked,
        this,
        [this] {
            if (revolve_draft_ &&
                revolve_draft_->setOperation(
                    part::RevolveOperation::add)) {
                refreshRevolvePreview();
                notifyCadInputContextChanged();
            }
        });
    QObject::connect(
        revolve_cut_button_,
        &QPushButton::clicked,
        this,
        [this] {
            if (!revolve_draft_ ||
                document_session_ == nullptr) {
                return;
            }
            const auto& features =
                document_session_->document()
                    .body().features;
            bool cut_allowed = !features.empty();
            if (revolve_draft_->mode() ==
                    application::RevolveDraftMode::edit &&
                revolve_draft_->featureId()) {
                const auto found =
                    std::find_if(
                        features.begin(),
                        features.end(),
                        [this](const part::PartFeature& feature) {
                            return feature.id ==
                                *revolve_draft_->featureId();
                        });
                cut_allowed =
                    found != features.end() &&
                    found != features.begin();
            }
            if (!cut_allowed) {
                setStatusText(
                    QStringLiteral(
                        "The first solid-producing Revolve must be Add."));
                syncRevolveUi();
                return;
            }
            if (revolve_draft_->setOperation(
                    part::RevolveOperation::cut)) {
                refreshRevolvePreview();
                notifyCadInputContextChanged();
            }
        });
    QObject::connect(
        revolve_one_side_button_,
        &QPushButton::clicked,
        this,
        [this] {
            if (revolve_draft_ &&
                revolve_draft_->setExtentMode(
                    application::RevolveDraftExtentMode::
                        one_side)) {
                refreshRevolvePreview();
                notifyCadInputContextChanged();
            }
        });
    QObject::connect(
        revolve_midplane_button_,
        &QPushButton::clicked,
        this,
        [this] {
            if (revolve_draft_ &&
                revolve_draft_->setExtentMode(
                    application::RevolveDraftExtentMode::
                        midplane)) {
                refreshRevolvePreview();
                notifyCadInputContextChanged();
            }
        });
    QObject::connect(
        revolve_reverse_button_,
        &QPushButton::clicked,
        this,
        [this](bool checked) {
            if (!revolve_draft_) return;
            if (!revolve_draft_->setReversed(
                    checked)) {
                syncRevolveUi();
                return;
            }
            refreshRevolvePreview();
            notifyCadInputContextChanged();
        });
    QObject::connect(
        revolve_angle_edit_,
        &QLineEdit::textChanged,
        this,
        [this](const QString& text_value) {
            if (syncing_revolve_ui_ ||
                !revolve_draft_ ||
                document_session_ == nullptr) {
                return;
            }
            const auto parsed =
                application::parseCadQuantity(
                    toUtf8(text_value),
                    {
                        application::
                            CadQuantityDimension::angle,
                        document_session_->document()
                            .lengthUnit()});
            constexpr double full_turn =
                2.0 * std::numbers::pi_v<double>;
            if (!parsed ||
                !(parsed->canonical_value > 0.0) ||
                parsed->canonical_value > full_turn) {
                revolve_angle_input_valid_ = false;
                revolve_evaluation_.reset();
                if (revolve_preview_timer_ != nullptr) {
                    revolve_preview_timer_->stop();
                }
                if (viewport_controller_) {
                    viewport_controller_->clearSolidPreview();
                    if (revolve_draft_->mode() ==
                            application::RevolveDraftMode::edit &&
                        revolve_draft_->profileId()) {
                        viewport_controller_->
                            setTransientProfilePresentationOverride(
                                *revolve_draft_->profileId(),
                                std::nullopt);
                    } else {
                        viewport_controller_->
                            setTransientProfilePresentationOverride(
                                std::nullopt,
                                std::nullopt);
                    }
                    viewport_controller_->
                        setTransientAxisEmphasis(
                            revolve_draft_->axis());
                }
                syncRevolveUi();
                return;
            }
            static_cast<void>(
                setRevolveAngle(
                    core::AngleValue{
                        parsed->canonical_value},
                    toUtf8(text_value),
                    false));
        });
    QObject::connect(
        revolve_angle_edit_,
        &QLineEdit::returnPressed,
        this,
        [this] {
            flushRevolvePreview();
            static_cast<void>(
                finishRevolve());
        });
    QObject::connect(
        revolve_finish_button_,
        &QPushButton::clicked,
        this,
        [this] {
            static_cast<void>(
                finishRevolve());
        });
    QObject::connect(
        revolve_cancel_button_,
        &QPushButton::clicked,
        this,
        [this] {
            cancelRevolve();
        });

    QObject::connect(
        fillet_button_,
        &QPushButton::clicked,
        this,
        [this] {
            if (fillet_draft_ || chamfer_draft_) {
                cancelEdgeFeature();
                return;
            }
            static_cast<void>(
                startFilletTool());
        });
    QObject::connect(
        chamfer_button_,
        &QPushButton::clicked,
        this,
        [this] {
            if (fillet_draft_ || chamfer_draft_) {
                cancelEdgeFeature();
                return;
            }
            static_cast<void>(
                startChamferTool());
        });
    QObject::connect(
        edge_feature_clear_button_,
        &QPushButton::clicked,
        this,
        [this] {
            clearEdgeFeatureSelection();
        });
    QObject::connect(
        edge_feature_parameter_edit_,
        &QLineEdit::textChanged,
        this,
        [this](const QString& text_value) {
            if (syncing_edge_feature_ui_ ||
                (!fillet_draft_ &&
                 !chamfer_draft_) ||
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
                edge_feature_parameter_input_valid_ =
                    false;
                edge_feature_evaluation_.reset();
                if (edge_feature_preview_timer_ != nullptr) {
                    edge_feature_preview_timer_->stop();
                }
                if (viewport_controller_ != nullptr) {
                    viewport_controller_->clearSolidPreview();
                }
                syncEdgeFeatureUi();
                return;
            }

            static_cast<void>(
                setEdgeFeatureParameter(
                    core::LengthValue{*parsed},
                    toUtf8(text_value),
                    false));
        });
    QObject::connect(
        edge_feature_parameter_edit_,
        &QLineEdit::returnPressed,
        this,
        [this] {
            flushEdgeFeaturePreview();
            static_cast<void>(
                finishEdgeFeature());
        });
    QObject::connect(
        edge_feature_finish_button_,
        &QPushButton::clicked,
        this,
        [this] {
            static_cast<void>(
                finishEdgeFeature());
        });
    QObject::connect(
        edge_feature_cancel_button_,
        &QPushButton::clicked,
        this,
        [this] {
            cancelEdgeFeature();
        });

    QObject::connect(
        project_link_break_button_, &QPushButton::clicked,
        this, [this] { breakSelectedProjectedEdgeLink(); });
    QObject::connect(
        project_edge_button_, &QPushButton::clicked,
        this, [this] {
            if (project_edge_active_) {
                cancelProjectEdgeTool();
            } else {
                static_cast<void>(startProjectEdgeTool());
            }
        });
    QObject::connect(
        project_edge_edges_button_, &QPushButton::clicked,
        this, [this] { setProjectEdgeFaceMode(false); });
    QObject::connect(
        project_edge_face_button_, &QPushButton::clicked,
        this, [this] { setProjectEdgeFaceMode(true); });
    QObject::connect(
        project_edge_boundary_button_, &QPushButton::clicked,
        this, [this] { setProjectEdgeBoundaryMode(); });
    QObject::connect(
        project_edge_regular_button_, &QPushButton::clicked,
        this, [this] {
            setProjectEdgeRole(sketch::EntityRole::regular);
        });
    QObject::connect(
        project_edge_construction_button_, &QPushButton::clicked,
        this, [this] {
            setProjectEdgeRole(sketch::EntityRole::construction);
        });
    QObject::connect(
        project_edge_clear_button_, &QPushButton::clicked,
        this, [this] { clearProjectEdgeSelection(); });
    QObject::connect(
        project_edge_remove_button_, &QPushButton::clicked,
        this, [this] {
            if (project_edge_boundary_mode_) {
                removeProjectBoundarySelection();
            } else if (project_edge_face_mode_) {
                removeProjectFaceSelection();
            } else if (viewport_controller_ &&
                       viewport_controller_->
                           removePrimaryBodyTopologyToolSelection()) {
                tryStageProjectEdgeSelection();
            }
        });
    QObject::connect(
        project_edge_finish_button_, &QPushButton::clicked,
        this, [this] {
            static_cast<void>(finishProjectEdgeTool());
        });
    QObject::connect(
        project_edge_cancel_button_, &QPushButton::clicked,
        this, [this] { cancelProjectEdgeTool(); });

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
    syncAxisUi();
    syncDatumPlaneUi();
    syncExtrudeUi();
    syncRevolveUi();
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
    clearAxisRuntimeContext();
    clearDatumPlaneRuntimeContext();
    clearExtrudeRuntimeContext();
    clearRevolveRuntimeContext();
    clearEdgeFeatureRuntimeContext();
    clearProjectEdgeRuntimeContext();
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

    clearAxisRuntimeContext();
    clearDatumPlaneRuntimeContext();
    clearExtrudeRuntimeContext();
    clearRevolveRuntimeContext();
    clearEdgeFeatureRuntimeContext();
    clearProjectEdgeRuntimeContext();
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
        revolve_draft_ ||
        fillet_draft_ ||
        chamfer_draft_ ||
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
        revolve_draft_ ||
        fillet_draft_ ||
        chamfer_draft_ ||
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
        revolve_draft_ ||
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
        revolve_draft_ ||
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
    if (revolve_draft_) {
        setStatusText(
            QStringLiteral(
                "Finish or cancel Revolve before deleting an Axis."));
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
                    source_already_designated) {
            result_text =
                QStringLiteral(
                    "Rejected — this Line is already designated by another authored Axis.");
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
    if (axis_draft_ ||
        extrude_profile_pick_active_ ||
        extrude_draft_ ||
        revolve_draft_ ||
        fillet_draft_ ||
        chamfer_draft_ ||
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
        axis_draft_ ||
        extrude_profile_pick_active_ ||
        extrude_draft_ ||
        revolve_draft_ ||
        fillet_draft_ ||
        chamfer_draft_ ||
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
        revolve_draft_ ||
        fillet_draft_ ||
        chamfer_draft_ ||
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
    if (extrude_draft_ ||
        revolve_draft_) {
        setStatusText(
            QStringLiteral(
                "Finish or cancel the active solid Feature command before Extrude."));
        return false;
    }
    if (datum_plane_draft_ ||
        axis_draft_ ||
        active_sketch_id_ ||
        sketch_support_pick_active_) {
        setStatusText(
            QStringLiteral(
                "Finish or cancel the active Datum Plane/Sketch context before Extrude."));
        return false;
    }

    if (selected_profile_id_) {
        const auto evaluation =
            evaluateCurrentProfile(
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
        evaluateCurrentProfile(
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
    if (extrude_draft_ ||
        revolve_draft_) {
        setStatusText(
            QStringLiteral(
                "Finish or cancel the active solid Feature command before Extrude."));
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
        evaluateCurrentProfile(
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
    if (extrude_draft_ ||
        revolve_draft_) {
        setStatusText(
            QStringLiteral(
                "Finish or cancel the active solid Feature command before editing another Feature."));
        return false;
    }
    if (datum_plane_draft_ ||
        axis_draft_ ||
        active_sketch_id_ ||
        sketch_support_pick_active_) {
        setStatusText(
            QStringLiteral(
                "Finish or cancel the active Axis/Datum Plane/Sketch context before Edit Extrude."));
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


bool CadWorkbench::startRevolveTool() {
    auto* document_session =
        activeDocumentSession();
    if (document_session == nullptr ||
        solid_modeling_kernel_ == nullptr ||
        revolve_draft_ ||
        sketch_support_pick_active_ ||
        axis_draft_ ||
        datum_plane_draft_ ||
        extrude_profile_pick_active_ ||
        extrude_draft_ ||
        active_sketch_id_) {
        return false;
    }

    auto draft =
        application::RevolveDraft::beginCreate(
            *document_session);
    if (selected_profile_id_) {
        const auto evaluation =
            evaluateCurrentProfile(
                    *selected_profile_id_);
        if (evaluation && evaluation->valid()) {
            static_cast<void>(
                draft.setProfile(
                    *selected_profile_id_));
        }
    }

    if (selected_axis_id_) {
        const auto found =
            std::find_if(
                axis_evaluation_statuses_.begin(),
                axis_evaluation_statuses_.end(),
                [this](const AxisEvaluationUiState& item) {
                    return item.axis_id ==
                        *selected_axis_id_;
                });
        if (found != axis_evaluation_statuses_.end() &&
            found->status ==
                part::AxisEvaluationStatus::resolved &&
            found->line) {
            static_cast<void>(
                draft.setAxis(
                    part::AxisReference{
                        part::AuthoredAxisReference{
                            *selected_axis_id_}}));
        }
    } else if (viewport_controller_ != nullptr) {
        if (const auto role =
                viewport_controller_->primarySelection();
            role && part::isOriginAxis(*role)) {
            static_cast<void>(
                draft.setAxis(
                    part::AxisReference{
                        part::BuiltinOriginAxisReference{
                            *role}}));
        }
    }

    revolve_draft_ = std::move(draft);
    revolve_evaluation_.reset();
    revolve_angle_input_valid_ = true;
    if (revolve_angle_edit_ != nullptr) {
        const QSignalBlocker blocked{
            revolve_angle_edit_};
        revolve_angle_edit_->setText(
            formatAngleForCad(
                revolve_draft_->angle()));
    }
    if (viewport_controller_) {
        viewport_controller_->clearSolidPreview();
        viewport_controller_->
            setTransientProfilePresentationOverride(
                std::nullopt,
                std::nullopt);
        viewport_controller_->
            setTransientAxisEmphasis(
                std::nullopt);
    }

    refreshRevolvePreview();
    syncActionState();
    notifyCadInputContextChanged();
    setStatusText(
        revolve_draft_->profileId()
            ? QStringLiteral(
                  "Revolve active — select an Origin X/Y/Z or authored Axis.")
            : QStringLiteral(
                  "Revolve active — select a valid Profile, then an Origin X/Y/Z or authored Axis."));
    return true;
}

bool CadWorkbench::startRevolveEdit(
    part::FeatureId feature_id) {
    auto* document_session =
        activeDocumentSession();
    if (document_session == nullptr ||
        solid_modeling_kernel_ == nullptr ||
        revolve_draft_ ||
        sketch_support_pick_active_ ||
        axis_draft_ ||
        datum_plane_draft_ ||
        extrude_profile_pick_active_ ||
        extrude_draft_ ||
        active_sketch_id_) {
        return false;
    }

    auto draft =
        application::RevolveDraft::beginEdit(
            *document_session,
            feature_id);
    if (!draft) {
        setStatusText(
            QStringLiteral(
                "Selected Feature is not an editable Revolve."));
        return false;
    }

    revolve_draft_ = std::move(*draft);
    revolve_evaluation_.reset();
    revolve_angle_input_valid_ = true;
    if (revolve_angle_edit_ != nullptr) {
        const QSignalBlocker blocked{
            revolve_angle_edit_};
        revolve_angle_edit_->setText(
            formatAngleForCad(
                revolve_draft_->angle()));
    }
    if (viewport_controller_) {
        if (revolve_draft_->profileId()) {
            viewport_controller_->
                setTransientProfilePresentationOverride(
                    *revolve_draft_->profileId(),
                    std::nullopt);
        }
        viewport_controller_->
            setTransientAxisEmphasis(
                revolve_draft_->axis());
    }

    refreshRevolvePreview();
    syncActionState();
    notifyCadInputContextChanged();
    setStatusText(
        QStringLiteral(
            "Edit Revolve active — FeatureId is preserved."));
    return true;
}

void CadWorkbench::tryStageRevolveProfile(
    std::optional<part::ProfileId> profile_id) {
    if (!revolve_draft_ ||
        !profile_id ||
        document_session_ == nullptr) {
        return;
    }

    const auto evaluation =
        evaluateCurrentProfile(*profile_id);
    if (!evaluation || !evaluation->valid()) {
        setStatusText(
            QStringLiteral(
                "Revolve requires one valid Profile."));
        return;
    }

    if (revolve_draft_->setProfile(
            *profile_id)) {
        revolve_evaluation_.reset();
        refreshRevolvePreview();
        notifyCadInputContextChanged();
        setStatusText(
            revolve_draft_->axis()
                ? QStringLiteral(
                      "Revolve Profile selected.")
                : QStringLiteral(
                      "Revolve Profile selected — now select an Origin X/Y/Z or authored Axis."));
    }
}

void CadWorkbench::tryStageRevolveAxisFromBuiltin(
    std::optional<core::BuiltinReferenceRole> role) {
    if (!revolve_draft_ ||
        !role ||
        !part::isOriginAxis(*role)) {
        return;
    }

    if (revolve_draft_->setAxis(
            part::AxisReference{
                part::BuiltinOriginAxisReference{
                    *role}})) {
        revolve_evaluation_.reset();
        refreshRevolvePreview();
        notifyCadInputContextChanged();
        setStatusText(
            revolve_draft_->profileId()
                ? QStringLiteral(
                      "Revolve Origin Axis selected.")
                : QStringLiteral(
                      "Revolve Axis selected — now select a valid Profile."));
    }
}

void CadWorkbench::tryStageRevolveAxisFromAuthored(
    std::optional<part::AxisId> axis_id) {
    if (!revolve_draft_ ||
        !axis_id ||
        document_session_ == nullptr) {
        return;
    }

    if (!part_evaluation_revision_ ||
        *part_evaluation_revision_ !=
            document_session_->document()
                .revision()) {
        refreshPartFeatureEvaluationSnapshot();
    }
    const auto found =
        std::find_if(
            axis_evaluation_statuses_.begin(),
            axis_evaluation_statuses_.end(),
            [axis_id](const AxisEvaluationUiState& item) {
                return item.axis_id == *axis_id;
            });
    if (found == axis_evaluation_statuses_.end() ||
        found->status !=
            part::AxisEvaluationStatus::resolved ||
        !found->line) {
        setStatusText(
            QStringLiteral(
                "Revolve requires a resolved authored Axis."));
        return;
    }

    if (revolve_draft_->setAxis(
            part::AxisReference{
                part::AuthoredAxisReference{
                    *axis_id}})) {
        revolve_evaluation_.reset();
        refreshRevolvePreview();
        notifyCadInputContextChanged();
        setStatusText(
            revolve_draft_->profileId()
                ? QStringLiteral(
                      "Revolve authored Axis selected.")
                : QStringLiteral(
                      "Revolve Axis selected — now select a valid Profile."));
    }
}

void CadWorkbench::cancelRevolve() {
    if (!revolve_draft_) {
        return;
    }
    clearRevolveRuntimeContext();
    syncActionState();
    notifyCadInputContextChanged();
    setStatusText(
        QStringLiteral(
            "Revolve cancelled — no authored change."));
    if (viewport_widget_ != nullptr) {
        viewport_widget_->setFocus(
            Qt::OtherFocusReason);
    }
}

bool CadWorkbench::finishRevolve() {
    flushRevolvePreview();
    auto* document_session =
        activeDocumentSession();
    if (document_session == nullptr ||
        solid_modeling_kernel_ == nullptr ||
        !revolve_draft_ ||
        !revolve_evaluation_ ||
        !revolve_angle_input_valid_ ||
        !revolve_evaluation_->committable()) {
        setStatusText(
            QStringLiteral(
                "Revolve cannot finish until Profile, Axis and preview candidate are valid."));
        return false;
    }

    const auto result =
        application::finishRevolveDraft(
            *document_session,
            *revolve_draft_,
            *revolve_evaluation_,
            *solid_modeling_kernel_);
    if (!result.ok()) {
        setStatusText(
            result.diagnostic.empty()
                ? QStringLiteral(
                      "Revolve Finish was rejected.")
                : fromUtf8(result.diagnostic));
        refreshRevolvePreview();
        return false;
    }

    const auto committed_feature_id =
        result.feature_id;
    clearRevolveRuntimeContext();
    refreshActiveContext();
    if (committed_feature_id) {
        navigateToFeature(
            *committed_feature_id);
    }
    notifyCadInputContextChanged();
    setStatusText(
        QStringLiteral(
            "Revolve finished — Feature committed."));
    if (viewport_widget_ != nullptr) {
        viewport_widget_->setFocus(
            Qt::OtherFocusReason);
    }
    return true;
}

void CadWorkbench::clearRevolveRuntimeContext() {
    if (revolve_preview_timer_ != nullptr) {
        revolve_preview_timer_->stop();
    }
    revolve_draft_.reset();
    revolve_evaluation_.reset();
    revolve_angle_input_valid_ = true;
    if (viewport_controller_) {
        viewport_controller_->clearSolidPreview();
        viewport_controller_->
            setTransientProfilePresentationOverride(
                std::nullopt,
                std::nullopt);
        viewport_controller_->
            setTransientAxisEmphasis(
                std::nullopt);
    }
    if (revolve_angle_edit_ != nullptr) {
        const QSignalBlocker blocked{
            revolve_angle_edit_};
        revolve_angle_edit_->clear();
    }
    syncRevolveUi();
}

bool CadWorkbench::setRevolveAngle(
    core::AngleValue angle,
    std::optional<std::string_view> display_text,
    bool refresh_now) {
    constexpr double full_turn =
        2.0 * std::numbers::pi_v<double>;
    if (!revolve_draft_ ||
        !angle.finite() ||
        !(angle.radians > 0.0) ||
        angle.radians > full_turn) {
        return false;
    }
    if (!revolve_draft_->setAngle(angle)) {
        return false;
    }

    revolve_angle_input_valid_ = true;
    if (display_text &&
        revolve_angle_edit_ != nullptr) {
        const QSignalBlocker blocked{
            revolve_angle_edit_};
        revolve_angle_edit_->setText(
            fromUtf8(*display_text));
    }

    revolve_evaluation_.reset();
    if (refresh_now) {
        refreshRevolvePreview();
    } else {
        scheduleRevolvePreview();
        syncRevolveUi();
    }
    notifyCadInputContextChanged();
    return true;
}

void CadWorkbench::scheduleRevolvePreview() {
    if (revolve_preview_timer_ == nullptr) {
        refreshRevolvePreview();
        return;
    }
    revolve_preview_timer_->start();
}

void CadWorkbench::flushRevolvePreview() {
    if (revolve_preview_timer_ == nullptr ||
        !revolve_preview_timer_->isActive()) {
        return;
    }
    revolve_preview_timer_->stop();
    refreshRevolvePreview();
}

void CadWorkbench::refreshRevolvePreview() {
    if (revolve_preview_timer_ != nullptr) {
        revolve_preview_timer_->stop();
    }
    revolve_evaluation_.reset();

    if (viewport_controller_) {
        viewport_controller_->clearSolidPreview();
        viewport_controller_->setTransientAxisEmphasis(
            revolve_draft_
                ? revolve_draft_->axis()
                : std::nullopt);
    }

    const auto sync_source_profile =
        [this](bool preview_ready) {
            if (viewport_controller_ == nullptr) {
                return;
            }
            if (!revolve_draft_ ||
                !revolve_draft_->profileId()) {
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
                        *revolve_draft_->profileId());
                return;
            }
            if (revolve_draft_->mode() ==
                application::RevolveDraftMode::edit) {
                viewport_controller_->
                    setTransientProfilePresentationOverride(
                        *revolve_draft_->profileId(),
                        std::nullopt);
            } else {
                viewport_controller_->
                    setTransientProfilePresentationOverride(
                        std::nullopt,
                        std::nullopt);
            }
        };

    if (!revolve_draft_ ||
        !revolve_angle_input_valid_ ||
        document_session_ == nullptr ||
        solid_modeling_kernel_ == nullptr) {
        sync_source_profile(false);
        syncRevolveUi();
        return;
    }

    auto evaluation =
        document_session_->evaluateRevolveDraft(
            *revolve_draft_,
            *solid_modeling_kernel_);
    bool preview_ready = false;
    if (evaluation.previewSolidAvailable() &&
        viewport_controller_ != nullptr) {
        const auto tone =
            revolve_draft_->operation() ==
                    part::RevolveOperation::cut
                ? viewer::SolidPreviewTone::subtractive
                : viewer::SolidPreviewTone::additive;
        if (viewport_controller_->setSolidPreview(
                *evaluation.preview_delta_mesh,
                tone)) {
            preview_ready = true;
        } else {
            evaluation.preview_delta_mesh.reset();
        }
    }

    sync_source_profile(preview_ready);
    revolve_evaluation_ = std::move(evaluation);
    syncRevolveUi();
}

void CadWorkbench::syncRevolveUi() {
    const bool active =
        revolve_draft_.has_value();
    if (revolve_operations_widget_ != nullptr) {
        revolve_operations_widget_->setVisible(
            active);
    }
    if (!active) {
        return;
    }

    syncing_revolve_ui_ = true;
    if (revolve_profile_label_ != nullptr) {
        revolve_profile_label_->setText(
            revolve_draft_->profileId()
                ? fromUtf8(
                      revolve_draft_->profileId()
                          ->serialized())
                : QStringLiteral("<select Profile>"));
    }
    if (revolve_axis_label_ != nullptr) {
        revolve_axis_label_->setText(
            revolve_draft_->axis()
                ? revolveAxisText(
                      *revolve_draft_->axis())
                : QStringLiteral("<select Axis>"));
    }

    const bool editing =
        revolve_draft_->mode() ==
        application::RevolveDraftMode::edit;
    const bool add =
        revolve_draft_->operation() ==
        part::RevolveOperation::add;
    revolve_add_button_->setChecked(add);
    revolve_cut_button_->setChecked(!add);

    const bool one_side =
        revolve_draft_->extentMode() ==
        application::RevolveDraftExtentMode::
            one_side;
    revolve_one_side_button_->setChecked(
        one_side);
    revolve_midplane_button_->setChecked(
        !one_side);
    revolve_reverse_button_->setEnabled(
        one_side);
    revolve_reverse_button_->setChecked(
        one_side &&
        revolve_draft_->reversed());

    bool cut_allowed = false;
    if (document_session_ != nullptr) {
        const auto& features =
            document_session_->document()
                .body().features;
        if (!editing) {
            cut_allowed = !features.empty();
        } else if (revolve_draft_->featureId()) {
            const auto found =
                std::find_if(
                    features.begin(),
                    features.end(),
                    [this](const part::PartFeature& feature) {
                        return feature.id ==
                            *revolve_draft_->featureId();
                    });
            cut_allowed =
                found != features.end() &&
                found != features.begin();
        }
    }
    revolve_cut_button_->setEnabled(
        cut_allowed);

    const bool committable =
        revolve_angle_input_valid_ &&
        revolve_evaluation_ &&
        revolve_evaluation_->committable();
    revolve_finish_button_->setEnabled(
        committable);
    revolve_finish_button_->setText(
        editing
            ? QStringLiteral("Finish Edit")
            : QStringLiteral("Finish Revolve"));

    if (!revolve_angle_input_valid_) {
        revolve_result_label_->setText(
            QStringLiteral(
                "Angle must satisfy 0 < angle <= 360 deg."));
    } else if (revolve_evaluation_) {
        revolve_result_label_->setText(
            revolveEvaluationText(
                *revolve_evaluation_));
    } else {
        revolve_result_label_->setText(
            QStringLiteral(
                "Select a Profile and an Origin/Authored Axis."));
    }

    if (operations_placeholder_ != nullptr) {
        operations_placeholder_->setText(
            QStringLiteral(
                "%1 — %2 · %3%4")
                .arg(
                    editing
                        ? QStringLiteral("Edit Revolve")
                        : QStringLiteral("Revolve"),
                    add
                        ? QStringLiteral("Add")
                        : QStringLiteral("Cut"),
                    one_side
                        ? QStringLiteral("One Side")
                        : QStringLiteral("Midplane"),
                    one_side &&
                            revolve_draft_->reversed()
                        ? QStringLiteral(" · Reverse")
                        : QString{}));
    }

    syncing_revolve_ui_ = false;
}

application::CadInputSubmitResult
CadWorkbench::submitRevolveCadInput(
    std::string_view text) {
    if (!revolve_draft_ ||
        document_session_ == nullptr) {
        return {
            false,
            "No active Revolve draft."};
    }

    const auto keyword =
        upperAsciiTrimmed(text);
    if (keyword == "CANCEL" ||
        keyword == "ESC") {
        cancelRevolve();
        return {true, {}};
    }
    if (keyword == "REVOLVE") {
        return {true, {}};
    }

    const auto set_origin_axis =
        [this](core::BuiltinReferenceRole role) {
            return revolve_draft_->setAxis(
                part::AxisReference{
                    part::BuiltinOriginAxisReference{
                        role}});
        };
    if (keyword == "X" ||
        keyword == "XAXIS" ||
        keyword == "X AXIS") {
        if (set_origin_axis(
                core::BuiltinReferenceRole::x_axis)) {
            refreshRevolvePreview();
            notifyCadInputContextChanged();
        }
        return {true, {}};
    }
    if (keyword == "Y" ||
        keyword == "YAXIS" ||
        keyword == "Y AXIS") {
        if (set_origin_axis(
                core::BuiltinReferenceRole::y_axis)) {
            refreshRevolvePreview();
            notifyCadInputContextChanged();
        }
        return {true, {}};
    }
    if (keyword == "Z" ||
        keyword == "ZAXIS" ||
        keyword == "Z AXIS") {
        if (set_origin_axis(
                core::BuiltinReferenceRole::z_axis)) {
            refreshRevolvePreview();
            notifyCadInputContextChanged();
        }
        return {true, {}};
    }

    if (!revolve_draft_->profileId()) {
        return {
            false,
            "REVOLVE is waiting for one valid Profile selection; use Tree/viewport or CANCEL."};
    }

    if (!revolve_draft_->axis()) {
        return {
            false,
            "REVOLVE is waiting for X/Y/Z Origin Axis or one resolved authored Axis selection."};
    }

    if (keyword.empty() ||
        keyword == "FINISH") {
        return finishRevolve()
            ? application::CadInputSubmitResult{
                  true, {}}
            : application::CadInputSubmitResult{
                  false,
                  "Revolve Finish was rejected."};
    }
    if (keyword == "ADD") {
        if (!revolve_draft_->setOperation(
                part::RevolveOperation::add)) {
            return {
                false,
                "ADD could not be applied."};
        }
        refreshRevolvePreview();
        notifyCadInputContextChanged();
        return {true, {}};
    }
    if (keyword == "CUT") {
        const auto& features =
            document_session_->document()
                .body().features;
        bool cut_allowed = !features.empty();
        if (revolve_draft_->mode() ==
                application::RevolveDraftMode::edit &&
            revolve_draft_->featureId()) {
            const auto found =
                std::find_if(
                    features.begin(),
                    features.end(),
                    [this](const part::PartFeature& feature) {
                        return feature.id ==
                            *revolve_draft_->featureId();
                    });
            cut_allowed =
                found != features.end() &&
                found != features.begin();
        }
        if (!cut_allowed) {
            return {
                false,
                "The first solid-producing Revolve must be ADD."};
        }
        if (!revolve_draft_->setOperation(
                part::RevolveOperation::cut)) {
            return {
                false,
                "CUT could not be applied."};
        }
        refreshRevolvePreview();
        notifyCadInputContextChanged();
        return {true, {}};
    }
    if (keyword == "REVERSE") {
        if (revolve_draft_->extentMode() !=
            application::RevolveDraftExtentMode::
                one_side) {
            return {
                false,
                "REVERSE is available only for One Side Revolve."};
        }
        static_cast<void>(
            revolve_draft_->setReversed(
                !revolve_draft_->reversed()));
        refreshRevolvePreview();
        notifyCadInputContextChanged();
        return {true, {}};
    }
    if (keyword == "MIDPLANE") {
        static_cast<void>(
            revolve_draft_->setExtentMode(
                application::RevolveDraftExtentMode::
                    midplane));
        refreshRevolvePreview();
        notifyCadInputContextChanged();
        return {true, {}};
    }
    if (keyword == "ONESIDE" ||
        keyword == "ONE SIDE") {
        static_cast<void>(
            revolve_draft_->setExtentMode(
                application::RevolveDraftExtentMode::
                    one_side));
        refreshRevolvePreview();
        notifyCadInputContextChanged();
        return {true, {}};
    }

    const auto quantity =
        application::parseCadQuantity(
            text,
            {
                application::CadQuantityDimension::
                    angle,
                document_session_->document()
                    .lengthUnit()});
    constexpr double full_turn =
        2.0 * std::numbers::pi_v<double>;
    if (!quantity ||
        !(quantity->canonical_value > 0.0) ||
        quantity->canonical_value > full_turn) {
        return {
            false,
            "Revolve Angle expects 0 < angle <= 360 deg."};
    }

    return setRevolveAngle(
               core::AngleValue{
                   quantity->canonical_value},
               text)
        ? application::CadInputSubmitResult{
              true, {}}
        : application::CadInputSubmitResult{
              false,
              "Revolve Angle could not be applied."};
}


bool CadWorkbench::startFilletTool() {
    auto* document_session =
        activeDocumentSession();
    if (document_session == nullptr ||
        solid_modeling_kernel_ == nullptr ||
        fillet_draft_ ||
        chamfer_draft_ ||
        sketch_support_pick_active_ ||
        axis_draft_ ||
        datum_plane_draft_ ||
        extrude_profile_pick_active_ ||
        extrude_draft_ ||
        revolve_draft_ ||
        active_sketch_id_) {
        return false;
    }
    if (document_session->document()
            .body().features.empty()) {
        setStatusText(
            QStringLiteral(
                "Fillet requires an existing evaluated Body."));
        return false;
    }

    const auto selected =
        viewport_controller_ != nullptr
            ? viewport_controller_->
                  selectedMaterialEdgeReferences()
            : std::nullopt;
    if (selected) {
        fillet_draft_ =
            application::FilletDraft::beginCreate(
                *document_session,
                *selected);
        if (!fillet_draft_) {
            setStatusText(
                QStringLiteral(
                    "Selected Edges cannot seed Fillet at the current Body stage."));
            return false;
        }
    } else {
        fillet_draft_ =
            application::FilletDraft::beginCreate(
                *document_session);
    }

    edge_feature_evaluation_.reset();
    edge_feature_parameter_input_valid_ = true;
    if (edge_feature_parameter_edit_ != nullptr) {
        const QSignalBlocker blocked{
            edge_feature_parameter_edit_};
        edge_feature_parameter_edit_->clear();
    }
    if (viewport_controller_ != nullptr) {
        viewport_controller_->
            setBodyTopologyEdgeDraftMode(true);
        viewport_controller_->clearSolidPreview();
    }
    tryStageEdgeFeatureSelection();
    syncActionState();
    syncEdgeFeatureUi();
    notifyCadInputContextChanged();
    setStatusText(
        selected
            ? QStringLiteral(
                  "Fillet active — selected material Edges seeded; enter Radius or toggle more Edges.")
            : QStringLiteral(
                  "Fillet active — select/toggle one or more material Edges, then enter Radius."));
    if (viewport_widget_ != nullptr) {
        viewport_widget_->setFocus(
            Qt::OtherFocusReason);
    }
    return true;
}

bool CadWorkbench::startChamferTool() {
    auto* document_session =
        activeDocumentSession();
    if (document_session == nullptr ||
        solid_modeling_kernel_ == nullptr ||
        fillet_draft_ ||
        chamfer_draft_ ||
        sketch_support_pick_active_ ||
        axis_draft_ ||
        datum_plane_draft_ ||
        extrude_profile_pick_active_ ||
        extrude_draft_ ||
        revolve_draft_ ||
        active_sketch_id_) {
        return false;
    }
    if (document_session->document()
            .body().features.empty()) {
        setStatusText(
            QStringLiteral(
                "Chamfer requires an existing evaluated Body."));
        return false;
    }

    const auto selected =
        viewport_controller_ != nullptr
            ? viewport_controller_->
                  selectedMaterialEdgeReferences()
            : std::nullopt;
    if (selected) {
        chamfer_draft_ =
            application::ChamferDraft::beginCreate(
                *document_session,
                *selected);
        if (!chamfer_draft_) {
            setStatusText(
                QStringLiteral(
                    "Selected Edges cannot seed Chamfer at the current Body stage."));
            return false;
        }
    } else {
        chamfer_draft_ =
            application::ChamferDraft::beginCreate(
                *document_session);
    }

    edge_feature_evaluation_.reset();
    edge_feature_parameter_input_valid_ = true;
    if (edge_feature_parameter_edit_ != nullptr) {
        const QSignalBlocker blocked{
            edge_feature_parameter_edit_};
        edge_feature_parameter_edit_->clear();
    }
    if (viewport_controller_ != nullptr) {
        viewport_controller_->
            setBodyTopologyEdgeDraftMode(true);
        viewport_controller_->clearSolidPreview();
    }
    tryStageEdgeFeatureSelection();
    syncActionState();
    syncEdgeFeatureUi();
    notifyCadInputContextChanged();
    setStatusText(
        selected
            ? QStringLiteral(
                  "Chamfer active — selected material Edges seeded; enter Distance or toggle more Edges.")
            : QStringLiteral(
                  "Chamfer active — select/toggle one or more material Edges, then enter Distance."));
    if (viewport_widget_ != nullptr) {
        viewport_widget_->setFocus(
            Qt::OtherFocusReason);
    }
    return true;
}

bool CadWorkbench::startEdgeFeatureEdit(
    part::FeatureId feature_id) {
    auto* document_session =
        activeDocumentSession();
    if (document_session == nullptr ||
        solid_modeling_kernel_ == nullptr ||
        viewport_controller_ == nullptr ||
        fillet_draft_ ||
        chamfer_draft_ ||
        sketch_support_pick_active_ ||
        axis_draft_ ||
        datum_plane_draft_ ||
        extrude_profile_pick_active_ ||
        extrude_draft_ ||
        revolve_draft_ ||
        active_sketch_id_) {
        return false;
    }

    const auto* feature =
        document_session->document()
            .findFeature(feature_id);
    if (feature == nullptr ||
        feature->suppressed) {
        setStatusText(
            QStringLiteral(
                "Selected Edge Feature is unavailable or Suppressed."));
        return false;
    }

    std::optional<application::FilletDraft> fillet;
    std::optional<application::ChamferDraft> chamfer;
    if (std::holds_alternative<
            part::FilletFeature>(
            feature->definition)) {
        fillet =
            application::FilletDraft::beginEdit(
                *document_session,
                feature_id);
    } else if (
        std::holds_alternative<
            part::ChamferFeature>(
            feature->definition)) {
        chamfer =
            application::ChamferDraft::beginEdit(
                *document_session,
                feature_id);
    } else {
        return false;
    }

    const auto required_stage =
        fillet
            ? fillet->requiredStage()
            : chamfer
                ? chamfer->requiredStage()
                : std::nullopt;
    if (!required_stage) {
        setStatusText(
            QStringLiteral(
                "Edge Feature Edit has no current upstream Body stage."));
        return false;
    }

    syncing_edge_feature_selection_ = true;
    viewport_controller_->
        setBodyTopologyEdgeDraftMode(true);
    if (!viewport_controller_->
             setBodyTopologyToolStage(
                 *required_stage)) {
        viewport_controller_->
            setBodyTopologyEdgeDraftMode(false);
        syncing_edge_feature_selection_ = false;
        setStatusText(
            QStringLiteral(
                "Edge Feature upstream Body stage is unavailable; repair cannot start."));
        return false;
    }

    fillet_draft_ = std::move(fillet);
    chamfer_draft_ = std::move(chamfer);
    edge_feature_evaluation_.reset();
    edge_feature_parameter_input_valid_ = true;

    const auto& authored_edges =
        fillet_draft_
            ? fillet_draft_->edges()
            : chamfer_draft_->edges();
    const auto unresolved =
        viewport_controller_->
            restoreMaterialEdgeToolSelection(
                authored_edges);
    if (!unresolved) {
        syncing_edge_feature_selection_ = false;
        clearEdgeFeatureRuntimeContext();
        setStatusText(
            QStringLiteral(
                "Edge Feature authored selection could not be reconstructed for Edit."));
        return false;
    }
    edge_feature_unresolved_edit_edges_ =
        *unresolved;
    syncing_edge_feature_selection_ = false;

    if (edge_feature_parameter_edit_ != nullptr) {
        const QSignalBlocker blocked{
            edge_feature_parameter_edit_};
        const auto parameter =
            fillet_draft_
                ? fillet_draft_->radius()
                : chamfer_draft_->distance();
        edge_feature_parameter_edit_->setText(
            parameter
                ? formatLengthForPart(
                      *parameter,
                      document_session->document()
                          .lengthUnit())
                : QString{});
    }

    refreshEdgeFeaturePreview();
    syncActionState();
    syncEdgeFeatureUi();
    notifyCadInputContextChanged();

    const auto kind =
        fillet_draft_
            ? QStringLiteral("Fillet")
            : QStringLiteral("Chamfer");
    setStatusText(
        edge_feature_unresolved_edit_edges_.empty()
            ? QStringLiteral(
                  "Edit %1 active — FeatureId and exact upstream Body stage are preserved.")
                  .arg(kind)
            : QStringLiteral(
                  "Edit %1 active — %2 authored Edge input(s) are unresolved; use Clear and explicitly reselect repair Edges.")
                  .arg(kind)
                  .arg(static_cast<qulonglong>(
                      edge_feature_unresolved_edit_edges_
                          .size())));
    return true;
}

void CadWorkbench::clearEdgeFeatureSelection() {
    if (!fillet_draft_ &&
        !chamfer_draft_) {
        return;
    }

    edge_feature_unresolved_edit_edges_.clear();
    const bool applied =
        fillet_draft_
            ? fillet_draft_->setEdges({})
            : chamfer_draft_->setEdges({});
    if (!applied) {
        return;
    }

    edge_feature_evaluation_.reset();
    syncing_edge_feature_selection_ = true;
    if (viewport_controller_ != nullptr) {
        viewport_controller_->
            clearBodyTopologyToolSelection();
    }
    syncing_edge_feature_selection_ = false;
    refreshEdgeFeaturePreview();
    notifyCadInputContextChanged();
}

void CadWorkbench::cancelEdgeFeature() {
    if (!fillet_draft_ &&
        !chamfer_draft_) {
        return;
    }
    const auto name =
        fillet_draft_
            ? QStringLiteral("Fillet")
            : QStringLiteral("Chamfer");
    const auto edit_feature_id =
        fillet_draft_ &&
                fillet_draft_->mode() ==
                    application::EdgeFeatureDraftMode::edit
            ? fillet_draft_->featureId()
            : chamfer_draft_ &&
                      chamfer_draft_->mode() ==
                          application::EdgeFeatureDraftMode::edit
                ? chamfer_draft_->featureId()
                : std::nullopt;
    clearEdgeFeatureRuntimeContext();
    if (edit_feature_id) {
        navigateToFeature(*edit_feature_id);
    }
    syncActionState();
    notifyCadInputContextChanged();
    setStatusText(
        QStringLiteral(
            "%1 cancelled — no authored change.")
            .arg(name));
    if (viewport_widget_ != nullptr) {
        viewport_widget_->setFocus(
            Qt::OtherFocusReason);
    }
}

bool CadWorkbench::finishEdgeFeature() {
    flushEdgeFeaturePreview();
    auto* document_session =
        activeDocumentSession();
    if (document_session == nullptr ||
        solid_modeling_kernel_ == nullptr ||
        (!fillet_draft_ &&
         !chamfer_draft_) ||
        !edge_feature_evaluation_ ||
        !edge_feature_parameter_input_valid_ ||
        !edge_feature_evaluation_->committable()) {
        setStatusText(
            QStringLiteral(
                "Edge Feature cannot finish until the Edge set, parameter and exact preview candidate are valid."));
        return false;
    }

    const bool fillet =
        fillet_draft_.has_value();
    const auto result =
        fillet
            ? application::finishFilletDraft(
                  *document_session,
                  *fillet_draft_,
                  *edge_feature_evaluation_,
                  *solid_modeling_kernel_)
            : application::finishChamferDraft(
                  *document_session,
                  *chamfer_draft_,
                  *edge_feature_evaluation_,
                  *solid_modeling_kernel_);
    if (!result.ok()) {
        setStatusText(
            result.diagnostic.empty()
                ? QStringLiteral(
                      "%1 Finish was rejected.")
                      .arg(
                          fillet
                              ? QStringLiteral("Fillet")
                              : QStringLiteral("Chamfer"))
                : fromUtf8(result.diagnostic));
        refreshEdgeFeaturePreview();
        return false;
    }

    const auto committed_feature_id =
        result.feature_id;
    const auto name =
        fillet
            ? QStringLiteral("Fillet")
            : QStringLiteral("Chamfer");
    clearEdgeFeatureRuntimeContext();
    refreshActiveContext();
    if (committed_feature_id) {
        navigateToFeature(*committed_feature_id);
    }
    notifyCadInputContextChanged();
    setStatusText(
        QStringLiteral(
            "%1 finished — Feature committed.")
            .arg(name));
    if (viewport_widget_ != nullptr) {
        viewport_widget_->setFocus(
            Qt::OtherFocusReason);
    }
    return true;
}

void CadWorkbench::clearEdgeFeatureRuntimeContext() {
    if (edge_feature_preview_timer_ != nullptr) {
        edge_feature_preview_timer_->stop();
    }
    fillet_draft_.reset();
    chamfer_draft_.reset();
    edge_feature_evaluation_.reset();
    edge_feature_unresolved_edit_edges_.clear();
    edge_feature_parameter_input_valid_ = true;
    syncing_edge_feature_selection_ = true;
    if (viewport_controller_ != nullptr) {
        viewport_controller_->clearSolidPreview();
        viewport_controller_->
            setBodyTopologyEdgeDraftMode(false);
        static_cast<void>(
            viewport_controller_->
                setBodyTopologyToolStage(
                    std::nullopt));
    }
    syncing_edge_feature_selection_ = false;
    if (edge_feature_parameter_edit_ != nullptr) {
        const QSignalBlocker blocked{
            edge_feature_parameter_edit_};
        edge_feature_parameter_edit_->clear();
    }
    syncEdgeFeatureUi();
}

void CadWorkbench::tryStageEdgeFeatureSelection() {
    if (syncing_edge_feature_selection_ ||
        (!fillet_draft_ &&
         !chamfer_draft_) ||
        viewport_controller_ == nullptr) {
        return;
    }

    const auto selected =
        viewport_controller_->
            selectedMaterialEdgeReferences();
    std::vector<part::MaterialEdgeReference> edges;
    if (selected) {
        edges = *selected;
    }

    const bool editing =
        fillet_draft_
            ? fillet_draft_->mode() ==
                  application::EdgeFeatureDraftMode::edit
            : chamfer_draft_->mode() ==
                  application::EdgeFeatureDraftMode::edit;
    if (editing &&
        !edge_feature_unresolved_edit_edges_.empty()) {
        edges.insert(
            edges.end(),
            edge_feature_unresolved_edit_edges_.begin(),
            edge_feature_unresolved_edit_edges_.end());
    }

    const bool applied =
        fillet_draft_
            ? fillet_draft_->setEdges(
                  std::move(edges))
            : chamfer_draft_->setEdges(
                  std::move(edges));
    if (!applied) {
        syncing_edge_feature_selection_ = true;
        const auto restored =
            viewport_controller_->
                restoreMaterialEdgeToolSelection(
                    fillet_draft_
                        ? fillet_draft_->edges()
                        : chamfer_draft_->edges());
        if (restored) {
            edge_feature_unresolved_edit_edges_ =
                *restored;
        }
        syncing_edge_feature_selection_ = false;
        setStatusText(
            QStringLiteral(
                "Edge selection is stale, duplicated or not valid for the current consumed Body stage. Clear unresolved intent before replacement."));
        return;
    }

    edge_feature_evaluation_.reset();
    refreshEdgeFeaturePreview();
    notifyCadInputContextChanged();
}

bool CadWorkbench::setEdgeFeatureParameter(
    core::LengthValue parameter,
    std::optional<std::string_view> display_text,
    bool refresh_now) {
    if ((!fillet_draft_ &&
         !chamfer_draft_) ||
        !parameter.finite() ||
        !(parameter.millimetres > 0.0)) {
        return false;
    }

    const bool applied =
        fillet_draft_
            ? fillet_draft_->setRadius(parameter)
            : chamfer_draft_->setDistance(parameter);
    if (!applied) {
        return false;
    }

    edge_feature_parameter_input_valid_ = true;
    if (display_text &&
        edge_feature_parameter_edit_ != nullptr) {
        const QSignalBlocker blocked{
            edge_feature_parameter_edit_};
        edge_feature_parameter_edit_->setText(
            fromUtf8(*display_text));
    }

    edge_feature_evaluation_.reset();
    if (refresh_now) {
        refreshEdgeFeaturePreview();
    } else {
        scheduleEdgeFeaturePreview();
        syncEdgeFeatureUi();
    }
    notifyCadInputContextChanged();
    return true;
}

void CadWorkbench::scheduleEdgeFeaturePreview() {
    if (edge_feature_preview_timer_ == nullptr) {
        refreshEdgeFeaturePreview();
        return;
    }
    edge_feature_preview_timer_->start();
}

void CadWorkbench::flushEdgeFeaturePreview() {
    if (edge_feature_preview_timer_ == nullptr ||
        !edge_feature_preview_timer_->isActive()) {
        return;
    }
    edge_feature_preview_timer_->stop();
    refreshEdgeFeaturePreview();
}

void CadWorkbench::refreshEdgeFeaturePreview() {
    if (edge_feature_preview_timer_ != nullptr) {
        edge_feature_preview_timer_->stop();
    }
    edge_feature_evaluation_.reset();
    if (viewport_controller_ != nullptr) {
        viewport_controller_->clearSolidPreview();
    }

    if ((!fillet_draft_ &&
         !chamfer_draft_) ||
        !edge_feature_parameter_input_valid_ ||
        document_session_ == nullptr ||
        solid_modeling_kernel_ == nullptr) {
        syncEdgeFeatureUi();
        return;
    }

    auto evaluation =
        fillet_draft_
            ? document_session_->evaluateFilletDraft(
                  *fillet_draft_,
                  *solid_modeling_kernel_)
            : document_session_->evaluateChamferDraft(
                  *chamfer_draft_,
                  *solid_modeling_kernel_);

    if (evaluation.previewSolidAvailable() &&
        viewport_controller_ != nullptr) {
        // One generation-scoped, atomic local removed/orange and
        // added/blue preview; the unchanged Body stays neutral.
        if (!viewport_controller_->setSolidMaterialDeltaPreview(
                evaluation.preview_mesh,
                evaluation.preview_added_mesh)) {
            evaluation.preview_mesh.reset();
            evaluation.preview_added_mesh.reset();
        }
    }

    edge_feature_evaluation_ =
        std::move(evaluation);
    syncEdgeFeatureUi();
}

void CadWorkbench::syncEdgeFeatureUi() {
    const bool active =
        fillet_draft_.has_value() ||
        chamfer_draft_.has_value();
    if (edge_feature_operations_widget_ != nullptr) {
        edge_feature_operations_widget_->setVisible(
            active);
    }
    if (!active) {
        return;
    }

    syncing_edge_feature_ui_ = true;
    const bool fillet =
        fillet_draft_.has_value();
    const auto edge_count =
        fillet
            ? fillet_draft_->edges().size()
            : chamfer_draft_->edges().size();
    const bool editing =
        fillet
            ? fillet_draft_->mode() ==
                  application::EdgeFeatureDraftMode::edit
            : chamfer_draft_->mode() ==
                  application::EdgeFeatureDraftMode::edit;

    if (edge_feature_title_label_ != nullptr) {
        edge_feature_title_label_->setText(
            editing
                ? (fillet
                       ? QStringLiteral("EDIT FILLET")
                       : QStringLiteral("EDIT CHAMFER"))
                : (fillet
                       ? QStringLiteral("FILLET")
                       : QStringLiteral("CHAMFER")));
    }
    if (edge_feature_selection_label_ != nullptr) {
        edge_feature_selection_label_->setText(
            edge_feature_unresolved_edit_edges_.empty()
                ? QStringLiteral("Selected edges: %1")
                      .arg(
                          static_cast<qulonglong>(
                              edge_count))
                : QStringLiteral(
                      "Selected edges: %1 · unresolved: %2")
                      .arg(
                          static_cast<qulonglong>(
                              edge_count))
                      .arg(
                          static_cast<qulonglong>(
                              edge_feature_unresolved_edit_edges_
                                  .size())));
    }
    if (edge_feature_parameter_name_label_ != nullptr) {
        edge_feature_parameter_name_label_->setText(
            fillet
                ? QStringLiteral("Radius")
                : QStringLiteral("Distance"));
    }
    if (edge_feature_finish_button_ != nullptr) {
        edge_feature_finish_button_->setText(
            editing
                ? (fillet
                       ? QStringLiteral("Finish Edit Fillet")
                       : QStringLiteral("Finish Edit Chamfer"))
                : (fillet
                       ? QStringLiteral("Finish Fillet")
                       : QStringLiteral("Finish Chamfer")));
        edge_feature_finish_button_->setEnabled(
            edge_feature_parameter_input_valid_ &&
            edge_feature_evaluation_ &&
            edge_feature_evaluation_->committable());
    }
    if (edge_feature_clear_button_ != nullptr) {
        edge_feature_clear_button_->setEnabled(
            edge_count != 0U);
    }
    if (edge_feature_result_label_ != nullptr) {
        if (!edge_feature_parameter_input_valid_) {
            edge_feature_result_label_->setText(
                QStringLiteral(
                    "%1 must be a positive length.")
                    .arg(
                        fillet
                            ? QStringLiteral("Radius")
                            : QStringLiteral("Distance")));
        } else if (edge_feature_evaluation_) {
            edge_feature_result_label_->setText(
                edgeFeatureEvaluationText(
                    *edge_feature_evaluation_,
                    fillet
                        ? QStringView{
                              u"Fillet"}
                        : QStringView{
                              u"Chamfer"}));
        } else {
            edge_feature_result_label_->setText(
                QStringLiteral(
                    "Select one or more current material Edges and enter a positive %1.")
                    .arg(
                        fillet
                            ? QStringLiteral("Radius")
                            : QStringLiteral("Distance")));
        }
    }
    if (operations_placeholder_ != nullptr) {
        operations_placeholder_->setText(
            QStringLiteral("%1 — %2 edge(s)")
                .arg(
                    fillet
                        ? QStringLiteral("Fillet")
                        : QStringLiteral("Chamfer"))
                .arg(
                    static_cast<qulonglong>(
                        edge_count)));
    }

    syncing_edge_feature_ui_ = false;
}

application::CadInputSubmitResult
CadWorkbench::submitEdgeFeatureCadInput(
    std::string_view text) {
    if ((!fillet_draft_ &&
         !chamfer_draft_) ||
        document_session_ == nullptr) {
        return {
            false,
            "No active Fillet/Chamfer draft."};
    }

    const bool fillet =
        fillet_draft_.has_value();
    const auto keyword =
        upperAsciiTrimmed(text);
    if (keyword == "CANCEL" ||
        keyword == "ESC") {
        cancelEdgeFeature();
        return {true, {}};
    }
    if ((fillet && keyword == "FILLET") ||
        (!fillet && keyword == "CHAMFER")) {
        return {true, {}};
    }
    if (keyword == "CLEAR") {
        clearEdgeFeatureSelection();
        return {true, {}};
    }
    if (keyword == "REMOVE") {
        if (viewport_controller_ == nullptr ||
            !viewport_controller_->
                 removePrimaryBodyTopologyToolSelection()) {
            return {
                false,
                "REMOVE requires at least one selected material Edge."};
        }
        tryStageEdgeFeatureSelection();
        return {true, {}};
    }
    if (keyword.empty() ||
        keyword == "FINISH") {
        return finishEdgeFeature()
            ? application::CadInputSubmitResult{
                  true, {}}
            : application::CadInputSubmitResult{
                  false,
                  "Edge Feature Finish was rejected."};
    }

    const auto parsed =
        application::parseBareCadDistance(
            text,
            application::CadInputNumberFormat{
                toUtf8(
                    QLocale{}.decimalPoint()),
                document_session_->document()
                    .lengthUnit()});
    if (!parsed || !(*parsed > 0.0)) {
        return {
            false,
            fillet
                ? "FILLET expects REMOVE, CLEAR, FINISH, CANCEL or a positive Radius."
                : "CHAMFER expects REMOVE, CLEAR, FINISH, CANCEL or a positive Distance."};
    }

    if (!setEdgeFeatureParameter(
            core::LengthValue{*parsed},
            text)) {
        return {
            false,
            "Edge Feature parameter could not be applied."};
    }
    return {true, {}};
}

bool CadWorkbench::startProjectEdgeTool() {
    if (project_edge_active_) {
        return true;
    }
    auto* session = activeDocumentSession();
    if (session == nullptr ||
        solid_modeling_kernel_ == nullptr ||
        viewport_controller_ == nullptr ||
        !active_sketch_id_ ||
        !sketch_interaction_controller_ ||
        !sketch_interaction_controller_->active() ||
        sketch_support_pick_active_ ||
        axis_draft_ || datum_plane_draft_ ||
        extrude_draft_ || revolve_draft_ ||
        fillet_draft_ || chamfer_draft_) {
        setStatusText(QStringLiteral(
            "PROJECT requires an active Part Sketch and exact Body provider."));
        return false;
    }

    // Select is the existing Sketch input baseline. The Viewer remains
    // the stage-scoped Edge picker and never authors Part geometry.
    sketch_interaction_controller_->activateSelect();
    const auto summary = viewport_controller_->bodyTopologySummary();
    if (!summary || summary->stage.kind !=
            part::BodyStageKind::after_feature ||
        summary->edges.referenceable == 0U) {
        setStatusText(QStringLiteral(
            "PROJECT needs a resolved source Body stage with material Edges."));
        return false;
    }

    project_edge_active_ = true;
    project_edge_revision_ = session->document().revision();
    project_edge_stage_ = summary->stage;
    project_edge_role_ = sketch::EntityRole::regular;
    project_edge_sources_.clear();
    project_edge_manual_sources_.clear();
    project_edge_face_sources_.clear();
    project_edge_face_skipped_.clear();
    project_edge_face_membership_.reset();
    project_edge_face_pick_.reset();
    project_edge_face_mode_ = false;
    project_edge_boundary_mode_ = false;
    project_edge_boundary_faces_.clear();
    project_edge_switching_mode_ = false;
    project_edge_preview_valid_ = false;
    viewport_controller_->clearSketchPreview();

    viewport_controller_->clearBodyTopologyToolSelection();
    viewport_controller_->setBodyTopologyEdgeDraftMode(true);
    // Sketch Select normally routes left clicks to spatial_tool_input,
    // bypassing the Viewer's Body picker entirely. PROJECT is a strict
    // stage-scoped material Edge acquisition tool, so it temporarily
    // owns presentation selection instead. All other Sketch tools keep
    // their accepted spatial routing policy.
    if (!viewport_controller_->setSketchPrimaryPointerRouting(
            viewer::PrimaryPointerRouting::
                presentation_selection)) {
        clearProjectEdgeRuntimeContext();
        setStatusText(QStringLiteral(
            "PROJECT could not acquire the Sketch Edge pick route."));
        return false;
    }
    syncSketchInteractionUi();
    syncProjectEdgeUi();
    syncActionState();
    notifyCadInputContextChanged();
    setStatusText(QStringLiteral(
        "PROJECT — pick current-stage material Edges; Regular/Construction, FINISH or CANCEL."));
    if (viewport_widget_ != nullptr) {
        viewport_widget_->setFocus(Qt::OtherFocusReason);
    }
    return true;
}

void CadWorkbench::clearProjectEdgeRuntimeContext() {
    if (!project_edge_active_) {
        return;
    }
    // Clear the tool before viewport callbacks can re-enter selection sync.
    project_edge_active_ = false;
    project_edge_face_mode_ = false;
    project_edge_boundary_mode_ = false;
    project_edge_boundary_faces_.clear();
    project_edge_switching_mode_ = false;
    project_edge_face_pick_.reset();
    project_edge_face_membership_.reset();
    project_edge_face_sources_.clear();
    project_edge_face_skipped_.clear();
    project_edge_manual_sources_.clear();
    project_edge_revision_.reset();
    project_edge_stage_.reset();
    project_edge_sources_.clear();
    project_edge_preview_valid_ = false;
    project_edge_role_ = sketch::EntityRole::regular;
    if (viewport_controller_ != nullptr) {
        viewport_controller_->clearSketchPreview();
        static_cast<void>(
            viewport_controller_->setProjectFaceSourceFeedback({}, {}));
        viewport_controller_->setBodyTopologyFacePickOnly(false);
        viewport_controller_->setBodyTopologyEdgeDraftMode(false);
        viewport_controller_->clearBodyTopologyToolSelection();
        // Return control to the existing Sketcher input grammar.
        // Do not activate Select here: a different Sketch tool may have
        // replaced PROJECT and must retain its own active tool identity.
        static_cast<void>(
            viewport_controller_->setSketchPrimaryPointerRouting(
                viewer::PrimaryPointerRouting::spatial_tool_input));
    }
    syncSketchInteractionUi();
    syncProjectEdgeUi();
    syncActionState();
    notifyCadInputContextChanged();
}

void CadWorkbench::escapeProjectEdgeTool() {
    if (!project_edge_active_) {
        return;
    }
    // One staged selection is an unfinished source-pick stage. The
    // first empty-buffer Esc discards it; only the next Esc exits.
    // Explicit CANCEL always exits immediately, in contrast.
    if (!project_edge_sources_.empty() ||
        project_edge_face_pick_ ||
        !project_edge_boundary_faces_.empty()) {
        clearProjectEdgeSelection();
        setStatusText(QStringLiteral(
            "PROJECT — Edge source selection cleared; Esc again to Cancel."));
        return;
    }
    cancelProjectEdgeTool();
}

void CadWorkbench::cancelProjectEdgeTool() {
    if (!project_edge_active_) {
        return;
    }
    clearProjectEdgeRuntimeContext();
    if (sketch_interaction_controller_ &&
        sketch_interaction_controller_->active()) {
        sketch_interaction_controller_->activateSelect();
    }
    setStatusText(QStringLiteral(
        "Project Geometry cancelled — no authored change."));
    if (viewport_widget_ != nullptr) {
        viewport_widget_->setFocus(Qt::OtherFocusReason);
    }
}

void CadWorkbench::clearProjectEdgeSelection() {
    if (!project_edge_active_ || !viewport_controller_) {
        return;
    }
    project_edge_sources_.clear();
    project_edge_manual_sources_.clear();
    project_edge_face_sources_.clear();
    project_edge_face_skipped_.clear();
    project_edge_face_membership_.reset();
    project_edge_face_pick_.reset();
    project_edge_boundary_faces_.clear();
    project_edge_preview_valid_ = false;
    viewport_controller_->clearSketchPreview();
    static_cast<void>(
        viewport_controller_->setProjectFaceSourceFeedback({}, {}));
    viewport_controller_->clearBodyTopologyToolSelection();
    syncProjectEdgeUi();
    notifyCadInputContextChanged();
}

void CadWorkbench::removeProjectFaceSelection() {
    if (!project_edge_active_ || !project_edge_face_mode_ ||
        !viewport_controller_ || !project_edge_face_pick_) {
        return;
    }
    project_edge_face_pick_.reset();
    project_edge_face_membership_.reset();
    project_edge_face_sources_.clear();
    project_edge_face_skipped_.clear();
    viewport_controller_->clearBodyTopologyToolSelection();
    if (!commitProjectBoundarySources()) {
        clearProjectEdgeSelection();
        setStatusText(QStringLiteral(
            "PROJECT Face removal found stale source stage; draft cleared."));
        return;
    }
    syncProjectEdgeUi();
    notifyCadInputContextChanged();
    setStatusText(QStringLiteral(
        "PROJECT Face removed; prior manual Edge sources preserved."));
}

bool CadWorkbench::commitProjectBoundarySources() {
    if (!viewport_controller_) return false;
    auto accepted = project_edge_manual_sources_;
    accepted.insert(
        accepted.end(), project_edge_face_sources_.begin(),
        project_edge_face_sources_.end());
    auto highlighted = project_edge_face_sources_;
    auto skipped = project_edge_face_skipped_;
    for (const auto& face : project_edge_boundary_faces_) {
        accepted.insert(
            accepted.end(), face.accepted.begin(), face.accepted.end());
        highlighted.insert(
            highlighted.end(), face.accepted.begin(), face.accepted.end());
        skipped.insert(
            skipped.end(), face.skipped.begin(), face.skipped.end());
    }
    const dedup = [](auto& edges) {
        std::sort(edges.begin(), edges.end());
        edges.erase(
            std::unique(edges.begin(), edges.end()), edges.end());
    };
    dedup(accepted);
    dedup(highlighted);
    dedup(skipped);
    if (std::any_of(
            skipped.begin(), skipped.end(),
            [&highlighted](const auto& source) {
                return std::binary_search(
                    highlighted.begin(), highlighted.end(), source);
            })) {
        return false;
    }
    // Test the proposed entire batch BEFORE publishing this state;
    // failure of a later Face must not erase previously staged Edges.
    if (!accepted.empty() &&
        !viewport_controller_->setProjectedEdgeDraftPreview(
            accepted, project_edge_role_)) {
        return false;
    }
    if (!viewport_controller_->setProjectFaceSourceFeedback(
            highlighted, skipped)) {
        return false;
    }
    project_edge_sources_ = std::move(accepted);
    project_edge_preview_valid_ =
        !project_edge_sources_.empty();
    if (!project_edge_preview_valid_) {
        viewport_controller_->clearSketchPreview();
    }
    syncProjectEdgeUi();
    notifyCadInputContextChanged();
    return true;
}

void CadWorkbench::removeProjectBoundarySelection() {
    if (!project_edge_active_ ||
        !project_edge_boundary_mode_ ||
        !viewport_controller_ ||
        project_edge_boundary_faces_.empty()) {
        return;
    }
    const auto selected =
        viewport_controller_->primaryBodyTopologySelection();
    auto found = selected
        ? std::find_if(
              project_edge_boundary_faces_.begin(),
              project_edge_boundary_faces_.end(),
              [&selected](const auto& face) {
                  return face.picked == *selected;
              })
        : project_edge_boundary_faces_.end();
    if (found == project_edge_boundary_faces_.end()) {
        found = std::prev(project_edge_boundary_faces_.end());
    }
    const auto saved = project_edge_boundary_faces_;
    project_edge_boundary_faces_.erase(found);
    if (!commitProjectBoundarySources()) {
        project_edge_boundary_faces_ = saved;
        static_cast<void>(commitProjectBoundarySources());
        setStatusText(QStringLiteral(
            "PROJECT Remove rejected: source stage or preview changed."));
        return;
    }
    viewport_controller_->clearBodyTopologyToolSelection();
    setStatusText(QStringLiteral(
        "PROJECT Face Boundary: chosen Face removed; other Faces and Edges retained."));
}

void CadWorkbench::refreshProjectEdgePreview() {
    project_edge_preview_valid_ = false;
    if (viewport_controller_ == nullptr) {
        return;
    }
    if (!project_edge_active_ ||
        project_edge_sources_.empty()) {
        viewport_controller_->clearSketchPreview();
        return;
    }
    project_edge_preview_valid_ =
        viewport_controller_->setProjectedEdgeDraftPreview(
            project_edge_sources_, project_edge_role_);
    if (!project_edge_preview_valid_) {
        setStatusText(QStringLiteral(
            "PROJECT preview unavailable: exact current Edge source, provider or Sketch support invalid; no authored geometry changed."));
    }
}

void CadWorkbench::setProjectEdgeFaceMode(bool enabled) {
    if (!project_edge_active_ || !viewport_controller_ ||
        (project_edge_face_mode_ == enabled &&
         !project_edge_boundary_mode_)) {
        return;
    }
    // Switching source acquisition must not fabricate an authored change.
    // Staged semantic Edges remain; only transient Viewer picks are swapped.
    project_edge_switching_mode_ = true;
    project_edge_face_mode_ = enabled;
    project_edge_boundary_mode_ = false;
    viewport_controller_->setBodyTopologyFacePickOnly(enabled);
    viewport_controller_->clearBodyTopologyToolSelection();
    if (!enabled && !project_edge_sources_.empty()) {
        const auto unresolved =
            viewport_controller_->restoreMaterialEdgeToolSelection(
                project_edge_sources_);
        if (!unresolved || !unresolved->empty()) {
            // Stale scene must never make old sources authoritative.
            project_edge_sources_.clear();
            project_edge_manual_sources_.clear();
            project_edge_face_sources_.clear();
            project_edge_face_skipped_.clear();
            project_edge_face_membership_.reset();
            project_edge_face_pick_.reset();
            project_edge_preview_valid_ = false;
            viewport_controller_->clearSketchPreview();
        }
    }
    project_edge_switching_mode_ = false;
    refreshProjectEdgePreview();
    syncProjectEdgeUi();
    notifyCadInputContextChanged();
    setStatusText(enabled
        ? QStringLiteral(
            "PROJECT Planar Face — select one current bounded planar Face; only geometric Unsupported curves may be skipped.")
        : QStringLiteral(
            "PROJECT Edges — staged Face members remain linked sources; pick more material Edges."));
}

void CadWorkbench::setProjectEdgeBoundaryMode() {
    if (!project_edge_active_ || !viewport_controller_ ||
        project_edge_boundary_mode_) {
        return;
    }
    project_edge_switching_mode_ = true;
    project_edge_boundary_mode_ = true;
    project_edge_face_mode_ = false;
    viewport_controller_->setBodyTopologyFacePickOnly(true);
    viewport_controller_->clearBodyTopologyToolSelection();
    project_edge_switching_mode_ = false;
    refreshProjectEdgePreview();
    syncProjectEdgeUi();
    notifyCadInputContextChanged();
    setStatusText(QStringLiteral(
        "PROJECT Face Boundary — click individual bounded Faces to add them; no automatic region traversal. REMOVE, CLEAR or FINISH."));
}

void CadWorkbench::tryStageProjectBoundarySelection() {
    if (!project_edge_active_ ||
        !project_edge_boundary_mode_ ||
        !document_session_ ||
        !project_edge_revision_ ||
        !project_edge_stage_ ||
        !viewport_controller_ ||
        document_session_->document().revision() !=
            *project_edge_revision_) {
        return;
    }
    const auto picked =
        viewport_controller_->primaryBodyTopologySelection();
    if (!picked || !picked->valid() ||
        picked->kind !=
            viewer::BodyTopologyPresentationKind::face) {
        return;
    }
    if (std::any_of(
            project_edge_boundary_faces_.begin(),
            project_edge_boundary_faces_.end(),
            [&picked](const auto& face) {
                return face.picked == *picked;
            })) {
        setStatusText(QStringLiteral(
            "PROJECT Face Boundary: this exact Face is already staged."));
        return;
    }
    const auto admission =
        viewport_controller_->inspectCurrentSelectedFaceBoundary(
            *picked);
    if (!admission.ok()) {
        setStatusText(QStringLiteral(
            "PROJECT Face Boundary rejected: scoped Face, native wire or material Edge identity unavailable; existing draft retained."));
        return;
    }
    std::vector<part::MaterialEdgeReference> accepted;
    std::vector<part::MaterialEdgeReference> skipped;
    std::size_t excluded = 0U;
    for (const auto& wire : admission.wires) {
        for (const auto& member : wire.edges) {
            if (member.excluded_nonmaterial) {
                ++excluded;
                continue;
            }
            if (!member.material ||
                member.material->stage != *project_edge_stage_) {
                setStatusText(QStringLiteral(
                    "PROJECT Face Boundary rejected: material Edge source stage invalid."));
                return;
            }
            const auto status =
                viewport_controller_->
                    currentMaterialEdgeProjectionStatus(
                        *member.material);
            if (status ==
                    part::ProjectedSketchSourceStatus::resolved) {
                accepted.push_back(*member.material);
            } else if (status ==
                           part::ProjectedSketchSourceStatus::
                               unsupported_projection ||
                       status ==
                           part::ProjectedSketchSourceStatus::
                               degenerate_projection) {
                skipped.push_back(*member.material);
            } else {
                setStatusText(QStringLiteral(
                    "PROJECT Face Boundary rejected: material identity, provider or projection integrity failure; prior selections retained."));
                return;
            }
        }
    }
    std::sort(accepted.begin(), accepted.end());
    accepted.erase(
        std::unique(accepted.begin(), accepted.end()),
        accepted.end());
    std::sort(skipped.begin(), skipped.end());
    skipped.erase(
        std::unique(skipped.begin(), skipped.end()),
        skipped.end());
    project_edge_boundary_faces_.push_back(
        ProjectBoundaryFaceDraft{
            *picked, admission,
            std::move(accepted), std::move(skipped)});
    if (!commitProjectBoundarySources()) {
        project_edge_boundary_faces_.pop_back();
        static_cast<void>(commitProjectBoundarySources());
        setStatusText(QStringLiteral(
            "PROJECT Face Boundary rejected: exact preview or source generation changed; earlier staging retained."));
        return;
    }
    setStatusText(QStringLiteral(
        "PROJECT Face Boundary: %1 Face(s), %2 eligible source Edge(s); %3 known nonmaterial use(s) excluded. Unsupported geometry skipped individually; open Sketch allowed.")
        .arg(static_cast<qulonglong>(
            project_edge_boundary_faces_.size()))
        .arg(static_cast<qulonglong>(
            project_edge_sources_.size()))
        .arg(static_cast<qulonglong>(excluded)));
}

void CadWorkbench::tryStageProjectFaceSelection() {
    if (!project_edge_active_ || !project_edge_face_mode_ ||
        !document_session_ || !project_edge_revision_ ||
        !project_edge_stage_ || !viewport_controller_ ||
        document_session_->document().revision() !=
            *project_edge_revision_) {
        return;
    }
    const auto picked =
        viewport_controller_->primaryBodyTopologySelection();
    if (!picked || !picked->valid() ||
        picked->kind !=
            viewer::BodyTopologyPresentationKind::face) {
        return;
    }
    const auto admitted =
        viewport_controller_->selectedMaterialFaceBoundaryAdmission();
    if (!admitted.ok()) {
        // No partial Face if topology/semantic identity itself is invalid.
        setStatusText(QStringLiteral(
            "PROJECT Face rejected: bounded Face or material boundary identity unavailable; previous staging preserved."));
        return;
    }
    std::vector<part::MaterialEdgeReference> accepted;
    std::vector<part::MaterialEdgeReference> skipped;
    for (const auto& wire : admitted.wires) {
        for (const auto& member : wire.edges) {
            if (member.reference.stage != *project_edge_stage_) {
                setStatusText(QStringLiteral(
                    "PROJECT Face rejected: wrong source stage."));
                return;
            }
            const auto status =
                viewport_controller_->currentMaterialEdgeProjectionStatus(
                    member.reference);
            if (status ==
                    part::ProjectedSketchSourceStatus::resolved) {
                accepted.push_back(member.reference);
            } else if (status ==
                       part::ProjectedSketchSourceStatus::
                           unsupported_projection) {
                skipped.push_back(member.reference);
            } else {
                // Missing, Ambiguous, provider failure, degenerate and stale
                // are not licensed per-Edge skip outcomes.
                setStatusText(QStringLiteral(
                    "PROJECT Face rejected: unresolved source, degenerate image or provider failure; no partial commit."));
                return;
            }
        }
    }
    std::sort(accepted.begin(), accepted.end());
    accepted.erase(
        std::unique(accepted.begin(), accepted.end()),
        accepted.end());
    std::sort(skipped.begin(), skipped.end());
    skipped.erase(
        std::unique(skipped.begin(), skipped.end()),
        skipped.end());

    auto combined = project_edge_manual_sources_;
    combined.insert(
        combined.end(), accepted.begin(), accepted.end());
    auto highlighted = accepted;
    auto highlighted_skipped = skipped;
    for (const auto& face : project_edge_boundary_faces_) {
        combined.insert(
            combined.end(), face.accepted.begin(), face.accepted.end());
        highlighted.insert(
            highlighted.end(), face.accepted.begin(),
            face.accepted.end());
        highlighted_skipped.insert(
            highlighted_skipped.end(), face.skipped.begin(),
            face.skipped.end());
    }
    std::sort(highlighted.begin(), highlighted.end());
    highlighted.erase(
        std::unique(highlighted.begin(), highlighted.end()),
        highlighted.end());
    std::sort(
        highlighted_skipped.begin(), highlighted_skipped.end());
    highlighted_skipped.erase(
        std::unique(
            highlighted_skipped.begin(), highlighted_skipped.end()),
        highlighted_skipped.end());
    std::sort(combined.begin(), combined.end());
    combined.erase(
        std::unique(combined.begin(), combined.end()),
        combined.end());
    if (!combined.empty() &&
        !viewport_controller_->setProjectedEdgeDraftPreview(
            combined, project_edge_role_)) {
        refreshProjectEdgePreview();
        setStatusText(QStringLiteral(
            "PROJECT Face preview rejected; previously staged sources preserved."));
        return;
    }

    if (!viewport_controller_->setProjectFaceSourceFeedback(
            highlighted, highlighted_skipped)) {
        refreshProjectEdgePreview();
        setStatusText(QStringLiteral(
            "PROJECT Face rejected: stale source overlay stage/generation; previous staging preserved."));
        return;
    }

    project_edge_face_pick_ = *picked;
    project_edge_face_membership_ = admitted;
    project_edge_face_sources_ = std::move(accepted);
    project_edge_face_skipped_ = std::move(skipped);
    project_edge_sources_ = std::move(combined);
    project_edge_preview_valid_ = !project_edge_sources_.empty();
    if (!project_edge_preview_valid_) {
        viewport_controller_->clearSketchPreview();
    }
    syncProjectEdgeUi();
    notifyCadInputContextChanged();
    setStatusText(project_edge_face_skipped_.empty()
        ? QStringLiteral("PROJECT Face staged: exact material outer/hole members.")
        : QStringLiteral(
            "PROJECT Face PARTIAL: unsupported geometric members skipped; open contour may not form a Profile."));
}

void CadWorkbench::tryStageProjectEdgeSelection() {
    if (project_edge_switching_mode_) return;
    if (project_edge_boundary_mode_) {
        tryStageProjectBoundarySelection();
        return;
    }
    if (project_edge_face_mode_) {
        tryStageProjectFaceSelection();
        return;
    }
    if (!project_edge_active_ ||
        !document_session_ ||
        !project_edge_revision_ ||
        !project_edge_stage_ ||
        !viewport_controller_) {
        return;
    }
    if (document_session_->document().revision() !=
            *project_edge_revision_) {
        clearProjectEdgeSelection();
        setStatusText(QStringLiteral(
            "PROJECT source selection stale after Document revision change."));
        return;
    }
    const auto selected =
        viewport_controller_->selectedMaterialEdgeReferences();
    if (selected) {
        const bool stage_matches = std::all_of(
            selected->begin(), selected->end(),
            [this](const part::MaterialEdgeReference& source) {
                return source.valid() &&
                    source.stage == *project_edge_stage_;
            });
        if (!stage_matches) {
            setStatusText(QStringLiteral(
                "PROJECT Edge belongs to another source Body stage."));
            return;
        }
        // Reject only the new unsupported pick. Previously staged,
        // exact material references and their preview remain usable.
        // Do not partially commit an invalid command batch.
        if (*selected != project_edge_sources_ &&
            !selected->empty()) {
            if (!viewport_controller_->setProjectedEdgeDraftPreview(
                    *selected, project_edge_role_)) {
                const auto prior = project_edge_sources_;
                const auto restored =
                    viewport_controller_->
                        restoreMaterialEdgeToolSelection(prior);
                if (!restored || !restored->empty()) {
                    // Stage/generation loss is not a reason to
                    // resurrect a last-good preview.
                    clearProjectEdgeSelection();
                    setStatusText(QStringLiteral(
                        "PROJECT source changed; selection cleared."));
                    return;
                }
                // Restoring the Viewer's tokens may notify this tool;
                // the same exact prior references remain authoritative.
                project_edge_sources_ = prior;
                refreshProjectEdgePreview();
                syncProjectEdgeUi();
                notifyCadInputContextChanged();
                setStatusText(QStringLiteral(
                    "PROJECT unsupported or unresolved Edge rejected; previous valid sources preserved."));
                return;
            }
            project_edge_sources_ = *selected;
            project_edge_preview_valid_ = true;
        } else {
            project_edge_sources_ = *selected;
            refreshProjectEdgePreview();
        }
    } else if (viewport_controller_->bodyTopologySelection().empty()) {
        project_edge_sources_.clear();
        refreshProjectEdgePreview();
    } else {
        setStatusText(QStringLiteral(
            "PROJECT selection contains unsupported or stale Body topology."));
        return;
    }
    // Current Edge selection is the complete batch shown by the Viewer.
    // If one Face member was removed in Edge mode, the Face gesture is no
    // longer complete: drop its transient membership instead of persisting
    // a misleading partially selected Face identity.
    const bool retained_face =
        project_edge_face_pick_ &&
        std::includes(
            project_edge_sources_.begin(),
            project_edge_sources_.end(),
            project_edge_face_sources_.begin(),
            project_edge_face_sources_.end());
    if (!retained_face) {
        project_edge_face_pick_.reset();
        project_edge_face_membership_.reset();
        project_edge_face_sources_.clear();
        project_edge_face_skipped_.clear();
    }
    // Manual Edge editing may remove members of previously clicked
    // Faces. Never retain a partial Face gesture with stale membership;
    // drop only the affected transient Face, preserving other picks.
    std::erase_if(
        project_edge_boundary_faces_,
        [this](const auto& face) {
            return !std::includes(
                project_edge_sources_.begin(),
                project_edge_sources_.end(),
                face.accepted.begin(), face.accepted.end());
        });
    auto face_sources = project_edge_face_sources_;
    for (const auto& face : project_edge_boundary_faces_) {
        face_sources.insert(
            face_sources.end(),
            face.accepted.begin(), face.accepted.end());
    }
    std::sort(face_sources.begin(), face_sources.end());
    face_sources.erase(
        std::unique(face_sources.begin(), face_sources.end()),
        face_sources.end());
    project_edge_manual_sources_.clear();
    std::set_difference(
        project_edge_sources_.begin(),
        project_edge_sources_.end(),
        face_sources.begin(),
        face_sources.end(),
        std::back_inserter(project_edge_manual_sources_));
    if (!commitProjectBoundarySources()) {
        clearProjectEdgeSelection();
        setStatusText(QStringLiteral(
            "PROJECT Edge draft became stale; selection cleared."));
        return;
    }
    syncProjectEdgeUi();
    notifyCadInputContextChanged();
}

bool CadWorkbench::finishProjectEdgeTool() {
    auto* session = activeDocumentSession();
    if (!project_edge_active_ || session == nullptr ||
        solid_modeling_kernel_ == nullptr ||
        !project_edge_revision_ || !project_edge_stage_ ||
        !active_sketch_id_ ||
        !sketch_edit_document_id_ ||
        *sketch_edit_document_id_ != session->documentId() ||
        session->document().revision() !=
            *project_edge_revision_ ||
        project_edge_sources_.empty() ||
        !project_edge_preview_valid_) {
        setStatusText(QStringLiteral(
            "PROJECT Finish rejected: selection empty or Sketch/Document context stale."));
        return false;
    }
    if (!viewport_controller_ ||
        !std::all_of(
            project_edge_sources_.begin(),
            project_edge_sources_.end(),
            [this](const part::MaterialEdgeReference& source) {
                return source.valid() &&
                    source.stage == *project_edge_stage_;
            })) {
        setStatusText(QStringLiteral(
            "PROJECT Finish rejected: invalid current Edge source stage."));
        return false;
    }
    if (project_edge_face_pick_) {
        if (!project_edge_face_membership_) {
            setStatusText(QStringLiteral(
                "PROJECT Face draft has no verified membership."));
            return false;
        }
        const auto fresh =
            viewport_controller_->inspectCurrentMaterialFaceBoundary(
                *project_edge_face_pick_);
        const auto& prior = *project_edge_face_membership_;
        bool same = fresh.ok() &&
            fresh.bounded_face == prior.bounded_face &&
            fresh.wires.size() == prior.wires.size();
        if (same) {
            for (std::size_t i = 0U;
                 i < fresh.wires.size() && same; ++i) {
                const auto& lhs = fresh.wires[i];
                const auto& rhs = prior.wires[i];
                same = lhs.outer == rhs.outer &&
                    lhs.edges.size() == rhs.edges.size();
                for (std::size_t j = 0U;
                     j < lhs.edges.size() && same; ++j) {
                    same =
                        lhs.edges[j].current_edge ==
                            rhs.edges[j].current_edge &&
                        lhs.edges[j].reference ==
                            rhs.edges[j].reference &&
                        lhs.edges[j].reversed ==
                            rhs.edges[j].reversed;
                }
            }
        }
        if (!same) {
            setStatusText(QStringLiteral(
                "PROJECT Finish rejected: Face membership or provider generation changed since preview."));
            return false;
        }
        std::vector<part::MaterialEdgeReference> accepted;
        std::vector<part::MaterialEdgeReference> skipped;
        for (const auto& wire : fresh.wires) {
            for (const auto& member : wire.edges) {
                const auto status =
                    viewport_controller_->
                        currentMaterialEdgeProjectionStatus(
                            member.reference);
                if (status ==
                        part::ProjectedSketchSourceStatus::resolved) {
                    accepted.push_back(member.reference);
                } else if (status ==
                           part::ProjectedSketchSourceStatus::
                               unsupported_projection) {
                    skipped.push_back(member.reference);
                } else {
                    setStatusText(QStringLiteral(
                        "PROJECT Finish rejected: Face source resolution or geometric skip status changed."));
                    return false;
                }
            }
        }
        std::sort(accepted.begin(), accepted.end());
        accepted.erase(
            std::unique(accepted.begin(), accepted.end()),
            accepted.end());
        std::sort(skipped.begin(), skipped.end());
        skipped.erase(
            std::unique(skipped.begin(), skipped.end()),
            skipped.end());
        if (accepted != project_edge_face_sources_ ||
            skipped != project_edge_face_skipped_) {
            setStatusText(QStringLiteral(
                "PROJECT Finish rejected: Face supported/skipped membership changed."));
            return false;
        }
    }

    // In multi-Face mode the Viewer only contains the current/last
    // click. Every earlier picked Face is instead re-bound and
    // revalidated using its exact current generation-bound address.
    for (const auto& face : project_edge_boundary_faces_) {
        const auto fresh =
            viewport_controller_->inspectCurrentSelectedFaceBoundary(
                face.picked);
        if (!fresh.ok() || fresh != face.membership) {
            setStatusText(QStringLiteral(
                "PROJECT Finish rejected: one selected Face's current native wire/material membership changed."));
            return false;
        }
        std::vector<part::MaterialEdgeReference> accepted;
        std::vector<part::MaterialEdgeReference> skipped;
        for (const auto& wire : fresh.wires) {
            for (const auto& member : wire.edges) {
                if (member.excluded_nonmaterial) continue;
                if (!member.material ||
                    member.material->stage !=
                        *project_edge_stage_) {
                    setStatusText(QStringLiteral(
                        "PROJECT Finish rejected: invalid selected Face material stage."));
                    return false;
                }
                const auto status =
                    viewport_controller_->
                        currentMaterialEdgeProjectionStatus(
                            *member.material);
                if (status ==
                        part::ProjectedSketchSourceStatus::resolved) {
                    accepted.push_back(*member.material);
                } else if (status ==
                               part::ProjectedSketchSourceStatus::
                                   unsupported_projection ||
                           status ==
                               part::ProjectedSketchSourceStatus::
                                   degenerate_projection) {
                    skipped.push_back(*member.material);
                } else {
                    setStatusText(QStringLiteral(
                        "PROJECT Finish rejected: selected Face source geometry/identity status changed."));
                    return false;
                }
            }
        }
        std::sort(accepted.begin(), accepted.end());
        accepted.erase(
            std::unique(accepted.begin(), accepted.end()),
            accepted.end());
        std::sort(skipped.begin(), skipped.end());
        skipped.erase(
            std::unique(skipped.begin(), skipped.end()),
            skipped.end());
        if (accepted != face.accepted ||
            skipped != face.skipped) {
            setStatusText(QStringLiteral(
                "PROJECT Finish rejected: selected Face PARTIAL membership changed."));
            return false;
        }
    }
    auto expected = project_edge_manual_sources_;
    expected.insert(
        expected.end(),
        project_edge_face_sources_.begin(),
        project_edge_face_sources_.end());
    for (const auto& face : project_edge_boundary_faces_) {
        expected.insert(
            expected.end(), face.accepted.begin(),
            face.accepted.end());
    }
    std::sort(expected.begin(), expected.end());
    expected.erase(
        std::unique(expected.begin(), expected.end()),
        expected.end());
    if (expected != project_edge_sources_) {
        setStatusText(QStringLiteral(
            "PROJECT Finish rejected: staged source batch no longer matches selected Faces."));
        return false;
    }

    if (project_edge_face_mode_) {
        const auto selected =
            viewport_controller_->primaryBodyTopologySelection();
        if (project_edge_face_pick_ &&
            (!selected || *selected != *project_edge_face_pick_)) {
            setStatusText(QStringLiteral(
                "PROJECT Finish rejected: selected Face changed."));
            return false;
        }
    } else if (!project_edge_boundary_mode_) {
        const auto selected =
            viewport_controller_->selectedMaterialEdgeReferences();
        if (!selected || *selected != project_edge_sources_) {
            setStatusText(QStringLiteral(
                "PROJECT Finish rejected: selected Edge stage or generation changed."));
            return false;
        }
    }

    const auto result = session->execute(
        application::CreateProjectedSketchEdgesCommand{
            *active_sketch_id_,
            *project_edge_revision_,
            project_edge_sources_,
            project_edge_role_},
        *solid_modeling_kernel_);
    if (!result.ok()) {
        setStatusText(result.diagnostic.message.empty()
            ? QStringLiteral(
                "PROJECT rejected an unresolved, invalid or cyclic source Edge; no batch committed.")
            : fromUtf8(result.diagnostic.message));
        return false;
    }

    const auto count = result.entity_ids.size();
    clearProjectEdgeRuntimeContext();
    refreshActiveContext();
    if (sketch_interaction_controller_ &&
        sketch_interaction_controller_->active()) {
        sketch_interaction_controller_->activateSelect();
    }
    setStatusText(
        QStringLiteral("Project Geometry finished — %1 linked Edges.")
            .arg(static_cast<qulonglong>(count)));
    if (viewport_widget_ != nullptr) {
        viewport_widget_->setFocus(Qt::OtherFocusReason);
    }
    return true;
}

void CadWorkbench::setProjectEdgeRole(sketch::EntityRole role) {
    if (!project_edge_active_ ||
        (role != sketch::EntityRole::regular &&
         role != sketch::EntityRole::construction)) {
        return;
    }
    project_edge_role_ = role;
    refreshProjectEdgePreview();
    syncProjectEdgeUi();
    notifyCadInputContextChanged();
}

void CadWorkbench::syncProjectEdgeUi() {
    if (project_edge_operations_widget_ != nullptr) {
        project_edge_operations_widget_->setVisible(project_edge_active_);
    }
    if (project_edge_button_ != nullptr) {
        project_edge_button_->setChecked(project_edge_active_);
    }
    if (!project_edge_active_) {
        return;
    }
    const auto count = project_edge_sources_.size();
    if (project_edge_edges_button_ != nullptr) {
        project_edge_edges_button_->setChecked(
            !project_edge_face_mode_ &&
            !project_edge_boundary_mode_);
    }
    if (project_edge_face_button_ != nullptr) {
        project_edge_face_button_->setChecked(
            project_edge_face_mode_);
    }
    if (project_edge_boundary_button_ != nullptr) {
        project_edge_boundary_button_->setChecked(
            project_edge_boundary_mode_);
    }
    if (project_edge_stage_label_ != nullptr) {
        project_edge_stage_label_->setText(
            project_edge_stage_ && project_edge_stage_->feature_id
                ? QStringLiteral("Source stage: after Feature %1")
                      .arg(fromUtf8(
                          project_edge_stage_->feature_id->serialized()))
                : QStringLiteral("Source stage: unavailable"));
    }
    if (project_edge_selection_label_ != nullptr) {
        std::size_t manual_face_wires = 0U;
        std::size_t total_skipped =
            project_edge_face_skipped_.size();
        std::size_t excluded_nonmaterial = 0U;
        for (const auto& face : project_edge_boundary_faces_) {
            manual_face_wires += face.membership.wires.size();
            total_skipped += face.skipped.size();
            for (const auto& wire : face.membership.wires) {
                for (const auto& member : wire.edges) {
                    if (member.excluded_nonmaterial) {
                        ++excluded_nonmaterial;
                    }
                }
            }
        }
        const auto face_wires =
            project_edge_face_membership_
                ? project_edge_face_membership_->wires.size()
                : 0U;
        project_edge_selection_label_->setText(
            QStringLiteral(
                "Material Edges selected: %1 | Face wires: %2 (Planar Face holes %3) | Unsupported skipped: %4 | Faces staged: %5 | Excluded native artifacts: %6")
                .arg(static_cast<qulonglong>(count))
                .arg(static_cast<qulonglong>(
                    face_wires + manual_face_wires))
                .arg(static_cast<qulonglong>(
                    face_wires == 0U ? 0U : face_wires - 1U))
                .arg(static_cast<qulonglong>(total_skipped))
                .arg(static_cast<qulonglong>(
                    project_edge_boundary_faces_.size()))
                .arg(static_cast<qulonglong>(
                    excluded_nonmaterial)));
    }
    if (project_edge_regular_button_ != nullptr) {
        project_edge_regular_button_->setChecked(
            project_edge_role_ == sketch::EntityRole::regular);
    }
    if (project_edge_construction_button_ != nullptr) {
        project_edge_construction_button_->setChecked(
            project_edge_role_ == sketch::EntityRole::construction);
    }
    if (project_edge_finish_button_ != nullptr) {
        project_edge_finish_button_->setEnabled(
            count > 0U && project_edge_preview_valid_ &&
            document_session_ != nullptr &&
            project_edge_revision_ &&
            document_session_->document().revision() ==
                *project_edge_revision_);
    }
    if (project_edge_remove_button_ != nullptr) {
        project_edge_remove_button_->setEnabled(
            project_edge_boundary_mode_
                ? !project_edge_boundary_faces_.empty()
                : project_edge_face_mode_
                    ? project_edge_face_pick_.has_value()
                    : count > 0U);
    }
    if (project_edge_clear_button_ != nullptr) {
        // A geometrically Unsupported-only Face may admit zero links,
        // but its transient draft must still be removable from the UI.
        project_edge_clear_button_->setEnabled(
            count > 0U || project_edge_face_pick_.has_value() ||
            !project_edge_boundary_faces_.empty());
    }
    if (project_edge_result_label_ != nullptr) {
        QString result_message =
            !project_edge_face_skipped_.empty()
                ? QStringLiteral(
                    "PARTIAL Face: %1 unsupported geometric Edge(s) skipped, without closing gaps. %2 supported linked Edge(s); an open contour may not create a Profile.")
                    .arg(static_cast<qulonglong>(
                        project_edge_face_skipped_.size()))
                    .arg(static_cast<qulonglong>(count))
                : count == 0U
                ? QStringLiteral(
                    "Pick one or more exact material Edges, then Finish; Cancel makes no changes.")
                : !project_edge_preview_valid_
                    ? QStringLiteral(
                        "Current source unresolved — exact projection preview unavailable; Finish disabled. Clear/Remove or Cancel.")
                    : QStringLiteral(
                        "Current preview: %1 derived Edge(s), %2. FINISH commits once; REMOVE/CLEAR or CANCEL.")
                        .arg(static_cast<qulonglong>(count))
                        .arg(project_edge_role_ ==
                                 sketch::EntityRole::regular
                                 ? QStringLiteral("Regular")
                                 : QStringLiteral("Construction"));

        // Source-by-source diagnostics are read from the one currently
        // staged native Face occurrence. Wire/order labels are transient
        // presentation aids, never authored Face/Edge identifiers.
        if (project_edge_face_membership_) {
            QStringList members;
            std::size_t hole_number = 0U;
            for (const auto& wire :
                 project_edge_face_membership_->wires) {
                if (!wire.outer) ++hole_number;
                for (std::size_t index = 0U;
                     index < wire.edges.size(); ++index) {
                    const auto& member = wire.edges[index];
                    const bool skipped = std::binary_search(
                        project_edge_face_skipped_.begin(),
                        project_edge_face_skipped_.end(),
                        member.reference);
                    const QString location = wire.outer
                        ? QStringLiteral("Outer Edge %1")
                              .arg(static_cast<qulonglong>(index + 1U))
                        : QStringLiteral("Hole %1 Edge %2")
                              .arg(static_cast<qulonglong>(hole_number))
                              .arg(static_cast<qulonglong>(index + 1U));
                    members.push_back(
                        QStringLiteral("%1: %2 (Feature %3)")
                            .arg(location)
                            .arg(skipped
                                ? QStringLiteral(
                                    "SKIPPED — geometric Unsupported")
                                : QStringLiteral("supported"))
                            .arg(fromUtf8(
                                member.reference.curve
                                    .producer_feature_id.serialized())));
                }
            }
            if (!members.empty()) {
                result_message += QStringLiteral("\n");
                result_message += members.join(
                    QStringLiteral("\n"));
            }
        }
        if (!project_edge_boundary_faces_.empty()) {
            QStringList notes;
            for (std::size_t i = 0U;
                 i < project_edge_boundary_faces_.size(); ++i) {
                const auto& face = project_edge_boundary_faces_[i];
                std::size_t excluded = 0U;
                for (const auto& wire : face.membership.wires) {
                    for (const auto& member : wire.edges) {
                        if (member.excluded_nonmaterial) ++excluded;
                    }
                }
                notes.push_back(
                    QStringLiteral(
                        "Face %1: %2 supported, %3 SKIPPED geometric Unsupported, %4 native seam/partition excluded")
                        .arg(static_cast<qulonglong>(i + 1U))
                        .arg(static_cast<qulonglong>(
                            face.accepted.size()))
                        .arg(static_cast<qulonglong>(
                            face.skipped.size()))
                        .arg(static_cast<qulonglong>(excluded)));
            }
            result_message += QStringLiteral("\n");
            result_message += notes.join(QStringLiteral("\n"));
            if (std::any_of(
                    project_edge_boundary_faces_.begin(),
                    project_edge_boundary_faces_.end(),
                    [](const auto& face) {
                        return !face.skipped.empty();
                    })) {
                result_message += QStringLiteral(
                    "\nPARTIAL Face Boundary: only supported material Edges will be authored; open or disconnected Sketch contours are allowed.");
            }
        }
        project_edge_result_label_->setText(result_message);
    }
    if (operations_placeholder_ != nullptr) {
        operations_placeholder_->setText(
            QStringLiteral("Project Geometry — %1 source Edge(s)")
                .arg(static_cast<qulonglong>(count)));
    }
}

application::CadInputSubmitResult
CadWorkbench::submitProjectEdgeCadInput(std::string_view text) {
    if (!project_edge_active_) {
        return {false, "No active Project Geometry tool."};
    }
    const auto keyword = upperAsciiTrimmed(text);
    if (keyword == "PROJECT" || keyword == "PROJECTGEOMETRY") {
        return {true, {}};
    }
    if (keyword == "ESC") {
        escapeProjectEdgeTool();
        return {true, {}};
    }
    if (keyword == "CANCEL") {
        cancelProjectEdgeTool();
        return {true, {}};
    }
    if (keyword == "EDGES" || keyword == "EDGE") {
        setProjectEdgeFaceMode(false);
        return {true, {}};
    }
    if (keyword == "FACE" ||
        keyword == "PLANARFACE" ||
        keyword == "PLANAR FACE") {
        setProjectEdgeFaceMode(true);
        return {true, {}};
    }
    if (keyword == "FACEBOUNDARY" ||
        keyword == "FACE BOUNDARY" ||
        keyword == "FACES") {
        setProjectEdgeBoundaryMode();
        return {true, {}};
    }
    if (keyword == "REGULAR") {
        setProjectEdgeRole(sketch::EntityRole::regular);
        return {true, {}};
    }
    if (keyword == "CONSTRUCTION") {
        setProjectEdgeRole(sketch::EntityRole::construction);
        return {true, {}};
    }
    if (keyword == "REMOVE") {
        if (project_edge_boundary_mode_) {
            if (project_edge_boundary_faces_.empty()) {
                return {false, "REMOVE requires a staged bounded Face."};
            }
            removeProjectBoundarySelection();
            return {true, {}};
        }
        if (project_edge_face_mode_) {
            if (!project_edge_face_pick_) {
                return {false, "REMOVE requires one staged planar Face."};
            }
            removeProjectFaceSelection();
            return {true, {}};
        }
        if (!viewport_controller_ ||
            !viewport_controller_->removePrimaryBodyTopologyToolSelection()) {
            return {false, "REMOVE requires one staged material Edge."};
        }
        tryStageProjectEdgeSelection();
        return {true, {}};
    }
    if (keyword == "CLEAR") {
        clearProjectEdgeSelection();
        return {true, {}};
    }
    if (keyword.empty() || keyword == "FINISH") {
        return finishProjectEdgeTool()
            ? application::CadInputSubmitResult{true, {}}
            : application::CadInputSubmitResult{
                false, "PROJECT Finish rejected; correct source selection or CANCEL."};
    }
    return {
        false,
        "PROJECT expects EDGES, FACE, FACEBOUNDARY, REGULAR, CONSTRUCTION, REMOVE, CLEAR, FINISH or CANCEL."};
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

void CadWorkbench::setSketchLineAxisDesignation(
    bool enabled) {
    if (!sketch_interaction_controller_ ||
        document_session_ == nullptr) {
        return;
    }

    if (sketch_interaction_controller_->tool() ==
        sketch::SketchTool::line) {
        if (!sketch_interaction_controller_->
                 setLineAxisDesignation(enabled)) {
            setStatusText(
                QStringLiteral(
                    "Axis designation is unavailable in the current Line context."));
            syncSketchInteractionUi();
            return;
        }
        setStatusText(
            enabled
                ? QStringLiteral(
                      "Axis armed for the next successfully committed Line only.")
                : QStringLiteral(
                      "Axis designation cleared for Line creation."));
        return;
    }

    if (sketch_interaction_controller_->tool() !=
            sketch::SketchTool::select ||
        sketch_interaction_controller_->
                selectedCount() != 1U ||
        !active_sketch_id_) {
        syncSketchInteractionUi();
        return;
    }

    const auto entity_id =
        sketch_interaction_controller_->
            selectedEntities().front();
    const auto* hosted =
        document_session_->document()
            .findSketch(*active_sketch_id_);
    if (hosted == nullptr ||
        hosted->model.findLine(entity_id) ==
            nullptr) {
        syncSketchInteractionUi();
        return;
    }

    const part::SketchLineAxisSource source{
        *active_sketch_id_,
        entity_id};
    std::vector<part::AxisId> matches;
    for (const auto& axis :
         document_session_->document().axes()) {
        if (axis.source == source) {
            matches.push_back(axis.id);
        }
    }

    if (matches.size() > 1U) {
        setStatusText(
            QStringLiteral(
                "Axis designation conflict — repair duplicate legacy Axis sources through Axis Tree/Properties."));
        syncSketchInteractionUi();
        return;
    }

    if (enabled) {
        if (!matches.empty()) {
            syncSketchInteractionUi();
            return;
        }
        if (solid_modeling_kernel_ == nullptr) {
            setStatusText(
                QStringLiteral(
                    "Axis creation requires the active modeling kernel."));
            syncSketchInteractionUi();
            return;
        }

        const auto result =
            document_session_->execute(
                application::CreateAxisCommand{
                    source,
                    document_session_->document()
                        .revision(),
                    {},
                    true},
                *solid_modeling_kernel_);
        if (!result.ok()) {
            showFailure(result.diagnostic);
            syncSketchInteractionUi();
            return;
        }

        refreshActiveContext();
        notifyDocumentStateChanged();
        setStatusText(
            QStringLiteral(
                "Selected Line designated as one authored Part Axis."));
        return;
    }

    if (matches.size() != 1U) {
        syncSketchInteractionUi();
        return;
    }

    const auto axis_id = matches.front();
    bool referenced_by_revolve = false;
    for (const auto& feature :
         document_session_->document()
             .body().features) {
        const auto* revolve =
            std::get_if<part::RevolveFeature>(
                &feature.definition);
        if (revolve == nullptr) {
            continue;
        }
        const auto referenced =
            part::authoredAxisIdForAxisReference(
                revolve->axis);
        if (referenced &&
            *referenced == axis_id) {
            referenced_by_revolve = true;
            break;
        }
    }

    if (referenced_by_revolve) {
        const auto answer =
            QMessageBox::question(
                this,
                QStringLiteral(
                    "Remove Axis designation"),
                QStringLiteral(
                    "This Axis is referenced by a Revolve. Removing the Axis keeps the Revolve authored but leaves its Axis intent Missing/Blocked until repaired.\n\nRemove the Axis designation?"),
                QMessageBox::Yes |
                    QMessageBox::No,
                QMessageBox::No);
        if (answer != QMessageBox::Yes) {
            syncSketchInteractionUi();
            return;
        }
    }

    const auto result =
        document_session_->execute(
            application::DeleteAxisCommand{
                axis_id,
                document_session_->document()
                    .revision()});
    if (!result.ok()) {
        showFailure(result.diagnostic);
        syncSketchInteractionUi();
        return;
    }

    if (selected_axis_id_ &&
        *selected_axis_id_ == axis_id) {
        selected_axis_id_.reset();
    }
    refreshActiveContext();
    notifyDocumentStateChanged();
    setStatusText(
        QStringLiteral(
            "Axis designation removed; the Sketch Line remains authored."));
}

void CadWorkbench::startSketchTool() {
    if (activeDocumentSession() == nullptr ||
        axis_draft_ ||
        datum_plane_draft_ ||
        sketch_support_pick_active_ ||
        extrude_profile_pick_active_ ||
        extrude_draft_ ||
        revolve_draft_ ||
        fillet_draft_ ||
        chamfer_draft_) {
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
        axis_draft_ ||
        datum_plane_draft_ ||
        sketch_support_pick_active_ ||
        extrude_profile_pick_active_ ||
        extrude_draft_ ||
        revolve_draft_ ||
        fillet_draft_ ||
        chamfer_draft_) {
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
    if (project_edge_active_) {
        cancelProjectEdgeTool();
    }
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

void CadWorkbench::breakSelectedProjectedEdgeLink() {
    if (project_edge_active_ ||
        !document_session_ ||
        !solid_modeling_kernel_ ||
        !active_sketch_id_ ||
        !sketch_interaction_controller_ ||
        !sketch_interaction_controller_->active() ||
        sketch_interaction_controller_->tool() !=
            sketch::SketchTool::select ||
        sketch_interaction_controller_->selectedCount() == 0U) {
        setStatusText(QStringLiteral(
            "Break Link requires one or more selected, currently resolvable linked Sketch Edges."));
        return;
    }

    const auto targets =
        sketch_interaction_controller_->selectedEntities();
    const auto* sketch =
        document_session_->document().findSketch(
            *active_sketch_id_);
    if (!sketch ||
        std::any_of(
            targets.begin(), targets.end(),
            [sketch](const sketch::EntityId& target) {
                return std::none_of(
                    sketch->projection_bindings.begin(),
                    sketch->projection_bindings.end(),
                    [&target](const part::ProjectedEdgeBinding& item) {
                        return item.target_entity == target;
                    });
            })) {
        setStatusText(QStringLiteral(
            "Break Link rejected: all selected entities must be linked Project Edges; no changes made."));
        return;
    }

    const auto result = document_session_->execute(
        application::BreakProjectedEdgeLinksCommand{
            *active_sketch_id_,
            targets,
            document_session_->document().revision()},
        *solid_modeling_kernel_);
    if (!result.ok()) {
        setStatusText(result.diagnostic.message.empty()
            ? QStringLiteral(
                "Break Link rejected: one or more exact current sources cannot be resolved; links preserved.")
            : fromUtf8(result.diagnostic.message));
        return;
    }

    refreshActiveContext();
    if (sketch_interaction_controller_ &&
        sketch_interaction_controller_->active()) {
        sketch_interaction_controller_->activateSelect();
    }
    setStatusText(QStringLiteral(
        "Break Link finished — selected current geometry retained, source associations removed in one Undo."));
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
    if (axis_draft_) {
        constexpr application::CadInputContextGeneration
            axis_namespace =
                application::CadInputContextGeneration{
                    1ULL << 59U};
        return axis_namespace |
               (axis_draft_->generation() &
                (axis_namespace - 1U));
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
    if (revolve_draft_) {
        constexpr application::CadInputContextGeneration
            revolve_namespace =
                application::CadInputContextGeneration{
                    1ULL << 58U};
        return revolve_namespace |
               (revolve_draft_->generation() &
                (revolve_namespace - 1U));
    }
    if (fillet_draft_) {
        constexpr application::CadInputContextGeneration
            fillet_namespace =
                application::CadInputContextGeneration{
                    1ULL << 56U};
        return fillet_namespace |
               (fillet_draft_->generation() &
                (fillet_namespace - 1U));
    }
    if (chamfer_draft_) {
        constexpr application::CadInputContextGeneration
            chamfer_namespace =
                application::CadInputContextGeneration{
                    1ULL << 57U};
        return chamfer_namespace |
               (chamfer_draft_->generation() &
                (chamfer_namespace - 1U));
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
    if (axis_draft_) {
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
    if (revolve_draft_) {
        return {
            application::CadDynamicInputField{
                application::
                    CadDynamicInputFieldSemantic::
                        angle,
                "Angle"}};
    }

    if (fillet_draft_ ||
        chamfer_draft_) {
        return {
            application::CadDynamicInputField{
                application::
                    CadDynamicInputFieldSemantic::
                        distance,
                fillet_draft_
                    ? "Radius"
                    : "Distance"}};
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

    if (revolve_draft_) {
        if (index != 0U ||
            document_session_ == nullptr) {
            return {
                false,
                "Revolve has one Angle input field."};
        }
        const auto quantity =
            application::parseCadQuantity(
                text,
                {
                    application::CadQuantityDimension::
                        angle,
                    document_session_->document()
                        .lengthUnit()});
        constexpr double full_turn =
            2.0 * std::numbers::pi_v<double>;
        if (!quantity ||
            !(quantity->canonical_value > 0.0) ||
            quantity->canonical_value > full_turn ||
            !setRevolveAngle(
                core::AngleValue{
                    quantity->canonical_value},
                text)) {
            return {
                false,
                "Revolve Angle expects 0 < angle <= 360 deg."};
        }
        return {true, {}};
    }

    if (fillet_draft_ ||
        chamfer_draft_) {
        if (index != 0U ||
            document_session_ == nullptr) {
            return {
                false,
                "Fillet/Chamfer has one length parameter field."};
        }
        const auto distance =
            application::parseBareCadDistance(
                text,
                application::CadInputNumberFormat{
                    toUtf8(
                        QLocale{}.decimalPoint()),
                    document_session_->document()
                        .lengthUnit()});
        if (!distance ||
            !(*distance > 0.0) ||
            !setEdgeFeatureParameter(
                core::LengthValue{*distance},
                text)) {
            return {
                false,
                fillet_draft_
                    ? "Fillet Radius expects a positive Length."
                    : "Chamfer Distance expects a positive Length."};
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
    if (project_edge_active_) {
        return submitProjectEdgeCadInput({});
    }
    if (axis_draft_) {
        return finishAxis()
            ? application::CadInputSubmitResult{
                  true, {}}
            : application::CadInputSubmitResult{
                  false,
                  "Axis Finish was rejected."};
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
    if (revolve_draft_) {
        return finishRevolve()
            ? application::CadInputSubmitResult{
                  true, {}}
            : application::CadInputSubmitResult{
                  false,
                  "Revolve Finish was rejected."};
    }

    if (fillet_draft_ ||
        chamfer_draft_) {
        return finishEdgeFeature()
            ? application::CadInputSubmitResult{
                  true, {}}
            : application::CadInputSubmitResult{
                  false,
                  "Edge Feature Finish was rejected."};
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
    if (project_edge_active_) {
        return submitProjectEdgeCadInput(text);
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
    if (revolve_draft_) {
        auto result =
            submitRevolveCadInput(text);
        if (!result.accepted &&
            status_ != nullptr &&
            !result.diagnostic.empty()) {
            setStatusText(
                fromUtf8(result.diagnostic));
        }
        return result;
    }

    if (fillet_draft_ ||
        chamfer_draft_) {
        auto result =
            submitEdgeFeatureCadInput(text);
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
    if (top_level_keyword == "PROJECT" ||
        top_level_keyword == "PROJECTGEOMETRY") {
        return startProjectEdgeTool()
            ? application::CadInputSubmitResult{true, {}}
            : application::CadInputSubmitResult{
                false, "PROJECT could not be activated in the active Sketch."};
    }
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
    if (top_level_keyword == "REVOLVE") {
        return startRevolveTool()
            ? application::CadInputSubmitResult{
                  true, {}}
            : application::CadInputSubmitResult{
                  false,
                  "REVOLVE could not be activated."};
    }
    if (top_level_keyword == "FILLET") {
        return startFilletTool()
            ? application::CadInputSubmitResult{
                  true, {}}
            : application::CadInputSubmitResult{
                  false,
                  "FILLET could not be activated."};
    }
    if (top_level_keyword == "CHAMFER") {
        return startChamferTool()
            ? application::CadInputSubmitResult{
                  true, {}}
            : application::CadInputSubmitResult{
                  false,
                  "CHAMFER could not be activated."};
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
    if (project_edge_active_) {
        return project_edge_sources_.empty()
            ? QStringLiteral(
                "Command: PROJECT — pick material Edges · REGULAR/CONSTRUCTION · REMOVE/CLEAR · FINISH/Enter · CANCEL/Esc")
            : QStringLiteral(
                "Command: PROJECT — Edges staged · Esc clears sources, next Esc cancels · FINISH/Enter · CANCEL");
    }
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
    if (revolve_draft_) {
        if (!revolve_draft_->profileId()) {
            return QStringLiteral(
                "Command: REVOLVE — Select one valid Profile · CANCEL/Esc");
        }
        if (!revolve_draft_->axis()) {
            return QStringLiteral(
                "Command: REVOLVE — Select X/Y/Z Origin Axis or one resolved authored Axis · X/Y/Z · CANCEL/Esc");
        }
        return QStringLiteral(
            "Command: REVOLVE — ADD/CUT · ONESIDE/MIDPLANE · REVERSE · Angle · FINISH/CANCEL");
    }

    if (fillet_draft_) {
        return QStringLiteral(
            "Command: FILLET — toggle material Edges · Radius · REMOVE/CLEAR · FINISH/CANCEL");
    }
    if (chamfer_draft_) {
        return QStringLiteral(
            "Command: CHAMFER — toggle material Edges · Distance · REMOVE/CLEAR · FINISH/CANCEL");
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
    clearProjectEdgeRuntimeContext();
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
    if (project_edge_active_) {
        cancelProjectEdgeTool();
    }
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
    if (project_edge_active_) {
        cancelProjectEdgeTool();
    }
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
    if (revolve_draft_) {
        refreshRevolvePreview();
    }
    if (fillet_draft_ ||
        chamfer_draft_) {
        refreshEdgeFeaturePreview();
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
    clearRevolveRuntimeContext();
    clearEdgeFeatureRuntimeContext();
    clearProjectEdgeRuntimeContext();
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

std::optional<part::ResolvedProfileRegion>
CadWorkbench::evaluateCurrentProfile(
    part::ProfileId profile_id) const {
    if (document_session_ == nullptr) {
        return std::nullopt;
    }
    if (viewport_controller_ != nullptr) {
        return viewport_controller_->
            currentProfileResolution(profile_id);
    }

    // No provider-backed viewport means no authority to resolve a
    // source-controlled Profile. Keep ordinary authored Profiles usable.
    const auto& document = document_session_->document();
    const auto* profile = document.findProfile(profile_id);
    if (profile == nullptr) {
        return std::nullopt;
    }
    const auto* source =
        document.findSketch(profile->source_sketch_id);
    return source != nullptr &&
            source->projection_bindings.empty()
        ? document.evaluateProfile(profile_id)
        : std::nullopt;
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
        evaluateCurrentProfile(profile_id);
    const bool valid =
        evaluation && evaluation->valid();
    profile_status_->setText(
        valid
            ? QStringLiteral("Valid")
            : QStringLiteral("Invalid"));

    QString diagnostic = evaluation
        ? QStringLiteral("—")
        : QStringLiteral("Current Profile source unavailable or stale");
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
    const auto* revolve =
        std::get_if<part::RevolveFeature>(
            &feature->definition);
    const auto* fillet =
        std::get_if<part::FilletFeature>(
            &feature->definition);
    const auto* chamfer =
        std::get_if<part::ChamferFeature>(
            &feature->definition);
    if (extrude == nullptr &&
        revolve == nullptr &&
        fillet == nullptr &&
        chamfer == nullptr) {
        return;
    }

    selected_feature_id_ = feature_id;
    feature_name_->setText(
        fromUtf8(feature->name));
    feature_identity_->setText(
        fromUtf8(feature->id.serialized()));

    const auto unit =
        document_session_->document()
            .lengthUnit();
    if (extrude != nullptr) {
        feature_operation_->setText(
            extrude->operation ==
                    part::ExtrudeOperation::cut
                ? QStringLiteral("Cut")
                : QStringLiteral("Add"));
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
        feature_edit_button_->setText(
            QStringLiteral("Edit Extrude"));
    } else if (revolve != nullptr) {
        feature_operation_->setText(
            revolve->operation ==
                    part::RevolveOperation::cut
                ? QStringLiteral("Cut")
                : QStringLiteral("Add"));
        if (const auto* one =
                std::get_if<
                    part::OneSidedRevolveExtent>(
                    &revolve->extent)) {
            feature_extent_->setText(
                QStringLiteral("One Side"));
            feature_distance_->setText(
                formatAngleForCad(
                    one->angle));
            feature_direction_->setText(
                one->reversed
                    ? QStringLiteral("Reverse")
                    : QStringLiteral("Forward"));
        } else if (const auto* midplane =
                       std::get_if<
                           part::MidplaneRevolveExtent>(
                           &revolve->extent)) {
            feature_extent_->setText(
                QStringLiteral("Midplane"));
            feature_distance_->setText(
                formatAngleForCad(
                    midplane->total_angle));
            feature_direction_->setText(
                QStringLiteral("Centered"));
        }
        feature_edit_button_->setText(
            QStringLiteral("Edit Revolve"));
    } else if (fillet != nullptr) {
        feature_operation_->setText(
            QStringLiteral("Fillet"));
        feature_extent_->setText(
            QStringLiteral("Explicit Edges: %1")
                .arg(
                    static_cast<qulonglong>(
                        fillet->edges.size())));
        feature_distance_->setText(
            formatLengthForPart(
                fillet->radius,
                unit));
        feature_direction_->setText(
            QStringLiteral("Constant Radius"));
        feature_edit_button_->setText(
            QStringLiteral("Edit Fillet"));
    } else {
        feature_operation_->setText(
            QStringLiteral("Chamfer"));
        feature_extent_->setText(
            QStringLiteral("Explicit Edges: %1")
                .arg(
                    static_cast<qulonglong>(
                        chamfer->edges.size())));
        feature_distance_->setText(
            formatLengthForPart(
                chamfer->distance,
                unit));
        feature_direction_->setText(
            QStringLiteral("Equal Distance"));
        feature_edit_button_->setText(
            QStringLiteral("Edit Chamfer"));
    }

    const auto source_profile =
        part::sourceProfileId(*feature);
    const part::ProfileId* source_profile_id =
        source_profile
            ? &*source_profile
            : nullptr;
    const auto* profile =
        source_profile_id != nullptr
            ? document_session_->document()
                  .findProfile(*source_profile_id)
            : nullptr;
    feature_source_profile_->setText(
        source_profile_id != nullptr
            ? fromUtf8(
                  source_profile_id->serialized())
            : QStringLiteral("—"));
    feature_source_sketch_->setText(
        profile != nullptr
            ? fromUtf8(
                  profile->source_sketch_id.value())
            : QStringLiteral("—"));

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
        !revolve_draft_ &&
        !fillet_draft_ &&
        !chamfer_draft_ &&
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
            project_edge_active_) {
            if (key_event->key() == Qt::Key_Escape) {
                escapeProjectEdgeTool();
                return true;
            }
            if (key_event->key() == Qt::Key_Return ||
                key_event->key() == Qt::Key_Enter) {
                static_cast<void>(finishProjectEdgeTool());
                return true;
            }
        }

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

        if (watched == viewport_widget_ &&
            revolve_draft_) {
            if (key_event->key() ==
                Qt::Key_Escape) {
                cancelRevolve();
                return true;
            }
            if (key_event->key() ==
                    Qt::Key_Return ||
                key_event->key() ==
                    Qt::Key_Enter) {
                static_cast<void>(
                    finishRevolve());
                return true;
            }
        }

        if (watched == viewport_widget_ &&
            (fillet_draft_ ||
             chamfer_draft_)) {
            if (key_event->key() ==
                Qt::Key_Escape) {
                cancelEdgeFeature();
                return true;
            }
            if (key_event->key() ==
                    Qt::Key_Return ||
                key_event->key() ==
                    Qt::Key_Enter) {
                static_cast<void>(
                    finishEdgeFeature());
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
    if (project_link_status_label_ != nullptr) {
        project_link_status_label_->setVisible(false);
    }
    if (project_link_break_button_ != nullptr) {
        project_link_break_button_->setVisible(false);
    }
    if (regular_role_button_ != nullptr) {
        regular_role_button_->setVisible(false);
    }
    if (construction_role_button_ != nullptr) {
        construction_role_button_->setVisible(false);
    }
    if (line_part_reference_label_ != nullptr) {
        line_part_reference_label_->setVisible(false);
    }
    if (line_axis_designation_check_ != nullptr) {
        line_axis_designation_check_->setVisible(false);
        line_axis_designation_check_->setEnabled(false);
        line_axis_designation_check_->setTristate(false);
        line_axis_designation_check_->setChecked(false);
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

    if (project_edge_active_ &&
        (!editing ||
         sketch_interaction_controller_->tool() !=
             sketch::SketchTool::select ||
         sketch_interaction_controller_->profileToolActive())) {
        // An ordinary Sketch tool has replaced Project Geometry.
        cancelProjectEdgeTool();
    }
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
    if (project_edge_button_ != nullptr) {
        project_edge_button_->setVisible(editing);
        project_edge_button_->setEnabled(
            editing && solid_modeling_kernel_ != nullptr);
        project_edge_button_->setChecked(project_edge_active_);
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

    if (project_edge_active_) {
        delete_selection_button_->setVisible(false);
        finish_line_button_->setVisible(false);
        cancel_line_button_->setVisible(false);
        if (profile_operations_widget_ != nullptr) {
            profile_operations_widget_->setVisible(false);
        }
        syncProjectEdgeUi();
        return;
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

        entity_role_label_->setText(
            QStringLiteral(
                "Selected geometry role:"));
        entity_role_label_->setVisible(
            selected > 0U);
        regular_role_button_->setVisible(
            selected > 0U);
        construction_role_button_->setVisible(
            selected > 0U);
        // D2-D: a linked source is not an ordinary editable seed.
        // The same Sketch Select context exposes typed source origin
        // and routes Break Link through the existing atomic command.
        if (selected > 0U &&
            active_sketch_id_ &&
            document_session_ != nullptr) {
            const auto targets =
                sketch_interaction_controller_->selectedEntities();
            const auto* sketch =
                document_session_->document().findSketch(
                    *active_sketch_id_);
            if (sketch != nullptr) {
                bool all_linked = true;
                bool all_current = viewport_controller_ != nullptr;
                QString single_source_stage;
                for (const auto& target : targets) {
                    const auto found = std::find_if(
                        sketch->projection_bindings.begin(),
                        sketch->projection_bindings.end(),
                        [&target](const part::ProjectedEdgeBinding& item) {
                            return item.target_entity == target;
                        });
                    if (found == sketch->projection_bindings.end()) {
                        all_linked = false;
                        break;
                    }
                    if (selected == 1U) {
                        single_source_stage =
                            found->source.stage.feature_id
                                ? fromUtf8(
                                      found->source.stage.feature_id
                                          ->serialized())
                                : QStringLiteral("Missing");
                    }
                    all_current = all_current &&
                        viewport_controller_->sketchPresentationFor(
                            target).has_value();
                }
                if (all_linked) {
                    if (project_link_status_label_ != nullptr) {
                        project_link_status_label_->setText(
                            selected == 1U
                                ? QStringLiteral(
                                    "Projected Edge — %1\nSource stage: %2")
                                      .arg(all_current
                                          ? QStringLiteral("Current")
                                          : QStringLiteral("Unresolved"))
                                      .arg(single_source_stage)
                                : QStringLiteral(
                                    "%1 projected Edges — %2")
                                      .arg(static_cast<qulonglong>(selected))
                                      .arg(all_current
                                          ? QStringLiteral("Current")
                                          : QStringLiteral("Unresolved")));
                        project_link_status_label_->setVisible(true);
                    }
                    if (project_link_break_button_ != nullptr) {
                        project_link_break_button_->setVisible(true);
                        project_link_break_button_->setEnabled(
                            all_current && solid_modeling_kernel_ != nullptr);
                    }
                }
            }
        }
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

        if (selected == 1U &&
            active_sketch_id_ &&
            document_session_ != nullptr &&
            line_part_reference_label_ != nullptr &&
            line_axis_designation_check_ != nullptr) {
            const auto entity_id =
                sketch_interaction_controller_->
                    selectedEntities().front();
            const auto* hosted =
                document_session_->document()
                    .findSketch(
                        *active_sketch_id_);
            if (hosted != nullptr &&
                hosted->model.findLine(
                    entity_id) != nullptr) {
                const part::SketchLineAxisSource
                    source{
                        *active_sketch_id_,
                        entity_id};
                std::size_t matches = 0U;
                for (const auto& axis :
                     document_session_->document()
                         .axes()) {
                    if (axis.source == source) {
                        ++matches;
                    }
                }

                line_part_reference_label_->
                    setVisible(true);
                line_axis_designation_check_->
                    setVisible(true);

                if (matches > 1U) {
                    line_axis_designation_check_->
                        setTristate(true);
                    line_axis_designation_check_->
                        setCheckState(
                            Qt::PartiallyChecked);
                    line_axis_designation_check_->
                        setEnabled(false);
                    line_axis_designation_check_->
                        setToolTip(
                            QStringLiteral(
                                "Conflict: more than one legacy Axis references this Line. Repair through Axis Tree/Properties."));
                } else {
                    line_axis_designation_check_->
                        setTristate(false);
                    line_axis_designation_check_->
                        setChecked(
                            matches == 1U);
                    line_axis_designation_check_->
                        setEnabled(
                            matches == 1U ||
                            solid_modeling_kernel_ !=
                                nullptr);
                    line_axis_designation_check_->
                        setToolTip(
                            matches == 1U
                                ? QStringLiteral(
                                      "Remove the Part-owned Axis designation from this Line.")
                                : QStringLiteral(
                                      "Create one Part-owned Axis referencing this Line."));
                }
            }
        }

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
            creation_tool &&
            tool != sketch::SketchTool::line);
    }
    if (rectangle_diagonals_button_ != nullptr) {
        rectangle_diagonals_button_->setVisible(
            tool == sketch::SketchTool::rectangle);
    }

    if (tool == sketch::SketchTool::line) {
        if (entity_role_label_ != nullptr) {
            entity_role_label_->setText(
                QStringLiteral("Geometry role:"));
            entity_role_label_->setVisible(true);
        }
        if (regular_role_button_ != nullptr) {
            regular_role_button_->setVisible(true);
            regular_role_button_->setChecked(
                sketch_interaction_controller_->
                    creationRole() ==
                sketch::EntityRole::regular);
        }
        if (construction_role_button_ != nullptr) {
            construction_role_button_->setVisible(true);
            construction_role_button_->setChecked(
                sketch_interaction_controller_->
                    creationRole() ==
                sketch::EntityRole::construction);
        }
        if (line_part_reference_label_ != nullptr) {
            line_part_reference_label_->setVisible(true);
        }
        if (line_axis_designation_check_ != nullptr) {
            line_axis_designation_check_->
                setTristate(false);
            line_axis_designation_check_->
                setVisible(true);
            line_axis_designation_check_->
                setEnabled(
                    solid_modeling_kernel_ !=
                    nullptr);
            line_axis_designation_check_->
                setChecked(
                    sketch_interaction_controller_->
                        lineAxisDesignation());
            line_axis_designation_check_->
                setToolTip(
                    QStringLiteral(
                        "One-shot: the next successfully committed Line is also designated as a Part Axis; the option then resets."));
        }

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
    const bool edge_feature_active =
        fillet_draft_.has_value() ||
        chamfer_draft_.has_value();

    apply_button_->setEnabled(active);
    undo_button_->setEnabled(
        active &&
        !project_edge_active_ &&
        !sketch_support_pick_active_ &&
        !axis_draft_ &&
        !datum_plane_draft_ &&
        !extrude_profile_pick_active_ &&
        !extrude_draft_ &&
        !revolve_draft_ &&
        !edge_feature_active &&
        document_session->canUndo());
    redo_button_->setEnabled(
        active &&
        !project_edge_active_ &&
        !sketch_support_pick_active_ &&
        !axis_draft_ &&
        !datum_plane_draft_ &&
        !extrude_profile_pick_active_ &&
        !extrude_draft_ &&
        !revolve_draft_ &&
        !edge_feature_active &&
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
        !axis_draft_ &&
        !datum_plane_draft_ &&
        !extrude_profile_pick_active_ &&
        !extrude_draft_ &&
        !revolve_draft_ &&
        !edge_feature_active);

    if (axis_button_ != nullptr) {
        axis_button_->setVisible(active);
        axis_button_->setEnabled(
            active &&
            !sketch_support_pick_active_ &&
            !datum_plane_draft_ &&
            !extrude_profile_pick_active_ &&
            !extrude_draft_ &&
        !revolve_draft_ &&
        !edge_feature_active &&
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
        !revolve_draft_ &&
        !edge_feature_active &&
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
        !revolve_draft_ &&
        !edge_feature_active &&
            solid_modeling_kernel_ != nullptr);
        extrude_button_->setChecked(
            extrude_profile_pick_active_ ||
            extrude_draft_.has_value());
    }

    if (revolve_button_ != nullptr) {
        revolve_button_->setVisible(
            !editing_sketch);
        revolve_button_->setEnabled(
            active &&
            !editing_sketch &&
            !sketch_support_pick_active_ &&
            !axis_draft_ &&
            !datum_plane_draft_ &&
            !extrude_profile_pick_active_ &&
            !extrude_draft_ &&
            !revolve_draft_ &&
        !edge_feature_active &&
            solid_modeling_kernel_ != nullptr);
        revolve_button_->setChecked(
            revolve_draft_.has_value());
    }

    if (part_create_tools_label_ != nullptr) {
        part_create_tools_label_->setVisible(
            !editing_sketch);
    }
    if (part_modify_tools_label_ != nullptr) {
        part_modify_tools_label_->setVisible(
            !editing_sketch);
    }
    const bool edge_feature_tool_available =
        active &&
        !editing_sketch &&
        !sketch_support_pick_active_ &&
        !axis_draft_ &&
        !datum_plane_draft_ &&
        !extrude_profile_pick_active_ &&
        !extrude_draft_ &&
        !revolve_draft_ &&
        solid_modeling_kernel_ != nullptr &&
        !document_session->document()
             .body().features.empty();
    if (fillet_button_ != nullptr) {
        fillet_button_->setVisible(
            !editing_sketch);
        fillet_button_->setEnabled(
            edge_feature_tool_available &&
            !chamfer_draft_);
        fillet_button_->setChecked(
            fillet_draft_.has_value());
    }
    if (chamfer_button_ != nullptr) {
        chamfer_button_->setVisible(
            !editing_sketch);
        chamfer_button_->setEnabled(
            edge_feature_tool_available &&
            !fillet_draft_);
        chamfer_button_->setChecked(
            chamfer_draft_.has_value());
    }

    if (project_edge_button_ != nullptr) {
        project_edge_button_->setVisible(editing_sketch);
        project_edge_button_->setEnabled(
            editing_sketch &&
            solid_modeling_kernel_ != nullptr);
        project_edge_button_->setChecked(project_edge_active_);
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
        !revolve_draft_ &&
        !edge_feature_active &&
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
        !revolve_draft_ &&
        !edge_feature_active &&
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
    syncRevolveUi();
    syncEdgeFeatureUi();
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
