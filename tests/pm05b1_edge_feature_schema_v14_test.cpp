#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/part/feature.hpp>
#include <simplesolid2/part/part_document_store.hpp>
#include <simplesolid2/persistence/native_document_container.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-05B1 schema-v14 CHECK failed at line "
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
            ("simplesolid2_pm05b1_" +
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

std::string repackage(
    const persistence::NativeDocumentPackage& package,
    int schema_version,
    std::string authored_json) {
    if (schema_version < 15) {
        auto authored =
            nlohmann::json::parse(
                authored_json, nullptr, false);
        CHECK(authored.is_object());
        for (auto& sketch : authored["sketches"]) {
            sketch.erase("projected_edges");
        }
        authored_json = authored.dump(2) + "\n";
    }
    const auto built =
        persistence::buildNativeDocumentContainer(
            persistence::NativeDocumentDescriptor{
                "part",
                package.descriptor.document_id,
                schema_version},
            std::move(authored_json));
    CHECK(built.ok());
    return std::move(*built.bytes);
}

part::FeatureSurfaceAddress capSurface(
    part::FeatureId producer) {
    return {
        producer,
        part::FeatureSurfaceRoleKind::extent_cap,
        std::nullopt,
        0U,
        0U,
        false};
}

part::FeatureSurfaceAddress sideSurface(
    part::FeatureId producer,
    sketch::EntityId source,
    std::uint32_t use_index) {
    return {
        producer,
        part::FeatureSurfaceRoleKind::side,
        source,
        0U,
        use_index,
        false};
}

part::FeatureCurveAddress curveBetween(
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

part::FeaturePointAddress pointAt(
    part::FeatureId producer,
    part::FeatureSurfaceAddress first,
    part::FeatureSurfaceAddress second,
    part::FeatureSurfaceAddress third) {
    std::vector<part::FeatureSurfaceAddress>
        surfaces{
            std::move(first),
            std::move(second),
            std::move(third)};
    std::sort(
        surfaces.begin(),
        surfaces.end());
    part::FeaturePointAddress result{
        producer,
        std::move(surfaces)};
    CHECK(result.valid());
    return result;
}

struct Fixture final {
    part::PartDocument document;
    part::FeatureId base_id;
    part::FeatureId fillet_id;
    part::FeatureId chamfer_id;
};

Fixture makeFixture() {
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
                {0.0, 0.0},
                {40.0, 30.0},
                sketch::EntityRole::regular,
                false});
    CHECK(rectangle.ok());
    CHECK(rectangle.entity_ids.size() == 4U);

    const auto* sketch =
        session.document().findSketch(
            sketch_id);
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
                sketch_id,
                session.document().revision(),
                *intent});
    CHECK(profile.ok() && profile.profile_id);

    auto state =
        session.document().state();
    const auto base_id =
        state.body.next_feature_id.allocate();
    const auto fillet_id =
        state.body.next_feature_id.allocate();
    const auto chamfer_id =
        state.body.next_feature_id.allocate();
    CHECK(base_id && fillet_id && chamfer_id);

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
        capSurface(*base_id);
    const auto side0 =
        sideSurface(
            *base_id,
            rectangle.entity_ids.at(0),
            0U);
    const auto side1 =
        sideSurface(
            *base_id,
            rectangle.entity_ids.at(1),
            1U);
    const auto side2 =
        sideSurface(
            *base_id,
            rectangle.entity_ids.at(2),
            2U);

    auto edge0 =
        part::MaterialEdgeReference{
            {
                part::BodyStageKind::after_feature,
                *base_id},
            curveBetween(
                *base_id,
                cap,
                side0),
            part::SingularAtAuthoredStage{}};
    auto edge1 =
        part::MaterialEdgeReference{
            {
                part::BodyStageKind::after_feature,
                *base_id},
            curveBetween(
                *base_id,
                cap,
                side1),
            part::SingularAtAuthoredStage{}};
    CHECK(edge0.valid());
    CHECK(edge1.valid());

    std::vector<part::MaterialEdgeReference>
        fillet_edges{edge1, edge0};
    std::sort(
        fillet_edges.begin(),
        fillet_edges.end());

    state.body.features.push_back(
        part::PartFeature{
            *fillet_id,
            "Fillet001",
            false,
            part::FilletFeature{
                std::move(fillet_edges),
                core::LengthValue{2.0}}});

    auto first_point =
        pointAt(
            *base_id,
            cap,
            side0,
            side1);
    auto second_point =
        pointAt(
            *base_id,
            cap,
            side0,
            side2);
    if (second_point < first_point) {
        std::swap(
            first_point,
            second_point);
    }

    part::MaterialEdgeReference
        chamfer_edge{
            {
                part::BodyStageKind::after_feature,
                *fillet_id},
            curveBetween(
                *base_id,
                cap,
                side0),
            part::BetweenSemanticPoints{
                std::move(first_point),
                std::move(second_point)}};
    CHECK(chamfer_edge.valid());

    state.body.features.push_back(
        part::PartFeature{
            *chamfer_id,
            "Chamfer001",
            false,
            part::ChamferFeature{
                {std::move(chamfer_edge)},
                core::LengthValue{1.0}}});

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
        *chamfer_id};
}

