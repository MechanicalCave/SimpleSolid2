#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/part/profile.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <numbers>
#include <optional>
#include <utility>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-04C1 Revolve semantic/kernel CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

constexpr double pi =
    std::numbers::pi_v<double>;
constexpr double full_turn =
    2.0 * pi;

bool near(double a, double b) {
    return std::abs(a - b) < 1.0e-12;
}

part::AxisReference originAxis(
    core::BuiltinReferenceRole role) {
    return part::AxisReference{
        part::BuiltinOriginAxisReference{
            role}};
}

part::AxisReference authoredAxis(
    part::AxisId id) {
    return part::AxisReference{
        part::AuthoredAxisReference{id}};
}

class FakeSolid final
    : public kernel::RuntimeSolid {};

class RevolveKernel final
    : public kernel::ISolidModelingKernel {
public:
    std::vector<kernel::AngularRevolveInput>
        revolve_inputs;

    kernel::SolidModelingResult extrude(
        const kernel::LinearExtrudeInput&,
        kernel::RuntimeSolidHandle = {}) noexcept override {
        kernel::SolidModelingResult result;
        result.status =
            kernel::SolidModelingStatus::
                provider_failure;
        return result;
    }

    kernel::SolidModelingResult revolve(
        const kernel::AngularRevolveInput& input,
        kernel::RuntimeSolidHandle upstream = {}) noexcept override {
        kernel::SolidModelingResult result;
        if (!input.valid()) {
            result.status =
                kernel::SolidModelingStatus::
                    invalid_input;
            return result;
        }
        if (input.operation ==
                kernel::SolidBooleanOperation::cut &&
            upstream == nullptr) {
            result.status =
                kernel::SolidModelingStatus::
                    missing_upstream;
            return result;
        }

        revolve_inputs.push_back(input);

        auto runtime =
            std::make_shared<FakeSolid>();
        std::uint64_t next_face = 1U;
        std::uint64_t next_surface = 1U;

        auto publish =
            [&](kernel::GeneratedFaceRole role,
                kernel::SurfaceKind kind,
                std::optional<kernel::Frame3> frame =
                    std::nullopt) {
                const kernel::RuntimeFaceToken face{
                    next_face++};
                const kernel::RuntimeSurfaceToken surface{
                    next_surface++};
                result.current_faces.push_back(face);
                result.new_faces.push_back(
                    {
                        role,
                        kernel::ReferenceStatus::
                            resolved,
                        1U,
                        face,
                    });
                result.new_surfaces.push_back(
                    {
                        std::move(role),
                        kernel::ReferenceStatus::
                            resolved,
                        kernel::ReferenceStatus::
                            resolved,
                        1U,
                        kind,
                        std::move(frame),
                        surface,
                        {face},
                    });
            };

        if (!input.fullTurn()) {
            publish(
                {
                    kernel::GeneratedFaceRoleKind::
                        revolve_start_cap,
                    std::nullopt,
                    std::nullopt,
                },
                kernel::SurfaceKind::plane,
                input.profile.frame);
            publish(
                {
                    kernel::GeneratedFaceRoleKind::
                        revolve_end_cap,
                    std::nullopt,
                    std::nullopt,
                },
                kernel::SurfaceKind::plane,
                input.profile.frame);
        }

        const auto publish_loop =
            [&](const kernel::ProfileLoopInput& loop) {
                for (const auto& use :
                     loop.boundary) {
                    publish(
                        {
                            kernel::GeneratedFaceRoleKind::
                                revolve_side,
                            std::nullopt,
                            use.provenance,
                        },
                        kernel::SurfaceKind::other);
                }
            };
        publish_loop(input.profile.outer);
        for (const auto& hole : input.profile.holes) {
            publish_loop(hole);
        }

        // Full-turn provider seam evidence is runtime-only. It is deliberately
        // not given a semantic Curve relation.
        if (input.fullTurn()) {
            const kernel::RuntimeEdgeToken seam{1U};
            result.current_edges.push_back(seam);
            result.current_edge_semantics.push_back(
                {
                    seam,
                    kernel::CurveKind::circle,
                    true,
                    {},
                    false,
                });
        }

        result.status =
            kernel::SolidModelingStatus::ok;
        result.solid = std::move(runtime);
        result.brep_valid = true;
        result.solid_count = 1U;
        result.face_count =
            result.current_faces.size();
        result.edge_count =
            result.current_edges.size();
        result.vertex_count = 0U;
        return result;
    }
};

