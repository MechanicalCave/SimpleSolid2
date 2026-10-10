#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/kernel/face_boundary.hpp>
#include <simplesolid2/kernel_occt/solid_modeling_kernel.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/part/part_document_store.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <map>
#include <set>
#include <cmath>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-02J R2 Add Surface continuation CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

kernel::BoundaryUse2D lineUse(
    kernel::Point2 start,
    kernel::Point2 end,
    std::string source,
    std::uint32_t use_index) {
    return {
        kernel::Line2{start, end},
        0.0,
        1.0,
        true,
        false,
        false,
        kernel::BoundaryUseProvenance{
            std::move(source),
            0U,
            use_index,
            false},
    };
}

kernel::PlanarProfileInput rectangle(
    kernel::Frame3 frame,
    double u0,
    double v0,
    double u1,
    double v1,
    std::string_view prefix) {
    kernel::PlanarProfileInput profile;
    profile.frame = frame;
    profile.outer.boundary = {
        lineUse(
            {u0, v0},
            {u1, v0},
            std::string{prefix} + "-bottom",
            0U),
        lineUse(
            {u1, v0},
            {u1, v1},
            std::string{prefix} + "-right",
            1U),
        lineUse(
            {u1, v1},
            {u0, v1},
            std::string{prefix} + "-top",
            2U),
        lineUse(
            {u0, v1},
            {u0, v0},
            std::string{prefix} + "-left",
            3U),
    };
    CHECK(profile.valid());
    return profile;
}

kernel::LinearExtrudeInput add(
    kernel::PlanarProfileInput profile,
    double distance) {
    kernel::LinearExtrudeInput input;
    input.profile = std::move(profile);
    input.start_offset_mm = 0.0;
    input.end_offset_mm = distance;
    input.start_cap_role =
        kernel::ExtrudeCapRole::profile_cap;
    input.end_cap_role =
        kernel::ExtrudeCapRole::extent_cap;
    input.operation =
        kernel::SolidBooleanOperation::add;
    CHECK(input.valid());
    return input;
}

const kernel::NewSurfaceLineage*
newCap(
    const kernel::SolidModelingResult& result,
    kernel::ExtrudeCapRole role) {
    const auto found =
        std::find_if(
            result.new_surfaces.begin(),
            result.new_surfaces.end(),
            [role](const auto& surface) {
                return surface.role.kind ==
                           kernel::ExtrudeGeneratedFaceRoleKind::cap &&
                       surface.role.cap_role ==
                           std::optional<kernel::ExtrudeCapRole>{role};
            });
    return found == result.new_surfaces.end()
        ? nullptr
        : &*found;
}

const kernel::NewSurfaceLineage*
newSide(
    const kernel::SolidModelingResult& result,
    std::string_view source) {
    const auto found =
        std::find_if(
            result.new_surfaces.begin(),
            result.new_surfaces.end(),
            [source](const auto& surface) {
                return surface.role.kind ==
                           kernel::ExtrudeGeneratedFaceRoleKind::side &&
                       surface.role.side_provenance &&
                       surface.role.side_provenance->source_entity ==
                           source;
            });
    return found == result.new_surfaces.end()
        ? nullptr
        : &*found;
}

const kernel::InheritedSurfaceLineage*
inheritedSurface(
    const kernel::SolidModelingResult& result,
    kernel::RuntimeSurfaceToken token) {
    const auto found =
        std::find_if(
            result.inherited_surfaces.begin(),
            result.inherited_surfaces.end(),
            [token](const auto& surface) {
                return surface.token == token;
            });
    return found == result.inherited_surfaces.end()
        ? nullptr
        : &*found;
}

sketch::Point2 projectToFrame(
    const kernel::Frame3& frame,
    kernel::Point3 point) {
    const kernel::Point3 delta{
        point.x - frame.origin.x,
        point.y - frame.origin.y,
        point.z - frame.origin.z};
    return {
        delta.x * frame.u_axis.x +
            delta.y * frame.u_axis.y +
            delta.z * frame.u_axis.z,
        delta.x * frame.v_axis.x +
            delta.y * frame.v_axis.y +
            delta.z * frame.v_axis.z,
    };
}

part::ProfileId createRectangleProfile(
    application::DocumentSession& session,
    sketch::SketchId sketch_id,
    sketch::Point2 first,
    sketch::Point2 opposite) {
    const auto rectangle =
        session.execute(
            application::AddSketchRectangleCommand{
                sketch_id,
                session.document().revision(),
                first,
                opposite,
                sketch::EntityRole::regular,
                false});
    CHECK(rectangle.ok());

    const auto* hosted =
        session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    const auto regions =
        sketch::analyzeRegions(hosted->model);
    CHECK(regions.complete());
    CHECK(regions.regions.size() == 1U);
    const auto intent =
        part::makeProfileRegionIntent(
            regions.regions.front());
    CHECK(intent.has_value());

    const auto profile =
        session.execute(
            application::CreateProfileCommand{
                sketch_id,
                session.document().revision(),
                *intent});
    CHECK(profile.ok());
    CHECK(profile.profile_id.has_value());
    return *profile.profile_id;
}

// D2-B research ONLY. Retain the provider's actual final-stage vertex
// observations from the *same* OCCT evaluation consumed by Part. These
// runtime tokens are used for diagnostic correlation only, never authoring.
class Pg01dObservedOcctKernel final
    : public kernel::ISolidModelingKernel {
public:
    kernel_occt::OcctSolidModelingKernel occt;
    std::optional<kernel::SolidModelingResult>
        last_edge_feature;

    [[nodiscard]] kernel::SolidModelingResult extrude(
        const kernel::LinearExtrudeInput& input,
        kernel::RuntimeSolidHandle upstream = {}) noexcept override {
        return occt.extrude(input, std::move(upstream));
    }
    [[nodiscard]] kernel::SolidModelingResult revolve(
        const kernel::AngularRevolveInput& input,
        kernel::RuntimeSolidHandle upstream = {}) noexcept override {
        return occt.revolve(input, std::move(upstream));
    }
    [[nodiscard]] kernel::SolidModelingResult edgeFeature(
        const kernel::EdgeFeatureInput& input,
        kernel::RuntimeSolidHandle upstream = {}) noexcept override {
        auto result = occt.edgeFeature(input, std::move(upstream));
        last_edge_feature = result;
        return result;
    }
};

// Research-only hypothetical selector. It does NOT author a reference:
// it asks whether a semantic Curve family and one currently resolved
// semantic Point uniquely identify an exact material Edge in a stage.
[[nodiscard]] std::vector<kernel::RuntimeEdgeToken>
pg01dCurveEdgesIncidentToSemanticPoint(
    const part::BodyStageTopologyCatalog& catalog,
    const part::FeatureCurveAddress& curve_address,
    const part::FeaturePointAddress& point_address) {
    const auto curve = std::find_if(
        catalog.curves.begin(), catalog.curves.end(),
        [&curve_address](const auto& item) {
            return item.address == curve_address;
        });
    const auto point = std::find_if(
        catalog.points.begin(), catalog.points.end(),
        [&point_address](const auto& item) {
            return item.address == point_address;
        });
    if (curve == catalog.curves.end() ||
        point == catalog.points.end() ||
        point->status != kernel::ReferenceStatus::resolved ||
        point->current_vertices.size() != 1U) {
        return {};
    }
    const auto vertex = std::find_if(
        catalog.vertices.begin(), catalog.vertices.end(),
        [token = point->current_vertices.front()](const auto& item) {
            return item.runtime_token == token;
        });
    if (vertex == catalog.vertices.end() ||
        vertex->accounting_class !=
            part::TopologyAccountingClass::referenceable ||
        vertex->referenceability !=
            kernel::ReferenceStatus::resolved ||
        vertex->point_candidates.size() != 1U ||
        vertex->point_candidates.front() != point_address) {
        return {};
    }
    std::vector<kernel::RuntimeEdgeToken> candidates;
    for (const auto edge_token : curve->current_edges) {
        const auto edge = std::find_if(
            catalog.edges.begin(), catalog.edges.end(),
            [edge_token](const auto& item) {
                return item.runtime_token == edge_token;
            });
        if (edge == catalog.edges.end() ||
            edge->accounting_class !=
                part::TopologyAccountingClass::referenceable ||
            edge->periodic_seam ||
            edge->representation_partition ||
            edge->curve_candidates.size() != 1U ||
            edge->curve_candidates.front() != curve_address) {
            continue;
        }
        if (std::find(
                vertex->incident_material_edges.begin(),
                vertex->incident_material_edges.end(),
                edge_token) !=
            vertex->incident_material_edges.end()) {
            candidates.push_back(edge_token);
        }
    }
    return candidates;
}

// D2 design feasibility ONLY: the proposed third branch must be
// disjoint from BOTH existing v15 branches at its exact source stage.
// The point must be the one and only strictly certified Point endpoint
// of one specific current material Edge in a multi-realization Curve.
// All native tokens are stage-local evidence, never stored.
[[nodiscard]] std::vector<kernel::RuntimeEdgeToken>
pg01dOnePointDisjointCandidate(
    const part::BodyStageTopologyCatalog& catalog,
    const part::BodyStageRef& requested_stage,
    const part::FeatureCurveAddress& curve_address,
    const part::FeaturePointAddress& point_address) {
    if (!catalog.complete() ||
        catalog.stage != requested_stage ||
        catalog.stage.kind != part::BodyStageKind::after_feature) {
        return {};
    }
    const auto curve = std::find_if(
        catalog.curves.begin(), catalog.curves.end(),
        [&curve_address](const auto& item) {
            return item.address == curve_address;
        });
    if (curve == catalog.curves.end() ||
        curve->current_edges.size() < 2U) {
        return {};
    }
    const auto candidates =
        pg01dCurveEdgesIncidentToSemanticPoint(
            catalog, curve_address, point_address);
    if (candidates.size() != 1U) {
        // No "first wins" on zero or multiple edges. This helper
        // only returns admissible unique candidates; the future
        // resolver must retain separate typed Missing/Ambiguous.
        return {};
    }
    const auto edge = candidates.front();
    std::size_t incident = 0U;
    std::size_t certified = 0U;
    bool selected_point_certified = false;
    for (const auto& vertex : catalog.vertices) {
        if (std::find(
                vertex.incident_material_edges.begin(),
                vertex.incident_material_edges.end(),
                edge) ==
            vertex.incident_material_edges.end()) {
            continue;
        }
        ++incident;
        if (vertex.accounting_class !=
                part::TopologyAccountingClass::referenceable ||
            vertex.referenceability !=
                kernel::ReferenceStatus::resolved ||
            vertex.point_candidates.size() != 1U) {
            continue;
        }
        const auto& address = vertex.point_candidates.front();
        const auto found = std::find_if(
            catalog.points.begin(), catalog.points.end(),
            [&address](const auto& item) {
                return item.address == address;
            });
        if (found == catalog.points.end() ||
            found->status != kernel::ReferenceStatus::resolved ||
            found->current_vertices.size() != 1U ||
            found->current_vertices.front() != vertex.runtime_token) {
            continue;
        }
        ++certified;
        if (address == point_address) {
            selected_point_certified = true;
        }
    }
    return incident == 2U && certified == 1U &&
                   selected_point_certified
        ? candidates : std::vector<kernel::RuntimeEdgeToken>{};
}

// D2 research guard: a complete *final* current Body stage is
// mandatory. A resolved-prefix topology from Failed/Blocked feature
// evaluation is diagnostic only, never an alternate source catalog.
[[nodiscard]] std::vector<kernel::RuntimeEdgeToken>
pg01dStrictFinalStageOnePointCandidates(
    const part::PartEvaluation& evaluated,
    const part::BodyStageRef& stage,
    const part::FeatureCurveAddress& curve,
    const part::FeaturePointAddress& point) {
    if (evaluated.body_status !=
            part::BodyEvaluationStatus::up_to_date ||
        !evaluated.current_topology ||
        !evaluated.current_topology->complete() ||
        evaluated.current_topology->stage != stage ||
        stage.kind != part::BodyStageKind::after_feature ||
        !stage.feature_id) {
        return {};
    }
    const auto* feature = evaluated.findFeature(*stage.feature_id);
    if (!feature ||
        feature->status != part::FeatureEvaluationStatus::up_to_date ||
        !feature->result_topology ||
        feature->result_topology->stage != stage) {
        return {};
    }
    return pg01dOnePointDisjointCandidate(
        *evaluated.current_topology, stage, curve, point);
}

