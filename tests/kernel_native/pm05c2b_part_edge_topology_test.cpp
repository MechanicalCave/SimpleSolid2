#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel_occt/solid_modeling_kernel.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/part/datum_evaluation.hpp>
#include <simplesolid2/part/part_document_store.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-05C2b Part topology CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

part::PartDocument makeBasePart() {
    auto source =
        part::PartDocument::create(
            core::DocumentId::generate());
    application::DocumentSession session{
        {},
        std::move(source)};

    const auto sketch_created =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::
                    xy_plane});
    CHECK(
        sketch_created.ok() &&
        sketch_created.sketch_id);

    const auto rectangle =
        session.execute(
            application::AddSketchRectangleCommand{
                *sketch_created.sketch_id,
                session.document().revision(),
                {0.0, 0.0},
                {40.0, 30.0},
                sketch::EntityRole::regular,
                false});
    CHECK(rectangle.ok());

    const auto* sketch =
        session.document().findSketch(
            *sketch_created.sketch_id);
    CHECK(sketch != nullptr);
    const auto regions =
        sketch::analyzeRegions(
            sketch->model);
    CHECK(regions.complete());
    CHECK(regions.regions.size() == 1U);
    const auto intent =
        part::makeProfileRegionIntent(
            regions.regions.front());
    CHECK(intent.has_value());

    const auto profile =
        session.execute(
            application::CreateProfileCommand{
                *sketch_created.sketch_id,
                session.document().revision(),
                *intent});
    CHECK(profile.ok() && profile.profile_id);

    auto state =
        session.document().state();
    const auto base_id =
        state.body.next_feature_id.allocate();
    CHECK(base_id.has_value());
    state.body.features.push_back(
        part::PartFeature{
            *base_id,
            "Base",
            false,
            part::ExtrudeFeature{
                *profile.profile_id,
                part::ExtrudeOperation::add,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{20.0},
                    false}}});

    auto restored =
        part::PartDocument::restore(
            session.document().documentId(),
            std::move(state),
            session.document().revision());
    CHECK(restored.ok());
    return std::move(*restored.document);
}

// G0/V03: a real Part-owned Add followed by a native Cut. Each Profile has
// its own Sketch; never infer new topological identity from coordinates.
part::PartDocument withInteriorCut(
    const part::PartDocument& base,
    sketch::Point2 first_corner = {10.0, 5.0},
    sketch::Point2 opposite_corner = {30.0, 25.0},
    double cut_depth = 10.0) {
    auto restored =
        part::PartDocument::restore(
            base.documentId(),
            base.state(),
            base.revision());
    CHECK(restored.ok());
    application::DocumentSession session{
        {},
        std::move(*restored.document)};

    const auto sketch_created =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
    CHECK(
        sketch_created.ok() &&
        sketch_created.sketch_id);

    const auto cut_rectangle =
        session.execute(
            application::AddSketchRectangleCommand{
                *sketch_created.sketch_id,
                session.document().revision(),
                first_corner,
                opposite_corner,
                sketch::EntityRole::regular,
                false});
    CHECK(cut_rectangle.ok());

    const auto* cut_sketch =
        session.document().findSketch(
            *sketch_created.sketch_id);
    CHECK(cut_sketch != nullptr);
    const auto cut_regions =
        sketch::analyzeRegions(cut_sketch->model);
    CHECK(cut_regions.complete());
    CHECK(cut_regions.regions.size() == 1U);
    const auto intent =
        part::makeProfileRegionIntent(
            cut_regions.regions.front());
    CHECK(intent.has_value());
    const auto cut_profile =
        session.execute(
            application::CreateProfileCommand{
                *sketch_created.sketch_id,
                session.document().revision(),
                *intent});
    CHECK(cut_profile.ok() && cut_profile.profile_id);

    auto state = session.document().state();
    const auto cut_id =
        state.body.next_feature_id.allocate();
    CHECK(cut_id.has_value());
    state.body.features.push_back(
        part::PartFeature{
            *cut_id,
            "InteriorCut",
            false,
            part::ExtrudeFeature{
                *cut_profile.profile_id,
                part::ExtrudeOperation::cut,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{cut_depth},
                    false}}});
    auto cut_document =
        part::PartDocument::restore(
            session.document().documentId(),
            std::move(state),
            session.document().revision());
    CHECK(cut_document.ok());
    return std::move(*cut_document.document);
}

// G0/V02: edit only the upstream Extrude extent in the same authored
// Part document, retaining Feature/Sketch/Profile semantic identity.
part::PartDocument withEditedBaseHeight(
    const part::PartDocument& base,
    double new_height) {
    auto state = base.state();
    CHECK(state.body.features.size() == 1U);
    auto* extrude = std::get_if<part::ExtrudeFeature>(
        &state.body.features.front().definition);
    CHECK(extrude != nullptr);
    const auto* current =
        std::get_if<part::OneSidedExtrudeExtent>(
            &extrude->extent);
    CHECK(current != nullptr);
    extrude->extent =
        part::OneSidedExtrudeExtent{
            core::LengthValue{new_height},
            current->reversed};
    auto rebuilt = part::PartDocument::restore(
        base.documentId(),
        std::move(state),
        base.revision());
    CHECK(rebuilt.ok());
    return std::move(*rebuilt.document);
}

// G0/V12: deliberately produce the same geometric prism with a NEW
// FeatureId. The complete old Feature is no longer in the authored history.
// Equality of XYZ is a diagnostic fixture fact, never a Point address repair.
part::PartDocument withReplacementBaseFeature(
    const part::PartDocument& base) {
    auto state = base.state();
    CHECK(state.body.features.size() == 1U);
    const auto replacement_id =
        state.body.next_feature_id.allocate();
    CHECK(replacement_id.has_value());
    CHECK(
        *replacement_id !=
        state.body.features.front().id);
    state.body.features.front().id =
        *replacement_id;
    state.body.features.front().name =
        "ReplacementBase";
    auto rebuilt = part::PartDocument::restore(
        base.documentId(),
        std::move(state),
        base.revision());
    CHECK(rebuilt.ok());
    return std::move(*rebuilt.document);
}

// Q1-06 real native Part fixtures adapted from the existing PM-04C2
// provider lifecycle test: half-cylinder Cut and partial Revolve. These
// construct actual authored Feature history and current OCCT topology.
part::PartDocument makeQ1PartRevolve(
    core::BuiltinReferenceRole axis_role,
    sketch::Point2 first,
    sketch::Point2 opposite,
    double angle,
    bool reverse = false) {
    auto source =
        part::PartDocument::create(
            core::DocumentId::generate());
    application::DocumentSession session{
        {},
        std::move(source)};

    const auto sketch_created =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::
                    xy_plane});
    CHECK(
        sketch_created.ok() &&
        sketch_created.sketch_id);
    const auto sketch_id =
        *sketch_created.sketch_id;

    const auto rectangle_created =
        session.execute(
            application::AddSketchRectangleCommand{
                sketch_id,
                session.document().revision(),
                first,
                opposite,
                sketch::EntityRole::regular,
                false});
    CHECK(rectangle_created.ok());

    const auto* sketch =
        session.document().findSketch(
            sketch_id);
    CHECK(sketch != nullptr);
    const auto analysis =
        sketch::analyzeRegions(
            sketch->model);
    CHECK(analysis.complete());
    CHECK(analysis.regions.size() == 1U);
    const auto intent =
        part::makeProfileRegionIntent(
            analysis.regions.front());
    CHECK(intent.has_value());

    const auto profile =
        session.execute(
            application::CreateProfileCommand{
                sketch_id,
                session.document().revision(),
                *intent});
    CHECK(profile.ok() && profile.profile_id);

    auto state = session.document().state();
    const auto feature_id =
        state.body.next_feature_id.allocate();
    CHECK(feature_id.has_value());
    state.body.features.push_back(
        part::PartFeature{
            *feature_id,
            "Revolve001",
            false,
            part::RevolveFeature{
                *profile.profile_id,
                part::AxisReference{
                    part::BuiltinOriginAxisReference{
                        axis_role}},
                part::RevolveOperation::add,
                part::OneSidedRevolveExtent{
                    core::AngleValue{angle},
                    reverse}}});

    auto restored =
        part::PartDocument::restore(
            session.document().documentId(),
            std::move(state),
            session.document().revision());
    CHECK(restored.ok());
    return std::move(*restored.document);
}

part::PartDocument makeQ1ExtrudeCylinder() {
    auto source =
        part::PartDocument::create(
            core::DocumentId::generate());
    application::DocumentSession session{
        {},
        std::move(source)};

    const auto sketch =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
    CHECK(sketch.ok() && sketch.sketch_id);
    const auto circle =
        session.execute(
            application::AddSketchCircleCommand{
                *sketch.sketch_id,
                {0.0, 0.0},
                10.0,
                sketch::EntityRole::regular});
    CHECK(circle.ok());
    const auto* model =
        session.document().findSketch(
            *sketch.sketch_id);
    CHECK(model != nullptr);
    const auto regions =
        sketch::analyzeRegions(model->model);
    CHECK(regions.complete());
    CHECK(regions.regions.size() == 1U);
    const auto intent =
        part::makeProfileRegionIntent(
            regions.regions.front());
    CHECK(intent.has_value());
    const auto profile =
        session.execute(
            application::CreateProfileCommand{
                *sketch.sketch_id,
                session.document().revision(),
                *intent});
    CHECK(profile.ok() && profile.profile_id);

    auto state = session.document().state();
    const auto id = state.body.next_feature_id.allocate();
    CHECK(id.has_value());
    state.body.features.push_back(
        part::PartFeature{
            *id,
            "ExtrudeCylinder",
            false,
            part::ExtrudeFeature{
                *profile.profile_id,
                part::ExtrudeOperation::add,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{10.0},
                    false}}});
    auto restored = part::PartDocument::restore(
        session.document().documentId(),
        std::move(state),
        session.document().revision());
    CHECK(restored.ok());
    return std::move(*restored.document);
}

// G0/V08: a real analytic cylinder clipped by one planar Boolean
// Cut through its axis. Two distinct native rim Vertices can share the
// same THREE semantic Surface sources (cap, cylinder, cut wall).
part::PartDocument withQ1CylinderHalfCut(
    const part::PartDocument& source) {
    auto restored = part::PartDocument::restore(
        source.documentId(),
        source.state(),
        source.revision());
    CHECK(restored.ok());
    application::DocumentSession session{
        {},
        std::move(*restored.document)};
    const auto sketch_created = session.execute(
        application::CreatePartSketchCommand{
            core::BuiltinReferenceRole::xy_plane});
    CHECK(
        sketch_created.ok() &&
        sketch_created.sketch_id);
    const auto rectangle = session.execute(
        application::AddSketchRectangleCommand{
            *sketch_created.sketch_id,
            session.document().revision(),
            {-15.0, -15.0},
            {0.0, 15.0},
            sketch::EntityRole::regular,
            false});
    CHECK(rectangle.ok());
    const auto* sketch = session.document().findSketch(
        *sketch_created.sketch_id);
    CHECK(sketch != nullptr);
    const auto analysis = sketch::analyzeRegions(
        sketch->model);
    CHECK(analysis.complete());
    CHECK(analysis.regions.size() == 1U);
    const auto intent = part::makeProfileRegionIntent(
        analysis.regions.front());
    CHECK(intent.has_value());
    const auto profile = session.execute(
        application::CreateProfileCommand{
            *sketch_created.sketch_id,
            session.document().revision(),
            *intent});
    CHECK(profile.ok() && profile.profile_id);
    auto state = session.document().state();
    const auto feature_id =
        state.body.next_feature_id.allocate();
    CHECK(feature_id.has_value());
    state.body.features.push_back(
        part::PartFeature{
            *feature_id,
            "G0HalfCylinderCut",
            false,
            part::ExtrudeFeature{
                *profile.profile_id,
                part::ExtrudeOperation::cut,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{12.0},
                    false}}});
    auto rebuilt = part::PartDocument::restore(
        session.document().documentId(),
        std::move(state),
        session.document().revision());
    CHECK(rebuilt.ok());
    return std::move(*rebuilt.document);
}


std::vector<part::MaterialEdgeReference>
trihedralReferences(
    const part::BodyStageTopologyCatalog& catalog) {
    for (const auto& vertex : catalog.vertices) {
        if (vertex.referenceability !=
                kernel::ReferenceStatus::resolved ||
            vertex.point_candidates.size() != 1U ||
            vertex.incident_material_edges.size() < 3U) {
            continue;
        }

        std::vector<part::MaterialEdgeReference>
            result;
        for (std::size_t index = 0U;
             index < 3U;
             ++index) {
            const auto authored =
                part::authorMaterialEdgeReference(
                    catalog,
                    vertex.incident_material_edges[
                        index]);
            if (!authored.ok()) {
                result.clear();
                break;
            }
            result.push_back(
                *authored.reference);
        }
        if (result.size() != 3U) {
            continue;
        }
        std::sort(
            result.begin(),
            result.end());
        if (std::adjacent_find(
                result.begin(),
                result.end()) ==
            result.end()) {
            return result;
        }
    }
    return {};
}

class RotatedGeneratedPlaneFrameKernel final
    : public kernel::ISolidModelingKernel {
public:
    kernel::SolidModelingResult extrude(
        const kernel::LinearExtrudeInput& input,
        kernel::RuntimeSolidHandle upstream = {}) noexcept override {
        return provider_.extrude(
            input,
            std::move(upstream));
    }

    kernel::SolidModelingResult edgeFeature(
        const kernel::EdgeFeatureInput& input,
        kernel::RuntimeSolidHandle upstream = {}) noexcept override {
        auto result =
            provider_.edgeFeature(
                input,
                std::move(upstream));
        if (!result.ok()) {
            return result;
        }

        for (auto& surface :
             result.edge_feature_surfaces) {
            if (surface.surface_kind !=
                    kernel::SurfaceKind::plane ||
                !surface.canonical_frame) {
                continue;
            }

            const auto original =
                *surface.canonical_frame;
            surface.canonical_frame->u_axis =
                original.v_axis;
            surface.canonical_frame->v_axis = {
                -original.u_axis.x,
                -original.u_axis.y,
                -original.u_axis.z};
            CHECK(surface.canonical_frame->valid());
        }
        return result;
    }

private:
    kernel_occt::OcctSolidModelingKernel
        provider_;
};

using PlanarFrameEvidence =
    std::vector<
        std::pair<
            part::FeatureSurfaceAddress,
            kernel::Frame3>>;

PlanarFrameEvidence generatedPlanarFrames(
    const part::FeatureEvaluation& feature) {
    PlanarFrameEvidence result;
    for (const auto& surface :
         feature.produced_surfaces) {
        if (surface.surface_kind !=
                kernel::SurfaceKind::plane) {
            continue;
        }
        CHECK(surface.canonical_frame);
        CHECK(surface.canonical_frame->valid());
        result.emplace_back(
            surface.address,
            *surface.canonical_frame);
    }
    std::sort(
        result.begin(),
        result.end(),
        [](const auto& first, const auto& second) {
            return first.first < second.first;
        });
    return result;
}

part::PartDocument withEdgeFeature(
    const part::PartDocument& source,
    std::vector<part::MaterialEdgeReference> edges,
    bool chamfer,
    double parameter) {
    auto state = source.state();
    const auto feature_id =
        state.body.next_feature_id.allocate();
    CHECK(feature_id.has_value());
    if (chamfer) {
        state.body.features.push_back(
            part::PartFeature{
                *feature_id,
                "ChamferProbe",
                false,
                part::ChamferFeature{
                    std::move(edges),
                    core::LengthValue{
                        parameter}}});
    } else {
        state.body.features.push_back(
            part::PartFeature{
                *feature_id,
                "FilletProbe",
                false,
                part::FilletFeature{
                    std::move(edges),
                    core::LengthValue{
                        parameter}}});
    }

    auto restored =
        part::PartDocument::restore(
            source.documentId(),
            std::move(state),
            source.revision());
    CHECK(restored.ok());
    return std::move(*restored.document);
}

std::vector<part::MaterialEdgeReference>
singleEdgeCandidates(
    const part::BodyStageTopologyCatalog& catalog) {
    std::vector<part::MaterialEdgeReference>
        result;
    for (const auto& edge : catalog.edges) {
        if (edge.accounting_class !=
                part::TopologyAccountingClass::referenceable ||
            edge.referenceability !=
                kernel::ReferenceStatus::resolved) {
            continue;
        }
        const auto authored =
            part::authorMaterialEdgeReference(
                catalog,
                edge.runtime_token);
        if (authored.ok()) {
            result.push_back(
                *authored.reference);
        }
    }

    std::sort(
        result.begin(),
        result.end());
    result.erase(
        std::unique(
            result.begin(),
            result.end()),
        result.end());
    return result;
}

std::optional<std::vector<part::MaterialEdgeReference>>
explicitContourReferences(
    const part::FeatureEvaluation& upstream_feature,
    const part::MaterialEdgeReference& seed,
    kernel_occt::OcctSolidModelingKernel& kernel) {
    if (!upstream_feature.result_topology ||
        !upstream_feature.result_solid) {
        return std::nullopt;
    }

    const auto seed_resolution =
        part::resolveMaterialEdgeReference(
            seed,
            *upstream_feature.result_topology);
    if (!seed_resolution ||
        !seed_resolution->resolved()) {
        return std::nullopt;
    }

    const kernel::EdgeFeatureInput probe{
        kernel::EdgeFeatureOperation::chamfer,
        {seed_resolution->current_edges.front()},
        0.75};
    CHECK(probe.valid());

    const auto probe_result =
        kernel.edgeFeature(
            probe,
            upstream_feature.result_solid);
    if (!probe_result.edge_feature_input_membership ||
        !probe_result.edge_feature_input_membership
             ->valid()) {
        return std::nullopt;
    }

    const auto& contour =
        probe_result.edge_feature_input_membership
            ->provider_contour_edges;
    CHECK(!contour.empty());
    CHECK(
        std::find(
            contour.begin(),
            contour.end(),
            seed_resolution
                ->current_edges.front()) !=
        contour.end());

    std::vector<part::MaterialEdgeReference>
        authored;
    authored.reserve(contour.size());
    for (const auto token : contour) {
        const auto result =
            part::authorMaterialEdgeReference(
                *upstream_feature.result_topology,
                token);
        if (!result.ok()) {
            return std::nullopt;
        }
        authored.push_back(
            *result.reference);
    }

    std::sort(
        authored.begin(),
        authored.end());
    authored.erase(
        std::unique(
            authored.begin(),
            authored.end()),
        authored.end());
    if (authored.empty()) {
        return std::nullopt;
    }

    // This helper is test-only evidence extraction. It proves that the native
    // contour required by T1 can be expressed completely as durable semantic
    // authored intent. Product code never expands one authored Edge into this
    // set automatically.
    return authored;
}

std::size_t countRole(
    const part::FeatureEvaluation& feature,
    part::FeatureSurfaceRoleKind role) {
    return static_cast<std::size_t>(
        std::count_if(
            feature.produced_surfaces.begin(),
            feature.produced_surfaces.end(),
            [role](const auto& surface) {
                return surface.address.role ==
                       role;
            }));
}

std::vector<part::MaterialEdgeReference>
generatedBoundaryReferences(
    const part::FeatureEvaluation& feature) {
    std::vector<part::MaterialEdgeReference>
        result;
    if (!feature.result_topology) {
        return result;
    }

    for (const auto& curve :
         feature.produced_curves) {
        if (curve.address.role !=
                part::FeatureCurveRoleKind::
                    edge_feature_boundary ||
            curve.status !=
                kernel::ReferenceStatus::resolved ||
            curve.strict_edge_status !=
                kernel::ReferenceStatus::resolved ||
            curve.current_edges.size() != 1U) {
            continue;
        }
        const auto authored =
            part::authorMaterialEdgeReference(
                *feature.result_topology,
                curve.current_edges.front());
        if (authored.ok()) {
            result.push_back(
                *authored.reference);
        }
    }

    std::sort(
        result.begin(),
        result.end());
    result.erase(
        std::unique(
            result.begin(),
            result.end()),
        result.end());
    return result;
}


struct G0VertexCounts final {
    std::size_t native{};
    std::size_t resolved{};
    std::size_t ambiguous{};
    std::size_t missing{};
    std::size_t unsupported{};
    std::size_t point_witnesses{};
    std::size_t diagnostic_xyz{};
};

