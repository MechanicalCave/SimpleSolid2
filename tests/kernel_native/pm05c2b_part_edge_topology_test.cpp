#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel_occt/solid_modeling_kernel.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <algorithm>
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
    const part::PartDocument& base) {
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
                {10.0, 5.0},
                {30.0, 25.0},
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
                    core::LengthValue{10.0},
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
    CHECK(inherited_points >= 8U);
    CHECK(new_points > 0U);
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
    (void)characterizeNativeVertices(
        *chamfer_feature.result_topology,
        "V06-chamfer");
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
    (void)characterizeNativeVertices(
        *fillet.result_topology,
        "V05-fillet");
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