void verifyPg01bB1BindingStructure(
    const Fixture& fixture) {
    const auto base =
        fixture.document.state();
    CHECK(!base.sketches.empty());
    const auto* fillet =
        std::get_if<part::FilletFeature>(
            &base.body.features.at(1).definition);
    CHECK(fillet && !fillet->edges.empty());
    const auto valid_source =
        fillet->edges.front();
    CHECK(valid_source.valid());

    auto withLink = [&]() {
        auto next = base;
        const auto support =
            part::partSketchSupportForBuiltinPlane(
                core::BuiltinReferenceRole::xy_plane);
        CHECK(support);
        part::PartSketch target{
            sketch::SketchId::generate(),
            *support, true, {}};
        const auto entity =
            target.model.addLine(
                {1.0, 2.0}, {3.0, 4.0},
                sketch::EntityRole::regular);
        CHECK(entity.valid());
        target.projection_bindings.push_back(
            {entity, valid_source});
        next.sketches.push_back(std::move(target));
        return next;
    };
    auto original = withLink();
    const auto& linked = original.sketches.back();
    CHECK(linked.projection_bindings.size() == 1U);
    CHECK(linked.projection_bindings[0].valid());
    const auto accepted =
        part::PartDocument::restore(
            fixture.document.documentId(), original);
    CHECK(accepted.ok());

    // A binding is neither a detached geometry copy nor persistent BRep.
    // Until B5 schema v15, Save must explicitly fail, not drop the link.
    TempDirectory tmp;
    part::PartDocumentStore store;
    const auto unsaved = store.createNew(
        tmp.path / "UnsafeProjection.ss2part",
        *accepted.document);
    CHECK(!unsaved.ok());

    auto missing_target = withLink();
    missing_target.sketches.back()
        .projection_bindings[0].target_entity =
            *sketch::EntityId::parse("9999");
    CHECK(!part::PartDocument::restore(
        fixture.document.documentId(),
        std::move(missing_target)).ok());

    auto duplicate = withLink();
    duplicate.sketches.back()
        .projection_bindings.push_back(
            duplicate.sketches.back()
                .projection_bindings.front());
    CHECK(!part::PartDocument::restore(
        fixture.document.documentId(),
        std::move(duplicate)).ok());

    auto unallocated = withLink();
    unallocated.sketches.back()
        .projection_bindings[0].source.stage.feature_id =
            *part::FeatureId::parse("9999");
    CHECK(!part::PartDocument::restore(
        fixture.document.documentId(),
        std::move(unallocated)).ok());

    // Authored source after target-consuming Feature is a cycle.
    auto self_cycle = base;
    CHECK(!self_cycle.sketches.empty());
    const auto source_entity =
        self_cycle.sketches.front()
            .model.state().lines.front().id;
    self_cycle.sketches.front()
        .projection_bindings.push_back(
            {source_entity, valid_source});
    CHECK(!part::PartDocument::restore(
        fixture.document.documentId(),
        std::move(self_cycle)).ok());

    std::cout << "PG01B_B1_STRUCTURE_PASS"
              << " stable_entity=1"
              << " duplicate_reject=1"
              << " stage_cycle_reject=1"
              << " unsafe_save_reject=1\n";
}

} // namespace