// RG-02A/G0: read-only *native Part-stage* characterization. A click in
// the Viewer is not itself a durable Point, and XYZ never grants identity.
// Retain every unsupported/ambiguous outcome as evidence, not a test fix.
G0VertexCounts characterizeNativeVertices(
    const part::BodyStageTopologyCatalog& catalog,
    const char* fixture) {
    CHECK(catalog.valid());
    CHECK(catalog.complete());

    G0VertexCounts counts;
    counts.native = catalog.vertices.size();
    for (const auto& vertex : catalog.vertices) {
        CHECK(vertex.runtime_token.valid());
        CHECK(
            vertex.accounting_class !=
            part::TopologyAccountingClass::
                integrity_failure);
        if (vertex.provider_point) {
            ++counts.diagnostic_xyz;
        }
        switch (vertex.referenceability) {
        case kernel::ReferenceStatus::resolved: {
            ++counts.resolved;
            CHECK(vertex.point_candidates.size() == 1U);
            const auto& address =
                vertex.point_candidates.front();
            CHECK(address.valid());
            CHECK(address.adjacent_surfaces.size() == 3U);
            const auto point =
                std::find_if(
                    catalog.points.begin(),
                    catalog.points.end(),
                    [&address](const auto& candidate) {
                        return candidate.address == address;
                    });
            CHECK(point != catalog.points.end());
            CHECK(
                point->status ==
                kernel::ReferenceStatus::resolved);
            CHECK(point->current_vertices.size() == 1U);
            CHECK(
                point->current_vertices.front() ==
                vertex.runtime_token);
            ++counts.point_witnesses;
            break;
        }
        case kernel::ReferenceStatus::ambiguous:
            ++counts.ambiguous;
            break;
        case kernel::ReferenceStatus::missing:
            ++counts.missing;
            break;
        case kernel::ReferenceStatus::unsupported:
            ++counts.unsupported;
            break;
        }
    }
    CHECK(
        counts.native ==
        counts.resolved + counts.ambiguous +
            counts.missing + counts.unsupported);
    std::cout
        << "RG02A_G0_VERTEX_NATIVE"
        << " fixture=" << fixture
        << " native=" << counts.native
        << " resolved=" << counts.resolved
        << " ambiguous=" << counts.ambiguous
        << " missing=" << counts.missing
        << " unsupported=" << counts.unsupported
        << " unique_point_witnesses=" << counts.point_witnesses
        << " provider_xyz_diagnostic=" << counts.diagnostic_xyz
        << " stage_valid=" << catalog.stage.valid()
        << " viewport_pickability=NOT_PROVEN"
        << " cold_rebuild=NOT_PROVEN\n";
    return counts;
}


// RG-02B1/E0-01/02: evidence from real current-stage OCCT Part topology.
// The real three-Surface semantic Point must be independently Resolved
// BEFORE consulting transient material-Edge incidence. This never authors
// an Edge-local END selector or upgrades an Ambiguous Point by adjacency.
using E0LineEndWitness = std::pair<
    part::MaterialEdgeReference,
    std::vector<part::FeaturePointAddress>>;

std::vector<E0LineEndWitness>
characterizeE0CanonicalStraightEnds(
    const part::BodyStageTopologyCatalog& catalog,
    const char* fixture) {
    CHECK(catalog.valid());
    CHECK(catalog.complete());
    CHECK(catalog.edges.size() == 12U);
    CHECK(catalog.vertices.size() == 8U);

    std::vector<E0LineEndWitness> witnesses;
    std::size_t independently_resolved_end_uses = 0U;
    for (const auto& edge : catalog.edges) {
        CHECK(edge.curve_kind == kernel::CurveKind::line);
        CHECK(edge.accounting_class ==
              part::TopologyAccountingClass::referenceable);
        CHECK(edge.referenceability ==
              kernel::ReferenceStatus::resolved);
        const auto authored =
            part::authorMaterialEdgeReference(
                catalog, edge.runtime_token);
        CHECK(authored.ok());
        CHECK(authored.reference->stage == catalog.stage);
        const auto current =
            part::resolveMaterialEdgeReference(
                *authored.reference, catalog);
        CHECK(current.has_value());
        CHECK(current->resolved());
        CHECK(current->current_edges.front() ==
              edge.runtime_token);

        std::vector<part::FeaturePointAddress> endpoints;
        for (const auto& vertex : catalog.vertices) {
            if (std::find(
                    vertex.incident_material_edges.begin(),
                    vertex.incident_material_edges.end(),
                    edge.runtime_token) ==
                vertex.incident_material_edges.end()) {
                continue;
            }
            // Critically, this Point must already be Resolved independently.
            CHECK(vertex.referenceability ==
                  kernel::ReferenceStatus::resolved);
            CHECK(vertex.point_candidates.size() == 1U);
            const auto& address =
                vertex.point_candidates.front();
            CHECK(address.valid());
            const auto point =
                std::find_if(
                    catalog.points.begin(),
                    catalog.points.end(),
                    [&address](const auto& candidate) {
                        return candidate.address == address;
                    });
            CHECK(point != catalog.points.end());
            CHECK(point->status ==
                  kernel::ReferenceStatus::resolved);
            CHECK(point->current_vertices.size() == 1U);
            CHECK(point->current_vertices.front() ==
                  vertex.runtime_token);
            endpoints.push_back(address);
            ++independently_resolved_end_uses;
        }
        CHECK(endpoints.size() == 2U);
        std::sort(endpoints.begin(), endpoints.end());
        CHECK(endpoints.front() != endpoints.back());
        witnesses.emplace_back(
            *authored.reference, std::move(endpoints));
    }
    CHECK(witnesses.size() == 12U);
    CHECK(independently_resolved_end_uses == 24U);
    std::sort(witnesses.begin(), witnesses.end());
    CHECK(
        std::adjacent_find(
            witnesses.begin(), witnesses.end(),
            [](const auto& first, const auto& second) {
                return first.first == second.first;
            }) == witnesses.end());

    // A shared trihedral corner has three independently strict Edges.
    // Membership here is an assertion about an already-Resolved Point,
    // never a way to choose a descendant of an Ambiguous semantic Point.
    std::size_t strict_three_edge_corners = 0U;
    for (const auto& vertex : catalog.vertices) {
        CHECK(vertex.referenceability ==
              kernel::ReferenceStatus::resolved);
        CHECK(vertex.point_candidates.size() == 1U);
        CHECK(vertex.incident_material_edges.size() == 3U);
        std::size_t semantic_incident_edges = 0U;
        for (const auto& witness : witnesses) {
            if (std::find(
                    witness.second.begin(),
                    witness.second.end(),
                    vertex.point_candidates.front()) !=
                witness.second.end()) {
                ++semantic_incident_edges;
            }
        }
        CHECK(semantic_incident_edges == 3U);
        ++strict_three_edge_corners;
    }
    CHECK(strict_three_edge_corners == 8U);

    // E0-05: the Point source is independently Resolved at this SAME
    // stage before any geometric classification. XYZ is used only as
    // current provider diagnostic geometry to prove non-collinearity,
    // never to author, find or persist a semantic source identity.
    const auto certifiedVertex = [&catalog](
        const part::FeaturePointAddress& address) {
        const auto found = std::find_if(
            catalog.vertices.begin(), catalog.vertices.end(),
            [&address](const auto& vertex) {
                return vertex.referenceability ==
                           kernel::ReferenceStatus::resolved &&
                       vertex.point_candidates.size() == 1U &&
                       vertex.point_candidates.front() == address;
            });
        CHECK(found != catalog.vertices.end());
        CHECK(found->provider_point.has_value());
        return &*found;
    };

    std::size_t independent_offline_pairs = 0U;
    for (const auto& witness : witnesses) {
        CHECK(witness.second.size() == 2U);
        const auto& a =
            *certifiedVertex(witness.second[0])->provider_point;
        const auto& b =
            *certifiedVertex(witness.second[1])->provider_point;
        const double dx = b.x - a.x;
        const double dy = b.y - a.y;
        const double dz = b.z - a.z;
        const double line_length_squared =
            dx * dx + dy * dy + dz * dz;
        CHECK(line_length_squared > 1.0);
        for (const auto& other : catalog.vertices) {
            CHECK(other.point_candidates.size() == 1U);
            const auto& independent_address =
                other.point_candidates.front();
            if (std::find(
                    witness.second.begin(),
                    witness.second.end(),
                    independent_address) !=
                witness.second.end()) {
                continue;
            }
            const auto& p =
                *certifiedVertex(independent_address)
                     ->provider_point;
            const double ex = p.x - a.x;
            const double ey = p.y - a.y;
            const double ez = p.z - a.z;
            const double cx = dy * ez - dz * ey;
            const double cy = dz * ex - dx * ez;
            const double cz = dx * ey - dy * ex;
            const double cross_squared =
                cx * cx + cy * cy + cz * cz;
            CHECK(cross_squared >
                  1.0e-12 * line_length_squared);
            ++independent_offline_pairs;
        }
    }
    // Every one of the 12 box Edges has six other uniquely certified
    // Point sources that are off its infinite supporting line.
    CHECK(independent_offline_pairs == 72U);

    std::cout << "RG02B1_E0_NATIVE_END"
              << " fixture=" << fixture
              << " strict_line_edges=" << witnesses.size()
              << " independent_resolved_end_uses="
              << independently_resolved_end_uses
              << " strict_three_edge_corners="
              << strict_three_edge_corners
              << " declared_stage_consistent=1"
              << " nearest_xyz_rebind=0"
              << " durable_edge_local_end=NOT_IMPLEMENTED"
              << " ui_point_first=NOT_TESTED"
              << " 3d_mid_query=NOT_TESTED\\n";
    return witnesses;
}


std::vector<double> verifyQ0StrictPartStageGeometry(
    const part::FeatureEvaluation& feature,
    kernel_occt::OcctSolidModelingKernel& provider,
    const char* fixture) {
    CHECK(feature.result_solid);
    CHECK(feature.result_topology);
    const auto& catalog = *feature.result_topology;
    CHECK(catalog.valid() && catalog.complete());

    // Part semantic author + same-stage resolution MUST precede every
    // transient provider query. Numeric token/physical XYZ is never
    // allowed to become a persistent source ID.
    std::vector<double> lengths;
    for (const auto& material : catalog.edges) {
        if (material.curve_kind != kernel::CurveKind::line ||
            material.referenceability != kernel::ReferenceStatus::resolved) {
            continue;
        }
        const auto authored = part::authorMaterialEdgeReference(
            catalog, material.runtime_token);
        CHECK(authored.ok());
        CHECK(authored.reference->stage == catalog.stage);
        const auto resolved = part::resolveMaterialEdgeReference(
            *authored.reference, catalog);
        CHECK(resolved.has_value());
        CHECK(resolved->resolved());
        CHECK(resolved->current_edges.size() == 1U);

        const auto bound = provider.bindExactEdgeToBody(
            feature.result_solid,
            resolved->current_edges.front());
        CHECK(bound && bound->valid());
        const auto result = provider.queryExactBoundedEdge3d(
            feature.result_solid, *bound);
        CHECK(result.ok());
        CHECK(result.geometry->kind ==
              kernel::ExactEdge3dKind::bounded_line);
        CHECK(result.geometry->bounded_length_mm > 0.0);
        const auto& g = *result.geometry;
        const auto& a = g.current_terminal_positions_world[0];
        const auto& b = g.current_terminal_positions_world[1];
        CHECK(std::abs(g.midpoint_at_half_bounded_length_world.x -
                   (a.x+b.x)/2.0) < 1.0e-6);
        CHECK(std::abs(g.midpoint_at_half_bounded_length_world.y -
                   (a.y+b.y)/2.0) < 1.0e-6);
        CHECK(std::abs(g.midpoint_at_half_bounded_length_world.z -
                   (a.z+b.z)/2.0) < 1.0e-6);
        lengths.push_back(g.bounded_length_mm);
    }
    CHECK(lengths.size() == 12U);
    std::sort(lengths.begin(), lengths.end());
    std::cout << "RG02B1_Q0_STRICT_PART_STAGE"
              << " fixture=" << fixture
              << " source_stage_authorized_first=1"
              << " bounded_native_line_edges=" << lengths.size()
              << " runtime_XYZ_not_durable=1\n";
    return lengths;
}

// Test-only Q0 decorator: reorders complete native endpoint tuples (or
// corrupts one tuple's token). Part must join by certified runtime membership,
// never by OCCT traversal order or a coordinate/first/last heuristic.
class Q1TerminalTupleQuery final :
    public kernel::IExactBoundedEdgeGeometryQuery {
public:
    Q1TerminalTupleQuery(
        kernel::IExactBoundedEdgeGeometryQuery& delegate,
        bool mismatch,
        bool duplicate_terminal_token = false)
        : delegate_{delegate}, mismatch_{mismatch},
          duplicate_terminal_token_{duplicate_terminal_token} {}

    [[nodiscard]] std::optional<kernel::ScopedExactEdge>
    bindExactEdgeToBody(
        kernel::RuntimeSolidHandle body,
        kernel::RuntimeEdgeToken token) noexcept override {
        return delegate_.bindExactEdgeToBody(std::move(body), token);
    }

    [[nodiscard]] kernel::ExactEdge3dResult queryExactBoundedEdge3d(
        kernel::RuntimeSolidHandle body,
        const kernel::ScopedExactEdge& edge) noexcept override {
        auto q = delegate_.queryExactBoundedEdge3d(
            std::move(body), edge);
        if (!q.ok()) return q;
        auto& g = *q.geometry;
        std::swap(g.current_terminal_vertex_tokens[0],
                  g.current_terminal_vertex_tokens[1]);
        std::swap(g.current_terminal_positions_world[0],
                  g.current_terminal_positions_world[1]);
        std::swap(g.terminal_tangent_lines[0],
                  g.terminal_tangent_lines[1]);
        if (mismatch_) {
            g.current_terminal_vertex_tokens[0] =
                kernel::RuntimeVertexToken{999999999U};
        }
        if (duplicate_terminal_token_) {
            g.current_terminal_vertex_tokens[0] =
                g.current_terminal_vertex_tokens[1];
        }
        return q;
    }
private:
    kernel::IExactBoundedEdgeGeometryQuery& delegate_;
    bool mismatch_;
    bool duplicate_terminal_token_;
};

// A foreign OCCT *instance* is not a different Body: Q0 authority is the
// exact RuntimeSolidHandle. This deliberately misbinds an otherwise valid
// native Edge to a DIFFERENT actual Body and must be rejected before query.
class Q1ForeignBodyBinderQuery final :
    public kernel::IExactBoundedEdgeGeometryQuery {
public:
    Q1ForeignBodyBinderQuery(
        kernel::IExactBoundedEdgeGeometryQuery& delegate,
        kernel::RuntimeSolidHandle foreign_body,
        kernel::RuntimeEdgeToken foreign_edge)
        : delegate_{delegate},
          foreign_body_{std::move(foreign_body)},
          foreign_edge_{foreign_edge} {}

    [[nodiscard]] std::optional<kernel::ScopedExactEdge>
    bindExactEdgeToBody(
        kernel::RuntimeSolidHandle,
        kernel::RuntimeEdgeToken) noexcept override {
        return delegate_.bindExactEdgeToBody(
            foreign_body_, foreign_edge_);
    }

    [[nodiscard]] kernel::ExactEdge3dResult queryExactBoundedEdge3d(
        kernel::RuntimeSolidHandle body,
        const kernel::ScopedExactEdge& edge) noexcept override {
        return delegate_.queryExactBoundedEdge3d(
            std::move(body), edge);
    }
private:
    kernel::IExactBoundedEdgeGeometryQuery& delegate_;
    kernel::RuntimeSolidHandle foreign_body_;
    kernel::RuntimeEdgeToken foreign_edge_;
};

// Q2-M06: inject native provider failure and malformed exact MID payload.
// A rejected MID never leaks a current Edge or geometry observation.
class Q2BadMidQuery final :
    public kernel::IExactBoundedEdgeGeometryQuery {
public:
    enum class Failure {
        kernel_failure,
        nonfinite_mid,
        zero_tangent,
        zero_length,
    };
    Q2BadMidQuery(
        kernel::IExactBoundedEdgeGeometryQuery& delegate,
        Failure failure)
        : delegate_{delegate}, failure_{failure} {}

    [[nodiscard]] std::optional<kernel::ScopedExactEdge>
    bindExactEdgeToBody(
        kernel::RuntimeSolidHandle body,
        kernel::RuntimeEdgeToken edge) noexcept override {
        return delegate_.bindExactEdgeToBody(std::move(body), edge);
    }

    [[nodiscard]] kernel::ExactEdge3dResult queryExactBoundedEdge3d(
        kernel::RuntimeSolidHandle body,
        const kernel::ScopedExactEdge& edge) noexcept override {
        auto current = delegate_.queryExactBoundedEdge3d(
            std::move(body), edge);
        if (!current.ok()) return current;
        switch (failure_) {
        case Failure::kernel_failure:
            current.geometry.reset();
            current.status = kernel::ExactEdge3dStatus::kernel_failure;
            break;
        case Failure::nonfinite_mid:
            current.geometry->midpoint_at_half_bounded_length_world.x =
                std::numeric_limits<double>::infinity();
            break;
        case Failure::zero_tangent:
            current.geometry->midpoint_tangent_line.representative =
                {0.0, 0.0, 0.0};
            break;
        case Failure::zero_length:
            current.geometry->bounded_length_mm = 0.0;
            break;
        }
        return current;
    }
private:
    kernel::IExactBoundedEdgeGeometryQuery& delegate_;
    Failure failure_;
};

// Q1-06: real Fillet/Chamfer generated Vertices remain globally
// Unsupported; incident Edge selection cannot upgrade their Point meaning.
void verifyQ1NoGlobalPointPromotion(
    const part::PartDocument& document,
    const part::PartEvaluation& evaluation,
    kernel_occt::OcctSolidModelingKernel& provider,
    const char* fixture) {
    CHECK(!evaluation.features.empty());
    CHECK(evaluation.features.back().result_topology);
    const auto& catalog =
        *evaluation.features.back().result_topology;
    CHECK(catalog.complete());
    std::size_t unsupported_vertices = 0U;
    std::size_t rejected_end_uses = 0U;
    for (const auto& vertex : catalog.vertices) {
        if (vertex.referenceability !=
                kernel::ReferenceStatus::unsupported &&
            vertex.referenceability !=
                kernel::ReferenceStatus::ambiguous) {
            continue;
        }
        ++unsupported_vertices;
        const auto original_status = vertex.referenceability;
        for (const auto edge : vertex.incident_material_edges) {
            const auto attempt = part::authorSelectedMaterialEdgeEnd(
                document, evaluation, catalog.stage,
                edge, vertex.runtime_token, provider);
            CHECK(!attempt.ok());
            CHECK(!attempt.reference);
            CHECK(attempt.status !=
                  part::SelectedEdgeEndStatus::resolved);
            CHECK(attempt.status !=
                  part::SelectedEdgeEndStatus::native_geometry_unavailable);
            CHECK(vertex.referenceability == original_status);
            ++rejected_end_uses;
        }
    }
    CHECK(unsupported_vertices >= 1U);
    CHECK(rejected_end_uses >= 1U);
    // Q2-M07: real Fillet/Chamfer may have unsupported endpoint
    // Points; MID does not adopt their Point meaning. It may resolve
    // only when the independently strict Edge and Q0 exact MID do.
    CHECK(evaluation.features.back().result_solid);
    std::size_t q2_admitted_mids = 0U;
    std::size_t q2_checked_edges = 0U;
    for (const auto& edge : catalog.edges) {
        const auto authored =
            part::authorSelectedMaterialEdgeMid(
                document, evaluation, catalog.stage,
                edge.runtime_token, provider);
        const auto strict = part::authorMaterialEdgeReference(
            catalog, edge.runtime_token);
        if (!strict.ok()) {
            CHECK(!authored.ok());
            CHECK(!authored.reference);
            ++q2_checked_edges;
            continue;
        }
        const auto bound = provider.bindExactEdgeToBody(
            evaluation.features.back().result_solid,
            edge.runtime_token);
        const auto exact = bound
            ? provider.queryExactBoundedEdge3d(
                evaluation.features.back().result_solid, *bound)
            : kernel::ExactEdge3dResult{};
        if (!exact.ok()) {
            CHECK(!authored.ok());
            CHECK(!authored.reference);
        } else {
            CHECK(authored.ok());
            const auto replay =
                part::resolveSelectedMaterialEdgeMid(
                    document, *authored.reference,
                    evaluation, provider);
            CHECK(replay.resolved());
            CHECK(replay.current_edge == edge.runtime_token);
            ++q2_admitted_mids;
        }
        ++q2_checked_edges;
    }
    CHECK(q2_checked_edges == catalog.edges.size());
    CHECK(q2_checked_edges >= 1U);
    for (const auto& vertex : catalog.vertices) {
        if (vertex.referenceability ==
                kernel::ReferenceStatus::unsupported ||
            vertex.referenceability ==
                kernel::ReferenceStatus::ambiguous) {
            CHECK(vertex.referenceability !=
                  kernel::ReferenceStatus::resolved);
        }
    }
    std::cout << "RG02B1_Q2_FILLET_CHAMFER_MID"
              << " fixture=" << fixture
              << " checked_edges=" << q2_checked_edges
              << " admitted_strict_mids=" << q2_admitted_mids
              << " global_point_promotions=0\n";
    std::cout << "RG02B1_Q1_NEGATIVE"
              << " fixture=" << fixture
              << " unsupported_or_ambiguous_vertices="
              << unsupported_vertices
              << " rejected_edge_terminal_uses=" << rejected_end_uses
              << " global_point_promotions=0\n";
}

