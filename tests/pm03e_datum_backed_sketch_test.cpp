#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/part/datum_evaluation.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/part/part_document_store.hpp>
#include <simplesolid2/persistence/native_document_container.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-03E Datum-backed Sketch CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

struct TempDirectory final {
    std::filesystem::path path;

    TempDirectory() {
        path =
            std::filesystem::temp_directory_path() /
            ("simplesolid2_pm03e_" +
             std::to_string(
                 std::filesystem::file_time_type::clock::now()
                     .time_since_epoch()
                     .count()));
        std::filesystem::create_directories(path);
    }

    ~TempDirectory() {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};

class TestSolid final : public kernel::RuntimeSolid {
public:
    struct Surface final {
        kernel::RuntimeFaceToken face;
        kernel::RuntimeSurfaceToken surface;
        kernel::SurfaceKind kind{
            kernel::SurfaceKind::plane};
        std::optional<kernel::Frame3> frame;
    };

    std::vector<Surface> surfaces;
    std::uint64_t next_face{1U};
    std::uint64_t next_surface{1U};
};

class TestKernel final : public kernel::ISolidModelingKernel {
public:
    std::size_t extrude_calls{};

    kernel::SolidModelingResult extrude(
        const kernel::LinearExtrudeInput& input,
        kernel::RuntimeSolidHandle upstream = {}) noexcept override {
        ++extrude_calls;

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
            std::make_shared<TestSolid>();
        if (upstream != nullptr) {
            const auto* previous =
                dynamic_cast<const TestSolid*>(
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
                std::optional<kernel::Frame3> frame) {
                const kernel::RuntimeFaceToken face{
                    runtime->next_face++};
                const kernel::RuntimeSurfaceToken surface{
                    runtime->next_surface++};
                runtime->surfaces.push_back(
                    TestSolid::Surface{
                        face,
                        surface,
                        kernel::SurfaceKind::plane,
                        frame});
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
                        kernel::SurfaceKind::plane,
                        std::move(frame),
                        surface,
                        {face},
                    });
            };

        auto start = input.profile.frame;
        start.origin.x +=
            start.normal.x * input.start_offset_mm;
        start.origin.y +=
            start.normal.y * input.start_offset_mm;
        start.origin.z +=
            start.normal.z * input.start_offset_mm;
        publish(
            {
                kernel::ExtrudeGeneratedFaceRoleKind::cap,
                input.start_cap_role,
                std::nullopt,
            },
            start);

        auto end = input.profile.frame;
        end.origin.x +=
            end.normal.x * input.end_offset_mm;
        end.origin.y +=
            end.normal.y * input.end_offset_mm;
        end.origin.z +=
            end.normal.z * input.end_offset_mm;
        publish(
            {
                kernel::ExtrudeGeneratedFaceRoleKind::cap,
                input.end_cap_role,
                std::nullopt,
            },
            end);

        CHECK(!input.profile.outer.boundary.empty());
        publish(
            {
                kernel::ExtrudeGeneratedFaceRoleKind::side,
                std::nullopt,
                input.profile.outer.boundary.front()
                    .provenance,
            },
            input.profile.frame);

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

struct Fixture final {
    application::DocumentSession session;
    sketch::SketchId base_sketch_id;
    part::ProfileId base_profile_id;
    part::FeatureId base_feature_id;
};

Fixture makeFixture(TestKernel& kernel) {
    application::DocumentSession session{
        {},
        part::PartDocument::create(
            core::DocumentId::generate())};

    const auto sketch_created =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
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
    CHECK(rectangle.entity_ids.size() == 4U);

    const auto* sketch_ptr =
        session.document().findSketch(
            *sketch_created.sketch_id);
    CHECK(sketch_ptr != nullptr);
    const auto regions =
        sketch::analyzeRegions(
            sketch_ptr->model);
    CHECK(
        regions.complete() &&
        regions.regions.size() == 1U);
    const auto intent =
        part::makeProfileRegionIntent(
            regions.regions.front());
    CHECK(intent);

    const auto profile =
        session.execute(
            application::CreateProfileCommand{
                *sketch_created.sketch_id,
                session.document().revision(),
                *intent});
    CHECK(profile.ok() && profile.profile_id);

    const auto feature =
        session.execute(
            application::CreateExtrudeFeatureCommand{
                *profile.profile_id,
                session.document().revision(),
                part::ExtrudeOperation::add,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{10.0},
                    false},
                "Base"},
            kernel);
    CHECK(
        feature.ok() &&
        feature.changed &&
        feature.feature_id);

