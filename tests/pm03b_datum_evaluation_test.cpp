#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/part/datum_evaluation.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <cstdint>
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
            << "PM-03B Datum evaluation CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

enum class ExtentMode {
    resolved_plane,
    missing,
    ambiguous,
    non_planar,
};

class StageSolid final : public kernel::RuntimeSolid {
public:
    std::uint64_t next_face{1U};
    std::uint64_t next_surface{1U};
};

kernel::Frame3 offsetFrame(
    const kernel::Frame3& source,
    double offset) {
    auto result = source;
    result.origin.x +=
        result.normal.x * offset;
    result.origin.y +=
        result.normal.y * offset;
    result.origin.z +=
        result.normal.z * offset;
    return result;
}

class StageKernel final
    : public kernel::ISolidModelingKernel {
public:
    ExtentMode extent_mode{
        ExtentMode::resolved_plane};

    kernel::SolidModelingResult extrude(
        const kernel::LinearExtrudeInput& input,
        kernel::RuntimeSolidHandle upstream = {}) noexcept override {
        kernel::SolidModelingResult result;
        if (!input.valid() || upstream != nullptr) {
            result.status =
                kernel::SolidModelingStatus::
                    invalid_input;
            return result;
        }

        auto runtime =
            std::make_shared<StageSolid>();

        const auto publish_resolved =
            [&result, &runtime, &input](
                kernel::ExtrudeCapRole cap_role,
                double offset,
                kernel::SurfaceKind kind) {
                const kernel::RuntimeFaceToken face{
                    runtime->next_face++};
                const kernel::RuntimeSurfaceToken surface{
                    runtime->next_surface++};

                std::optional<kernel::Frame3> frame;
                if (kind ==
                    kernel::SurfaceKind::plane) {
                    frame =
                        offsetFrame(
                            input.profile.frame,
                            offset);
                }

                result.current_faces.push_back(face);

                const kernel::ExtrudeFaceRole role{
                    kernel::ExtrudeGeneratedFaceRoleKind::
                        cap,
                    cap_role,
                    std::nullopt};

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
                        role,
                        kernel::ReferenceStatus::
                            resolved,
                        kernel::ReferenceStatus::
                            resolved,
                        1U,
                        kind,
                        frame,
                        surface,
                        {face},
                    });
            };

        const auto publish_missing =
            [&result](
                kernel::ExtrudeCapRole cap_role) {
                const kernel::ExtrudeFaceRole role{
                    kernel::ExtrudeGeneratedFaceRoleKind::
                        cap,
                    cap_role,
                    std::nullopt};
                result.new_faces.push_back(
                    {
                        role,
                        kernel::ReferenceStatus::
                            missing,
                        0U,
                        std::nullopt,
                    });
                result.new_surfaces.push_back(
                    {
                        role,
                        kernel::ReferenceStatus::
                            missing,
                        kernel::ReferenceStatus::
                            missing,
                        0U,
                        kernel::SurfaceKind::plane,
                        std::nullopt,
                        std::nullopt,
                        {},
                    });
            };

        const auto publish_ambiguous =
            [&result, &runtime](
                kernel::ExtrudeCapRole cap_role) {
                const kernel::RuntimeFaceToken face{
                    runtime->next_face++};
                result.current_faces.push_back(face);

                const kernel::ExtrudeFaceRole role{
                    kernel::ExtrudeGeneratedFaceRoleKind::
                        cap,
                    cap_role,
                    std::nullopt};
                result.new_faces.push_back(
                    {
                        role,
                        kernel::ReferenceStatus::
                            ambiguous,
                        1U,
                        std::nullopt,
                    });
                result.new_surfaces.push_back(
                    {
                        role,
                        kernel::ReferenceStatus::
                            ambiguous,
                        kernel::ReferenceStatus::
                            ambiguous,
                        1U,
                        kernel::SurfaceKind::plane,
                        std::nullopt,
                        std::nullopt,
                        {face},
                    });
            };

        publish_resolved(
            input.start_cap_role,
            input.start_offset_mm,
            kernel::SurfaceKind::plane);

        switch (extent_mode) {
        case ExtentMode::resolved_plane:
            publish_resolved(
                input.end_cap_role,
                input.end_offset_mm,
                kernel::SurfaceKind::plane);
            break;
        case ExtentMode::missing:
            publish_missing(
                input.end_cap_role);
            break;
        case ExtentMode::ambiguous:
            publish_ambiguous(
                input.end_cap_role);
            break;
        case ExtentMode::non_planar:
            publish_resolved(
                input.end_cap_role,
                input.end_offset_mm,
                kernel::SurfaceKind::cylinder);
            break;
        }

        result.status =
            kernel::SolidModelingStatus::ok;
        result.solid = std::move(runtime);
        result.brep_valid = true;
        result.solid_count = 1U;
        result.face_count =
            result.current_faces.size();
        result.edge_count = 0U;
        result.vertex_count = 0U;
        return result;
    }
};

