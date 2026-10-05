#include <simplesolid2/core/document.hpp>
#include <simplesolid2/part/datum_evaluation.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/part/part_document.hpp>
#include <simplesolid2/part/profile.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <cstdlib>
#include <iostream>
#include <optional>
#include <utility>

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

part::OffsetDatumPlane makeDatum(
    part::DatumId id,
    part::PlaneReference source,
    double offset_mm) {
    return part::OffsetDatumPlane{
        id,
        std::move(source),
        core::LengthValue{offset_mm},
        true};
}

struct Fixture final {
    part::PartDocument document;
    part::FeatureId feature_id;
    part::FeatureSurfaceAddress surface_address;
    part::BodyStageRef stage;
    part::DatumId xy_datum;
    part::DatumId xz_datum;
    part::DatumId yz_datum;
    part::DatumId zero_datum;
    part::DatumId surface_datum;
    part::DatumId chained_datum;
};

Fixture makeFixture() {
    auto document =
        part::PartDocument::create(
            core::DocumentId::generate());
    auto state = document.state();

    sketch::SketchModel model;
    CHECK(
        model.addLine(
            {0.0, 0.0},
            {20.0, 0.0})
            .valid());
    CHECK(
        model.addLine(
            {20.0, 0.0},
            {20.0, 10.0})
            .valid());
    CHECK(
        model.addLine(
            {20.0, 10.0},
            {0.0, 10.0})
            .valid());
    CHECK(
        model.addLine(
            {0.0, 10.0},
            {0.0, 0.0})
            .valid());

    const auto regions =
        sketch::analyzeRegions(model);
    CHECK(regions.complete());
    CHECK(regions.regions.size() == 1U);
    const auto intent =
        part::makeProfileRegionIntent(
            regions.regions.front());
    CHECK(intent.has_value());

    const auto sketch_id =
        sketch::SketchId::generate();
    const auto sketch_support =
        part::partSketchSupportForBuiltinPlane(
            core::BuiltinReferenceRole::xy_plane);
    CHECK(sketch_support.has_value());
    state.sketches.push_back(
        part::PartSketch{
            sketch_id,
            *sketch_support,
            true,
            std::move(model)});

    const auto profile_id =
        state.next_profile_id.allocate();
    CHECK(profile_id.has_value());
    state.profiles.push_back(
        part::PartProfile{
            *profile_id,
            sketch_id,
            "Base Profile",
            part::ProfileVisibilityPolicy::automatic,
            *intent});

    const auto feature_id =
        state.body.next_feature_id.allocate();
    CHECK(feature_id.has_value());
    state.body.features.push_back(
        part::PartFeature{
            *feature_id,
            "Base",
            false,
            part::ExtrudeFeature{
                *profile_id,
                part::ExtrudeOperation::add,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{10.0},
                    false}}});

    const part::BodyStageRef stage{
        part::BodyStageKind::after_feature,
        *feature_id};
    const part::FeatureSurfaceAddress
        surface_address{
            *feature_id,
            part::FeatureSurfaceRoleKind::extent_cap,
            std::nullopt,
            0U,
            0U,
            false};
    const part::SurfaceReference surface{
        stage,
        surface_address};
    CHECK(surface.valid());

    const auto xy =
        state.next_datum_id.allocate();
    const auto xz =
        state.next_datum_id.allocate();
    const auto yz =
        state.next_datum_id.allocate();
    const auto zero =
        state.next_datum_id.allocate();
    const auto surface_id =
        state.next_datum_id.allocate();
    const auto chained =
        state.next_datum_id.allocate();
    CHECK(
        xy && xz && yz && zero &&
        surface_id && chained);

    state.datum_planes.push_back(
        makeDatum(
            *xy,
            originPlane(
                core::BuiltinReferenceRole::xy_plane),
            10.0));
    state.datum_planes.push_back(
        makeDatum(
            *xz,
            originPlane(
                core::BuiltinReferenceRole::xz_plane),
            4.0));
    state.datum_planes.push_back(
        makeDatum(
            *yz,
            originPlane(
                core::BuiltinReferenceRole::yz_plane),
            -2.0));
    state.datum_planes.push_back(
        makeDatum(
            *zero,
            datumPlane(*xy),
            0.0));
    state.datum_planes.push_back(
        makeDatum(
            *surface_id,
            bodySurface(surface),
            5.0));
    state.datum_planes.push_back(
        makeDatum(
            *chained,
            datumPlane(*surface_id),
            -3.0));

    auto restored =
        part::PartDocument::restore(
            document.documentId(),
            std::move(state),
            document.revision());
    CHECK(restored.ok());

    return Fixture{
        std::move(*restored.document),
        *feature_id,
        surface_address,
        stage,
        *xy,
        *xz,
        *yz,
        *zero,
        *surface_id,
        *chained};
}

