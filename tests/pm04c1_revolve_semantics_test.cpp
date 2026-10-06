#include <simplesolid2/core/document.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/part/axis.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/part/part_document.hpp>
#include <simplesolid2/part/profile.hpp>
#include <simplesolid2/sketch/entity_role.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <numbers>
#include <optional>
#include <utility>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-04C1 Revolve semantics CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

bool near(double lhs, double rhs) {
    return std::abs(lhs - rhs) < 1.0e-12;
}

part::PartCommitResult replaceState(
    part::PartDocument& document,
    part::PartAuthoredState state) {
    part::PartDocumentTransaction tx{document};
    tx.replaceState(std::move(state));
    return tx.commit();
}

part::AxisReference originAxis(
    core::BuiltinReferenceRole role) {
    return part::AxisReference{
        part::BuiltinOriginAxisReference{role}};
}

part::AxisReference authoredAxis(
    part::AxisId id) {
    return part::AxisReference{
        part::AuthoredAxisReference{id}};
}

part::RevolveExtent oneSide(
    double angle,
    bool reversed = false) {
    return part::OneSidedRevolveExtent{
        core::AngleValue{angle},
        reversed};
}

part::RevolveExtent midplane(double total_angle) {
    return part::MidplaneRevolveExtent{
        core::AngleValue{total_angle}};
}

struct Fixture final {
    part::PartDocument document;
    part::ProfileId profile_id;
    sketch::SketchId sketch_id;
    sketch::EntityId axis_line;
};

Fixture rectangleFixture(
    double min_u,
    double max_u,
    double min_v,
    double max_v) {
    auto document =
        part::PartDocument::create(
            core::DocumentId::generate());

    const auto support =
        part::partSketchSupportForBuiltinPlane(
            core::BuiltinReferenceRole::xy_plane);
    CHECK(support.has_value());

    sketch::SketchModel model;
    static_cast<void>(
        model.addLine(
            {min_u, min_v},
            {max_u, min_v}));
    static_cast<void>(
        model.addLine(
            {max_u, min_v},
            {max_u, max_v}));
    static_cast<void>(
        model.addLine(
            {max_u, max_v},
            {min_u, max_v}));
    static_cast<void>(
        model.addLine(
            {min_u, max_v},
            {min_u, min_v}));

    // Construction geometry remains excluded from Profile generation but is
    // a legal authored Axis source.
    const auto axis_line =
        model.addLine(
            {0.0, -50.0},
            {0.0, 50.0},
            sketch::EntityRole::construction);

    const auto analysis =
        sketch::analyzeRegions(model);
    CHECK(analysis.complete());
    CHECK(analysis.regions.size() == 1U);
    const auto intent =
        part::makeProfileRegionIntent(
            analysis.regions.front());
    CHECK(intent.has_value());

    auto state = document.state();
    const auto profile_id =
        state.next_profile_id.allocate();
    CHECK(profile_id.has_value());
    const auto sketch_id =
        sketch::SketchId::generate();

    state.sketches.push_back(
        part::PartSketch{
            sketch_id,
            *support,
            true,
            std::move(model)});
    state.profiles.push_back(
        part::PartProfile{
            *profile_id,
            sketch_id,
            "Profile",
            part::ProfileVisibilityPolicy::automatic,
            *intent});
    const auto committed =
        replaceState(document, std::move(state));
    CHECK(committed.ok());
    CHECK(committed.changed);

    return {
        std::move(document),
        *profile_id,
        sketch_id,
        axis_line};
}

part::AxisId addAuthoredAxis(Fixture& fixture) {
    auto state = fixture.document.state();
    const auto id =
        state.next_axis_id.allocate();
    CHECK(id.has_value());
    state.axes.push_back(
        part::PartAxis{
            *id,
            "Axis001",
            {fixture.sketch_id,
             fixture.axis_line},
            true});
    const auto committed =
        replaceState(
            fixture.document,
            std::move(state));
    CHECK(committed.ok());
    return *id;
}

part::FeatureId addRevolve(
    part::PartDocument& document,
    part::RevolveFeature definition) {
    auto state = document.state();
    const auto id =
        state.body.next_feature_id.allocate();
    CHECK(id.has_value());
    state.body.features.push_back(
        part::PartFeature{
            *id,
            "Revolve",
            false,
            std::move(definition)});
    const auto committed =
        replaceState(document, std::move(state));
    CHECK(committed.ok());
    return *id;
}

