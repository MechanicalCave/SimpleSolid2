#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-05C1 edge semantic/kernel CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

part::FeatureId featureId(const char* text) {
    const auto parsed =
        part::FeatureId::parse(text);
    CHECK(parsed.has_value());
    return *parsed;
}

part::FeatureSurfaceAddress surface(
    part::FeatureId producer,
    part::FeatureSurfaceRoleKind role) {
    part::FeatureSurfaceAddress result{
        producer,
        role,
        std::nullopt,
        0U,
        0U,
        false};
    CHECK(result.valid());
    return result;
}

part::FeatureCurveAddress curve(
    part::FeatureId producer,
    part::FeatureSurfaceAddress first,
    part::FeatureSurfaceAddress second) {
    std::vector<part::FeatureSurfaceAddress>
        surfaces{
            std::move(first),
            std::move(second)};
    std::sort(
        surfaces.begin(),
        surfaces.end());
    part::FeatureCurveAddress result{
        producer,
        part::FeatureCurveRoleKind::cap_side,
        std::move(surfaces)};
    CHECK(result.valid());
    return result;
}

part::BodyEdgeTopologyRecord edgeRecord(
    kernel::RuntimeEdgeToken token,
    const part::FeatureCurveAddress& address,
    kernel::ReferenceStatus referenceability =
        kernel::ReferenceStatus::resolved) {
    part::BodyEdgeTopologyRecord result;
    result.runtime_token = token;
    result.accounting_class =
        part::TopologyAccountingClass::referenceable;
    result.referenceability =
        referenceability;
    result.curve_kind =
        kernel::CurveKind::line;
    result.curve_candidates = {address};
    CHECK(result.valid());
    return result;
}

part::BodyStageTopologyCatalog catalogFor(
    part::FeatureId producer,
    const part::FeatureCurveAddress& address,
    kernel::ReferenceStatus status,
    std::vector<kernel::RuntimeEdgeToken> edges) {
    part::BodyStageTopologyCatalog catalog;
    catalog.stage = {
        part::BodyStageKind::after_feature,
        producer};

    part::FeatureCurveResolution resolution;
    resolution.address = address;
    resolution.status = status;
    resolution.strict_edge_status =
        status == kernel::ReferenceStatus::resolved &&
                edges.size() != 1U
            ? kernel::ReferenceStatus::ambiguous
            : status;
    resolution.candidate_edge_count =
        edges.size();
    resolution.curve_kind =
        kernel::CurveKind::line;
    resolution.current_edges = edges;
    CHECK(resolution.valid());
    catalog.curves.push_back(
        std::move(resolution));

    for (const auto token : edges) {
        catalog.edges.push_back(
            edgeRecord(
                token,
                address,
                edges.size() == 1U
                    ? kernel::ReferenceStatus::resolved
                    : kernel::ReferenceStatus::ambiguous));
    }
    CHECK(catalog.complete());
    return catalog;
}

part::MaterialEdgeReference referenceFor(
    part::FeatureId producer,
    const part::FeatureCurveAddress& address) {
    part::MaterialEdgeReference result{
        {
            part::BodyStageKind::after_feature,
            producer},
        address,
        part::SingularAtAuthoredStage{}};
    CHECK(result.valid());
    return result;
}

class FakeSolid final
    : public kernel::RuntimeSolid {};