// Q2-M01/03/04: the MID is the one half-arc-length role of each
// current-stage material Edge. It has no invented FeaturePointAddress,
// terminal choice, OCCT enumeration index or persistent XYZ.
std::vector<part::SelectedMaterialEdgeMid>
verifyQ2CanonicalSelectedMids(
    const part::PartDocument& document,
    const part::PartEvaluation& evaluation,
    kernel_occt::OcctSolidModelingKernel& provider,
    const char* fixture) {
    CHECK(evaluation.source_document_id);
    CHECK(*evaluation.source_document_id == document.documentId());
    CHECK(evaluation.source_revision == document.revision());
    CHECK(evaluation.features.size() == 1U);
    CHECK(evaluation.features.front().result_solid);
    CHECK(evaluation.features.front().result_topology);
    const auto& stage = evaluation.features.front();
    const auto& catalog = *stage.result_topology;
    CHECK(catalog.complete());
    CHECK(catalog.edges.size() == 12U);
    std::vector<part::SelectedMaterialEdgeMid> sources;
    for (const auto& edge : catalog.edges) {
        CHECK(edge.curve_kind == kernel::CurveKind::line);
        const auto authored = part::authorSelectedMaterialEdgeMid(
            document, evaluation, catalog.stage,
            edge.runtime_token, provider);
        CHECK(authored.ok());
        CHECK(authored.status == part::SelectedEdgeMidStatus::resolved);
        CHECK(authored.reference->source_edge.stage == catalog.stage);
        CHECK(std::find(sources.begin(), sources.end(),
                        *authored.reference) == sources.end());
        const auto mid = part::resolveSelectedMaterialEdgeMid(
            document, *authored.reference, evaluation, provider);
        CHECK(mid.resolved());
        CHECK(mid.current_edge == edge.runtime_token);
        CHECK(mid.current_position_world);
        CHECK(mid.current_tangent_line);
        const auto bound = provider.bindExactEdgeToBody(
            stage.result_solid, edge.runtime_token);
        CHECK(bound);
        const auto exact = provider.queryExactBoundedEdge3d(
            stage.result_solid, *bound);
        CHECK(exact.ok());
        CHECK(exact.geometry->kind ==
              kernel::ExactEdge3dKind::bounded_line);
        const auto& g = *exact.geometry;
        const auto& pos = *mid.current_position_world;
        const auto& tangent = mid.current_tangent_line->representative;
        const auto& a = g.current_terminal_positions_world[0];
        const auto& b = g.current_terminal_positions_world[1];
        CHECK(std::abs(pos.x - (a.x + b.x)*0.5) < 1.0e-6);
        CHECK(std::abs(pos.y - (a.y + b.y)*0.5) < 1.0e-6);
        CHECK(std::abs(pos.z - (a.z + b.z)*0.5) < 1.0e-6);
        CHECK(std::abs(pos.x -
            g.midpoint_at_half_bounded_length_world.x) < 1.0e-9);
        CHECK(std::abs(pos.y -
            g.midpoint_at_half_bounded_length_world.y) < 1.0e-9);
        CHECK(std::abs(pos.z -
            g.midpoint_at_half_bounded_length_world.z) < 1.0e-9);
        CHECK(std::abs(tangent.x*tangent.x +
                       tangent.y*tangent.y +
                       tangent.z*tangent.z - 1.0) < 1.0e-7);
        sources.push_back(*authored.reference);
    }
    CHECK(sources.size() == 12U);
    std::cout << "RG02B1_Q2_CANONICAL_MID"
              << " fixture=" << fixture
              << " strict_line_mids=" << sources.size()
              << " Q1_END_unchanged=1"
              << " durable_XYZ=0"
              << " durable_vertex_token=0"
              << " UI_MID_tested=0\n";
    return sources;
}

// Q1-01/Q1-02: real OCCT material Edge selection plus one independently
// certified current native terminal. This helper never chooses by XYZ/order.
std::vector<part::SelectedMaterialEdgeEnd>
verifyQ1CanonicalSelectedEnds(
    const part::PartDocument& document,
    const part::PartEvaluation& evaluation,
    kernel_occt::OcctSolidModelingKernel& provider,
    const char* fixture) {
    CHECK(evaluation.source_document_id.has_value());
    CHECK(*evaluation.source_document_id == document.documentId());
    CHECK(evaluation.source_revision == document.revision());
    CHECK(evaluation.features.size() == 1U);
    CHECK(evaluation.features.front().result_topology);
    const auto& catalog =
        *evaluation.features.front().result_topology;
    CHECK(catalog.complete());
    CHECK(catalog.edges.size() == 12U);
    CHECK(catalog.vertices.size() == 8U);

    std::vector<part::SelectedMaterialEdgeEnd> result;
    for (const auto& edge : catalog.edges) {
        std::size_t end_uses = 0U;
        for (const auto& vertex : catalog.vertices) {
            if (std::find(
                    vertex.incident_material_edges.begin(),
                    vertex.incident_material_edges.end(),
                    edge.runtime_token) ==
                vertex.incident_material_edges.end()) {
                continue;
            }
            const auto authored =
                part::authorSelectedMaterialEdgeEnd(
                    document, evaluation, catalog.stage,
                    edge.runtime_token, vertex.runtime_token, provider);
            CHECK(authored.ok());
            const auto resolved =
                part::resolveSelectedMaterialEdgeEnd(
                    document, *authored.reference,
                    evaluation, provider);
            CHECK(resolved.resolved());
            CHECK(resolved.current_edge == edge.runtime_token);
            CHECK(resolved.current_terminal == vertex.runtime_token);
            CHECK(vertex.provider_point);
            CHECK(std::abs(resolved.current_position_world->x -
                           vertex.provider_point->x) < 1.0e-6);
            CHECK(std::abs(resolved.current_position_world->y -
                           vertex.provider_point->y) < 1.0e-6);
            CHECK(std::abs(resolved.current_position_world->z -
                           vertex.provider_point->z) < 1.0e-6);
            const auto& t =
                resolved.current_tangent_line->representative;
            CHECK(std::abs(t.x*t.x + t.y*t.y + t.z*t.z - 1.0) <
                  1.0e-7);
            for (const auto& other : result) {
                CHECK(other != *authored.reference);
            }
            result.push_back(*authored.reference);
            ++end_uses;
        }
        CHECK(end_uses == 2U);
    }
    CHECK(result.size() == 24U);
    std::size_t trihedral = 0U;
    for (const auto& vertex : catalog.vertices) {
        CHECK(vertex.incident_material_edges.size() == 3U);
        std::vector<part::SelectedMaterialEdgeEnd> at_corner;
        for (const auto edge : vertex.incident_material_edges) {
            const auto authored =
                part::authorSelectedMaterialEdgeEnd(
                    document, evaluation, catalog.stage, edge,
                    vertex.runtime_token, provider);
            CHECK(authored.ok());
            CHECK(std::find(result.begin(), result.end(),
                            *authored.reference) != result.end());
            at_corner.push_back(*authored.reference);
        }
        CHECK(at_corner.size() == 3U);
        CHECK(at_corner[0] != at_corner[1]);
        CHECK(at_corner[1] != at_corner[2]);
        CHECK(at_corner[0] != at_corner[2]);
        // B+corner stays B, regardless of an earlier A transient hint.
        const auto edge_b =
            part::authorSelectedMaterialEdgeEnd(
                document, evaluation, catalog.stage,
                vertex.incident_material_edges[1],
                vertex.runtime_token, provider);
        CHECK(edge_b.ok());
        CHECK(*edge_b.reference == at_corner[1]);
        CHECK(*edge_b.reference != at_corner[0]);
        ++trihedral;
    }
    CHECK(trihedral == 8U);
    std::cout << "RG02B1_Q1_CANONICAL"
              << " fixture=" << fixture
              << " semantic_edge_ends=" << result.size()
              << " trihedral_corners=" << trihedral
              << " provider_token_durable=0"
              << " xyz_rebind=0"
              << " UI_picker_tested=0\n";
    return result;
}

// S4-P0: Q1's 24 Edge-local END identities are NOT, by themselves,
// proof of eight durable Points. Certify a stage-qualified semantic Point
// independently for each END against the current real OCCT catalog.
// Native Vertex tokens only corroborate *same-evaluation* incidence;
// neither tokens nor XYZ choose or persist a Point identity.
using S4P0PointKey = std::pair<
    part::BodyStageRef, part::FeaturePointAddress>;

std::vector<S4P0PointKey> verifyS4P0NativeEndPointIdentity(
    const part::PartDocument& document,
    const part::PartEvaluation& evaluation,
    kernel_occt::OcctSolidModelingKernel& provider,
    const std::vector<part::SelectedMaterialEdgeEnd>& sources,
    const char* fixture,
    const std::vector<S4P0PointKey>* expected_keys = nullptr) {
    CHECK(evaluation.source_document_id.has_value());
    CHECK(*evaluation.source_document_id == document.documentId());
    CHECK(evaluation.source_revision == document.revision());
    CHECK(evaluation.features.size() == 1U);
    CHECK(evaluation.features.front().result_topology);
    const auto& catalog = *evaluation.features.front().result_topology;
    CHECK(catalog.complete());
    CHECK(catalog.edges.size() == 12U);
    CHECK(catalog.vertices.size() == 8U);
    CHECK(sources.size() == 24U);

    std::vector<S4P0PointKey> keys;
    std::vector<kernel::RuntimeVertexToken> current_vertices;
    std::vector<kernel::Point3> diagnostic_positions;
    std::vector<std::size_t> incidence_counts;

    for (const auto& source : sources) {
        CHECK(source.valid());
        CHECK(source.source_edge.stage == catalog.stage);
        const auto resolved = part::resolveSelectedMaterialEdgeEnd(
            document, source, evaluation, provider);
        CHECK(resolved.resolved());
        const auto point_key = S4P0PointKey{
            source.source_edge.stage,
            source.terminal.certified_semantic_point};
        CHECK(point_key.second.valid());
        const auto native_vertex = *resolved.current_terminal;
        const auto point = std::find_if(
            catalog.points.begin(), catalog.points.end(),
            [&point_key](const auto& entry) {
                return entry.address == point_key.second;
            });
        CHECK(point != catalog.points.end());
        CHECK(point->status == kernel::ReferenceStatus::resolved);
        CHECK(point->candidate_vertex_count == 1U);
        CHECK(point->current_vertices.size() == 1U);
        CHECK(point->current_vertices.front() == native_vertex);

        const auto vertex = std::find_if(
            catalog.vertices.begin(), catalog.vertices.end(),
            [native_vertex](const auto& entry) {
                return entry.runtime_token == native_vertex;
            });
        CHECK(vertex != catalog.vertices.end());
        CHECK(vertex->referenceability ==
              kernel::ReferenceStatus::resolved);
        CHECK(vertex->point_candidates.size() == 1U);
        CHECK(vertex->point_candidates.front() == point_key.second);
        CHECK(vertex->provider_point.has_value());
        CHECK(std::find(vertex->incident_material_edges.begin(),
                        vertex->incident_material_edges.end(),
                        *resolved.current_edge) !=
              vertex->incident_material_edges.end());

        const auto prior = std::find(keys.begin(), keys.end(), point_key);
        if (prior == keys.end()) {
            keys.push_back(point_key);
            current_vertices.push_back(native_vertex);
            diagnostic_positions.push_back(*vertex->provider_point);
            incidence_counts.push_back(1U);
        } else {
            const auto i = static_cast<std::size_t>(
                std::distance(keys.begin(), prior));
            // An identical semantic key cannot identify two distinct
            // native Vertices in this exact current-stage evaluation.
            CHECK(current_vertices[i] == native_vertex);
            ++incidence_counts[i];
        }
    }

    CHECK(keys.size() == 8U);
    CHECK(current_vertices.size() == keys.size());
    CHECK(diagnostic_positions.size() == keys.size());
    CHECK(std::all_of(incidence_counts.begin(), incidence_counts.end(),
                      [](std::size_t n) { return n == 3U; }));
    if (expected_keys != nullptr) {
        CHECK(expected_keys->size() == keys.size());
        for (const auto& key : *expected_keys) {
            CHECK(std::find(keys.begin(), keys.end(), key) != keys.end());
        }
    }

    // P0-02: geometric admissibility observations only, NOT a Datum
    // frame, modeling-tolerance policy or XYZ-based identity lookup.
    // Three different box-corner keys are noncollinear; selecting the same
    // corner via two other incident Edges is a duplicate, not another P.
    const auto area_squared = [](
        const kernel::Point3& a,
        const kernel::Point3& b,
        const kernel::Point3& c) {
        const double ux = b.x - a.x;
        const double uy = b.y - a.y;
        const double uz = b.z - a.z;
        const double vx = c.x - a.x;
        const double vy = c.y - a.y;
        const double vz = c.z - a.z;
        const double cx = uy * vz - uz * vy;
        const double cy = uz * vx - ux * vz;
        const double cz = ux * vy - uy * vx;
        return cx * cx + cy * cy + cz * cz;
    };
    CHECK(area_squared(diagnostic_positions[0],
                       diagnostic_positions[1],
                       diagnostic_positions[2]) > 1.0e-8);
    CHECK(area_squared(diagnostic_positions[0],
                       diagnostic_positions[1],
                       diagnostic_positions[0]) == 0.0);
    const auto& a = diagnostic_positions[0];
    const auto& b = diagnostic_positions[1];
    const kernel::Point3 diagnostic_midpoint{
        (a.x + b.x) * 0.5,
        (a.y + b.y) * 0.5,
        (a.z + b.z) * 0.5};
    CHECK(area_squared(a, diagnostic_midpoint, b) < 1.0e-16);

    std::cout << "RG02B1_S4_P0_NATIVE"
              << " fixture=" << fixture
              << " edge_local_END=" << sources.size()
              << " uniquely_resolved_stage_points=" << keys.size()
              << " incident_END_per_point=3"
              << " independently_certified=1"
              << " noncollinear_triple=1"
              << " duplicate_and_collinear_rejected_as_inputs=1"
              << " native_tokens_durable=0"
              << " xyz_identity=0"
              << " authored_Datum=0\n";
    return keys;
}

// S4A-F01/F02/F04/F06: actual Part-owned v18 ThreePointDatum source,
// strict geometric frame and Command/Undo/cold provider replay. P0's 24→8
// stage-qualified semantic keys are the ONLY candidate source identities.
void verifyS4AThreePointDatumLifecycle(
    const part::PartDocument& document,
    const part::PartEvaluation& evaluation,
    kernel_occt::OcctSolidModelingKernel& provider,
    const std::vector<S4P0PointKey>& unique_keys) {
    CHECK(unique_keys.size() == 8U);
    CHECK(evaluation.features.front().result_topology);
    const auto& catalog = *evaluation.features.front().result_topology;
    const auto positionFor = [&catalog](const S4P0PointKey& key) {
        const auto it = std::find_if(
            catalog.vertices.begin(),catalog.vertices.end(),
            [&key](const auto& v) {
                return v.referenceability == kernel::ReferenceStatus::resolved &&
                       v.point_candidates.size() == 1U &&
                       v.point_candidates.front() == key.second &&
                       v.provider_point.has_value();
            });
        CHECK(it != catalog.vertices.end());
        return *it->provider_point;
    };
    const auto crossNorm2 = [](kernel::Point3 p,
                               kernel::Point3 q,
                               kernel::Point3 r) {
        const kernel::Point3 u{q.x-p.x,q.y-p.y,q.z-p.z};
        const kernel::Point3 v{r.x-p.x,r.y-p.y,r.z-p.z};
        const double cx=u.y*v.z-u.z*v.y;
        const double cy=u.z*v.x-u.x*v.z;
        const double cz=u.x*v.y-u.y*v.x;
        return cx*cx+cy*cy+cz*cz;
    };
    // Geometry chooses a *test fixture triple* only, never durable identity.
    std::array<std::size_t,3> sample{};
    bool sample_found=false;
    for (std::size_t i=0;i<unique_keys.size() && !sample_found;++i)
        for (std::size_t j=i+1;j<unique_keys.size() && !sample_found;++j)
            for (std::size_t k=j+1;k<unique_keys.size() && !sample_found;++k)
                if (crossNorm2(positionFor(unique_keys[i]),
                               positionFor(unique_keys[j]),
                               positionFor(unique_keys[k])) > 1.0e-4) {
                    sample={i,j,k};
                    sample_found=true;
                }
    CHECK(sample_found);
    std::array<part::StageSemanticPointReference,3> points{};
    for (std::size_t i=0;i<3;++i) {
        const auto& key=unique_keys[sample[i]];
        points[i]={key.first,key.second};
    }
    CHECK(part::threePointDatumPlaneStructurallyValid(
        part::ThreePointDatumPlane{part::DatumId{1},points,false,true}) ==
        part::DatumId{1}.valid()); // structural witness, allocated ID separately

    auto cloned=part::PartDocument::restore(
        document.documentId(),document.state(),document.revision());
    CHECK(cloned.ok());
    application::DocumentSession session{{},std::move(*cloned.document)};
    const auto created=session.execute(
        application::CreateThreePointDatumPlaneCommand{
            points,session.document().revision(),false,true},provider);
    CHECK(created.ok() && created.changed && created.datum_id);
    const auto id=*created.datum_id;
    CHECK(session.document().threePointDatumPlanes().size()==1U);
    const auto first=part::evaluateDatums(
        session.document(),part::evaluatePart(session.document(),provider),
        provider);
    const auto* first_frame=first.find(id);
    CHECK(first_frame && first_frame->status==
          part::DatumPlaneEvaluationStatus::resolved && first_frame->frame);
    const auto frame=*first_frame->frame;

    const auto before=session.document().state();
    const auto depth=session.undoDepth();
    const auto duplicate=points;
    auto rejected_points=duplicate;
    rejected_points[2]=rejected_points[0];
    const auto rejected=session.execute(
        application::EditThreePointDatumPlaneCommand{
            id,rejected_points,session.document().revision(),false},provider);
    CHECK(!rejected.ok() && !rejected.changed);
    CHECK(session.document().state()==before);
    CHECK(session.undoDepth()==depth);

    const auto reversed=session.execute(
        application::EditThreePointDatumPlaneCommand{
            id,points,session.document().revision(),true},provider);
    CHECK(reversed.ok() && reversed.changed);
    CHECK(session.undoDepth()==depth+1U);
    const auto reversed_eval=part::evaluateDatums(
        session.document(),part::evaluatePart(session.document(),provider),
        provider);
    const auto* reversed_frame=reversed_eval.find(id);
    CHECK(reversed_frame && reversed_frame->frame);
    CHECK(s1Near3(reversed_frame->frame->origin,frame.origin));
    CHECK(s1Near3(reversed_frame->frame->u_axis,frame.u_axis));
    CHECK(s1Near3(reversed_frame->frame->normal,
                  {-frame.normal.x,-frame.normal.y,-frame.normal.z}));
    CHECK(session.undo().ok());
    CHECK(session.document().state()==before);
    CHECK(session.redo().ok());
    CHECK(session.document().findThreePointDatumPlane(id)->reverse);

    const auto filename =
        "ss2_s4a_three_point_native_" +
        std::to_string(std::chrono::steady_clock::now()
                           .time_since_epoch().count()) + ".ss2part";
    const auto path=std::filesystem::temp_directory_path()/filename;
    struct NativeCleanup {
        std::filesystem::path path;
        ~NativeCleanup() {
            std::error_code ec;
            std::filesystem::remove(path,ec);
        }
    } cleanup{path};
    part::PartDocumentStore store;
    const auto saved=store.createNew(path,session.document());
    CHECK(saved.ok());
    auto reopened=store.load(path);
    CHECK(reopened.ok());
    CHECK(reopened.document->threePointDatumPlanes().size()==1U);
    const auto* cold_source=reopened.document->findThreePointDatumPlane(id);
    CHECK(cold_source && cold_source->points==points);
    CHECK(cold_source->reverse);
    kernel_occt::OcctSolidModelingKernel separate;
    const auto body=part::evaluatePart(*reopened.document,separate);
    const auto cold=part::evaluateDatums(
        *reopened.document,body,separate);
    const auto* cold_frame=cold.find(id);
    CHECK(cold_frame && cold_frame->frame);
    CHECK(s1Near3(cold_frame->frame->origin,frame.origin));
    CHECK(s1Near3(cold_frame->frame->normal,
                  {-frame.normal.x,-frame.normal.y,-frame.normal.z}));
    std::cout << "RG02B1_S4A_NATIVE"
              << " exact_semantic_points=3"
              << " independent_provider=1"
              << " source_edge_durable=0"
              << " v18_cold_reopen=1"
              << " reverse_and_undo_redo=1"
              << " typed_invalid_edit_no_mutation=1\\n";
}

