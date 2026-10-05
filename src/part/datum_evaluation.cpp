#include <simplesolid2/part/datum_evaluation.hpp>

#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/part/part_document.hpp>

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

namespace simplesolid2::part {
namespace {

enum class VisitState {
    unvisited,
    evaluating,
    done,
};

[[nodiscard]] kernel::Frame3 originPlaneFrame(
    core::BuiltinReferenceRole role) noexcept {
    kernel::Frame3 frame;
    switch (role) {
    case core::BuiltinReferenceRole::xy_plane:
        frame.u_axis = {1.0, 0.0, 0.0};
        frame.v_axis = {0.0, 1.0, 0.0};
        frame.normal = {0.0, 0.0, 1.0};
        return frame;
    case core::BuiltinReferenceRole::xz_plane:
        frame.u_axis = {1.0, 0.0, 0.0};
        frame.v_axis = {0.0, 0.0, 1.0};
        frame.normal = {0.0, -1.0, 0.0};
        return frame;
    case core::BuiltinReferenceRole::yz_plane:
        frame.u_axis = {0.0, 1.0, 0.0};
        frame.v_axis = {0.0, 0.0, 1.0};
        frame.normal = {1.0, 0.0, 0.0};
        return frame;
    default:
        return {};
    }
}

[[nodiscard]] std::optional<kernel::Frame3> offsetFrame(
    const kernel::Frame3& source,
    double offset_mm) noexcept {
    if (!source.valid()) {
        return std::nullopt;
    }

    auto result = source;
    result.origin.x +=
        source.normal.x * offset_mm;
    result.origin.y +=
        source.normal.y * offset_mm;
    result.origin.z +=
        source.normal.z * offset_mm;
    return result.valid()
        ? std::optional<kernel::Frame3>{result}
        : std::nullopt;
}

[[nodiscard]] DatumPlaneEvaluation unresolved(
    DatumId id,
    DatumPlaneEvaluationStatus status,
    DatumPlaneEvaluationDiagnostic diagnostic,
    std::optional<BodyStageRef> required_stage =
        std::nullopt) noexcept {
    return DatumPlaneEvaluation{
        id,
        status,
        diagnostic,
        std::nullopt,
        std::move(required_stage)};
}

[[nodiscard]] DatumPlaneEvaluation resolved(
    DatumId id,
    kernel::Frame3 frame,
    std::optional<BodyStageRef> required_stage =
        std::nullopt) noexcept {
    return DatumPlaneEvaluation{
        id,
        DatumPlaneEvaluationStatus::resolved,
        DatumPlaneEvaluationDiagnostic::none,
        std::move(frame),
        std::move(required_stage)};
}

class Evaluator final {
public:
    Evaluator(
        const PartDocument& document,
        const PartEvaluation& part_evaluation)
        : document_{document},
          part_evaluation_{part_evaluation},
          states_(
              document.datumPlanes().size(),
              VisitState::unvisited),
          results_(document.datumPlanes().size()) {}

    [[nodiscard]] DatumEvaluation run() noexcept {
        for (std::size_t index = 0U;
             index < results_.size();
             ++index) {
            evaluate(index);
        }

        DatumEvaluation result;
        result.source_revision = document_.revision();
        result.planes = std::move(results_);
        return result;
    }

private:
    [[nodiscard]] std::optional<std::size_t>
    indexOf(DatumId id) const noexcept {
        const auto& datums = document_.datumPlanes();
        const auto found =
            std::find_if(
                datums.begin(),
                datums.end(),
                [id](const OffsetDatumPlane& datum) {
                    return datum.id == id;
                });
        if (found == datums.end()) {
            return std::nullopt;
        }
        return static_cast<std::size_t>(
            std::distance(datums.begin(), found));
    }