void verifyPartIntegration() {
    kernel_occt::OcctSolidModelingKernel provider;
    auto document =
        part::PartDocument::create(
            core::DocumentId::generate());
    application::DocumentSession session{
        std::filesystem::path{
            "pm02jr2-add-continuation.ss2part"},
        std::move(document)};

    const auto base_sketch =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
    CHECK(base_sketch.ok());
    CHECK(base_sketch.sketch_id.has_value());

    const auto base_profile =
        createRectangleProfile(
            session,
            *base_sketch.sketch_id,
            {0.0, 0.0},
            {40.0, 30.0});

    const auto base =
        session.execute(
            application::CreateExtrudeFeatureCommand{
                base_profile,
                session.document().revision(),
                part::ExtrudeOperation::add,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{10.0},
                    false},
                "Base"},
            provider);
    CHECK(base.ok());
    CHECK(base.feature_id.has_value());

    const auto base_eval =
        part::evaluatePart(
            session.document(),
            provider);
    CHECK(
        base_eval.body_status ==
        part::BodyEvaluationStatus::up_to_date);
    CHECK(base_eval.current_topology.has_value());

    const auto& base_topology =
        *base_eval.current_topology;
    const auto top =
        std::find_if(
            base_topology.surfaces.begin(),
            base_topology.surfaces.end(),
            [id = *base.feature_id](
                const auto& surface) {
                return surface.status ==
                           kernel::ReferenceStatus::resolved &&
                       surface.address.producer_feature_id ==
                           id &&
                       surface.address.role ==
                           part::FeatureSurfaceRoleKind::
                               extent_cap &&
                       surface.surface_kind ==
                           kernel::SurfaceKind::plane &&
                       surface.canonical_frame;
            });
    CHECK(top != base_topology.surfaces.end());
    CHECK(top->canonical_frame.has_value());

    const auto continued_side =
        std::find_if(
            base_topology.surfaces.begin(),
            base_topology.surfaces.end(),
            [id = *base.feature_id](
                const auto& surface) {
                return surface.status ==
                           kernel::ReferenceStatus::resolved &&
                       surface.address.producer_feature_id ==
                           id &&
                       surface.address.role ==
                           part::FeatureSurfaceRoleKind::side &&
                       surface.surface_kind ==
                           kernel::SurfaceKind::plane &&
                       surface.canonical_frame &&
                       surface.canonical_frame->normal.x > 0.9;
            });
    CHECK(continued_side !=
          base_topology.surfaces.end());

    const part::SurfaceReference top_ref{
        base_topology.stage,
        top->address};
    const auto top_support =
        part::partSketchSupportForBodyPlanarSurface(
            top_ref);
    CHECK(top_support.has_value());

    const auto boss_sketch =
        session.execute(
            application::CreatePartSketchOnSupportCommand{
                *top_support,
                session.document().revision()},
            &provider);
    CHECK(boss_sketch.ok());
    CHECK(boss_sketch.sketch_id.has_value());

    const auto& top_frame =
        *top->canonical_frame;
    const std::vector<kernel::Point3> boss_corners{
        {20.0, 5.0, 10.0},
        {40.0, 5.0, 10.0},
        {20.0, 25.0, 10.0},
        {40.0, 25.0, 10.0},
    };
    std::vector<sketch::Point2> uv;
    uv.reserve(boss_corners.size());
    for (const auto& point : boss_corners) {
        uv.push_back(
            projectToFrame(
                top_frame,
                point));
    }
    double min_u = uv.front().u;
    double max_u = uv.front().u;
    double min_v = uv.front().v;
    double max_v = uv.front().v;
    for (const auto point : uv) {
        min_u = std::min(min_u, point.u);
        max_u = std::max(max_u, point.u);
        min_v = std::min(min_v, point.v);
        max_v = std::max(max_v, point.v);
    }
    CHECK(max_u - min_u > 1.0);
    CHECK(max_v - min_v > 1.0);

    const auto boss_profile =
        createRectangleProfile(
            session,
            *boss_sketch.sketch_id,
            {min_u, min_v},
            {max_u, max_v});

    const auto boss =
        session.execute(
            application::CreateExtrudeFeatureCommand{
                boss_profile,
                session.document().revision(),
                part::ExtrudeOperation::add,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{10.0},
                    false},
                "Boss"},
            provider);
    CHECK(boss.ok());
    CHECK(boss.feature_id.has_value());

    const auto final_eval =
        part::evaluatePart(
            session.document(),
            provider);
    CHECK(
        final_eval.body_status ==
        part::BodyEvaluationStatus::up_to_date);
    CHECK(final_eval.current_topology.has_value());
    const auto& topology =
        *final_eval.current_topology;

    const auto current_side =
        std::find_if(
            topology.surfaces.begin(),
            topology.surfaces.end(),
            [address = continued_side->address](
                const auto& surface) {
                return surface.address == address;
            });
    CHECK(current_side != topology.surfaces.end());
    CHECK(
        current_side->status ==
        kernel::ReferenceStatus::resolved);
    CHECK(current_side->current_faces.size() >= 2U);
    CHECK(
        current_side->strict_face_status ==
        kernel::ReferenceStatus::ambiguous);

    for (const auto face_token :
         current_side->current_faces) {
        const auto face =
            std::find_if(
                topology.faces.begin(),
                topology.faces.end(),
                [face_token](const auto& record) {
                    return record.runtime_token ==
                           face_token;
                });
        CHECK(face != topology.faces.end());
        CHECK(face->surface_candidates.size() == 1U);
        CHECK(
            face->surface_candidates.front() ==
            current_side->address);
    }

    // PG-01D Owner follow-up: unlike a generic two-Add/Chamfer Part,
    // THIS controlled real OCCT fixture has a proven semantic Surface
    // with >=2 individual bounded Faces. Apply a downstream Chamfer
    // in an independent cold provider, then classify manual bounded
    // Face admission without publishing any private Owner geometry.
    // A non-authorable material source remains an integrity refusal;
    // this probe must never skip it as geometric Unsupported.
    {
        bool split_chamfer_probed = false;
        std::size_t attempts = 0U;
        for (const auto& candidate : topology.edges) {
            const auto source =
                part::authorMaterialEdgeReference(
                    topology, candidate.runtime_token);
            if (!source.ok() || !source.reference) continue;
            ++attempts;
            auto clone = part::PartDocument::restore(
                core::DocumentId::generate(),
                session.document().state());
            CHECK(clone.ok());
            application::DocumentSession chamfer_session{
                {}, std::move(*clone.document)};
            kernel_occt::OcctSolidModelingKernel chamfer_kernel;
            const auto added_chamfer = chamfer_session.execute(
                application::CreateChamferFeatureCommand{
                    {*source.reference},
                    chamfer_session.document().revision(),
                    core::LengthValue{1.0},
                    "PG01D synthetic split-surface Chamfer"},
                chamfer_kernel);
            if (!added_chamfer.ok()) continue;
            const auto chamfered = part::evaluatePart(
                chamfer_session.document(), chamfer_kernel);
            if (chamfered.body_status !=
                    part::BodyEvaluationStatus::up_to_date ||
                !chamfered.current_topology ||
                chamfered.features.size() != 3U) {
                continue;
            }
            const auto& next_catalog = *chamfered.current_topology;
            const auto split_after =
                std::find_if(
                    next_catalog.surfaces.begin(),
                    next_catalog.surfaces.end(),
                    [address = continued_side->address](
                        const auto& surface) {
                        return surface.address == address;
                    });
            if (split_after == next_catalog.surfaces.end() ||
                split_after->status !=
                    kernel::ReferenceStatus::resolved ||
                split_after->current_faces.size() < 2U) {
                continue;
            }
            std::size_t admitted = 0U;
            std::size_t rejected_material = 0U;
            std::size_t rejected_other = 0U;
            for (const auto face_token : split_after->current_faces) {
                const auto admission =
                    part::inspectSelectedFaceBoundary(
                        chamfered.features.back(),
                        face_token, chamfer_kernel);
                if (admission.ok()) {
                    ++admitted;
                    continue;
                }
                if (admission.status ==
                        part::MaterialFaceBoundaryStatus::
                            material_edge_unavailable) {
                    ++rejected_material;
                    const auto* detail = admission.rejected_edge
                        ? &*admission.rejected_edge : nullptr;
                    std::cout
                        << "PG01D_SPLIT_CHAMFER_FACE_REJECTION"
                        << " face=" << face_token.value
                        << " failed_edge="
                        << (detail ? detail->edge.value : 0U)
                        << " kind="
                        << (detail ? static_cast<int>(detail->kind) : -1)
                        << " accounting="
                        << (detail && detail->accounting_class
                            ? static_cast<int>(*detail->accounting_class)
                            : -1)
                        << " reference="
                        << (detail && detail->referenceability
                            ? static_cast<int>(*detail->referenceability)
                            : -1)
                        << " candidates="
                        << (detail ? detail->curve_candidate_count : 0U)
                        << " partition="
                        << (detail && detail->representation_partition)
                        << '\n';
                } else {
                    ++rejected_other;
                }
            }
            CHECK(admitted + rejected_material + rejected_other ==
                  split_after->current_faces.size());
            std::cout
                << "PG01D_SPLIT_ADD_CHAMFER_BOUNDARY_PROBE_PASS"
                << " split_faces=" << split_after->current_faces.size()
                << " admitted=" << admitted
                << " material_blocked=" << rejected_material
                << " other_blocked=" << rejected_other
                << " tried_chamfer_sources=" << attempts
                << " private_document_committed=0"
                << '\n';
            // This fixture has two certifiably authorable bounded
            // Faces of the continued Surface even after a real Chamfer.
            // Do not generalize this positive witness to every Chamfer
            // corner, CurveKind or the Owner's private Part.
            CHECK(admitted == split_after->current_faces.size());
            CHECK(rejected_material == 0U);
            CHECK(rejected_other == 0U);
            split_chamfer_probed = true;
            break;
        }
        CHECK(split_chamfer_probed);

        // Additional *diagnostic* sweep over independent authorable
        // source Edges and nontrivial Chamfer sizes on the controlled
        // split-Surface Add continuation. No Owner geometry or IDs.
        // This does not assume every native Edge is durable material.
        std::size_t possible_sources = 0U;
        std::size_t committed_chamfers = 0U;
        std::size_t preserved_split = 0U;
        std::size_t rejected_material = 0U;
        std::size_t rejected_other = 0U;
        std::size_t admitted_faces = 0U;
        std::size_t point_audit_samples = 0U;
        std::size_t missing_endpoint_two_surfaces = 0U;
        std::size_t missing_endpoint_three_surfaces = 0U;
        std::size_t missing_endpoint_other_surfaces = 0U;
        std::size_t missing_endpoint_inherited = 0U;
        std::size_t missing_endpoint_new = 0U;
        std::size_t missing_endpoint_not_accounted = 0U;
        std::size_t two_surface_pair_unique = 0U;
        std::size_t two_surface_pair_collided = 0U;
        std::size_t one_semantic_endpoint_unique = 0U;
        std::size_t one_semantic_endpoint_ambiguous = 0U;
        std::size_t one_semantic_endpoint_cold_unique = 0U;
        bool tested_ambiguous_incidence = false;
        bool tested_missing_reference = false;
        bool tested_native_file_reopen = false;
        bool tested_undo_redo = false;
        bool tested_upstream_edit = false;
        bool tested_upstream_edit_undo_redo = false;
        std::size_t rejected_upstream_edits = 0U;
        bool tested_radical_upstream_matrix = false;
        std::size_t radical_attempts = 0U;
        std::size_t radical_command_rejected = 0U;
        std::size_t radical_final_unavailable = 0U;
        std::size_t radical_stage_missing = 0U;
        std::size_t radical_source_missing = 0U;
        std::size_t radical_source_unique = 0U;
        std::size_t radical_source_ambiguous = 0U;
        std::size_t native_multipiece_curve_point_probes = 0U;
        std::size_t native_onepoint_resolved = 0U;
        std::size_t native_onepoint_ambiguous = 0U;
        std::size_t native_curvepoint_two_point_aliases = 0U;
        std::size_t disjoint_onepoint_admitted = 0U;
        std::size_t disjoint_old_twopoint_refused = 0U;
        bool tested_wrong_stage_refusal = false;
        bool tested_predecessor_suppression = false;
        bool predecessor_suppression_committed = false;
        bool predecessor_suppression_undo_restored = false;
        std::size_t predecessor_suppression_rejections = 0U;
        bool tested_removed_final_stage = false;
        bool tested_suppressed_final_stage = false;
        bool tested_predecessor_stage_suppression = false;
        std::size_t predecessor_stage_absent = 0U;
        std::size_t predecessor_stage_survived = 0U;
        for (const auto& candidate : topology.edges) {
            const auto source =
                part::authorMaterialEdgeReference(
                    topology, candidate.runtime_token);
            if (!source.ok() || !source.reference) continue;
            ++possible_sources;
            for (const double distance : {0.6, 1.7, 3.3, 7.5}) {
                auto cloned = part::PartDocument::restore(
                    core::DocumentId::generate(),
                    session.document().state());
                CHECK(cloned.ok());
                application::DocumentSession trial{
                    {}, std::move(*cloned.document)};
                Pg01dObservedOcctKernel native;
                const auto finish = trial.execute(
                    application::CreateChamferFeatureCommand{
                        {*source.reference},
                        trial.document().revision(),
                        core::LengthValue{distance},
                        "PG01D synthetic varied Chamfer"},
                    native);
                if (!finish.ok()) continue;
                const auto evaluated = part::evaluatePart(
                    trial.document(), native);
                if (evaluated.body_status !=
                        part::BodyEvaluationStatus::up_to_date ||
                    !evaluated.current_topology ||
                    !evaluated.current_topology->complete() ||
                    evaluated.features.size() != 3U) {
                    continue;
                }
                ++committed_chamfers;
                const auto& ledger = *evaluated.current_topology;
                // E5: survey *actual* OCCT material incidence on every
                // current multi-realization Curve family, without
                // injecting any fabricated topology. This can expose
                // genuine Curve + Point cardinality conflicts. The
                // earlier synthetic two-incidence counterexample
                // remains a separate fail-closed unit proof.
                for (const auto& family : ledger.curves) {
                    if (family.current_edges.size() < 2U) continue;
                    for (const auto& point : ledger.points) {
                        if (point.status !=
                                kernel::ReferenceStatus::resolved ||
                            point.current_vertices.size() != 1U) {
                            continue;
                        }
                        ++native_multipiece_curve_point_probes;
                        const auto candidates =
                            pg01dCurveEdgesIncidentToSemanticPoint(
                                ledger, family.address, point.address);
                        if (candidates.size() == 1U) {
                            ++native_onepoint_resolved;
                            const auto authored_old =
                                part::authorMaterialEdgeReference(
                                    ledger, candidates.front());
                            const auto disjoint =
                                pg01dOnePointDisjointCandidate(
                                    ledger, ledger.stage,
                                    family.address, point.address);
                            if (!tested_wrong_stage_refusal) {
                                CHECK(topology.stage != ledger.stage);
                                CHECK(pg01dOnePointDisjointCandidate(
                                    ledger, topology.stage,
                                    family.address, point.address).empty());
                                tested_wrong_stage_refusal = true;
                            }
                            if (authored_old.ok() &&
                                authored_old.reference &&
                                std::holds_alternative<
                                    part::BetweenSemanticPoints>(
                                    authored_old.reference->branch)) {
                                // This actual OCCT material Edge is also
                                // uniquely selected by Curve+one Point,
                                // but it already has valid two-Point
                                // identity in current v15.
                                ++native_curvepoint_two_point_aliases;
                                CHECK(disjoint.empty());
                                ++disjoint_old_twopoint_refused;
                            } else {
                                // Existing one-Point-only gaps are the
                                // ONLY admissible stage domain for the
                                // proposed third branch.
                                CHECK(disjoint.size() == 1U);
                                CHECK(disjoint.front() ==
                                      candidates.front());
                                ++disjoint_onepoint_admitted;
                            }
                        } else if (candidates.size() > 1U) {
                            ++native_onepoint_ambiguous;
                            std::cout
                                << "PG01D_NATIVE_ONEPOINT_AMBIGUOUS"
                                << " source=" << candidate.runtime_token.value
                                << " distance=" << distance
                                << " family_edges="
                                << family.current_edges.size()
                                << " incident_candidates="
                                << candidates.size()
                                << '\n';
                        }
                    }
                }
                const auto continued = std::find_if(
                    ledger.surfaces.begin(), ledger.surfaces.end(),
                    [address = continued_side->address](
                        const auto& item) {
                        return item.address == address;
                    });
                if (continued != ledger.surfaces.end() &&
                    continued->current_faces.size() >= 2U) {
                    ++preserved_split;
                }
                for (const auto& face : ledger.faces) {
                    if (face.accounting_class !=
                            part::TopologyAccountingClass::
                                referenceable) {
                        continue;
                    }
                    const auto admission =
                        part::inspectSelectedFaceBoundary(
                            evaluated.features.back(),
                            face.runtime_token, native.occt);
                    if (admission.ok()) {
                        ++admitted_faces;
                    } else if (admission.status ==
                            part::MaterialFaceBoundaryStatus::
                                material_edge_unavailable) {
                        ++rejected_material;
                        const auto* detail = admission.rejected_edge
                            ? &*admission.rejected_edge : nullptr;
                        std::size_t family_realizations = 0U;
                        std::size_t incident_vertices = 0U;
                        std::size_t certified_endpoints = 0U;
                        std::optional<part::FeaturePointAddress>
                            certified_point_address;
                        if (detail) {
                            CHECK(native.last_edge_feature);
                            CHECK(native.last_edge_feature->ok());
                            const auto record = std::find_if(
                                ledger.edges.begin(), ledger.edges.end(),
                                [token = detail->edge](const auto& item) {
                                    return item.runtime_token == token;
                                });
                            if (record != ledger.edges.end() &&
                                record->curve_candidates.size() == 1U) {
                                const auto family = std::find_if(
                                    ledger.curves.begin(),
                                    ledger.curves.end(),
                                    [address = record->curve_candidates.front()](
                                        const auto& item) {
                                        return item.address == address;
                                    });
                                if (family != ledger.curves.end()) {
                                    family_realizations =
                                        family->current_edges.size();
                                }
                            }
                            for (const auto& vertex : ledger.vertices) {
                                if (std::find(
                                        vertex.incident_material_edges.begin(),
                                        vertex.incident_material_edges.end(),
                                        detail->edge) ==
                                    vertex.incident_material_edges.end()) {
                                    continue;
                                }
                                ++incident_vertices;
                                const bool is_certified =
                                    vertex.accounting_class ==
                                        part::TopologyAccountingClass::
                                            referenceable &&
                                    vertex.referenceability ==
                                        kernel::ReferenceStatus::resolved &&
                                    vertex.point_candidates.size() == 1U;
                                if (is_certified) {
                                    ++certified_endpoints;
                                    CHECK(!certified_point_address);
                                    certified_point_address =
                                        vertex.point_candidates.front();
                                } else {
                                    // Same exact provider invocation as the
                                    // Part semantic stage, NOT a rebuilt
                                    // or numerically matched OCCT Vertex.
                                    const auto& raw =
                                        *native.last_edge_feature;
                                    const auto observed = std::find_if(
                                        raw.current_vertex_semantics.begin(),
                                        raw.current_vertex_semantics.end(),
                                        [token = vertex.runtime_token](
                                            const auto& item) {
                                            return item.runtime_token == token;
                                        });
                                    CHECK(observed !=
                                          raw.current_vertex_semantics.end());
                                    std::vector<
                                        part::FeatureSurfaceAddress>
                                        semantic_surfaces;
                                    bool complete_mapping = true;
                                    for (const auto carrier :
                                         observed->adjacent_surfaces) {
                                        const auto resolved = std::find_if(
                                            ledger.surfaces.begin(),
                                            ledger.surfaces.end(),
                                            [carrier](const auto& item) {
                                                return item.status ==
                                                    kernel::ReferenceStatus::
                                                        resolved &&
                                                    item.runtime_token &&
                                                    *item.runtime_token ==
                                                        carrier;
                                            });
                                        if (resolved ==
                                                ledger.surfaces.end()) {
                                            complete_mapping = false;
                                            continue;
                                        }
                                        if (std::find(
                                                semantic_surfaces.begin(),
                                                semantic_surfaces.end(),
                                                resolved->address) ==
                                            semantic_surfaces.end()) {
                                            semantic_surfaces.push_back(
                                                resolved->address);
                                        }
                                    }
                                    CHECK(complete_mapping);
                                    std::sort(
                                        semantic_surfaces.begin(),
                                        semantic_surfaces.end());
                                    // Does the 2-Surface proposal identify
                                    // exactly one actual current Vertex?
                                    // Count identical semantic relations,
                                    // never geometric proximity or ordinals.
                                    std::size_t same_pair_vertices = 0U;
                                    for (const auto& raw_vertex :
                                         raw.current_vertex_semantics) {
                                        std::vector<
                                            part::FeatureSurfaceAddress>
                                            other_surfaces;
                                        bool other_resolved = true;
                                        for (const auto token :
                                             raw_vertex.adjacent_surfaces) {
                                            const auto found =
                                                std::find_if(
                                                    ledger.surfaces.begin(),
                                                    ledger.surfaces.end(),
                                                    [token](const auto& item) {
                                                        return item.status ==
                                                            kernel::ReferenceStatus::
                                                                resolved &&
                                                            item.runtime_token &&
                                                            *item.runtime_token ==
                                                                token;
                                                    });
                                            if (found ==
                                                    ledger.surfaces.end()) {
                                                other_resolved = false;
                                                break;
                                            }
                                            if (std::find(
                                                    other_surfaces.begin(),
                                                    other_surfaces.end(),
                                                    found->address) ==
                                                other_surfaces.end()) {
                                                other_surfaces.push_back(
                                                    found->address);
                                            }
                                        }
                                        if (!other_resolved) continue;
                                        std::sort(
                                            other_surfaces.begin(),
                                            other_surfaces.end());
                                        if (other_surfaces ==
                                            semantic_surfaces) {
                                            ++same_pair_vertices;
                                        }
                                    }
                                    CHECK(same_pair_vertices > 0U);
                                    if (same_pair_vertices == 1U) {
                                        ++two_surface_pair_unique;
                                    } else {
                                        ++two_surface_pair_collided;
                                    }
                                    if (semantic_surfaces.size() == 2U) {
                                        ++missing_endpoint_two_surfaces;
                                    } else if (
                                        semantic_surfaces.size() == 3U) {
                                        ++missing_endpoint_three_surfaces;
                                    } else {
                                        ++missing_endpoint_other_surfaces;
                                    }
                                    std::size_t current_lineage = 0U;
                                    for (const auto& inherited :
                                         raw.inherited_vertex_realizations) {
                                        if (std::find(
                                                inherited.current_vertices
                                                    .begin(),
                                                inherited.current_vertices
                                                    .end(),
                                                vertex.runtime_token) !=
                                            inherited.current_vertices
                                                .end()) {
                                            ++current_lineage;
                                        }
                                    }
                                    if (current_lineage == 0U) {
                                        ++missing_endpoint_new;
                                    } else {
                                        ++missing_endpoint_inherited;
                                    }
                                    if (vertex.accounting_class !=
                                            part::TopologyAccountingClass::
                                                semantically_unsupported) {
                                        ++missing_endpoint_not_accounted;
                                    }
                                    if (point_audit_samples++ < 8U) {
                                        std::cout
                                            << "PG01D_D2B_POINT_OBSERVATION"
                                            << " semanticSurfaces="
                                            << semantic_surfaces.size()
                                            << " nativeSurfaces="
                                            << observed->adjacent_surfaces
                                                   .size()
                                            << " pointAccounting="
                                            << static_cast<int>(
                                                   vertex.accounting_class)
                                            << " pointReference="
                                            << static_cast<int>(
                                                   vertex.referenceability)
                                            << " inheritedSources="
                                            << current_lineage
                                            << " pointCandidates="
                                            << vertex.point_candidates.size()
                                            << " sameSemanticPairVertices="
                                            << same_pair_vertices
                                            << '\n';
                                    }
                                }
                            }
                        }
                        if (detail &&
                            certified_point_address &&
                            detail->curve_candidate_count == 1U) {
                            const auto edge = std::find_if(
                                ledger.edges.begin(), ledger.edges.end(),
                                [token = detail->edge](const auto& item) {
                                    return item.runtime_token == token;
                                });
                            CHECK(edge != ledger.edges.end());
                            const auto& curve_address =
                                edge->curve_candidates.front();
                            const auto current_choices =
                                pg01dCurveEdgesIncidentToSemanticPoint(
                                    ledger, curve_address,
                                    *certified_point_address);
                            if (current_choices.size() == 1U &&
                                current_choices.front() == detail->edge) {
                                ++one_semantic_endpoint_unique;
                                // Fresh Part Document and fresh OCCT provider:
                                // use only durable semantic Curve + Point
                                // addresses; never compare native tokens
                                // between independently evaluated Bodies.
                                auto restored = part::PartDocument::restore(
                                    core::DocumentId::generate(),
                                    trial.document().state());
                                CHECK(restored.ok());
                                kernel_occt::OcctSolidModelingKernel
                                    cold_provider;
                                const auto cold = part::evaluatePart(
                                    *restored.document, cold_provider);
                                CHECK(cold.body_status ==
                                      part::BodyEvaluationStatus::
                                          up_to_date);
                                CHECK(cold.current_topology &&
                                      cold.current_topology->complete());
                                const auto cold_choices =
                                    pg01dCurveEdgesIncidentToSemanticPoint(
                                        *cold.current_topology,
                                        curve_address,
                                        *certified_point_address);
                                if (cold_choices.size() == 1U) {
                                    ++one_semantic_endpoint_cold_unique;
                                }

                                // Test-only fabricated collision of
                                // material incidence, never a claim about
                                // certified native OCCT topology. Cardinality
                                // MUST be enforced: no "first Edge wins".
                                if (!tested_ambiguous_incidence) {
                                    const auto curve = std::find_if(
                                        ledger.curves.begin(),
                                        ledger.curves.end(),
                                        [&curve_address](const auto& item) {
                                            return item.address ==
                                                curve_address;
                                        });
                                    CHECK(curve != ledger.curves.end());
                                    CHECK(curve->current_edges.size() == 2U);
                                    const auto alternate =
                                        *std::find_if(
                                            curve->current_edges.begin(),
                                            curve->current_edges.end(),
                                            [token = detail->edge](
                                                const auto& item) {
                                                return item != token;
                                            });
                                    auto injected = ledger;
                                    const auto point_record =
                                        std::find_if(
                                            injected.points.begin(),
                                            injected.points.end(),
                                            [addr = *certified_point_address](
                                                const auto& item) {
                                                return item.address == addr;
                                            });
                                    CHECK(point_record !=
                                          injected.points.end());
                                    CHECK(point_record->status ==
                                          kernel::ReferenceStatus::resolved);
                                    CHECK(point_record->current_vertices.size() ==
                                          1U);
                                    auto vertex_record = std::find_if(
                                        injected.vertices.begin(),
                                        injected.vertices.end(),
                                        [token = point_record->
                                            current_vertices.front()](
                                            const auto& item) {
                                            return item.runtime_token == token;
                                        });
                                    CHECK(vertex_record !=
                                          injected.vertices.end());
                                    CHECK(std::find(
                                        vertex_record->
                                            incident_material_edges.begin(),
                                        vertex_record->
                                            incident_material_edges.end(),
                                        alternate) ==
                                        vertex_record->
                                            incident_material_edges.end());
                                    vertex_record->incident_material_edges
                                        .push_back(alternate);
                                    vertex_record->incident_material_edge_count =
                                        vertex_record->incident_material_edges
                                            .size();
                                    const auto ambiguous =
                                        pg01dCurveEdgesIncidentToSemanticPoint(
                                            injected, curve_address,
                                            *certified_point_address);
                                    CHECK(ambiguous.size() == 2U);
                                    CHECK(ambiguous.front() != ambiguous.back());
                                    tested_ambiguous_incidence = true;
                                }
                                if (!tested_missing_reference) {
                                    auto missing = ledger;
                                    missing.points.erase(
                                        std::remove_if(
                                            missing.points.begin(),
                                            missing.points.end(),
                                            [addr = *certified_point_address](
                                                const auto& item) {
                                                return item.address == addr;
                                            }),
                                        missing.points.end());
                                    CHECK(pg01dCurveEdgesIncidentToSemanticPoint(
                                        missing, curve_address,
                                        *certified_point_address).empty());
                                    tested_missing_reference = true;
                                }

                                if (!tested_native_file_reopen) {
                                    part::PartDocumentStore store;
                                    const auto path =
                                        std::filesystem::temp_directory_path() /
                                        ("ss2_pg01d_d2b_" +
                                         std::to_string(
                                             std::chrono::steady_clock::now()
                                                 .time_since_epoch().count()) +
                                         ".ss2part");
                                    const auto save = store.createNew(
                                        path, trial.document());
                                    CHECK(save.ok());
                                    const auto loaded = store.load(path);
                                    CHECK(loaded.ok());
                                    CHECK(loaded.document->state() ==
                                          trial.document().state());
                                    kernel_occt::OcctSolidModelingKernel disk_provider;
                                    const auto disk = part::evaluatePart(
                                        *loaded.document, disk_provider);
                                    CHECK(disk.body_status ==
                                          part::BodyEvaluationStatus::up_to_date);
                                    CHECK(disk.current_topology &&
                                          disk.current_topology->complete());
                                    const auto disk_choices =
                                        pg01dCurveEdgesIncidentToSemanticPoint(
                                            *disk.current_topology,
                                            curve_address,
                                            *certified_point_address);
                                    CHECK(disk_choices.size() == 1U);
                                    std::error_code removal_error;
                                    CHECK(std::filesystem::remove(
                                        path, removal_error));
                                    CHECK(!removal_error);
                                    tested_native_file_reopen = true;
                                }

                                if (!tested_undo_redo) {
                                    const auto undo = trial.undo();
                                    CHECK(undo.ok());
                                    CHECK(trial.document().body().features.size()
                                          == 2U);
                                    const auto redo = trial.redo();
                                    CHECK(redo.ok());
                                    CHECK(trial.document().body().features.size()
                                          == 3U);
                                    kernel_occt::OcctSolidModelingKernel
                                        redo_provider;
                                    const auto after_redo = part::evaluatePart(
                                        trial.document(), redo_provider);
                                    CHECK(after_redo.body_status ==
                                          part::BodyEvaluationStatus::
                                              up_to_date);
                                    CHECK(after_redo.current_topology &&
                                          after_redo.current_topology->complete());
                                    const auto after_redo_choices =
                                        pg01dCurveEdgesIncidentToSemanticPoint(
                                            *after_redo.current_topology,
                                            curve_address,
                                            *certified_point_address);
                                    CHECK(after_redo_choices.size() == 1U);
                                    tested_undo_redo = true;
                                }
                                if (!tested_predecessor_suppression) {
                                    auto suppressed = part::PartDocument::restore(
                                        core::DocumentId::generate(),
                                        trial.document().state());
                                    CHECK(suppressed.ok());
                                    application::DocumentSession suppress_session{
                                        {}, std::move(*suppressed.document)};
                                    const auto before_suppression =
                                        suppress_session.document().state();
                                    const auto suppress =
                                        suppress_session.execute(
                                            application::SetFeatureSuppressedCommand{
                                                *boss.feature_id,
                                                suppress_session.document()
                                                    .revision(),
                                                true});
                                    if (suppress.ok()) {
                                        predecessor_suppression_committed = true;
                                        kernel_occt::OcctSolidModelingKernel
                                            suppress_provider;
                                        const auto suppressed_result =
                                            part::evaluatePart(
                                                suppress_session.document(),
                                                suppress_provider);
                                        const auto source_after_suppression =
                                            pg01dStrictFinalStageOnePointCandidates(
                                                suppressed_result, ledger.stage,
                                                curve_address,
                                                *certified_point_address);
                                        CHECK(source_after_suppression.size() <= 1U);
                                        if (source_after_suppression.empty()) {
                                            ++predecessor_stage_absent;
                                        } else {
                                            ++predecessor_stage_survived;
                                        }
                                        // Never replace the final Body
                                        // with a diagnostically preserved
                                        // prefix (which may have identical
                                        // native token integers).
                                        if (suppressed_result.body_status !=
                                                part::BodyEvaluationStatus::
                                                    up_to_date) {
                                            CHECK(!suppressed_result
                                                .current_topology);
                                            CHECK(source_after_suppression.empty());
                                            tested_predecessor_stage_suppression =
                                                true;
                                        }
                                        const auto rollback =
                                            suppress_session.undo();
                                        CHECK(rollback.ok());
                                        kernel_occt::OcctSolidModelingKernel
                                            unsuppressed_provider;
                                        const auto restored_eval =
                                            part::evaluatePart(
                                                suppress_session.document(),
                                                unsuppressed_provider);
                                        CHECK(restored_eval.body_status ==
                                            part::BodyEvaluationStatus::
                                                up_to_date);
                                        CHECK(restored_eval.current_topology &&
                                              restored_eval.current_topology
                                                  ->complete());
                                        const auto restored_candidates =
                                            pg01dCurveEdgesIncidentToSemanticPoint(
                                                *restored_eval.current_topology,
                                                curve_address,
                                                *certified_point_address);
                                        CHECK(restored_candidates.size() == 1U);
                                        predecessor_suppression_undo_restored =
                                            true;
                                    } else {
                                        CHECK(suppress_session.document().state() ==
                                              before_suppression);
                                        ++predecessor_suppression_rejections;
                                    }
                                    tested_predecessor_suppression = true;
                                }
                                if (!tested_removed_final_stage) {
                                    // Delete the producing Chamfer Feature,
                                    // not a graphically similar Edge.
                                    // The old after-Chamfer stage cannot
                                    // be read from the new final Body.
                                    auto removed = part::PartDocument::restore(
                                        core::DocumentId::generate(),
                                        trial.document().state());
                                    CHECK(removed.ok());
                                    application::DocumentSession delete_session{
                                        {}, std::move(*removed.document)};
                                    CHECK(delete_session.document()
                                          .body().features.size() == 3U);
                                    const auto chamfer_id =
                                        delete_session.document()
                                            .body().features.back().id;
                                    const auto erase = delete_session.execute(
                                        application::DeleteFeatureCommand{
                                            chamfer_id,
                                            delete_session.document().revision()});
                                    CHECK(erase.ok());
                                    CHECK(delete_session.document()
                                          .body().features.size() == 2U);
                                    kernel_occt::OcctSolidModelingKernel
                                        deleted_provider;
                                    const auto deleted_eval = part::evaluatePart(
                                        delete_session.document(),
                                        deleted_provider);
                                    CHECK(pg01dStrictFinalStageOnePointCandidates(
                                        deleted_eval, ledger.stage,
                                        curve_address,
                                        *certified_point_address).empty());
                                    const auto restore = delete_session.undo();
                                    CHECK(restore.ok());
                                    kernel_occt::OcctSolidModelingKernel
                                        restored_provider;
                                    const auto restored_eval =
                                        part::evaluatePart(
                                            delete_session.document(),
                                            restored_provider);
                                    CHECK(pg01dStrictFinalStageOnePointCandidates(
                                        restored_eval, ledger.stage,
                                        curve_address,
                                        *certified_point_address).size() == 1U);
                                    tested_removed_final_stage = true;
                                }
                                if (!tested_suppressed_final_stage) {
                                    auto disabled = part::PartDocument::restore(
                                        core::DocumentId::generate(),
                                        trial.document().state());
                                    CHECK(disabled.ok());
                                    application::DocumentSession disable_session{
                                        {}, std::move(*disabled.document)};
                                    const auto final_id =
                                        disable_session.document().body()
                                            .features.back().id;
                                    const auto disable = disable_session.execute(
                                        application::SetFeatureSuppressedCommand{
                                            final_id,
                                            disable_session.document().revision(),
                                            true});
                                    CHECK(disable.ok());
                                    kernel_occt::OcctSolidModelingKernel
                                        disabled_provider;
                                    const auto disabled_eval =
                                        part::evaluatePart(
                                            disable_session.document(),
                                            disabled_provider);
                                    CHECK(pg01dStrictFinalStageOnePointCandidates(
                                        disabled_eval, ledger.stage,
                                        curve_address,
                                        *certified_point_address).empty());
                                    CHECK(disable_session.undo().ok());
                                    kernel_occt::OcctSolidModelingKernel
                                        reenabled_provider;
                                    const auto reenabled =
                                        part::evaluatePart(
                                            disable_session.document(),
                                            reenabled_provider);
                                    CHECK(pg01dStrictFinalStageOnePointCandidates(
                                        reenabled, ledger.stage,
                                        curve_address,
                                        *certified_point_address).size() == 1U);
                                    tested_suppressed_final_stage = true;
                                }
                                if (!tested_radical_upstream_matrix) {
                                    // Read-only D2 feasibility across
                                    // *actual rebuilt OCCT* topology after
                                    // substantial parent-Feature changes.
                                    // Do not carry any runtime token from
                                    // one variant to another, and do not
                                    // claim a unique current match proves
                                    // unbroken material segment provenance.
                                    for (const bool reversed :
                                         {false, true}) {
                                        for (const double span :
                                             {0.25, 1.0, 3.0, 7.5,
                                              15.0, 30.0}) {
                                            ++radical_attempts;
                                            auto rebuilt =
                                                part::PartDocument::restore(
                                                    core::DocumentId::generate(),
                                                    trial.document().state());
                                            CHECK(rebuilt.ok());
                                            application::DocumentSession
                                                revised{
                                                    {}, std::move(
                                                        *rebuilt.document)};
                                            kernel_occt::OcctSolidModelingKernel
                                                revised_kernel;
                                            const auto before_change =
                                                revised.document().state();
                                            const auto command =
                                                revised.execute(
                                                    application::
                                                        EditExtrudeFeatureCommand{
                                                            *boss.feature_id,
                                                            revised.document()
                                                                .revision(),
                                                            boss_profile,
                                                            part::ExtrudeOperation::
                                                                add,
                                                            part::
                                                                OneSidedExtrudeExtent{
                                                                    core::
                                                                        LengthValue{
                                                                            span},
                                                                    reversed},
                                                            "D2 radical Add edit"},
                                                    revised_kernel);
                                            if (!command.ok()) {
                                                CHECK(revised.document().state() ==
                                                      before_change);
                                                ++radical_command_rejected;
                                                continue;
                                            }
                                            const auto rebuild =
                                                part::evaluatePart(
                                                    revised.document(),
                                                    revised_kernel);
                                            const auto fresh =
                                                pg01dStrictFinalStageOnePointCandidates(
                                                    rebuild,
                                                    ledger.stage,
                                                    curve_address,
                                                    *certified_point_address);
                                            CHECK(fresh.size() <= 1U);
                                            if (rebuild.body_status !=
                                                    part::BodyEvaluationStatus::
                                                        up_to_date) {
                                                CHECK(fresh.empty());
                                                ++radical_final_unavailable;
                                            } else if (
                                                !rebuild.current_topology ||
                                                rebuild.current_topology->
                                                        stage != ledger.stage) {
                                                CHECK(fresh.empty());
                                                ++radical_stage_missing;
                                            } else if (fresh.empty()) {
                                                ++radical_source_missing;
                                            } else {
                                                ++radical_source_unique;
                                            }
                                        }
                                    }
                                    CHECK(radical_attempts == 12U);
                                    CHECK(radical_attempts ==
                                          radical_command_rejected +
                                          radical_final_unavailable +
                                          radical_stage_missing +
                                          radical_source_missing +
                                          radical_source_unique +
                                          radical_source_ambiguous);
                                    tested_radical_upstream_matrix = true;
                                }
                                if (!tested_upstream_edit) {
                                    // Edit the immediately previous Add
                                    // feature, without editing the Chamfer,
                                    // then resolve ONLY the durable Curve
                                    // and Point addresses. Every rejected
                                    // transaction must leave the authored
                                    // Part untouched.
                                    auto edit_clone =
                                        part::PartDocument::restore(
                                            core::DocumentId::generate(),
                                            trial.document().state());
                                    CHECK(edit_clone.ok());
                                    application::DocumentSession edit_session{
                                        {}, std::move(*edit_clone.document)};
                                    kernel_occt::OcctSolidModelingKernel
                                        edit_kernel;
                                    const auto before_state =
                                        edit_session.document().state();
                                    const auto update = edit_session.execute(
                                        application::EditExtrudeFeatureCommand{
                                            *boss.feature_id,
                                            edit_session.document().revision(),
                                            boss_profile,
                                            part::ExtrudeOperation::add,
                                            part::OneSidedExtrudeExtent{
                                                core::LengthValue{11.0},
                                                false},
                                            "Boss enlarged (D2 study)"},
                                        edit_kernel);
                                    if (update.ok()) {
                                        const auto changed = part::evaluatePart(
                                            edit_session.document(),
                                            edit_kernel);
                                        if (changed.body_status ==
                                                part::BodyEvaluationStatus::
                                                    up_to_date &&
                                            changed.current_topology &&
                                            changed.current_topology->complete()) {
                                            const auto updated_choices =
                                                pg01dCurveEdgesIncidentToSemanticPoint(
                                                    *changed.current_topology,
                                                    curve_address,
                                                    *certified_point_address);
                                            // A valid source can survive or
                                            // become unavailable, but cannot
                                            // be accepted ambiguously.
                                            CHECK(updated_choices.size() <= 1U);
                                            const auto undo_edit =
                                                edit_session.undo();
                                            CHECK(undo_edit.ok());
                                            kernel_occt::OcctSolidModelingKernel
                                                undo_provider;
                                            const auto restored_eval =
                                                part::evaluatePart(
                                                    edit_session.document(),
                                                    undo_provider);
                                            CHECK(restored_eval.body_status ==
                                                part::BodyEvaluationStatus::
                                                    up_to_date);
                                            CHECK(restored_eval.current_topology &&
                                                  restored_eval.current_topology
                                                      ->complete());
                                            const auto restored_choices =
                                                pg01dCurveEdgesIncidentToSemanticPoint(
                                                    *restored_eval.current_topology,
                                                    curve_address,
                                                    *certified_point_address);
                                            CHECK(restored_choices.size() == 1U);
                                            const auto redo_edit =
                                                edit_session.redo();
                                            CHECK(redo_edit.ok());
                                            kernel_occt::OcctSolidModelingKernel
                                                redo_edit_provider;
                                            const auto redo_evaluated =
                                                part::evaluatePart(
                                                    edit_session.document(),
                                                    redo_edit_provider);
                                            CHECK(redo_evaluated.body_status ==
                                                part::BodyEvaluationStatus::
                                                    up_to_date);
                                            CHECK(redo_evaluated.current_topology &&
                                                  redo_evaluated.current_topology
                                                      ->complete());
                                            const auto redo_choices =
                                                pg01dCurveEdgesIncidentToSemanticPoint(
                                                    *redo_evaluated.current_topology,
                                                    curve_address,
                                                    *certified_point_address);
                                            CHECK(redo_choices.size() <= 1U);
                                            CHECK(redo_choices.size() ==
                                                  updated_choices.size());
                                            tested_upstream_edit = true;
                                            tested_upstream_edit_undo_redo = true;
                                            std::cout
                                                << "PG01D_D2_UPSTREAM_EDIT_PROBE"
                                                << " after_edit_candidates="
                                                << updated_choices.size()
                                                << " undo_candidates="
                                                << restored_choices.size()
                                                << " redo_candidates="
                                                << redo_choices.size()
                                                << '\n';
                                        }
                                    } else {
                                        CHECK(edit_session.document().state() ==
                                              before_state);
                                        ++rejected_upstream_edits;
                                    }
                                }
                            } else {
                                ++one_semantic_endpoint_ambiguous;
                            }
                        }
                        std::cout
                            << "PG01D_CHAMFER_SWEEP_REJECT"
                            << " family_realizations=" << family_realizations
                            << " incident_vertices=" << incident_vertices
                            << " certified_endpoints=" << certified_endpoints
                            << " chamfer_source="
                            << candidate.runtime_token.value
                            << " distance=" << distance
                            << " face=" << face.runtime_token.value
                            << " failed_edge="
                            << (detail ? detail->edge.value : 0U)
                            << " kind="
                            << (detail ? static_cast<int>(detail->kind)
                                       : -1)
                            << " accounting="
                            << (detail && detail->accounting_class
                                ? static_cast<int>(*detail->accounting_class)
                                : -1)
                            << " strict_status="
                            << (detail && detail->referenceability
                                ? static_cast<int>(*detail->referenceability)
                                : -1)
                            << " candidates="
                            << (detail ? detail->curve_candidate_count : 0U)
                            << " partition="
                            << (detail && detail->representation_partition)
                            << '\n';
                    } else {
                        ++rejected_other;
                    }
                }
            }
        }
        CHECK(possible_sources > 1U);
        CHECK(committed_chamfers > 1U);
        std::cout
            << "PG01D_CHAMFER_SWEEP_SUMMARY"
            << " possible_sources=" << possible_sources
            << " committed=" << committed_chamfers
            << " preserved_split=" << preserved_split
            << " admitted_faces=" << admitted_faces
            << " blocked_material=" << rejected_material
            << " blocked_other=" << rejected_other
            << " missing_two_semantic_surfaces="
            << missing_endpoint_two_surfaces
            << " missing_three_semantic_surfaces="
            << missing_endpoint_three_surfaces
            << " missing_other_semantic_surfaces="
            << missing_endpoint_other_surfaces
            << " missing_endpoints_inherited="
            << missing_endpoint_inherited
            << " missing_endpoints_new="
            << missing_endpoint_new
            << " missing_endpoints_other_accounting="
            << missing_endpoint_not_accounted
            << " two_surface_pair_unique="
            << two_surface_pair_unique
            << " two_surface_pair_collided="
            << two_surface_pair_collided
            << " one_endpoint_unique="
            << one_semantic_endpoint_unique
            << " one_endpoint_ambiguous="
            << one_semantic_endpoint_ambiguous
            << " one_endpoint_fresh_provider_unique="
            << one_semantic_endpoint_cold_unique
            << " upstream_edited=" << tested_upstream_edit
            << " upstream_edit_undo_redo="
            << tested_upstream_edit_undo_redo
            << " upstream_edit_rejected="
            << rejected_upstream_edits
            << " radical_attempts=" << radical_attempts
            << " radical_command_rejected="
            << radical_command_rejected
            << " radical_final_unavailable="
            << radical_final_unavailable
            << " radical_stage_missing="
            << radical_stage_missing
            << " radical_source_missing="
            << radical_source_missing
            << " radical_source_unique="
            << radical_source_unique
            << " radical_source_ambiguous="
            << radical_source_ambiguous
            << " native_onepoint_probes="
            << native_multipiece_curve_point_probes
            << " native_onepoint_resolved="
            << native_onepoint_resolved
            << " native_onepoint_ambiguous="
            << native_onepoint_ambiguous
            << " old_twopoint_alias_candidates="
            << native_curvepoint_two_point_aliases
            << " disjoint_onepoint_admitted="
            << disjoint_onepoint_admitted
            << " disjoint_old_twopoint_refused="
            << disjoint_old_twopoint_refused
            << " predecessor_suppression_committed="
            << predecessor_suppression_committed
            << " predecessor_suppression_rejected="
            << predecessor_suppression_rejections
            << " predecessor_suppression_undo_restored="
            << predecessor_suppression_undo_restored
            << " predecessor_stage_absent="
            << predecessor_stage_absent
            << " predecessor_stage_survived="
            << predecessor_stage_survived
            << " source_stage_removed_undo_restored="
            << tested_removed_final_stage
            << " source_stage_suppressed_undo_restored="
            << tested_suppressed_final_stage
            << " private_part_committed=0"
            << '\n';
        // Research witness on the accepted immutable Point rule:
        // all observed rejected Edge endpoints are newly generated
        // two-semantic-Surface vertices, NOT inherited stable Points.
        // Never turn these into authored Points or skip these material
        // Edges without a further accepted D2 identity contract.
        CHECK(rejected_material > 0U);
        CHECK(missing_endpoint_two_surfaces == rejected_material);
        CHECK(missing_endpoint_three_surfaces == 0U);
        CHECK(missing_endpoint_other_surfaces == 0U);
        CHECK(missing_endpoint_inherited == 0U);
        CHECK(missing_endpoint_new == rejected_material);
        CHECK(missing_endpoint_not_accounted == 0U);
        CHECK(two_surface_pair_unique +
                  two_surface_pair_collided == rejected_material);
        // Research only: a uniquely resolved semantic Curve + one
        // already certified Point identifies each blocked Edge in THIS
        // synthetic family, also after a fresh native Part rebuild.
        // No authorable branch or v15 schema is introduced here.
        CHECK(two_surface_pair_unique == rejected_material);
        CHECK(two_surface_pair_collided == 0U);
        CHECK(one_semantic_endpoint_unique == rejected_material);
        CHECK(one_semantic_endpoint_ambiguous == 0U);
        CHECK(one_semantic_endpoint_cold_unique == rejected_material);
        CHECK(tested_ambiguous_incidence);
        CHECK(tested_missing_reference);
        CHECK(tested_native_file_reopen);
        CHECK(tested_undo_redo);
        CHECK(tested_upstream_edit);
        CHECK(tested_upstream_edit_undo_redo);
        CHECK(tested_radical_upstream_matrix);
        CHECK(radical_attempts == 12U);
        CHECK(native_multipiece_curve_point_probes > 0U);
        // The independent OCCT matrix has no naturally ambiguous
        // Curve+Point pairs; this is a bounded positive observation,
        // NOT a proof that a native counterexample cannot exist.
        // The separate injected double-incidence test still requires
        // the candidate selector to return the entire ambiguous set.
        CHECK(native_onepoint_resolved > 0U);
        CHECK(native_onepoint_ambiguous == 0U);
        CHECK(tested_predecessor_suppression);
        CHECK(predecessor_suppression_committed ||
              predecessor_suppression_rejections > 0U);
        CHECK(!predecessor_suppression_committed ||
              predecessor_suppression_undo_restored);
        CHECK(tested_wrong_stage_refusal);
        CHECK(native_curvepoint_two_point_aliases > 0U);
        CHECK(disjoint_onepoint_admitted > 0U);
        CHECK(disjoint_old_twopoint_refused ==
              native_curvepoint_two_point_aliases);
        CHECK(disjoint_onepoint_admitted +
                  disjoint_old_twopoint_refused ==
              native_onepoint_resolved);
        CHECK(predecessor_suppression_committed);
        CHECK(predecessor_suppression_undo_restored);
        CHECK(tested_removed_final_stage);
        CHECK(tested_suppressed_final_stage);
        // In this actual OCCT fixture, predecessor suppression makes
        // the exact final source Body unavailable; no diagnostic
        // resolved-prefix catalog may substitute for final truth.
        CHECK(tested_predecessor_stage_suppression);
        CHECK(predecessor_stage_absent == 1U);
        CHECK(predecessor_stage_survived == 0U);
        // Controlled OCCT regression: reversed Add invalidates the
        // dependent Chamfer, and six valid forward variants retain one
        // strict stage-local candidate. These are *not* sufficient to
        // certify per-segment provenance under arbitrary geometry edits.
        CHECK(radical_command_rejected == 0U);
        CHECK(radical_final_unavailable == 6U);
        CHECK(radical_stage_missing == 0U);
        CHECK(radical_source_missing == 0U);
        CHECK(radical_source_unique == 6U);
        CHECK(radical_source_ambiguous == 0U);
    }

    const auto contribution =
        part::currentFeatureContribution(
            topology,
            *boss.feature_id);
    CHECK(contribution.valid());
    CHECK(!contribution.faces.empty());

    const auto partition_count =
        std::count_if(
            topology.edges.begin(),
            topology.edges.end(),
            [](const auto& edge) {
                return edge.representation_partition &&
                       edge.accounting_class ==
                           part::TopologyAccountingClass::
                               known_representation_artifact &&
                       edge.referenceability ==
                           kernel::ReferenceStatus::
                               unsupported;
            });
    CHECK(partition_count >= 1);

    // PG-01D D1: the Surface carrier remains one semantic Surface even
    // where it spans two distinct current bounded Face realizations.
    // Read *each* native Face's real oriented wires independently; do
    // not reinterpret the carrier as one guessed Face perimeter.
    CHECK(final_eval.body_solid);
    std::vector<kernel::RuntimeFaceToken> observed_faces;
    std::vector<part::FeatureFaceAddress> strict_bounded_addresses;
    std::size_t carrier_only_faces = 0U;
    std::size_t admitted_strict_faces = 0U;
    std::size_t boundary_material_uses = 0U;
    std::size_t boundary_partition_uses = 0U;
    std::size_t boundary_other_nonmaterial_uses = 0U;
    // E0 Face Boundary is still a read-only native incidence
    // experiment: one exact same-Surface carrier, several bounded
    // Faces, current-scoped real provider Edge tokens only.
    std::map<std::uint64_t, std::size_t>
        grouped_same_surface_native_edge_uses;
    // Native per-Face oriented uses retained separately: opposite
    // orientation is necessary before same-Surface cancellation.
    std::map<std::uint64_t, std::vector<bool>>
        grouped_same_surface_native_edge_orientations;
    std::size_t blocked_face_admissions = 0U;
    std::size_t accepted_face_admissions = 0U;
    for (const auto token : current_side->current_faces) {
        CHECK(std::find(
            observed_faces.begin(),
            observed_faces.end(), token) == observed_faces.end());
        observed_faces.push_back(token);
        const auto* face = [&]() -> const part::BodyFaceTopologyRecord* {
            for (const auto& record : topology.faces) {
                if (record.runtime_token == token) return &record;
            }
            return nullptr;
        }();
        CHECK(face != nullptr);
        CHECK(face->surface_candidates.size() == 1U);
        CHECK(face->surface_candidates.front() ==
              current_side->address);
        if (face->semantic_address) {
            // Each bounded Face is independent current semantic meaning
            // even when the shared Surface carrier cannot name one Face.
            CHECK(std::find(
                strict_bounded_addresses.begin(),
                strict_bounded_addresses.end(),
                *face->semantic_address) ==
                strict_bounded_addresses.end());
            strict_bounded_addresses.push_back(
                *face->semantic_address);
            ++admitted_strict_faces;
        } else {
            // Surface-carrier support alone does not authorize a Face.
            ++carrier_only_faces;
        }
        const auto scoped =
            provider.bindFaceToBody(final_eval.body_solid, token);
        CHECK(scoped && scoped->valid());
        const auto native =
            provider.queryFaceBoundary(final_eval.body_solid, *scoped);
        CHECK(native.ok());
        std::size_t outer_wires = 0U;
        bool all_members_material = true;
        for (const auto& wire : native.wires) {
            CHECK(wire.valid());
            if (wire.outer) ++outer_wires;
            for (const auto& occurrence : wire.edges) {
                CHECK(occurrence.valid());
                ++grouped_same_surface_native_edge_uses[
                    occurrence.edge.value];
                grouped_same_surface_native_edge_orientations[
                    occurrence.edge.value].push_back(
                        occurrence.reversed);
                const auto edge = std::find_if(
                    topology.edges.begin(),
                    topology.edges.end(),
                    [&occurrence](const auto& record) {
                        return record.runtime_token ==
                               occurrence.edge;
                    });
                CHECK(edge != topology.edges.end());
                const auto author =
                    part::authorMaterialEdgeReference(
                        topology, occurrence.edge);
                if (edge->representation_partition ||
                    edge->periodic_seam) {
                    // Native wires contain representation artifacts.
                    // These are never material Sketch source references.
                    CHECK(!author.ok());
                    all_members_material = false;
                    ++boundary_partition_uses;
                } else if (author.ok()) {
                    CHECK(author.reference &&
                          author.reference->stage == topology.stage);
                    ++boundary_material_uses;
                } else {
                    all_members_material = false;
                    ++boundary_other_nonmaterial_uses;
                }
            }
        }
        CHECK(outer_wires == 1U);

        const auto inspected =
            part::inspectMaterialFaceBoundary(
                final_eval.features.back(), token, provider);
        if (!face->semantic_address) {
            CHECK(!inspected.ok());
            CHECK(inspected.status ==
                  part::MaterialFaceBoundaryStatus::face_not_strict);
            ++blocked_face_admissions;
        } else if (!all_members_material) {
            // Do not partly author a bounded Face whose native wire
            // contains a nonmaterial partition/seam occurrence.
            CHECK(!inspected.ok());
            CHECK(inspected.status ==
                  part::MaterialFaceBoundaryStatus::
                      material_edge_unavailable);
            ++blocked_face_admissions;
        } else {
            CHECK(inspected.ok());
            CHECK(inspected.bounded_face ==
                  face->semantic_address);
            CHECK(inspected.wires.size() ==
                  native.wires.size());
            for (std::size_t index = 0U;
                 index < native.wires.size(); ++index) {
                CHECK(inspected.wires[index].outer ==
                      native.wires[index].outer);
                CHECK(inspected.wires[index].edges.size() ==
                      native.wires[index].edges.size());
            }
            ++accepted_face_admissions;
        }
    }
    CHECK(observed_faces.size() ==
          current_side->current_faces.size());
    CHECK(carrier_only_faces + admitted_strict_faces ==
          observed_faces.size());
    CHECK(strict_bounded_addresses.size() ==
          admitted_strict_faces);
    // The Surface reference is ambiguous for bounded Face selection,
    // but individual Face addresses can still be strict and distinct.
    CHECK(admitted_strict_faces >= 2U);
    CHECK(boundary_material_uses > 0U);
    std::cout
        << "PG01D_D1_SPLIT_FACE_BOUNDARY_PASS"
        << " face_realizations=" << observed_faces.size()
        << " carrier_only=" << carrier_only_faces
        << " strict_face=" << admitted_strict_faces
        << " material_uses=" << boundary_material_uses
        << " rejected_partition_or_seam="
        << boundary_partition_uses
        << " other_nonmaterial="
        << boundary_other_nonmaterial_uses
        << " blocked_admissions="
        << blocked_face_admissions
        << " accepted_admissions="
        << accepted_face_admissions
        << '\n';
    // The former shared-Surface partition Edge is an actual native
    // boundary use and must never be promoted into material identity.
    CHECK(boundary_partition_uses >= 1U);
    CHECK(blocked_face_admissions >= 1U);

    // Face Boundary E0: cancel ONLY certified internal representation
    // edges from this one strictly-equal Surface carrier. Every such
    // partition must occur exactly twice in the real oriented Face
    // wire ledger. Every remaining source must be an individually
    // referenceable material Edge occurring once on this region's
    // perimeter. This is not yet a contour/wire reconstruction or
    // an authorization to merge other coplanar Surfaces. The
    // surviving set still needs an exact single-cycle proof below;
    // outer/hole classification for general regions is not implied.
    std::size_t certified_internal_partitions = 0U;
    std::vector<part::MaterialEdgeReference>
        certified_perimeter_sources;
    std::vector<kernel::RuntimeEdgeToken>
        certified_perimeter_tokens;
    for (const auto& [value, occurrences] :
         grouped_same_surface_native_edge_uses) {
        const auto token = kernel::RuntimeEdgeToken{value};
        const auto found = std::find_if(
            topology.edges.begin(), topology.edges.end(),
            [token](const auto& edge) {
                return edge.runtime_token == token;
            });
        CHECK(found != topology.edges.end());
        if (found->representation_partition) {
            CHECK(found->accounting_class ==
                  part::TopologyAccountingClass::
                      known_representation_artifact);
            CHECK(occurrences == 2U);
            const auto oriented =
                grouped_same_surface_native_edge_orientations.find(
                    token.value);
            CHECK(oriented !=
                  grouped_same_surface_native_edge_orientations.end());
            CHECK(oriented->second.size() == 2U);
            CHECK(oriented->second[0] != oriented->second[1]);
            CHECK(!part::authorMaterialEdgeReference(
                topology, token).ok());
            ++certified_internal_partitions;
            continue;
        }
        // A periodic seam is a separate representation artifact and
        // requires its own positive incidence proof on curved faces.
        CHECK(!found->periodic_seam);
        const auto material =
            part::authorMaterialEdgeReference(
                topology, token);
        CHECK(material.ok());
        CHECK(material.reference);
        CHECK(occurrences == 1U);
        const auto oriented =
            grouped_same_surface_native_edge_orientations.find(
                token.value);
        CHECK(oriented !=
              grouped_same_surface_native_edge_orientations.end());
        CHECK(oriented->second.size() == 1U);
        certified_perimeter_sources.push_back(
            *material.reference);
        certified_perimeter_tokens.push_back(token);
    }
    CHECK(certified_internal_partitions >= 1U);
    CHECK(!certified_perimeter_sources.empty());
    std::sort(
        certified_perimeter_sources.begin(),
        certified_perimeter_sources.end());
    CHECK(std::adjacent_find(
        certified_perimeter_sources.begin(),
        certified_perimeter_sources.end()) ==
        certified_perimeter_sources.end());
    // E0 positive proof for THIS planar split-carrier fixture only:
    // retain exact current native provider Vertex/Edge incidences; never
    // pair coordinates or infer missing geometry. Degree two alone could
    // allow several DISCONNECTED cycles, which is insufficient for this
    // fixture's expected one contiguous material perimeter.
    std::map<std::uint64_t, std::vector<std::uint64_t>>
        perimeter_vertex_edges;
    std::map<std::uint64_t, std::vector<std::uint64_t>>
        perimeter_edge_vertices;
    for (const auto& vertex : topology.vertices) {
        std::vector<std::uint64_t> incident;
        for (const auto token : certified_perimeter_tokens) {
            if (std::find(
                    vertex.incident_material_edges.begin(),
                    vertex.incident_material_edges.end(),
                    token) != vertex.incident_material_edges.end()) {
                incident.push_back(token.value);
            }
        }
        if (incident.empty()) continue;
        CHECK(vertex.runtime_token.value != 0U);
        CHECK(incident.size() == 2U);
        CHECK(incident[0] != incident[1]);
        for (const auto edge : incident) {
            perimeter_edge_vertices[edge].push_back(
                vertex.runtime_token.value);
        }
        CHECK(perimeter_vertex_edges.emplace(
            vertex.runtime_token.value,
            std::move(incident)).second);
    }
    CHECK(!perimeter_edge_vertices.empty());
    CHECK(perimeter_edge_vertices.size() ==
          certified_perimeter_tokens.size());
    CHECK(perimeter_vertex_edges.size() ==
          certified_perimeter_tokens.size());
    for (const auto& [edge, endpoints] :
         perimeter_edge_vertices) {
        CHECK(endpoints.size() == 2U);
        CHECK(endpoints[0] != endpoints[1]);
        (void)edge;
    }

    // Reconstruct and exhaust the SINGLE closed cycle starting from the
    // lowest current Edge token. A disconnected second cycle would cause
    // this walk to return to its start too early and fail closed.
    const auto start_edge =
        perimeter_edge_vertices.begin()->first;
    const auto start_vertex =
        perimeter_edge_vertices.begin()->second.front();
    auto current_edge = start_edge;
    auto current_vertex = start_vertex;
    std::set<std::uint64_t> visited_edges;
    for (std::size_t walk = 0U;
         walk < perimeter_edge_vertices.size();
         ++walk) {
        CHECK(visited_edges.insert(current_edge).second);
        const auto edge =
            perimeter_edge_vertices.find(current_edge);
        CHECK(edge != perimeter_edge_vertices.end());
        const auto& endpoints = edge->second;
        CHECK(endpoints[0] == current_vertex ||
              endpoints[1] == current_vertex);
        const auto next_vertex =
            endpoints[0] == current_vertex
                ? endpoints[1]
                : endpoints[0];
        const auto vertex =
            perimeter_vertex_edges.find(next_vertex);
        CHECK(vertex != perimeter_vertex_edges.end());
        const auto& neighbors = vertex->second;
        CHECK(neighbors.size() == 2U);
        CHECK(neighbors[0] == current_edge ||
              neighbors[1] == current_edge);
        const auto next_edge =
            neighbors[0] == current_edge
                ? neighbors[1]
                : neighbors[0];
        if (walk + 1U == perimeter_edge_vertices.size()) {
            CHECK(next_vertex == start_vertex);
            CHECK(next_edge == start_edge);
        } else {
            CHECK(next_edge != start_edge);
        }
        current_vertex = next_vertex;
        current_edge = next_edge;
    }
    CHECK(visited_edges.size() ==
          certified_perimeter_tokens.size());
    // E0 negative boundary distinction for this real Add-continuation
    // Body: a native Edge shared with a Face of ANOTHER semantic Surface
    // must remain a material perimeter member of the selected carrier.
    // Do not cancel it as a representation partition merely because
    // OCCT reports the same Edge on two adjacent Face wires.
    // This is current-stage equality of explicit Surface addresses,
    // not an inference from normal/coplanarity/curve geometry.
    std::size_t foreign_carrier_shared_material_edges = 0U;
    std::vector<part::FeatureSurfaceAddress>
        observed_distinct_neighbor_carriers;
    std::set<std::uint64_t> observed_neighbor_material_tokens;
    for (const auto& foreign_face : topology.faces) {
        if (foreign_face.surface_candidates.size() != 1U ||
            foreign_face.surface_candidates.front() ==
                current_side->address) {
            continue;
        }
        const auto& other_carrier =
            foreign_face.surface_candidates.front();
        const auto scoped = provider.bindFaceToBody(
            final_eval.body_solid, foreign_face.runtime_token);
        CHECK(scoped && scoped->valid());
        const auto raw = provider.queryFaceBoundaryAnySurface(
            final_eval.body_solid, *scoped);
        CHECK(raw.ok());
        for (const auto& wire : raw.wires) {
            for (const auto& use : wire.edges) {
                if (std::find(
                        certified_perimeter_tokens.begin(),
                        certified_perimeter_tokens.end(),
                        use.edge) ==
                    certified_perimeter_tokens.end()) {
                    continue;
                }
                const auto in_selected =
                    grouped_same_surface_native_edge_uses.find(
                        use.edge.value);
                CHECK(in_selected !=
                      grouped_same_surface_native_edge_uses.end());
                CHECK(in_selected->second == 1U);
                const auto material =
                    part::authorMaterialEdgeReference(
                        topology, use.edge);
                CHECK(material.ok());
                const auto edge = std::find_if(
                    topology.edges.begin(),
                    topology.edges.end(),
                    [&use](const auto& record) {
                        return record.runtime_token == use.edge;
                    });
                CHECK(edge != topology.edges.end());
                CHECK(!edge->representation_partition);
                CHECK(!edge->periodic_seam);
                if (observed_neighbor_material_tokens.insert(
                        use.edge.value).second) {
                    ++foreign_carrier_shared_material_edges;
                }
                if (std::find(
                        observed_distinct_neighbor_carriers.begin(),
                        observed_distinct_neighbor_carriers.end(),
                        other_carrier) ==
                    observed_distinct_neighbor_carriers.end()) {
                    observed_distinct_neighbor_carriers.push_back(
                        other_carrier);
                }
            }
        }
    }
    CHECK(foreign_carrier_shared_material_edges >= 1U);
    CHECK(!observed_distinct_neighbor_carriers.empty());
    for (const auto& candidate :
         observed_distinct_neighbor_carriers) {
        CHECK(candidate != current_side->address);
    }
    std::cout
        << "PG01D_FACE_BOUNDARY_E0_SPLIT_CARRIER_PERIMETER_PASS"
        << " same_semantic_surface=1"
        << " split_faces=" << observed_faces.size()
        << " cancelled_internal_partitions="
        << certified_internal_partitions
        << " partition_opposite_oriented_uses=1"
        << " distinct_material_perimeter_edges="
        << certified_perimeter_sources.size()
        << " single_closed_component=1"
        << " foreign_surface_carriers="
        << observed_distinct_neighbor_carriers.size()
        << " foreign_carrier_shared_material_edges="
        << foreign_carrier_shared_material_edges
        << " source_proximity_guessing=0"
        << '\n';
    const auto invalid_face =
        part::inspectMaterialFaceBoundary(
            final_eval.features.back(),
            kernel::RuntimeFaceToken{}, provider);
    CHECK(invalid_face.status ==
          part::MaterialFaceBoundaryStatus::face_unavailable);
    auto detached_stage = final_eval.features.back();
    detached_stage.result_solid.reset();
    const auto invalid_stage =
        part::inspectMaterialFaceBoundary(
            detached_stage,
            observed_faces.front(), provider);
    CHECK(invalid_stage.status ==
          part::MaterialFaceBoundaryStatus::invalid_stage);

    const part::SurfaceReference final_side_ref{
        topology.stage,
        current_side->address};
    const auto side_support =
        part::partSketchSupportForBodyPlanarSurface(
            final_side_ref);
    CHECK(side_support.has_value());

    const auto new_sketch =
        session.execute(
            application::CreatePartSketchOnSupportCommand{
                *side_support,
                session.document().revision()},
            &provider);
    CHECK(new_sketch.ok());
    CHECK(new_sketch.sketch_id.has_value());

    // PG-01D E0: actual Cut through the LOWER portion of the same
    // previously split +X material Surface. This is deliberately not
    // a guessed stitched Face: source Sketch is explicitly supported by
    // the inherited same-stage semantic Surface, and OCCT re-evaluates
    // the full Part with a real circular hole in one bounded fragment.
    CHECK(current_side->canonical_frame.has_value());
    CHECK(current_side->canonical_frame->normal.x > 0.9);
    const auto hole_uv = projectToFrame(
        *current_side->canonical_frame,
        kernel::Point3{40.0, 15.0, 5.0});
    const auto hole_circle = session.execute(
        application::AddSketchCircleCommand{
            *new_sketch.sketch_id,
            hole_uv,
            2.5,
            sketch::EntityRole::regular});
    CHECK(hole_circle.ok());
    const auto* hole_host =
        session.document().findSketch(*new_sketch.sketch_id);
    CHECK(hole_host);
    const auto hole_regions =
        sketch::analyzeRegions(hole_host->model);
    CHECK(hole_regions.complete());
    CHECK(hole_regions.regions.size() == 1U);
    const auto hole_intent =
        part::makeProfileRegionIntent(
            hole_regions.regions.front());
    CHECK(hole_intent.has_value());
    const auto hole_profile = session.execute(
        application::CreateProfileCommand{
            *new_sketch.sketch_id,
            session.document().revision(),
            *hole_intent});
    CHECK(hole_profile.ok() && hole_profile.profile_id);
    const auto hole_cut = session.execute(
        application::CreateExtrudeFeatureCommand{
            *hole_profile.profile_id,
            session.document().revision(),
            part::ExtrudeOperation::cut,
            part::OneSidedExtrudeExtent{
                core::LengthValue{45.0},
                true},
            "PG01D split-carrier lower side through-hole"},
        provider);
    CHECK(hole_cut.ok() && hole_cut.feature_id);

    const auto drilled =
        part::evaluatePart(session.document(), provider);
    CHECK(drilled.body_status ==
          part::BodyEvaluationStatus::up_to_date);
    CHECK(drilled.body_solid && drilled.current_topology);
    CHECK(drilled.current_topology->complete());
    const auto& drilled_catalog = *drilled.current_topology;
    CHECK(drilled_catalog.stage.feature_id &&
          *drilled_catalog.stage.feature_id == *hole_cut.feature_id);
    const auto drilled_side = std::find_if(
        drilled_catalog.surfaces.begin(),
        drilled_catalog.surfaces.end(),
        [address = continued_side->address](
            const auto& surface) {
            return surface.address == address;
        });
    CHECK(drilled_side != drilled_catalog.surfaces.end());
    CHECK(drilled_side->status ==
          kernel::ReferenceStatus::resolved);
    CHECK(drilled_side->current_faces.size() >= 2U);

    // Native oriented wire accounting within the ONE certified carrier.
    // A real hole is identified by the OCCT Face's INNER wire, not by
    // geometry proximity, Circle radius or viewer outline. Partition
    // cancellation is allowed only for exactly two opposite-oriented
    // native uses on different fragments of this same Surface.
    struct OrientedRegionUse final {
        bool reversed{};
        bool inner{};
    };
    std::map<std::uint64_t, std::vector<OrientedRegionUse>>
        drilled_native_uses;
    std::size_t drilled_native_outer_wires = 0U;
    std::size_t drilled_native_hole_wires = 0U;
    std::size_t drilled_faces_with_holes = 0U;
    for (const auto token : drilled_side->current_faces) {
        const auto face = std::find_if(
            drilled_catalog.faces.begin(),
            drilled_catalog.faces.end(),
            [token](const auto& item) {
                return item.runtime_token == token;
            });
        CHECK(face != drilled_catalog.faces.end());
        CHECK(face->surface_candidates.size() == 1U);
        CHECK(face->surface_candidates.front() ==
              drilled_side->address);
        const auto scoped =
            provider.bindFaceToBody(drilled.body_solid, token);
        CHECK(scoped && scoped->valid());
        const auto native = provider.queryFaceBoundaryAnySurface(
            drilled.body_solid, *scoped);
        CHECK(native.ok());
        std::size_t face_outer_wires = 0U;
        std::size_t face_hole_wires = 0U;
        for (const auto& wire : native.wires) {
            if (wire.outer) {
                ++face_outer_wires;
                ++drilled_native_outer_wires;
            } else {
                ++face_hole_wires;
                ++drilled_native_hole_wires;
            }
            for (const auto& use : wire.edges) {
                CHECK(use.valid());
                drilled_native_uses[use.edge.value].push_back(
                    OrientedRegionUse{
                        use.reversed,
                        !wire.outer});
            }
        }
        CHECK(face_outer_wires == 1U);
        if (face_hole_wires != 0U) {
            ++drilled_faces_with_holes;
        }
    }
    CHECK(drilled_native_outer_wires ==
          drilled_side->current_faces.size());
    CHECK(drilled_native_hole_wires == 1U);
    CHECK(drilled_faces_with_holes == 1U);

    std::size_t drilled_partition_edges = 0U;
    std::size_t drilled_outer_material = 0U;
    std::size_t drilled_hole_material = 0U;
    std::vector<part::MaterialEdgeReference>
        drilled_unique_material_sources;
    for (const auto& [value, uses] : drilled_native_uses) {
        const kernel::RuntimeEdgeToken token{value};
        const auto edge = std::find_if(
            drilled_catalog.edges.begin(),
            drilled_catalog.edges.end(),
            [token](const auto& item) {
                return item.runtime_token == token;
            });
        CHECK(edge != drilled_catalog.edges.end());
        const auto material = part::authorMaterialEdgeReference(
            drilled_catalog, token);
        if (edge->representation_partition) {
            CHECK(edge->accounting_class ==
                  part::TopologyAccountingClass::
                      known_representation_artifact);
            CHECK(!edge->periodic_seam);
            CHECK(!material.ok());
            CHECK(uses.size() == 2U);
            CHECK(uses[0].reversed != uses[1].reversed);
            CHECK(!uses[0].inner && !uses[1].inner);
            ++drilled_partition_edges;
            continue;
        }
        // Any periodic seam or unknown nonmaterial source is a STOP
        // in this planar case, never an omitted partial material link.
        CHECK(!edge->periodic_seam);
        CHECK(material.ok() && material.reference);
        CHECK(material.reference->stage ==
              drilled_catalog.stage);
        CHECK(uses.size() == 1U);
        drilled_unique_material_sources.push_back(
            *material.reference);
        if (uses.front().inner) {
            ++drilled_hole_material;
        } else {
            ++drilled_outer_material;
        }
    }
    CHECK(drilled_partition_edges >= 1U);
    CHECK(drilled_hole_material >= 1U);
    CHECK(drilled_outer_material >= 1U);
    std::sort(
        drilled_unique_material_sources.begin(),
        drilled_unique_material_sources.end());
    CHECK(std::adjacent_find(
        drilled_unique_material_sources.begin(),
        drilled_unique_material_sources.end()) ==
        drilled_unique_material_sources.end());
    // Stronger E0 proof for THIS single-hole planar Surface carrier:
    // removing certified internal partition uses must leave exactly
    // one connected native material outer perimeter, plus exactly one
    // separate, closed native inner Circle wire. We never infer a
    // connection from endpoint coordinates or curve proximity. The
    // inner Circle is a full periodic native Edge and is intentionally
    // NOT forced to have two distinct catalog Vertex endpoints.
    std::set<std::uint64_t> outer_material_tokens;
    std::set<std::uint64_t> hole_material_tokens;
    for (const auto& [value, uses] : drilled_native_uses) {
        CHECK(!uses.empty());
        const auto edge = std::find_if(
            drilled_catalog.edges.begin(),
            drilled_catalog.edges.end(),
            [value](const auto& item) {
                return item.runtime_token.value == value;
            });
        CHECK(edge != drilled_catalog.edges.end());
        if (edge->representation_partition) continue;
        CHECK(uses.size() == 1U);
        if (uses.front().inner) {
            CHECK(hole_material_tokens.insert(value).second);
        } else {
            CHECK(outer_material_tokens.insert(value).second);
        }
    }
    CHECK(outer_material_tokens.size() == drilled_outer_material);
    CHECK(hole_material_tokens.size() == drilled_hole_material);
    CHECK(hole_material_tokens.size() == 1U);
    CHECK(outer_material_tokens.count(
              *hole_material_tokens.begin()) == 0U);
    const auto hole_edge = std::find_if(
        drilled_catalog.edges.begin(),
        drilled_catalog.edges.end(),
        [hole = *hole_material_tokens.begin()](const auto& item) {
            return item.runtime_token.value == hole;
        });
    CHECK(hole_edge != drilled_catalog.edges.end());
    CHECK(hole_edge->curve_kind == kernel::CurveKind::circle);
    CHECK(!hole_edge->periodic_seam);
    CHECK(!hole_edge->representation_partition);

    // The exact Part catalog's native Vertex -> material Edge incidence
    // builds an undirected graph for the *outer* material members only.
    // Every Vertex on this boundary must have degree two, every member
    // exactly two distinct current Vertices, and every member must be
    // exhausted by one closed traversal (not several isolated cycles).
    std::map<std::uint64_t, std::vector<std::uint64_t>>
        outer_vertex_neighbors;
    std::map<std::uint64_t, std::vector<std::uint64_t>>
        outer_edge_endpoints;
    for (const auto& vertex : drilled_catalog.vertices) {
        std::vector<std::uint64_t> outer_incident;
        for (const auto edge : vertex.incident_material_edges) {
            if (outer_material_tokens.count(edge.value)) {
                outer_incident.push_back(edge.value);
            }
        }
        if (outer_incident.empty()) continue;
        CHECK(vertex.runtime_token.valid());
        CHECK(outer_incident.size() == 2U);
        CHECK(outer_incident[0] != outer_incident[1]);
        for (const auto edge : outer_incident) {
            outer_edge_endpoints[edge].push_back(
                vertex.runtime_token.value);
        }
        CHECK(outer_vertex_neighbors.emplace(
            vertex.runtime_token.value,
            std::move(outer_incident)).second);
    }
    CHECK(!outer_edge_endpoints.empty());
    CHECK(outer_edge_endpoints.size() ==
          outer_material_tokens.size());
    CHECK(outer_vertex_neighbors.size() ==
          outer_material_tokens.size());
    for (const auto& [edge, endpoints] : outer_edge_endpoints) {
        CHECK(outer_material_tokens.count(edge) == 1U);
        CHECK(endpoints.size() == 2U);
        CHECK(endpoints[0] != endpoints[1]);
    }
    const auto outer_start_edge =
        outer_edge_endpoints.begin()->first;
    const auto outer_start_vertex =
        outer_edge_endpoints.begin()->second.front();
    auto active_edge = outer_start_edge;
    auto active_vertex = outer_start_vertex;
    std::set<std::uint64_t> visited_outer_edges;
    for (std::size_t step = 0U;
         step < outer_material_tokens.size();
         ++step) {
        CHECK(visited_outer_edges.insert(active_edge).second);
        const auto current =
            outer_edge_endpoints.find(active_edge);
        CHECK(current != outer_edge_endpoints.end());
        const auto& ends = current->second;
        CHECK(ends[0] == active_vertex ||
              ends[1] == active_vertex);
        const auto next_vertex =
            ends[0] == active_vertex ? ends[1] : ends[0];
        const auto at_vertex =
            outer_vertex_neighbors.find(next_vertex);
        CHECK(at_vertex != outer_vertex_neighbors.end());
        const auto& neighbors = at_vertex->second;
        CHECK(neighbors.size() == 2U);
        CHECK(neighbors[0] == active_edge ||
              neighbors[1] == active_edge);
        const auto next_edge =
            neighbors[0] == active_edge
                ? neighbors[1] : neighbors[0];
        if (step + 1U == outer_material_tokens.size()) {
            CHECK(next_vertex == outer_start_vertex);
            CHECK(next_edge == outer_start_edge);
        } else {
            CHECK(next_edge != outer_start_edge);
        }
        active_edge = next_edge;
        active_vertex = next_vertex;
    }
    CHECK(visited_outer_edges == outer_material_tokens);
    std::cout
        << "PG01D_FACE_BOUNDARY_E0_OUTER_HOLE_COMPONENT_PROOF"
        << " native_outer_cycles=1"
        << " native_closed_circle_holes=1"
        << " outer_edges=" << outer_material_tokens.size()
        << " hole_edges=" << hole_material_tokens.size()
        << " exact_vertex_incidence=1"
        << " proximity_healing=0"
        << '\n';
    // E0 multi-hole continuation: perform a SECOND genuine through-Cut
    // from a NEW Sketch supported by the current, post-first-Cut stage.
    // The holes occupy different fragments of the same inherited planar
    // Surface, avoiding a single bounded Face / one-hole shortcut.
    CHECK(drilled_side->canonical_frame.has_value());
    const part::SurfaceReference drilled_side_ref{
        drilled_catalog.stage, drilled_side->address};
    const auto second_support =
        part::partSketchSupportForBodyPlanarSurface(
            drilled_side_ref);
    CHECK(second_support.has_value());
    const auto second_sketch = session.execute(
        application::CreatePartSketchOnSupportCommand{
            *second_support,
            session.document().revision()},
        &provider);
    CHECK(second_sketch.ok() && second_sketch.sketch_id);
    const auto second_hole_uv = projectToFrame(
        *drilled_side->canonical_frame,
        kernel::Point3{40.0, 15.0, 15.0});
    CHECK(session.execute(application::AddSketchCircleCommand{
        *second_sketch.sketch_id,
        second_hole_uv,
        2.0,
        sketch::EntityRole::regular}).ok());
    const auto* second_host =
        session.document().findSketch(*second_sketch.sketch_id);
    CHECK(second_host);
    const auto second_regions =
        sketch::analyzeRegions(second_host->model);
    CHECK(second_regions.complete());
    CHECK(second_regions.regions.size() == 1U);
    const auto second_intent =
        part::makeProfileRegionIntent(
            second_regions.regions.front());
    CHECK(second_intent.has_value());
    const auto second_profile = session.execute(
        application::CreateProfileCommand{
            *second_sketch.sketch_id,
            session.document().revision(),
            *second_intent});
    CHECK(second_profile.ok() && second_profile.profile_id);
    const auto second_cut = session.execute(
        application::CreateExtrudeFeatureCommand{
            *second_profile.profile_id,
            session.document().revision(),
            part::ExtrudeOperation::cut,
            part::OneSidedExtrudeExtent{
                core::LengthValue{45.0},
                true},
            "PG01D split-carrier upper side through-hole"},
        provider);
    CHECK(second_cut.ok() && second_cut.feature_id);

    const auto twice_drilled =
        part::evaluatePart(session.document(), provider);
    CHECK(twice_drilled.body_status ==
          part::BodyEvaluationStatus::up_to_date);
    CHECK(twice_drilled.body_solid);
    CHECK(twice_drilled.current_topology);
    CHECK(twice_drilled.current_topology->complete());
    const auto& two_catalog = *twice_drilled.current_topology;
    CHECK(two_catalog.stage.feature_id &&
          *two_catalog.stage.feature_id == *second_cut.feature_id);
    const auto two_side = std::find_if(
        two_catalog.surfaces.begin(),
        two_catalog.surfaces.end(),
        [address = continued_side->address](
            const auto& surface) {
            return surface.address == address;
        });
    CHECK(two_side != two_catalog.surfaces.end());
    CHECK(two_side->status == kernel::ReferenceStatus::resolved);
    CHECK(two_side->current_faces.size() >= 2U);

    // Native Face-wire membership and exact Part Edge source identity.
    // Every actual hole belongs to a DIFFERENT current Face fragment;
    // internal same-carrier partitions have two opposite oriented uses.
    struct TwoHoleUse final {
        bool reversed{};
        bool inner{};
        kernel::RuntimeFaceToken face;
    };
    std::map<std::uint64_t, std::vector<TwoHoleUse>>
        two_native_uses;
    // Preserve the provider's native ORDER and ORIENTATION of each
    // Face wire, not merely its set of material Edge tokens.
    std::vector<std::vector<kernel::FaceBoundaryEdgeUse>>
        two_face_outer_cycles;
    std::vector<std::vector<kernel::FaceBoundaryEdgeUse>>
        two_face_hole_cycles;
    std::set<std::uint64_t> two_hole_face_tokens;
    // A strict directed closed wire has one live OCCT start/end
    // native Vertex per oriented occurrence. Pairwise shared XYZ
    // or mere unsigned Vertex adjacency is NOT sufficient.
    const auto native_directed_closed =
        [](const std::vector<kernel::FaceBoundaryEdgeUse>& uses) {
            if (uses.empty()) return false;
            for (std::size_t i = 0U; i < uses.size(); ++i) {
                const auto& a = uses[i];
                const auto& next = uses[
                    (i + 1U) % uses.size()];
                if (!a.valid() || !next.valid() ||
                    !a.start_vertex || !a.end_vertex ||
                    !next.start_vertex || !next.end_vertex ||
                    *a.end_vertex != *next.start_vertex) {
                    return false;
                }
            }
            return true;
        };
    std::size_t two_native_inner_wires = 0U;
    std::size_t two_native_outer_wires = 0U;
    for (const auto face_token : two_side->current_faces) {
        const auto face = std::find_if(
            two_catalog.faces.begin(),
            two_catalog.faces.end(),
            [face_token](const auto& record) {
                return record.runtime_token == face_token;
            });
        CHECK(face != two_catalog.faces.end());
        CHECK(face->surface_candidates.size() == 1U);
        CHECK(face->surface_candidates.front() ==
              two_side->address);
        const auto bound = provider.bindFaceToBody(
            twice_drilled.body_solid, face_token);
        CHECK(bound && bound->valid());
        const auto native = provider.queryFaceBoundaryAnySurface(
            twice_drilled.body_solid, *bound);
        CHECK(native.ok());
        std::size_t current_face_outer_wires = 0U;
        for (const auto& wire : native.wires) {
            // Every native Face wire (outer or hole) is a real
            // directed closed path, certified by OCCT exact Vertex
            // tokens belonging to this current RuntimeSolid.
            CHECK(native_directed_closed(wire.edges));
            if (wire.outer) {
                ++current_face_outer_wires;
                ++two_native_outer_wires;
                two_face_outer_cycles.push_back(wire.edges);
            } else {
                ++two_native_inner_wires;
                two_hole_face_tokens.insert(face_token.value);
                two_face_hole_cycles.push_back(wire.edges);
            }
            for (const auto& use : wire.edges) {
                CHECK(use.valid());
                two_native_uses[use.edge.value].push_back(
                    TwoHoleUse{
                        use.reversed, !wire.outer, face_token});
            }
        }
        CHECK(current_face_outer_wires == 1U);
    }
    CHECK(two_native_outer_wires == two_side->current_faces.size());
    CHECK(two_native_inner_wires == 2U);
    CHECK(two_hole_face_tokens.size() == 2U);

    std::set<std::uint64_t> two_outer_tokens;
    std::set<std::uint64_t> two_hole_tokens;
    std::vector<part::MaterialEdgeReference> two_material_sources;
    std::vector<std::uint64_t> two_partition_tokens;
    std::size_t two_cancelled_partitions = 0U;
    for (const auto& [value, uses] : two_native_uses) {
        const kernel::RuntimeEdgeToken edge_token{value};
        const auto current_edge = std::find_if(
            two_catalog.edges.begin(),
            two_catalog.edges.end(),
            [edge_token](const auto& record) {
                return record.runtime_token == edge_token;
            });
        CHECK(current_edge != two_catalog.edges.end());
        const auto source = part::authorMaterialEdgeReference(
            two_catalog, edge_token);
        if (current_edge->representation_partition) {
            CHECK(current_edge->accounting_class ==
                  part::TopologyAccountingClass::
                      known_representation_artifact);
            CHECK(!current_edge->periodic_seam);
            CHECK(!source.ok());
            CHECK(uses.size() == 2U);
            CHECK(uses[0].face != uses[1].face);
            CHECK(!uses[0].inner && !uses[1].inner);
            CHECK(uses[0].reversed != uses[1].reversed);
            two_partition_tokens.push_back(value);
            ++two_cancelled_partitions;
            continue;
        }
        CHECK(!current_edge->periodic_seam);
        CHECK(source.ok() && source.reference);
        CHECK(source.reference->stage == two_catalog.stage);
        CHECK(uses.size() == 1U);
        two_material_sources.push_back(*source.reference);
        if (uses.front().inner) {
            CHECK(current_edge->curve_kind ==
                  kernel::CurveKind::circle);
            CHECK(two_hole_tokens.insert(value).second);
        } else {
            CHECK(two_outer_tokens.insert(value).second);
        }
    }
    CHECK(two_cancelled_partitions >= 1U);
    CHECK(two_hole_tokens.size() == 2U);
    CHECK(!two_outer_tokens.empty());
    for (const auto edge : two_hole_tokens) {
        CHECK(two_outer_tokens.count(edge) == 0U);
    }
    std::sort(two_material_sources.begin(),
              two_material_sources.end());
    CHECK(std::adjacent_find(
        two_material_sources.begin(),
        two_material_sources.end()) == two_material_sources.end());
    CHECK(two_material_sources.size() ==
          two_outer_tokens.size() + two_hole_tokens.size());

    // Outer boundary connectivity after exactly-certified cancellation:
    // every native catalog material Vertex has degree two with respect
    // to retained outer Edge tokens, every Edge has two distinct
    // endpoints, and a single closed walk exhausts the entire set.
    std::map<std::uint64_t, std::vector<std::uint64_t>>
        two_outer_vertex_edges;
    std::map<std::uint64_t, std::vector<std::uint64_t>>
        two_outer_edge_vertices;
    for (const auto& vertex : two_catalog.vertices) {
        std::vector<std::uint64_t> incident;
        for (const auto edge : vertex.incident_material_edges) {
            if (two_outer_tokens.count(edge.value)) {
                incident.push_back(edge.value);
            }
        }
        if (incident.empty()) continue;
        CHECK(vertex.runtime_token.valid());
        CHECK(incident.size() == 2U);
        CHECK(incident[0] != incident[1]);
        for (const auto edge : incident) {
            two_outer_edge_vertices[edge].push_back(
                vertex.runtime_token.value);
        }
        CHECK(two_outer_vertex_edges.emplace(
            vertex.runtime_token.value,
            std::move(incident)).second);
    }
    CHECK(two_outer_edge_vertices.size() ==
          two_outer_tokens.size());
    CHECK(two_outer_vertex_edges.size() ==
          two_outer_tokens.size());
    for (const auto& [edge, endpoints] :
         two_outer_edge_vertices) {
        CHECK(two_outer_tokens.count(edge) == 1U);
        CHECK(endpoints.size() == 2U);
        CHECK(endpoints[0] != endpoints[1]);
    }
    const auto first_edge = two_outer_edge_vertices.begin()->first;
    const auto first_vertex =
        two_outer_edge_vertices.begin()->second.front();
    auto active_two_edge = first_edge;
    auto active_two_vertex = first_vertex;
    std::set<std::uint64_t> visited_two_outer;
    for (std::size_t step = 0U;
         step < two_outer_tokens.size(); ++step) {
        CHECK(visited_two_outer.insert(active_two_edge).second);
        const auto edge =
            two_outer_edge_vertices.find(active_two_edge);
        CHECK(edge != two_outer_edge_vertices.end());
        const auto& ends = edge->second;
        CHECK(ends[0] == active_two_vertex ||
              ends[1] == active_two_vertex);
        const auto next_vertex =
            ends[0] == active_two_vertex ? ends[1] : ends[0];
        const auto at_vertex =
            two_outer_vertex_edges.find(next_vertex);
        CHECK(at_vertex != two_outer_vertex_edges.end());
        CHECK(at_vertex->second.size() == 2U);
        const auto& neighbors = at_vertex->second;
        CHECK(neighbors[0] == active_two_edge ||
              neighbors[1] == active_two_edge);
        const auto next_edge =
            neighbors[0] == active_two_edge
                ? neighbors[1] : neighbors[0];
        if (step + 1U == two_outer_tokens.size()) {
            CHECK(next_vertex == first_vertex);
            CHECK(next_edge == first_edge);
        } else {
            CHECK(next_edge != first_edge);
        }
        active_two_vertex = next_vertex;
        active_two_edge = next_edge;
    }
    CHECK(visited_two_outer == two_outer_tokens);

    // E0 ordered/oriented wire-gluing proof for this exact two-hole
    // planar fixture. Each certified internal partition appears once
    // on each of TWO DIFFERENT native outer Face wires, with opposite
    // orientation. Splice the ordered native cycles at these two uses
    // and remove the pair. NO spatial intersection, tangent repair,
    // face-token ordering, curve approximation or sewing is permitted.
    //
    // Important: this is a test-only combinatorial construction for
    // strictly certified partitions. A partition whose occurrences
    // are in one already-merged loop (or >2 loops) must fail CLOSED;
    // more general topology needs a separate proof before E1.
    CHECK(two_face_outer_cycles.size() ==
          two_side->current_faces.size());
    CHECK(two_face_hole_cycles.size() == 2U);
    CHECK(two_partition_tokens.size() ==
          two_cancelled_partitions);
    for (const auto& hole : two_face_hole_cycles) {
        CHECK(hole.size() == 1U);
        CHECK(two_hole_tokens.count(hole.front().edge.value) == 1U);
    }
    std::vector<std::vector<kernel::FaceBoundaryEdgeUse>>
        stitched_outer = two_face_outer_cycles;
    std::size_t oriented_splices = 0U;
    for (const auto partition_token : two_partition_tokens) {
        std::vector<std::pair<std::size_t, std::size_t>>
            occurrences;
        for (std::size_t cycle_index = 0U;
             cycle_index < stitched_outer.size(); ++cycle_index) {
            for (std::size_t use_index = 0U;
                 use_index < stitched_outer[cycle_index].size();
                 ++use_index) {
                if (stitched_outer[cycle_index][use_index]
                        .edge.value == partition_token) {
                    occurrences.emplace_back(
                        cycle_index, use_index);
                }
            }
        }
        CHECK(occurrences.size() == 2U);
        const auto [first_loop, first_use] = occurrences[0];
        const auto [second_loop, second_use] = occurrences[1];
        CHECK(first_loop != second_loop);
        const auto& left = stitched_outer[first_loop];
        const auto& right = stitched_outer[second_loop];
        CHECK(left[first_use].reversed !=
              right[second_use].reversed);
        std::vector<kernel::FaceBoundaryEdgeUse> combined;
        const auto append_after_cancelled =
            [&combined](
                const std::vector<kernel::FaceBoundaryEdgeUse>&
                    original,
                std::size_t deleted_use) {
                for (std::size_t offset = 1U;
                     offset < original.size(); ++offset) {
                    combined.push_back(
                        original[
                            (deleted_use + offset) %
                            original.size()]);
                }
            };
        append_after_cancelled(left, first_use);
        append_after_cancelled(right, second_use);
        CHECK(!combined.empty());
        std::vector<std::vector<kernel::FaceBoundaryEdgeUse>>
            remaining;
        for (std::size_t i = 0U;
             i < stitched_outer.size(); ++i) {
            if (i != first_loop && i != second_loop) {
                remaining.push_back(
                    std::move(stitched_outer[i]));
            }
        }
        remaining.push_back(std::move(combined));
        stitched_outer = std::move(remaining);
        ++oriented_splices;
    }
    CHECK(oriented_splices == two_cancelled_partitions);
    CHECK(stitched_outer.size() == 1U);
    const auto& ordered_outer = stitched_outer.front();
    CHECK(ordered_outer.size() == two_outer_tokens.size());
    std::set<std::uint64_t> stitched_outer_tokens;
    for (std::size_t i = 0U;
         i < ordered_outer.size(); ++i) {
        const auto current = ordered_outer[i].edge.value;
        const auto next = ordered_outer[
            (i + 1U) % ordered_outer.size()].edge.value;
        CHECK(two_outer_tokens.count(current) == 1U);
        CHECK(stitched_outer_tokens.insert(current).second);
        CHECK(current != next);
        const auto a = two_outer_edge_vertices.find(current);
        const auto b = two_outer_edge_vertices.find(next);
        CHECK(a != two_outer_edge_vertices.end());
        CHECK(b != two_outer_edge_vertices.end());
        std::size_t common_native_vertices = 0U;
        for (const auto v : a->second) {
            if (std::find(
                    b->second.begin(), b->second.end(), v) !=
                b->second.end()) {
                ++common_native_vertices;
            }
        }
        // Ordered neighboring uses share exactly one CURRENT native
        // material Vertex. The last use must also close to the first.
        CHECK(common_native_vertices == 1U);
    }
    CHECK(stitched_outer_tokens == two_outer_tokens);
    // This stronger condition was NOT provable with Edge+reversed
    // alone: native signed endpoint evidence now proves that the
    // exact result of partition splicing has NO backward-traversed
    // material member, including its cyclic last-to-first join.
    CHECK(native_directed_closed(ordered_outer));
    for (const auto& hole : two_face_hole_cycles) {
        CHECK(native_directed_closed(hole));
        CHECK(hole.front().start_vertex ==
              hole.front().end_vertex);
    }
    CHECK(ordered_outer.size() >= 3U);
    auto incorrectly_reversed = ordered_outer;
    std::swap(
        incorrectly_reversed.front().start_vertex,
        incorrectly_reversed.front().end_vertex);
    // Negative E0 control: reversing one directed material use
    // without reversing the FULL native wire must be rejected,
    // although its Edge set and undirected Vertex adjacency
    // remain identical.
    CHECK(!native_directed_closed(incorrectly_reversed));
    std::cout
        << "PG01D_FACE_BOUNDARY_E0_DIRECTED_VERTICES_PASS"
        << " outer_directed_uses=" << ordered_outer.size()
        << " native_holes=" << two_face_hole_cycles.size()
        << " incorrect_single_use_reversal_rejected=1"
        << " exact_start_end_occt_vertex_tokens=1"
        << " xyz_tolerance_joins=0"
        << '\n';
    std::cout
        << "PG01D_FACE_BOUNDARY_E0_ORDERED_OUTER_TWO_HOLES_PASS"
        << " native_outer_fragments="
        << two_face_outer_cycles.size()
        << " exact_opposite_oriented_splices="
        << oriented_splices
        << " resulting_outer_cycles=" << stitched_outer.size()
        << " ordered_outer_material_uses=" << ordered_outer.size()
        << " unchanged_native_inner_wires="
        << two_face_hole_cycles.size()
        << " native_vertex_adjacency=1"
        << " source_geometry_guessing=0"
        << '\n';
    std::cout
        << "PG01D_FACE_BOUNDARY_E0_SPLIT_TWO_HOLES_PASS"
        << " same_surface_fragments="
        << two_side->current_faces.size()
        << " native_inner_wires=" << two_native_inner_wires
        << " holed_fragments=" << two_hole_face_tokens.size()
        << " independently_authored_hole_edges="
        << two_hole_tokens.size()
        << " certified_internal_partitions="
        << two_cancelled_partitions
        << " exact_single_outer_cycle=1"
        << " native_geometry_guessing=0"
        << '\n';
    // E0 mixed multi-edge inner-wire continuation: a THIRD genuine
    // native through Cut. Rectangle lives entirely within the UPPER
    // same-Surface fragment but is disjoint from its existing Circle
    // hole. Crucially its *inner* native OCCT wire has FOUR line uses,
    // not a single periodic Circle Edge. This is test-only and uses
    // wholly synthetic coordinates (not an Owner CAD model).
    CHECK(two_side->canonical_frame.has_value());
    const part::SurfaceReference third_side_support_ref{
        two_catalog.stage, two_side->address};
    const auto third_side_support =
        part::partSketchSupportForBodyPlanarSurface(
            third_side_support_ref);
    CHECK(third_side_support.has_value());
    const auto rectangle_hole_sketch = session.execute(
        application::CreatePartSketchOnSupportCommand{
            *third_side_support,
            session.document().revision()},
        &provider);
    CHECK(rectangle_hole_sketch.ok());
    CHECK(rectangle_hole_sketch.sketch_id);
    const auto rectangle_profile = createRectangleProfile(
        session,
        *rectangle_hole_sketch.sketch_id,
        projectToFrame(
            *two_side->canonical_frame,
            kernel::Point3{40.0, 20.0, 13.0}),
        projectToFrame(
            *two_side->canonical_frame,
            kernel::Point3{40.0, 23.0, 16.0}));
    const auto third_cut = session.execute(
        application::CreateExtrudeFeatureCommand{
            rectangle_profile,
            session.document().revision(),
            part::ExtrudeOperation::cut,
            part::OneSidedExtrudeExtent{
                core::LengthValue{45.0},
                true},
            "PG01D upper-fragment independent rectangle through-hole"},
        provider);
    CHECK(third_cut.ok() && third_cut.feature_id);
    const auto mixed_drilled =
        part::evaluatePart(session.document(), provider);
    CHECK(mixed_drilled.body_status ==
          part::BodyEvaluationStatus::up_to_date);
    CHECK(mixed_drilled.body_solid);
    CHECK(mixed_drilled.current_topology);
    CHECK(mixed_drilled.current_topology->complete());
    const auto& mixed_catalog = *mixed_drilled.current_topology;
    CHECK(mixed_catalog.stage.feature_id &&
          *mixed_catalog.stage.feature_id == *third_cut.feature_id);
    const auto mixed_carrier = std::find_if(
        mixed_catalog.surfaces.begin(),
        mixed_catalog.surfaces.end(),
        [address = continued_side->address](const auto& surface) {
            return surface.address == address;
        });
    CHECK(mixed_carrier != mixed_catalog.surfaces.end());
    CHECK(mixed_carrier->status ==
          kernel::ReferenceStatus::resolved);
    CHECK(mixed_carrier->current_faces.size() >= 2U);

    std::vector<std::vector<kernel::FaceBoundaryEdgeUse>>
        mixed_outer_cycles;
    std::vector<std::vector<kernel::FaceBoundaryEdgeUse>>
        mixed_inner_cycles;
    std::set<std::uint64_t> mixed_holed_faces;
    std::map<std::uint64_t, std::vector<TwoHoleUse>>
        mixed_native_uses;
    std::size_t mixed_circle_holes = 0U;
    std::size_t mixed_rectangle_holes = 0U;
    for (const auto face_token : mixed_carrier->current_faces) {
        const auto face = std::find_if(
            mixed_catalog.faces.begin(),
            mixed_catalog.faces.end(),
            [face_token](const auto& record) {
                return record.runtime_token == face_token;
            });
        CHECK(face != mixed_catalog.faces.end());
        CHECK(face->surface_candidates.size() == 1U);
        CHECK(face->surface_candidates.front() ==
              mixed_carrier->address);
        const auto bound = provider.bindFaceToBody(
            mixed_drilled.body_solid, face_token);
        CHECK(bound && bound->valid());
        const auto native = provider.queryFaceBoundaryAnySurface(
            mixed_drilled.body_solid, *bound);
        CHECK(native.ok());
        std::size_t outer_count = 0U;
        for (const auto& wire : native.wires) {
            CHECK(native_directed_closed(wire.edges));
            if (wire.outer) {
                ++outer_count;
                mixed_outer_cycles.push_back(wire.edges);
            } else {
                mixed_inner_cycles.push_back(wire.edges);
                mixed_holed_faces.insert(face_token.value);
                if (wire.edges.size() == 1U) {
                    ++mixed_circle_holes;
                } else if (wire.edges.size() == 4U) {
                    ++mixed_rectangle_holes;
                } else {
                    CHECK(false);
                }
            }
            for (const auto& use : wire.edges) {
                CHECK(use.valid());
                CHECK(use.start_vertex && use.end_vertex);
                mixed_native_uses[use.edge.value].push_back(
                    TwoHoleUse{use.reversed, !wire.outer,
                               face_token});
            }
        }
        CHECK(outer_count == 1U);
    }
    CHECK(mixed_inner_cycles.size() == 3U);
    CHECK(mixed_circle_holes == 2U);
    CHECK(mixed_rectangle_holes == 1U);
    CHECK(mixed_holed_faces.size() == 2U);
    CHECK(mixed_outer_cycles.size() ==
          mixed_carrier->current_faces.size());

    std::vector<std::uint64_t> mixed_partition_tokens;
    std::set<std::uint64_t> mixed_outer_tokens;
    std::set<std::uint64_t> mixed_hole_tokens;
    std::vector<part::MaterialEdgeReference>
        mixed_material_sources;
    std::size_t mixed_circle_source_edges = 0U;
    std::size_t mixed_line_source_edges = 0U;
    for (const auto& [value, uses] : mixed_native_uses) {
        const kernel::RuntimeEdgeToken token{value};
        const auto record = std::find_if(
            mixed_catalog.edges.begin(),
            mixed_catalog.edges.end(),
            [token](const auto& item) {
                return item.runtime_token == token;
            });
        CHECK(record != mixed_catalog.edges.end());
        const auto material = part::authorMaterialEdgeReference(
            mixed_catalog, token);
        if (record->representation_partition) {
            CHECK(!record->periodic_seam);
            CHECK(record->accounting_class ==
                  part::TopologyAccountingClass::
                      known_representation_artifact);
            CHECK(!material.ok());
            CHECK(uses.size() == 2U);
            CHECK(uses[0].face != uses[1].face);
            CHECK(!uses[0].inner && !uses[1].inner);
            CHECK(uses[0].reversed != uses[1].reversed);
            mixed_partition_tokens.push_back(value);
            continue;
        }
        CHECK(!record->periodic_seam);
        CHECK(material.ok() && material.reference);
        CHECK(material.reference->stage == mixed_catalog.stage);
        CHECK(uses.size() == 1U);
        mixed_material_sources.push_back(*material.reference);
        if (uses.front().inner) {
            CHECK(mixed_hole_tokens.insert(value).second);
            if (record->curve_kind == kernel::CurveKind::circle) {
                ++mixed_circle_source_edges;
            } else {
                CHECK(record->curve_kind ==
                      kernel::CurveKind::line);
                ++mixed_line_source_edges;
            }
        } else {
            CHECK(mixed_outer_tokens.insert(value).second);
        }
    }
    CHECK(!mixed_partition_tokens.empty());
    CHECK(mixed_circle_source_edges == 2U);
    CHECK(mixed_line_source_edges == 4U);
    CHECK(mixed_hole_tokens.size() == 6U);
    CHECK(!mixed_outer_tokens.empty());
    for (const auto token : mixed_hole_tokens) {
        CHECK(mixed_outer_tokens.count(token) == 0U);
    }
    std::sort(mixed_material_sources.begin(),
              mixed_material_sources.end());
    CHECK(std::adjacent_find(
        mixed_material_sources.begin(),
        mixed_material_sources.end()) ==
        mixed_material_sources.end());
    CHECK(mixed_material_sources.size() ==
          mixed_outer_tokens.size() + mixed_hole_tokens.size());

    // Multi-edge inner wire: all FOUR directed line members have
    // distinct exact current Vertex endpoints and form one
    // independently closed native cycle. Two full Circles instead
    // close using the SAME exact start/end Vertex token.
    for (const auto& hole : mixed_inner_cycles) {
        CHECK(native_directed_closed(hole));
        if (hole.size() == 1U) {
            CHECK(hole.front().start_vertex ==
                  hole.front().end_vertex);
            CHECK(mixed_hole_tokens.count(
                hole.front().edge.value) == 1U);
        } else {
            CHECK(hole.size() == 4U);
            std::set<std::uint64_t> hole_vertices;
            for (const auto& use : hole) {
                CHECK(use.start_vertex != use.end_vertex);
                CHECK(mixed_hole_tokens.count(use.edge.value) == 1U);
                CHECK(hole_vertices.insert(
                    use.start_vertex->value).second);
            }
            CHECK(hole_vertices.size() == 4U);
            // A negative directed-use reversal within a multi-Edge
            // hole must fail without needing any coordinate query.
            auto broken_hole = hole;
            std::swap(
                broken_hole[0].start_vertex,
                broken_hole[0].end_vertex);
            CHECK(!native_directed_closed(broken_hole));
            CHECK(!(kernel::FaceBoundaryWire{
                false, broken_hole}).valid());
        }
    }

    // The previously proven bounded splice must still work at the
    // FINAL three-Cut BodyStage. The extra rectangle may subdivide
    // the native outer carrier edges, but must not change partition
    // authority or consume any inner material source.
    auto mixed_stitched_outer = mixed_outer_cycles;
    for (const auto partition : mixed_partition_tokens) {
        std::vector<std::pair<std::size_t, std::size_t>>
            occurrences;
        for (std::size_t li = 0U;
             li < mixed_stitched_outer.size(); ++li) {
            for (std::size_t ui = 0U;
                 ui < mixed_stitched_outer[li].size(); ++ui) {
                if (mixed_stitched_outer[li][ui].edge.value ==
                    partition) {
                    occurrences.emplace_back(li, ui);
                }
            }
        }
        CHECK(occurrences.size() == 2U);
        const auto [li, ui] = occurrences[0];
        const auto [lj, uj] = occurrences[1];
        CHECK(li != lj);
        const auto& a = mixed_stitched_outer[li];
        const auto& b = mixed_stitched_outer[lj];
        CHECK(a[ui].reversed != b[uj].reversed);
        CHECK(a[ui].start_vertex == b[uj].end_vertex);
        CHECK(a[ui].end_vertex == b[uj].start_vertex);
        std::vector<kernel::FaceBoundaryEdgeUse> united;
        for (std::size_t k = 1U; k < a.size(); ++k) {
            united.push_back(a[(ui + k) % a.size()]);
        }
        for (std::size_t k = 1U; k < b.size(); ++k) {
            united.push_back(b[(uj + k) % b.size()]);
        }
        CHECK(!united.empty());
        CHECK(native_directed_closed(united));
        std::vector<std::vector<kernel::FaceBoundaryEdgeUse>>
            remaining;
        for (std::size_t index = 0U;
             index < mixed_stitched_outer.size(); ++index) {
            if (index != li && index != lj) {
                remaining.push_back(
                    std::move(mixed_stitched_outer[index]));
            }
        }
        remaining.push_back(std::move(united));
        mixed_stitched_outer = std::move(remaining);
    }
    CHECK(mixed_stitched_outer.size() == 1U);
    CHECK(native_directed_closed(
        mixed_stitched_outer.front()));
    std::set<std::uint64_t> mixed_stitched_tokens;
    for (const auto& use : mixed_stitched_outer.front()) {
        CHECK(mixed_stitched_tokens.insert(use.edge.value).second);
    }
    CHECK(mixed_stitched_tokens == mixed_outer_tokens);
    std::cout
        << "PG01D_FACE_BOUNDARY_E0_MIXED_INNER_WIRES_PASS"
        << " planar_carrier_faces="
        << mixed_carrier->current_faces.size()
        << " independent_circle_holes=" << mixed_circle_holes
        << " four_line_rectangle_holes="
        << mixed_rectangle_holes
        << " unique_strict_hole_edges="
        << mixed_hole_tokens.size()
        << " certified_partitions="
        << mixed_partition_tokens.size()
        << " single_signed_outer_cycle=1"
        << " native_directed_hole_cycle_reversal_rejected=1"
        << " xyz_based_joining=0"
        << '\n';
    std::cout
        << "PG01D_FACE_BOUNDARY_E0_SPLIT_CARRIER_HOLE_PASS"
        << " same_semantic_surface=1"
        << " native_faces=" << drilled_side->current_faces.size()
        << " native_outer_wires=" << drilled_native_outer_wires
        << " native_hole_wires=" << drilled_native_hole_wires
        << " cancelled_partitions=" << drilled_partition_edges
        << " strict_outer_material_edges=" << drilled_outer_material
        << " strict_hole_material_edges=" << drilled_hole_material
        << " source_geometry_guessing=0"
        << '\n';
}

