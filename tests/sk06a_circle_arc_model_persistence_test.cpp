#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/part/part_document_store.hpp>
#include <simplesolid2/persistence/native_document_container.hpp>

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <numbers>
#include <stdexcept>
#include <string>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "SK-06A Circle/Arc model/persistence CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(...) \
    check(static_cast<bool>((__VA_ARGS__)), #__VA_ARGS__, __LINE__)

struct TempDirectory final {
    std::filesystem::path path;

    TempDirectory() {
        path =
            std::filesystem::temp_directory_path() /
            ("simplesolid2_sk06a_model_" +
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
        static_cast<std::streamsize>(bytes.size()));
    CHECK(static_cast<bool>(out));
}

std::string buildV4PackageWithModel(
    const core::DocumentId& document_id,
    const sketch::SketchId& sketch_id,
    const std::string& model_json) {
    const std::string authored =
        std::string{
            "{"
            "\"properties\":{"
                "\"number\":\"\","
                "\"title\":\"\","
                "\"description\":\"\","
                "\"engineering_revision\":\"\""
            "},"
            "\"presentation\":{"
                "\"builtin_reference_visibility_mask\":15"
            "},"
            "\"sketches\":[{"
                "\"id\":\""} +
        std::string{sketch_id.value()} +
        "\","
        "\"support\":{"
            "\"kind\":\"builtin_origin_plane\","
            "\"builtin_plane\":\"xy_plane\""
        "},"
        "\"placement\":{"
            "\"origin\":[0,0,0],"
            "\"u_axis\":[1,0,0],"
            "\"v_axis\":[0,1,0]"
        "},"
        "\"visible\":true,"
        "\"model\":" +
        model_json +
        "}]}";

    const auto built =
        persistence::buildNativeDocumentContainer(
            persistence::NativeDocumentDescriptor{
                "part",
                std::string{document_id.value()},
                4,
            },
            authored);
    CHECK(built.ok());
    return *built.bytes;
}

} // namespace

int main() {
    constexpr double pi =
        std::numbers::pi_v<double>;

    sketch::SketchModel model;
    const auto line =
        model.addLine({0.0, 0.0}, {10.0, 0.0});
    const auto circle =
        model.addCircle({5.0, 5.0}, 3.0);
    const auto arc =
        model.addArc({2.0, 3.0}, 4.0, 0.25, -pi);

    CHECK(line.serialized() == "1");
    CHECK(circle.serialized() == "2");
    CHECK(arc.serialized() == "3");
    CHECK(model.entityCount() == 3U);
    CHECK(model.contains(line));
    CHECK(model.contains(circle));
    CHECK(model.contains(arc));
    CHECK(model.findCircle(circle) != nullptr);
    CHECK(model.findArc(arc) != nullptr);
    CHECK(model.findCircle(circle)->radius() == 3.0);
    CHECK(model.findArc(arc)->sweepAngle() == -pi);

    bool invalid_circle = false;
    try {
        static_cast<void>(
            model.addCircle({0.0, 0.0}, 0.0));
    } catch (const std::invalid_argument&) {
        invalid_circle = true;
    }
    CHECK(invalid_circle);

    bool invalid_arc = false;
    try {
        static_cast<void>(
            model.addArc(
                {0.0, 0.0},
                1.0,
                0.0,
                2.0 * pi));
    } catch (const std::invalid_argument&) {
        invalid_arc = true;
    }
    CHECK(invalid_arc);

    const auto state = model.state();
    const auto restored =
        sketch::SketchModel::restore(state);
    CHECK(restored.has_value());
    CHECK(restored->state() == state);
    CHECK(restored->entityIdCursor().serialized() == "4");

    auto duplicate_state = state;
    duplicate_state.circles.front().id =
        duplicate_state.lines.front().id;
    CHECK(!sketch::SketchModel::restore(
        std::move(duplicate_state))
        .has_value());

    TempDirectory temp;
    part::PartDocumentStore store;
    const auto path =
        temp.path / "CircleArc.ss2part";
    auto document =
        part::PartDocument::create(
            core::DocumentId::generate());
    CHECK(store.createNew(path, document).ok());

    application::DocumentSession session{
        path,
        std::move(document)};

    const auto created =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
    CHECK(created.ok() && created.sketch_id.has_value());
    const auto sketch_id = *created.sketch_id;

    const auto line_result =
        session.execute(
            application::AddSketchLineCommand{
                sketch_id,
                {0.0, 0.0},
                {10.0, 0.0}});
    const auto circle_result =
        session.execute(
            application::AddSketchCircleCommand{
                sketch_id,
                {5.0, 5.0},
                2.0});
    const auto arc_result =
        session.execute(
            application::AddSketchArcCommand{
                sketch_id,
                {3.0, 4.0},
                5.0,
                0.0,
                pi * 0.75});

    CHECK(line_result.ok() && line_result.entity_id.has_value());
    CHECK(circle_result.ok() && circle_result.entity_id.has_value());
    CHECK(arc_result.ok() && arc_result.entity_id.has_value());
    CHECK(line_result.entity_id->serialized() == "1");
    CHECK(circle_result.entity_id->serialized() == "2");
    CHECK(arc_result.entity_id->serialized() == "3");

    const auto baseline_undo = session.undoDepth();
    const auto revision = session.document().revision();
    const auto updated =
        session.execute(
            application::UpdateSketchGeometryCommand{
                sketch_id,
                revision,
                {
                    application::SketchLineGeometryUpdate{
                        *line_result.entity_id,
                        {1.0, 2.0},
                        {11.0, 2.0}},
                },
                {
                    application::SketchCircleGeometryUpdate{
                        *circle_result.entity_id,
                        {6.0, 7.0},
                        4.0},
                },
                {
                    application::SketchArcGeometryUpdate{
                        *arc_result.entity_id,
                        {4.0, 5.0},
                        6.0,
                        0.5,
                        -pi * 0.5},
                }});
    CHECK(updated.ok() && updated.changed);
    CHECK(session.undoDepth() == baseline_undo + 1U);

    const auto* hosted =
        session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    CHECK(
        hosted->model.findLine(*line_result.entity_id)
            ->start() == sketch::Point2{1.0, 2.0});
    CHECK(
        hosted->model.findCircle(*circle_result.entity_id)
            ->center() == sketch::Point2{6.0, 7.0});
    CHECK(
        hosted->model.findCircle(*circle_result.entity_id)
            ->radius() == 4.0);
    CHECK(
        hosted->model.findArc(*arc_result.entity_id)
            ->center() == sketch::Point2{4.0, 5.0});
    CHECK(
        hosted->model.findArc(*arc_result.entity_id)
            ->sweepAngle() == -pi * 0.5);

    const auto before_erase =
        session.document().state();
    const auto erased =
        session.execute(
            application::EraseSketchEntitiesCommand{
                sketch_id,
                {
                    *circle_result.entity_id,
                    *arc_result.entity_id,
                }});
    CHECK(erased.ok() && erased.changed);
    hosted = session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    CHECK(
        hosted->model.findCircle(
            *circle_result.entity_id) == nullptr);
    CHECK(
        hosted->model.findArc(
            *arc_result.entity_id) == nullptr);

    CHECK(session.undo().ok());
    CHECK(session.document().state() == before_erase);
    CHECK(session.redo().ok());
    CHECK(session.undo().ok());

    CHECK(session.save().ok());
    const auto package =
        persistence::readNativeDocumentContainer(path);
    CHECK(package.ok());
    CHECK(
        package.package->descriptor.domain_schema_version ==
        4);
    CHECK(
        package.package->authored_json.find(
            "\"kind\": \"circle\"") !=
        std::string::npos);
    CHECK(
        package.package->authored_json.find(
            "\"kind\": \"arc\"") !=
        std::string::npos);
    CHECK(
        package.package->authored_json.find(
            "\"entities\"") !=
        std::string::npos);

    auto loaded = store.load(path);
    CHECK(loaded.ok());
    const auto* reopened =
        loaded.document->findSketch(sketch_id);
    CHECK(reopened != nullptr);
    CHECK(
        reopened->model.findLine(
            *line_result.entity_id) != nullptr);
    CHECK(
        reopened->model.findCircle(
            *circle_result.entity_id) != nullptr);
    CHECK(
        reopened->model.findArc(
            *arc_result.entity_id) != nullptr);
    CHECK(
        reopened->model.findCircle(
            *circle_result.entity_id)->id() ==
        *circle_result.entity_id);
    CHECK(
        reopened->model.findArc(
            *arc_result.entity_id)->id() ==
        *arc_result.entity_id);

    const auto unknown_path =
        temp.path / "UnknownKind.ss2part";
    writeBytes(
        unknown_path,
        buildV4PackageWithModel(
            core::DocumentId::generate(),
            sketch::SketchId::generate(),
            "{"
            "\"next_entity_id\":\"2\","
            "\"entities\":["
                "{\"kind\":\"spline\",\"id\":\"1\"}"
            "]"
            "}"));
    const auto unknown = store.load(unknown_path);
    CHECK(!unknown.ok());
    CHECK(
        unknown.diagnostic.code ==
        part::PartStoreErrorCode::malformed_document);

    return EXIT_SUCCESS;
}