enum class SurfaceMode {
    resolved_plane,
    missing,
    ambiguous,
    unsupported,
    non_planar,
    stage_unavailable,
};

kernel::Frame3 planeFrame(double z) {
    kernel::Frame3 frame;
    frame.origin = {1.0, 2.0, z};
    frame.u_axis = {1.0, 0.0, 0.0};
    frame.v_axis = {0.0, 1.0, 0.0};
    frame.normal = {0.0, 0.0, 1.0};
    CHECK(frame.valid());
    return frame;
}

part::PartEvaluation makePartEvaluation(
    const Fixture& fixture,
    SurfaceMode mode,
    double z = 10.0,
    std::uint64_t token_seed = 100U) {
    part::PartEvaluation result;
    result.source_revision =
        fixture.document.revision();

    part::FeatureEvaluation feature;
    feature.feature_id =
        fixture.feature_id;

    if (mode ==
        SurfaceMode::stage_unavailable) {
        feature.status =
            part::FeatureEvaluationStatus::failed;
        feature.diagnostic =
            part::FeatureEvaluationDiagnosticCode::
                kernel_provider_failure;
        result.features.push_back(
            std::move(feature));
        result.body_status =
            part::BodyEvaluationStatus::unavailable;
        return result;
    }

    feature.status =
        part::FeatureEvaluationStatus::up_to_date;
    feature.diagnostic =
        part::FeatureEvaluationDiagnosticCode::none;

    part::BodyStageTopologyCatalog catalog;
    catalog.stage = fixture.stage;

    part::FeatureSurfaceResolution surface;
    surface.address =
        fixture.surface_address;

    switch (mode) {
    case SurfaceMode::resolved_plane:
        surface.status =
            kernel::ReferenceStatus::resolved;
        surface.strict_face_status =
            kernel::ReferenceStatus::resolved;
        surface.candidate_face_count = 1U;
        surface.surface_kind =
            kernel::SurfaceKind::plane;
        surface.canonical_frame =
            planeFrame(z);
        surface.runtime_token =
            kernel::RuntimeSurfaceToken{
                token_seed};
        surface.current_faces = {
            kernel::RuntimeFaceToken{
                token_seed + 1U}};
        break;

    case SurfaceMode::missing:
        surface.status =
            kernel::ReferenceStatus::missing;
        surface.strict_face_status =
            kernel::ReferenceStatus::missing;
        surface.candidate_face_count = 0U;
        surface.surface_kind =
            kernel::SurfaceKind::plane;
        break;

    case SurfaceMode::ambiguous:
        surface.status =
            kernel::ReferenceStatus::ambiguous;
        surface.strict_face_status =
            kernel::ReferenceStatus::ambiguous;
        surface.candidate_face_count = 1U;
        surface.surface_kind =
            kernel::SurfaceKind::plane;
        surface.current_faces = {
            kernel::RuntimeFaceToken{
                token_seed + 1U}};
        break;

    case SurfaceMode::unsupported:
        surface.status =
            kernel::ReferenceStatus::unsupported;
        surface.strict_face_status =
            kernel::ReferenceStatus::unsupported;
        surface.candidate_face_count = 0U;
        surface.surface_kind =
            kernel::SurfaceKind::other;
        break;

    case SurfaceMode::non_planar:
        surface.status =
            kernel::ReferenceStatus::resolved;
        surface.strict_face_status =
            kernel::ReferenceStatus::resolved;
        surface.candidate_face_count = 1U;
        surface.surface_kind =
            kernel::SurfaceKind::cylinder;
        surface.runtime_token =
            kernel::RuntimeSurfaceToken{
                token_seed};
        surface.current_faces = {
            kernel::RuntimeFaceToken{
                token_seed + 1U}};
        break;

    case SurfaceMode::stage_unavailable:
        CHECK(false);
        break;
    }

    CHECK(surface.valid());
    catalog.surfaces.push_back(
        std::move(surface));
    CHECK(catalog.complete());
    feature.result_topology =
        std::move(catalog);
    result.features.push_back(
        std::move(feature));
    result.body_status =
        part::BodyEvaluationStatus::up_to_date;
    return result;
}