    return {
        std::move(session),
        *sketch_created.sketch_id,
        *profile.profile_id,
        *feature.feature_id,
    };
}

part::SurfaceReference findBaseCap(
    const application::DocumentSession& session,
    TestKernel& kernel,
    part::FeatureId producer) {
    const auto evaluation =
        part::evaluatePart(
            session.document(),
            kernel);
    CHECK(
        evaluation.body_status ==
        part::BodyEvaluationStatus::up_to_date);
    CHECK(evaluation.current_topology);

    const auto& topology =
        *evaluation.current_topology;
    const auto found =
        std::find_if(
            topology.surfaces.begin(),
            topology.surfaces.end(),
            [producer](const auto& surface) {
                return
                    surface.address
                            .producer_feature_id ==
                        producer &&
                    surface.address.role ==
                        part::FeatureSurfaceRoleKind::
                            extent_cap &&
                    surface.status ==
                        kernel::ReferenceStatus::
                            resolved &&
                    surface.surface_kind ==
                        kernel::SurfaceKind::plane;
            });
    CHECK(found != topology.surfaces.end());

    part::SurfaceReference reference{
        topology.stage,
        found->address};
    CHECK(reference.valid());
    return reference;
}

part::SketchPlacement resolvedDatumSketchFrame(
    const application::DocumentSession& session,
    TestKernel& kernel,
    const part::PartSketchSupport& support) {
    const auto part_evaluation =
        part::evaluatePart(
            session.document(),
            kernel);
    const auto datums =
        part::evaluateDatums(
            session.document(),
            part_evaluation);
    const auto resolved =
        part::resolveSketchSupport(
            support,
            nullptr,
            &datums);
    CHECK(
        resolved.status ==
        part::SketchSupportResolutionStatus::
            resolved);
    CHECK(resolved.frame);
    return *resolved.frame;
}

} // namespace