// Q1-06: do not classify an actual half-cylinder cut's ambiguous
// three-Surface Point as uniquely selected merely because an Edge touches
// its current native Vertex. Partial Revolve is a distinct real positive
// family, not conflated with the half-cylinder's ambiguous Points.
void verifyQ1RealHalfCylinderAndRevolve(
    kernel_occt::OcctSolidModelingKernel& kernel) {
    const auto cylinder = makeQ1ExtrudeCylinder();
    const auto cylinder_eval = part::evaluatePart(cylinder, kernel);
    CHECK(cylinder_eval.body_status ==
          part::BodyEvaluationStatus::up_to_date);
    CHECK(cylinder_eval.features.size() == 1U);
    CHECK(cylinder_eval.features[0].result_topology);
    const auto& whole = *cylinder_eval.features[0].result_topology;
    CHECK(whole.vertices.size() == 2U);
    for (const auto& vertex : whole.vertices) {
        CHECK(vertex.referenceability ==
              kernel::ReferenceStatus::unsupported);
        CHECK(vertex.point_candidates.empty());
        for (const auto current_edge : vertex.incident_material_edges) {
            const auto rejected = part::authorSelectedMaterialEdgeEnd(
                cylinder, cylinder_eval, whole.stage,
                current_edge, vertex.runtime_token, kernel);
            CHECK(!rejected.ok());
            CHECK(!rejected.reference);
        }
    }

    const auto half_document = withQ1CylinderHalfCut(cylinder);
    const auto half_evaluation = part::evaluatePart(
        half_document, kernel);
    CHECK(half_evaluation.body_status ==
          part::BodyEvaluationStatus::up_to_date);
    CHECK(half_evaluation.features.size() == 2U);
    CHECK(half_evaluation.features.back().result_topology);
    const auto& cat =
        *half_evaluation.features.back().result_topology;
    CHECK(cat.complete());
    CHECK(cat.vertices.size() == 6U);
    std::size_t real_ambiguous = 0U;
    std::size_t real_unsupported = 0U;
    std::size_t ambiguous_terminal_uses = 0U;
    for (const auto& vertex : cat.vertices) {
        if (vertex.referenceability ==
                kernel::ReferenceStatus::ambiguous) {
            ++real_ambiguous;
            CHECK(vertex.point_candidates.size() == 1U);
            const auto& address = vertex.point_candidates.front();
            const auto point = std::find_if(
                cat.points.begin(), cat.points.end(),
                [&address](const auto& item) {
                    return item.address == address;
                });
            CHECK(point != cat.points.end());
            CHECK(point->status ==
                  kernel::ReferenceStatus::ambiguous);
            CHECK(point->current_vertices.size() > 1U);
        } else {
            CHECK(vertex.referenceability ==
                  kernel::ReferenceStatus::unsupported);
            CHECK(vertex.point_candidates.empty());
            ++real_unsupported;
        }
        for (const auto edge : vertex.incident_material_edges) {
            const auto attempt = part::authorSelectedMaterialEdgeEnd(
                half_document, half_evaluation, cat.stage,
                edge, vertex.runtime_token, kernel);
            CHECK(!attempt.ok());
            CHECK(!attempt.reference);
            CHECK(attempt.status !=
                  part::SelectedEdgeEndStatus::resolved);
            CHECK(attempt.status !=
                  part::SelectedEdgeEndStatus::native_geometry_unavailable);
            if (vertex.referenceability ==
                    kernel::ReferenceStatus::ambiguous) {
                ++ambiguous_terminal_uses;
            }
        }
    }
    CHECK(real_ambiguous == 4U);
    CHECK(real_unsupported == 2U);
    CHECK(ambiguous_terminal_uses >= 1U);
    std::size_t ambiguous_point_addresses = 0U;
    for (const auto& point : cat.points) {
        if (point.status ==
                kernel::ReferenceStatus::ambiguous) {
            ++ambiguous_point_addresses;
            CHECK(point.candidate_vertex_count > 1U);
        }
    }
    CHECK(ambiguous_point_addresses == 2U);
    // Q2-M07: MID requires strict Edge+exact Q0 evidence, not a terminal
    // Point. These real ambiguous global Points must remain unchanged.
    std::size_t q2_checked_edges = 0U;
    for (const auto& edge : cat.edges) {
        const auto strict = part::authorMaterialEdgeReference(
            cat, edge.runtime_token);
        const auto candidate = part::authorSelectedMaterialEdgeMid(
            half_document, half_evaluation, cat.stage,
            edge.runtime_token, kernel);
        if (strict.ok()) {
            const auto bound = kernel.bindExactEdgeToBody(
                half_evaluation.features.back().result_solid,
                edge.runtime_token);
            const auto exact = bound
                ? kernel.queryExactBoundedEdge3d(
                    half_evaluation.features.back().result_solid, *bound)
                : kernel::ExactEdge3dResult{};
            if (exact.ok()) {
                CHECK(candidate.ok());
                const auto replay = part::resolveSelectedMaterialEdgeMid(
                    half_document, *candidate.reference,
                    half_evaluation, kernel);
                CHECK(replay.resolved());
            } else {
                CHECK(!candidate.ok());
                CHECK(!candidate.reference);
            }
        } else {
            CHECK(!candidate.ok());
            CHECK(!candidate.reference);
        }
        ++q2_checked_edges;
    }
    CHECK(q2_checked_edges == cat.edges.size());
    for (const auto& point : cat.points) {
        if (point.status == kernel::ReferenceStatus::ambiguous) {
            CHECK(point.candidate_vertex_count > 1U);
        }
    }
    std::cout << "RG02B1_Q1_REAL_HALF_CYLINDER"
              << " ambiguous_vertices=" << real_ambiguous
              << " unsupported_vertices=" << real_unsupported
              << " ambiguous_terminal_uses_denied="
              << ambiguous_terminal_uses
              << " false_resolved=0\n";

    const auto partial = makeQ1PartRevolve(
        core::BuiltinReferenceRole::x_axis,
        {10.0, 0.0}, {20.0, 10.0},
        1.57079632679489661923);
    const auto partial_eval = part::evaluatePart(partial, kernel);
    CHECK(partial_eval.body_status ==
          part::BodyEvaluationStatus::up_to_date);
    CHECK(partial_eval.features.size() == 1U);
    CHECK(partial_eval.features[0].result_topology);
    const auto& revolved =
        *partial_eval.features[0].result_topology;
    CHECK(revolved.complete());
    CHECK(revolved.vertices.size() == 6U);
    for (const auto& vertex : revolved.vertices) {
        CHECK(vertex.referenceability ==
              kernel::ReferenceStatus::resolved);
        CHECK(vertex.point_candidates.size() == 1U);
    }
    std::size_t line_terminal_uses = 0U;
    std::size_t arc_terminal_uses = 0U;
    for (const auto& edge : revolved.edges) {
        for (const auto& vertex : revolved.vertices) {
            if (std::find(
                    vertex.incident_material_edges.begin(),
                    vertex.incident_material_edges.end(),
                    edge.runtime_token) ==
                vertex.incident_material_edges.end()) {
                continue;
            }
            const auto authored = part::authorSelectedMaterialEdgeEnd(
                partial, partial_eval, revolved.stage,
                edge.runtime_token, vertex.runtime_token, kernel);
            if (!authored.ok()) {
                CHECK(authored.status !=
                      part::SelectedEdgeEndStatus::resolved);
                continue;
            }
            const auto resolved = part::resolveSelectedMaterialEdgeEnd(
                partial, *authored.reference, partial_eval, kernel);
            CHECK(resolved.resolved());
            CHECK(resolved.current_edge == edge.runtime_token);
            CHECK(resolved.current_terminal == vertex.runtime_token);
            const auto s1 = part::initializeSelectedEdgeEndFrame(
                partial,*authored.reference,partial_eval,kernel);
            CHECK(s1.resolved());
            CHECK(s1.initialized_intent);
            CHECK(s1.frame->valid());
            const auto s1_replay = part::resolveSelectedEdgeEndFrame(
                partial,*authored.reference,*s1.initialized_intent,
                partial_eval,kernel);
            CHECK(s1_replay.resolved());
            CHECK(s1_replay.frame->valid());
            switch (edge.curve_kind) {
            case kernel::CurveKind::line:
                ++line_terminal_uses;
                break;
            case kernel::CurveKind::circle:
                ++arc_terminal_uses;
                break;
            default:
                CHECK(false);
            }
        }
    }
    CHECK(line_terminal_uses >= 2U);
    CHECK(arc_terminal_uses >= 2U);
    // Q2-M02/M07: actual Part Revolve open circular Arc MIDs use
    // the strict Edge source alone, without choosing an END/Vertex.
    std::size_t q2_arc_mids = 0U;
    std::size_t q2_line_mids = 0U;
    for (const auto& edge : revolved.edges) {
        const auto mid = part::authorSelectedMaterialEdgeMid(
            partial, partial_eval, revolved.stage,
            edge.runtime_token, kernel);
        if (!mid.ok()) {
            CHECK(mid.status != part::SelectedEdgeMidStatus::resolved);
            continue;
        }
        const auto current = part::resolveSelectedMaterialEdgeMid(
            partial, *mid.reference, partial_eval, kernel);
        CHECK(current.resolved());
        CHECK(current.current_edge == edge.runtime_token);
        const auto s1 = part::initializeSelectedEdgeMidFrame(
            partial,*mid.reference,partial_eval,kernel);
        CHECK(s1.resolved());
        CHECK(s1.initialized_intent);
        CHECK(s1.frame->valid());
        const auto s1_replay = part::resolveSelectedEdgeMidFrame(
            partial,*mid.reference,*s1.initialized_intent,
            partial_eval,kernel);
        CHECK(s1_replay.resolved());
        const auto bound = kernel.bindExactEdgeToBody(
            partial_eval.features.front().result_solid,
            edge.runtime_token);
        CHECK(bound);
        const auto exact = kernel.queryExactBoundedEdge3d(
            partial_eval.features.front().result_solid, *bound);
        CHECK(exact.ok());
        CHECK(std::abs(
            current.current_position_world->x -
            exact.geometry->midpoint_at_half_bounded_length_world.x) <
            1.0e-9);
        if (edge.curve_kind == kernel::CurveKind::circle) {
            CHECK(exact.geometry->kind ==
                  kernel::ExactEdge3dKind::open_circular_arc);
            ++q2_arc_mids;
        } else if (edge.curve_kind == kernel::CurveKind::line) {
            ++q2_line_mids;
        } else {
            CHECK(false);
        }
    }
    CHECK(q2_line_mids >= 1U);
    CHECK(q2_arc_mids >= 1U);
    std::cout << "RG02B1_Q2_REAL_REVOLVE_MID"
              << " supported_line_mids=" << q2_line_mids
              << " supported_arc_mids=" << q2_arc_mids
              << " new_global_points=0\n";
    std::cout << "RG02B1_Q1_REAL_PARTIAL_REVOLVE"
              << " certified_line_end_uses=" << line_terminal_uses
              << " certified_arc_end_uses=" << arc_terminal_uses
              << " global_point_promotion=0\n";

    // S1-F02: reverse signed sweep is a real Part + OCCT geometry
    // fixture, not a synthetic perturbation of the tangent.
    const auto reversed_part = makeQ1PartRevolve(
        core::BuiltinReferenceRole::x_axis,
        {10.0, 0.0}, {20.0, 10.0},
        1.57079632679489661923, true);
    const auto reverse_eval = part::evaluatePart(reversed_part,kernel);
    CHECK(reverse_eval.body_status ==
          part::BodyEvaluationStatus::up_to_date);
    CHECK(reverse_eval.features.size() == 1U);
    CHECK(reverse_eval.features.front().result_topology);
    const auto& reverse_catalog =
        *reverse_eval.features.front().result_topology;
    CHECK(reverse_catalog.complete());
    std::size_t reverse_line_frames = 0U;
    std::size_t reverse_arc_frames = 0U;
    std::size_t reverse_arc_ends = 0U;
    for (const auto& edge : reverse_catalog.edges) {
        const auto mid = part::authorSelectedMaterialEdgeMid(
            reversed_part,reverse_eval,reverse_catalog.stage,
            edge.runtime_token,kernel);
        if (!mid.ok()) continue;
        const auto frame = part::initializeSelectedEdgeMidFrame(
            reversed_part,*mid.reference,reverse_eval,kernel);
        CHECK(frame.resolved());
        CHECK(frame.initialized_intent);
        CHECK(frame.frame->valid());
        const auto replay = part::resolveSelectedEdgeMidFrame(
            reversed_part,*mid.reference,*frame.initialized_intent,
            reverse_eval,kernel);
        CHECK(replay.resolved());
        CHECK(replay.frame->valid());
        if (edge.curve_kind == kernel::CurveKind::circle) {
            ++reverse_arc_frames;
        } else if (edge.curve_kind == kernel::CurveKind::line) {
            ++reverse_line_frames;
        } else {
            CHECK(false);
        }
        for (const auto& vertex : reverse_catalog.vertices) {
            if (std::find(
                    vertex.incident_material_edges.begin(),
                    vertex.incident_material_edges.end(),
                    edge.runtime_token) ==
                vertex.incident_material_edges.end()) {
                continue;
            }
            const auto end = part::authorSelectedMaterialEdgeEnd(
                reversed_part,reverse_eval,reverse_catalog.stage,
                edge.runtime_token,vertex.runtime_token,kernel);
            if (!end.ok()) continue;
            const auto at_end = part::initializeSelectedEdgeEndFrame(
                reversed_part,*end.reference,reverse_eval,kernel);
            CHECK(at_end.resolved());
            CHECK(at_end.frame->valid());
            CHECK(at_end.initialized_intent);
            if (edge.curve_kind == kernel::CurveKind::circle)
                ++reverse_arc_ends;
        }
    }
    CHECK(reverse_line_frames >= 1U);
    CHECK(reverse_arc_frames >= 1U);
    CHECK(reverse_arc_ends >= 2U);
    std::cout << "RG02B1_P4_S1_REVOLVED_REVERSE_SWEEP"
              << " line_frames=" << reverse_line_frames
              << " open_arc_frames=" << reverse_arc_frames
              << " arc_ends=" << reverse_arc_ends
              << " viewport_or_authoring_changed=0\n";
}


[[nodiscard]] bool s1Near(double a,double b) {
    return std::abs(a-b) <= 1.0e-8;
}
[[nodiscard]] bool s1Near3(kernel::Point3 a,kernel::Point3 b) {
    return s1Near(a.x,b.x) && s1Near(a.y,b.y) && s1Near(a.z,b.z);
}

void checkS1Frame(const part::SelectedEdgeFrameResult& r) {
    CHECK(r.resolved());
    CHECK(r.status == part::SelectedEdgeFrameStatus::resolved);
    CHECK(r.frame.has_value());
    CHECK(r.frame->valid());
    CHECK(!r.native_error);
    const auto& f = *r.frame;
    const auto dot = [](kernel::Point3 a,kernel::Point3 b) {
        return a.x*b.x + a.y*b.y + a.z*b.z;
    };
    CHECK(s1Near(dot(f.u_axis,f.u_axis),1.0));
    CHECK(s1Near(dot(f.v_axis,f.v_axis),1.0));
    CHECK(s1Near(dot(f.normal,f.normal),1.0));
    CHECK(s1Near(dot(f.u_axis,f.v_axis),0.0));
    CHECK(s1Near(dot(f.u_axis,f.normal),0.0));
    CHECK(s1Near(dot(f.v_axis,f.normal),0.0));
    const kernel::Point3 cross{
        f.u_axis.y*f.v_axis.z-f.u_axis.z*f.v_axis.y,
        f.u_axis.z*f.v_axis.x-f.u_axis.x*f.v_axis.z,
        f.u_axis.x*f.v_axis.y-f.u_axis.y*f.v_axis.x};
    CHECK(s1Near(dot(cross,f.normal),1.0));
}

// Test-only provider changes the current exact unit tangent line, not
// its semantic source, stage, current physical MID or runtime token.
class S1OverrideMidTangentQuery final :
    public kernel::IExactBoundedEdgeGeometryQuery {
public:
    S1OverrideMidTangentQuery(
        kernel::IExactBoundedEdgeGeometryQuery& delegate,
        kernel::Point3 new_representative)
        : delegate_{delegate}, t_{new_representative} {}

    [[nodiscard]] std::optional<kernel::ScopedExactEdge>
    bindExactEdgeToBody(
        kernel::RuntimeSolidHandle body,
        kernel::RuntimeEdgeToken token) noexcept override {
        return delegate_.bindExactEdgeToBody(std::move(body),token);
    }

    [[nodiscard]] kernel::ExactEdge3dResult queryExactBoundedEdge3d(
        kernel::RuntimeSolidHandle body,
        const kernel::ScopedExactEdge& edge) noexcept override {
        auto r = delegate_.queryExactBoundedEdge3d(std::move(body),edge);
        if (r.ok())
            r.geometry->midpoint_tangent_line.representative = t_;
        return r;
    }
private:
    kernel::IExactBoundedEdgeGeometryQuery& delegate_;
    kernel::Point3 t_;
};