    [[nodiscard]] DatumPlaneEvaluation
    resolveSurface(
        const OffsetDatumPlane& datum,
        const SurfaceReference& reference) const noexcept {
        const auto required_stage =
            std::optional<BodyStageRef>{reference.stage};

        if (part_evaluation_.source_revision !=
            document_.revision()) {
            return unresolved(
                datum.id,
                DatumPlaneEvaluationStatus::blocked,
                DatumPlaneEvaluationDiagnostic::
                    stale_part_evaluation,
                required_stage);
        }

        if (!reference.stage.feature_id) {
            return unresolved(
                datum.id,
                DatumPlaneEvaluationStatus::blocked,
                DatumPlaneEvaluationDiagnostic::
                    body_stage_unavailable,
                required_stage);
        }

        const auto* feature =
            part_evaluation_.findFeature(
                *reference.stage.feature_id);
        if (feature == nullptr ||
            feature->status !=
                FeatureEvaluationStatus::up_to_date ||
            !feature->result_topology ||
            !feature->result_topology->complete() ||
            feature->result_topology->stage !=
                reference.stage) {
            return unresolved(
                datum.id,
                DatumPlaneEvaluationStatus::blocked,
                DatumPlaneEvaluationDiagnostic::
                    body_stage_unavailable,
                required_stage);
        }

        const FeatureSurfaceResolution* found =
            nullptr;
        for (const auto& surface :
             feature->result_topology->surfaces) {
            if (surface.address !=
                reference.surface) {
                continue;
            }
            if (found != nullptr) {
                return unresolved(
                    datum.id,
                    DatumPlaneEvaluationStatus::
                        ambiguous,
                    DatumPlaneEvaluationDiagnostic::
                        ambiguous_surface,
                    required_stage);
            }
            found = &surface;
        }

        if (found == nullptr ||
            found->status ==
                kernel::ReferenceStatus::missing) {
            return unresolved(
                datum.id,
                DatumPlaneEvaluationStatus::missing,
                DatumPlaneEvaluationDiagnostic::
                    missing_surface,
                required_stage);
        }

        if (found->status ==
            kernel::ReferenceStatus::ambiguous) {
            return unresolved(
                datum.id,
                DatumPlaneEvaluationStatus::ambiguous,
                DatumPlaneEvaluationDiagnostic::
                    ambiguous_surface,
                required_stage);
        }

        if (found->status ==
            kernel::ReferenceStatus::unsupported) {
            return unresolved(
                datum.id,
                DatumPlaneEvaluationStatus::unsupported,
                DatumPlaneEvaluationDiagnostic::
                    unsupported_surface,
                required_stage);
        }

        if (found->surface_kind !=
            kernel::SurfaceKind::plane) {
            return unresolved(
                datum.id,
                DatumPlaneEvaluationStatus::unsupported,
                DatumPlaneEvaluationDiagnostic::
                    unsupported_non_planar,
                required_stage);
        }

        if (!found->canonical_frame ||
            !found->canonical_frame->valid()) {
            return unresolved(
                datum.id,
                DatumPlaneEvaluationStatus::unsupported,
                DatumPlaneEvaluationDiagnostic::
                    invalid_frame,
                required_stage);
        }

        const auto frame =
            offsetFrame(
                *found->canonical_frame,
                datum.offset.millimetres);
        if (!frame) {
            return unresolved(
                datum.id,
                DatumPlaneEvaluationStatus::unsupported,
                DatumPlaneEvaluationDiagnostic::
                    invalid_frame,
                required_stage);
        }

        return resolved(
            datum.id,
            *frame,
            required_stage);
    }