struct Fixture final {
    part::PartDocument document;
    part::ProfileId profile_id;
    sketch::SketchId sketch_id;
    sketch::EntityId axis_line_id;
};

Fixture makeFixture(
    sketch::Point2 first,
    sketch::Point2 opposite) {
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
    CHECK(rectangle.entity_ids.size() == 4U);

    // Construction Line is available as an authored Axis source without
    // changing Profile/region generation.
    const auto axis_line =
        session.execute(
            application::AddSketchLineCommand{
                sketch_id,
                {0.0, 0.0},
                {30.0, 0.0},
                sketch::EntityRole::
                    construction});
    CHECK(
        axis_line.ok() &&
        axis_line.entity_id);

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

    auto restored =
        part::PartDocument::restore(
            session.document().documentId(),
            session.document().state(),
            session.document().revision());
    CHECK(restored.ok());

    return {
        std::move(*restored.document),
        *profile.profile_id,
        sketch_id,
        *axis_line.entity_id};
}

std::pair<part::PartDocument, part::AxisId>
withAuthoredAxis(
    const Fixture& fixture) {
    auto state = fixture.document.state();
    const auto axis_id =
        state.next_axis_id.allocate();
    CHECK(axis_id.has_value());
    state.axes.push_back(
        part::PartAxis{
            *axis_id,
            "Axis001",
            {
                fixture.sketch_id,
                fixture.axis_line_id,
            },
            true});

    auto restored =
        part::PartDocument::restore(
            fixture.document.documentId(),
            std::move(state),
            fixture.document.revision());
    CHECK(restored.ok());
    return {
        std::move(*restored.document),
        *axis_id};
}

part::PartDocument withRevolve(
    const part::PartDocument& source,
    part::RevolveFeature revolve) {
    auto state = source.state();
    const auto feature_id =
        state.body.next_feature_id.allocate();
    CHECK(feature_id.has_value());
    state.body.features.push_back(
        part::PartFeature{
            *feature_id,
            "Revolve001",
            false,
            std::move(revolve)});

    auto restored =
        part::PartDocument::restore(
            source.documentId(),
            std::move(state),
            source.revision());
    CHECK(restored.ok());
    return std::move(*restored.document);
}

std::size_t countSurfaceRole(
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

} // namespace