// PG-01D E0 native negative certification: a full circular base
// cylinder plus a geometrically coaxial half-disc Add. Exact geometric
// continuation is NOT semantic continuation: current provider Add
// lineage only certifies planar Surface continuations, not cylinders.
// A current bounded Face or same radius must never authorize a merged
// curved Face Boundary without an explicit same-carrier certificate.
void verifyNativeCurvedSurfaceContinuationE0() {
    kernel_occt::OcctSolidModelingKernel provider;
    kernel::PlanarProfileInput round_base;
    round_base.outer.boundary = {{
        kernel::Circle2{{0.0, 0.0}, 10.0},
        0.0, 1.0, true, false, true,
        kernel::BoundaryUseProvenance{
            "cylinder-full-circle", 0U, 0U, false},
    }};
    CHECK(round_base.valid());
    const auto base = provider.extrude(add(round_base, 10.0));
    CHECK(base.ok());
    const auto* source_side = newSide(
        base, "cylinder-full-circle");
    CHECK(source_side != nullptr);
    CHECK(source_side->surface_status ==
          kernel::ReferenceStatus::resolved);
    CHECK(source_side->resolved_token.has_value());
    CHECK(source_side->surface_kind ==
          kernel::SurfaceKind::cylinder);

    kernel::Frame3 top_frame;
    top_frame.origin = {0.0, 0.0, 10.0};
    CHECK(top_frame.valid());
    kernel::PlanarProfileInput upper_half;
    upper_half.frame = top_frame;
    upper_half.outer.boundary = {{
        kernel::Arc2{
            {0.0, 0.0}, 10.0,
            0.0, std::acos(-1.0)},
        0.0, 1.0, true, false, false,
        kernel::BoundaryUseProvenance{
            "half-circle-arc", 0U, 0U, false},
    }, lineUse(
        {-10.0, 0.0}, {10.0, 0.0},
        "half-circle-diameter", 1U)};
    CHECK(upper_half.valid());
    const auto extended =
        provider.extrude(add(upper_half, 10.0), base.solid);
    CHECK(extended.ok());

    const auto* inherited = inheritedSurface(
        extended, *source_side->resolved_token);
    CHECK(inherited != nullptr);
    CHECK(inherited->surface_status ==
          kernel::ReferenceStatus::resolved);
    CHECK(inherited->surface_kind ==
          kernel::SurfaceKind::cylinder);
    CHECK(inherited->current_faces.size() == 1U);
    const auto* generated = newSide(
        extended, "half-circle-arc");
    CHECK(generated != nullptr);
    CHECK(generated->surface_kind ==
          kernel::SurfaceKind::cylinder);
    // Explicit STOP: no same-semantic-carrier certificate despite
    // exact coaxial geometry and topologically connected Add.
    CHECK(!generated->continued_into.has_value());
    CHECK(!generated->contribution_faces.empty());

    std::set<std::uint64_t> inherited_face_tokens;
    std::size_t inherited_native_wires = 0U;
    std::size_t created_native_wires = 0U;
    for (const auto token : inherited->current_faces) {
        CHECK(inherited_face_tokens.insert(token.value).second);
        const auto bound =
            provider.bindFaceToBody(extended.solid, token);
        CHECK(bound && bound->valid());
        CHECK(provider.queryFaceBoundary(
            extended.solid, *bound).status ==
                kernel::FaceBoundaryStatus::unsupported_surface);
        const auto raw = provider.queryFaceBoundaryAnySurface(
            extended.solid, *bound);
        CHECK(raw.ok());
        inherited_native_wires += raw.wires.size();
    }
    for (const auto token : generated->contribution_faces) {
        CHECK(inherited_face_tokens.count(token.value) == 0U);
        const auto bound =
            provider.bindFaceToBody(extended.solid, token);
        CHECK(bound && bound->valid());
        CHECK(provider.queryFaceBoundary(
            extended.solid, *bound).status ==
                kernel::FaceBoundaryStatus::unsupported_surface);
        const auto raw = provider.queryFaceBoundaryAnySurface(
            extended.solid, *bound);
        CHECK(raw.ok());
        created_native_wires += raw.wires.size();
    }
    CHECK(inherited_native_wires >= 1U);
    CHECK(created_native_wires >= 1U);
    std::cout
        << "PG01D_CURVED_E0_NO_CONTINUATION_CERTIFICATE_PASS"
        << " inherited_faces=" << inherited->current_faces.size()
        << " created_contribution_faces="
        << generated->contribution_faces.size()
        << " inherited_native_wires=" << inherited_native_wires
        << " created_native_wires=" << created_native_wires
        << " claimed_same_carrier=0"
        << " similarity_rebinding=0"
        << '\n';
}