class EdgeKernel final
    : public kernel::ISolidModelingKernel {
public:
    enum class EdgeMode {
        fail_provider,
        fake_success_membership_mismatch,
    };

    explicit EdgeKernel(
        EdgeMode mode = EdgeMode::fail_provider)
        : mode_{mode} {}

    std::size_t edge_calls{};
    std::vector<kernel::EdgeFeatureInput>
        edge_inputs;

    kernel::SolidModelingResult extrude(
        const kernel::LinearExtrudeInput& input,
        kernel::RuntimeSolidHandle = {}) noexcept override {
        kernel::SolidModelingResult result;
        if (!input.valid() ||
            input.profile.outer.boundary.empty()) {
            result.status =
                kernel::SolidModelingStatus::
                    invalid_input;
            return result;
        }

        const auto publish =
            [&result](
                kernel::GeneratedFaceRole role,
                kernel::SurfaceKind kind,
                kernel::RuntimeFaceToken face,
                kernel::RuntimeSurfaceToken surface) {
                result.current_faces.push_back(face);
                result.new_faces.push_back(
                    {
                        role,
                        kernel::ReferenceStatus::resolved,
                        1U,
                        face,
                    });
                result.new_surfaces.push_back(
                    {
                        std::move(role),
                        kernel::ReferenceStatus::resolved,
                        kernel::ReferenceStatus::resolved,
                        1U,
                        kind,
                        std::nullopt,
                        surface,
                        {face},
                    });
            };

        const kernel::RuntimeFaceToken cap_face{1U};
        const kernel::RuntimeFaceToken side_face{2U};
        const kernel::RuntimeSurfaceToken cap_surface{11U};
        const kernel::RuntimeSurfaceToken side_surface{12U};

        publish(
            {
                kernel::GeneratedFaceRoleKind::cap,
                kernel::ExtrudeCapRole::profile_cap,
                std::nullopt,
            },
            kernel::SurfaceKind::plane,
            cap_face,
            cap_surface);
        publish(
            {
                kernel::GeneratedFaceRoleKind::side,
                std::nullopt,
                input.profile.outer.boundary.front()
                    .provenance,
            },
            kernel::SurfaceKind::plane,
            side_face,
            side_surface);

        const kernel::RuntimeEdgeToken edge{41U};
        result.current_edges = {edge};
        result.current_edge_semantics.push_back(
            {
                edge,
                kernel::CurveKind::line,
                false,
                {cap_surface, side_surface},
                false,
            });

        result.status =
            kernel::SolidModelingStatus::ok;
        result.solid =
            std::make_shared<FakeSolid>();
        result.brep_valid = true;
        result.solid_count = 1U;
        result.face_count =
            result.current_faces.size();
        result.edge_count = 1U;
        result.vertex_count = 0U;
        return result;
    }

    kernel::SolidModelingResult edgeFeature(
        const kernel::EdgeFeatureInput& input,
        kernel::RuntimeSolidHandle upstream = {}) noexcept override {
        ++edge_calls;
        edge_inputs.push_back(input);

        kernel::SolidModelingResult result;
        if (!input.valid()) {
            result.status =
                kernel::SolidModelingStatus::
                    invalid_input;
            return result;
        }
        if (!upstream) {
            result.status =
                kernel::SolidModelingStatus::
                    missing_upstream;
            return result;
        }

        if (mode_ ==
            EdgeMode::fake_success_membership_mismatch) {
            result.status =
                kernel::SolidModelingStatus::ok;
            result.solid =
                std::make_shared<FakeSolid>();
            result.brep_valid = true;
            result.solid_count = 1U;
            result.edge_feature_input_membership =
                kernel::EdgeFeatureInputMembership{
                    {kernel::RuntimeEdgeToken{999U}}};
            return result;
        }

        result.status =
            kernel::SolidModelingStatus::
                provider_failure;
        return result;
    }

private:
    EdgeMode mode_;
};

struct EvalFixture final {
    part::PartDocument document;
    part::FeatureId base_id;
    part::FeatureId edge_feature_id;
    part::MaterialEdgeReference edge_reference;
};

