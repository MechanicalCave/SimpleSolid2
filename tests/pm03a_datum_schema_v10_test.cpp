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
    authored.erase("next_datum_id");
    authored.erase("datum_planes");

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

} // namespace

int main() {
    CHECK(
        part::PartDocumentStore::current_schema_version ==
        11);

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
            .domain_schema_version == 11);

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

    const auto reloaded =
        store.load(current_path);
    CHECK(reloaded.ok());
    CHECK(
        reloaded.document->datumPlanes() ==
        document.datumPlanes());
    CHECK(
        reloaded.document->datumIdCursor() ==
        document.datumIdCursor());

    // Exact v9 -> v10 migration: v9 has no Datum fields. Loading yields an
    // empty Datum collection/high-water cursor while preserving existing
    // document/body/sketch identity, then the next save emits schema v10.
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
            .domain_schema_version == 11);
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