// E0 separate from Add: a through-circumference GROOVE removes an
// annular band of an otherwise continuous full cylinder, preserving
// a single connected solid through the radius-8 interior core. If OCCT
// correctly splits the inherited side carrier into independent native
// Faces, their identical semantic carrier must NOT be interpreted as a
// single CONTIGUOUS material Face Boundary region. Exact native Edge
// and Vertex incidence, not radius/proximity, establishes the stop.
void verifyNativeCylindricalCutSplitE0() {
    kernel_occt::OcctSolidModelingKernel provider;
    kernel::PlanarProfileInput cylinder;
    cylinder.outer.boundary = {{
        kernel::Circle2{{0.0, 0.0}, 10.0},
        0.0, 1.0, true, false, true,
        kernel::BoundaryUseProvenance{
            "cut-split-cylinder", 0U, 0U, false},
    }};
    CHECK(cylinder.valid());
    const auto base = provider.extrude(add(cylinder, 10.0));
    CHECK(base.ok());
    const auto* old_side = newSide(
        base, "cut-split-cylinder");
    CHECK(old_side && old_side->resolved_token);
    CHECK(old_side->surface_status ==
          kernel::ReferenceStatus::resolved);
    CHECK(old_side->surface_kind ==
          kernel::SurfaceKind::cylinder);

    kernel::PlanarProfileInput groove_profile;
    groove_profile.frame.origin = {0.0, 0.0, 4.0};
    CHECK(groove_profile.frame.valid());
    groove_profile.outer.boundary = {{
        kernel::Circle2{{0.0, 0.0}, 12.0},
        0.0, 1.0, true, false, true,
        kernel::BoundaryUseProvenance{
            "cut-ring-outer", 0U, 0U, false},
    }};
    kernel::ProfileLoopInput inner;
    inner.boundary = {{
        kernel::Circle2{{0.0, 0.0}, 8.0},
        0.0, 1.0, true, false, true,
        kernel::BoundaryUseProvenance{
            "cut-ring-inner", 1U, 0U, true},
    }};
    groove_profile.holes.push_back(std::move(inner));
    CHECK(groove_profile.valid());
    auto groove = add(groove_profile, 2.0);
    groove.operation = kernel::SolidBooleanOperation::cut;
    CHECK(groove.valid());
    const auto cut = provider.extrude(groove, base.solid);
    CHECK(cut.ok());
    CHECK(cut.solid_count == 1U);

    const auto* inherited = inheritedSurface(
        cut, *old_side->resolved_token);
    CHECK(inherited);
    std::cerr
        << "PG01D_CURVED_CUT_E0_SPLIT_PROBE"
        << " status=" << static_cast<int>(inherited->surface_status)
        << " faces=" << inherited->current_faces.size()
        << " strict_status="
        << static_cast<int>(inherited->strict_face_status)
        << '\n';
    CHECK(inherited->surface_status ==
          kernel::ReferenceStatus::resolved);
    CHECK(inherited->surface_kind ==
          kernel::SurfaceKind::cylinder);
    CHECK(inherited->current_faces.size() == 2U);
    CHECK(inherited->strict_face_status ==
          kernel::ReferenceStatus::ambiguous);

    std::vector<std::set<std::uint64_t>> face_edges;
    std::vector<std::set<std::uint64_t>> face_material_edges;
    std::vector<std::set<std::uint64_t>> face_vertices;
    // An OCCT seam is TWO oriented uses of ONE representation-only
    // Edge on the SAME native bounded Face, never a join between
    // different Faces sharing one semantic cylindrical carrier.
    struct NativeSeamUse final {
        kernel::RuntimeFaceToken face;
        bool reversed{};
        kernel::RuntimeVertexToken start;
        kernel::RuntimeVertexToken end;
    };
    std::map<std::uint64_t, std::vector<NativeSeamUse>>
        seam_native_uses;
    std::size_t seam_uses = 0U;
    for (const auto token : inherited->current_faces) {
        const auto scoped =
            provider.bindFaceToBody(cut.solid, token);
        CHECK(scoped && scoped->valid());
        CHECK(provider.queryFaceBoundary(
            cut.solid, *scoped).status ==
                kernel::FaceBoundaryStatus::unsupported_surface);
        const auto native =
            provider.queryFaceBoundaryAnySurface(
                cut.solid, *scoped);
        CHECK(native.ok());
        std::set<std::uint64_t> edges;
        std::set<std::uint64_t> material_edges;
        for (const auto& wire : native.wires) {
            CHECK(wire.valid());
            for (std::size_t i = 0U; i < wire.edges.size();
                 ++i) {
                const auto& use = wire.edges[i];
                const auto& next =
                    wire.edges[(i + 1U) % wire.edges.size()];
                CHECK(use.valid());
                CHECK(use.start_vertex && use.end_vertex);
                CHECK(next.start_vertex);
                CHECK(*use.end_vertex == *next.start_vertex);
                edges.insert(use.edge.value);
                const auto observation = std::find_if(
                    cut.current_edge_semantics.begin(),
                    cut.current_edge_semantics.end(),
                    [&use](const auto& item) {
                        return item.runtime_token == use.edge;
                    });
                CHECK(observation !=
                      cut.current_edge_semantics.end());
                if (observation->periodic_seam) {
                    CHECK(!observation->same_surface_partition);
                    seam_native_uses[use.edge.value].push_back(
                        NativeSeamUse{
                            token, use.reversed,
                            *use.start_vertex, *use.end_vertex});
                    ++seam_uses;
                } else {
                    // Material-only graph: do NOT permit native seam
                    // edges or known representation partitions as
                    // contour-joining material authority.
                    CHECK(!observation->same_surface_partition);
                    material_edges.insert(use.edge.value);
                }
            }
        }
        CHECK(!edges.empty());
        CHECK(!material_edges.empty());
        std::set<std::uint64_t> vertices;
        for (const auto& v : cut.current_vertex_semantics) {
            for (const auto edge : v.incident_material_edges) {
                if (edges.count(edge.value)) {
                    vertices.insert(v.runtime_token.value);
                }
            }
        }
        face_edges.push_back(std::move(edges));
        face_material_edges.push_back(std::move(material_edges));
        face_vertices.push_back(std::move(vertices));
    }
    CHECK(face_edges.size() == 2U);
    CHECK(face_material_edges.size() == 2U);
    CHECK(face_vertices.size() == 2U);
    CHECK(!face_vertices[0].empty());
    CHECK(!face_vertices[1].empty());
    // These two bounded realizations have no common current material
    // boundary Edge or material-incidence Vertex. The common Surface
    // lineage alone is NOT authority for a connected region traversal.
    for (const auto token : face_edges[0]) {
        CHECK(face_edges[1].count(token) == 0U);
    }
    for (const auto token : face_vertices[0]) {
        CHECK(face_vertices[1].count(token) == 0U);
    }
    for (const auto token : face_material_edges[0]) {
        CHECK(face_material_edges[1].count(token) == 0U);
    }
    // Provider-signed directed observations guarantee that a seam
    // cannot sneak into a cross-Face stitching graph: precisely two
    // opposite uses, same Face, reversed native Vertex endpoints.
    CHECK(!seam_native_uses.empty());
    CHECK(seam_uses == 2U * seam_native_uses.size());
    std::set<std::uint64_t> seam_host_faces;
    for (const auto& [edge, uses] : seam_native_uses) {
        CHECK(edge != 0U);
        CHECK(uses.size() == 2U);
        CHECK(uses[0].face == uses[1].face);
        CHECK(uses[0].reversed != uses[1].reversed);
        CHECK(uses[0].start == uses[1].end);
        CHECK(uses[0].end == uses[1].start);
        CHECK(face_material_edges[0].count(edge) == 0U);
        CHECK(face_material_edges[1].count(edge) == 0U);
        seam_host_faces.insert(uses[0].face.value);
    }
    CHECK(seam_host_faces.size() == 2U);
    std::cout
        << "PG01D_CURVED_CUT_E0_DIRECTED_SEAM_ISOLATION_PASS"
        << " same_carrier_bounded_faces=2"
        << " strictly_same_face_opposite_oriented_seams="
        << seam_native_uses.size()
        << " signed_seam_uses=" << seam_uses
        << " disconnected_material_components=2"
        << " seam_material_authority=0"
        << " cross_face_vertex_stitching=0"
        << " coordinate_matching=0"
        << '\n';
    std::cout
        << "PG01D_CURVED_CUT_E0_DISCONNECTED_CARRIER_PASS"
        << " current_same_carrier_faces=" << inherited->current_faces.size()
        << " shared_native_edges=0"
        << " shared_material_vertices=0"
        << " seam_native_uses=" << seam_uses
        << " distinct_contiguous_regions=2"
        << " geometry_proximity_merging=0"
        << '\n';
}