class CaptureKernel final
    : public kernel::ISolidModelingKernel {
public:
    int extrude_calls{};
    int revolve_calls{};
    std::optional<kernel::AngularRevolveInput>
        last_revolve;

    kernel::SolidModelingResult extrude(
        const kernel::LinearExtrudeInput&,
        kernel::RuntimeSolidHandle = {}) noexcept override {
        ++extrude_calls;
        kernel::SolidModelingResult result;
        result.status =
            kernel::SolidModelingStatus::
                provider_failure;
        return result;
    }

    kernel::SolidModelingResult revolve(
        const kernel::AngularRevolveInput& input,
        kernel::RuntimeSolidHandle = {}) noexcept override {
        ++revolve_calls;
        last_revolve = input;
        kernel::SolidModelingResult result;
        result.status =
            kernel::SolidModelingStatus::
                provider_failure;
        return result;
    }
};

} // namespace

int main() {
    constexpr double pi =
        std::numbers::pi_v<double>;
    constexpr double full_turn =
        2.0 * std::numbers::pi_v<double>;

    // Authored parameter contract: Add/Cut, OneSide/Midplane and
    // 0 < Angle <= 360 degrees.
    {
        auto fixture =
            rectangleFixture(
                10.0, 20.0, -5.0, 5.0);
        part::RevolveFeature valid{
            fixture.profile_id,
            originAxis(
                core::BuiltinReferenceRole::y_axis),
            part::RevolveOperation::add,
            oneSide(pi * 0.5)};
        CHECK(
            part::revolveFeatureStructurallyValid(
                valid));

        auto zero = valid;
        zero.extent = oneSide(0.0);
        CHECK(
            !part::revolveFeatureStructurallyValid(
                zero));

        auto negative = valid;
        negative.extent = oneSide(-0.1);
        CHECK(
            !part::revolveFeatureStructurallyValid(
                negative));

        auto over = valid;
        over.extent =
            midplane(full_turn + 0.01);
        CHECK(
            !part::revolveFeatureStructurallyValid(
                over));

        auto full = valid;
        full.extent = midplane(full_turn);
        CHECK(
            part::revolveFeatureStructurallyValid(
                full));
    }

    // OneSide forward/reverse and Midplane map to one provider-neutral
    // angular interval. Origin Y lies in XY and the Profile is wholly x>0.
    auto positive =
        rectangleFixture(
            10.0, 20.0, -5.0, 5.0);
    {
        part::RevolveFeature feature{
            positive.profile_id,
            originAxis(
                core::BuiltinReferenceRole::y_axis),
            part::RevolveOperation::add,
            oneSide(pi * 0.5)};
        auto resolved =
            part::resolveKernelRevolveInput(
                positive.document,
                feature,
                nullptr,
                nullptr);
        CHECK(resolved.ok());
        CHECK(resolved.input.has_value());
        CHECK(
            near(
                resolved.input->
                    start_angle_radians,
                0.0));
        CHECK(
            near(
                resolved.input->
                    end_angle_radians,
                pi * 0.5));
        CHECK(!resolved.input->fullTurn());

        feature.extent =
            oneSide(pi * 0.5, true);
        resolved =
            part::resolveKernelRevolveInput(
                positive.document,
                feature,
                nullptr,
                nullptr);
        CHECK(resolved.ok());
        CHECK(
            near(
                resolved.input->
                    start_angle_radians,
                0.0));
        CHECK(
            near(
                resolved.input->
                    end_angle_radians,
                -pi * 0.5));

        feature.extent =
            midplane(2.0 * pi / 3.0);
        resolved =
            part::resolveKernelRevolveInput(
                positive.document,
                feature,
                nullptr,
                nullptr);
        CHECK(resolved.ok());
        CHECK(
            near(
                resolved.input->
                    start_angle_radians,
                -pi / 3.0));
        CHECK(
            near(
                resolved.input->
                    end_angle_radians,
                pi / 3.0));

        feature.extent =
            midplane(full_turn);
        resolved =
            part::resolveKernelRevolveInput(
                positive.document,
                feature,
                nullptr,
                nullptr);
        CHECK(resolved.ok());
        CHECK(resolved.input->fullTurn());
    }

    // Merely intersecting the support plane is not enough: Origin Z is
    // non-coplanar with an XY Profile.
    {
        part::RevolveFeature feature{
            positive.profile_id,
            originAxis(
                core::BuiltinReferenceRole::z_axis),
            part::RevolveOperation::add,
            oneSide(pi)};
        const auto resolved =
            part::resolveKernelRevolveInput(
                positive.document,
                feature,
                nullptr,
                nullptr);
        CHECK(!resolved.ok());
        CHECK(
            resolved.status ==
            part::RevolveKernelInputStatus::
                axis_not_in_profile_plane);
    }

    // Material interior on both sides of the axis is rejected.
    {
        auto crossing =
            rectangleFixture(
                -5.0, 5.0, 2.0, 12.0);
        part::RevolveFeature feature{
            crossing.profile_id,
            originAxis(
                core::BuiltinReferenceRole::y_axis),
            part::RevolveOperation::add,
            oneSide(pi)};
        const auto resolved =
            part::resolveKernelRevolveInput(
                crossing.document,
                feature,
                nullptr,
                nullptr);
        CHECK(!resolved.ok());
        CHECK(
            resolved.status ==
            part::RevolveKernelInputStatus::
                profile_crosses_axis);
    }

    // Closed-half-plane semantics allow the Profile to touch the Axis.
    {
        auto touching =
            rectangleFixture(
                0.0, 10.0, 2.0, 12.0);
        part::RevolveFeature feature{
            touching.profile_id,
            originAxis(
                core::BuiltinReferenceRole::y_axis),
            part::RevolveOperation::add,
            oneSide(pi)};
        CHECK(
            part::resolveKernelRevolveInput(
                touching.document,
                feature,
                nullptr,
                nullptr)
                .ok());
    }

    // A normal Sketch Line becomes a Part-owned AxisReference without
    // conversion. Same support identity proves coplanarity independently of
    // Viewer/provider representation.
    {
        auto authored =
            rectangleFixture(
                10.0, 20.0, -5.0, 5.0);
        const auto axis_id =
            addAuthoredAxis(authored);
        part::RevolveFeature feature{
            authored.profile_id,
            authoredAxis(axis_id),
            part::RevolveOperation::add,
            oneSide(pi)};
        auto resolved =
            part::resolveKernelRevolveInput(
                authored.document,
                feature,
                nullptr,
                nullptr);
        CHECK(resolved.ok());
        CHECK(!resolved.required_axis_stage);

        // Delete Axis preserves the consumer reference as repairable Missing.
        auto state = authored.document.state();
        state.axes.clear();
        CHECK(
            replaceState(
                authored.document,
                std::move(state))
                .ok());
        resolved =
            part::resolveKernelRevolveInput(
                authored.document,
                feature,
                nullptr,
                nullptr);
        CHECK(!resolved.ok());
        CHECK(
            resolved.status ==
            part::RevolveKernelInputStatus::
                missing_axis);
    }

    // A syntactically valid but never-allocated AxisId is not legal authored
    // consumer intent. Deleted allocated IDs remain legal because the cursor
    // preserves their history.
    {
        auto invalid =
            rectangleFixture(
                10.0, 20.0, -5.0, 5.0);
        auto state = invalid.document.state();
        const auto feature_id =
            state.body.next_feature_id.allocate();
        CHECK(feature_id.has_value());
        const auto outside =
            part::AxisId::parse("999");
        CHECK(outside.has_value());
        state.body.features.push_back(
            part::PartFeature{
                *feature_id,
                "Invalid Revolve",
                false,
                part::RevolveFeature{
                    invalid.profile_id,
                    authoredAxis(*outside),
                    part::RevolveOperation::add,
                    oneSide(pi)}});
        const auto committed =
            replaceState(
                invalid.document,
                std::move(state));
        CHECK(!committed.ok());
        CHECK(
            committed.code ==
            part::PartCommitErrorCode::
                invalid_state);
    }

    // Ordered evaluator dispatches Revolve to the neutral kernel method, not
    // Extrude. Provider failure remains distinct from reference/admission
    // failures.
    {
        auto evaluated =
            rectangleFixture(
                10.0, 20.0, -5.0, 5.0);
        const auto feature_id =
            addRevolve(
                evaluated.document,
                part::RevolveFeature{
                    evaluated.profile_id,
                    originAxis(
                        core::BuiltinReferenceRole::
                            y_axis),
                    part::RevolveOperation::add,
                    oneSide(pi * 0.5)});

        CaptureKernel kernel;
        const auto result =
            part::evaluatePart(
                evaluated.document,
                kernel);
        CHECK(kernel.extrude_calls == 0);
        CHECK(kernel.revolve_calls == 1);
        CHECK(kernel.last_revolve.has_value());
        CHECK(
            near(
                kernel.last_revolve->
                    end_angle_radians,
                pi * 0.5));
        CHECK(result.features.size() == 1U);
        CHECK(
            result.features.front().feature_id ==
            feature_id);
        CHECK(
            result.features.front().status ==
            part::FeatureEvaluationStatus::
                failed);
        CHECK(
            result.features.front().diagnostic ==
            part::FeatureEvaluationDiagnosticCode::
                kernel_provider_failure);
        CHECK(
            result.body_status ==
            part::BodyEvaluationStatus::
                unavailable);
    }

    // Cut cannot be the first solid-producing Feature and therefore never
    // reaches the provider.
    {
        auto cut =
            rectangleFixture(
                10.0, 20.0, -5.0, 5.0);
        addRevolve(
            cut.document,
            part::RevolveFeature{
                cut.profile_id,
                originAxis(
                    core::BuiltinReferenceRole::
                        y_axis),
                part::RevolveOperation::cut,
                oneSide(pi)});

        CaptureKernel kernel;
        const auto result =
            part::evaluatePart(
                cut.document,
                kernel);
        CHECK(kernel.revolve_calls == 0);
        CHECK(
            result.features.front().status ==
            part::FeatureEvaluationStatus::
                blocked);
        CHECK(
            result.features.front().diagnostic ==
            part::FeatureEvaluationDiagnosticCode::
                missing_upstream_body);
    }

    // Bounded stage-cycle rule: Axis source Sketch supported by the same
    // Feature stage that consumes the Axis is structurally rejected.
    {
        auto cycle =
            rectangleFixture(
                10.0, 20.0, -5.0, 5.0);
        auto state = cycle.document.state();
        const auto feature_id =
            state.body.next_feature_id.allocate();
        const auto axis_id =
            state.next_axis_id.allocate();
        CHECK(feature_id.has_value());
        CHECK(axis_id.has_value());

        part::FeatureSurfaceAddress surface;
        surface.producer_feature_id =
            *feature_id;
        surface.role =
            part::FeatureSurfaceRoleKind::
                revolve_start_cap;
        const part::SurfaceReference reference{
            part::BodyStageRef{
                part::BodyStageKind::
                    after_feature,
                *feature_id},
            surface};
        const auto support =
            part::partSketchSupportForBodyPlanarSurface(
                reference);
        CHECK(support.has_value());

        sketch::SketchModel axis_model;
        const auto line =
            axis_model.addLine(
                {0.0, 0.0},
                {0.0, 10.0},
                sketch::EntityRole::regular);
        const auto axis_sketch =
            sketch::SketchId::generate();
        state.sketches.push_back(
            part::PartSketch{
                axis_sketch,
                *support,
                true,
                std::move(axis_model)});
        state.axes.push_back(
            part::PartAxis{
                *axis_id,
                "Cyclic Axis",
                {axis_sketch, line},
                true});
        state.body.features.push_back(
            part::PartFeature{
                *feature_id,
                "Cyclic Revolve",
                false,
                part::RevolveFeature{
                    cycle.profile_id,
                    authoredAxis(*axis_id),
                    part::RevolveOperation::add,
                    oneSide(pi * 0.5)}});

        const auto committed =
            replaceState(
                cycle.document,
                std::move(state));
        CHECK(!committed.ok());
        CHECK(
            committed.code ==
            part::PartCommitErrorCode::
                invalid_state);
    }

    std::cout
        << "PM-04C1 Revolve semantic/kernel contract tests passed\n";
    return EXIT_SUCCESS;
}
