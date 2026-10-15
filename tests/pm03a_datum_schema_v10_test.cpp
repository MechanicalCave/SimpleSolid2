#include <simplesolid2/core/document.hpp>
#include <simplesolid2/part/datum.hpp>
#include <simplesolid2/part/part_document.hpp>
#include <simplesolid2/part/part_document_store.hpp>
#include <simplesolid2/persistence/native_document_container.hpp>
#include <simplesolid2/sketch/sketch_id.hpp>

#include <nlohmann/json.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-03A Datum/schema-v10 CHECK failed at line "
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
            ("simplesolid2_pm03a_" +
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

part::OffsetDatumPlane makeDatum(
    part::DatumId id,
    part::PlaneReference source,
    double offset_mm,
    bool visible = true) {
    return part::OffsetDatumPlane{
        id,
        std::move(source),
        core::LengthValue{offset_mm},
        visible};
}

part::PartCommitResult replaceState(
    part::PartDocument& document,
    part::PartAuthoredState state) {
    part::PartDocumentTransaction tx{document};
    tx.replaceState(std::move(state));
    return tx.commit();
}

std::string schema9FromCurrent(
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
    CHECK(authored.contains("three_point_datum_planes"));
    CHECK(authored.at("three_point_datum_planes").is_array());
    CHECK(authored.at("three_point_datum_planes").empty());
    authored.erase("three_point_datum_planes");
    CHECK(authored.contains("edge_point_datum_planes"));
    CHECK(authored.at("edge_point_datum_planes").empty());
    authored.erase("edge_point_datum_planes");
    // A historical schema fixture cannot carry v15 projected bindings.
    // Refuse to downgrade a genuinely linked Sketch and never relax the
    // production legacy parser to accommodate current-schema extensions.
    for (auto& sketch_json : authored["sketches"]) {
        CHECK(sketch_json.contains("projected_edges"));
        CHECK(sketch_json.at("projected_edges").is_array());
        CHECK(sketch_json.at("projected_edges").empty());
        sketch_json.erase("projected_edges");
    }

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

std::string schema16FromCurrent(
    const persistence::NativeDocumentPackage& package) {
    auto authored = nlohmann::json::parse(package.authored_json);
    CHECK(authored.contains("edge_point_datum_planes"));
    CHECK(authored.at("edge_point_datum_planes").is_array());
    // Never downgrade a v17 file with real Edge-local authored intent.
    CHECK(authored.at("edge_point_datum_planes").empty());
    authored.erase("edge_point_datum_planes");
    auto text = authored.dump(2);
    text.push_back('\n');
    const auto built = persistence::buildNativeDocumentContainer(
        persistence::NativeDocumentDescriptor{
            "part",package.descriptor.document_id,16},text);
    CHECK(built.ok());
    return *built.bytes;
}

} // namespace

int main() {
    CHECK(
        part::PartDocumentStore::current_schema_version ==
        18);

    // DatumId follows the existing Part-local canonical positive-decimal and
    // high-water allocation rules.
    part::DatumIdCursor cursor;
    const auto first = cursor.allocate();
    const auto second = cursor.allocate();
    CHECK(first.has_value());
    CHECK(second.has_value());
    CHECK(first->serialized() == "1");
    CHECK(second->serialized() == "2");
    CHECK(cursor.containsAllocated(*first));
    CHECK(cursor.containsAllocated(*second));
    CHECK(!part::DatumId::parse("0"));
    CHECK(!part::DatumId::parse("01"));
    CHECK(!part::DatumId::parse("-1"));

    auto document =
        part::PartDocument::create(
            core::DocumentId::generate());
    CHECK(document.datumPlanes().empty());
    CHECK(
        document.datumIdCursor().serialized() ==
        "1");

    // A valid Origin-backed Datum chain is accepted and preserves semantic
    // identity independently of visibility and geometric coincidence.
    auto valid = document.state();
    const auto datum_a =
        valid.next_datum_id.allocate();
    const auto datum_b =
        valid.next_datum_id.allocate();
    CHECK(datum_a.has_value());
    CHECK(datum_b.has_value());

    valid.datum_planes.push_back(
        makeDatum(
            *datum_a,
            originPlane(
                core::BuiltinReferenceRole::xy_plane),
            10.0));
    valid.datum_planes.push_back(
        makeDatum(
            *datum_b,
            datumPlane(*datum_a),
            0.0,
            false));

    const auto committed =
        replaceState(document, valid);
    CHECK(committed.ok());
    CHECK(committed.changed);
    CHECK(document.datumPlanes().size() == 2U);
    CHECK(document.findDatumPlane(*datum_a) != nullptr);
    CHECK(document.findDatumPlane(*datum_b) != nullptr);
    CHECK(
        document.findDatumPlane(*datum_b)
            ->offset.millimetres == 0.0);
    CHECK(
        !document.findDatumPlane(*datum_b)
             ->visible);

    // Direct/indirect local Datum cycles fail closed.
    {
        auto invalid = document.state();
        invalid.datum_planes[0].source =
            datumPlane(*datum_b);
        const auto result =
            replaceState(document, invalid);
        CHECK(!result.ok());
        CHECK(
            result.code ==
            part::PartCommitErrorCode::
                invalid_state);
        CHECK(!result.changed);
    }

    // Missing Datum source fails closed.
    {
        auto invalid = document.state();
        const auto missing =
            invalid.next_datum_id.allocate();
        CHECK(missing.has_value());
        invalid.datum_planes[1].source =
            datumPlane(*missing);
        const auto result =
            replaceState(document, invalid);
        CHECK(!result.ok());
        CHECK(!result.changed);
    }

    // A semantic Body Surface source must resolve to declared authored
    // producer/stage identities, not merely contain syntactically valid IDs.
    {
        auto invalid = document.state();
        const auto absent_feature =
            invalid.body.next_feature_id.allocate();
        CHECK(absent_feature.has_value());

        part::FeatureSurfaceAddress address;
        address.producer_feature_id =
            *absent_feature;
        address.role =
            part::FeatureSurfaceRoleKind::
                extent_cap;

        part::SurfaceReference surface{
            part::BodyStageRef{
                part::BodyStageKind::
                    after_feature,
                *absent_feature},
            address};

        const auto id =
            invalid.next_datum_id.allocate();
        CHECK(id.has_value());
        invalid.datum_planes.push_back(
            makeDatum(
                *id,
                part::PlaneReference{
                    part::BodyPlanarSurfacePlaneReference{
                        surface}},
                4.0));

        const auto result =
            replaceState(document, invalid);
        CHECK(!result.ok());
        CHECK(!result.changed);
    }

    // Non-finite authored offset is never accepted.
    {
        auto invalid = document.state();
        invalid.datum_planes[0].offset =
            core::LengthValue{
                std::numeric_limits<double>::
                    quiet_NaN()};
        const auto result =
            replaceState(document, invalid);
        CHECK(!result.ok());
        CHECK(!result.changed);
    }

    // Duplicate durable identity is invalid even if both records are otherwise
    // structurally sound.
    {
        auto invalid = document.state();
        invalid.datum_planes.push_back(
            makeDatum(
                *datum_a,
                originPlane(
                    core::BuiltinReferenceRole::
                        yz_plane),
                -2.0));
        const auto result =
            replaceState(document, invalid);
        CHECK(!result.ok());
        CHECK(!result.changed);
    }

    TempDirectory temp;
    part::PartDocumentStore store;
    const auto current_path =
        temp.path / "DatumCurrent.ss2part";

    const auto created =
        store.createNew(
            current_path,
            document);
    CHECK(created.ok());

    const auto package =
        persistence::readNativeDocumentContainer(
            current_path);
    CHECK(package.ok());
    CHECK(
        package.package->descriptor
            .domain_schema_version == 18);

    const auto authored =
        nlohmann::json::parse(
            package.package->authored_json);
    CHECK(
        authored.at("next_datum_id")
            .get<std::string>() == "3");
    CHECK(
        authored.at("datum_planes")
            .size() == 2U);
    CHECK(
        authored.at("datum_planes")
            .at(0)
            .at("offset_mm")
            .get<double>() == 10.0);
    CHECK(
        authored.at("datum_planes")
            .at(1)
            .at("source")
            .at("kind")
            .get<std::string>() ==
        "datum_plane");

    CHECK(authored.at("edge_point_datum_planes").is_array());
    CHECK(authored.at("edge_point_datum_planes").empty());
    CHECK(authored.at("three_point_datum_planes").is_array());
    CHECK(authored.at("three_point_datum_planes").empty());

    const auto reloaded =
        store.load(current_path);
    CHECK(reloaded.ok());
    CHECK(
        reloaded.document->datumPlanes() ==
        document.datumPlanes());
    CHECK(
        reloaded.document->datumIdCursor() ==
        document.datumIdCursor());

    // An authentic Offset-only v16 native package remains readable.
    // Guarded next save upgrades it to v18 without renumbering DatumIds.
    const auto legacy_v16_path = temp.path / "LegacyOffsetV16.ss2part";
    writeBytes(legacy_v16_path,schema16FromCurrent(*package.package));
    const auto legacy_v16 = store.load(legacy_v16_path);
    CHECK(legacy_v16.ok());
    CHECK(legacy_v16.document->datumPlanes() ==
          document.datumPlanes());
    CHECK(legacy_v16.document->edgePointDatumPlanes().empty());
    CHECK(legacy_v16.document->datumIdCursor() ==
          document.datumIdCursor());
    CHECK(store.save(legacy_v16_path,*legacy_v16.document,
                     *legacy_v16.checkpoint).ok());
    const auto upgraded =
        persistence::readNativeDocumentContainer(legacy_v16_path);
    CHECK(upgraded.ok());
    CHECK(upgraded.package->descriptor.domain_schema_version == 18);
    const auto upgraded_json =
        nlohmann::json::parse(upgraded.package->authored_json);
    CHECK(upgraded_json.at("datum_planes") ==
          authored.at("datum_planes"));
    CHECK(upgraded_json.at("edge_point_datum_planes").empty());
    CHECK(upgraded_json.at("three_point_datum_planes").empty());

    // Genuine v17 Offset+EdgePoint grammar is backward readable and
    // next-save upgrade adds only an EMPTY v18 triple-Point collection.
    auto legacy17_json = authored;
    CHECK(legacy17_json.at("three_point_datum_planes").empty());
    legacy17_json.erase("three_point_datum_planes");
    const auto legacy17_bytes =
        persistence::buildNativeDocumentContainer(
            persistence::NativeDocumentDescriptor{
                "part",std::string{document.documentId().value()},17},
            legacy17_json.dump(2)+"\\n");
    CHECK(legacy17_bytes.ok());
    const auto legacy17_path = temp.path/"LegacyOffsetV17.ss2part";
    writeBytes(legacy17_path,*legacy17_bytes.bytes);
    const auto legacy17 = store.load(legacy17_path);
    CHECK(legacy17.ok());
    CHECK(legacy17.document->datumPlanes() == document.datumPlanes());
    CHECK(legacy17.document->threePointDatumPlanes().empty());
    CHECK(store.save(legacy17_path,*legacy17.document,
                     *legacy17.checkpoint).ok());
    const auto legacy17_upgraded =
        persistence::readNativeDocumentContainer(legacy17_path);
    CHECK(legacy17_upgraded.ok());
    CHECK(legacy17_upgraded.package->descriptor.domain_schema_version==18);
    CHECK(nlohmann::json::parse(
        legacy17_upgraded.package->authored_json)
        .at("three_point_datum_planes").empty());

    // v18 unknown 3-Point kind and malformed payload must fail closed.
    auto future18_json=authored;
    future18_json["three_point_datum_planes"].push_back({
        {"id","1"},{"kind","future_three_point_plane"},
        {"points",nlohmann::json::array()},
        {"orientation",{{"reverse",false}}},{"visible",true}});
    const auto invalid_v18 =
        persistence::buildNativeDocumentContainer(
            persistence::NativeDocumentDescriptor{
                "part",std::string{document.documentId().value()},18},
            future18_json.dump(2)+"\\n");
    CHECK(invalid_v18.ok());
    const auto invalid_v18_path = temp.path/"UnknownV18ThreePoint.ss2part";
    writeBytes(invalid_v18_path,*invalid_v18.bytes);
    CHECK(!store.load(invalid_v18_path).ok());

    // Unknown future v17 Datum types must be rejected, not skipped.
    auto future_json = authored;
    CHECK(future_json.at("three_point_datum_planes").empty());
    future_json.erase("three_point_datum_planes");
    future_json["edge_point_datum_planes"].push_back({
        {"id","1"},{"kind","future_edge_point_plane"},
        {"source",nlohmann::json::object()},
        {"orientation",nlohmann::json::object()},
        {"visible",true}});
    auto future_text = future_json.dump(2);
    future_text.push_back('\n');
    const auto future_bytes =
        persistence::buildNativeDocumentContainer(
            persistence::NativeDocumentDescriptor{
                "part",std::string{document.documentId().value()},17},future_text);
    CHECK(future_bytes.ok());
    const auto future_path = temp.path / "UnknownV17Datum.ss2part";
    writeBytes(future_path,*future_bytes.bytes);
    CHECK(!store.load(future_path).ok());

    // Exact v9 -> v10 migration: v9 has no Datum fields. Loading yields an
    // empty Datum collection/high-water cursor while preserving existing
    // document/body/sketch identity, then the next save emits current v18.
    const auto legacy_source_path =
        temp.path / "LegacySourceCurrent.ss2part";
    auto legacy_source =
        part::PartDocument::create(
            core::DocumentId::generate());
    const auto legacy_document_id =
        legacy_source.documentId();
    const auto legacy_body_id =
        legacy_source.body().id;
    const auto legacy_sketch_id =
        sketch::SketchId::generate();

    {
        auto state = legacy_source.state();
        auto support =
            part::partSketchSupportForBuiltinPlane(
                core::BuiltinReferenceRole::
                    xz_plane);
        CHECK(support.has_value());
        state.sketches.push_back(
            part::PartSketch{
                legacy_sketch_id,
                std::move(*support),
                true,
                sketch::SketchModel{}});
        const auto result =
            replaceState(
                legacy_source,
                std::move(state));
        CHECK(result.ok());
        CHECK(result.changed);
    }

    CHECK(
        store.createNew(
            legacy_source_path,
            legacy_source)
            .ok());
    const auto legacy_source_package =
        persistence::readNativeDocumentContainer(
            legacy_source_path);
    CHECK(legacy_source_package.ok());

    const auto legacy_v9_path =
        temp.path / "LegacyV9.ss2part";
    writeBytes(
        legacy_v9_path,
        schema9FromCurrent(
            *legacy_source_package.package));

    auto migrated =
        store.load(legacy_v9_path);
    CHECK(migrated.ok());
    CHECK(
        migrated.document->documentId() ==
        legacy_document_id);
    CHECK(
        migrated.document->body().id ==
        legacy_body_id);
    CHECK(
        migrated.document->findSketch(
            legacy_sketch_id) != nullptr);
    CHECK(migrated.document->datumPlanes().empty());
    CHECK(
        migrated.document->datumIdCursor()
            .serialized() == "1");

    CHECK(
        store.save(
            legacy_v9_path,
            *migrated.document,
            *migrated.checkpoint)
            .ok());

    const auto rewritten =
        persistence::readNativeDocumentContainer(
            legacy_v9_path);
    CHECK(rewritten.ok());
    CHECK(
        rewritten.package->descriptor
            .domain_schema_version == 18);
    const auto rewritten_authored =
        nlohmann::json::parse(
            rewritten.package->authored_json);
    CHECK(
        rewritten_authored.at("next_datum_id")
            .get<std::string>() == "1");
    CHECK(
        rewritten_authored.at("datum_planes")
            .empty());

    std::cout
        << "PM-03A Datum semantics and schema-v10 tests passed\n";
    return EXIT_SUCCESS;
}
