#include <simplesolid2/core/document.hpp>
#include <simplesolid2/part/axis.hpp>
#include <simplesolid2/part/axis_evaluation.hpp>
#include <simplesolid2/part/part_document.hpp>
#include <simplesolid2/part/part_document_store.hpp>
#include <simplesolid2/persistence/native_document_container.hpp>
#include <simplesolid2/sketch/entity_role.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-04A Axis/schema-v12 CHECK failed at line "
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
            ("simplesolid2_pm04a_" +
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

bool near(double lhs, double rhs) {
    return std::abs(lhs - rhs) < 1.0e-12;
}

std::string schema11FromCurrent(
    const persistence::NativeDocumentPackage& package) {
    auto authored =
        nlohmann::json::parse(
            package.authored_json);
    CHECK(authored.contains("next_axis_id"));
    CHECK(authored.contains("axes"));
    authored.erase("next_axis_id");
    authored.erase("axes");

    auto text = authored.dump(2);
    text.push_back('\n');

    const auto built =
        persistence::buildNativeDocumentContainer(
            persistence::NativeDocumentDescriptor{
                "part",
                package.descriptor.document_id,
                11},
            text);
    CHECK(built.ok());
    return *built.bytes;
}

} // namespace

int main() {
    CHECK(
        part::PartDocumentStore::current_schema_version ==
        14);

    part::AxisIdCursor cursor;
    const auto first = cursor.allocate();
    const auto second = cursor.allocate();
    CHECK(first.has_value());
    CHECK(second.has_value());
    CHECK(first->serialized() == "1");
    CHECK(second->serialized() == "2");
    CHECK(cursor.containsAllocated(*first));
    CHECK(cursor.containsAllocated(*second));
    CHECK(!part::AxisId::parse("0"));
    CHECK(!part::AxisId::parse("01"));
    CHECK(!part::AxisId::parse("-1"));

    auto document =
        part::PartDocument::create(
            core::DocumentId::generate());
    CHECK(document.axes().empty());
    CHECK(document.axisIdCursor().serialized() == "1");

    // Built-in Origin axes are semantic AxisReferences without synthetic
    // AxisId records.
    {
        const auto x =
            part::resolveAxisReference(
                document,
                originAxis(
                    core::BuiltinReferenceRole::x_axis));
        const auto y =
            part::resolveAxisReference(
                document,
                originAxis(
                    core::BuiltinReferenceRole::y_axis));
        const auto z =
            part::resolveAxisReference(
                document,
                originAxis(
                    core::BuiltinReferenceRole::z_axis));
        CHECK(x.valid());
        CHECK(y.valid());
        CHECK(z.valid());
        CHECK(x.line.has_value());
        CHECK(y.line.has_value());
        CHECK(z.line.has_value());
        CHECK(near(x.line->direction.x, 1.0));
        CHECK(near(x.line->direction.y, 0.0));
        CHECK(near(y.line->direction.y, 1.0));
        CHECK(near(z.line->direction.z, 1.0));
        CHECK(document.axes().empty());
    }

    const auto sketch_id =
        sketch::SketchId::generate();
    sketch::SketchModel model;
    const auto regular_line =
        model.addLine(
            {1.0, 2.0},
            {4.0, 6.0},
            sketch::EntityRole::regular);
    const auto construction_line =
        model.addLine(
            {-2.0, 1.0},
            {-2.0, 5.0},
            sketch::EntityRole::construction);
    const auto circle =
        model.addCircle(
            {0.0, 0.0},
            2.0,
            sketch::EntityRole::regular);

    auto state = document.state();
    const auto support =
        part::partSketchSupportForBuiltinPlane(
            core::BuiltinReferenceRole::xy_plane);
    CHECK(support.has_value());
    state.sketches.push_back(
        part::PartSketch{
            sketch_id,
            *support,
            true,
            std::move(model)});

    const auto regular_axis =
        state.next_axis_id.allocate();
    const auto construction_axis =
        state.next_axis_id.allocate();
    const auto wrong_kind_axis =
        state.next_axis_id.allocate();
    CHECK(regular_axis.has_value());
    CHECK(construction_axis.has_value());
    CHECK(wrong_kind_axis.has_value());

    state.axes.push_back(
        part::PartAxis{
            *regular_axis,
            "Axis 1",
            {sketch_id, regular_line},
            true});
    state.axes.push_back(
        part::PartAxis{
            *construction_axis,
            "Axis 2",
            {sketch_id, construction_line},
            false});
    state.axes.push_back(
        part::PartAxis{
            *wrong_kind_axis,
            "Axis 3",
            {sketch_id, circle},
            true});

    const auto committed =
        replaceState(document, state);
    CHECK(committed.ok());
    CHECK(committed.changed);
    CHECK(document.axes().size() == 3U);
    CHECK(document.findAxis(*regular_axis) != nullptr);
    CHECK(document.findAxis(*construction_axis) != nullptr);

    // Direction is authored Line start -> end, mapped through the current
    // Sketch frame. Regular/Construction roles do not alter Axis semantics.
    {
        const auto evaluated =
            part::resolveAxisReference(
                document,
                authoredAxis(*regular_axis));
        CHECK(evaluated.valid());
        CHECK(evaluated.line.has_value());
        CHECK(near(evaluated.line->origin.x, 1.0));
        CHECK(near(evaluated.line->origin.y, 2.0));
        CHECK(near(evaluated.line->origin.z, 0.0));
        CHECK(near(evaluated.line->direction.x, 0.6));
        CHECK(near(evaluated.line->direction.y, 0.8));
        CHECK(near(evaluated.line->direction.z, 0.0));

        const auto construction =
            part::resolveAxisReference(
                document,
                authoredAxis(*construction_axis));
        CHECK(construction.valid());
        CHECK(construction.line.has_value());
        CHECK(near(construction.line->direction.x, 0.0));
        CHECK(near(construction.line->direction.y, 1.0));
        CHECK(!document.findAxis(*construction_axis)->visible);
    }

    // An existing non-Line EntityId is a repairable Unsupported source, not
    // malformed persistence.
    {
        const auto evaluated =
            part::resolveAxisReference(
                document,
                authoredAxis(*wrong_kind_axis));
        CHECK(evaluated.valid());
        CHECK(!evaluated.line.has_value());
        CHECK(
            evaluated.status ==
            part::AxisEvaluationStatus::unsupported);
        CHECK(
            evaluated.diagnostic ==
            part::AxisEvaluationDiagnostic::source_not_line);
    }

    // Losing the source Line leaves durable Axis intent repairable as Missing.
    {
        auto missing = document.state();
        auto found =
            std::find_if(
                missing.sketches.begin(),
                missing.sketches.end(),
                [&sketch_id](const part::PartSketch& item) {
                    return item.id == sketch_id;
                });
        CHECK(found != missing.sketches.end());
        CHECK(found->model.erase(regular_line));
        const auto result =
            replaceState(document, std::move(missing));
        CHECK(result.ok());

        const auto evaluated =
            part::resolveAxisReference(
                document,
                authoredAxis(*regular_axis));
        CHECK(evaluated.valid());
        CHECK(!evaluated.line.has_value());
        CHECK(
            evaluated.status ==
            part::AxisEvaluationStatus::missing);
        CHECK(
            evaluated.diagnostic ==
            part::AxisEvaluationDiagnostic::missing_line);
        CHECK(document.findAxis(*regular_axis) != nullptr);
    }

    // Missing source Sketch is also authored repairable state.
    {
        auto missing = document.state();
        missing.axes.front().source.sketch_id =
            sketch::SketchId::generate();
        const auto result =
            replaceState(document, std::move(missing));
        CHECK(result.ok());

        const auto evaluated =
            part::resolveAxisReference(
                document,
                authoredAxis(*regular_axis));
        CHECK(evaluated.valid());
        CHECK(!evaluated.line.has_value());
        CHECK(
            evaluated.status ==
            part::AxisEvaluationStatus::missing);
        CHECK(
            evaluated.diagnostic ==
            part::AxisEvaluationDiagnostic::missing_sketch);
    }

    // Structural identity corruption fails closed.
    {
        auto invalid = document.state();
        invalid.axes.push_back(invalid.axes.front());
        const auto result =
            replaceState(document, std::move(invalid));
        CHECK(!result.ok());
        CHECK(
            result.code ==
            part::PartCommitErrorCode::invalid_state);
    }
    {
        auto invalid = document.state();
        const auto outside =
            part::AxisId::parse("999");
        CHECK(outside.has_value());
        invalid.axes.front().id = *outside;
        const auto result =
            replaceState(document, std::move(invalid));
        CHECK(!result.ok());
    }
    {
        auto invalid = document.state();
        invalid.axes.front().source.entity_id = {};
        const auto result =
            replaceState(document, std::move(invalid));
        CHECK(!result.ok());
    }

    TempDirectory temp;
    part::PartDocumentStore store;
    const auto current_path =
        temp.path / "AxisCurrent.ss2part";
    CHECK(
        store.createNew(
            current_path,
            document)
            .ok());

    const auto package =
        persistence::readNativeDocumentContainer(
            current_path);
    CHECK(package.ok());
    CHECK(
        package.package->descriptor
            .domain_schema_version == 14);

    const auto authored =
        nlohmann::json::parse(
            package.package->authored_json);
    CHECK(
        authored.at("next_axis_id")
            .get<std::string>() == "4");
    CHECK(authored.at("axes").size() == 3U);
    CHECK(
        authored.at("axes")
            .at(0)
            .at("kind")
            .get<std::string>() ==
        "sketch_line");
    CHECK(
        authored.at("axes")
            .at(0)
            .at("source_entity_id")
            .get<std::string>() ==
        regular_line.serialized());

    const auto reloaded =
        store.load(current_path);
    CHECK(reloaded.ok());
    CHECK(
        reloaded.document->axes() ==
        document.axes());
    CHECK(
        reloaded.document->axisIdCursor() ==
        document.axisIdCursor());

    // Exact v11 Axis migration adds no synthetic Axis records and preserves
    // all pre-existing authored identities.
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
        auto legacy = legacy_source.state();
        sketch::SketchModel legacy_model;
        static_cast<void>(
            legacy_model.addLine(
                {0.0, 0.0},
                {10.0, 0.0}));
        legacy.sketches.push_back(
            part::PartSketch{
                legacy_sketch_id,
                *support,
                true,
                std::move(legacy_model)});
        CHECK(
            replaceState(
                legacy_source,
                std::move(legacy))
                .ok());
    }

    const auto legacy_source_path =
        temp.path / "LegacySourceCurrent.ss2part";
    CHECK(
        store.createNew(
            legacy_source_path,
            legacy_source)
            .ok());
    const auto legacy_source_package =
        persistence::readNativeDocumentContainer(
            legacy_source_path);
    CHECK(legacy_source_package.ok());

    const auto legacy_v11_path =
        temp.path / "LegacyV11.ss2part";
    writeBytes(
        legacy_v11_path,
        schema11FromCurrent(
            *legacy_source_package.package));

    auto migrated =
        store.load(legacy_v11_path);
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
    CHECK(migrated.document->axes().empty());
    CHECK(
        migrated.document->axisIdCursor()
            .serialized() == "1");

    CHECK(
        store.save(
            legacy_v11_path,
            *migrated.document,
            *migrated.checkpoint)
            .ok());
    const auto rewritten =
        persistence::readNativeDocumentContainer(
            legacy_v11_path);
    CHECK(rewritten.ok());
    CHECK(
        rewritten.package->descriptor
            .domain_schema_version == 14);
    const auto rewritten_authored =
        nlohmann::json::parse(
            rewritten.package->authored_json);
    CHECK(
        rewritten_authored.at("next_axis_id")
            .get<std::string>() == "1");
    CHECK(rewritten_authored.at("axes").empty());

    std::cout
        << "PM-04A Axis semantics/schema-v12 compatibility tests passed\n";
    return EXIT_SUCCESS;
}
