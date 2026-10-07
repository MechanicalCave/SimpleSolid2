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
        CHECK(generated_edges.size() <= 16U);

        // PM-05A C1 proved raw provider feasibility for individual generated
        // boundary Edges, but that evidence did not apply T1 to the second
        // operation. Production must never accept provider contour growth.
        // Therefore exercise every explicit non-empty authored subset of the
        // generated semantic Edge set and require at least one exact-T1
        // feasible chain. This is test enumeration only; the product never
        // adds an Edge that the user did not author.
        const std::uint64_t subset_count =
            std::uint64_t{1}
            << generated_edges.size();
        for (std::uint64_t mask = 1U;
             mask < subset_count;
             ++mask) {
            std::vector<part::MaterialEdgeReference>
                chamfer_edges;
            for (std::size_t index = 0U;
                 index < generated_edges.size();
                 ++index) {
                if ((mask &
                     (std::uint64_t{1} << index)) !=
                    0U) {
                    chamfer_edges.push_back(
                        generated_edges[index]);
                }
            }
            CHECK(!chamfer_edges.empty());
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
                        std::move(chamfer_edges),
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