const part::DatumPlaneEvaluation& requireDatum(
    const part::DatumEvaluation& evaluation,
    part::DatumId id) {
    const auto* found =
        evaluation.find(id);
    CHECK(found != nullptr);
    CHECK(found->valid());
    return *found;
}

void checkOrigin(
    const kernel::Frame3& frame,
    double x,
    double y,
    double z) {
    CHECK(frame.origin.x == x);
    CHECK(frame.origin.y == y);
    CHECK(frame.origin.z == z);
}

void verifyUnresolvedSurface(
    const Fixture& fixture,
    SurfaceMode mode,
    part::DatumPlaneEvaluationStatus expected_status,
    part::DatumPlaneEvaluationDiagnostic expected_diagnostic) {
    const auto evaluation =
        part::evaluateDatums(
            fixture.document,
            makePartEvaluation(
                fixture,
                mode));
    CHECK(evaluation.valid());

    const auto& surface =
        requireDatum(
            evaluation,
            fixture.surface_datum);
    CHECK(surface.status == expected_status);
    CHECK(surface.diagnostic == expected_diagnostic);
    CHECK(!surface.frame.has_value());
    CHECK(
        surface.required_body_stage ==
        fixture.stage);

    const auto& chained =
        requireDatum(
            evaluation,
            fixture.chained_datum);
    CHECK(
        chained.status ==
        part::DatumPlaneEvaluationStatus::blocked);
    CHECK(
        chained.diagnostic ==
        part::DatumPlaneEvaluationDiagnostic::
            upstream_datum_unavailable);
    CHECK(!chained.frame.has_value());
    CHECK(
        chained.required_body_stage ==
        fixture.stage);
}

} // namespace