void verifyS1SelectedSignedFrames(
    const part::PartDocument& document,
    const part::PartEvaluation& evaluation,
    kernel_occt::OcctSolidModelingKernel& provider,
    const std::vector<part::SelectedMaterialEdgeEnd>& ends,
    const std::vector<part::SelectedMaterialEdgeMid>& mids) {
    CHECK(ends.size() == 24U);
    CHECK(mids.size() == 12U);
    for (const auto& source : ends) {
        const auto initial = part::initializeSelectedEdgeEndFrame(
            document,source,evaluation,provider);
        checkS1Frame(initial);
        CHECK(initial.initialized_intent.has_value());
        const auto replay = part::resolveSelectedEdgeEndFrame(
            document,source,*initial.initialized_intent,evaluation,provider);
        checkS1Frame(replay);
        CHECK(!replay.initialized_intent);
        CHECK(s1Near3(initial.frame->origin,replay.frame->origin));
        CHECK(s1Near3(initial.frame->normal,replay.frame->normal));
        auto reverse = *initial.initialized_intent;
        reverse.reverse = true;
        const auto flipped = part::resolveSelectedEdgeEndFrame(
            document,source,reverse,evaluation,provider);
        checkS1Frame(flipped);
        CHECK(s1Near3(flipped.frame->origin,initial.frame->origin));
        CHECK(s1Near3(flipped.frame->u_axis,initial.frame->u_axis));
        CHECK(s1Near3(flipped.frame->normal,{
            -initial.frame->normal.x,
            -initial.frame->normal.y,
            -initial.frame->normal.z}));
        reverse.reverse = false;
        const auto restored = part::resolveSelectedEdgeEndFrame(
            document,source,reverse,evaluation,provider);
        checkS1Frame(restored);
        CHECK(s1Near3(restored.frame->normal,initial.frame->normal));
    }
    for (const auto& source : mids) {
        const auto initial = part::initializeSelectedEdgeMidFrame(
            document,source,evaluation,provider);
        checkS1Frame(initial);
        CHECK(initial.initialized_intent);
        const auto replay = part::resolveSelectedEdgeMidFrame(
            document,source,*initial.initialized_intent,evaluation,provider);
        checkS1Frame(replay);
        CHECK(!replay.initialized_intent);
        CHECK(s1Near3(replay.frame->normal,initial.frame->normal));
        CHECK(s1Near3(replay.frame->u_axis,initial.frame->u_axis));
    }
    // P4-S2 implementation lane: the authored Datum value preserves the
    // independent semantic source and orientation intent, and evaluates
    // via the certified S1 path (no saved/native Frame).
    {
        auto cursor = document.datumIdCursor();
        const auto first_id = cursor.allocate();
        const auto second_id = cursor.allocate();
        CHECK(first_id && second_id && *first_id != *second_id);

        const auto end_initial = part::initializeSelectedEdgeEndFrame(
            document, ends.front(), evaluation, provider);
        checkS1Frame(end_initial);
        CHECK(end_initial.initialized_intent);
        part::EdgePointDatumPlane end_datum{
            *first_id, ends.front(), *end_initial.initialized_intent, true};
        CHECK(part::edgePointDatumPlaneStructurallyValid(end_datum));
        const auto end_frame = part::resolveEdgePointDatumFrame(
            document, end_datum, evaluation, provider);
        checkS1Frame(end_frame);
        CHECK(s1Near3(end_frame.frame->origin,
                      end_initial.frame->origin));
        CHECK(s1Near3(end_frame.frame->u_axis,
                      end_initial.frame->u_axis));
        CHECK(s1Near3(end_frame.frame->normal,
                      end_initial.frame->normal));

        const auto mid_initial = part::initializeSelectedEdgeMidFrame(
            document, mids.front(), evaluation, provider);
        checkS1Frame(mid_initial);
        CHECK(mid_initial.initialized_intent);
        part::EdgePointDatumPlane mid_datum{
            *second_id, mids.front(), *mid_initial.initialized_intent, false};
        CHECK(part::edgePointDatumPlaneStructurallyValid(mid_datum));
        const auto mid_frame = part::resolveEdgePointDatumFrame(
            document, mid_datum, evaluation, provider);
        checkS1Frame(mid_frame);
        CHECK(s1Near3(mid_frame.frame->origin,
                      mid_initial.frame->origin));
        CHECK(s1Near3(mid_frame.frame->normal,
                      mid_initial.frame->normal));

        // Reverse is an authored choice; it never mutates source or U.
        mid_datum.orientation.reverse = true;
        const auto reversed_frame = part::resolveEdgePointDatumFrame(
            document, mid_datum, evaluation, provider);
        checkS1Frame(reversed_frame);
        CHECK(s1Near3(reversed_frame.frame->origin,
                      mid_frame.frame->origin));
        CHECK(s1Near3(reversed_frame.frame->u_axis,
                      mid_frame.frame->u_axis));
        CHECK(s1Near3(reversed_frame.frame->normal,{
            -mid_frame.frame->normal.x,
            -mid_frame.frame->normal.y,
            -mid_frame.frame->normal.z}));
        mid_datum.orientation.reverse = false;
        const auto restored_frame = part::resolveEdgePointDatumFrame(
            document, mid_datum, evaluation, provider);
        checkS1Frame(restored_frame);
        CHECK(s1Near3(restored_frame.frame->normal,
                      mid_frame.frame->normal));

        auto malformed = mid_datum;
        malformed.orientation.u_seed_world = {};
        CHECK(!part::edgePointDatumPlaneStructurallyValid(malformed));
        const auto rejected = part::resolveEdgePointDatumFrame(
            document, malformed, evaluation, provider);
        CHECK(rejected.status ==
              part::SelectedEdgeFrameStatus::invalid_orientation_intent);
        CHECK(!rejected.frame && !rejected.initialized_intent);
        CHECK(!rejected.native_error);

        part::EdgePointDatumPlane missing_id = end_datum;
        missing_id.id = {};
        CHECK(!part::edgePointDatumPlaneStructurallyValid(missing_id));
        const auto invalid = part::resolveEdgePointDatumFrame(
            document, missing_id, evaluation, provider);
        CHECK(invalid.status ==
              part::SelectedEdgeFrameStatus::invalid_context);
        CHECK(!invalid.frame);
    }

    // Application command is the real authored creation boundary:
    // initialize END/MID from certified Part source, atomically store
    // Reverse and materialize a Sketch using that Datum immediately.
    {
        auto copy = part::PartDocument::restore(
            document.documentId(),document.state(),document.revision());
        CHECK(copy.ok());
        application::DocumentSession session{
            {},std::move(*copy.document)};
        const auto added = session.execute(
            application::CreateEdgePointDatumPlaneCommand{
                part::EdgePointDatumSource{mids.front()},
                session.document().revision(),true,false},
            provider);
        CHECK(added.ok() && added.changed && added.datum_id);
        CHECK(session.document().edgePointDatumPlanes().size() == 1U);
        const auto* created = session.document().findEdgePointDatumPlane(
            *added.datum_id);
        CHECK(created);
        CHECK(created->orientation.reverse);
        CHECK(!created->visible);
        CHECK(std::get_if<part::SelectedMaterialEdgeMid>(
            &created->source) != nullptr);

        const auto observed = part::evaluatePart(
            session.document(),provider);
        CHECK(observed.body_status ==
              part::BodyEvaluationStatus::up_to_date);
        const auto planes = part::evaluateDatums(
            session.document(),observed,provider);
        const auto* plane = planes.find(*added.datum_id);
        CHECK(plane && plane->frame && plane->valid());
        const auto source_support =
            part::partSketchSupportForDatumPlane(*added.datum_id);
        CHECK(source_support);
        const auto hosted = session.execute(
            application::CreatePartSketchOnSupportCommand{
                *source_support,session.document().revision()},
            &provider);
        CHECK(hosted.ok() && hosted.sketch_id);
        CHECK(session.document().findSketch(*hosted.sketch_id));
        CHECK(session.document().findSketch(*hosted.sketch_id)
                  ->support == *source_support);

        // Real downstream Axis command on the newly hosted Sketch must
        // evaluate its 3D line via current Edge-local Datum Frame3.
        const auto axis_line = session.execute(
            application::AddSketchLineCommand{
                *hosted.sketch_id,{0.0,0.0},{12.0,0.0},
                sketch::EntityRole::construction});
        CHECK(axis_line.ok() && axis_line.entity_id);
        const auto axis = session.execute(
            application::CreateAxisCommand{
                part::SketchLineAxisSource{
                    *hosted.sketch_id,*axis_line.entity_id},
                session.document().revision(),
                "Axis on Edge Datum",true},
            provider);
        CHECK(axis.ok() && axis.axis_id);
        CHECK(session.document().findAxis(*axis.axis_id));

        const auto edited = session.execute(
            application::EditEdgePointDatumPlaneCommand{
                *added.datum_id,session.document().revision(),false},
            provider);
        CHECK(edited.ok() && edited.changed);
        CHECK(!session.document()
                  .findEdgePointDatumPlane(*added.datum_id)
                  ->orientation.reverse);
        const auto updated_body = part::evaluatePart(
            session.document(),provider);
        const auto updated_datums = part::evaluateDatums(
            session.document(),updated_body,provider);
        const auto* updated = updated_datums.find(*added.datum_id);
        CHECK(updated && updated->frame);
        CHECK(s1Near3(updated->frame->origin,plane->frame->origin));
        CHECK(s1Near3(updated->frame->u_axis,plane->frame->u_axis));
        CHECK(s1Near3(updated->frame->normal,{
            -plane->frame->normal.x,
            -plane->frame->normal.y,
            -plane->frame->normal.z}));


        // Full semantic Edit: keep the same DatumId and downstream Sketch
        // and Axis while replacing MID with the certified END of an Edge.
        // One atomic undo must restore the prior authored source exactly.
        const auto before_source_replace = session.document().state();
        const auto undo_before_replace = session.undoDepth();
        const auto replaced = session.execute(
            application::ReplaceEdgePointDatumPlaneSourceCommand{
                *added.datum_id,
                part::EdgePointDatumSource{ends.front()},
                session.document().revision(),true},
            provider);
        CHECK(replaced.ok() && replaced.changed);
        CHECK(session.undoDepth() == undo_before_replace + 1U);
        const auto* source_changed =
            session.document().findEdgePointDatumPlane(*added.datum_id);
        CHECK(source_changed);
        CHECK(source_changed->id == *added.datum_id);
        CHECK(source_changed->source ==
              part::EdgePointDatumSource{ends.front()});
        CHECK(source_changed->orientation.reverse);
        CHECK(session.document().findSketch(*hosted.sketch_id));
        CHECK(session.document().findAxis(*axis.axis_id));
        const auto body_after_replace =
            part::evaluatePart(session.document(),provider);
        const auto derived_after_replace =
            part::evaluateDatums(
                session.document(),body_after_replace,provider);
        const auto* frame_after_replace =
            derived_after_replace.find(*added.datum_id);
        CHECK(frame_after_replace && frame_after_replace->frame);
        const auto directly_initialized_end =
            part::initializeSelectedEdgeEndFrame(
                session.document(),ends.front(),
                body_after_replace,provider,true);
        CHECK(directly_initialized_end.resolved());
        CHECK(s1Near3(frame_after_replace->frame->origin,
                      directly_initialized_end.frame->origin));
        CHECK(s1Near3(frame_after_replace->frame->normal,
                      directly_initialized_end.frame->normal));
        const auto after_source_replace = session.document().state();

        const auto undo_before_reject = session.undoDepth();
        const auto rejected_source = session.execute(
            application::ReplaceEdgePointDatumPlaneSourceCommand{
                *added.datum_id,
                part::EdgePointDatumSource{
                    part::SelectedMaterialEdgeMid{}},
                session.document().revision(),false},
            provider);
        CHECK(!rejected_source.ok() && !rejected_source.changed);
        CHECK(session.undoDepth() == undo_before_reject);
        CHECK(session.document().state() == after_source_replace);
        const auto stale_source = session.execute(
            application::ReplaceEdgePointDatumPlaneSourceCommand{
                *added.datum_id,
                part::EdgePointDatumSource{mids.front()},
                document.revision(),false},
            provider);
        CHECK(!stale_source.ok() && !stale_source.changed);
        CHECK(session.document().state() == after_source_replace);

        const auto undone_source = session.undo();
        CHECK(undone_source.ok() && undone_source.changed);
        CHECK(session.document().state() == before_source_replace);
        CHECK(session.document().findEdgePointDatumPlane(*added.datum_id)
                  ->source == part::EdgePointDatumSource{mids.front()});
        const auto redone_source = session.redo();
        CHECK(redone_source.ok() && redone_source.changed);
        CHECK(session.document().state() == after_source_replace);
        CHECK(session.document().findEdgePointDatumPlane(*added.datum_id)
                  ->source == part::EdgePointDatumSource{ends.front()});

        // END -> MID uses the same Datum, reinitializes the signed seed
        // from the newly certified source, and requires only one Undo.
        const auto undo_before_mid = session.undoDepth();
        const auto replaced_with_mid = session.execute(
            application::ReplaceEdgePointDatumPlaneSourceCommand{
                *added.datum_id,
                part::EdgePointDatumSource{mids.front()},
                session.document().revision(),false},
            provider);
        CHECK(replaced_with_mid.ok() && replaced_with_mid.changed);
        CHECK(session.undoDepth() == undo_before_mid + 1U);
        CHECK(session.document().findEdgePointDatumPlane(*added.datum_id)
                  ->source == part::EdgePointDatumSource{mids.front()});
        CHECK(!session.document().findEdgePointDatumPlane(
                   *added.datum_id)->orientation.reverse);
        CHECK(session.document().findSketch(*hosted.sketch_id));
        CHECK(session.document().findAxis(*axis.axis_id));

        const auto shown = session.execute(
            application::SetDatumPlaneVisibilityCommand{
                {*added.datum_id},session.document().revision(),true});
        CHECK(shown.ok() && shown.changed);
        CHECK(session.document()
                  .findEdgePointDatumPlane(*added.datum_id)->visible);

        // A hosted Sketch forbids deleting its Datum source.
        const auto before_delete = session.document().state();
        const auto refused = session.execute(
            application::DeleteDatumPlaneCommand{
                *added.datum_id,session.document().revision()});
        CHECK(!refused.ok() && !refused.changed);
        CHECK(session.document().state() == before_delete);

        const auto source_before = session.document().state();
        const auto stale = session.execute(
            application::CreateEdgePointDatumPlaneCommand{
                part::EdgePointDatumSource{mids.front()},
                document.revision(),false,true},
            provider);
        CHECK(!stale.ok() && !stale.changed && !stale.datum_id);
        CHECK(session.document().state() == source_before);
    }

    // P4-S2 exact Save/Reopen: serialize only source+signed orientation;
    // rebuild a fresh Part/OCCT body and prove equivalent derived frames.
    {
        auto authored = document.state();
        auto cursor = authored.next_datum_id;
        const auto end_id = cursor.allocate();
        const auto mid_id = cursor.allocate();
        CHECK(end_id && mid_id && *end_id != *mid_id);
        authored.next_datum_id = cursor;

        const auto end = part::initializeSelectedEdgeEndFrame(
            document, ends.front(), evaluation, provider);
        const auto mid = part::initializeSelectedEdgeMidFrame(
            document, mids.front(), evaluation, provider, true);
        checkS1Frame(end);
        checkS1Frame(mid);
        CHECK(end.initialized_intent && mid.initialized_intent);
        part::EdgePointDatumPlane end_datum{
            *end_id, ends.front(), *end.initialized_intent, true};
        part::EdgePointDatumPlane mid_datum{
            *mid_id, mids.front(), *mid.initialized_intent, false};
        authored.edge_point_datum_planes.push_back(end_datum);
        authored.edge_point_datum_planes.push_back(mid_datum);

        const auto created = part::PartDocument::restore(
            document.documentId(),authored,document.revision());
        CHECK(created.ok());
        CHECK(created.document->edgePointDatumPlanes().size() == 2U);
        CHECK(created.document->findEdgePointDatumPlane(*end_id));
        CHECK(created.document->findEdgePointDatumPlane(*mid_id));

        // Globally unique DatumIds across legacy Offset and new source kind.
        auto colliding = authored;
        colliding.datum_planes.push_back(
            part::OffsetDatumPlane{
                *end_id,
                part::PlaneReference{
                    part::BuiltinOriginPlaneReference{
                        core::BuiltinReferenceRole::xy_plane}},
                core::LengthValue{1.0},true});
        CHECK(!part::PartDocument::restore(
            document.documentId(),std::move(colliding),
            document.revision()).ok());
        auto invalid_orientation = authored;
        invalid_orientation.edge_point_datum_planes[0]
            .orientation.u_seed_world = {};
        CHECK(!part::PartDocument::restore(
            document.documentId(),std::move(invalid_orientation),
            document.revision()).ok());

        const auto filename = std::string{"ss2-s2-datum-"} +
            std::to_string(
                std::chrono::steady_clock::now()
                    .time_since_epoch().count()) +
            ".ss2part";
        const auto path =
            std::filesystem::temp_directory_path() / filename;
        struct ScopedNativeFile {
            std::filesystem::path path;
            ~ScopedNativeFile() {
                std::error_code ec;
                std::filesystem::remove(path,ec);
            }
        } scratch{path};

        part::PartDocumentStore store;
        const auto saved = store.createNew(
            scratch.path,*created.document);
        CHECK(saved.ok());
        auto loaded = store.load(scratch.path);
        CHECK(loaded.ok());
        CHECK(loaded.document->edgePointDatumPlanes().size() == 2U);
        const auto* reopened_end =
            loaded.document->findEdgePointDatumPlane(*end_id);
        const auto* reopened_mid =
            loaded.document->findEdgePointDatumPlane(*mid_id);
        CHECK(reopened_end && reopened_mid);
        CHECK(*reopened_end == end_datum);
        CHECK(*reopened_mid == mid_datum);
        CHECK(reopened_mid->orientation.reverse);
        CHECK(!reopened_mid->visible);

        // Provider/body identity changes on cold Part load: no runtime
        // tokens or cached Frame3 were authored or written to the file.
        kernel_occt::OcctSolidModelingKernel cold_provider;
        const auto cold_evaluation =
            part::evaluatePart(*loaded.document,cold_provider);
        CHECK(cold_evaluation.body_status ==
              part::BodyEvaluationStatus::up_to_date);
        const auto rebuilt_end = part::resolveEdgePointDatumFrame(
            *loaded.document,*reopened_end,
            cold_evaluation,cold_provider);
        const auto rebuilt_mid = part::resolveEdgePointDatumFrame(
            *loaded.document,*reopened_mid,
            cold_evaluation,cold_provider);
        checkS1Frame(rebuilt_end);
        checkS1Frame(rebuilt_mid);
        CHECK(s1Near3(rebuilt_end.frame->origin,end.frame->origin));
        CHECK(s1Near3(rebuilt_end.frame->u_axis,end.frame->u_axis));
        CHECK(s1Near3(rebuilt_end.frame->normal,end.frame->normal));
        CHECK(s1Near3(rebuilt_mid.frame->origin,mid.frame->origin));
        CHECK(s1Near3(rebuilt_mid.frame->normal,mid.frame->normal));
        CHECK(s1Near3(rebuilt_mid.frame->u_axis,mid.frame->u_axis));

        const auto derived = part::evaluateDatums(
            *loaded.document,cold_evaluation,cold_provider);
        CHECK(derived.valid());
        const auto* evaluated_end = derived.find(*end_id);
        const auto* evaluated_mid = derived.find(*mid_id);
        CHECK(evaluated_end && evaluated_mid);
        CHECK(evaluated_end->status ==
              part::DatumPlaneEvaluationStatus::resolved);
        CHECK(evaluated_mid->status ==
              part::DatumPlaneEvaluationStatus::resolved);
        CHECK(evaluated_end->required_body_stage ==
              ends.front().source_edge.stage);
        CHECK(evaluated_mid->required_body_stage ==
              mids.front().source_edge.stage);
        CHECK(evaluated_end->frame && evaluated_mid->frame);
        CHECK(s1Near3(evaluated_end->frame->normal,
                      rebuilt_end.frame->normal));
        CHECK(s1Near3(evaluated_mid->frame->normal,
                      rebuilt_mid.frame->normal));

        // Real application Sketch creation and a second Save/Reopen
        // consume the authored Edge-local MID Datum, not a cached plane.
        const auto support =
            part::partSketchSupportForDatumPlane(*mid_id);
        CHECK(support.has_value());
        const auto current_support = part::resolveSketchSupport(
            *support,nullptr,&derived);
        CHECK(current_support.status ==
              part::SketchSupportResolutionStatus::resolved);
        CHECK(current_support.frame.has_value());

        application::DocumentSession sketch_session{
            {},std::move(*loaded.document)};
        const auto sketch_created = sketch_session.execute(
            application::CreatePartSketchOnSupportCommand{
                *support,sketch_session.document().revision()},
            &cold_provider);
        CHECK(sketch_created.ok() && sketch_created.sketch_id);
        const auto* hosted = sketch_session.document().findSketch(
            *sketch_created.sketch_id);
        CHECK(hosted && hosted->support == *support);

        const auto again = store.save(
            scratch.path,sketch_session.document(),
            *loaded.checkpoint);
        CHECK(again.ok());
        const auto reopened_with_sketch = store.load(scratch.path);
        CHECK(reopened_with_sketch.ok());
        const auto* restored_sketch =
            reopened_with_sketch.document->findSketch(
                *sketch_created.sketch_id);
        CHECK(restored_sketch);
        CHECK(restored_sketch->support == *support);
        kernel_occt::OcctSolidModelingKernel sketch_provider;
        const auto current_body = part::evaluatePart(
            *reopened_with_sketch.document,sketch_provider);
        CHECK(current_body.body_status ==
              part::BodyEvaluationStatus::up_to_date);
        const auto current_datums = part::evaluateDatums(
            *reopened_with_sketch.document,current_body,sketch_provider);
        const auto current_frame = part::resolveSketchSupport(
            restored_sketch->support,nullptr,&current_datums);
        CHECK(current_frame.status ==
              part::SketchSupportResolutionStatus::resolved);
        CHECK(current_frame.frame.has_value());
        CHECK(s1Near(
            current_frame.frame->origin[0],
            rebuilt_mid.frame->origin.x));
        CHECK(s1Near(
            current_frame.frame->origin[1],
            rebuilt_mid.frame->origin.y));
        CHECK(s1Near(
            current_frame.frame->origin[2],
            rebuilt_mid.frame->origin.z));

        // A feature's source Sketch may not depend on its own Edge:
        // that would create an authored current/future Body-stage cycle.
        auto cyclic = reopened_with_sketch.document->state();
        CHECK(!cyclic.sketches.empty());
        cyclic.sketches.front().support = *support;
        CHECK(!part::PartDocument::restore(
            document.documentId(),std::move(cyclic),
            document.revision()).ok());

        std::cout << "RG02B1_P4_S2_DATUM_V17"
                  << " saved=2 restored=2"
                  << " cold_rebuild=1 reverse_persisted=1"
                  << " collisions_rejected=1\n";
    }

    const auto& sample = mids.front();
    const auto initial = part::initializeSelectedEdgeMidFrame(
        document,sample,evaluation,provider);
    checkS1Frame(initial);
    const auto& intent = *initial.initialized_intent;
    const auto& n = intent.signed_normal_reference_world;
    const auto& u = intent.u_seed_world;
    const auto old = part::resolveSelectedMaterialEdgeMid(
        document,sample,evaluation,provider);
    CHECK(old.resolved());

    // Q0 ±analytic tangent representative is not an authored sign.
    const auto& current = old.current_tangent_line->representative;
    S1OverrideMidTangentQuery negated{provider,{
        -current.x,-current.y,-current.z}};
    const auto same = part::initializeSelectedEdgeMidFrame(
        document,sample,evaluation,negated);
    checkS1Frame(same);
    CHECK(s1Near3(same.frame->normal,initial.frame->normal));

    // S1-F05: real Part-certified MID with test-controlled unit
    // tangent ±representatives and deterministic dominant-axis ties.
    {
        const double half_diagonal = std::sqrt(0.5);
        S1OverrideMidTangentQuery tie{provider,
            {half_diagonal,half_diagonal,0.0}};
        S1OverrideMidTangentQuery opposite{provider,
            {-half_diagonal,-half_diagonal,0.0}};
        const auto a = part::initializeSelectedEdgeMidFrame(
            document,sample,evaluation,tie);
        const auto b = part::initializeSelectedEdgeMidFrame(
            document,sample,evaluation,opposite);
        checkS1Frame(a);
        checkS1Frame(b);
        CHECK(s1Near3(a.frame->normal,b.frame->normal));
        CHECK(s1Near3(a.frame->u_axis,b.frame->u_axis));
        CHECK(a.frame->normal.x > 0.0);
        CHECK(a.frame->normal.y > 0.0);
    }
    // +X first-seed is parallel to an X normal: fallback must use +Y,
    // independent of the provider's chosen unoriented sign.
    {
        S1OverrideMidTangentQuery x{provider,{1.0,0.0,0.0}};
        S1OverrideMidTangentQuery minus_x{provider,{-1.0,0.0,0.0}};
        const auto a = part::initializeSelectedEdgeMidFrame(
            document,sample,evaluation,x);
        const auto b = part::initializeSelectedEdgeMidFrame(
            document,sample,evaluation,minus_x);
        checkS1Frame(a);
        checkS1Frame(b);
        CHECK(s1Near3(a.frame->normal,{1.0,0.0,0.0}));
        CHECK(s1Near3(b.frame->normal,a.frame->normal));
        CHECK(s1Near3(a.frame->u_axis,{0.0,1.0,0.0}));
    }
    // Floating-point neighborhoods of the 1/16 projected seed
    // threshold: above accepts +X, below must select next +Y.
    // Exactly 1/16 also resolves deterministically, never silently
    // substitutes a camera or OCCT-derived seed.
    for (const double y : {0.249999,0.25,0.250001}) {
        const double x = std::sqrt(1.0-y*y);
        S1OverrideMidTangentQuery decorated{provider,{x,y,0.0}};
        const auto result = part::initializeSelectedEdgeMidFrame(
            document,sample,evaluation,decorated);
        checkS1Frame(result);
        CHECK(result.initialized_intent);
        if (y < 0.25) CHECK(result.frame->u_axis.x < 0.0);
        if (y > 0.25) CHECK(result.frame->u_axis.x > 0.0);
    }

    // Controlled real-Body source with exact test-only orientation
    // perturbation: 29 degrees accepted; 31/89/90 degrees reviewed.
    const auto angleProbe = [&](double degrees) {
        const double radians = degrees*std::acos(-1.0)/180.0;
        const double a = std::cos(radians),b = std::sin(radians);
        S1OverrideMidTangentQuery decorated{provider,{
            a*n.x+b*u.x,a*n.y+b*u.y,a*n.z+b*u.z}};
        return part::resolveSelectedEdgeMidFrame(
            document,sample,intent,evaluation,decorated);
    };
    checkS1Frame(angleProbe(29.0));
    // S1-F06 exact 30 degree boundary: compare against the exact
    // contracted sqrt(3)/2 threshold, not a rounded degree label.
    {
        const double c = std::sqrt(3.0)/2.0;
        S1OverrideMidTangentQuery boundary{provider,{
            c*n.x+0.5*u.x,c*n.y+0.5*u.y,c*n.z+0.5*u.z}};
        const auto admitted = part::resolveSelectedEdgeMidFrame(
            document,sample,intent,evaluation,boundary);
        checkS1Frame(admitted);
        CHECK(!admitted.initialized_intent);
    }
    for (const double angle : {31.0,89.0,90.0,91.0}) {
        const auto review = angleProbe(angle);
        CHECK(review.status ==
              part::SelectedEdgeFrameStatus::orientation_review_required);
        CHECK(!review.frame);
        CHECK(!review.initialized_intent);
    }
    const auto badIntent = [&](part::SelectedEdgeFrameIntent bad) {
        const auto rejected = part::resolveSelectedEdgeMidFrame(
            document,sample,bad,evaluation,provider);
        CHECK(rejected.status ==
              part::SelectedEdgeFrameStatus::invalid_orientation_intent);
        CHECK(!rejected.frame);
        CHECK(!rejected.initialized_intent);
    };
    auto malformed = intent;
    malformed.u_seed_world = {0.0,0.0,0.0};
    badIntent(malformed);
    malformed = intent;
    malformed.signed_normal_reference_world.x =
        std::numeric_limits<double>::quiet_NaN();
    badIntent(malformed);
    malformed = intent;
    malformed.u_seed_world = malformed.signed_normal_reference_world;
    badIntent(malformed);
    malformed = intent;
    malformed.u_seed_world = {
        intent.u_seed_world.x*1.001,
        intent.u_seed_world.y*1.001,
        intent.u_seed_world.z*1.001};
    badIntent(malformed);
    malformed = intent;
    malformed.signed_normal_reference_world.y =
        std::numeric_limits<double>::infinity();
    badIntent(malformed);

    auto reversed = intent;
    reversed.reverse = true;
    const auto flipped = part::resolveSelectedEdgeMidFrame(
        document,sample,reversed,evaluation,provider);
    checkS1Frame(flipped);
    CHECK(s1Near3(flipped.frame->u_axis,initial.frame->u_axis));
    CHECK(s1Near3(flipped.frame->origin,initial.frame->origin));
    CHECK(s1Near3(flipped.frame->v_axis,{
        -initial.frame->v_axis.x,
        -initial.frame->v_axis.y,
        -initial.frame->v_axis.z}));
    reversed.reverse = false;
    const auto restored = part::resolveSelectedEdgeMidFrame(
        document,sample,reversed,evaluation,provider);
    checkS1Frame(restored);
    CHECK(s1Near3(restored.frame->normal,initial.frame->normal));

    // The S1 status must not manufacture a frame on wrong owner or
    // provider failure. Native error metadata is passed through only
    // from a genuinely failing Q0 geometry query.
    const auto foreign = makeBasePart();
    const auto invalid = part::resolveSelectedEdgeMidFrame(
        foreign,sample,intent,evaluation,provider);
    CHECK(invalid.status ==
          part::SelectedEdgeFrameStatus::invalid_context);
    CHECK(!invalid.resolved() && !invalid.frame && !invalid.native_error);
    Q2BadMidQuery failing{provider,Q2BadMidQuery::Failure::kernel_failure};
    const auto failed = part::resolveSelectedEdgeMidFrame(
        document,sample,intent,evaluation,failing);
    CHECK(failed.status ==
          part::SelectedEdgeFrameStatus::native_geometry_unavailable);
    CHECK(!failed.frame && !failed.initialized_intent);
    CHECK(failed.native_error == kernel::ExactEdge3dStatus::kernel_failure);

    std::cout << "RG02B1_P4_S1_SIGNED_FRAME"
              << " real_box_end=" << ends.size()
              << " real_box_mid=" << mids.size()
              << " source_derived=1"
              << " reverse_involution=1"
              << " orientation_review=1"
              << " ui_datum_persistence=0\n";
}


} // namespace