int main() {
    CHECK(
        part::PartDocumentStore::current_schema_version ==
        15);

    const auto fixture = makeFixture();
    verifyPg01bB1BindingStructure(fixture);
    CHECK(
        fixture.document.body().features.size() ==
        3U);

    const auto* fillet_feature =
        fixture.document.findFeature(
            fixture.fillet_id);
    CHECK(fillet_feature != nullptr);
    const auto* fillet =
        std::get_if<part::FilletFeature>(
            &fillet_feature->definition);
    CHECK(fillet != nullptr);
    CHECK(fillet->edges.size() == 2U);
    CHECK(std::is_sorted(
        fillet->edges.begin(),
        fillet->edges.end()));
    CHECK(
        fillet->edges.front().stage.feature_id ==
        fixture.base_id);

    const auto* chamfer_feature =
        fixture.document.findFeature(
            fixture.chamfer_id);
    CHECK(chamfer_feature != nullptr);
    const auto* chamfer =
        std::get_if<part::ChamferFeature>(
            &chamfer_feature->definition);
    CHECK(chamfer != nullptr);
    CHECK(chamfer->edges.size() == 1U);
    CHECK(
        std::holds_alternative<
            part::BetweenSemanticPoints>(
            chamfer->edges.front().branch));

    // Canonical set ordering and duplicate rejection are authored-state
    // invariants, not presentation cleanup.
    {
        auto invalid =
            fixture.document.state();
        auto* feature =
            std::get_if<part::FilletFeature>(
                &invalid.body.features.at(1)
                     .definition);
        CHECK(feature != nullptr);
        std::reverse(
            feature->edges.begin(),
            feature->edges.end());
        const auto restored =
            part::PartDocument::restore(
                fixture.document.documentId(),
                std::move(invalid));
        CHECK(!restored.ok());
    }
    {
        auto invalid =
            fixture.document.state();
        auto* feature =
            std::get_if<part::FilletFeature>(
                &invalid.body.features.at(1)
                     .definition);
        CHECK(feature != nullptr);
        feature->edges.push_back(
            feature->edges.front());
        std::sort(
            feature->edges.begin(),
            feature->edges.end());
        const auto restored =
            part::PartDocument::restore(
                fixture.document.documentId(),
                std::move(invalid));
        CHECK(!restored.ok());
    }
    {
        auto invalid =
            fixture.document.state();
        auto* feature =
            std::get_if<part::FilletFeature>(
                &invalid.body.features.at(1)
                     .definition);
        CHECK(feature != nullptr);
        feature->edges.front().stage =
            {
                part::BodyStageKind::after_feature,
                fixture.fillet_id};
        std::sort(
            feature->edges.begin(),
            feature->edges.end());
        const auto restored =
            part::PartDocument::restore(
                fixture.document.documentId(),
                std::move(invalid));
        CHECK(!restored.ok());
    }

    // Structural persistence must preserve repairable edge intent when a
    // previously consumed/producing upstream Feature is later deleted.
    // Resolution becomes Missing/Blocked in B2; B1 must not reject the
    // authored document merely because the historical producer is absent.
    {
        auto broken =
            fixture.document.state();
        CHECK(
            broken.body.features.front().id ==
            fixture.base_id);
        broken.body.features.erase(
            broken.body.features.begin());

        auto restored =
            part::PartDocument::restore(
                fixture.document.documentId(),
                std::move(broken));
        CHECK(restored.ok());
        CHECK(
            restored.document->findFeature(
                fixture.base_id) == nullptr);

        const auto* retained =
            restored.document->findFeature(
                fixture.fillet_id);
        CHECK(retained != nullptr);
        const auto* retained_fillet =
            std::get_if<part::FilletFeature>(
                &retained->definition);
        CHECK(retained_fillet != nullptr);
        CHECK(
            retained_fillet->edges.front()
                .stage.feature_id ==
            fixture.base_id);
    }

    TempDirectory temp;
    part::PartDocumentStore store;

    const auto current_path =
        temp.path / "EdgeFeaturesCurrent.ss2part";
    CHECK(
        store.createNew(
            current_path,
            fixture.document)
            .ok());

    const auto package =
        persistence::readNativeDocumentContainer(
            current_path);
    CHECK(package.ok());
    CHECK(
        package.package->descriptor
            .domain_schema_version == 15);

    const auto authored =
        nlohmann::json::parse(
            package.package->authored_json);
    const auto& features =
        authored.at("body").at("features");
    CHECK(features.size() == 3U);
    CHECK(features.at(1).at("kind") == "fillet");
    CHECK(
        features.at(1).at("edges").size() ==
        2U);
    CHECK(
        features.at(1).at("radius_mm") ==
        2.0);
    CHECK(
        features.at(1)
            .at("edges")
            .at(0)
            .at("branch")
            .at("kind") ==
        "singular_at_authored_stage");
    CHECK(features.at(2).at("kind") == "chamfer");
    CHECK(
        features.at(2).at("distance_mm") ==
        1.0);
    CHECK(
        features.at(2)
            .at("edges")
            .at(0)
            .at("branch")
            .at("kind") ==
        "between_semantic_points");

    // Save/close/reopen retains semantic references exactly and contains no
    // provider topology tokens or geometric fallback coordinates.
    const auto loaded =
        store.load(current_path);
    CHECK(loaded.ok());
    CHECK(
        loaded.document->state() ==
        fixture.document.state());
    CHECK(
        package.package->authored_json.find(
            "runtime_token") ==
        std::string::npos);
    CHECK(
        package.package->authored_json.find(
            "provider") ==
        std::string::npos);

    // A schema-v13 authored state without Edge Features remains loadable and
    // is written back as schema v15 without rewriting existing identities.
    auto legacy_state =
        fixture.document.state();
    legacy_state.body.features.resize(1U);
    legacy_state.body.next_feature_id =
        fixture.document.body()
            .next_feature_id;
    auto legacy_document =
        part::PartDocument::restore(
            fixture.document.documentId(),
            std::move(legacy_state));
    CHECK(legacy_document.ok());

    const auto legacy_current_path =
        temp.path / "LegacySourceCurrent.ss2part";
    CHECK(
        store.createNew(
            legacy_current_path,
            *legacy_document.document)
            .ok());
    const auto legacy_current =
        persistence::readNativeDocumentContainer(
            legacy_current_path);
    CHECK(legacy_current.ok());

    const auto legacy_v13_path =
        temp.path / "LegacyV13.ss2part";
    writeBytes(
        legacy_v13_path,
        repackage(
            *legacy_current.package,
            13,
            legacy_current.package
                ->authored_json));
    const auto legacy_loaded =
        store.load(legacy_v13_path);
    CHECK(legacy_loaded.ok());
    CHECK(
        legacy_loaded.document->state() ==
        legacy_document.document->state());

    const auto migrated_path =
        temp.path / "MigratedV14.ss2part";
    CHECK(
        store.createNew(
            migrated_path,
            *legacy_loaded.document)
            .ok());
    const auto migrated =
        persistence::readNativeDocumentContainer(
            migrated_path);
    CHECK(migrated.ok());
    CHECK(
        migrated.package->descriptor
            .domain_schema_version == 15);

    // New Edge Feature records are schema-v14-only and must fail closed when
    // deliberately mislabeled as a legacy v13 payload.
    const auto mislabeled_path =
        temp.path / "MislabeledV13.ss2part";
    writeBytes(
        mislabeled_path,
        repackage(
            *package.package,
            13,
            package.package->authored_json));
    const auto mislabeled =
        store.load(mislabeled_path);
    CHECK(!mislabeled.ok());
    CHECK(
        mislabeled.diagnostic.code ==
        part::PartStoreErrorCode::
            malformed_document);

    // Persistence also rejects malformed/noncanonical durable intent.
    {
        auto bad = authored;
        bad["body"]["features"][1]["radius_mm"] =
            0.0;
        auto text = bad.dump(2);
        text.push_back('\n');
        const auto path =
            temp.path / "BadRadius.ss2part";
        writeBytes(
            path,
            repackage(
                *package.package,
                14,
                std::move(text)));
        CHECK(!store.load(path).ok());
    }
    {
        auto bad = authored;
        bad["body"]["features"][1]["edges"].push_back(
            bad["body"]["features"][1]["edges"].at(0));
        auto text = bad.dump(2);
        text.push_back('\n');
        const auto path =
            temp.path / "DuplicateEdge.ss2part";
        writeBytes(
            path,
            repackage(
                *package.package,
                14,
                std::move(text)));
        CHECK(!store.load(path).ok());
    }
    {
        auto bad = authored;
        bad["body"]["features"][1]["edges"][0]
           ["stage"]["feature_id"] =
            fixture.fillet_id.serialized();
        auto text = bad.dump(2);
        text.push_back('\n');
        const auto path =
            temp.path / "SelfStage.ss2part";
        writeBytes(
            path,
            repackage(
                *package.package,
                14,
                std::move(text)));
        CHECK(!store.load(path).ok());
    }

    std::cout
        << "PM05B1_EDGE_FEATURE_SCHEMA_V14_PASS"
        << " schema=15"
        << " edge_feature_kinds=2"
        << " provider_identity_persisted=0\n";
    return EXIT_SUCCESS;
}
