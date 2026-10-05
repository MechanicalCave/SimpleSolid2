#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <algorithm>
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
        std::cerr << "PM-02G CHECK failed at line "
                  << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

enum class ExtentMode { plane, non_planar };

class SupportSolid final : public kernel::RuntimeSolid {
public:
    struct Surface final {
        kernel::RuntimeFaceToken face;
        kernel::RuntimeSurfaceToken surface;
        kernel::SurfaceKind kind{kernel::SurfaceKind::plane};
        std::optional<kernel::Frame3> frame;
    };

    std::vector<Surface> surfaces;
    std::uint64_t next_face{1U};
    std::uint64_t next_surface{1U};
};

class SupportKernel final : public kernel::ISolidModelingKernel {
public:
    ExtentMode extent_mode{ExtentMode::plane};

    kernel::SolidModelingResult extrude(
        const kernel::LinearExtrudeInput& input,
        kernel::RuntimeSolidHandle upstream = {}) noexcept override {
        kernel::SolidModelingResult result;
        if (!input.valid()) {
            result.status =
                kernel::SolidModelingStatus::invalid_input;
            return result;
        }
        if (input.operation ==
                kernel::SolidBooleanOperation::cut &&
            upstream == nullptr) {
            result.status =
                kernel::SolidModelingStatus::missing_upstream;
            return result;
        }

        auto runtime =
            std::make_shared<SupportSolid>();
        if (upstream != nullptr) {
            const auto* previous =
                dynamic_cast<const SupportSolid*>(
                    upstream.get());
            if (previous == nullptr) {
                result.status =
                    kernel::SolidModelingStatus::
                        provider_mismatch;
                return result;
            }
            runtime->surfaces = previous->surfaces;
            runtime->next_face = previous->next_face;
            runtime->next_surface =
                previous->next_surface;

            for (const auto& inherited :
                 previous->surfaces) {
                result.current_faces.push_back(
                    inherited.face);
                result.inherited_faces.push_back(
                    {
                        inherited.face,
                        kernel::ReferenceStatus::resolved,
                        1U,
                    });
                result.inherited_surfaces.push_back(
                    {
                        inherited.surface,
                        kernel::ReferenceStatus::resolved,
                        kernel::ReferenceStatus::resolved,
                        1U,
                        inherited.kind,
                        inherited.frame,
                        {inherited.face},
                    });
            }
        }

        const auto publish =
            [&result, &runtime](
                kernel::ExtrudeFaceRole role,
                kernel::SurfaceKind kind,
                std::optional<kernel::Frame3> frame) {
                const kernel::RuntimeFaceToken face{
                    runtime->next_face++};
                const kernel::RuntimeSurfaceToken surface{
                    runtime->next_surface++};
                runtime->surfaces.push_back(
                    SupportSolid::Surface{
                        face,
                        surface,
                        kind,
                        frame});
                result.current_faces.push_back(face);
                result.new_faces.push_back(
                    {role, kernel::ReferenceStatus::resolved, 1U, face});
                result.new_surfaces.push_back(
                    {std::move(role),
                     kernel::ReferenceStatus::resolved,
                     kernel::ReferenceStatus::resolved,
                     1U,
                     kind,
                     std::move(frame),
                     surface,
                     {face}});
            };

        auto start = input.profile.frame;
        start.origin.x += start.normal.x * input.start_offset_mm;
        start.origin.y += start.normal.y * input.start_offset_mm;
        start.origin.z += start.normal.z * input.start_offset_mm;
        publish(
            {kernel::ExtrudeGeneratedFaceRoleKind::cap,
             input.start_cap_role,
             std::nullopt},
            kernel::SurfaceKind::plane,
            start);

        auto end = input.profile.frame;
        end.origin.x += end.normal.x * input.end_offset_mm;
        end.origin.y += end.normal.y * input.end_offset_mm;
        end.origin.z += end.normal.z * input.end_offset_mm;
        const auto end_kind =
            extent_mode == ExtentMode::plane
                ? kernel::SurfaceKind::plane
                : kernel::SurfaceKind::cylinder;
        publish(
            {kernel::ExtrudeGeneratedFaceRoleKind::cap,
             input.end_cap_role,
             std::nullopt},
            end_kind,
            end_kind == kernel::SurfaceKind::plane
                ? std::optional<kernel::Frame3>{end}
                : std::nullopt);

        CHECK(!input.profile.outer.boundary.empty());
        publish(
            {kernel::ExtrudeGeneratedFaceRoleKind::side,
             std::nullopt,
             input.profile.outer.boundary.front().provenance},
            kernel::SurfaceKind::plane,
            input.profile.frame);

        result.status = kernel::SolidModelingStatus::ok;
        result.solid = std::move(runtime);
        result.brep_valid = true;
        result.solid_count = 1U;
        result.face_count = result.current_faces.size();
        result.edge_count = 0U;
        result.vertex_count = 0U;
        return result;
    }
};

