#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/part/part_document_store.hpp>
#include <simplesolid2/persistence/native_document_container.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <nlohmann/json.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-02E schema-v9 CHECK failed at line "
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
            ("simplesolid2_pm02e_" +
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

void writeBytes(
    const std::filesystem::path& path,
    const std::string& bytes) {
    std::ofstream out{
        path,
        std::ios::binary | std::ios::trunc};
    CHECK(static_cast<bool>(out));
    out.write(
        bytes.data(),
        static_cast<std::streamsize>(
            bytes.size()));
    CHECK(static_cast<bool>(out));
}

class OpaqueSolid final
    : public kernel::RuntimeSolid {};

class OpaqueKernel final
    : public kernel::ISolidModelingKernel {
public:
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
        if (upstream != nullptr &&
            dynamic_cast<const OpaqueSolid*>(
                upstream.get()) == nullptr) {
            result.status =
                kernel::SolidModelingStatus::provider_mismatch;
            return result;
        }

        result.status =
            kernel::SolidModelingStatus::ok;
        result.solid =
            std::make_shared<OpaqueSolid>();
        result.brep_valid = true;
        result.solid_count = 1U;
        return result;
    }
};

struct RichDocumentIds final {
    sketch::SketchId sketch_id;
    std::vector<sketch::EntityId> entity_ids;
    part::ProfileId profile_id;
    part::FeatureId feature_id;
    part::BodyId body_id;
};

RichDocumentIds authorRichOriginDocument(
    application::DocumentSession& session,
    OpaqueKernel& kernel) {
    const auto sketch_result =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
    CHECK(
        sketch_result.ok() &&
        sketch_result.sketch_id);

    const auto rectangle =
        session.execute(
            application::AddSketchRectangleCommand{
                *sketch_result.sketch_id,
                session.document().revision(),
                {0.0, 0.0},
                {40.0, 30.0},
                sketch::EntityRole::regular,
                false});
    CHECK(rectangle.ok());
    CHECK(rectangle.entity_ids.size() == 4U);

    const auto* source =
        session.document().findSketch(
            *sketch_result.sketch_id);
    CHECK(source != nullptr);
    const auto analysis =
        sketch::analyzeRegions(source->model);
    CHECK(analysis.complete());
    CHECK(analysis.regions.size() == 1U);
    const auto intent =
        part::makeProfileRegionIntent(
            analysis.regions.front());
    CHECK(intent.has_value());

    const auto profile =
        session.execute(
            application::CreateProfileCommand{
                *sketch_result.sketch_id,
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
                "Base Add"},
            kernel);
    CHECK(feature.ok() && feature.feature_id);

    return {
        *sketch_result.sketch_id,
        rectangle.entity_ids,
        *profile.profile_id,
        *feature.feature_id,
        session.document().body().id};
}

nlohmann::json legacyPlacementFor(
    const nlohmann::json& support) {
    CHECK(support.is_object());
    CHECK(
        support.at("kind").get<std::string>() ==
        "builtin_origin_plane");
    const auto plane =
        support.at("builtin_plane")
            .get<std::string>();

    if (plane == "xy_plane") {
        return {
            {"origin", {0.0, 0.0, 0.0}},
            {"u_axis", {1.0, 0.0, 0.0}},
            {"v_axis", {0.0, 1.0, 0.0}},
        };
    }
    if (plane == "xz_plane") {
        return {
            {"origin", {0.0, 0.0, 0.0}},
            {"u_axis", {1.0, 0.0, 0.0}},
            {"v_axis", {0.0, 0.0, 1.0}},
        };
    }
    CHECK(plane == "yz_plane");
    return {
        {"origin", {0.0, 0.0, 0.0}},
        {"u_axis", {0.0, 1.0, 0.0}},
        {"v_axis", {0.0, 0.0, 1.0}},
    };
}

