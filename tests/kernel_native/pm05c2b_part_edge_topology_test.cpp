#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel_occt/solid_modeling_kernel.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
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
    double angle) {
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
                    false}}});

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
        bool mismatch)
        : delegate_{delegate}, mismatch_{mismatch} {}

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
        return q;
    }
private:
    kernel::IExactBoundedEdgeGeometryQuery& delegate_;
    bool mismatch_;
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
    std::cout << "RG02B1_Q1_NEGATIVE"
              << " fixture=" << fixture
              << " unsupported_or_ambiguous_vertices="
              << unsupported_vertices
              << " rejected_edge_terminal_uses=" << rejected_end_uses
              << " global_point_promotions=0\n";
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

} // namespace

int main() {
    kernel_occt::OcctSolidModelingKernel kernel;

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
    // Q1-07: full tuple reorder is harmless; a forged membership token
    // causes typed unavailable with no partial END/geometry publication.
    {
        Q1TerminalTupleQuery permuted{kernel, false};
        Q1TerminalTupleQuery corrupted{kernel, true};
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