// Native E0 adjacency-graph exploration on REAL OCCT planar Add.
// A connected U-shaped top extrusion has TWO DISJOINT strips of +X
// sidewall attached to a SINGLE inherited base +X side carrier.
// Thus the ideal certified group is a three-Face TREE with two exact
// opposite-oriented same-carrier representation partitions, a single
// outer boundary, and no hole. Nothing may be grouped by XY or normals
// alone: only provider continuation claims authorize same-carrier uses.
void verifyNativePlanarBranchedSurfaceContinuationsE0() {
    kernel_occt::OcctSolidModelingKernel provider;
    const auto base = provider.extrude(add(
        rectangle(kernel::Frame3{}, 0.0, 0.0, 40.0, 30.0,
                  "branch-base"), 10.0));
    CHECK(base.ok());
    const auto* inherited_right =
        newSide(base, "branch-base-right");
    CHECK(inherited_right && inherited_right->resolved_token);
    CHECK(inherited_right->surface_kind ==
          kernel::SurfaceKind::plane);
    CHECK(inherited_right->surface_status ==
          kernel::ReferenceStatus::resolved);

    kernel::PlanarProfileInput branched;
    branched.frame.origin = {0.0, 0.0, 10.0};
    CHECK(branched.frame.valid());
    const std::vector<kernel::Point2> corners{
        {20.0, 5.0},
        {40.0, 5.0},
        {40.0, 10.0},
        {25.0, 10.0},
        {25.0, 20.0},
        {40.0, 20.0},
        {40.0, 25.0},
        {20.0, 25.0},
    };
    for (std::size_t i = 0U; i < corners.size(); ++i) {
        const auto name = i == 1U
            ? "branch-top-right-lower"
            : i == 5U
                ? "branch-top-right-upper"
                : "branch-top-other";
        branched.outer.boundary.push_back(lineUse(
            corners[i],
            corners[(i + 1U) % corners.size()],
            name, static_cast<std::uint32_t>(i)));
    }
    CHECK(branched.valid());
    const auto extended =
        provider.extrude(add(branched, 10.0), base.solid);
    CHECK(extended.ok());
    CHECK(extended.solid_count == 1U);
    const auto* inherited =
        inheritedSurface(
            extended, *inherited_right->resolved_token);
    CHECK(inherited);
    const auto* lower =
        newSide(extended, "branch-top-right-lower");
    const auto* upper =
        newSide(extended, "branch-top-right-upper");
    CHECK(lower && upper);
    std::cerr
        << "PG01D_E0_BRANCH_NATIVE_PROBE"
        << " inherited_status="
        << static_cast<int>(inherited->surface_status)
        << " inherited_faces="
        << inherited->current_faces.size()
        << " lower_continued=" << lower->continued_into.has_value()
        << " upper_continued=" << upper->continued_into.has_value()
        << '\n';
    CHECK(inherited->surface_status ==
          kernel::ReferenceStatus::resolved);
    CHECK(inherited->current_faces.size() >= 3U);
    CHECK(lower->continued_into ==
          inherited_right->resolved_token);
    CHECK(upper->continued_into ==
          inherited_right->resolved_token);
    // Each upper right-wall segment has its OWN current bounded
    // native Face. Neither may be merged solely by equal coplanar
    // geometry; both are instead explicitly certified as continuing
    // this one inherited semantic carrier by the provider.
    CHECK(lower->contribution_faces.size() == 1U);
    CHECK(upper->contribution_faces.size() == 1U);
    const auto lower_patch = lower->contribution_faces.front();
    const auto upper_patch = upper->contribution_faces.front();
    CHECK(lower_patch != upper_patch);
    CHECK(std::find(
        inherited->current_faces.begin(),
        inherited->current_faces.end(), lower_patch) !=
        inherited->current_faces.end());
    CHECK(std::find(
        inherited->current_faces.begin(),
        inherited->current_faces.end(), upper_patch) !=
        inherited->current_faces.end());
    std::set<std::uint64_t> base_wall_faces;
    for (const auto token : inherited->current_faces) {
        if (token != lower_patch && token != upper_patch) {
            base_wall_faces.insert(token.value);
        }
    }
    // One original base Face with TWO separate attached branches.
    CHECK(base_wall_faces.size() == 1U);
    CHECK(inherited->current_faces.size() == 3U);
    const auto base_wall_face = *base_wall_faces.begin();

    struct NativeUse final {
        kernel::RuntimeFaceToken face;
        kernel::FaceBoundaryEdgeUse use;
        bool outer{};
    };
    std::map<std::uint64_t, std::vector<NativeUse>> uses;
    std::vector<std::vector<kernel::FaceBoundaryEdgeUse>>
        native_outer_cycles;
    for (const auto face : inherited->current_faces) {
        const auto bound =
            provider.bindFaceToBody(extended.solid, face);
        CHECK(bound && bound->valid());
        const auto observed =
            provider.queryFaceBoundaryAnySurface(
                extended.solid, *bound);
        CHECK(observed.ok());
        for (const auto& wire : observed.wires) {
            CHECK(wire.valid());
            CHECK(wire.outer); // U-shaped Add makes no holes.
            native_outer_cycles.push_back(wire.edges);
            for (const auto& use : wire.edges) {
                CHECK(use.start_vertex && use.end_vertex);
                uses[use.edge.value].push_back(
                    NativeUse{face, use, wire.outer});
            }
        }
    }
    CHECK(native_outer_cycles.size() ==
          inherited->current_faces.size());
    std::set<std::uint64_t> material_tokens;
    std::vector<std::uint64_t> partitions;
    std::map<std::uint64_t,
             std::set<std::uint64_t>> partition_face_adj;
    for (const auto& [edge_value, members] : uses) {
        const kernel::RuntimeEdgeToken token{edge_value};
        const auto observation = std::find_if(
            extended.current_edge_semantics.begin(),
            extended.current_edge_semantics.end(),
            [token](const auto& item) {
                return item.runtime_token == token;
            });
        CHECK(observation !=
              extended.current_edge_semantics.end());
        CHECK(!observation->periodic_seam);
        if (observation->same_surface_partition) {
            CHECK(members.size() == 2U);
            CHECK(members[0].face != members[1].face);
            CHECK(members[0].use.reversed !=
                  members[1].use.reversed);
            CHECK(members[0].use.start_vertex ==
                  members[1].use.end_vertex);
            CHECK(members[0].use.end_vertex ==
                  members[1].use.start_vertex);
            partitions.push_back(edge_value);
            partition_face_adj[edge_value].insert(
                members[0].face.value);
            partition_face_adj[edge_value].insert(
                members[1].face.value);
        } else {
            CHECK(members.size() == 1U);
            CHECK(material_tokens.insert(edge_value).second);
        }
    }
    CHECK(partitions.size() == 2U);
    CHECK(partition_face_adj.size() == partitions.size());
    // The exact native incidence graph is a three-node TWO-EDGE
    // tree (not a chain): both partition Edges connect the retained
    // base wall Face to a DIFFERENT created branch Face.
    std::set<std::uint64_t> attached_patch_faces;
    for (const auto& [edge, touching] : partition_face_adj) {
        CHECK(edge != 0U);
        CHECK(touching.size() == 2U);
        CHECK(touching.count(base_wall_face) == 1U);
        for (const auto face_token : touching) {
            if (face_token != base_wall_face) {
                CHECK(
                    face_token == lower_patch.value ||
                    face_token == upper_patch.value);
                CHECK(attached_patch_faces.insert(
                    face_token).second);
            }
        }
    }
    CHECK(attached_patch_faces.size() == 2U);
    CHECK(attached_patch_faces.count(lower_patch.value) == 1U);
    CHECK(attached_patch_faces.count(upper_patch.value) == 1U);
    CHECK(!material_tokens.empty());
    auto stitched = native_outer_cycles;
    const auto directed_closed = [](
        const std::vector<kernel::FaceBoundaryEdgeUse>& v) {
        if (v.empty()) return false;
        for (std::size_t i = 0; i < v.size(); ++i) {
            const auto& a = v[i];
            const auto& b = v[(i + 1U) % v.size()];
            if (!a.valid() || !a.end_vertex ||
                !b.start_vertex ||
                a.end_vertex != b.start_vertex) return false;
        }
        return true;
    };
    for (const auto partition : partitions) {
        std::vector<std::pair<std::size_t, std::size_t>>
            locations;
        for (std::size_t ring = 0; ring < stitched.size(); ++ring) {
            for (std::size_t i = 0; i < stitched[ring].size(); ++i) {
                if (stitched[ring][i].edge.value == partition) {
                    locations.emplace_back(ring, i);
                }
            }
        }
        CHECK(locations.size() == 2U);
        const auto [ai, aj] = locations[0];
        const auto [bi, bj] = locations[1];
        CHECK(ai != bi);
        std::vector<kernel::FaceBoundaryEdgeUse> combined;
        for (std::size_t i = 1U; i < stitched[ai].size(); ++i) {
            combined.push_back(
                stitched[ai][(aj + i) % stitched[ai].size()]);
        }
        for (std::size_t i = 1U; i < stitched[bi].size(); ++i) {
            combined.push_back(
                stitched[bi][(bj + i) % stitched[bi].size()]);
        }
        CHECK(directed_closed(combined));
        std::vector<std::vector<kernel::FaceBoundaryEdgeUse>>
            remaining;
        for (std::size_t i = 0U; i < stitched.size(); ++i) {
            if (i != ai && i != bi) {
                remaining.push_back(std::move(stitched[i]));
            }
        }
        remaining.push_back(std::move(combined));
        stitched = std::move(remaining);
    }
    CHECK(stitched.size() == 1U);
    CHECK(directed_closed(stitched.front()));
    std::set<std::uint64_t> retained;
    for (const auto& use : stitched.front()) {
        CHECK(retained.insert(use.edge.value).second);
    }
    CHECK(retained == material_tokens);

    // E0: the real three-Face *branching* topology must not depend on
    // which native Face/wire is visited first or on which of the TWO
    // independently certified partition pairs is removed first.
    // This is a different incidence graph from the two-Face cyclic
    // two-hole fixture. Every comparison includes native Edge sense
    // and BOTH exact directed Vertex endpoint tokens.
    const auto same_signed_cycle = [](
        const std::vector<kernel::FaceBoundaryEdgeUse>& candidate,
        const std::vector<kernel::FaceBoundaryEdgeUse>& expected) {
        if (candidate.empty() ||
            candidate.size() != expected.size()) return false;
        for (std::size_t start = 0U; start < candidate.size(); ++start) {
            bool same = true;
            for (std::size_t i = 0U; i < expected.size(); ++i) {
                if (!(candidate[(start + i) % candidate.size()] ==
                      expected[i])) {
                    same = false;
                    break;
                }
            }
            if (same) return true;
        }
        return false;
    };
    const auto reconstruct = [&](
        std::vector<std::vector<kernel::FaceBoundaryEdgeUse>> rings,
        const std::vector<std::uint64_t>& cancel_order)
        -> std::optional<std::vector<kernel::FaceBoundaryEdgeUse>> {
        if (rings.size() != 3U ||
            cancel_order.size() != partitions.size()) {
            return std::nullopt;
        }
        for (const auto& ring : rings) {
            if (!directed_closed(ring)) return std::nullopt;
        }
        for (const auto partition : cancel_order) {
            std::vector<std::pair<std::size_t, std::size_t>> locations;
            for (std::size_t r = 0U; r < rings.size(); ++r) {
                for (std::size_t i = 0U; i < rings[r].size(); ++i) {
                    if (rings[r][i].edge.value == partition) {
                        locations.emplace_back(r, i);
                    }
                }
            }
            if (locations.size() != 2U ||
                locations[0].first == locations[1].first) {
                return std::nullopt;
            }
            const auto [ar, ai] = locations[0];
            const auto [br, bi] = locations[1];
            const auto& a = rings[ar][ai];
            const auto& b = rings[br][bi];
            if (a.reversed == b.reversed ||
                a.start_vertex != b.end_vertex ||
                a.end_vertex != b.start_vertex) {
                return std::nullopt;
            }
            std::vector<kernel::FaceBoundaryEdgeUse> combined;
            for (std::size_t i = 1U; i < rings[ar].size(); ++i) {
                combined.push_back(
                    rings[ar][(ai + i) % rings[ar].size()]);
            }
            for (std::size_t i = 1U; i < rings[br].size(); ++i) {
                combined.push_back(
                    rings[br][(bi + i) % rings[br].size()]);
            }
            if (!directed_closed(combined)) return std::nullopt;
            std::vector<std::vector<kernel::FaceBoundaryEdgeUse>> next;
            for (std::size_t r = 0U; r < rings.size(); ++r) {
                if (r != ar && r != br) {
                    next.push_back(std::move(rings[r]));
                }
            }
            next.push_back(std::move(combined));
            rings = std::move(next);
        }
        if (rings.size() != 1U ||
            !directed_closed(rings.front())) {
            return std::nullopt;
        }
        std::set<std::uint64_t> covered;
        for (const auto& use : rings.front()) {
            if (!covered.insert(use.edge.value).second) {
                return std::nullopt;
            }
        }
        if (covered != material_tokens) return std::nullopt;
        return std::move(rings.front());
    };

    std::vector<std::size_t> face_order{0U, 1U, 2U};
    std::size_t permutations_checked = 0U;
    do {
        const auto& c0 = native_outer_cycles[face_order[0]];
        const auto& c1 = native_outer_cycles[face_order[1]];
        const auto& c2 = native_outer_cycles[face_order[2]];
        for (std::size_t s0 = 0U; s0 < c0.size(); ++s0) {
            for (std::size_t s1 = 0U; s1 < c1.size(); ++s1) {
                for (std::size_t s2 = 0U; s2 < c2.size(); ++s2) {
                    std::vector<std::vector<kernel::FaceBoundaryEdgeUse>>
                        rotated{c0, c1, c2};
                    const std::vector<std::size_t> starts{s0, s1, s2};
                    for (std::size_t r = 0U; r < rotated.size(); ++r) {
                        auto& cycle = rotated[r];
                        std::rotate(
                            cycle.begin(),
                            cycle.begin() + starts[r],
                            cycle.end());
                        CHECK(directed_closed(cycle));
                    }
                    auto order = partitions;
                    std::sort(order.begin(), order.end());
                    do {
                        const auto result = reconstruct(rotated, order);
                        CHECK(result.has_value());
                        CHECK(same_signed_cycle(*result, stitched.front()));
                        ++permutations_checked;
                    } while (std::next_permutation(
                        order.begin(), order.end()));
                }
            }
        }
    } while (std::next_permutation(
        face_order.begin(), face_order.end()));
    CHECK(permutations_checked ==
          12U * native_outer_cycles[0].size() *
          native_outer_cycles[1].size() *
          native_outer_cycles[2].size());

    // Negative controls mutate actual native observations, never
    // manufacture a geometric proximity join. Reversed endpoint
    // evidence or omitted partition identity must reject the WHOLE
    // proposed reconstruction rather than produce a partial outline.
    auto bad_vertex = native_outer_cycles;
    bool corrupted = false;
    for (auto& ring : bad_vertex) {
        for (auto& use : ring) {
            if (use.edge.value == partitions.front()) {
                CHECK(use.start_vertex && use.end_vertex);
                CHECK(use.start_vertex != use.end_vertex);
                std::swap(use.start_vertex, use.end_vertex);
                use.reversed = !use.reversed;
                corrupted = true;
                break;
            }
        }
        if (corrupted) break;
    }
    CHECK(corrupted);
    CHECK(!reconstruct(bad_vertex, partitions).has_value());
    CHECK(!reconstruct(
        native_outer_cycles,
        std::vector<std::uint64_t>{partitions.front()}).has_value());
    auto duplicate_partition = partitions;
    duplicate_partition.back() = duplicate_partition.front();
    CHECK(!reconstruct(
        native_outer_cycles, duplicate_partition).has_value());
    auto missing_material = native_outer_cycles;
    bool removed = false;
    for (auto& ring : missing_material) {
        for (auto it = ring.begin(); it != ring.end(); ++it) {
            if (material_tokens.count(it->edge.value) != 0U) {
                ring.erase(it);
                removed = true;
                break;
            }
        }
        if (removed) break;
    }
    CHECK(removed);
    CHECK(!reconstruct(missing_material, partitions).has_value());
    std::cout
        << "PG01D_E0_BRANCH_FACE_ORDER_INVARIANCE_PASS"
        << " real_native_faces=3"
        << " certified_partitions=2"
        << " face_orders=6"
        << " partition_orders=2"
        << " signed_variants=" << permutations_checked
        << " malformed_native_vertex_rejected=1"
        << " omitted_partition_rejected=1"
        << " duplicate_partition_rejected=1"
        << " dropped_material_edge_rejected=1"
        << " false_material_boundary=0"
        << '\n';
    std::cout
        << "PG01D_E0_NATIVE_BRANCHED_PLANAR_TREE_PASS"
        << " same_carrier_faces=" << inherited->current_faces.size()
        << " opposite_partitions=" << partitions.size()
        << " exact_three_face_branch_tree=1"
        << " upper_patch_faces=2"
        << " outer_cycles_after_cancel=1"
        << " retained_material_edges=" << material_tokens.size()
        << " geometric_stitching=0"
        << '\n';
}

