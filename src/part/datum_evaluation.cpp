#include <simplesolid2/part/datum_evaluation.hpp>

#include <simplesolid2/part/part_document.hpp>
#include <simplesolid2/part/part_sketch.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

namespace simplesolid2::part {
namespace {

kernel::Point3 cross(
    const kernel::Point3& a,
    const kernel::Point3& b) noexcept {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x,
    };
}

std::optional<kernel::Point3> normalizedNormal(
    const kernel::Point3& u,
    const kernel::Point3& v) noexcept {
    auto n = cross(u, v);
    const double length =
        std::sqrt(
            n.x * n.x +
            n.y * n.y +
            n.z * n.z);
    if (!std::isfinite(length) ||
        !(length > 0.0)) {
        return std::nullopt;
    }
    n.x /= length;
    n.y /= length;
    n.z /= length;
    return n;
}

std::optional<kernel::Frame3> frameFromPlacement(
    const SketchPlacement& placement) noexcept {
    if (!placement.valid()) {
        return std::nullopt;
    }

    kernel::Frame3 frame{
        {
            placement.origin[0],
            placement.origin[1],
            placement.origin[2],
        },
        {
            placement.u_axis[0],
            placement.u_axis[1],
            placement.u_axis[2],
        },
        {
            placement.v_axis[0],
            placement.v_axis[1],
            placement.v_axis[2],
        },
        {},
    };
    const auto normal =
        normalizedNormal(
            frame.u_axis,
            frame.v_axis);
    if (!normal) {
        return std::nullopt;
    }
    frame.normal = *normal;
    return frame.valid()
        ? std::optional<kernel::Frame3>{frame}
        : std::nullopt;
}

std::optional<kernel::Frame3> offsetFrame(
    const kernel::Frame3& source,
    double offset_mm) noexcept {
    if (!source.valid() ||
        !std::isfinite(offset_mm)) {
        return std::nullopt;
    }

    const auto normal =
        normalizedNormal(
            source.u_axis,
            source.v_axis);
    if (!normal) {
        return std::nullopt;
    }

    auto result = source;
    result.normal = *normal;
    result.origin.x +=
        normal->x * offset_mm;
    result.origin.y +=
        normal->y * offset_mm;
    result.origin.z +=
        normal->z * offset_mm;
    return result.valid()
        ? std::optional<kernel::Frame3>{result}
        : std::nullopt;
}

std::optional<kernel::Frame3> originFrame(
    core::BuiltinReferenceRole role) noexcept {
    const auto support =
        partSketchSupportForBuiltinPlane(role);
    if (!support) {
        return std::nullopt;
    }
    const auto placement =
        sketchPlacementForSupport(*support);
    return placement
        ? frameFromPlacement(*placement)
        : std::nullopt;
}

class DatumEvaluator final {
public:
    DatumEvaluator(
        const PartDocument& document,
        core::DocumentRevision source_revision,
        const std::vector<FeatureEvaluation>&
            feature_evaluations)
        : document_{document},
          source_revision_{source_revision},
          feature_evaluations_{feature_evaluations},
          states_(
              document.datumPlanes().size(),
              VisitState::unvisited),
          evaluated_(
              document.datumPlanes().size()) {}

    DatumEvaluation run() {
        DatumEvaluation result;
        result.source_revision =
            source_revision_;

        if (source_revision_ !=
            document_.revision()) {
            result.planes.reserve(
                document_.datumPlanes().size());
            for (const auto& datum :
                 document_.datumPlanes()) {
                DatumPlaneEvaluation item;
                item.datum_id = datum.id;
                item.status =
                    DatumPlaneEvaluationStatus::
                        blocked;
                item.diagnostic =
                    DatumPlaneEvaluationDiagnostic::
                        stale_part_evaluation;
                result.planes.push_back(
                    std::move(item));
            }
            return result;
        }

        for (std::size_t index = 0U;
             index < document_.datumPlanes().size();
             ++index) {
            resolve(index);
        }

        result.planes = evaluated_;
        return result;
    }

private:
    enum class VisitState {
        unvisited,
        visiting,
        done,
    };