std::string buildSchema8FromCurrentOriginDocument(
    const persistence::NativeDocumentPackage& package,
    bool corrupt_placement = false) {
    auto authored =
        nlohmann::json::parse(
            package.authored_json);
    CHECK(authored.contains("sketches"));
    // Current schema may contain later-domain fields. An exact v8 payload
    // must remove everything introduced after v8 before adding legacy
    // authored placement.
    authored.erase("next_datum_id");
    authored.erase("datum_planes");
    authored.erase("next_axis_id");
    authored.erase("axes");
    for (auto& sketch : authored["sketches"]) {
        auto placement =
            legacyPlacementFor(
                sketch.at("support"));
        if (corrupt_placement) {
            placement["u_axis"] =
                nlohmann::json::array(
                    {0.0, 1.0, 0.0});
        }
        sketch["placement"] =
            std::move(placement);
    }

    auto text = authored.dump(2);
    text.push_back('\n');
    const auto built =
        persistence::buildNativeDocumentContainer(
            persistence::NativeDocumentDescriptor{
                "part",
                package.descriptor.document_id,
                8},
            text);
    CHECK(built.ok());
    return *built.bytes;
}

std::string buildSchema9FromCurrentDocument(
    const persistence::NativeDocumentPackage& package) {
    auto authored =
        nlohmann::json::parse(
            package.authored_json);
    CHECK(authored.contains("next_datum_id"));
    CHECK(authored.contains("datum_planes"));
    CHECK(authored.contains("next_axis_id"));
    CHECK(authored.contains("axes"));
    authored.erase("next_datum_id");
    authored.erase("datum_planes");
    authored.erase("next_axis_id");
    authored.erase("axes");

    auto text = authored.dump(2);
    text.push_back('\n');
    const auto built =
        persistence::buildNativeDocumentContainer(
            persistence::NativeDocumentDescriptor{
                "part",
                package.descriptor.document_id,
                9},
            text);
    CHECK(built.ok());
    return *built.bytes;
}

part::BodyStageTopologyCatalog resolvedPlanarCatalog(
    const part::SurfaceReference& reference) {
    CHECK(reference.stage.feature_id.has_value());

    const kernel::RuntimeFaceToken face_token{1U};
    const kernel::RuntimeSurfaceToken surface_token{1U};

    kernel::Frame3 frame;
    frame.origin = {0.0, 0.0, 10.0};

    part::FeatureSurfaceResolution surface;
    surface.address = reference.surface;
    surface.status =
        kernel::ReferenceStatus::resolved;
    surface.strict_face_status =
        kernel::ReferenceStatus::resolved;
    surface.candidate_face_count = 1U;
    surface.surface_kind =
        kernel::SurfaceKind::plane;
    surface.canonical_frame = frame;
    surface.runtime_token = surface_token;
    surface.current_faces = {face_token};
    CHECK(surface.valid());

    part::BodyFaceTopologyRecord face;
    face.runtime_token = face_token;
    face.accounting_class =
        part::TopologyAccountingClass::referenceable;
    face.surface_candidates = {
        reference.surface};
    CHECK(face.valid());

    part::BodyStageTopologyCatalog catalog;
    catalog.stage = reference.stage;
    catalog.faces = {face};
    catalog.surfaces = {surface};
    CHECK(catalog.complete());
    return catalog;
}

} // namespace