struct Fixture final {
    application::DocumentSession session;
    sketch::SketchId base_sketch_id;
};

Fixture makeFixture(SupportKernel& kernel) {
    application::DocumentSession session{
        {},
        part::PartDocument::create(core::DocumentId::generate())};

    const auto sketch_created = session.execute(
        application::CreatePartSketchCommand{
            core::BuiltinReferenceRole::xy_plane});
    CHECK(sketch_created.ok() && sketch_created.sketch_id);

    const auto rectangle = session.execute(
        application::AddSketchRectangleCommand{
            *sketch_created.sketch_id,
            session.document().revision(),
            {0.0, 0.0},
            {40.0, 30.0},
            sketch::EntityRole::regular,
            false});
    CHECK(rectangle.ok());
    CHECK(rectangle.entity_ids.size() == 4U);

    const auto* sketch_ptr =
        session.document().findSketch(*sketch_created.sketch_id);
    CHECK(sketch_ptr != nullptr);
    const auto regions = sketch::analyzeRegions(sketch_ptr->model);
    CHECK(regions.complete() && regions.regions.size() == 1U);
    const auto intent =
        part::makeProfileRegionIntent(regions.regions.front());
    CHECK(intent);

    const auto profile = session.execute(
        application::CreateProfileCommand{
            *sketch_created.sketch_id,
            session.document().revision(),
            *intent});
    CHECK(profile.ok() && profile.profile_id);

    const auto feature = session.execute(
        application::CreateExtrudeFeatureCommand{
            *profile.profile_id,
            session.document().revision(),
            part::ExtrudeOperation::add,
            part::OneSidedExtrudeExtent{
                core::LengthValue{10.0}, false},
            "Base"},
        kernel);
    CHECK(feature.ok() && feature.changed && feature.feature_id);

    return {std::move(session), *sketch_created.sketch_id};
}

part::SurfaceReference findSurfaceReference(
    const application::DocumentSession& session,
    SupportKernel& kernel,
    part::FeatureSurfaceRoleKind role,
    std::optional<part::FeatureId> producer = std::nullopt) {
    const auto evaluation =
        part::evaluatePart(session.document(), kernel);
    CHECK(evaluation.body_status ==
          part::BodyEvaluationStatus::up_to_date);
    CHECK(evaluation.current_topology);

    const auto& topology = *evaluation.current_topology;
    const auto found = std::find_if(
        topology.surfaces.begin(),
        topology.surfaces.end(),
        [role, producer](const auto& surface) {
            return surface.address.role == role &&
                   (!producer ||
                    surface.address.producer_feature_id ==
                        *producer) &&
                   surface.status == kernel::ReferenceStatus::resolved;
        });
    CHECK(found != topology.surfaces.end());

    part::SurfaceReference result{topology.stage, found->address};
    CHECK(result.valid());
    return result;
}

part::PartSketchSupport supportFor(part::SurfaceReference reference) {
    const auto support =
        part::partSketchSupportForBodyPlanarSurface(std::move(reference));
    CHECK(support);
    return *support;
}

} // namespace