// E0 real OCCT annulus-cycle candidate: pierce the shared Face
// partition of a base planar side plus its continued top boss.
// The cut Circle is CENTERED on the native z=10 interface, not
// inside either fragment. Exact OCCT topology must decide whether
// the original one shared internal partition becomes TWO distinct
// certified opposite oriented partition Edges. If so, cancelling
// the pairs may generate a region OUTER and a region HOLE contour
// from multiple Face outer wires rather than native inner wires.
void verifyNativePlanarHoleAcrossPartitionE0() {
    kernel_occt::OcctSolidModelingKernel provider;
    const auto base = provider.extrude(add(
        rectangle(kernel::Frame3{}, 0.0, 0.0, 40.0, 30.0,
                  "cross-partition-base"), 10.0));
    CHECK(base.ok());
    const auto* base_wall =
        newSide(base, "cross-partition-base-right");
    CHECK(base_wall && base_wall->resolved_token);
    CHECK(base_wall->surface_kind ==
          kernel::SurfaceKind::plane);
    kernel::Frame3 boss_frame;
    boss_frame.origin = {0.0, 0.0, 10.0};
    CHECK(boss_frame.valid());
    const auto joined = provider.extrude(
        add(rectangle(
            boss_frame, 20.0, 5.0, 40.0, 25.0,
            "cross-partition-boss"), 10.0),
        base.solid);
    CHECK(joined.ok());
    const auto* continued = inheritedSurface(
        joined, *base_wall->resolved_token);
    CHECK(continued);
    CHECK(continued->surface_status ==
          kernel::ReferenceStatus::resolved);
    CHECK(continued->current_faces.size() >= 2U);
    const auto* created =
        newSide(joined, "cross-partition-boss-right");
    CHECK(created &&
          created->continued_into == base_wall->resolved_token);

    kernel::Frame3 hole_frame;
    hole_frame.origin = {40.0, 0.0, 0.0};
    hole_frame.u_axis = {0.0, 0.0, 1.0};
    hole_frame.v_axis = {0.0, 1.0, 0.0};
    hole_frame.normal = {-1.0, 0.0, 0.0};
    CHECK(hole_frame.valid());
    kernel::PlanarProfileInput hole;
    hole.frame = hole_frame;
    hole.outer.boundary = {{
        kernel::Circle2{{10.0, 15.0}, 2.5},
        0.0, 1.0, true, false, true,
        kernel::BoundaryUseProvenance{
            "cross-partition-hole", 0U, 0U, false},
    }};
    CHECK(hole.valid());
    auto cut_input = add(std::move(hole), 45.0);
    cut_input.operation = kernel::SolidBooleanOperation::cut;
    CHECK(cut_input.valid());
    const auto cut = provider.extrude(
        cut_input, joined.solid);
    CHECK(cut.ok());
    CHECK(cut.solid_count == 1U);
    // Exact native semantic lineage for the actual CUT-created
    // cylindrical side, not a geometry/radius-identity inference.
    const auto* cut_cylinder =
        newSide(cut, "cross-partition-hole");
    std::cerr
        << "PG01D_E0_NATIVE_CUT_SIDE_LINEAGE"
        << " generated_role_found=" << (cut_cylinder != nullptr)
        << " kind=" << (cut_cylinder
            ? static_cast<int>(cut_cylinder->surface_kind) : -1)
        << " surface_status=" << (cut_cylinder
            ? static_cast<int>(cut_cylinder->surface_status) : -1)
        << " unique_token=" << (cut_cylinder &&
            cut_cylinder->resolved_token.has_value())
        << " contribution_faces=" << (cut_cylinder
            ? cut_cylinder->contribution_faces.size() : 0U)
        << '\n';
    CHECK(cut_cylinder);
    CHECK(cut_cylinder->surface_kind ==
          kernel::SurfaceKind::cylinder);
    CHECK(cut_cylinder->surface_status ==
          kernel::ReferenceStatus::resolved);
    CHECK(cut_cylinder->resolved_token);
    const auto* inherited = inheritedSurface(
        cut, *base_wall->resolved_token);
    CHECK(inherited);
    std::cerr
        << "PG01D_E0_CROSS_PARTITION_HOLE_PROBE"
        << " status="
        << static_cast<int>(inherited->surface_status)
        << " faces=" << inherited->current_faces.size()
        << " strict_face_status="
        << static_cast<int>(inherited->strict_face_status)
        << '\n';
    CHECK(inherited->surface_status ==
          kernel::ReferenceStatus::resolved);
    CHECK(inherited->surface_kind ==
          kernel::SurfaceKind::plane);
    CHECK(inherited->current_faces.size() >= 2U);

    struct NativeUse final {
        kernel::RuntimeFaceToken face;
        kernel::FaceBoundaryEdgeUse use;
        bool inner{};
    };
    std::map<std::uint64_t, std::vector<NativeUse>> ledger;
    std::vector<std::vector<kernel::FaceBoundaryEdgeUse>>
        native_outer;
    std::size_t native_inner_wires = 0U;
    for (const auto face : inherited->current_faces) {
        const auto scoped = provider.bindFaceToBody(
            cut.solid, face);
        CHECK(scoped && scoped->valid());
        const auto native =
            provider.queryFaceBoundaryAnySurface(
                cut.solid, *scoped);
        CHECK(native.ok());
        for (const auto& wire : native.wires) {
            CHECK(wire.valid());
            if (wire.outer) {
                native_outer.push_back(wire.edges);
            } else {
                ++native_inner_wires;
            }
            for (const auto& use : wire.edges) {
                CHECK(use.start_vertex && use.end_vertex);
                ledger[use.edge.value].push_back(
                    NativeUse{face, use, !wire.outer});
            }
        }
    }
    std::vector<std::uint64_t> certified_partitions;
    std::set<std::uint64_t> material_edges;
    for (const auto& [edge, uses] : ledger) {
        const kernel::RuntimeEdgeToken token{edge};
        const auto observation = std::find_if(
            cut.current_edge_semantics.begin(),
            cut.current_edge_semantics.end(),
            [token](const auto& item) {
                return item.runtime_token == token;
            });
        CHECK(observation !=
              cut.current_edge_semantics.end());
        if (observation->same_surface_partition) {
            CHECK(!observation->periodic_seam);
            CHECK(uses.size() == 2U);
            CHECK(uses[0].face != uses[1].face);
            CHECK(!uses[0].inner && !uses[1].inner);
            CHECK(uses[0].use.reversed !=
                  uses[1].use.reversed);
            CHECK(uses[0].use.start_vertex ==
                  uses[1].use.end_vertex);
            CHECK(uses[0].use.end_vertex ==
                  uses[1].use.start_vertex);
            certified_partitions.push_back(edge);
        } else {
            CHECK(!observation->periodic_seam);
            CHECK(uses.size() == 1U);
            material_edges.insert(edge);
        }
    }
    std::cerr
        << "PG01D_E0_CROSS_PARTITION_HOLE_LEDGER"
        << " native_faces=" << inherited->current_faces.size()
        << " native_outer_wires=" << native_outer.size()
        << " native_inner_wires=" << native_inner_wires
        << " certified_partitions=" << certified_partitions.size()
        << " unique_material_edges=" << material_edges.size()
        << '\n';
    CHECK(certified_partitions.size() == 2U);
    CHECK(inherited->current_faces.size() == 2U);
    CHECK(native_outer.size() == 2U);
    // The transverse through-hole has NO per-Face inner wire:
    // the hole crosses the representation partition and is instead
    // represented by MATERIAL arcs on both Face outer wires.
    CHECK(native_inner_wires == 0U);
    const auto directed_closed = [](
        const std::vector<kernel::FaceBoundaryEdgeUse>& ring) {
        if (ring.empty()) return false;
        for (std::size_t i = 0; i < ring.size(); ++i) {
            const auto& a = ring[i];
            const auto& b = ring[(i + 1U) % ring.size()];
            if (!a.valid() || !a.end_vertex ||
                !b.start_vertex ||
                a.end_vertex != b.start_vertex) {
                return false;
            }
        }
        return true;
    };
    std::vector<std::vector<kernel::FaceBoundaryEdgeUse>>
        stitched = native_outer;
    std::size_t joined_rings = 0U;
    std::size_t split_rings = 0U;
    for (const auto partition : certified_partitions) {
        std::vector<std::pair<std::size_t, std::size_t>>
            positions;
        for (std::size_t li = 0U; li < stitched.size(); ++li) {
            for (std::size_t ui = 0U;
                 ui < stitched[li].size(); ++ui) {
                if (stitched[li][ui].edge.value == partition) {
                    positions.emplace_back(li, ui);
                }
            }
        }
        CHECK(positions.size() == 2U);
        const auto [first_loop, first_use] = positions[0];
        const auto [second_loop, second_use] = positions[1];
        const auto& first = stitched[first_loop][first_use];
        const auto& second = stitched[second_loop][second_use];
        CHECK(first.reversed != second.reversed);
        CHECK(first.start_vertex == second.end_vertex);
        CHECK(first.end_vertex == second.start_vertex);
        std::vector<std::vector<kernel::FaceBoundaryEdgeUse>>
            next;
        for (std::size_t li = 0U; li < stitched.size(); ++li) {
            if (li != first_loop && li != second_loop) {
                next.push_back(std::move(stitched[li]));
            }
        }
        if (first_loop != second_loop) {
            // Cancel one internal partition joining TWO distinct
            // native Face rings. Exact OCCT directed ends, not
            // geometric point tolerance, close the joined sequence.
            std::vector<kernel::FaceBoundaryEdgeUse> combined;
            const auto append_after = [&combined](
                const std::vector<kernel::FaceBoundaryEdgeUse>&
                    original,
                std::size_t removed) {
                for (std::size_t n = 1U;
                     n < original.size(); ++n) {
                    combined.push_back(original[
                        (removed + n) % original.size()]);
                }
            };
            append_after(
                stitched[first_loop], first_use);
            append_after(
                stitched[second_loop], second_use);
            CHECK(directed_closed(combined));
            next.push_back(std::move(combined));
            ++joined_rings;
        } else {
            // The SECOND opposite partition pair is now contained
            // twice in the SAME native directed loop. Deleting it
            // must SPLIT the loop into two independently closed
            // material contours, not fail or silently discard one.
            CHECK(first_use < second_use);
            const auto& loop = stitched[first_loop];
            std::vector<kernel::FaceBoundaryEdgeUse> interior;
            std::vector<kernel::FaceBoundaryEdgeUse> exterior;
            for (std::size_t i = first_use + 1U;
                 i < second_use; ++i) {
                interior.push_back(loop[i]);
            }
            for (std::size_t step = 1U;
                 step < loop.size() -
                     (second_use - first_use);
                 ++step) {
                exterior.push_back(
                    loop[(second_use + step) % loop.size()]);
            }
            CHECK(directed_closed(interior));
            CHECK(directed_closed(exterior));
            next.push_back(std::move(interior));
            next.push_back(std::move(exterior));
            ++split_rings;
        }
        stitched = std::move(next);
    }
    CHECK(joined_rings == 1U);
    CHECK(split_rings == 1U);
    CHECK(stitched.size() == 2U);
    std::set<std::uint64_t> retained_tokens;
    std::set<std::uint64_t> circle_tokens;
    std::size_t circle_bearing_contours = 0U;
    for (const auto& contour : stitched) {
        CHECK(directed_closed(contour));
        std::size_t contour_circles = 0U;
        for (const auto& use : contour) {
            CHECK(material_edges.count(use.edge.value) == 1U);
            CHECK(retained_tokens.insert(use.edge.value).second);
            const auto edge =
                std::find_if(
                    cut.current_edge_semantics.begin(),
                    cut.current_edge_semantics.end(),
                    [&use](const auto& item) {
                        return item.runtime_token == use.edge;
                    });
            CHECK(edge != cut.current_edge_semantics.end());
            CHECK(!edge->same_surface_partition);
            CHECK(!edge->periodic_seam);
            if (edge->provider_curve_kind == kernel::CurveKind::circle) {
                ++contour_circles;
                circle_tokens.insert(use.edge.value);
            }
        }
        if (contour_circles != 0U) {
            ++circle_bearing_contours;
        }
    }
    CHECK(retained_tokens == material_edges);
    CHECK(circle_tokens.size() >= 2U);
    CHECK(circle_bearing_contours == 1U);
    // A stage-scoped SEMANTIC witness for THIS bounded test:
    // an opening created by an identified Cut cylindrical side has
    // all its material boundary edges incident to that exact Cut
    // Surface token; the external material boundary has none.
    // Neither contour is classified by Edge shape, winding sign,
    // coordinates, nesting heuristics or viewer tessellation.
    std::size_t side_incident_contours = 0U;
    std::size_t outside_contours = 0U;
    for (const auto& contour : stitched) {
        std::size_t touches_cut_side = 0U;
        for (const auto& use : contour) {
            const auto observation = std::find_if(
                cut.current_edge_semantics.begin(),
                cut.current_edge_semantics.end(),
                [&use](const auto& item) {
                    return item.runtime_token == use.edge;
                });
            CHECK(observation !=
                  cut.current_edge_semantics.end());
            const auto& adjacent = observation->adjacent_surfaces;
            CHECK(std::find(
                adjacent.begin(), adjacent.end(),
                *base_wall->resolved_token) != adjacent.end());
            const bool incident_to_new_cut_side =
                std::find(
                    adjacent.begin(), adjacent.end(),
                    *cut_cylinder->resolved_token) != adjacent.end();
            if (incident_to_new_cut_side) ++touches_cut_side;
        }
        if (touches_cut_side == contour.size()) {
            ++side_incident_contours;
        } else {
            CHECK(touches_cut_side == 0U);
            ++outside_contours;
        }
    }
    CHECK(side_incident_contours == 1U);
    CHECK(outside_contours == 1U);
    // OCCT can give native Circle-arc provenance for one contour,
    // but material Edge curve kind ALONE is not a general
    // certification of outer-vs-hole surface-side classification.
    std::cout
        << "PG01D_E0_NATIVE_CROSS_PARTITION_TWO_CONTOURS_PASS"
        << " same_carrier_native_faces="
        << inherited->current_faces.size()
        << " exact_partition_pairs="
        << certified_partitions.size()
        << " cancelled_join_steps=" << joined_rings
        << " cancelled_split_steps=" << split_rings
        << " exact_directed_material_contours="
        << stitched.size()
        << " retained_unique_material_edges="
        << retained_tokens.size()
        << " native_face_inner_wires=" << native_inner_wires
        << " circular_source_members=" << circle_tokens.size()
        << " cut_side_exact_semantic_witness=1"
        << " contour_all_cut_surface_edges="
        << side_incident_contours
        << " contour_zero_cut_surface_edges="
        << outside_contours
        << " general_outer_hole_witness=0"
        << " coordinate_joins=0"
        << '\n';

    // Repeat the REAL native cross-partition through Cut on a
    // disjoint section of the SAME inherited planar carrier. This
    // cannot be reduced to reading two Circle members of independent
    // Face-inner wires: BOTH holes straddle the certified Face split.
    // Demand two distinct current semantic generated Cut side tokens.
    kernel::PlanarProfileInput second_hole;
    second_hole.frame = hole_frame;
    second_hole.outer.boundary = {{
        kernel::Circle2{{10.0, 21.0}, 2.0},
        0.0, 1.0, true, false, true,
        kernel::BoundaryUseProvenance{
            "cross-partition-second-hole", 0U, 0U, false},
    }};
    CHECK(second_hole.valid());
    auto second_input = add(std::move(second_hole), 45.0);
    second_input.operation = kernel::SolidBooleanOperation::cut;
    CHECK(second_input.valid());
    const auto twice_cut = provider.extrude(
        second_input, cut.solid);
    CHECK(twice_cut.ok());
    CHECK(twice_cut.solid_count == 1U);
    const auto* twice_carrier = inheritedSurface(
        twice_cut, *base_wall->resolved_token);
    const auto* first_cut_side_inherited = inheritedSurface(
        twice_cut, *cut_cylinder->resolved_token);
    const auto* second_cut_side =
        newSide(twice_cut, "cross-partition-second-hole");
    CHECK(twice_carrier);
    CHECK(first_cut_side_inherited);
    CHECK(second_cut_side);
    CHECK(twice_carrier->surface_status ==
          kernel::ReferenceStatus::resolved);
    CHECK(twice_carrier->surface_kind ==
          kernel::SurfaceKind::plane);
    CHECK(first_cut_side_inherited->surface_status ==
          kernel::ReferenceStatus::resolved);
    CHECK(first_cut_side_inherited->surface_kind ==
          kernel::SurfaceKind::cylinder);
    CHECK(second_cut_side->surface_status ==
          kernel::ReferenceStatus::resolved);
    CHECK(second_cut_side->surface_kind ==
          kernel::SurfaceKind::cylinder);
    CHECK(second_cut_side->resolved_token);
    CHECK(*cut_cylinder->resolved_token !=
          *second_cut_side->resolved_token);
    CHECK(twice_carrier->current_faces.size() == 2U);

    // Only per-Face provider-owned ORIENTED wires and precise current
    // native Edge, Vertex, Surface identities participate in this
    // ledger. Three same-carrier partitions are expected: one to
    // the left, one between, and one to the right of the two holes.
    std::vector<std::vector<kernel::FaceBoundaryEdgeUse>>
        twice_outer;
    std::map<std::uint64_t, std::vector<NativeUse>>
        twice_uses;
    std::size_t twice_inner = 0U;
    for (const auto face : twice_carrier->current_faces) {
        const auto scoped = provider.bindFaceToBody(
            twice_cut.solid, face);
        CHECK(scoped && scoped->valid());
        const auto native =
            provider.queryFaceBoundaryAnySurface(
                twice_cut.solid, *scoped);
        CHECK(native.ok());
        std::size_t face_outer = 0U;
        for (const auto& wire : native.wires) {
            CHECK(wire.valid());
            if (wire.outer) {
                ++face_outer;
                twice_outer.push_back(wire.edges);
            } else {
                ++twice_inner;
            }
            for (const auto& use : wire.edges) {
                CHECK(use.start_vertex && use.end_vertex);
                twice_uses[use.edge.value].push_back(
                    NativeUse{face, use, !wire.outer});
            }
        }
        CHECK(face_outer == 1U);
    }
    CHECK(twice_outer.size() == 2U);
    CHECK(twice_inner == 0U);

    std::vector<std::uint64_t> twice_partitions;
    std::set<std::uint64_t> twice_material;
    std::set<std::uint64_t> first_hole_sources;
    std::set<std::uint64_t> second_hole_sources;
    for (const auto& [value, members] : twice_uses) {
        const kernel::RuntimeEdgeToken token{value};
        const auto record = std::find_if(
            twice_cut.current_edge_semantics.begin(),
            twice_cut.current_edge_semantics.end(),
            [token](const auto& edge) {
                return edge.runtime_token == token;
            });
        CHECK(record !=
              twice_cut.current_edge_semantics.end());
        CHECK(!record->periodic_seam);
        const auto& sides = record->adjacent_surfaces;
        CHECK(std::find(
            sides.begin(), sides.end(),
            *base_wall->resolved_token) != sides.end());
        if (record->same_surface_partition) {
            CHECK(members.size() == 2U);
            CHECK(members[0].face != members[1].face);
            CHECK(!members[0].inner && !members[1].inner);
            CHECK(members[0].use.reversed !=
                  members[1].use.reversed);
            CHECK(members[0].use.start_vertex ==
                  members[1].use.end_vertex);
            CHECK(members[0].use.end_vertex ==
                  members[1].use.start_vertex);
            twice_partitions.push_back(value);
            continue;
        }
        CHECK(members.size() == 1U);
        CHECK(!members.front().inner);
        CHECK(twice_material.insert(value).second);
        const bool from_first = std::find(
            sides.begin(), sides.end(),
            *cut_cylinder->resolved_token) != sides.end();
        const bool from_second = std::find(
            sides.begin(), sides.end(),
            *second_cut_side->resolved_token) != sides.end();
        CHECK(!from_first || !from_second);
        if (from_first) {
            first_hole_sources.insert(value);
        } else if (from_second) {
            second_hole_sources.insert(value);
        }
    }
    std::cerr
        << "PG01D_E0_NATIVE_TWO_CROSS_PARTITION_CUTS_PROBE"
        << " same_carrier_faces="
        << twice_carrier->current_faces.size()
        << " exact_partition_pairs=" << twice_partitions.size()
        << " per_face_inner_wires=" << twice_inner
        << " first_cut_source_edges=" << first_hole_sources.size()
        << " second_cut_source_edges=" << second_hole_sources.size()
        << '\n';
    CHECK(twice_partitions.size() == 3U);
    CHECK(first_hole_sources.size() >= 2U);
    CHECK(second_hole_sources.size() >= 2U);

    // Cancel the THREE exact and opposite-directed shared partition
    // pairs. One join followed by two splits produces three distinct
    // exact directed material cycles. The algorithm is a TEST-ONLY
    // native incidence proof, never a production E1 region selector.
    auto twice_cycles = twice_outer;
    std::size_t twice_joins = 0U;
    std::size_t twice_splits = 0U;
    for (const auto partition : twice_partitions) {
        std::vector<std::pair<std::size_t, std::size_t>>
            occurrences;
        for (std::size_t ring = 0U; ring < twice_cycles.size();
             ++ring) {
            for (std::size_t pos = 0U;
                 pos < twice_cycles[ring].size(); ++pos) {
                if (twice_cycles[ring][pos].edge.value ==
                    partition) {
                    occurrences.emplace_back(ring, pos);
                }
            }
        }
        CHECK(occurrences.size() == 2U);
        const auto [ai, aj] = occurrences[0];
        const auto [bi, bj] = occurrences[1];
        const auto& first = twice_cycles[ai][aj];
        const auto& other = twice_cycles[bi][bj];
        CHECK(first.reversed != other.reversed);
        CHECK(first.start_vertex == other.end_vertex);
        CHECK(first.end_vertex == other.start_vertex);
        std::vector<std::vector<kernel::FaceBoundaryEdgeUse>>
            remaining;
        for (std::size_t ring = 0U; ring < twice_cycles.size();
             ++ring) {
            if (ring != ai && ring != bi) {
                remaining.push_back(std::move(twice_cycles[ring]));
            }
        }
        if (ai != bi) {
            std::vector<kernel::FaceBoundaryEdgeUse> joined_cycle;
            for (std::size_t step = 1U;
                 step < twice_cycles[ai].size(); ++step) {
                joined_cycle.push_back(twice_cycles[ai][
                    (aj + step) % twice_cycles[ai].size()]);
            }
            for (std::size_t step = 1U;
                 step < twice_cycles[bi].size(); ++step) {
                joined_cycle.push_back(twice_cycles[bi][
                    (bj + step) % twice_cycles[bi].size()]);
            }
            CHECK(directed_closed(joined_cycle));
            remaining.push_back(std::move(joined_cycle));
            ++twice_joins;
        } else {
            CHECK(aj < bj);
            const auto& cycle = twice_cycles[ai];
            std::vector<kernel::FaceBoundaryEdgeUse> inside;
            std::vector<kernel::FaceBoundaryEdgeUse> outside;
            for (std::size_t pos = aj + 1U; pos < bj; ++pos) {
                inside.push_back(cycle[pos]);
            }
            for (std::size_t step = 1U;
                 step < cycle.size() - (bj - aj); ++step) {
                outside.push_back(
                    cycle[(bj + step) % cycle.size()]);
            }
            CHECK(directed_closed(inside));
            CHECK(directed_closed(outside));
            remaining.push_back(std::move(inside));
            remaining.push_back(std::move(outside));
            ++twice_splits;
        }
        twice_cycles = std::move(remaining);
    }
    CHECK(twice_joins == 1U);
    CHECK(twice_splits == 2U);
    CHECK(twice_cycles.size() == 3U);

    std::set<std::uint64_t> retained_twice;
    std::size_t first_cut_contours = 0U;
    std::size_t second_cut_contours = 0U;
    std::size_t outer_contours = 0U;
    for (const auto& contour : twice_cycles) {
        CHECK(directed_closed(contour));
        std::size_t from_first = 0U;
        std::size_t from_second = 0U;
        for (const auto& occurrence : contour) {
            CHECK(twice_material.count(
                occurrence.edge.value) == 1U);
            CHECK(retained_twice.insert(
                occurrence.edge.value).second);
            if (first_hole_sources.count(
                occurrence.edge.value)) ++from_first;
            if (second_hole_sources.count(
                occurrence.edge.value)) ++from_second;
        }
        // Each loop is completely attributable to ONE exact
        // generated Cut side token, or to NEITHER: any mix means
        // ambiguous per-material-surface source identity (STOP).
        CHECK(!(from_first && from_second));
        if (from_first == contour.size()) {
            ++first_cut_contours;
        } else if (from_second == contour.size()) {
            ++second_cut_contours;
        } else {
            CHECK(from_first == 0U);
            CHECK(from_second == 0U);
            ++outer_contours;
        }
    }
    CHECK(retained_twice == twice_material);
    CHECK(first_cut_contours == 1U);
    CHECK(second_cut_contours == 1U);
    CHECK(outer_contours == 1U);

    // E0 negative controls on the actual native Edge/Vertex cycles:
    // the *classification function* must refuse even one missing,
    // conflicting, or falsely promoted member of the two Cut source
    // sets. No partial shape/coordinate healing is permissible.
    const auto exact_cut_side_classification =
        [&](const std::set<std::uint64_t>& first_sources,
            const std::set<std::uint64_t>& second_sources) {
            std::set<std::uint64_t> once;
            std::size_t first_contours = 0U;
            std::size_t second_contours = 0U;
            std::size_t unclaimed_contours = 0U;
            for (const auto& ring : twice_cycles) {
                if (!directed_closed(ring)) return false;
                std::size_t a = 0U;
                std::size_t b = 0U;
                for (const auto& member : ring) {
                    const auto token = member.edge.value;
                    if (twice_material.count(token) != 1U ||
                        !once.insert(token).second) {
                        return false;
                    }
                    if (first_sources.count(token)) ++a;
                    if (second_sources.count(token)) ++b;
                }
                if (a != 0U && b != 0U) return false;
                if (a == ring.size()) {
                    ++first_contours;
                } else if (b == ring.size()) {
                    ++second_contours;
                } else if (a == 0U && b == 0U) {
                    ++unclaimed_contours;
                } else {
                    return false;
                }
            }
            return once == twice_material &&
                first_contours == 1U &&
                second_contours == 1U &&
                unclaimed_contours == 1U;
        };
    CHECK(exact_cut_side_classification(
        first_hole_sources, second_hole_sources));
    CHECK(!first_hole_sources.empty());
    CHECK(!second_hole_sources.empty());
    auto missing_first_member = first_hole_sources;
    missing_first_member.erase(
        *missing_first_member.begin());
    CHECK(!exact_cut_side_classification(
        missing_first_member, second_hole_sources));
    auto aliased_second_cut = second_hole_sources;
    aliased_second_cut.insert(
        *first_hole_sources.begin());
    CHECK(!exact_cut_side_classification(
        first_hole_sources, aliased_second_cut));
    std::set<std::uint64_t> external_members;
    for (const auto material : twice_material) {
        if (!first_hole_sources.count(material) &&
            !second_hole_sources.count(material)) {
            external_members.insert(material);
        }
    }
    CHECK(!external_members.empty());
    auto mislabeled_exterior = first_hole_sources;
    mislabeled_exterior.insert(*external_members.begin());
    CHECK(!exact_cut_side_classification(
        mislabeled_exterior, second_hole_sources));
    auto conflated_cut_sides = first_hole_sources;
    conflated_cut_sides.insert(
        second_hole_sources.begin(),
        second_hole_sources.end());
    CHECK(!exact_cut_side_classification(
        conflated_cut_sides, second_hole_sources));
    // E0 ordering invariant on REAL current OCCT native topology:
    // token-map iteration order is NOT semantic authority. Enumerate
    // every 3! partition cancellation order and certify identical
    // signed material contours up to cyclic rotation and contour
    // permutation. This is TEST-ONLY surgery, not an E1 algorithm.
    using E0Use = kernel::FaceBoundaryEdgeUse;
    using E0Cycles = std::vector<std::vector<E0Use>>;
    const std::set<std::uint64_t> authentic_partitions{
        twice_partitions.begin(), twice_partitions.end()};
    CHECK(authentic_partitions.size() == 3U);
    const auto cancel_in_order =
        [&](const E0Cycles& original,
            const std::vector<std::uint64_t>& order)
            -> std::optional<E0Cycles> {
            if (order.size() != authentic_partitions.size() ||
                std::set<std::uint64_t>{
                    order.begin(), order.end()} !=
                    authentic_partitions) {
                return std::nullopt;
            }
            auto cycles = original;
            for (const auto& cycle : cycles) {
                if (!directed_closed(cycle)) return std::nullopt;
            }
            for (const auto partition : order) {
                std::vector<std::pair<std::size_t, std::size_t>>
                    locations;
                for (std::size_t li = 0U; li < cycles.size(); ++li) {
                    for (std::size_t ui = 0U;
                         ui < cycles[li].size(); ++ui) {
                        if (cycles[li][ui].edge.value ==
                            partition) {
                            locations.emplace_back(li, ui);
                        }
                    }
                }
                if (locations.size() != 2U) return std::nullopt;
                const auto [ai, aj] = locations[0];
                const auto [bi, bj] = locations[1];
                const auto& a = cycles[ai][aj];
                const auto& b = cycles[bi][bj];
                if (!a.valid() || !b.valid() ||
                    a.reversed == b.reversed ||
                    !a.start_vertex || !a.end_vertex ||
                    a.start_vertex != b.end_vertex ||
                    a.end_vertex != b.start_vertex) {
                    return std::nullopt;
                }
                E0Cycles next;
                for (std::size_t li = 0U;
                     li < cycles.size(); ++li) {
                    if (li != ai && li != bi) {
                        next.push_back(std::move(cycles[li]));
                    }
                }
                if (ai != bi) {
                    std::vector<E0Use> joined;
                    for (std::size_t i = 1U;
                         i < cycles[ai].size(); ++i) {
                        joined.push_back(cycles[ai][
                            (aj + i) % cycles[ai].size()]);
                    }
                    for (std::size_t i = 1U;
                         i < cycles[bi].size(); ++i) {
                        joined.push_back(cycles[bi][
                            (bj + i) % cycles[bi].size()]);
                    }
                    if (!directed_closed(joined)) {
                        return std::nullopt;
                    }
                    next.push_back(std::move(joined));
                } else {
                    if (aj >= bj) return std::nullopt;
                    const auto& ring = cycles[ai];
                    std::vector<E0Use> inside;
                    std::vector<E0Use> outside;
                    for (std::size_t i = aj + 1U;
                         i < bj; ++i) {
                        inside.push_back(ring[i]);
                    }
                    for (std::size_t i = 1U;
                         i < ring.size() - (bj - aj);
                         ++i) {
                        outside.push_back(ring[
                            (bj + i) % ring.size()]);
                    }
                    if (!directed_closed(inside) ||
                        !directed_closed(outside)) {
                        return std::nullopt;
                    }
                    next.push_back(std::move(inside));
                    next.push_back(std::move(outside));
                }
                cycles = std::move(next);
            }
            std::set<std::uint64_t> remaining_material;
            for (const auto& cycle : cycles) {
                if (!directed_closed(cycle)) return std::nullopt;
                for (const auto& use : cycle) {
                    if (authentic_partitions.count(
                            use.edge.value) != 0U ||
                        twice_material.count(
                            use.edge.value) != 1U ||
                        !remaining_material.insert(
                            use.edge.value).second) {
                        return std::nullopt;
                    }
                }
            }
            if (remaining_material != twice_material ||
                cycles.size() != 3U) {
                return std::nullopt;
            }
            return cycles;
        };

    // Compare complete (Edge token, occurrence direction) sequences,
    // not only undirected Edge membership. Ignore only cyclic origin
    // and order of the distinct resulting region contours.
    const auto canonical_signed_cycles =
        [](const E0Cycles& cycles) {
            using SignedEdge =
                std::pair<std::uint64_t, bool>;
            std::vector<std::vector<SignedEdge>> signatures;
            for (const auto& cycle : cycles) {
                std::vector<SignedEdge> signed_edges;
                for (const auto& use : cycle) {
                    signed_edges.emplace_back(
                        use.edge.value, use.reversed);
                }
                const auto smallest = std::min_element(
                    signed_edges.begin(), signed_edges.end());
                std::rotate(
                    signed_edges.begin(), smallest,
                    signed_edges.end());
                signatures.push_back(std::move(signed_edges));
            }
            std::sort(signatures.begin(), signatures.end());
            return signatures;
        };
    auto reference_order = twice_partitions;
    std::sort(
        reference_order.begin(), reference_order.end());
    const auto initial_cancellation =
        cancel_in_order(twice_outer, reference_order);
    CHECK(initial_cancellation.has_value());
    CHECK(canonical_signed_cycles(*initial_cancellation) ==
          canonical_signed_cycles(twice_cycles));
    const auto expected_cycles =
        canonical_signed_cycles(*initial_cancellation);
    std::size_t certified_orders = 0U;
    do {
        const auto result =
            cancel_in_order(twice_outer, reference_order);
        CHECK(result.has_value());
        CHECK(canonical_signed_cycles(*result) ==
              expected_cycles);
        ++certified_orders;
    } while (std::next_permutation(
        reference_order.begin(), reference_order.end()));
    CHECK(certified_orders == 6U);

    // E0 exact native wire enumeration independence. The OCCT
    // provider does NOT promise which bounded Face wire is returned
    // first, nor which directed Edge use begins a cyclic outer wire.
    // Exhaust each independent cyclic rotation of the two REAL
    // Face outer rings, both Face enumeration orders, and all 3!
    // authenticated partition cancellation orders. An accidental
    // dependency on a chosen first Edge or first Face is a STOP.
    CHECK(twice_outer.size() == 2U);
    CHECK(twice_outer[0].size() >= 2U);
    CHECK(twice_outer[1].size() >= 2U);
    std::size_t native_wire_variants = 0U;
    std::size_t complete_order_checks = 0U;
    for (std::size_t first_offset = 0U;
         first_offset < twice_outer[0].size();
         ++first_offset) {
        for (std::size_t second_offset = 0U;
             second_offset < twice_outer[1].size();
             ++second_offset) {
            E0Cycles rotated = twice_outer;
            std::rotate(
                rotated[0].begin(),
                rotated[0].begin() + first_offset,
                rotated[0].end());
            std::rotate(
                rotated[1].begin(),
                rotated[1].begin() + second_offset,
                rotated[1].end());
            CHECK(directed_closed(rotated[0]));
            CHECK(directed_closed(rotated[1]));
            for (int swapped = 0; swapped != 2; ++swapped) {
                if (swapped == 1) {
                    std::swap(rotated[0], rotated[1]);
                }
                auto order = twice_partitions;
                std::sort(order.begin(), order.end());
                do {
                    const auto candidate =
                        cancel_in_order(rotated, order);
                    CHECK(candidate.has_value());
                    CHECK(canonical_signed_cycles(*candidate) ==
                          expected_cycles);
                    ++complete_order_checks;
                } while (std::next_permutation(
                    order.begin(), order.end()));
                ++native_wire_variants;
            }
        }
    }
    CHECK(native_wire_variants ==
          2U * twice_outer[0].size() * twice_outer[1].size());
    CHECK(complete_order_checks ==
          certified_orders * native_wire_variants);

    // Native Vertex tokens matter independently of Edge orientation:
    // a forged pair of endpoints is not a geometrically close fit.
    // A missing outer Edge use must also refuse reconstruction.
    auto false_vertex = twice_outer;
    bool damaged_vertex = false;
    for (auto& ring : false_vertex) {
        for (auto& use : ring) {
            if (use.edge.value == twice_partitions.front()) {
                CHECK(use.start_vertex && use.end_vertex);
                CHECK(use.start_vertex != use.end_vertex);
                use.start_vertex = use.end_vertex;
                damaged_vertex = true;
                break;
            }
        }
        if (damaged_vertex) break;
    }
    CHECK(damaged_vertex);
    CHECK(!cancel_in_order(
        false_vertex, twice_partitions).has_value());
    auto dropped_outer_use = twice_outer;
    bool dropped = false;
    for (auto& ring : dropped_outer_use) {
        const auto member = std::find_if(
            ring.begin(), ring.end(),
            [&twice_material](const auto& use) {
                return twice_material.count(
                    use.edge.value) != 0U;
            });
        if (member != ring.end()) {
            ring.erase(member);
            dropped = true;
            break;
        }
    }
    CHECK(dropped);
    CHECK(!cancel_in_order(
        dropped_outer_use, twice_partitions).has_value());

    std::cout
        << "PG01D_E0_NATIVE_WIRE_ENUMERATION_INVARIANCE_PASS"
        << " native_face_wires=2"
        << " independent_cyclic_starts="
        << twice_outer[0].size() * twice_outer[1].size()
        << " face_order_permutations=2"
        << " partition_orders=6"
        << " validated_enumeration_variants="
        << native_wire_variants
        << " total_exact_signed_checks="
        << complete_order_checks
        << " forged_vertex_endpoints_rejected=1"
        << " missing_native_material_member_rejected=1"
        << " xy_coordinate_matching=0"
        << '\n';

    // Negative mutations of the genuine native ledger: input Edge
    // direction and exact Vertex closure are authority, and dropping
    // a partition is never permitted as a partial reconstruction.
    auto reversed_partition = twice_outer;
    std::size_t flipped_occurrences = 0U;
    for (auto& ring : reversed_partition) {
        for (auto& use : ring) {
            if (use.edge.value == twice_partitions.front()) {
                use.reversed = !use.reversed;
                ++flipped_occurrences;
                break;
            }
        }
        if (flipped_occurrences) break;
    }
    CHECK(flipped_occurrences == 1U);
    CHECK(!cancel_in_order(
        reversed_partition, twice_partitions).has_value());
    auto truncated_order = twice_partitions;
    truncated_order.pop_back();
    CHECK(!cancel_in_order(
        twice_outer, truncated_order).has_value());

    std::cout
        << "PG01D_E0_NATIVE_PARTITION_ORDER_INDEPENDENT_PASS"
        << " native_partition_pairs=3"
        << " exhaustive_orders=" << certified_orders
        << " same_signed_material_cycles=3"
        << " corrupt_native_orientation_rejected=1"
        << " missing_partition_rejected=1"
        << " token_iteration_authority=0"
        << " coordinate_stitching=0"
        << '\n';
    std::cout
        << "PG01D_E0_TWO_CUT_SIDE_PROVENANCE_FAIL_CLOSED_PASS"
        << " one_missing_edge_rejected=1"
        << " foreign_cut_alias_rejected=1"
        << " exterior_edge_false_promote_rejected=1"
        << " two_cut_origins_conflated_rejected=1"
        << " genuine_native_contours_preserved=3"
        << '\n';
    std::cout
        << "PG01D_E0_NATIVE_TWO_CROSS_PARTITION_HOLES_PASS"
        << " same_carrier_faces=2"
        << " certified_partitions=3"
        << " cancelled_joins=" << twice_joins
        << " cancelled_splits=" << twice_splits
        << " exact_closed_material_contours=3"
        << " independent_cut_side_holes=2"
        << " unclaimed_external_contours=1"
        << " unique_material_edge_coverage=1"
        << " guessed_xyz_matching=0"
        << '\n';
}