int main() {
    TempDirectory temp;
    part::PartDocumentStore store;

    // Build a rich current-schema Origin-backed Part first. This is then
    // converted to an exact schema-v8 payload so migration exercises the
    // production loader rather than a hand-written partial legacy document.
    const auto current_path =
        temp.path / "RichCurrent.ss2part";
    const auto document_id =
        core::DocumentId::generate();
    auto document =
        part::PartDocument::create(
            document_id);
    const auto created =
        store.createNew(
            current_path,
            document);
    CHECK(created.ok());

    application::DocumentSession session{
        current_path,
        std::move(document),
        *created.checkpoint};
    OpaqueKernel kernel;
    const auto ids =
        authorRichOriginDocument(
            session,
            kernel);
    CHECK(session.save().ok());

    const auto current_package =
        persistence::readNativeDocumentContainer(
            current_path);
    CHECK(current_package.ok());
    CHECK(
        current_package.package->descriptor
            .domain_schema_version == 14);
    CHECK(
        current_package.package->authored_json.find(
            "\"placement\"") ==
        std::string::npos);

    // Exact v9 -> v10 migration preserves every durable identity already
    // present in PM-02 while introducing an empty Datum collection.
    const auto legacy_v9_path =
        temp.path / "LegacyV9Rich.ss2part";
    writeBytes(
        legacy_v9_path,
        buildSchema9FromCurrentDocument(
            *current_package.package));

    auto migrated_v9 =
        store.load(legacy_v9_path);
    CHECK(migrated_v9.ok());
    CHECK(
        migrated_v9.document->documentId() ==
        document_id);
    CHECK(
        migrated_v9.document->body().id ==
        ids.body_id);
    CHECK(
        migrated_v9.document->findFeature(
            ids.feature_id) != nullptr);
    CHECK(
        migrated_v9.document->findProfile(
            ids.profile_id) != nullptr);
    const auto* migrated_v9_sketch =
        migrated_v9.document->findSketch(
            ids.sketch_id);
    CHECK(migrated_v9_sketch != nullptr);
    CHECK(
        migrated_v9_sketch->model.entityCount() ==
        ids.entity_ids.size());
    for (const auto entity : ids.entity_ids) {
        CHECK(
            migrated_v9_sketch->model.findLine(
                entity) != nullptr);
    }
    CHECK(migrated_v9.document->datumPlanes().empty());
    CHECK(
        migrated_v9.document->datumIdCursor()
            .serialized() == "1");

    // v8 -> current migration: all durable identities and local Sketch
    // geometry survive; absolute placement is validated and discarded.
    const auto legacy_path =
        temp.path / "LegacyV8.ss2part";
    writeBytes(
        legacy_path,
        buildSchema8FromCurrentOriginDocument(
            *current_package.package));

    auto migrated =
        store.load(legacy_path);
    CHECK(migrated.ok());
    CHECK(
        migrated.document->documentId() ==
        document_id);
    CHECK(
        migrated.document->body().id ==
        ids.body_id);
    CHECK(
        migrated.document->body()
            .features.size() == 1U);
    CHECK(
        migrated.document->body()
            .features.front().id ==
        ids.feature_id);
    CHECK(
        migrated.document->profiles()
            .size() == 1U);
    CHECK(
        migrated.document->profiles()
            .front().id ==
        ids.profile_id);

    const auto* migrated_sketch =
        migrated.document->findSketch(
            ids.sketch_id);
    CHECK(migrated_sketch != nullptr);
    CHECK(
        migrated_sketch->model.entityCount() ==
        ids.entity_ids.size());
    for (const auto entity : ids.entity_ids) {
        CHECK(
            migrated_sketch->model.findLine(
                entity) != nullptr);
    }
    CHECK(
        part::builtinOriginPlaneForSketchSupport(
            migrated_sketch->support) ==
        std::optional<core::BuiltinReferenceRole>{
            core::BuiltinReferenceRole::xy_plane});
    const auto migrated_frame =
        part::resolveSketchSupport(
            migrated_sketch->support);
    CHECK(migrated_frame.valid());
    CHECK(migrated_frame.frame.has_value());

    CHECK(
        store.save(
            legacy_path,
            *migrated.document,
            *migrated.checkpoint)
            .ok());

    const auto rewritten =
        persistence::readNativeDocumentContainer(
            legacy_path);
    CHECK(rewritten.ok());
    CHECK(
        rewritten.package->descriptor
            .domain_schema_version == 14);
    CHECK(
        rewritten.package->authored_json.find(
            "\"placement\"") ==
        std::string::npos);

    const auto migrated_reload =
        store.load(legacy_path);
    CHECK(migrated_reload.ok());
    CHECK(
        migrated_reload.document->documentId() ==
        document_id);
    CHECK(
        migrated_reload.document->body().id ==
        ids.body_id);
    CHECK(
        migrated_reload.document->findFeature(
            ids.feature_id) != nullptr);
    CHECK(
        migrated_reload.document->findProfile(
            ids.profile_id) != nullptr);
    const auto* migrated_reloaded_sketch =
        migrated_reload.document->findSketch(
            ids.sketch_id);
    CHECK(migrated_reloaded_sketch != nullptr);
    CHECK(
        migrated_reloaded_sketch->support ==
        migrated_sketch->support);
    CHECK(
        migrated_reloaded_sketch->model.state() ==
        migrated_sketch->model.state());

    // A malformed legacy placement never gets normalized or tolerated.
    const auto malformed_v8_path =
        temp.path / "MalformedLegacyV8.ss2part";
    writeBytes(
        malformed_v8_path,
        buildSchema8FromCurrentOriginDocument(
            *current_package.package,
            true));
    const auto malformed =
        store.load(malformed_v8_path);
    CHECK(!malformed.ok());
    CHECK(
        malformed.diagnostic.code ==
        part::PartStoreErrorCode::
            malformed_document);

    // v9 Body Surface support roundtrip. Creation is direct authored-state
    // construction because user-facing face-supported Sketch commands belong
    // to PM-02G, not this checkpoint.
    auto body_state =
        session.document().state();

    const part::SurfaceReference surface_reference{
        part::BodyStageRef{
            part::BodyStageKind::after_feature,
            ids.feature_id},
        part::FeatureSurfaceAddress{
            ids.feature_id,
            part::FeatureSurfaceRoleKind::
                extent_cap,
            std::nullopt,
            0U,
            0U,
            false}};
    CHECK(surface_reference.valid());

    const auto body_support =
        part::partSketchSupportForBodyPlanarSurface(
            surface_reference);
    CHECK(body_support.has_value());

    sketch::SketchModel body_model;
    const auto body_entity =
        body_model.addLine(
            {1.0, 2.0},
            {4.0, 2.0});
    CHECK(body_entity.valid());

    auto body_sketch_id =
        sketch::SketchId::generate();
    while (body_sketch_id == ids.sketch_id) {
        body_sketch_id =
            sketch::SketchId::generate();
    }

    body_state.sketches.push_back(
        part::PartSketch{
            body_sketch_id,
            *body_support,
            true,
            std::move(body_model)});

    auto body_document =
        part::PartDocument::restore(
            document_id,
            std::move(body_state),
            session.document().revision());
    CHECK(body_document.ok());

    const auto body_path =
        temp.path / "BodySurfaceV9.ss2part";
    const auto body_saved =
        store.createNew(
            body_path,
            *body_document.document);
    CHECK(body_saved.ok());

    const auto body_package =
        persistence::readNativeDocumentContainer(
            body_path);
    CHECK(body_package.ok());
    CHECK(
        body_package.package->descriptor
            .domain_schema_version == 14);
    CHECK(
        body_package.package->authored_json.find(
            "\"body_planar_surface\"") !=
        std::string::npos);
    CHECK(
        body_package.package->authored_json.find(
            "\"placement\"") ==
        std::string::npos);

    auto body_loaded =
        store.load(body_path);
    CHECK(body_loaded.ok());
    const auto* loaded_body_sketch =
        body_loaded.document->findSketch(
            body_sketch_id);
    CHECK(loaded_body_sketch != nullptr);
    CHECK(
        loaded_body_sketch->support ==
        *body_support);
    CHECK(
        loaded_body_sketch->model.findLine(
            body_entity) != nullptr);

    const auto catalog =
        resolvedPlanarCatalog(
            surface_reference);
    const auto original_resolution =
        part::resolveSketchSupport(
            *body_support,
            &catalog);
    const auto reloaded_resolution =
        part::resolveSketchSupport(
            loaded_body_sketch->support,
            &catalog);
    CHECK(original_resolution.valid());
    CHECK(reloaded_resolution.valid());
    CHECK(
        original_resolution ==
        reloaded_resolution);
    CHECK(
        reloaded_resolution.frame
            ->origin[2] == 10.0);

    std::cout
        << "PM02E_SKETCH_SUPPORT_SCHEMA_V9_PASS"
        << " current_schema=11"
        << " v9_migration=1"
        << " v8_migration=1"
        << " malformed_legacy_rejected=1"
        << " body_surface_roundtrip=1"
        << " placement_persisted=0"
        << '\n';

    return EXIT_SUCCESS;
}