part::PlaneReference originPlane(
    core::BuiltinReferenceRole role) {
    return part::PlaneReference{
        part::BuiltinOriginPlaneReference{role}};
}

part::PlaneReference datumPlane(
    part::DatumId id) {
    return part::PlaneReference{
        part::DatumPlaneReference{id}};
}

part::PlaneReference bodySurface(
    part::SurfaceReference reference) {
    return part::PlaneReference{
        part::BodyPlanarSurfacePlaneReference{
            std::move(reference)}};
}

struct Fixture final {
    part::PartDocument document;
    part::FeatureId base_feature_id;
    part::DatumId xy_id;
    part::DatumId chained_origin_id;
    part::DatumId xz_id;
    part::DatumId yz_id;
    part::DatumId surface_id;
    part::DatumId chained_surface_id;
    part::BodyStageRef support_stage;
};

Fixture makeFixture() {
    auto document =
        part::PartDocument::create(
            core::DocumentId::generate());
    application::DocumentSession session{
        {},
        std::move(document)};

    const auto sketch_result =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::
                    xy_plane});
    CHECK(
        sketch_result.ok() &&
        sketch_result.sketch_id.has_value());

    CHECK(
        session.execute(
            application::AddSketchRectangleCommand{
                *sketch_result.sketch_id,
                session.document().revision(),
                {0.0, 0.0},
                {40.0, 20.0},
                sketch::EntityRole::regular,
                false})
            .ok());

    const auto* sketch =
        session.document().findSketch(
            *sketch_result.sketch_id);
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
                *sketch_result.sketch_id,
                session.document().revision(),
                *intent});
    CHECK(
        profile.ok() &&
        profile.profile_id.has_value());

    auto state =
        session.document().state();

    const auto base_feature =
        state.body.next_feature_id.allocate();
    CHECK(base_feature.has_value());

    const auto xy =
        state.next_datum_id.allocate();
    const auto chained_origin =
        state.next_datum_id.allocate();
    const auto xz =
        state.next_datum_id.allocate();
    const auto yz =
        state.next_datum_id.allocate();
    const auto surface =
        state.next_datum_id.allocate();
    const auto chained_surface =
        state.next_datum_id.allocate();
    CHECK(
        xy && chained_origin &&
        xz && yz && surface &&
        chained_surface);

    const part::BodyStageRef stage{
        part::BodyStageKind::after_feature,
        *base_feature};

    const part::SurfaceReference surface_reference{
        stage,
        part::FeatureSurfaceAddress{
            *base_feature,
            part::FeatureSurfaceRoleKind::
                extent_cap,
            std::nullopt,
            0U,
            0U,
            false}};
    CHECK(surface_reference.valid());

    state.datum_planes = {
        {
            *xy,
            originPlane(
                core::BuiltinReferenceRole::
                    xy_plane),
            core::LengthValue{10.0},
            true,
        },
        {
            *chained_origin,
            datumPlane(*xy),
            core::LengthValue{-2.0},
            true,
        },
        {
            *xz,
            originPlane(
                core::BuiltinReferenceRole::
                    xz_plane),
            core::LengthValue{10.0},
            true,
        },
        {
            *yz,
            originPlane(
                core::BuiltinReferenceRole::
                    yz_plane),
            core::LengthValue{10.0},
            true,
        },
        {
            *surface,
            bodySurface(surface_reference),
            core::LengthValue{5.0},
            true,
        },
        {
            *chained_surface,
            datumPlane(*surface),
            core::LengthValue{2.0},
            true,
        },
    };

    state.body.features = {
        part::PartFeature{
            *base_feature,
            "Base",
            false,
            part::ExtrudeFeature{
                *profile.profile_id,
                part::ExtrudeOperation::add,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{10.0},
                    false}}},
    };

    auto restored =
        part::PartDocument::restore(
            session.document().documentId(),
            std::move(state),
            session.document().revision());
    CHECK(restored.ok());

    return {
        std::move(*restored.document),
        *base_feature,
        *xy,
        *chained_origin,
        *xz,
        *yz,
        *surface,
        *chained_surface,
        stage,
    };
}

part::PartDocument withBaseDistance(
    const Fixture& fixture,
    double distance) {
    auto state =
        fixture.document.state();
    auto* extrude =
        std::get_if<part::ExtrudeFeature>(
            &state.body.features.front()
                 .definition);
    CHECK(extrude != nullptr);
    auto* extent =
        std::get_if<
            part::OneSidedExtrudeExtent>(
            &extrude->extent);
    CHECK(extent != nullptr);
    extent->distance =
        core::LengthValue{distance};

    auto restored =
        part::PartDocument::restore(
            fixture.document.documentId(),
            std::move(state),
            fixture.document.revision());
    CHECK(restored.ok());
    return std::move(*restored.document);
}