    std::optional<std::size_t> indexOf(
        DatumId id) const noexcept {
        const auto& datums =
            document_.datumPlanes();
        const auto found =
            std::find_if(
                datums.begin(),
                datums.end(),
                [id](const OffsetDatumPlane& item) {
                    return item.id == id;
                });
        if (found == datums.end()) {
            return std::nullopt;
        }
        return static_cast<std::size_t>(
            std::distance(
                datums.begin(),
                found));
    }

    const FeatureEvaluation* featureEvaluation(
        FeatureId id) const noexcept {
        const auto found =
            std::find_if(
                feature_evaluations_.begin(),
                feature_evaluations_.end(),
                [id](const FeatureEvaluation& item) {
                    return item.feature_id == id;
                });
        return found ==
                   feature_evaluations_.end()
            ? nullptr
            : &*found;
    }

    DatumPlaneEvaluation terminal(
        DatumId id,
        DatumPlaneEvaluationStatus status,
        DatumPlaneEvaluationDiagnostic diagnostic,
        std::optional<BodyStageRef> dependency =
            std::nullopt) const {
        DatumPlaneEvaluation result;
        result.datum_id = id;
        result.status = status;
        result.diagnostic = diagnostic;
        result.body_stage_dependency =
            std::move(dependency);
        return result;
    }