    [[nodiscard]] const DatumPlaneEvaluation&
    evaluate(std::size_t index) noexcept {
        if (states_[index] == VisitState::done) {
            return results_[index];
        }

        const auto& datum =
            document_.datumPlanes()[index];

        if (states_[index] ==
            VisitState::evaluating) {
            results_[index] =
                unresolved(
                    datum.id,
                    DatumPlaneEvaluationStatus::blocked,
                    DatumPlaneEvaluationDiagnostic::
                        cyclic_dependency);
            states_[index] = VisitState::done;
            return results_[index];
        }

        states_[index] = VisitState::evaluating;

        if (!offsetDatumPlaneStructurallyValid(
                datum)) {
            results_[index] =
                unresolved(
                    datum.id,
                    DatumPlaneEvaluationStatus::
                        unsupported,
                    DatumPlaneEvaluationDiagnostic::
                        invalid_datum);
            states_[index] = VisitState::done;
            return results_[index];
        }

        if (const auto role =
                builtinOriginPlaneForPlaneReference(
                    datum.source)) {
            const auto source =
                originPlaneFrame(*role);
            const auto frame =
                offsetFrame(
                    source,
                    datum.offset.millimetres);
            results_[index] =
                frame
                    ? resolved(
                          datum.id,
                          *frame)
                    : unresolved(
                          datum.id,
                          DatumPlaneEvaluationStatus::
                              unsupported,
                          DatumPlaneEvaluationDiagnostic::
                              invalid_frame);
            states_[index] = VisitState::done;
            return results_[index];
        }

        if (const auto* surface =
                bodyPlanarSurfaceForPlaneReference(
                    datum.source)) {
            results_[index] =
                resolveSurface(
                    datum,
                    *surface);
            states_[index] = VisitState::done;
            return results_[index];
        }

        const auto source_id =
            datumPlaneIdForPlaneReference(
                datum.source);
        if (!source_id) {
            results_[index] =
                unresolved(
                    datum.id,
                    DatumPlaneEvaluationStatus::
                        unsupported,
                    DatumPlaneEvaluationDiagnostic::
                        invalid_datum);
            states_[index] = VisitState::done;
            return results_[index];
        }

        const auto source_index =
            indexOf(*source_id);
        if (!source_index) {
            results_[index] =
                unresolved(
                    datum.id,
                    DatumPlaneEvaluationStatus::missing,
                    DatumPlaneEvaluationDiagnostic::
                        missing_datum);
            states_[index] = VisitState::done;
            return results_[index];
        }

        const auto& source =
            evaluate(*source_index);
        if (source.status !=
                DatumPlaneEvaluationStatus::resolved ||
            !source.frame) {
            results_[index] =
                unresolved(
                    datum.id,
                    DatumPlaneEvaluationStatus::blocked,
                    source.diagnostic ==
                            DatumPlaneEvaluationDiagnostic::
                                cyclic_dependency
                        ? DatumPlaneEvaluationDiagnostic::
                              cyclic_dependency
                        : DatumPlaneEvaluationDiagnostic::
                              upstream_datum_unavailable,
                    source.required_body_stage);
            states_[index] = VisitState::done;
            return results_[index];
        }

        const auto frame =
            offsetFrame(
                *source.frame,
                datum.offset.millimetres);
        results_[index] =
            frame
                ? resolved(
                      datum.id,
                      *frame,
                      source.required_body_stage)
                : unresolved(
                      datum.id,
                      DatumPlaneEvaluationStatus::
                          unsupported,
                      DatumPlaneEvaluationDiagnostic::
                          invalid_frame,
                      source.required_body_stage);
        states_[index] = VisitState::done;
        return results_[index];
    }

    const PartDocument& document_;
    const PartEvaluation& part_evaluation_;
    std::vector<VisitState> states_;
    std::vector<DatumPlaneEvaluation> results_;
};

} // namespace

bool DatumPlaneEvaluation::valid() const noexcept {
    if (!datum_id.valid()) {
        return false;
    }
    if (required_body_stage &&
        (!required_body_stage->valid() ||
         required_body_stage->kind !=
             BodyStageKind::after_feature)) {
        return false;
    }

    if (status ==
        DatumPlaneEvaluationStatus::resolved) {
        return diagnostic ==
                   DatumPlaneEvaluationDiagnostic::none &&
               frame.has_value() &&
               frame->valid();
    }

    return diagnostic !=
               DatumPlaneEvaluationDiagnostic::none &&
           !frame.has_value();
}

const DatumPlaneEvaluation*
DatumEvaluation::find(
    DatumId id) const noexcept {
    const auto found =
        std::find_if(
            planes.begin(),
            planes.end(),
            [id](const DatumPlaneEvaluation& item) {
                return item.datum_id == id;
            });
    return found == planes.end()
        ? nullptr
        : &*found;
}

bool DatumEvaluation::valid() const noexcept {
    for (std::size_t index = 0U;
         index < planes.size();
         ++index) {
        if (!planes[index].valid()) {
            return false;
        }
        for (std::size_t other = index + 1U;
             other < planes.size();
             ++other) {
            if (planes[index].datum_id ==
                planes[other].datum_id) {
                return false;
            }
        }
    }
    return true;
}

DatumEvaluation evaluateDatums(
    const PartDocument& document,
    const PartEvaluation& part_evaluation) noexcept {
    auto result =
        Evaluator{
            document,
            part_evaluation}
            .run();
    return result.valid()
        ? result
        : DatumEvaluation{
              document.revision(),
              {}};
}

} // namespace simplesolid2::part