int main() {
    const auto fixture = makeFixture();
    const auto authored_before =
        fixture.document.state();

    const auto body =
        makePartEvaluation(
            fixture,
            SurfaceMode::resolved_plane,
            10.0,
            100U);
    const auto first =
        part::evaluateDatums(
            fixture.document,
            body);
    CHECK(first.valid());
    CHECK(
        first.source_revision ==
        fixture.document.revision());
    CHECK(
        fixture.document.state() ==
        authored_before);

    const auto& xy =
        requireDatum(
            first,
            fixture.xy_datum);
    CHECK(
        xy.status ==
        part::DatumPlaneEvaluationStatus::resolved);
    CHECK(xy.frame.has_value());
    checkOrigin(*xy.frame, 0.0, 0.0, 10.0);
    CHECK(
        xy.frame->normal ==
        kernel::Point3{0.0, 0.0, 1.0});
    CHECK(!xy.required_body_stage.has_value());

    const auto& xz =
        requireDatum(
            first,
            fixture.xz_datum);
    CHECK(xz.frame.has_value());
    checkOrigin(*xz.frame, 0.0, -4.0, 0.0);
    CHECK(
        xz.frame->u_axis ==
        kernel::Point3{1.0, 0.0, 0.0});
    CHECK(
        xz.frame->v_axis ==
        kernel::Point3{0.0, 0.0, 1.0});
    CHECK(
        xz.frame->normal ==
        kernel::Point3{0.0, -1.0, 0.0});

    const auto& yz =
        requireDatum(
            first,
            fixture.yz_datum);
    CHECK(yz.frame.has_value());
    checkOrigin(*yz.frame, -2.0, 0.0, 0.0);
    CHECK(
        yz.frame->normal ==
        kernel::Point3{1.0, 0.0, 0.0});

    const auto& zero =
        requireDatum(
            first,
            fixture.zero_datum);
    CHECK(zero.frame.has_value());
    CHECK(*zero.frame == *xy.frame);
    CHECK(
        zero.datum_id !=
        xy.datum_id);

    const auto& surface =
        requireDatum(
            first,
            fixture.surface_datum);
    CHECK(
        surface.status ==
        part::DatumPlaneEvaluationStatus::resolved);
    CHECK(surface.frame.has_value());
    checkOrigin(*surface.frame, 1.0, 2.0, 15.0);
    CHECK(
        surface.required_body_stage ==
        fixture.stage);

    const auto& chained =
        requireDatum(
            first,
            fixture.chained_datum);
    CHECK(chained.frame.has_value());
    checkOrigin(*chained.frame, 1.0, 2.0, 12.0);
    CHECK(
        chained.required_body_stage ==
        fixture.stage);

    // A source move changes only derived frames. Authored Datum source/offset
    // state and local semantic identity stay unchanged.
    const auto moved =
        part::evaluateDatums(
            fixture.document,
            makePartEvaluation(
                fixture,
                SurfaceMode::resolved_plane,
                20.0,
                200U));
    CHECK(moved.valid());
    const auto& moved_surface =
        requireDatum(
            moved,
            fixture.surface_datum);
    const auto& moved_chain =
        requireDatum(
            moved,
            fixture.chained_datum);
    CHECK(moved_surface.frame.has_value());
    CHECK(moved_chain.frame.has_value());
    checkOrigin(
        *moved_surface.frame,
        1.0,
        2.0,
        25.0);
    checkOrigin(
        *moved_chain.frame,
        1.0,
        2.0,
        22.0);
    CHECK(
        fixture.document.state() ==
        authored_before);

    verifyUnresolvedSurface(
        fixture,
        SurfaceMode::missing,
        part::DatumPlaneEvaluationStatus::missing,
        part::DatumPlaneEvaluationDiagnostic::
            missing_surface);
    verifyUnresolvedSurface(
        fixture,
        SurfaceMode::ambiguous,
        part::DatumPlaneEvaluationStatus::ambiguous,
        part::DatumPlaneEvaluationDiagnostic::
            ambiguous_surface);
    verifyUnresolvedSurface(
        fixture,
        SurfaceMode::unsupported,
        part::DatumPlaneEvaluationStatus::unsupported,
        part::DatumPlaneEvaluationDiagnostic::
            unsupported_surface);
    verifyUnresolvedSurface(
        fixture,
        SurfaceMode::non_planar,
        part::DatumPlaneEvaluationStatus::unsupported,
        part::DatumPlaneEvaluationDiagnostic::
            unsupported_non_planar);

    // An authored Body stage that is unavailable blocks the Datum but never
    // falls back to a final Body or a previous last-good frame.
    {
        const auto failed =
            part::evaluateDatums(
                fixture.document,
                makePartEvaluation(
                    fixture,
                    SurfaceMode::stage_unavailable));
        const auto& blocked =
            requireDatum(
                failed,
                fixture.surface_datum);
        CHECK(
            blocked.status ==
            part::DatumPlaneEvaluationStatus::blocked);
        CHECK(
            blocked.diagnostic ==
            part::DatumPlaneEvaluationDiagnostic::
                body_stage_unavailable);
        CHECK(!blocked.frame.has_value());
    }

    // A stale Part evaluation cannot resolve Body-dependent Datum meaning.
    // Origin-only Datum chains do not consume that stale runtime evidence.
    {
        auto stale =
            makePartEvaluation(
                fixture,
                SurfaceMode::resolved_plane);
        stale.source_revision =
            core::DocumentRevision{
                fixture.document.revision().value() +
                1U};
        const auto evaluation =
            part::evaluateDatums(
                fixture.document,
                stale);
        const auto& origin =
            requireDatum(
                evaluation,
                fixture.xy_datum);
        CHECK(
            origin.status ==
            part::DatumPlaneEvaluationStatus::resolved);

        const auto& blocked =
            requireDatum(
                evaluation,
                fixture.surface_datum);
        CHECK(
            blocked.status ==
            part::DatumPlaneEvaluationStatus::blocked);
        CHECK(
            blocked.diagnostic ==
            part::DatumPlaneEvaluationDiagnostic::
                stale_part_evaluation);
        CHECK(!blocked.frame.has_value());

        const auto& downstream =
            requireDatum(
                evaluation,
                fixture.chained_datum);
        CHECK(
            downstream.status ==
            part::DatumPlaneEvaluationStatus::blocked);
        CHECK(!downstream.frame.has_value());
    }

    // Runtime/provider tokens are disposable. Equivalent fresh semantic
    // evidence gives the same Datum result after a cold-style reevaluation.
    const auto fresh_tokens =
        part::evaluateDatums(
            fixture.document,
            makePartEvaluation(
                fixture,
                SurfaceMode::resolved_plane,
                10.0,
                900U));
    CHECK(fresh_tokens == first);

    // Re-evaluating the same current inputs is deterministic.
    CHECK(
        part::evaluateDatums(
            fixture.document,
            body) ==
        first);

    std::cout
        << "PM-03B Datum evaluation tests passed\n";
    return EXIT_SUCCESS;
}