    DatumPlaneEvaluation resolve(
        std::size_t index) {
        if (states_[index] ==
            VisitState::done) {
            return evaluated_[index];
        }

        const auto& datum =
            document_.datumPlanes()[index];

        if (states_[index] ==
            VisitState::visiting) {
            auto cycle =
                terminal(
                    datum.id,
                    DatumPlaneEvaluationStatus::
                        unsupported,
                    DatumPlaneEvaluationDiagnostic::
                        dependency_cycle);
            evaluated_[index] = cycle;
            return cycle;
        }

        states_[index] =
            VisitState::visiting;

        if (!offsetDatumPlaneStructurallyValid(
                datum)) {
            auto invalid =
                terminal(
                    datum.id,
                    DatumPlaneEvaluationStatus::
                        unsupported,
                    DatumPlaneEvaluationDiagnostic::
                        invalid_datum);
            states_[index] =
                VisitState::done;
            evaluated_[index] = invalid;
            return invalid;
        }

        std::optional<kernel::Frame3>
            source_frame;
        std::optional<BodyStageRef>
            body_dependency;

        if (const auto origin =
                builtinOriginPlaneForPlaneReference(
                    datum.source)) {
            source_frame =
                originFrame(*origin);
            if (!source_frame) {
                auto invalid =
                    terminal(
                        datum.id,
                        DatumPlaneEvaluationStatus::
                            unsupported,
                        DatumPlaneEvaluationDiagnostic::
                            invalid_frame);
                states_[index] =
                    VisitState::done;
                evaluated_[index] = invalid;
                return invalid;
            }
        } else if (const auto* surface =
                       bodyPlanarSurfaceForPlaneReference(
                           datum.source)) {
            body_dependency =
                surface->stage;

            if (!surface->stage.feature_id ||
                document_.findFeature(
                    *surface->stage.feature_id) ==
                    nullptr) {
                auto missing =
                    terminal(
                        datum.id,
                        DatumPlaneEvaluationStatus::
                            missing,
                        DatumPlaneEvaluationDiagnostic::
                            missing_stage,
                        body_dependency);
                states_[index] =
                    VisitState::done;
                evaluated_[index] = missing;
                return missing;
            }

            const auto* stage =
                featureEvaluation(
                    *surface->stage.feature_id);
            if (stage == nullptr ||
                !stage->result_topology ||
                stage->result_topology->stage !=
                    surface->stage) {
                auto blocked =
                    terminal(
                        datum.id,
                        DatumPlaneEvaluationStatus::
                            blocked,
                        DatumPlaneEvaluationDiagnostic::
                            upstream_body_unavailable,
                        body_dependency);
                states_[index] =
                    VisitState::done;
                evaluated_[index] = blocked;
                return blocked;
            }

            const auto support =
                partSketchSupportForBodyPlanarSurface(
                    *surface);
            if (!support) {
                auto invalid =
                    terminal(
                        datum.id,
                        DatumPlaneEvaluationStatus::
                            unsupported,
                        DatumPlaneEvaluationDiagnostic::
                            invalid_datum,
                        body_dependency);
                states_[index] =
                    VisitState::done;
                evaluated_[index] = invalid;
                return invalid;
            }

            const auto resolved =
                resolveSketchSupport(
                    *support,
                    &*stage->result_topology);

            switch (resolved.status) {
            case SketchSupportResolutionStatus::
                missing: {
                auto missing =
                    terminal(
                        datum.id,
                        DatumPlaneEvaluationStatus::
                            missing,
                        DatumPlaneEvaluationDiagnostic::
                            missing_surface,
                        body_dependency);
                states_[index] =
                    VisitState::done;
                evaluated_[index] = missing;
                return missing;
            }
            case SketchSupportResolutionStatus::
                ambiguous: {
                auto ambiguous =
                    terminal(
                        datum.id,
                        DatumPlaneEvaluationStatus::
                            ambiguous,
                        DatumPlaneEvaluationDiagnostic::
                            ambiguous_surface,
                        body_dependency);
                states_[index] =
                    VisitState::done;
                evaluated_[index] = ambiguous;
                return ambiguous;
            }
            case SketchSupportResolutionStatus::
                unsupported: {
                const auto diagnostic =
                    resolved.diagnostic ==
                            SketchSupportResolutionDiagnostic::
                                unsupported_non_planar
                        ? DatumPlaneEvaluationDiagnostic::
                              unsupported_non_planar
                        : DatumPlaneEvaluationDiagnostic::
                              unsupported_surface;
                auto unsupported =
                    terminal(
                        datum.id,
                        DatumPlaneEvaluationStatus::
                            unsupported,
                        diagnostic,
                        body_dependency);
                states_[index] =
                    VisitState::done;
                evaluated_[index] = unsupported;
                return unsupported;
            }
            case SketchSupportResolutionStatus::
                resolved:
                break;
            }

            if (!resolved.valid() ||
                !resolved.frame) {
                auto invalid =
                    terminal(
                        datum.id,
                        DatumPlaneEvaluationStatus::
                            unsupported,
                        DatumPlaneEvaluationDiagnostic::
                            invalid_frame,
                        body_dependency);
                states_[index] =
                    VisitState::done;
                evaluated_[index] = invalid;
                return invalid;
            }
            source_frame =
                frameFromPlacement(
                    *resolved.frame);
        } else if (const auto source_datum =
                       datumPlaneIdForPlaneReference(
                           datum.source)) {
            const auto source_index =
                indexOf(*source_datum);
            if (!source_index) {
                auto missing =
                    terminal(
                        datum.id,
                        DatumPlaneEvaluationStatus::
                            missing,
                        DatumPlaneEvaluationDiagnostic::
                            missing_source_datum);
                states_[index] =
                    VisitState::done;
                evaluated_[index] = missing;
                return missing;
            }

            if (states_[*source_index] ==
                VisitState::visiting) {
                auto cycle =
                    terminal(
                        datum.id,
                        DatumPlaneEvaluationStatus::
                            unsupported,
                        DatumPlaneEvaluationDiagnostic::
                            dependency_cycle);
                states_[index] =
                    VisitState::done;
                evaluated_[index] = cycle;
                return cycle;
            }

            const auto upstream =
                resolve(*source_index);
            body_dependency =
                upstream.body_stage_dependency;
            if (upstream.status !=
                    DatumPlaneEvaluationStatus::
                        resolved ||
                !upstream.frame) {
                auto blocked =
                    terminal(
                        datum.id,
                        DatumPlaneEvaluationStatus::
                            blocked,
                        DatumPlaneEvaluationDiagnostic::
                            upstream_datum_unavailable,
                        body_dependency);
                states_[index] =
                    VisitState::done;
                evaluated_[index] = blocked;
                return blocked;
            }
            source_frame =
                upstream.frame;
        } else {
            auto invalid =
                terminal(
                    datum.id,
                    DatumPlaneEvaluationStatus::
                        unsupported,
                    DatumPlaneEvaluationDiagnostic::
                        invalid_datum);
            states_[index] =
                VisitState::done;
            evaluated_[index] = invalid;
            return invalid;
        }

        if (!source_frame) {
            auto invalid =
                terminal(
                    datum.id,
                    DatumPlaneEvaluationStatus::
                        unsupported,
                    DatumPlaneEvaluationDiagnostic::
                        invalid_frame,
                    body_dependency);
            states_[index] =
                VisitState::done;
            evaluated_[index] = invalid;
            return invalid;
        }

        const auto frame =
            offsetFrame(
                *source_frame,
                datum.offset.millimetres);
        if (!frame) {
            auto invalid =
                terminal(
                    datum.id,
                    DatumPlaneEvaluationStatus::
                        unsupported,
                    DatumPlaneEvaluationDiagnostic::
                        invalid_frame,
                    body_dependency);
            states_[index] =
                VisitState::done;
            evaluated_[index] = invalid;
            return invalid;
        }

        DatumPlaneEvaluation resolved;
        resolved.datum_id = datum.id;
        resolved.status =
            DatumPlaneEvaluationStatus::resolved;
        resolved.diagnostic =
            DatumPlaneEvaluationDiagnostic::none;
        resolved.frame = *frame;
        resolved.body_stage_dependency =
            body_dependency;

        states_[index] =
            VisitState::done;
        evaluated_[index] = resolved;
        return resolved;
    }