// Native E0 negative control: a LOCAL side-wall notch leaves the
// original cylinder Surface in precisely ONE current bounded Face.
// FOCUSED #2247 deliberately rejected a two-Face assertion after
// OCCT returned inherited_faces=1. Preserve that finding: material
// continuity in 3D does not itself certify a multi-Face group.
// No new semantic association may be guessed from coaxial geometry.
void verifyNativePartialCylinderCutContinuityProbeE0() {
    kernel_occt::OcctSolidModelingKernel provider;
    kernel::PlanarProfileInput cylinder;
    cylinder.outer.boundary = {{
        kernel::Circle2{{0.0, 0.0}, 10.0},
        0.0, 1.0, true, false, true,
        kernel::BoundaryUseProvenance{
            "partial-cylinder-cut", 0U, 0U, false},
    }};
    CHECK(cylinder.valid());
    const auto base = provider.extrude(add(cylinder, 10.0));
    CHECK(base.ok());
    const auto* inherited_source =
        newSide(base, "partial-cylinder-cut");
    CHECK(inherited_source);
    CHECK(inherited_source->resolved_token);
    CHECK(inherited_source->surface_kind ==
          kernel::SurfaceKind::cylinder);

    kernel::Frame3 notch_frame;
    notch_frame.origin = {0.0, 0.0, 4.0};
    CHECK(notch_frame.valid());
    auto notch = add(rectangle(
        notch_frame, 8.5, -3.0, 12.0, 3.0,
        "partial-groove"), 2.0);
    notch.operation = kernel::SolidBooleanOperation::cut;
    CHECK(notch.valid());
    const auto cut = provider.extrude(notch, base.solid);
    CHECK(cut.ok());
    CHECK(cut.solid_count == 1U);
    const auto* inherited = inheritedSurface(
        cut, *inherited_source->resolved_token);
    CHECK(inherited);
    std::cerr
        << "PG01D_CURVED_E0_PARTIAL_NOTCH_NATIVE_PROBE"
        << " inherited_status="
        << static_cast<int>(inherited->surface_status)
        << " inherited_faces="
        << inherited->current_faces.size()
        << " inherited_strict_face="
        << static_cast<int>(inherited->strict_face_status)
        << '\n';
    CHECK(inherited->surface_status ==
          kernel::ReferenceStatus::resolved);
    CHECK(inherited->surface_kind ==
          kernel::SurfaceKind::cylinder);
    CHECK(inherited->current_faces.size() == 1U);
    std::map<std::uint64_t, std::set<std::uint64_t>>
        edge_face_uses;
    for (const auto token : inherited->current_faces) {
        const auto bound =
            provider.bindFaceToBody(cut.solid, token);
        CHECK(bound && bound->valid());
        const auto native =
            provider.queryFaceBoundaryAnySurface(
                cut.solid, *bound);
        CHECK(native.ok());
        for (const auto& wire : native.wires) {
            CHECK(wire.valid());
            for (const auto& use : wire.edges) {
                CHECK(use.start_vertex && use.end_vertex);
                edge_face_uses[use.edge.value].insert(
                    token.value);
            }
        }
    }
    std::size_t shared_native_edges = 0U;
    for (const auto& [edge, faces] : edge_face_uses) {
        if (faces.size() > 1U) ++shared_native_edges;
    }
    std::cerr
        << "PG01D_CURVED_E0_PARTIAL_NOTCH_NATIVE_ADJACENCY"
        << " carrier_faces=" << inherited->current_faces.size()
        << " shared_native_edges=" << shared_native_edges
        << '\n';
    // One bounded Face supplies no cross-Face native adjacency;
    // no positive multi-Face certificate can be manufactured here.
    CHECK(shared_native_edges == 0U);
    std::cout
        << "PG01D_CURVED_E0_LOCAL_NOTCH_SINGLE_FACE_STOP_PASS"
        << " resolved_inherited_cylinder=1"
        << " same_carrier_native_faces=1"
        << " shared_between_distinct_faces=0"
        << " geometry_only_multi_face_inference=0"
        << '\n';
}