int main() {
    kernel_occt::OcctSolidModelingKernel kernel;

    // Q1-06: independent real Part/OCCT negative and Revolve witnesses.
    verifyQ1RealHalfCylinderAndRevolve(kernel);

    auto base_document =
        makeBasePart();
    const auto base_evaluation =
        part::evaluatePart(
            base_document,
            kernel);
    CHECK(
        base_evaluation.body_status ==
        part::BodyEvaluationStatus::up_to_date);
    CHECK(base_evaluation.features.size() == 1U);
    CHECK(
        base_evaluation.features[0].status ==
        part::FeatureEvaluationStatus::up_to_date);
    CHECK(
        base_evaluation.features[0]
            .result_topology);
    const auto g0_base = characterizeNativeVertices(
        *base_evaluation.features[0].result_topology,
        "V01-extrude");
    CHECK(g0_base.native == 8U);
    CHECK(g0_base.resolved == 8U);
    CHECK(g0_base.point_witnesses == 8U);
    CHECK(g0_base.ambiguous == 0U);
    CHECK(g0_base.unsupported == 0U);
    const auto e0_base_ends =
        characterizeE0CanonicalStraightEnds(
            *base_evaluation.features[0].result_topology,
            "E0-01-pristine-extrude");
    const auto q0_base_lengths = verifyQ0StrictPartStageGeometry(
        base_evaluation.features[0], kernel, "Q0-extrude");
    const auto q1_base_sources = verifyQ1CanonicalSelectedEnds(
        base_document, base_evaluation, kernel, "Q1-01-pristine");
    CHECK(q1_base_sources.size() == 24U);
    const auto s4p0_base_keys = verifyS4P0NativeEndPointIdentity(
        base_document, base_evaluation, kernel,
        q1_base_sources, "P0-01-pristine");
    verifyS4AThreePointDatumLifecycle(
        base_document,base_evaluation,kernel,s4p0_base_keys);
    const auto q2_base_sources = verifyQ2CanonicalSelectedMids(
        base_document, base_evaluation, kernel, "Q2-M01-pristine");
    CHECK(q2_base_sources.size() == 12U);
    verifyS1SelectedSignedFrames(base_document,base_evaluation,kernel,
                                 q1_base_sources,q2_base_sources);
    // Q2-M05/06/07: negative semantic and native provider matrix on a
    // REAL OCCT box stage. No failure publishes a partial observation.
    {
        const auto& source = q2_base_sources.front();
        const auto original = part::resolveSelectedMaterialEdgeMid(
            base_document, source, base_evaluation, kernel);
        CHECK(original.resolved());
        const auto token = *original.current_edge;
        const auto noObservation = [](
            const part::SelectedEdgeMidResolution& value,
            part::SelectedEdgeMidStatus expected) {
            CHECK(value.status == expected);
            CHECK(!value.resolved());
            CHECK(!value.current_edge);
            CHECK(!value.current_position_world);
            CHECK(!value.current_tangent_line);
        };
        const auto foreign_owner = makeBasePart();
        noObservation(part::resolveSelectedMaterialEdgeMid(
            foreign_owner, source, base_evaluation, kernel),
            part::SelectedEdgeMidStatus::invalid_context);
        auto unowned = base_evaluation;
        unowned.source_document_id.reset();
        noObservation(part::resolveSelectedMaterialEdgeMid(
            base_document, source, unowned, kernel),
            part::SelectedEdgeMidStatus::invalid_context);
        const auto stale_document = part::PartDocument::restore(
            base_document.documentId(),
            base_document.state(),
            core::DocumentRevision{
                base_document.revision().value() + 1U});
        CHECK(stale_document.ok());
        noObservation(part::resolveSelectedMaterialEdgeMid(
            *stale_document.document, source, base_evaluation, kernel),
            part::SelectedEdgeMidStatus::invalid_context);
        auto wrong_stage = source;
        wrong_stage.source_edge.stage.kind =
            part::BodyStageKind::empty_body;
        noObservation(part::resolveSelectedMaterialEdgeMid(
            base_document, wrong_stage, base_evaluation, kernel),
            part::SelectedEdgeMidStatus::invalid_context);
        const auto& catalog = *base_evaluation.features.front().result_topology;
        const auto wrong_author = part::authorSelectedMaterialEdgeMid(
            base_document, base_evaluation, wrong_stage.source_edge.stage,
            token, kernel);
        CHECK(!wrong_author.ok());
        CHECK(wrong_author.status ==
              part::SelectedEdgeMidStatus::invalid_context);
        CHECK(!wrong_author.reference);
        const auto absent_author = part::authorSelectedMaterialEdgeMid(
            base_document, base_evaluation, catalog.stage,
            kernel::RuntimeEdgeToken{999999999U}, kernel);
        CHECK(!absent_author.ok());
        CHECK(absent_author.status ==
              part::SelectedEdgeMidStatus::edge_missing);
        CHECK(!absent_author.reference);

        const auto editCatalog = [&](
            auto mutate,
            part::SelectedEdgeMidStatus expected) {
            auto synthetic = base_evaluation;
            auto& cat = *synthetic.features.front().result_topology;
            mutate(cat);
            CHECK(cat.complete());
            noObservation(part::resolveSelectedMaterialEdgeMid(
                base_document, source, synthetic, kernel), expected);
        };
        const auto sourceCurve = [&source](
            part::BodyStageTopologyCatalog& cat) {
            const auto c = std::find_if(
                cat.curves.begin(), cat.curves.end(),
                [&source](const auto& entry) {
                    return entry.address == source.source_edge.curve;
                });
            CHECK(c != cat.curves.end());
            return c;
        };
        editCatalog([&](auto& cat) {
            const auto c = sourceCurve(cat);
            c->status = kernel::ReferenceStatus::missing;
            c->strict_edge_status = kernel::ReferenceStatus::missing;
            c->current_edges.clear();
            c->candidate_edge_count = 0U;
        }, part::SelectedEdgeMidStatus::edge_missing);
        editCatalog([&](auto& cat) {
            const auto c = sourceCurve(cat);
            const auto other = std::find_if(
                cat.edges.begin(), cat.edges.end(),
                [token](const auto& e) {
                    return e.runtime_token != token;
                });
            CHECK(other != cat.edges.end());
            c->current_edges.push_back(other->runtime_token);
            c->candidate_edge_count = c->current_edges.size();
            c->strict_edge_status = kernel::ReferenceStatus::ambiguous;
        }, part::SelectedEdgeMidStatus::edge_ambiguous);
        editCatalog([&](auto& cat) {
            const auto e = std::find_if(
                cat.edges.begin(), cat.edges.end(),
                [token](const auto& item) {
                    return item.runtime_token == token;
                });
            CHECK(e != cat.edges.end());
            e->accounting_class =
                part::TopologyAccountingClass::semantically_unsupported;
            e->referenceability = kernel::ReferenceStatus::unsupported;
            e->curve_kind = kernel::CurveKind::other;
            e->curve_candidates.clear();
        }, part::SelectedEdgeMidStatus::edge_unsupported);
        // Even a globally Ambiguous Point is NOT a MID identity or a
        // request to promote that Point; only strict Edge provenance
        // and Q0 geometry determine MID source admissibility.
        {
            auto synthetic = base_evaluation;
            auto& cat = *synthetic.features.front().result_topology;
            CHECK(!cat.points.empty());
            auto& point = cat.points.front();
            CHECK(point.current_vertices.size() == 1U);
            const auto different = std::find_if(
                cat.vertices.begin(), cat.vertices.end(),
                [&point](const auto& vertex) {
                    return vertex.runtime_token !=
                        point.current_vertices.front();
                });
            CHECK(different != cat.vertices.end());
            point.status = kernel::ReferenceStatus::ambiguous;
            point.current_vertices.push_back(different->runtime_token);
            point.candidate_vertex_count = 2U;
            CHECK(cat.complete());
            const auto r = part::resolveSelectedMaterialEdgeMid(
                base_document, source, synthetic, kernel);
            CHECK(r.resolved());
            CHECK(cat.points.front().status ==
                  kernel::ReferenceStatus::ambiguous);
        }
        Q2BadMidQuery badKernel{kernel,
            Q2BadMidQuery::Failure::kernel_failure};
        Q2BadMidQuery badPosition{kernel,
            Q2BadMidQuery::Failure::nonfinite_mid};
        Q2BadMidQuery badTangent{kernel,
            Q2BadMidQuery::Failure::zero_tangent};
        Q2BadMidQuery badLength{kernel,
            Q2BadMidQuery::Failure::zero_length};
        for (auto* provider : std::array<
             kernel::IExactBoundedEdgeGeometryQuery*, 4>{
                &badKernel, &badPosition, &badTangent, &badLength}) {
            const auto rejected = part::resolveSelectedMaterialEdgeMid(
                base_document, source, base_evaluation, *provider);
            noObservation(
                rejected,
                part::SelectedEdgeMidStatus::native_geometry_unavailable);
            const auto failed_author = part::authorSelectedMaterialEdgeMid(
                base_document, base_evaluation, catalog.stage,
                token, *provider);
            CHECK(!failed_author.ok());
            CHECK(!failed_author.reference);
            CHECK(failed_author.status ==
                  part::SelectedEdgeMidStatus::native_geometry_unavailable);
        }
        const auto typed = part::resolveSelectedMaterialEdgeMid(
            base_document, source, base_evaluation, badKernel);
        CHECK(typed.native_error ==
              kernel::ExactEdge3dStatus::kernel_failure);
        kernel_occt::OcctSolidModelingKernel second_instance;
        const auto legal = part::resolveSelectedMaterialEdgeMid(
            base_document, source, base_evaluation, second_instance);
        CHECK(legal.resolved());
        const auto legal_author = part::authorSelectedMaterialEdgeMid(
            base_document, base_evaluation, catalog.stage,
            token, second_instance);
        CHECK(legal_author.ok());
        CHECK(*legal_author.reference == source);

        const auto unrelated = makeBasePart();
        const auto unrelated_eval = part::evaluatePart(
            unrelated, second_instance);
        CHECK(unrelated_eval.body_status ==
              part::BodyEvaluationStatus::up_to_date);
        CHECK(unrelated_eval.features.front().result_solid);
        CHECK(unrelated_eval.features.front().result_topology);
        CHECK(unrelated_eval.features.front().result_solid.get() !=
              base_evaluation.features.front().result_solid.get());
        Q1ForeignBodyBinderQuery foreign_body{
            second_instance,
            unrelated_eval.features.front().result_solid,
            unrelated_eval.features.front()
                .result_topology->edges.front().runtime_token};
        noObservation(part::resolveSelectedMaterialEdgeMid(
            base_document, source, base_evaluation, foreign_body),
            part::SelectedEdgeMidStatus::native_geometry_unavailable);
        const auto wrong_body_author = part::authorSelectedMaterialEdgeMid(
            base_document, base_evaluation, catalog.stage,
            token, foreign_body);
        CHECK(!wrong_body_author.ok());
        CHECK(!wrong_body_author.reference);
        CHECK(wrong_body_author.status ==
              part::SelectedEdgeMidStatus::native_geometry_unavailable);
    }
    // Q1-07: full tuple reorder is harmless; a forged membership token
    // causes typed unavailable with no partial END/geometry publication.
    {
        Q1TerminalTupleQuery permuted{kernel, false};
        Q1TerminalTupleQuery corrupted{kernel, true};
        Q1TerminalTupleQuery duplicated{kernel, false, true};
        for (const auto& source : q1_base_sources) {
            const auto original =
                part::resolveSelectedMaterialEdgeEnd(
                    base_document, source, base_evaluation, kernel);
            const auto reordered =
                part::resolveSelectedMaterialEdgeEnd(
                    base_document, source, base_evaluation, permuted);
            CHECK(original.resolved());
            CHECK(reordered.resolved());
            CHECK(reordered.current_terminal ==
                  original.current_terminal);
            CHECK(reordered.current_position_world->x ==
                  original.current_position_world->x);
            CHECK(reordered.current_position_world->y ==
                  original.current_position_world->y);
            CHECK(reordered.current_position_world->z ==
                  original.current_position_world->z);
            const auto forged =
                part::resolveSelectedMaterialEdgeEnd(
                    base_document, source, base_evaluation, corrupted);
            CHECK(forged.status ==
                  part::SelectedEdgeEndStatus::native_geometry_unavailable);
            CHECK(!forged.resolved());
            CHECK(!forged.current_edge);
            CHECK(!forged.current_terminal);
            CHECK(!forged.current_position_world);
            CHECK(!forged.current_tangent_line);
            const auto repeated = part::resolveSelectedMaterialEdgeEnd(
                base_document, source, base_evaluation, duplicated);
            CHECK(repeated.status ==
                  part::SelectedEdgeEndStatus::native_geometry_unavailable);
            CHECK(!repeated.resolved());
            CHECK(!repeated.current_edge);
            CHECK(!repeated.current_terminal);
            CHECK(!repeated.current_position_world);
            CHECK(!repeated.current_tangent_line);
        }
    }
    // Q1-06 synthetic mutation of a REAL native current-stage catalog:
    // global Point status is independently Ambiguous/Unsupported/Missing.
    // Exact Edge incidence and same-Body native geometry are unchanged;
    // no selected material Edge may promote that Point to Resolved.
    {
        const auto& source = q1_base_sources.front();
        const auto current = part::resolveSelectedMaterialEdgeEnd(
            base_document, source, base_evaluation, kernel);
        CHECK(current.resolved());
        const auto native_vertex = *current.current_terminal;
        const auto native_edge = *current.current_edge;
        const auto replayWithPointStatus = [&](
            kernel::ReferenceStatus point_status,
            part::SelectedEdgeEndStatus expected) {
            auto synthetic = base_evaluation;
            CHECK(synthetic.features.front().result_topology);
            auto& cat =
                *synthetic.features.front().result_topology;
            auto p = std::find_if(
                cat.points.begin(), cat.points.end(),
                [&source](const auto& item) {
                    return item.address ==
                        source.terminal.certified_semantic_point;
                });
            CHECK(p != cat.points.end());
            auto v = std::find_if(
                cat.vertices.begin(), cat.vertices.end(),
                [native_vertex](const auto& item) {
                    return item.runtime_token == native_vertex;
                });
            CHECK(v != cat.vertices.end());
            p->status = point_status;
            if (point_status ==
                    kernel::ReferenceStatus::ambiguous) {
                const auto different = std::find_if(
                    cat.vertices.begin(), cat.vertices.end(),
                    [native_vertex](const auto& item) {
                        return item.runtime_token != native_vertex;
                    });
                CHECK(different != cat.vertices.end());
                p->current_vertices.push_back(
                    different->runtime_token);
                p->candidate_vertex_count = 2U;
                v->referenceability =
                    kernel::ReferenceStatus::ambiguous;
            } else {
                p->current_vertices.clear();
                p->candidate_vertex_count = 0U;
                if (point_status ==
                        kernel::ReferenceStatus::unsupported) {
                    v->accounting_class =
                        part::TopologyAccountingClass::
                            semantically_unsupported;
                    v->referenceability =
                        kernel::ReferenceStatus::unsupported;
                    v->point_candidates.clear();
                }
            }
            CHECK(cat.complete());
            const auto absent = part::resolveSelectedMaterialEdgeEnd(
                base_document, source, synthetic, kernel);
            CHECK(absent.status == expected);
            CHECK(!absent.resolved());
            CHECK(!absent.current_edge);
            CHECK(!absent.current_terminal);
            CHECK(!absent.current_position_world);
            CHECK(!absent.current_tangent_line);
            const auto not_authored =
                part::authorSelectedMaterialEdgeEnd(
                    base_document, synthetic, cat.stage,
                    native_edge, native_vertex, kernel);
            CHECK(!not_authored.ok());
            CHECK(!not_authored.reference);
            CHECK(not_authored.status == expected);
        };
        replayWithPointStatus(
            kernel::ReferenceStatus::ambiguous,
            part::SelectedEdgeEndStatus::terminal_ambiguous);
        replayWithPointStatus(
            kernel::ReferenceStatus::unsupported,
            part::SelectedEdgeEndStatus::terminal_unsupported);
        replayWithPointStatus(
            kernel::ReferenceStatus::missing,
            part::SelectedEdgeEndStatus::terminal_missing);
    }
    // Q1-05: cross-document ownership and revision staleness cannot be
    // masked by equal source Feature IDs or coincident runtime geometry.
    {
        const auto other_document = makeBasePart();
        const auto foreign = part::resolveSelectedMaterialEdgeEnd(
            other_document, q1_base_sources.front(),
            base_evaluation, kernel);
        CHECK(foreign.status ==
              part::SelectedEdgeEndStatus::invalid_context);
        CHECK(!foreign.resolved());
        CHECK(!foreign.current_edge);
        CHECK(!foreign.current_position_world);
        const auto revised = part::PartDocument::restore(
            base_document.documentId(),
            base_document.state(),
            core::DocumentRevision{
                base_document.revision().value() + 1U});
        CHECK(revised.ok());
        const auto stale_owner = part::resolveSelectedMaterialEdgeEnd(
            *revised.document, q1_base_sources.front(),
            base_evaluation, kernel);
        CHECK(stale_owner.status ==
              part::SelectedEdgeEndStatus::invalid_context);
        CHECK(!stale_owner.current_terminal);
    }
    {
        const auto& cat = *base_evaluation.features[0].result_topology;
        const auto current_edge = cat.edges.front().runtime_token;
        const auto not_incident = std::find_if(
            cat.vertices.begin(), cat.vertices.end(),
            [current_edge](const auto& v) {
                return std::find(
                    v.incident_material_edges.begin(),
                    v.incident_material_edges.end(),
                    current_edge) == v.incident_material_edges.end();
            });
        CHECK(not_incident != cat.vertices.end());
        const auto wrong = part::authorSelectedMaterialEdgeEnd(
            base_document, base_evaluation, cat.stage,
            current_edge, not_incident->runtime_token, kernel);
        CHECK(!wrong.ok());
        CHECK(wrong.status ==
              part::SelectedEdgeEndStatus::terminal_missing);
    }
    // Q1-05: typed failure precedence for a still-live document but
    // semantically changed, otherwise complete *current* stage inventory.
    // Distinguish a vanished source, extra bounded Curve realization,
    // inadmissible material Edge, lost certified terminal and provider
    // mismatch; none may fall back to provider XYZ or a different Edge.
    {
        const auto& source = q1_base_sources.front();
        CHECK(std::holds_alternative<part::SingularAtAuthoredStage>(
            source.source_edge.branch));
        const auto native = part::resolveSelectedMaterialEdgeEnd(
            base_document, source, base_evaluation, kernel);
        CHECK(native.resolved());
        const auto current_edge = *native.current_edge;
        const auto current_vertex = *native.current_terminal;
        const auto assertNoPayload = [](
            const part::SelectedEdgeEndResolution& result,
            part::SelectedEdgeEndStatus expected) {
            if (result.status != expected) {
                std::cerr << "RG02B1_Q1_05_STATUS_DIAGNOSTIC"
                          << " expected=" << static_cast<int>(expected)
                          << " actual=" << static_cast<int>(result.status)
                          << " geometry_present="
                          << result.current_position_world.has_value()
                          << "\\n";
            }
            CHECK(result.status == expected);
            CHECK(!result.resolved());
            CHECK(!result.current_edge);
            CHECK(!result.current_terminal);
            CHECK(!result.current_position_world);
            CHECK(!result.current_tangent_line);
        };
        const auto withCatalogEdit = [&](
            auto edit,
            part::SelectedEdgeEndStatus expected) {
            auto changed = base_evaluation;
            CHECK(changed.features.front().result_topology);
            auto& catalog = *changed.features.front().result_topology;
            edit(catalog);
            CHECK(catalog.complete());
            assertNoPayload(part::resolveSelectedMaterialEdgeEnd(
                base_document, source, changed, kernel), expected);
        };
        const auto findSourceCurve = [&source](
            part::BodyStageTopologyCatalog& cat) {
            const auto c = std::find_if(
                cat.curves.begin(), cat.curves.end(),
                [&source](const auto& item) {
                    return item.address == source.source_edge.curve;
                });
            CHECK(c != cat.curves.end());
            return c;
        };
        withCatalogEdit([&](auto& cat) {
            auto c = findSourceCurve(cat);
            c->status = kernel::ReferenceStatus::missing;
            c->strict_edge_status = kernel::ReferenceStatus::missing;
            c->current_edges.clear();
            c->candidate_edge_count = 0U;
        }, part::SelectedEdgeEndStatus::edge_missing);
        withCatalogEdit([&](auto& cat) {
            auto c = findSourceCurve(cat);
            const auto other = std::find_if(
                cat.edges.begin(), cat.edges.end(),
                [current_edge](const auto& item) {
                    return item.runtime_token != current_edge;
                });
            CHECK(other != cat.edges.end());
            c->current_edges.push_back(other->runtime_token);
            c->candidate_edge_count = c->current_edges.size();
            c->strict_edge_status = kernel::ReferenceStatus::ambiguous;
        }, part::SelectedEdgeEndStatus::edge_ambiguous);
        withCatalogEdit([&](auto& cat) {
            const auto e = std::find_if(
                cat.edges.begin(), cat.edges.end(),
                [current_edge](const auto& item) {
                    return item.runtime_token == current_edge;
                });
            CHECK(e != cat.edges.end());
            e->accounting_class =
                part::TopologyAccountingClass::semantically_unsupported;
            e->referenceability = kernel::ReferenceStatus::unsupported;
            e->curve_kind = kernel::CurveKind::other;
            e->curve_candidates.clear();
        }, part::SelectedEdgeEndStatus::edge_unsupported);
        withCatalogEdit([&](auto& cat) {
            const auto p = std::find_if(
                cat.points.begin(), cat.points.end(),
                [&source](const auto& item) {
                    return item.address ==
                        source.terminal.certified_semantic_point;
                });
            CHECK(p != cat.points.end());
            cat.points.erase(p);
        }, part::SelectedEdgeEndStatus::terminal_missing);
        withCatalogEdit([&](auto& cat) {
            const auto p = std::find_if(
                cat.points.begin(), cat.points.end(),
                [&source](const auto& item) {
                    return item.address ==
                        source.terminal.certified_semantic_point;
                });
            CHECK(p != cat.points.end());
            p->status = kernel::ReferenceStatus::unsupported;
            p->current_vertices.clear();
            p->candidate_vertex_count = 0U;
        }, part::SelectedEdgeEndStatus::terminal_unsupported);
        const auto bad_stage = [&] {
            auto altered = source;
            altered.source_edge.stage.kind =
                part::BodyStageKind::empty_body;
            return altered;
        }();
        assertNoPayload(part::resolveSelectedMaterialEdgeEnd(
            base_document, bad_stage, base_evaluation, kernel),
            part::SelectedEdgeEndStatus::invalid_context);
        const auto& catalog =
            *base_evaluation.features.front().result_topology;
        const auto wrong_author = part::authorSelectedMaterialEdgeEnd(
            base_document, base_evaluation, bad_stage.source_edge.stage,
            current_edge, current_vertex, kernel);
        CHECK(!wrong_author.ok());
        CHECK(!wrong_author.reference);
        CHECK(wrong_author.status ==
              part::SelectedEdgeEndStatus::invalid_context);
        const auto gone_edge = part::authorSelectedMaterialEdgeEnd(
            base_document, base_evaluation, catalog.stage,
            kernel::RuntimeEdgeToken{999999999U},
            current_vertex, kernel);
        CHECK(!gone_edge.ok());
        CHECK(!gone_edge.reference);
        CHECK(gone_edge.status == part::SelectedEdgeEndStatus::edge_missing);
        kernel_occt::OcctSolidModelingKernel other_provider;
        const auto other_document = makeBasePart();
        const auto other_evaluation = part::evaluatePart(
            other_document, other_provider);
        CHECK(other_evaluation.body_status ==
              part::BodyEvaluationStatus::up_to_date);
        CHECK(other_evaluation.features.size() == 1U);
        const auto& other_feature = other_evaluation.features.front();
        CHECK(other_feature.result_solid);
        CHECK(other_feature.result_topology);
        CHECK(other_feature.result_topology->edges.size() == 12U);
        CHECK(other_feature.result_solid.get() !=
              base_evaluation.features.front().result_solid.get());
        Q1ForeignBodyBinderQuery wrong_body{
            other_provider, other_feature.result_solid,
            other_feature.result_topology->edges.front().runtime_token};
        assertNoPayload(part::resolveSelectedMaterialEdgeEnd(
            base_document, source, base_evaluation, wrong_body),
            part::SelectedEdgeEndStatus::native_geometry_unavailable);
        const auto foreign_native = part::authorSelectedMaterialEdgeEnd(
            base_document, base_evaluation, catalog.stage,
            current_edge, current_vertex, wrong_body);
        CHECK(!foreign_native.ok());
        CHECK(!foreign_native.reference);
        CHECK(foreign_native.status ==
              part::SelectedEdgeEndStatus::native_geometry_unavailable);
    }
    // G0/V11: reconstruct authored Part intent and reevaluate with an
    // independent OCCT provider instance. Only FeaturePointAddress
    // survives semantically; transient runtime tokens are neither
    // serialized nor compared as identities.
    auto reopened_document = part::PartDocument::restore(
        base_document.documentId(),
        base_document.state(),
        base_document.revision());
    CHECK(reopened_document.ok());
    kernel_occt::OcctSolidModelingKernel separate_kernel;
    const auto fresh_evaluation = part::evaluatePart(
        *reopened_document.document, separate_kernel);
    CHECK(
        fresh_evaluation.body_status ==
        part::BodyEvaluationStatus::up_to_date);
    CHECK(fresh_evaluation.features.size() == 1U);
    CHECK(fresh_evaluation.features[0].result_topology);
    const auto& rebuilt_topology =
        *fresh_evaluation.features[0].result_topology;
    const auto reconstructed_counts =
        characterizeNativeVertices(
            rebuilt_topology, "V11-new-provider");
    CHECK(reconstructed_counts.native == 8U);
    CHECK(reconstructed_counts.resolved == 8U);
    const auto e0_fresh_ends =
        characterizeE0CanonicalStraightEnds(
            rebuilt_topology, "E0-07-independent-provider");
    const auto q0_fresh_lengths = verifyQ0StrictPartStageGeometry(
        fresh_evaluation.features[0], separate_kernel,
        "Q0-cold-rebuilt-provider");
    const auto q1_fresh_sources = verifyQ1CanonicalSelectedEnds(
        *reopened_document.document, fresh_evaluation,
        separate_kernel, "Q1-03-cold-native");
    CHECK(q1_fresh_sources.size() == 24U);
    const auto s4p0_cold_keys = verifyS4P0NativeEndPointIdentity(
        *reopened_document.document, fresh_evaluation,
        separate_kernel, q1_fresh_sources,
        "P0-03-independent-OCCT", &s4p0_base_keys);
    CHECK(s4p0_cold_keys.size() == s4p0_base_keys.size());
    const auto q2_fresh_sources = verifyQ2CanonicalSelectedMids(
        *reopened_document.document, fresh_evaluation,
        separate_kernel, "Q2-M03-cold-native");
    for (const auto& original : q2_base_sources) {
        CHECK(std::find(q2_fresh_sources.begin(),
                        q2_fresh_sources.end(),
                        original) != q2_fresh_sources.end());
        const auto replay = part::resolveSelectedMaterialEdgeMid(
            *reopened_document.document, original,
            fresh_evaluation, separate_kernel);
        CHECK(replay.resolved());
    }
    // S1-F03: caller-carried intent is replayed against independent
    // native-provider and cold Part reconstruction, without persisting it.
    for (const auto& source : q1_base_sources) {
        const auto initial = part::initializeSelectedEdgeEndFrame(
            base_document,source,base_evaluation,kernel);
        CHECK(initial.resolved() && initial.initialized_intent);
        const auto cold = part::resolveSelectedEdgeEndFrame(
            *reopened_document.document,source,*initial.initialized_intent,
            fresh_evaluation,separate_kernel);
        CHECK(cold.resolved());
        CHECK(s1Near3(initial.frame->origin,cold.frame->origin));
        CHECK(s1Near3(initial.frame->u_axis,cold.frame->u_axis));
        CHECK(s1Near3(initial.frame->normal,cold.frame->normal));
    }
    for (const auto& source : q2_base_sources) {
        const auto initial = part::initializeSelectedEdgeMidFrame(
            base_document,source,base_evaluation,kernel);
        CHECK(initial.resolved() && initial.initialized_intent);
        const auto cold = part::resolveSelectedEdgeMidFrame(
            *reopened_document.document,source,*initial.initialized_intent,
            fresh_evaluation,separate_kernel);
        CHECK(cold.resolved());
        CHECK(s1Near3(initial.frame->origin,cold.frame->origin));
        CHECK(s1Near3(initial.frame->u_axis,cold.frame->u_axis));
        CHECK(s1Near3(initial.frame->normal,cold.frame->normal));
    }
    // Compare complete semantic identity sets independently of native
    // Edge/Vertex enumeration and fresh OCCT provider generation.
    for (const auto& original : q1_base_sources) {
        CHECK(std::find(q1_fresh_sources.begin(),
                        q1_fresh_sources.end(),
                        original) != q1_fresh_sources.end());
        const auto replay = part::resolveSelectedMaterialEdgeEnd(
            *reopened_document.document, original,
            fresh_evaluation, separate_kernel);
        CHECK(replay.resolved());
    }
    CHECK(q0_fresh_lengths.size() == q0_base_lengths.size());
    for (std::size_t i = 0; i < q0_base_lengths.size(); ++i) {
        CHECK(std::abs(q0_fresh_lengths[i] - q0_base_lengths[i]) < 1.0e-6);
    }
    const auto prior_bound = kernel.bindExactEdgeToBody(
        base_evaluation.features[0].result_solid,
        base_evaluation.features[0].result_topology->edges.front().runtime_token);
    CHECK(prior_bound.has_value());
    const auto stale = separate_kernel.queryExactBoundedEdge3d(
        fresh_evaluation.features[0].result_solid, *prior_bound);
    CHECK(stale.status == kernel::ExactEdge3dStatus::provider_mismatch);
    CHECK(!stale.geometry);
    CHECK(e0_fresh_ends == e0_base_ends);
    CHECK(
        rebuilt_topology.stage ==
        base_evaluation.features[0].result_topology->stage);
    std::size_t reconstructed_addresses = 0U;
    for (const auto& address_resolution :
         base_evaluation.features[0].result_topology->points) {
        CHECK(address_resolution.status ==
              kernel::ReferenceStatus::resolved);
        const auto found = std::find_if(
            rebuilt_topology.points.begin(),
            rebuilt_topology.points.end(),
            [&address_resolution](const auto& current) {
                return current.address ==
                       address_resolution.address;
            });
        CHECK(found != rebuilt_topology.points.end());
        CHECK(found->status ==
              kernel::ReferenceStatus::resolved);
        CHECK(found->current_vertices.size() == 1U);
        ++reconstructed_addresses;
    }
    CHECK(reconstructed_addresses == 8U);
    std::cout
        << "RG02A_G0_V11"
        << " reconstructed_semantic_addresses=8"
        << " independent_kernel=1"
        << " runtime_token_equality=NOT_AUTHORITY"
        << " persisted_rg02_point_source=NOT_IMPLEMENTED\\n";
    // G0/V02: changing the earlier Feature geometry must recompute
    // points without changing their authored FeaturePointAddress meaning.
    // XYZ is checked only AFTER semantic identity was matched.
    const auto taller_document =
        withEditedBaseHeight(base_document, 35.0);
    const auto taller_evaluation =
        part::evaluatePart(taller_document, kernel);
    CHECK(
        taller_evaluation.body_status ==
        part::BodyEvaluationStatus::up_to_date);
    CHECK(taller_evaluation.features.size() == 1U);
    CHECK(taller_evaluation.features[0].result_topology);
    const auto& original_stage =
        *base_evaluation.features[0].result_topology;
    const auto& taller_stage =
        *taller_evaluation.features[0].result_topology;
    CHECK(original_stage.stage == taller_stage.stage);
    const auto g0_taller = characterizeNativeVertices(
        taller_stage, "V02-edited-extrude");
    CHECK(g0_taller.native == 8U);
    CHECK(g0_taller.resolved == 8U);
    const auto e0_taller_ends =
        characterizeE0CanonicalStraightEnds(
            taller_stage, "E0-07-height-edit");
    const auto q0_taller_lengths = verifyQ0StrictPartStageGeometry(
        taller_evaluation.features[0], kernel, "Q0-height-edit");
    const auto q1_taller_sources = verifyQ1CanonicalSelectedEnds(
        taller_document, taller_evaluation, kernel, "Q1-04-height-edit");
    CHECK(q1_taller_sources.size() == 24U);
    const auto s4p0_edited_keys = verifyS4P0NativeEndPointIdentity(
        taller_document, taller_evaluation, kernel,
        q1_taller_sources, "P0-03-same-feature-height-edit",
        &s4p0_base_keys);
    CHECK(s4p0_edited_keys.size() == s4p0_base_keys.size());
    const auto q2_taller_sources = verifyQ2CanonicalSelectedMids(
        taller_document, taller_evaluation, kernel, "Q2-M04-height-edit");
    CHECK(q2_taller_sources.size() == q2_base_sources.size());
    // S1-F04: a source-preserving Extrude height edit must retain
    // the signed orientation through the caller's original intent.
    for (const auto& source : q1_base_sources) {
        const auto initial = part::initializeSelectedEdgeEndFrame(
            base_document,source,base_evaluation,kernel);
        CHECK(initial.resolved() && initial.initialized_intent);
        const auto edited = part::resolveSelectedEdgeEndFrame(
            taller_document,source,*initial.initialized_intent,
            taller_evaluation,kernel);
        CHECK(edited.resolved());
        CHECK(s1Near3(initial.frame->normal,edited.frame->normal));
    }
    for (const auto& source : q2_base_sources) {
        const auto initial = part::initializeSelectedEdgeMidFrame(
            base_document,source,base_evaluation,kernel);
        CHECK(initial.resolved() && initial.initialized_intent);
        const auto edited = part::resolveSelectedEdgeMidFrame(
            taller_document,source,*initial.initialized_intent,
            taller_evaluation,kernel);
        CHECK(edited.resolved());
        CHECK(s1Near3(initial.frame->normal,edited.frame->normal));
    }
    std::size_t changed_mid_positions = 0U;
    for (const auto& original : q2_base_sources) {
        CHECK(std::find(q2_taller_sources.begin(),
                        q2_taller_sources.end(),
                        original) != q2_taller_sources.end());
        const auto original_mid = part::resolveSelectedMaterialEdgeMid(
            base_document, original, base_evaluation, kernel);
        const auto edited_mid = part::resolveSelectedMaterialEdgeMid(
            taller_document, original, taller_evaluation, kernel);
        CHECK(original_mid.resolved() && edited_mid.resolved());
        const auto& a = *original_mid.current_position_world;
        const auto& b = *edited_mid.current_position_world;
        if (std::abs(a.z - b.z) > 1.0e-6) {
            ++changed_mid_positions;
        }
    }
    CHECK(changed_mid_positions >= 1U);
    // Q1 regression: original END identity unchanged
    for (const auto& original : q1_base_sources) {
        CHECK(std::find(q1_taller_sources.begin(),
                        q1_taller_sources.end(),
                        original) != q1_taller_sources.end());
        const auto replay = part::resolveSelectedMaterialEdgeEnd(
            taller_document, original, taller_evaluation, kernel);
        CHECK(replay.resolved());
    }
    CHECK(q0_taller_lengths.size() == q0_base_lengths.size());
    CHECK(q0_taller_lengths != q0_base_lengths);
    const auto stale_edit = kernel.queryExactBoundedEdge3d(
        taller_evaluation.features[0].result_solid, *prior_bound);
    CHECK(stale_edit.status == kernel::ExactEdge3dStatus::provider_mismatch);
    CHECK(!stale_edit.geometry);
    CHECK(e0_taller_ends == e0_base_ends);
    std::size_t moved_semantic_points = 0U;
    std::size_t stationary_semantic_points = 0U;
    for (const auto& before_vertex : original_stage.vertices) {
        CHECK(
            before_vertex.referenceability ==
            kernel::ReferenceStatus::resolved);
        CHECK(before_vertex.point_candidates.size() == 1U);
        CHECK(before_vertex.provider_point.has_value());
        const auto& identity =
            before_vertex.point_candidates.front();
        const auto later = std::find_if(
            taller_stage.vertices.begin(),
            taller_stage.vertices.end(),
            [&identity](const auto& vertex) {
                return vertex.referenceability ==
                           kernel::ReferenceStatus::resolved &&
                       vertex.point_candidates.size() == 1U &&
                       vertex.point_candidates.front() == identity;
            });
        CHECK(later != taller_stage.vertices.end());
        CHECK(later->provider_point.has_value());
        const auto delta_z =
            later->provider_point->z -
            before_vertex.provider_point->z;
        if (delta_z > 14.99 && delta_z < 15.01) {
            ++moved_semantic_points;
        } else {
            CHECK(delta_z > -0.01 && delta_z < 0.01);
            ++stationary_semantic_points;
        }
    }
    CHECK(moved_semantic_points == 4U);
    CHECK(stationary_semantic_points == 4U);
    // G0/V12: replacing the producer Feature, while keeping the same
    // Sketch/Profile/shape dimensions, must make every former semantic
    // address absent from the new stage. Equal provider XYZ is NO authority.
    const auto replaced_document =
        withReplacementBaseFeature(base_document);
    const auto replaced_evaluation =
        part::evaluatePart(replaced_document, kernel);
    CHECK(
        replaced_evaluation.body_status ==
        part::BodyEvaluationStatus::up_to_date);
    CHECK(replaced_evaluation.features.size() == 1U);
    CHECK(replaced_evaluation.features[0].result_topology);
    const auto& replacement_stage =
        *replaced_evaluation.features[0].result_topology;
    CHECK(replacement_stage.stage != original_stage.stage);
    for (const auto& original : q2_base_sources) {
        const auto missing = part::resolveSelectedMaterialEdgeMid(
            replaced_document, original, replaced_evaluation, kernel);
        CHECK(missing.status ==
              part::SelectedEdgeMidStatus::edge_missing);
        CHECK(!missing.resolved());
        CHECK(!missing.current_edge);
        CHECK(!missing.current_position_world);
        CHECK(!missing.current_tangent_line);
    }
    for (const auto& old_source : q1_base_sources) {
        const auto absent = part::resolveSelectedMaterialEdgeEnd(
            replaced_document, old_source, replaced_evaluation, kernel);
        CHECK(absent.status == part::SelectedEdgeEndStatus::edge_missing);
        CHECK(!absent.resolved());
        CHECK(!absent.current_edge);
        CHECK(!absent.current_terminal);
        CHECK(!absent.current_position_world);
    }
    const auto replaced_counts =
        characterizeNativeVertices(
            replacement_stage, "V12-replaced-producer");
    CHECK(replaced_counts.native == g0_base.native);
    CHECK(replaced_counts.resolved == g0_base.resolved);
    std::size_t equal_xyz_different_semantics = 0U;
    for (const auto& old_vertex : original_stage.vertices) {
        CHECK(old_vertex.referenceability ==
              kernel::ReferenceStatus::resolved);
        CHECK(old_vertex.provider_point.has_value());
        CHECK(old_vertex.point_candidates.size() == 1U);
        const auto old_address =
            old_vertex.point_candidates.front();
        CHECK(std::none_of(
            replacement_stage.points.begin(),
            replacement_stage.points.end(),
            [&old_address](const auto& point) {
                return point.address == old_address;
            }));
        // Only verify the requested same-looking-geometry fixture.
        // The test NEVER uses matching coordinates to resolve identity.
        const auto identical_xyz = std::count_if(
            replacement_stage.vertices.begin(),
            replacement_stage.vertices.end(),
            [&old_vertex](const auto& candidate) {
                if (!candidate.provider_point) return false;
                const auto& a = *old_vertex.provider_point;
                const auto& b = *candidate.provider_point;
                return std::abs(a.x - b.x) < 1.0e-8 &&
                       std::abs(a.y - b.y) < 1.0e-8 &&
                       std::abs(a.z - b.z) < 1.0e-8;
            });
        CHECK(identical_xyz == 1);
        ++equal_xyz_different_semantics;
    }
    CHECK(equal_xyz_different_semantics == 8U);
    std::cout
        << "RG02A_G0_V12"
        << " same_xyz_different_semantic_address=8"
        << " source_feature_removed_from_history=1"
        << " no_coordinate_rebind=1\\n";
    std::cout
        << "RG02A_G0_V02"
        << " same_semantic_stage=1"
        << " preserved_point_addresses=8"
        << " moved_z=4"
        << " stationary_z=4"
        << " xyz_used_for_identity=0\\n";

    // G0/V03: a real bounded Cut opening an interior pocket in the
    // positive-Z base prism. Inherited and new Vertex semantics are compared
    // by durable Point addresses, not runtime token/XYZ equality.
    const auto cut_document =
        withInteriorCut(base_document);
    const auto cut_evaluation =
        part::evaluatePart(cut_document, kernel);
    CHECK(
        cut_evaluation.body_status ==
        part::BodyEvaluationStatus::up_to_date);
    CHECK(cut_evaluation.features.size() == 2U);
    CHECK(
        cut_evaluation.features[0].result_topology);
    CHECK(
        cut_evaluation.features[1].status ==
        part::FeatureEvaluationStatus::up_to_date);
    CHECK(
        cut_evaluation.features[1].result_topology);
    const auto& g0_cut_stage =
        *cut_evaluation.features[1].result_topology;
    CHECK(
        g0_cut_stage.stage !=
        cut_evaluation.features[0]
            .result_topology->stage);
    const auto g0_cut =
        characterizeNativeVertices(
            g0_cut_stage,
            "V03-interior-cut");
    CHECK(g0_cut.native > g0_base.native);
    CHECK(g0_cut.native == 16U);
    CHECK(g0_cut.resolved == 16U);
    CHECK(g0_cut.ambiguous == 0U);
    CHECK(g0_cut.missing == 0U);
    CHECK(g0_cut.unsupported == 0U);
    std::size_t inherited_points = 0U;
    std::size_t new_points = 0U;
    for (const auto& vertex : g0_cut_stage.vertices) {
        if (vertex.referenceability !=
                kernel::ReferenceStatus::resolved) {
            continue;
        }
        CHECK(vertex.point_candidates.size() == 1U);
        const auto& current_point =
            vertex.point_candidates.front();
        const auto inherited =
            std::find_if(
                base_evaluation.features[0]
                    .result_topology->points.begin(),
                base_evaluation.features[0]
                    .result_topology->points.end(),
                [&current_point](const auto& point) {
                    return point.address == current_point;
                });
        if (inherited !=
            base_evaluation.features[0]
                .result_topology->points.end()) {
            ++inherited_points;
        } else {
            ++new_points;
        }
    }
    CHECK(inherited_points == 8U);
    CHECK(new_points == 8U);
    // G0/V07: remove a full-height corner with an overlapping Cut. A
    // three-Surface point carried by the removed corner must be Missing
    // in the final stage; it must never rebind by similar XYZ geometry.
    const auto removed_corner_document =
        withInteriorCut(
            base_document,
            {-5.0, -5.0},
            {15.0, 15.0},
            20.0);
    const auto removed_corner_eval =
        part::evaluatePart(
            removed_corner_document, kernel);
    CHECK(
        removed_corner_eval.body_status ==
        part::BodyEvaluationStatus::up_to_date);
    CHECK(removed_corner_eval.features.size() == 2U);
    CHECK(removed_corner_eval.features[0].result_topology);
    CHECK(removed_corner_eval.features[1].result_topology);
    const auto& removed_stage =
        *removed_corner_eval.features[1].result_topology;
    const auto removed_counts =
        characterizeNativeVertices(
            removed_stage, "V07-removed-corner");
    CHECK(removed_stage.stage != original_stage.stage);
    CHECK(removed_counts.native == 12U);
    CHECK(removed_counts.resolved == 12U);
    CHECK(removed_counts.unsupported == 0U);
    CHECK(removed_counts.ambiguous == 0U);
    CHECK(removed_counts.missing == 0U);
    std::size_t disappeared_addresses = 0U;
    std::size_t retained_addresses = 0U;
    for (const auto& old_point : original_stage.points) {
        CHECK(
            old_point.status ==
            kernel::ReferenceStatus::resolved);
        const auto current = std::find_if(
            removed_stage.points.begin(),
            removed_stage.points.end(),
            [&old_point](const auto& candidate) {
                return candidate.address == old_point.address;
            });
        CHECK(current != removed_stage.points.end());
        if (current->status ==
                kernel::ReferenceStatus::missing) {
            CHECK(current->candidate_vertex_count == 0U);
            CHECK(current->current_vertices.empty());
            ++disappeared_addresses;
        } else {
            CHECK(
                current->status ==
                kernel::ReferenceStatus::resolved);
            CHECK(current->current_vertices.size() == 1U);
            ++retained_addresses;
        }
    }
    CHECK(disappeared_addresses == 2U);
    CHECK(retained_addresses == 6U);
    CHECK(
        disappeared_addresses + retained_addresses ==
        original_stage.points.size());
    std::cout
        << "RG02A_G0_V07"
        << " disappeared_addresses=" << disappeared_addresses
        << " retained_addresses=" << retained_addresses
        << " current_native_vertices=" << removed_counts.native
        << " nearest_xyz_rebind=0\\n";
    std::cout
        << "RG02A_G0_V03"
        << " inherited_resolved=" << inherited_points
        << " new_resolved=" << new_points
        << " prior_stage_independent=1"
        << " persisted_new_point_source=NOT_IMPLEMENTED\\n";
    const auto fillet_edges =
        trihedralReferences(
            *base_evaluation.features[0]
                 .result_topology);
    CHECK(fillet_edges.size() == 3U);

    // Symmetric common-corner production proof for Chamfer. C2 must not only
    // execute the native operation; it must publish the same P2/P3 semantic
    // topology families and a complete Part catalog.
    auto trihedral_chamfer_document =
        withEdgeFeature(
            base_document,
            fillet_edges,
            true,
            1.5);
    const auto trihedral_chamfer =
        part::evaluatePart(
            trihedral_chamfer_document,
            kernel);
    CHECK(
        trihedral_chamfer.body_status ==
        part::BodyEvaluationStatus::up_to_date);
    CHECK(
        trihedral_chamfer.features.size() ==
        2U);
    const auto& chamfer_feature =
        trihedral_chamfer.features[1];
    CHECK(
        chamfer_feature.status ==
        part::FeatureEvaluationStatus::
            up_to_date);
    CHECK(chamfer_feature.result_topology);
    CHECK(
        chamfer_feature.result_topology
            ->complete());
    // G0/V09: an authored earlier-stage Vertex source stays bound
    // to its before-Chamfer history stage. This is catalog evidence,
    // not a persisted Datum or a repair behavior claim.
    CHECK(
        trihedral_chamfer.features[0].result_topology);
    const auto& earlier_stage =
        *trihedral_chamfer.features[0].result_topology;
    const auto& after_chamfer_stage =
        *chamfer_feature.result_topology;
    CHECK(earlier_stage.stage == original_stage.stage);
    CHECK(after_chamfer_stage.stage != earlier_stage.stage);
    CHECK(earlier_stage.vertices.size() == 8U);
    std::size_t original_addresses_in_earlier_stage = 0U;
    for (const auto& vertex : original_stage.vertices) {
        CHECK(vertex.point_candidates.size() == 1U);
        const auto& address = vertex.point_candidates.front();
        const auto found = std::find_if(
            earlier_stage.vertices.begin(),
            earlier_stage.vertices.end(),
            [&address](const auto& candidate) {
                return candidate.referenceability ==
                           kernel::ReferenceStatus::resolved &&
                       candidate.point_candidates.size() == 1U &&
                       candidate.point_candidates.front() == address;
            });
        CHECK(found != earlier_stage.vertices.end());
        ++original_addresses_in_earlier_stage;
    }
    CHECK(original_addresses_in_earlier_stage == 8U);
    std::cout
        << "RG02A_G0_V09"
        << " original_stage_vertices=8"
        << " before_chamfer_point_addresses=8"
        << " stage_scoping=1"
        << " persisted_datum_source=NOT_IMPLEMENTED\\n";
    // G0.2 characterization only: no guessed expected cardinality.
    const auto g0_chamfer = characterizeNativeVertices(
        *chamfer_feature.result_topology,
        "V06-chamfer");
    CHECK(g0_chamfer.native == 13U);
    CHECK(g0_chamfer.resolved == 10U);
    CHECK(g0_chamfer.unsupported == 3U);
    CHECK(g0_chamfer.ambiguous == 0U);
    CHECK(g0_chamfer.missing == 0U);
    verifyQ1NoGlobalPointPromotion(
        trihedral_chamfer_document, trihedral_chamfer, kernel,
        "Q1-06-real-chamfer");
    CHECK(
        countRole(
            chamfer_feature,
            part::FeatureSurfaceRoleKind::
                chamfer_surface) == 3U);
    CHECK(
        countRole(
            chamfer_feature,
            part::FeatureSurfaceRoleKind::
                corner_transition) >= 1U);
    CHECK(
        std::count_if(
            chamfer_feature.produced_curves.begin(),
            chamfer_feature.produced_curves.end(),
            [](const auto& curve) {
                return curve.address.role ==
                       part::FeatureCurveRoleKind::
                           edge_feature_boundary;
            }) > 0);

    // ADR-0016: provider UV axes are not semantic frame authority. Rotate
    // every provider-reported generated planar U/V basis while preserving its
    // plane equation; Part must reconstruct identical canonical frames from
    // upstream semantic provenance.
    RotatedGeneratedPlaneFrameKernel
        rotated_provider;
    const auto rotated_chamfer =
        part::evaluatePart(
            trihedral_chamfer_document,
            rotated_provider);
    CHECK(
        rotated_chamfer.body_status ==
        part::BodyEvaluationStatus::up_to_date);
    CHECK(
        rotated_chamfer.features.size() ==
        2U);
    CHECK(
        rotated_chamfer.features[1].status ==
        part::FeatureEvaluationStatus::
            up_to_date);

    const auto baseline_frames =
        generatedPlanarFrames(
            chamfer_feature);
    const auto rotated_frames =
        generatedPlanarFrames(
            rotated_chamfer.features[1]);
    CHECK(!baseline_frames.empty());
    CHECK(rotated_frames == baseline_frames);

    auto fillet_state =
        base_document.state();
    const auto fillet_id =
        fillet_state.body.next_feature_id
            .allocate();
    CHECK(fillet_id.has_value());
    fillet_state.body.features.push_back(
        part::PartFeature{
            *fillet_id,
            "Fillet001",
            false,
            part::FilletFeature{
                fillet_edges,
                core::LengthValue{2.0}}});

    auto fillet_document =
        part::PartDocument::restore(
            base_document.documentId(),
            std::move(fillet_state),
            base_document.revision());
    CHECK(fillet_document.ok());

    const auto fillet_evaluation =
        part::evaluatePart(
            *fillet_document.document,
            kernel);
    CHECK(
        fillet_evaluation.body_status ==
        part::BodyEvaluationStatus::up_to_date);
    CHECK(fillet_evaluation.features.size() == 2U);
    const auto& fillet =
        fillet_evaluation.features[1];
    CHECK(
        fillet.status ==
        part::FeatureEvaluationStatus::up_to_date);
    CHECK(fillet.result_topology);
    CHECK(fillet.result_topology->complete());
    // G0.2 characterization only: record any unsupported transitions.
    const auto g0_fillet = characterizeNativeVertices(
        *fillet.result_topology,
        "V05-fillet");
    CHECK(g0_fillet.native == 13U);
    CHECK(g0_fillet.resolved == 10U);
    CHECK(g0_fillet.unsupported == 3U);
    CHECK(g0_fillet.ambiguous == 0U);
    CHECK(g0_fillet.missing == 0U);
    verifyQ1NoGlobalPointPromotion(
        *fillet_document.document, fillet_evaluation, kernel,
        "Q1-06-real-fillet");
    CHECK(
        countRole(
            fillet,
            part::FeatureSurfaceRoleKind::
                fillet_surface) == 3U);
    CHECK(
        countRole(
            fillet,
            part::FeatureSurfaceRoleKind::
                corner_transition) >= 1U);
    CHECK(
        std::count_if(
            fillet.produced_curves.begin(),
            fillet.produced_curves.end(),
            [](const auto& curve) {
                return curve.address.role ==
                       part::FeatureCurveRoleKind::
                           edge_feature_boundary;
            }) > 0);

    // Chaining acceptance follows the Owner-accepted PM-05A C1 evidence
    // exactly: one explicit source Edge -> Fillet -> ordinary generated
    // material-boundary Edge -> Chamfer. Trihedral K1/P2/P3 coverage above is
    // intentionally separate; the evidence did not claim that boundaries
    // produced by a three-Edge Fillet are all strict-T1 Chamfer-feasible.
    const auto source_edge_candidates =
        singleEdgeCandidates(
            *base_evaluation.features[0]
                 .result_topology);
    CHECK(!source_edge_candidates.empty());

    // PM-05A C1 is an existence proof for a valid single-edge Fillet ->
    // generated boundary -> Chamfer chain. Do not let catalog/runtime
    // enumeration order silently pick which authored source Edge represents
    // that evidence. Each attempt below is a separate explicit authored
    // Feature sequence; the product itself never substitutes an Edge.
    std::size_t source_attempts = 0U;
    std::size_t fillet_successes = 0U;
    std::size_t chain_attempts = 0U;
    std::size_t chain_successes = 0U;

    for (const auto& source_edge :
         source_edge_candidates) {
        ++source_attempts;

        auto single_fillet_document =
            withEdgeFeature(
                base_document,
                {source_edge},
                false,
                2.0);
        const auto single_fillet_evaluation =
            part::evaluatePart(
                single_fillet_document,
                kernel);
        if (single_fillet_evaluation.body_status !=
                part::BodyEvaluationStatus::
                    up_to_date ||
            single_fillet_evaluation.features.size() !=
                2U ||
            single_fillet_evaluation.features[1].status !=
                part::FeatureEvaluationStatus::
                    up_to_date) {
            continue;
        }
        ++fillet_successes;

        const auto& single_fillet =
            single_fillet_evaluation.features[1];
        CHECK(single_fillet.result_topology);
        CHECK(
            single_fillet.result_topology->complete());

        const auto generated_edges =
            generatedBoundaryReferences(
                single_fillet);
        CHECK(!generated_edges.empty());

        // PM-05A C1 proved that an ordinary generated boundary can feed the
        // opposite operation, while T1 independently requires the complete
        // native contour to be explicit authored intent. Probe only the
        // runtime membership induced by each generated seed, then require that
        // every contour member can be re-authored as a durable semantic Edge.
        // The second Part Feature is created from exactly that semantic set;
        // no production fallback or automatic contour expansion is involved.
        for (const auto& generated_edge :
             generated_edges) {
            const auto explicit_edges =
                explicitContourReferences(
                    single_fillet,
                    generated_edge,
                    kernel);
            if (!explicit_edges) {
                continue;
            }
            ++chain_attempts;

            auto chamfer_state =
                single_fillet_document.state();
            const auto chamfer_id =
                chamfer_state.body.next_feature_id
                    .allocate();
            CHECK(chamfer_id.has_value());
            chamfer_state.body.features.push_back(
                part::PartFeature{
                    *chamfer_id,
                    "Chamfer001",
                    false,
                    part::ChamferFeature{
                        *explicit_edges,
                        core::LengthValue{0.75}}});

            auto chamfer_document =
                part::PartDocument::restore(
                    single_fillet_document
                        .documentId(),
                    std::move(chamfer_state),
                    single_fillet_document
                        .revision());
            CHECK(chamfer_document.ok());

            const auto chained =
                part::evaluatePart(
                    *chamfer_document.document,
                    kernel);
            if (chained.body_status !=
                    part::BodyEvaluationStatus::
                        up_to_date ||
                chained.features.size() != 3U ||
                chained.features[2].status !=
                    part::FeatureEvaluationStatus::
                        up_to_date) {
                continue;
            }

            CHECK(
                chained.features[2]
                    .result_topology);
            CHECK(
                chained.features[2]
                    .result_topology->complete());
            CHECK(
                countRole(
                    chained.features[2],
                    part::FeatureSurfaceRoleKind::
                        chamfer_surface) >= 1U);
            ++chain_successes;
            break;
        }

        if (chain_successes > 0U) {
            break;
        }
    }

    CHECK(source_attempts > 0U);
    CHECK(fillet_successes > 0U);
    CHECK(chain_attempts > 0U);
    CHECK(chain_successes > 0U);

    std::cout
        << "PM05C2B_PART_EDGE_TOPOLOGY_PASS"
        << " trihedral_edges=3"
        << " p2=1"
        << " p3=1"
        << " trihedral_chamfer=1"
        << " provider_uv_authority=0"
        << " generated_boundary=1"
        << " fillet_to_chamfer_chain=1"
        << " source_attempts=" << source_attempts
        << " fillet_successes=" << fillet_successes
        << " chain_attempts=" << chain_attempts
        << " chain_successes=" << chain_successes
        << " xyz_identity=0\n";
    return EXIT_SUCCESS;
}