    const PartDocument& document_;
    core::DocumentRevision source_revision_;
    const std::vector<FeatureEvaluation>&
        feature_evaluations_;
    std::vector<VisitState> states_;
    std::vector<DatumPlaneEvaluation>
        evaluated_;
};

} // namespace

bool DatumPlaneEvaluation::valid() const noexcept {
    if (!datum_id.valid()) {
        return false;
    }
    if (body_stage_dependency &&
        !body_stage_dependency->valid()) {
        return false;
    }

    switch (status) {
    case DatumPlaneEvaluationStatus::resolved:
        return diagnostic ==
                   DatumPlaneEvaluationDiagnostic::
                       none &&
               frame.has_value() &&
               frame->valid();
    case DatumPlaneEvaluationStatus::missing:
        return !frame.has_value() &&
               (diagnostic ==
                    DatumPlaneEvaluationDiagnostic::
                        missing_stage ||
                diagnostic ==
                    DatumPlaneEvaluationDiagnostic::
                        missing_surface ||
                diagnostic ==
                    DatumPlaneEvaluationDiagnostic::
                        missing_source_datum);
    case DatumPlaneEvaluationStatus::ambiguous:
        return !frame.has_value() &&
               diagnostic ==
                   DatumPlaneEvaluationDiagnostic::
                       ambiguous_surface;
    case DatumPlaneEvaluationStatus::unsupported:
        return !frame.has_value() &&
               (diagnostic ==
                    DatumPlaneEvaluationDiagnostic::
                        invalid_datum ||
                diagnostic ==
                    DatumPlaneEvaluationDiagnostic::
                        unsupported_non_planar ||
                diagnostic ==
                    DatumPlaneEvaluationDiagnostic::
                        unsupported_surface ||
                diagnostic ==
                    DatumPlaneEvaluationDiagnostic::
                        dependency_cycle ||
                diagnostic ==
                    DatumPlaneEvaluationDiagnostic::
                        invalid_frame);
    case DatumPlaneEvaluationStatus::blocked:
        return !frame.has_value() &&
               (diagnostic ==
                    DatumPlaneEvaluationDiagnostic::
                        stale_part_evaluation ||
                diagnostic ==
                    DatumPlaneEvaluationDiagnostic::
                        upstream_body_unavailable ||
                diagnostic ==
                    DatumPlaneEvaluationDiagnostic::
                        upstream_datum_unavailable);
    }
    return false;
}

const DatumPlaneEvaluation* DatumEvaluation::find(
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

DatumEvaluation evaluateDatumPlanes(
    const PartDocument& document,
    core::DocumentRevision source_revision,
    const std::vector<FeatureEvaluation>&
        feature_evaluations) {
    DatumEvaluator evaluator{
        document,
        source_revision,
        feature_evaluations};
    return evaluator.run();
}

DatumEvaluation evaluateDatumPlanes(
    const PartDocument& document,
    const PartEvaluation& part_evaluation) {
    return evaluateDatumPlanes(
        document,
        part_evaluation.source_revision,
        part_evaluation.features);
}

} // namespace simplesolid2::part