EvalFixture makeEvalFixture(bool reference_existing_edge) {
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
                {30.0, 20.0},
                sketch::EntityRole::regular,
                false});
    CHECK(rectangle.ok());
    CHECK(rectangle.entity_ids.size() == 4U);

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
    CHECK(intent->outer.boundary.size() >= 2U);

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
    const auto fillet_id =
        state.body.next_feature_id.allocate();
    CHECK(base_id && fillet_id);

    state.body.features.push_back(
        part::PartFeature{
            *base_id,
            "Base",
            false,
            part::ExtrudeFeature{
                *profile.profile_id,
                part::ExtrudeOperation::add,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{10.0},
                    false}}});

    const auto cap =
        surface(
            *base_id,
            part::FeatureSurfaceRoleKind::
                profile_cap);
    const std::size_t boundary_index =
        reference_existing_edge ? 0U : 1U;
    part::FeatureSurfaceAddress side{
        *base_id,
        part::FeatureSurfaceRoleKind::side,
        intent->outer.boundary
            .at(boundary_index)
            .source_entity,
        0U,
        static_cast<std::uint32_t>(
            boundary_index),
        false};
    CHECK(side.valid());

    const auto address =
        curve(
            *base_id,
            cap,
            side);
    const auto edge_reference =
        referenceFor(
            *base_id,
            address);

    state.body.features.push_back(
        part::PartFeature{
            *fillet_id,
            "Fillet001",
            false,
            part::FilletFeature{
                {edge_reference},
                core::LengthValue{2.0}}});

    auto restored =
        part::PartDocument::restore(
            session.document().documentId(),
            std::move(state),
            session.document().revision());
    CHECK(restored.ok());
    return {
        std::move(*restored.document),
        *base_id,
        *fillet_id,
        edge_reference};
}

} // namespace