int main() {
    CHECK(
        part::PartDocumentStore::
            current_schema_version == 11);

    TestKernel kernel;
    auto fixture = makeFixture(kernel);

    const auto base_surface =
        findBaseCap(
            fixture.session,
            kernel,
            fixture.base_feature_id);

    const part::PlaneReference datum_source{
        part::BodyPlanarSurfacePlaneReference{
            base_surface}};

    const auto datum =
        fixture.session.execute(
            application::CreateDatumPlaneCommand{
                datum_source,
                fixture.session.document()
                    .revision(),
                core::LengthValue{5.0},
                true},
            kernel);
    CHECK(
        datum.ok() &&
        datum.changed &&
        datum.datum_id);

    const auto datum_support =
        part::partSketchSupportForDatumPlane(
            *datum.datum_id);
    CHECK(datum_support);

    const auto create_sketch =
        fixture.session.execute(
            application::CreatePartSketchOnSupportCommand{
                *datum_support,
                fixture.session.document()
                    .revision()},
            &kernel);
    CHECK(
        create_sketch.ok() &&
        create_sketch.changed &&
        create_sketch.sketch_id);
    CHECK(
        create_sketch.status ==
        application::SketchSupportMutationStatus::
            applied);

    const auto datum_sketch_id =
        *create_sketch.sketch_id;
    const auto* authored_sketch =
        fixture.session.document().findSketch(
            datum_sketch_id);
    CHECK(authored_sketch != nullptr);
    CHECK(
        part::datumPlaneIdForSketchSupport(
            authored_sketch->support) ==
        datum.datum_id);

    const auto rectangle =
        fixture.session.execute(
            application::AddSketchRectangleCommand{
                datum_sketch_id,
                fixture.session.document()
                    .revision(),
                {2.0, 3.0},
                {12.0, 11.0},
                sketch::EntityRole::regular,
                false});
    CHECK(rectangle.ok());
    CHECK(rectangle.entity_ids.size() == 4U);

    const auto* with_geometry =
        fixture.session.document().findSketch(
            datum_sketch_id);
    CHECK(with_geometry != nullptr);
    const auto local_state =
        with_geometry->model.state();

    const auto regions =
        sketch::analyzeRegions(
            with_geometry->model);
    CHECK(
        regions.complete() &&
        regions.regions.size() == 1U);
    const auto intent =
        part::makeProfileRegionIntent(
            regions.regions.front());
    CHECK(intent);

    const auto profile =
        fixture.session.execute(
            application::CreateProfileCommand{
                datum_sketch_id,
                fixture.session.document()
                    .revision(),
                *intent});
    CHECK(profile.ok() && profile.profile_id);

    const auto add =
        fixture.session.execute(
            application::CreateExtrudeFeatureCommand{
                *profile.profile_id,
                fixture.session.document()
                    .revision(),
                part::ExtrudeOperation::add,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{4.0},
                    false},
                "Datum Add"},
            kernel);
    CHECK(
        add.ok() &&
        add.changed &&
        add.feature_id);

    const auto cut =
        fixture.session.execute(
            application::CreateExtrudeFeatureCommand{
                *profile.profile_id,
                fixture.session.document()
                    .revision(),
                part::ExtrudeOperation::cut,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{2.0},
                    false},
                "Datum Cut"},
            kernel);
    CHECK(
        cut.ok() &&
        cut.changed &&
        cut.feature_id);

    // Both existing Extrude operations consume the Datum-backed Profile
    // through the same feature evaluator and provider-neutral Kernel input.
    {
        kernel.extrude_calls = 0U;
        const auto evaluation =
            part::evaluatePart(
                fixture.session.document(),
                kernel);
        CHECK(
            evaluation.body_status ==
            part::BodyEvaluationStatus::
                up_to_date);
        CHECK(
            evaluation.findFeature(
                fixture.base_feature_id) !=
            nullptr);
        CHECK(
            evaluation.findFeature(
                *add.feature_id)->status ==
            part::FeatureEvaluationStatus::
                up_to_date);
        CHECK(
            evaluation.findFeature(
                *cut.feature_id)->status ==
            part::FeatureEvaluationStatus::
                up_to_date);
        CHECK(kernel.extrude_calls == 3U);
    }

    // Change Support Origin -> Datum -> Origin -> Datum preserves durable
    // SketchId, EntityIds and local U/V geometry. Only support intent moves.
    const auto yz_support =
        part::partSketchSupportForBuiltinPlane(
            core::BuiltinReferenceRole::
                yz_plane);
    CHECK(yz_support);

    const auto to_origin =
        fixture.session.execute(
            application::SetPartSketchSupportCommand{
                datum_sketch_id,
                *yz_support,
                fixture.session.document()
                    .revision()},
            &kernel);
    CHECK(to_origin.ok() && to_origin.changed);
    CHECK(
        fixture.session.document()
            .findSketch(datum_sketch_id)
            ->model.state() ==
        local_state);

    const auto back_to_datum =
        fixture.session.execute(
            application::SetPartSketchSupportCommand{
                datum_sketch_id,
                *datum_support,
                fixture.session.document()
                    .revision()},
            &kernel);
    CHECK(
        back_to_datum.ok() &&
        back_to_datum.changed);
    const auto* resupported =
        fixture.session.document().findSketch(
            datum_sketch_id);
    CHECK(resupported != nullptr);
    CHECK(resupported->id == datum_sketch_id);
    CHECK(resupported->model.state() == local_state);
    CHECK(
        part::datumPlaneIdForSketchSupport(
            resupported->support) ==
        datum.datum_id);

    // Editing the upstream Datum offset changes only the current derived
    // world frame. The authored Sketch local model remains byte-for-byte
    // equivalent and no placement/frame becomes durable state.
    const auto before_frame =
        resolvedDatumSketchFrame(
            fixture.session,
            kernel,
            *datum_support);

    const auto edit_datum =
        fixture.session.execute(
            application::EditDatumPlaneCommand{
                *datum.datum_id,
                datum_source,
                fixture.session.document()
                    .revision(),
                core::LengthValue{8.0}},
            kernel);
    CHECK(edit_datum.ok() && edit_datum.changed);

    const auto after_frame =
        resolvedDatumSketchFrame(
            fixture.session,
            kernel,
            *datum_support);
    CHECK(
        after_frame.origin !=
        before_frame.origin);
    CHECK(
        fixture.session.document()
            .findSketch(datum_sketch_id)
            ->model.state() ==
        local_state);

    // The transitive Body-stage floor rejects a cycle: the base Sketch
    // cannot be re-supported to a Datum whose source requires the Feature
    // that already consumes that same base Sketch.
    const auto cycle_state =
        fixture.session.document().state();
    const auto cycle_revision =
        fixture.session.document().revision();
    const auto cycle =
        fixture.session.execute(
            application::SetPartSketchSupportCommand{
                fixture.base_sketch_id,
                *datum_support,
                cycle_revision},
            &kernel);
    CHECK(!cycle.ok());
    CHECK(!cycle.changed);
    CHECK(
        cycle.status ==
        application::SketchSupportMutationStatus::
            cycle_dependency);
    CHECK(
        fixture.session.document().revision() ==
        cycle_revision);
    CHECK(
        fixture.session.document().state() ==
        cycle_state);

    auto cyclic_state =
        fixture.session.document().state();
    const auto base_sketch =
        std::find_if(
            cyclic_state.sketches.begin(),
            cyclic_state.sketches.end(),
            [&fixture](const auto& sketch) {
                return sketch.id ==
                       fixture.base_sketch_id;
            });
    CHECK(
        base_sketch !=
        cyclic_state.sketches.end());
    base_sketch->support = *datum_support;
    const auto cyclic_restore =
        part::PartDocument::restore(
            fixture.session.document()
                .documentId(),
            std::move(cyclic_state),
            fixture.session.document()
                .revision());
    CHECK(!cyclic_restore.ok());

    // A Datum used by a Sketch is lifecycle-owned semantic input and cannot
    // be deleted while the Sketch depends on it.
    const auto delete_state =
        fixture.session.document().state();
    const auto delete_revision =
        fixture.session.document().revision();
    const auto delete_datum =
        fixture.session.execute(
            application::DeleteDatumPlaneCommand{
                *datum.datum_id,
                delete_revision});
    CHECK(!delete_datum.ok());
    CHECK(!delete_datum.changed);
    CHECK(
        fixture.session.document().revision() ==
        delete_revision);
    CHECK(
        fixture.session.document().state() ==
        delete_state);

    // If the Datum's required upstream Body stage disappears, the Datum,
    // Sketch/Profile and downstream Features fail closed. No previous
    // world frame is reused and the Kernel is not called for stale geometry.
    const auto suppress =
        fixture.session.execute(
            application::SetFeatureSuppressedCommand{
                fixture.base_feature_id,
                fixture.session.document()
                    .revision(),
                true});
    CHECK(suppress.ok() && suppress.changed);

    kernel.extrude_calls = 0U;
    const auto blocked_part =
        part::evaluatePart(
            fixture.session.document(),
            kernel);
    CHECK(
        blocked_part.body_status ==
        part::BodyEvaluationStatus::
            unavailable);
    CHECK(kernel.extrude_calls == 0U);

    const auto blocked_datums =
        part::evaluateDatums(
            fixture.session.document(),
            blocked_part);
    const auto* blocked_datum =
        blocked_datums.find(
            *datum.datum_id);
    CHECK(blocked_datum != nullptr);
    CHECK(
        blocked_datum->status !=
        part::DatumPlaneEvaluationStatus::
            resolved);
    CHECK(!blocked_datum->frame);

    const auto blocked_support =
        part::resolveSketchSupport(
            *datum_support,
            nullptr,
            &blocked_datums);
    CHECK(
        blocked_support.status !=
        part::SketchSupportResolutionStatus::
            resolved);
    CHECK(!blocked_support.frame);

    const auto unsuppress =
        fixture.session.execute(
            application::SetFeatureSuppressedCommand{
                fixture.base_feature_id,
                fixture.session.document()
                    .revision(),
                false});
    CHECK(unsuppress.ok() && unsuppress.changed);

    // Schema v11 persists only semantic DatumId support. Derived world
    // placement is absent, and cold load rebuilds the same valid feature
    // chain from authored intent.
    TempDirectory temp;
    const auto path =
        temp.path / "DatumBacked.ss2part";
    part::PartDocumentStore store;
    const auto saved =
        store.createNew(
            path,
            fixture.session.document());
    CHECK(saved.ok());

    const auto package =
        persistence::readNativeDocumentContainer(
            path);
    CHECK(package.ok());
    CHECK(
        package.package->descriptor
            .domain_schema_version == 11);

    const auto authored =
        nlohmann::json::parse(
            package.package->authored_json);
    CHECK(authored.contains("sketches"));

    bool found_datum_sketch = false;
    for (const auto& item :
         authored["sketches"]) {
        if (item["id"].get<std::string>() !=
            datum_sketch_id.value()) {
            continue;
        }
        found_datum_sketch = true;
        CHECK(!item.contains("placement"));
        CHECK(item.contains("support"));
        CHECK(
            item["support"]["kind"]
                .get<std::string>() ==
            "datum_plane");
        CHECK(
            item["support"]["datum_id"]
                .get<std::string>() ==
            datum.datum_id->serialized());
        CHECK(
            !item["support"].contains("frame"));
        CHECK(
            !item["support"].contains("origin"));
        CHECK(
            !item["support"].contains("u_axis"));
        CHECK(
            !item["support"].contains("v_axis"));
    }
    CHECK(found_datum_sketch);

    auto loaded = store.load(path);
    CHECK(loaded.ok());
    const auto* loaded_sketch =
        loaded.document->findSketch(
            datum_sketch_id);
    CHECK(loaded_sketch != nullptr);
    CHECK(
        part::datumPlaneIdForSketchSupport(
            loaded_sketch->support) ==
        datum.datum_id);
    CHECK(
        loaded_sketch->model.state() ==
        local_state);

    kernel.extrude_calls = 0U;
    const auto rebuilt =
        part::evaluatePart(
            *loaded.document,
            kernel);
    CHECK(
        rebuilt.body_status ==
        part::BodyEvaluationStatus::
            up_to_date);
    CHECK(
        rebuilt.findFeature(
            *add.feature_id)->status ==
        part::FeatureEvaluationStatus::
            up_to_date);
    CHECK(
        rebuilt.findFeature(
            *cut.feature_id)->status ==
        part::FeatureEvaluationStatus::
            up_to_date);
    CHECK(kernel.extrude_calls == 3U);

    std::cout
        << "PM-03E_DATUM_BACKED_SKETCH_PASS"
        << " schema_v11=1"
        << " datum_support=1"
        << " local_uv_preserved=1"
        << " add_cut=1"
        << " offset_propagation=1"
        << " cycle_rejected=1"
        << " stale_frame_rejected=1"
        << " delete_dependency_safe=1"
        << " cold_rebuild=1\n";
    return EXIT_SUCCESS;
}