part::PartDocument withSuppressedBase(
    const Fixture& fixture) {
    auto state =
        fixture.document.state();
    state.body.features.front().suppressed =
        true;

    auto restored =
        part::PartDocument::restore(
            fixture.document.documentId(),
            std::move(state),
            fixture.document.revision());
    CHECK(restored.ok());
    return std::move(*restored.document);
}

const part::DatumPlaneEvaluation& requireDatum(
    const part::DatumEvaluation& evaluation,
    part::DatumId id) {
    const auto* datum =
        evaluation.find(id);
    CHECK(datum != nullptr);
    CHECK(datum->valid());
    return *datum;
}

void checkOriginFrames(
    const Fixture& fixture,
    const part::DatumEvaluation& evaluation) {
    const auto& xy =
        requireDatum(
            evaluation,
            fixture.xy_id);
    CHECK(
        xy.status ==
        part::DatumPlaneEvaluationStatus::
            resolved);
    CHECK(xy.frame.has_value());
    CHECK(xy.frame->origin.z == 10.0);
    CHECK(xy.frame->normal.z == 1.0);
    CHECK(!xy.body_stage_dependency);

    const auto& chained =
        requireDatum(
            evaluation,
            fixture.chained_origin_id);
    CHECK(chained.frame.has_value());
    CHECK(chained.frame->origin.z == 8.0);
    CHECK(!chained.body_stage_dependency);

    const auto& xz =
        requireDatum(
            evaluation,
            fixture.xz_id);
    CHECK(xz.frame.has_value());
    CHECK(xz.frame->origin.y == -10.0);
    CHECK(xz.frame->normal.y == -1.0);

    const auto& yz =
        requireDatum(
            evaluation,
            fixture.yz_id);
    CHECK(yz.frame.has_value());
    CHECK(yz.frame->origin.x == 10.0);
    CHECK(yz.frame->normal.x == 1.0);
}

void checkSurfaceFailure(
    const Fixture& fixture,
    ExtentMode mode,
    part::DatumPlaneEvaluationStatus
        expected_status,
    part::DatumPlaneEvaluationDiagnostic
        expected_diagnostic) {
    StageKernel kernel;
    kernel.extent_mode = mode;
    const auto part_evaluation =
        part::evaluatePart(
            fixture.document,
            kernel);
    CHECK(
        part_evaluation.body_status ==
        part::BodyEvaluationStatus::
            up_to_date);

    const auto datums =
        part::evaluateDatumPlanes(
            fixture.document,
            part_evaluation);
    CHECK(datums.valid());

    const auto& source =
        requireDatum(
            datums,
            fixture.surface_id);
    CHECK(source.status == expected_status);
    CHECK(
        source.diagnostic ==
        expected_diagnostic);
    CHECK(!source.frame);
    CHECK(source.body_stage_dependency);
    CHECK(
        *source.body_stage_dependency ==
        fixture.support_stage);

    const auto& chained =
        requireDatum(
            datums,
            fixture.chained_surface_id);
    CHECK(
        chained.status ==
        part::DatumPlaneEvaluationStatus::
            blocked);
    CHECK(
        chained.diagnostic ==
        part::DatumPlaneEvaluationDiagnostic::
            upstream_datum_unavailable);
    CHECK(!chained.frame);
    CHECK(chained.body_stage_dependency);
    CHECK(
        *chained.body_stage_dependency ==
        fixture.support_stage);
}

} // namespace