int main() {
    // Origin-backed semantic creation remains independent of any modeling
    // provider and keeps the pre-PM-02 Origin workflow intact.
    application::DocumentSession origin_session{
        {},
        part::PartDocument::create(
            core::DocumentId::generate())};
    const auto origin_support =
        part::partSketchSupportForBuiltinPlane(
            core::BuiltinReferenceRole::xz_plane);
    CHECK(origin_support.has_value());
    const auto origin_created =
        origin_session.execute(
            application::CreatePartSketchOnSupportCommand{
                *origin_support,
                origin_session.document().revision()});
    CHECK(origin_created.ok());
    CHECK(origin_created.changed);
    CHECK(origin_created.sketch_id.has_value());

    SupportKernel kernel;
    auto fixture = makeFixture(kernel);

    const auto cap_support = supportFor(
        findSurfaceReference(
            fixture.session,
            kernel,
            part::FeatureSurfaceRoleKind::extent_cap));
    const auto side_support = supportFor(
        findSurfaceReference(
            fixture.session,
            kernel,
            part::FeatureSurfaceRoleKind::side));

    const auto state_before_create = fixture.session.document().state();
    const auto revision_before_create =
        fixture.session.document().revision();
    const auto created = fixture.session.execute(
        application::CreatePartSketchOnSupportCommand{
            cap_support, revision_before_create},
        &kernel);
    CHECK(created.ok() && created.changed && created.sketch_id);
    CHECK(created.status ==
          application::SketchSupportMutationStatus::applied);
    const auto created_id = *created.sketch_id;

    const auto* created_sketch =
        fixture.session.document().findSketch(created_id);
    CHECK(created_sketch != nullptr);
    CHECK(created_sketch->support == cap_support);

    const auto undo_create = fixture.session.undo();
    CHECK(undo_create.ok() && undo_create.changed);
    CHECK(fixture.session.document().state() == state_before_create);
    CHECK(fixture.session.document().findSketch(created_id) == nullptr);

    const auto redo_create = fixture.session.redo();
    CHECK(redo_create.ok() && redo_create.changed);
    CHECK(fixture.session.document().findSketch(created_id) != nullptr);

    const auto rectangle = fixture.session.execute(
        application::AddSketchRectangleCommand{
            created_id,
            fixture.session.document().revision(),
            {2.0, 3.0},
            {8.0, 9.0},
            sketch::EntityRole::regular,
            false});
    CHECK(rectangle.ok());
    CHECK(rectangle.entity_ids.size() == 4U);

    const auto* before_resupport =
        fixture.session.document().findSketch(created_id);
    CHECK(before_resupport != nullptr);
    const auto local_before = before_resupport->model.state();

    const auto resupported = fixture.session.execute(
        application::SetPartSketchSupportCommand{
            created_id,
            side_support,
            fixture.session.document().revision()},
        &kernel);
    CHECK(resupported.ok() && resupported.changed);
    CHECK(resupported.sketch_id ==
          std::optional<sketch::SketchId>{created_id});
    const auto* after_resupport =
        fixture.session.document().findSketch(created_id);
    CHECK(after_resupport != nullptr);
    CHECK(after_resupport->support == side_support);
    CHECK(after_resupport->model.state() == local_before);

    const auto undo_resupport = fixture.session.undo();
    CHECK(undo_resupport.ok() && undo_resupport.changed);
    const auto* cap_again =
        fixture.session.document().findSketch(created_id);
    CHECK(cap_again != nullptr);
    CHECK(cap_again->support == cap_support);
    CHECK(cap_again->model.state() == local_before);

    const auto redo_resupport = fixture.session.redo();
    CHECK(redo_resupport.ok() && redo_resupport.changed);
    const auto* side_again =
        fixture.session.document().findSketch(created_id);
    CHECK(side_again != nullptr);
    CHECK(side_again->support == side_support);
    CHECK(side_again->model.state() == local_before);

    const auto cycle_state = fixture.session.document().state();
    const auto cycle_revision = fixture.session.document().revision();
    const auto cycle = fixture.session.execute(
        application::SetPartSketchSupportCommand{
            fixture.base_sketch_id,
            cap_support,
            cycle_revision},
        &kernel);
    CHECK(!cycle.ok() && !cycle.changed);
    CHECK(cycle.status ==
          application::SketchSupportMutationStatus::cycle_dependency);
    CHECK(fixture.session.document().revision() == cycle_revision);
    CHECK(fixture.session.document().state() == cycle_state);

    // The same bounded dependency rule is a Domain reconstruction invariant,
    // so malformed authored state cannot bypass the semantic command path.
    auto cyclic_state =
        fixture.session.document().state();
    auto cyclic_sketch =
        std::find_if(
            cyclic_state.sketches.begin(),
            cyclic_state.sketches.end(),
            [&fixture](const auto& sketch) {
                return sketch.id ==
                       fixture.base_sketch_id;
            });
    CHECK(cyclic_sketch !=
          cyclic_state.sketches.end());
    cyclic_sketch->support = cap_support;
    const auto cyclic_restore =
        part::PartDocument::restore(
            fixture.session.document().documentId(),
            std::move(cyclic_state),
            fixture.session.document().revision());
    CHECK(!cyclic_restore.ok());

    const auto same_support_created =
        fixture.session.execute(
            application::CreatePartSketchOnSupportCommand{
                cap_support,
                fixture.session.document().revision()},
            &kernel);
    CHECK(
        same_support_created.ok() &&
        same_support_created.sketch_id);

    kernel.extent_mode = ExtentMode::non_planar;
    const auto same_support_state =
        fixture.session.document().state();
    const auto same_support_revision =
        fixture.session.document().revision();
    const auto same_support_revalidated =
        fixture.session.execute(
            application::SetPartSketchSupportCommand{
                *same_support_created.sketch_id,
                cap_support,
                same_support_revision},
            &kernel);
    CHECK(!same_support_revalidated.ok());
    CHECK(!same_support_revalidated.changed);
    CHECK(
        same_support_revalidated.status ==
        application::SketchSupportMutationStatus::
            unsupported);
    CHECK(
        fixture.session.document().revision() ==
        same_support_revision);
    CHECK(
        fixture.session.document().state() ==
        same_support_state);

    const auto unsupported_state = fixture.session.document().state();
    const auto unsupported_revision =
        fixture.session.document().revision();
    const auto unsupported = fixture.session.execute(
        application::CreatePartSketchOnSupportCommand{
            cap_support,
            unsupported_revision},
        &kernel);
    CHECK(!unsupported.ok() && !unsupported.changed);
    CHECK(unsupported.status ==
          application::SketchSupportMutationStatus::unsupported);
    CHECK(fixture.session.document().revision() == unsupported_revision);
    CHECK(fixture.session.document().state() == unsupported_state);

    kernel.extent_mode = ExtentMode::plane;
    const auto stale_state = fixture.session.document().state();
    const auto stale = fixture.session.execute(
        application::CreatePartSketchOnSupportCommand{
            cap_support,
            revision_before_create},
        &kernel);
    CHECK(!stale.ok() && !stale.changed);
    CHECK(stale.status ==
          application::SketchSupportMutationStatus::stale_revision);
    CHECK(fixture.session.document().state() == stale_state);

    // A planar Surface produced by a later Cut is admitted by the identical
    // semantic SurfaceReference path; no cap-only or Add-only special case.
    const auto base_profile_id =
        fixture.session.document().profiles().front().id;
    const auto cut = fixture.session.execute(
        application::CreateExtrudeFeatureCommand{
            base_profile_id,
            fixture.session.document().revision(),
            part::ExtrudeOperation::cut,
            part::OneSidedExtrudeExtent{
                core::LengthValue{4.0}, false},
            "Cut"},
        kernel);
    CHECK(cut.ok() && cut.changed && cut.feature_id);

    const auto cut_support = supportFor(
        findSurfaceReference(
            fixture.session,
            kernel,
            part::FeatureSurfaceRoleKind::side,
            *cut.feature_id));
    const auto cut_sketch = fixture.session.execute(
        application::CreatePartSketchOnSupportCommand{
            cut_support,
            fixture.session.document().revision()},
        &kernel);
    CHECK(cut_sketch.ok());
    CHECK(cut_sketch.changed);
    CHECK(cut_sketch.sketch_id.has_value());
    const auto* cut_hosted =
        fixture.session.document().findSketch(
            *cut_sketch.sketch_id);
    CHECK(cut_hosted != nullptr);
    CHECK(cut_hosted->support == cut_support);

    std::cout
        << "PM02G_FACE_SUPPORTED_SKETCH_AUTHORING_PASS"
        << " origin_parity=1"
        << " cap=1"
        << " lateral=1"
        << " cut_exposed=1"
        << " ids_preserved=1"
        << " local_uv_preserved=1"
        << " cycle_rejected=1"
        << " unsupported_non_planar=1"
        << " same_support_revalidated=1"
        << " rejected_mutation=0\n";
    return EXIT_SUCCESS;
}