int main() {
    const auto producer =
        featureId("1");
    const auto first =
        surface(
            producer,
            part::FeatureSurfaceRoleKind::profile_cap);
    const auto second =
        surface(
            producer,
            part::FeatureSurfaceRoleKind::extent_cap);
    const auto address =
        curve(
            producer,
            first,
            second);
    const auto reference =
        referenceFor(
            producer,
            address);

    // Neutral kernel input is explicit/nonempty/unique and preserves semantic
    // authored order rather than sorting provider token values.
    {
        auto catalog =
            catalogFor(
                producer,
                address,
                kernel::ReferenceStatus::resolved,
                {kernel::RuntimeEdgeToken{77U}});
        const part::FilletFeature fillet{
            {reference},
            core::LengthValue{3.0}};
        const auto resolved =
            part::resolveKernelEdgeFeatureInput(
                fillet,
                &catalog);
        CHECK(resolved.ok());
        CHECK(
            resolved.input->operation ==
            kernel::EdgeFeatureOperation::fillet);
        CHECK(
            resolved.input->parameter_mm ==
            3.0);
        CHECK(
            resolved.input->edges ==
            std::vector<kernel::RuntimeEdgeToken>{
                kernel::RuntimeEdgeToken{77U}});

        const part::ChamferFeature chamfer{
            {reference},
            core::LengthValue{1.5}};
        const auto chamfer_input =
            part::resolveKernelEdgeFeatureInput(
                chamfer,
                &catalog);
        CHECK(chamfer_input.ok());
        CHECK(
            chamfer_input.input->operation ==
            kernel::EdgeFeatureOperation::chamfer);
        CHECK(
            chamfer_input.input->parameter_mm ==
            1.5);
    }

    // Whole-set resolution blocks before provider invocation on any unresolved
    // member and reports a deterministic canonical input index/status.
    {
        auto missing =
            catalogFor(
                producer,
                address,
                kernel::ReferenceStatus::missing,
                {});
        const part::FilletFeature feature{
            {reference},
            core::LengthValue{2.0}};
        const auto result =
            part::resolveKernelEdgeFeatureInput(
                feature,
                &missing);
        CHECK(!result.ok());
        CHECK(
            result.status ==
            part::EdgeFeatureKernelInputStatus::
                missing_edge);
        CHECK(
            result.failing_edge_input_index ==
            std::optional<std::size_t>{0U});
        CHECK(
            result.reference_status ==
            kernel::ReferenceStatus::missing);

        auto ambiguous =
            catalogFor(
                producer,
                address,
                kernel::ReferenceStatus::resolved,
                {
                    kernel::RuntimeEdgeToken{1U},
                    kernel::RuntimeEdgeToken{2U},
                });
        const auto split =
            part::resolveKernelEdgeFeatureInput(
                feature,
                &ambiguous);
        CHECK(!split.ok());
        CHECK(
            split.status ==
            part::EdgeFeatureKernelInputStatus::
                ambiguous_edge);

        CHECK(
            part::resolveKernelEdgeFeatureInput(
                feature,
                nullptr)
                .status ==
            part::EdgeFeatureKernelInputStatus::
                missing_upstream_body);
    }

    // T1 set comparison is independent from provider traversal order and
    // rejects growth/omission.
    {
        const std::vector<kernel::RuntimeEdgeToken>
            requested{
                {5U},
                {9U}};
        const kernel::EdgeFeatureInputMembership exact{
            {{9U}, {5U}}};
        CHECK(exact.valid());
        CHECK(exact.exactFor(requested));

        const kernel::EdgeFeatureInputMembership grown{
            {{9U}, {5U}, {12U}}};
        CHECK(grown.valid());
        CHECK(!grown.exactFor(requested));

        kernel::EdgeFeatureInput duplicate{
            kernel::EdgeFeatureOperation::fillet,
            {{5U}, {5U}},
            1.0};
        CHECK(!duplicate.valid());
    }

    // Ordered evaluation resolves the complete semantic set first. A resolved
    // set reaches the neutral kernel; provider failure is Failed, not Blocked.
    {
        auto fixture =
            makeEvalFixture(true);
        EdgeKernel kernel;
        const auto evaluation =
            part::evaluatePart(
                fixture.document,
                kernel);
        CHECK(evaluation.features.size() == 2U);
        CHECK(
            evaluation.features[0].status ==
            part::FeatureEvaluationStatus::
                up_to_date);
        CHECK(
            evaluation.features[1].status ==
            part::FeatureEvaluationStatus::
                failed);
        CHECK(
            evaluation.features[1].diagnostic ==
            part::FeatureEvaluationDiagnosticCode::
                kernel_provider_failure);
        CHECK(kernel.edge_calls == 1U);
        CHECK(kernel.edge_inputs.size() == 1U);
        CHECK(
            kernel.edge_inputs.front().edges ==
            std::vector<kernel::RuntimeEdgeToken>{
                kernel::RuntimeEdgeToken{41U}});
        CHECK(
            evaluation.body_status ==
            part::BodyEvaluationStatus::
                unavailable);
        CHECK(evaluation.resolved_prefix_solid != nullptr);
    }

    // Missing semantic input is Blocked and provider invocation count remains
    // zero. No first/nearest/geometry fallback is attempted.
    {
        auto fixture =
            makeEvalFixture(false);
        EdgeKernel kernel;
        const auto evaluation =
            part::evaluatePart(
                fixture.document,
                kernel);
        CHECK(evaluation.features.size() == 2U);
        CHECK(
            evaluation.features[1].status ==
            part::FeatureEvaluationStatus::
                blocked);
        CHECK(
            evaluation.features[1].diagnostic ==
            part::FeatureEvaluationDiagnosticCode::
                edge_reference_missing);
        CHECK(
            evaluation.features[1]
                .failing_edge_input_index ==
            std::optional<std::size_t>{0U});
        CHECK(
            evaluation.features[1]
                .edge_reference_status ==
            kernel::ReferenceStatus::missing);
        CHECK(kernel.edge_calls == 0U);
    }

    // A provider that reports geometric success but cannot prove exact T1
    // explicit membership is rejected before topology publication.
    {
        auto fixture =
            makeEvalFixture(true);
        EdgeKernel kernel{
            EdgeKernel::EdgeMode::
                fake_success_membership_mismatch};
        const auto evaluation =
            part::evaluatePart(
                fixture.document,
                kernel);
        CHECK(kernel.edge_calls == 1U);
        CHECK(
            evaluation.features[1].status ==
            part::FeatureEvaluationStatus::
                failed);
        CHECK(
            evaluation.features[1].diagnostic ==
            part::FeatureEvaluationDiagnosticCode::
                kernel_provider_mismatch);
        CHECK(
            evaluation.features[1].kernel_status ==
            kernel::SolidModelingStatus::
                provider_mismatch);
    }

    std::cout
        << "PM05C1_EDGE_SEMANTIC_KERNEL_PASS"
        << " whole_set_resolution=1"
        << " provider_call_on_unresolved=0"
        << " t1_exact_membership=1"
        << " occt_edge_operation=0"
        << " topology_publication=0\n";
    return EXIT_SUCCESS;
}