int main() {
    const auto fixture =
        makeFixture();
    const auto authored_before =
        fixture.document.state();

    StageKernel kernel;
    const auto part_evaluation =
        part::evaluatePart(
            fixture.document,
            kernel);
    CHECK(
        part_evaluation.body_status ==
        part::BodyEvaluationStatus::
            up_to_date);
    CHECK(part_evaluation.features.size() == 1U);
    CHECK(
        part_evaluation.features.front().status ==
        part::FeatureEvaluationStatus::
            up_to_date);
    CHECK(
        part_evaluation.features.front()
            .result_topology.has_value());

    const auto datums =
        part::evaluateDatumPlanes(
            fixture.document,
            part_evaluation);
    CHECK(datums.valid());
    CHECK(
        fixture.document.state() ==
        authored_before);

    checkOriginFrames(
        fixture,
        datums);

    const auto& surface =
        requireDatum(
            datums,
            fixture.surface_id);
    CHECK(
        surface.status ==
        part::DatumPlaneEvaluationStatus::
            resolved);
    CHECK(surface.frame.has_value());
    CHECK(surface.frame->origin.z == 15.0);
    CHECK(surface.body_stage_dependency);
    CHECK(
        *surface.body_stage_dependency ==
        fixture.support_stage);

    const auto& chained_surface =
        requireDatum(
            datums,
            fixture.chained_surface_id);
    CHECK(chained_surface.frame.has_value());
    CHECK(
        chained_surface.frame->origin.z ==
        17.0);
    CHECK(chained_surface.body_stage_dependency);
    CHECK(
        *chained_surface.body_stage_dependency ==
        fixture.support_stage);

    // A fresh evaluation from the same authored state reconstructs exactly
    // the same Datum semantic result without cached/runtime Datum state.
    StageKernel fresh_kernel;
    const auto cold_part =
        part::evaluatePart(
            fixture.document,
            fresh_kernel);
    const auto cold_datums =
        part::evaluateDatumPlanes(
            fixture.document,
            cold_part);
    CHECK(cold_datums == datums);

    // Moving the source Body Surface changes only the derived Datum world
    // frames. Datum IDs, source meaning and offsets remain authored.
    auto moved =
        withBaseDistance(
            fixture,
            20.0);
    StageKernel moved_kernel;
    const auto moved_part =
        part::evaluatePart(
            moved,
            moved_kernel);
    const auto moved_datums =
        part::evaluateDatumPlanes(
            moved,
            moved_part);
    CHECK(moved_datums.valid());

    const auto& moved_surface =
        requireDatum(
            moved_datums,
            fixture.surface_id);
    const auto& moved_chain =
        requireDatum(
            moved_datums,
            fixture.chained_surface_id);
    CHECK(moved_surface.frame->origin.z == 25.0);
    CHECK(moved_chain.frame->origin.z == 27.0);
    CHECK(
        moved.findDatumPlane(
            fixture.surface_id)->source ==
        fixture.document.findDatumPlane(
            fixture.surface_id)->source);
    CHECK(
        moved.findDatumPlane(
            fixture.surface_id)->offset ==
        fixture.document.findDatumPlane(
            fixture.surface_id)->offset);

    checkSurfaceFailure(
        fixture,
        ExtentMode::missing,
        part::DatumPlaneEvaluationStatus::
            missing,
        part::DatumPlaneEvaluationDiagnostic::
            missing_surface);
    checkSurfaceFailure(
        fixture,
        ExtentMode::ambiguous,
        part::DatumPlaneEvaluationStatus::
            ambiguous,
        part::DatumPlaneEvaluationDiagnostic::
            ambiguous_surface);
    checkSurfaceFailure(
        fixture,
        ExtentMode::non_planar,
        part::DatumPlaneEvaluationStatus::
            unsupported,
        part::DatumPlaneEvaluationDiagnostic::
            unsupported_non_planar);

    // An authored source stage that currently has no successful topology is
    // Blocked, not silently rebound or treated as a different Body stage.
    auto suppressed =
        withSuppressedBase(fixture);
    StageKernel suppressed_kernel;
    const auto suppressed_part =
        part::evaluatePart(
            suppressed,
            suppressed_kernel);
    const auto suppressed_datums =
        part::evaluateDatumPlanes(
            suppressed,
            suppressed_part);
    const auto& blocked_source =
        requireDatum(
            suppressed_datums,
            fixture.surface_id);
    CHECK(
        blocked_source.status ==
        part::DatumPlaneEvaluationStatus::
            blocked);
    CHECK(
        blocked_source.diagnostic ==
        part::DatumPlaneEvaluationDiagnostic::
            upstream_body_unavailable);
    CHECK(!blocked_source.frame);

    // Revision mismatch fails closed even for Origin-only Datum chains.
    const auto stale_revision =
        core::DocumentRevision{
            fixture.document.revision().value() +
            1U};
    const auto stale =
        part::evaluateDatumPlanes(
            fixture.document,
            stale_revision,
            part_evaluation.features);
    CHECK(stale.valid());
    CHECK(
        stale.planes.size() ==
        fixture.document.datumPlanes().size());
    for (const auto& datum : stale.planes) {
        CHECK(
            datum.status ==
            part::DatumPlaneEvaluationStatus::
                blocked);
        CHECK(
            datum.diagnostic ==
            part::DatumPlaneEvaluationDiagnostic::
                stale_part_evaluation);
        CHECK(!datum.frame);
    }

    std::cout
        << "PM03B_DATUM_EVALUATION_PASS"
        << " origin_frames=3"
        << " datum_chain=1"
        << " surface_stage=1"
        << " moved_source=1"
        << " missing=1"
        << " ambiguous=1"
        << " unsupported=1"
        << " blocked=1"
        << " stale=1"
        << " cold_deterministic=1"
        << '\n';

    return EXIT_SUCCESS;
}