int main() {
    // Authored angle contract is bounded to one turn.
    {
        const auto fixture =
            makeFixture(
                {10.0, 10.0},
                {20.0, 20.0});

        part::RevolveFeature feature{
            fixture.profile_id,
            originAxis(
                core::BuiltinReferenceRole::
                    x_axis),
            part::RevolveOperation::add,
            part::OneSidedRevolveExtent{
                core::AngleValue{pi / 2.0},
                false}};
        CHECK(
            part::revolveFeatureStructurallyValid(
                feature));

        feature.extent =
            part::OneSidedRevolveExtent{
                core::AngleValue{0.0},
                false};
        CHECK(
            !part::revolveFeatureStructurallyValid(
                feature));

        feature.extent =
            part::MidplaneRevolveExtent{
                core::AngleValue{full_turn}};
        CHECK(
            part::revolveFeatureStructurallyValid(
                feature));

        feature.extent =
            part::MidplaneRevolveExtent{
                core::AngleValue{
                    full_turn + 0.01}};
        CHECK(
            !part::revolveFeatureStructurallyValid(
                feature));
    }

    // Origin Axis admission and OneSide/Midplane angular mapping.
    {
        const auto fixture =
            makeFixture(
                {10.0, 10.0},
                {20.0, 20.0});

        part::RevolveFeature forward{
            fixture.profile_id,
            originAxis(
                core::BuiltinReferenceRole::
                    x_axis),
            part::RevolveOperation::add,
            part::OneSidedRevolveExtent{
                core::AngleValue{pi / 2.0},
                false}};
        auto resolved =
            part::resolveKernelRevolveInput(
                fixture.document,
                forward,
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
                pi / 2.0));
        CHECK(
            resolved.input->operation ==
            kernel::SolidBooleanOperation::add);

        auto reverse = forward;
        reverse.extent =
            part::OneSidedRevolveExtent{
                core::AngleValue{pi / 2.0},
                true};
        resolved =
            part::resolveKernelRevolveInput(
                fixture.document,
                reverse,
                nullptr,
                nullptr);
        CHECK(resolved.ok());
        CHECK(
            near(
                resolved.input->
                    end_angle_radians,
                -pi / 2.0));

        auto midplane = forward;
        midplane.operation =
            part::RevolveOperation::cut;
        midplane.extent =
            part::MidplaneRevolveExtent{
                core::AngleValue{pi}};
        resolved =
            part::resolveKernelRevolveInput(
                fixture.document,
                midplane,
                nullptr,
                nullptr);
        CHECK(resolved.ok());
        CHECK(
            near(
                resolved.input->
                    start_angle_radians,
                -pi / 2.0));
        CHECK(
            near(
                resolved.input->
                    end_angle_radians,
                pi / 2.0));
        CHECK(
            resolved.input->operation ==
            kernel::SolidBooleanOperation::cut);

        auto skew = forward;
        skew.axis =
            originAxis(
                core::BuiltinReferenceRole::
                    z_axis);
        const auto unsupported =
            part::resolveKernelRevolveInput(
                fixture.document,
                skew,
                nullptr,
                nullptr);
        CHECK(!unsupported.ok());
        CHECK(
            unsupported.status ==
            part::RevolveKernelInputStatus::
                axis_not_in_profile_plane);
    }

    // Closed-half-plane admission: touching the Axis is legal; material
    // interior on both sides is rejected.
    {
        const auto touching =
            makeFixture(
                {10.0, 0.0},
                {20.0, 10.0});
        const part::RevolveFeature feature{
            touching.profile_id,
            originAxis(
                core::BuiltinReferenceRole::
                    x_axis),
            part::RevolveOperation::add,
            part::OneSidedRevolveExtent{
                core::AngleValue{pi / 2.0},
                false}};
        CHECK(
            part::resolveKernelRevolveInput(
                touching.document,
                feature,
                nullptr,
                nullptr)
                .ok());

        const auto crossing =
            makeFixture(
                {10.0, -10.0},
                {20.0, 10.0});
        auto crossing_feature = feature;
        crossing_feature.profile_id =
            crossing.profile_id;
        const auto rejected =
            part::resolveKernelRevolveInput(
                crossing.document,
                crossing_feature,
                nullptr,
                nullptr);
        CHECK(!rejected.ok());
        CHECK(
            rejected.status ==
            part::RevolveKernelInputStatus::
                profile_crosses_axis);
    }

    // Authored AxisReference resolves from SketchId+EntityId; removing the
    // Axis object leaves the durable reference as explicit MissingAxis.
    {
        const auto fixture =
            makeFixture(
                {10.0, 10.0},
                {20.0, 20.0});
        auto [document, axis_id] =
            withAuthoredAxis(fixture);

        const part::RevolveFeature feature{
            fixture.profile_id,
            authoredAxis(axis_id),
            part::RevolveOperation::add,
            part::OneSidedRevolveExtent{
                core::AngleValue{pi / 2.0},
                false}};
        const auto resolved =
            part::resolveKernelRevolveInput(
                document,
                feature,
                nullptr,
                nullptr);
        CHECK(resolved.ok());
        CHECK(
            near(
                resolved.input->axis.direction.x,
                1.0));
        CHECK(
            near(
                resolved.input->axis.direction.y,
                0.0));
        CHECK(
            near(
                resolved.input->axis.direction.z,
                0.0));

        auto missing_state = document.state();
        missing_state.axes.clear();
        auto missing =
            part::PartDocument::restore(
                document.documentId(),
                std::move(missing_state),
                document.revision());
        CHECK(missing.ok());
        const auto unresolved =
            part::resolveKernelRevolveInput(
                *missing.document,
                feature,
                nullptr,
                nullptr);
        CHECK(!unresolved.ok());
        CHECK(
            unresolved.status ==
            part::RevolveKernelInputStatus::
                missing_axis);
    }

    // A syntactically valid but never-allocated AxisId cannot become
    // durable Revolve intent. Deleted previously allocated IDs remain legal
    // because AxisId high-water preserves their allocation history.
    {
        const auto fixture =
            makeFixture(
                {10.0, 10.0},
                {20.0, 20.0});
        auto state = fixture.document.state();
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
                    fixture.profile_id,
                    authoredAxis(*outside),
                    part::RevolveOperation::add,
                    part::OneSidedRevolveExtent{
                        core::AngleValue{
                            pi / 2.0},
                        false}}});

        const auto invalid =
            part::PartDocument::restore(
                fixture.document.documentId(),
                std::move(state),
                fixture.document.revision());
        CHECK(!invalid.ok());
        CHECK(
            invalid.code ==
            part::PartReconstructErrorCode::
                invalid_state);
    }

    // Bounded ordered-history cycle rule: an authored Axis sourced from a
    // Sketch supported by the same Body stage that consumes that Axis is
    // structurally invalid. No global dependency graph is introduced.
    {
        const auto fixture =
            makeFixture(
                {10.0, 10.0},
                {20.0, 20.0});
        auto state = fixture.document.state();
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
                {10.0, 0.0},
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
                    fixture.profile_id,
                    authoredAxis(*axis_id),
                    part::RevolveOperation::add,
                    part::OneSidedRevolveExtent{
                        core::AngleValue{
                            pi / 2.0},
                        false}}});

        const auto cyclic =
            part::PartDocument::restore(
                fixture.document.documentId(),
                std::move(state),
                fixture.document.revision());
        CHECK(!cyclic.ok());
        CHECK(
            cyclic.code ==
            part::PartReconstructErrorCode::
                invalid_state);
    }

    // Partial Revolve participates in the normal ordered Body evaluator and
    // maps provider-neutral start/end/side provenance into semantic topology.
    {
        const auto fixture =
            makeFixture(
                {10.0, 10.0},
                {20.0, 20.0});
        auto document =
            withRevolve(
                fixture.document,
                part::RevolveFeature{
                    fixture.profile_id,
                    originAxis(
                        core::BuiltinReferenceRole::
                            x_axis),
                    part::RevolveOperation::add,
                    part::OneSidedRevolveExtent{
                        core::AngleValue{
                            pi / 2.0},
                        false}});

        RevolveKernel kernel;
        const auto evaluated =
            part::evaluatePart(
                document,
                kernel);
        CHECK(
            evaluated.body_status ==
            part::BodyEvaluationStatus::
                up_to_date);
        CHECK(evaluated.features.size() == 1U);
        CHECK(
            evaluated.features.front().status ==
            part::FeatureEvaluationStatus::
                up_to_date);
        CHECK(kernel.revolve_inputs.size() == 1U);

        const auto& feature =
            evaluated.features.front();
        CHECK(
            countSurfaceRole(
                feature,
                part::FeatureSurfaceRoleKind::
                    revolve_start_cap) == 1U);
        CHECK(
            countSurfaceRole(
                feature,
                part::FeatureSurfaceRoleKind::
                    revolve_end_cap) == 1U);
        CHECK(
            countSurfaceRole(
                feature,
                part::FeatureSurfaceRoleKind::
                    revolve_side) == 4U);
        CHECK(feature.result_topology.has_value());
        CHECK(feature.result_topology->complete());
    }

    // Full turn has no authored start/end cap semantics. A provider periodic
    // seam is completely accounted but remains a non-referenceable
    // representation artifact.
    {
        const auto fixture =
            makeFixture(
                {10.0, 10.0},
                {20.0, 20.0});
        auto document =
            withRevolve(
                fixture.document,
                part::RevolveFeature{
                    fixture.profile_id,
                    originAxis(
                        core::BuiltinReferenceRole::
                            x_axis),
                    part::RevolveOperation::add,
                    part::MidplaneRevolveExtent{
                        core::AngleValue{
                            full_turn}}});

        RevolveKernel kernel;
        const auto evaluated =
            part::evaluatePart(
                document,
                kernel);
        CHECK(
            evaluated.body_status ==
            part::BodyEvaluationStatus::
                up_to_date);
        CHECK(evaluated.features.size() == 1U);
        const auto& feature =
            evaluated.features.front();
        CHECK(
            feature.status ==
            part::FeatureEvaluationStatus::
                up_to_date);
        CHECK(
            countSurfaceRole(
                feature,
                part::FeatureSurfaceRoleKind::
                    revolve_start_cap) == 0U);
        CHECK(
            countSurfaceRole(
                feature,
                part::FeatureSurfaceRoleKind::
                    revolve_end_cap) == 0U);
        CHECK(
            countSurfaceRole(
                feature,
                part::FeatureSurfaceRoleKind::
                    revolve_side) == 4U);
        CHECK(feature.result_topology.has_value());
        CHECK(
            feature.result_topology->
                edges.size() == 1U);
        const auto& seam =
            feature.result_topology->
                edges.front();
        CHECK(seam.periodic_seam);
        CHECK(
            seam.accounting_class ==
            part::TopologyAccountingClass::
                known_representation_artifact);
        CHECK(
            seam.referenceability ==
            kernel::ReferenceStatus::
                unsupported);
        CHECK(
            seam.curve_candidates.empty());
    }

    std::cout
        << "PM-04C1 Revolve semantic/kernel tests passed\n";
    return EXIT_SUCCESS;
}