// E0 pure native-identity graph counterexample for a GENERAL region
// stitcher: TWO bounded Face outer rings can share TWO certified
// opposite-oriented representation partitions, e.g. an annular
// surface divided into upper and lower sectors. Removing both
// partitions yields TWO closed material contour cycles, not one.
// This fixture is synthetic *topology only*, not a claimed native
// OCCT Part or an Owner model. In particular, identifying which
// cycle is OUTER vs HOLE requires an independent surface-region
// containment/side certificate; Edge identity alone cannot do it.
void verifyPg01dE0CyclicPartitionTwoContours() {
    using Use = kernel::FaceBoundaryEdgeUse;
    using kernel::RuntimeEdgeToken;
    using kernel::RuntimeVertexToken;
    const auto use = [](
        std::uint64_t edge, std::uint64_t start,
        std::uint64_t end, bool reversed = false) {
        return Use{
            RuntimeEdgeToken{edge}, reversed,
            RuntimeVertexToken{start},
            RuntimeVertexToken{end}};
    };
    // Shared exact vertices: outer-left/right 301/302 and
    // inner-left/right 303/304. Native partition Edge 405 joins
    // right endpoints, 406 joins left endpoints.
    const std::vector<Use> top_face{
        use(401U, 301U, 302U),
        use(405U, 302U, 304U),
        use(403U, 304U, 303U),
        use(406U, 303U, 301U),
    };
    const std::vector<Use> bottom_face{
        use(402U, 302U, 301U),
        use(406U, 301U, 303U, true),
        use(404U, 303U, 304U),
        use(405U, 304U, 302U, true),
    };
    const auto closed = [](
        const std::vector<Use>& ring) {
        if (ring.empty()) return false;
        for (std::size_t i = 0U; i < ring.size(); ++i) {
            const auto& a = ring[i];
            const auto& b = ring[(i + 1U) % ring.size()];
            if (!a.valid() || !a.start_vertex ||
                !a.end_vertex || !b.start_vertex ||
                *a.end_vertex != *b.start_vertex) return false;
        }
        return true;
    };
    CHECK(closed(top_face));
    CHECK(closed(bottom_face));
    CHECK((kernel::FaceBoundaryWire{true, top_face}).valid());
    CHECK((kernel::FaceBoundaryWire{true, bottom_face}).valid());
    CHECK(top_face[1].edge == bottom_face[3].edge);
    CHECK(top_face[1].reversed != bottom_face[3].reversed);
    CHECK(top_face[1].start_vertex ==
          bottom_face[3].end_vertex);
    CHECK(top_face[1].end_vertex ==
          bottom_face[3].start_vertex);
    CHECK(top_face[3].edge == bottom_face[1].edge);
    CHECK(top_face[3].reversed != bottom_face[1].reversed);
    CHECK(top_face[3].start_vertex ==
          bottom_face[1].end_vertex);
    CHECK(top_face[3].end_vertex ==
          bottom_face[1].start_vertex);

    // First certified partition 405 joins TWO separate native Face
    // outer rings. The second partition 406 then occurs twice on
    // the resulting SAME ring. An implementation that merely
    // insists every partition joins two different current loops
    // rejects this valid combinatorial annulus class.
    std::vector<Use> joined;
    for (std::size_t k = 1U; k < top_face.size(); ++k) {
        joined.push_back(
            top_face[(1U + k) % top_face.size()]);
    }
    for (std::size_t k = 1U; k < bottom_face.size(); ++k) {
        joined.push_back(
            bottom_face[(3U + k) % bottom_face.size()]);
    }
    CHECK(joined.size() == 6U);
    CHECK(closed(joined));
    std::vector<std::size_t> second_partition_positions;
    for (std::size_t i = 0U; i < joined.size(); ++i) {
        if (joined[i].edge == RuntimeEdgeToken{406U}) {
            second_partition_positions.push_back(i);
        }
    }
    CHECK(second_partition_positions.size() == 2U);
    const auto first = second_partition_positions[0];
    const auto second = second_partition_positions[1];
    CHECK(joined[first].reversed != joined[second].reversed);
    CHECK(joined[first].start_vertex ==
          joined[second].end_vertex);
    CHECK(joined[first].end_vertex ==
          joined[second].start_vertex);
    // Removing the second exact partition pair splits one
    // directed cyclic list into TWO independently closed loops.
    std::vector<Use> first_contour;
    std::vector<Use> second_contour;
    for (std::size_t i = first + 1U; i < second; ++i) {
        first_contour.push_back(joined[i]);
    }
    for (std::size_t offset = 1U;
         offset < joined.size() - (second - first);
         ++offset) {
        second_contour.push_back(
            joined[(second + offset) % joined.size()]);
    }
    CHECK(first_contour.size() == 2U);
    CHECK(second_contour.size() == 2U);
    CHECK(closed(first_contour));
    CHECK(closed(second_contour));
    std::set<std::uint64_t> material_sources;
    for (const auto& ring : {first_contour, second_contour}) {
        for (const auto& member : ring) {
            CHECK(member.edge.value != 405U);
            CHECK(member.edge.value != 406U);
            CHECK(material_sources.insert(
                member.edge.value).second);
        }
    }
    CHECK(material_sources ==
          (std::set<std::uint64_t>{401U, 402U, 403U, 404U}));
    // Two real original Face OUTER wires are NOT evidence that
    // their composite has one outer loop: absent an independent
    // oriented region/containment witness, the provider-owned
    // source identity can certify TWO contours but NOT label
    // one of them as the outer vs an opening.
    CHECK(first_contour.front().edge == RuntimeEdgeToken{401U});
    CHECK(second_contour.front().edge == RuntimeEdgeToken{404U});
    std::cout
        << "PG01D_FACE_BOUNDARY_E0_CYCLIC_PARTITION_TWO_LOOPS_PASS"
        << " original_native_outer_rings=2"
        << " certified_opposite_partition_pairs=2"
        << " first_partition_joins_rings=1"
        << " second_partition_splits_ring=1"
        << " resulting_directed_material_cycles=2"
        << " unique_material_edges=4"
        << " external_outer_hole_witness_required=1"
        << " inferred_hole_from_edge_graph=0"
        << '\n';
}

// E0 typed/portable negative matrix: malformed per-use endpoint coverage
// or wrong native Edge direction must be rejected at FaceBoundaryResult's
// public validation boundary, BEFORE any Part-authorable sources are used.
// These tiny tokens are synthetic, never an Owner geometry/fixture.
void verifyPg01dE0DirectedWireFailClosedContract() {
    using kernel::FaceBoundaryEdgeUse;
    using kernel::FaceBoundaryWire;
    using kernel::FaceBoundaryResult;
    using kernel::FaceBoundaryStatus;
    using kernel::RuntimeEdgeToken;
    using kernel::RuntimeVertexToken;

    const FaceBoundaryWire directed_outer{
        true,
        {
            FaceBoundaryEdgeUse{
                RuntimeEdgeToken{101U}, false,
                RuntimeVertexToken{201U},
                RuntimeVertexToken{202U}},
            FaceBoundaryEdgeUse{
                RuntimeEdgeToken{102U}, false,
                RuntimeVertexToken{202U},
                RuntimeVertexToken{203U}},
            FaceBoundaryEdgeUse{
                RuntimeEdgeToken{103U}, false,
                RuntimeVertexToken{203U},
                RuntimeVertexToken{201U}},
        },
    };
    const FaceBoundaryWire directed_circle{
        false,
        {
            FaceBoundaryEdgeUse{
                RuntimeEdgeToken{104U}, false,
                RuntimeVertexToken{204U},
                RuntimeVertexToken{204U}},
        },
    };
    CHECK(directed_outer.valid());
    CHECK(directed_circle.valid());
    CHECK((FaceBoundaryResult{
        FaceBoundaryStatus::ok,
        {directed_outer, directed_circle}}).ok());

    // Reversing just one oriented occurrence keeps the undirected
    // Edge/Vertex incidence identical, but breaks directed continuity.
    auto reverse_single_use = directed_outer;
    std::swap(
        reverse_single_use.edges[1].start_vertex,
        reverse_single_use.edges[1].end_vertex);
    CHECK(!reverse_single_use.valid());
    CHECK(!(FaceBoundaryResult{
        FaceBoundaryStatus::ok,
        {reverse_single_use, directed_circle}}).ok());

    // A pair of fully formed, valid Vertex tokens from other Edges is
    // NOT a valid continuation without exact shared OCCT identity.
    auto mismatched_vertex = directed_outer;
    mismatched_vertex.edges[1].start_vertex =
        RuntimeVertexToken{205U};
    CHECK(!mismatched_vertex.valid());

    // Half present start/end is always malformed.
    auto incomplete_use = directed_outer;
    incomplete_use.edges[1].end_vertex.reset();
    CHECK(!incomplete_use.edges[1].valid());
    CHECK(!incomplete_use.valid());

    // A valid Edge-only provider is still backward compatible.
    // But it is NOT evidence of signed topological closure.
    auto legacy_outer = directed_outer;
    for (auto& use : legacy_outer.edges) {
        use.start_vertex.reset();
        use.end_vertex.reset();
    }
    CHECK(legacy_outer.valid());
    CHECK((FaceBoundaryResult{
        FaceBoundaryStatus::ok,
        {legacy_outer}}).ok());
    CHECK(!legacy_outer.edges.front().start_vertex);

    // The WHOLE returned FaceBoundaryResult must have one atomic
    // directed endpoint-coverage mode, including ALL inner wires.
    // An unsigned inner hole cannot silently pass as a complete
    // provider-signed Face result merely because its outer is signed.
    auto legacy_circle = directed_circle;
    for (auto& use : legacy_circle.edges) {
        use.start_vertex.reset();
        use.end_vertex.reset();
    }
    CHECK(legacy_circle.valid());
    CHECK((FaceBoundaryResult{
        FaceBoundaryStatus::ok,
        {legacy_outer, legacy_circle}}).ok());
    CHECK(!(FaceBoundaryResult{
        FaceBoundaryStatus::ok,
        {directed_outer, legacy_circle}}).ok());
    CHECK(!(FaceBoundaryResult{
        FaceBoundaryStatus::ok,
        {legacy_outer, directed_circle}}).ok());
    // Two individually sound wire observations of different
    // coverage must be refused independent of their input order.
    CHECK(!(FaceBoundaryResult{
        FaceBoundaryStatus::ok,
        {legacy_circle, directed_outer}}).ok());

    // Mixed observed/legacy uses within the SAME wire are rejected,
    // even though each individual EdgeUse is structurally valid.
    auto mixed_coverage = directed_outer;
    mixed_coverage.edges[1].start_vertex.reset();
    mixed_coverage.edges[1].end_vertex.reset();
    CHECK(mixed_coverage.edges[1].valid());
    CHECK(!mixed_coverage.valid());
    CHECK(!(FaceBoundaryResult{
        FaceBoundaryStatus::ok,
        {mixed_coverage}}).ok());

    // One valid outer is necessary and a malformed hole is fatal:
    // never accept a partial collection of native boundary wires.
    auto malformed_circle = directed_circle;
    malformed_circle.edges.front().end_vertex =
        RuntimeVertexToken{206U};
    CHECK(!malformed_circle.valid());
    CHECK(!(FaceBoundaryResult{
        FaceBoundaryStatus::ok,
        {directed_outer, malformed_circle}}).ok());
    CHECK(!(FaceBoundaryResult{
        FaceBoundaryStatus::ok,
        {directed_outer, directed_outer}}).ok());
    CHECK(!(FaceBoundaryResult{
        FaceBoundaryStatus::malformed_boundary,
        {directed_outer}}).ok());

    std::cout
        << "PG01D_FACE_BOUNDARY_E0_DIRECTED_WIRE_FAIL_CLOSED_PASS"
        << " directed_outer_and_circle=1"
        << " reversed_use_refused=1"
        << " wrong_vertex_refused=1"
        << " partial_use_refused=1"
        << " mixed_wire_refused=1"
        << " mixed_result_outer_inner_refused=1"
        << " whole_result_legacy_compatible=1"
        << " malformed_hole_refused=1"
        << " legacy_provider_compatible=1"
        << " no_coordinate_matching=1"
        << '\n';
}

} // namespace

int main() {
    kernel_occt::OcctSolidModelingKernel provider;

    kernel::Frame3 xy;
    const auto base =
        provider.extrude(
            add(
                rectangle(
                    xy,
                    0.0,
                    0.0,
                    40.0,
                    30.0,
                    "base"),
                10.0));
    CHECK(base.ok());

    const auto* base_right =
        newSide(
            base,
            "base-right");
    CHECK(base_right != nullptr);
    CHECK(
        base_right->surface_status ==
        kernel::ReferenceStatus::resolved);
    CHECK(base_right->resolved_token.has_value());

    // Boss from the top plane touches the +X outer boundary. Its generated
    // "boss-right" side extends the existing "base-right" engineering
    // Surface upward. Any retained z=10 boundary is a representation
    // partition, not a second semantic Surface.
    kernel::Frame3 top_frame;
    top_frame.origin = {0.0, 0.0, 10.0};
    CHECK(top_frame.valid());

    const auto extended =
        provider.extrude(
            add(
                rectangle(
                    top_frame,
                    20.0,
                    5.0,
                    40.0,
                    25.0,
                    "boss"),
                10.0),
            base.solid);
    CHECK(extended.ok());

    const auto* inherited_right =
        inheritedSurface(
            extended,
            *base_right->resolved_token);
    CHECK(inherited_right != nullptr);
    CHECK(
        inherited_right->surface_status ==
        kernel::ReferenceStatus::resolved);
    CHECK(
        inherited_right->canonical_frame.has_value());

    const auto* created_right =
        newSide(
            extended,
            "boss-right");
    CHECK(created_right != nullptr);
    CHECK(created_right->continued_into.has_value());
    CHECK(
        *created_right->continued_into ==
        *base_right->resolved_token);
    CHECK(
        created_right->surface_status ==
        kernel::ReferenceStatus::unsupported);
    CHECK(!created_right->resolved_token.has_value());
    CHECK(created_right->current_faces.empty());
    CHECK(!created_right->contribution_faces.empty());

    CHECK(inherited_right->current_faces.size() >= 2U);
    CHECK(
        inherited_right->strict_face_status ==
        kernel::ReferenceStatus::ambiguous);

    for (const auto token :
         created_right->contribution_faces) {
        CHECK(
            std::find(
                inherited_right->current_faces.begin(),
                inherited_right->current_faces.end(),
                token) !=
            inherited_right->current_faces.end());
    }

    std::size_t partition_edges = 0U;
    for (const auto& edge :
         extended.current_edge_semantics) {
        if (!edge.same_surface_partition) {
            continue;
        }
        ++partition_edges;
        CHECK(!edge.periodic_seam);
        CHECK(edge.adjacent_surfaces.size() == 1U);
        CHECK(
            edge.adjacent_surfaces.front() ==
            *base_right->resolved_token);
    }
    CHECK(partition_edges >= 1U);

    verifyPartIntegration();
    verifyNativeCurvedSurfaceContinuationE0();
    verifyNativeCylindricalCutSplitE0();
    verifyNativePartialCylinderCutContinuityProbeE0();
    verifyNativePlanarBranchedSurfaceContinuationsE0();
    verifyNativePlanarHoleAcrossPartitionE0();
    verifyPg01dE0DirectedWireFailClosedContract();
    verifyPg01dE0CyclicPartitionTwoContours();

    std::cout
        << "PM02JR2_ADD_SURFACE_CONTINUATION_PASS"
        << " inherited_side_faces="
        << inherited_right->current_faces.size()
        << " contribution_faces="
        << created_right->contribution_faces.size()
        << " partition_edges="
        << partition_edges
        << " geometry_similarity_authority=0"
        << '\n';

    return EXIT_SUCCESS;
}
