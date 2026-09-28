#include "part_document_tree_controller.hpp"
#include "part_viewport_controller.hpp"

#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/part/part_document.hpp>
#include <simplesolid2/part/part_sketch.hpp>
#include <simplesolid2/sketch/transform.hpp>
#include <simplesolid2/viewer_qt_occt/qt_occt_viewer_widget.hpp>

#include <QApplication>
#include <QEventLoop>
#include <QTimer>
#include <QTreeWidget>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <numbers>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

using namespace simplesolid2;

namespace {

using Clock = std::chrono::steady_clock;

struct Stats final {
    double median_us{};
    double p95_us{};
    double max_us{};
    std::size_t samples{};
};

struct Fixture final {
    application::DocumentSession session;
    sketch::SketchId sketch_id;
    std::vector<sketch::EntityId> line_ids;
};

[[nodiscard]] Stats summarize(std::vector<double> samples) {
    if (samples.empty()) {
        throw std::runtime_error("measurement produced no samples");
    }
    std::sort(samples.begin(), samples.end());
    const auto middle = samples.size() / 2U;
    const double median = samples.size() % 2U == 0U
        ? (samples[middle - 1U] + samples[middle]) * 0.5
        : samples[middle];
    const auto p95_index = std::min(
        samples.size() - 1U,
        static_cast<std::size_t>(
            std::ceil(0.95 * static_cast<double>(samples.size()))) - 1U);
    return {median, samples[p95_index], samples.back(), samples.size()};
}

template <typename Operation>
[[nodiscard]] Stats measure(
    std::size_t warmups,
    std::size_t samples,
    Operation&& operation) {
    for (std::size_t index = 0; index < warmups; ++index) {
        if (!operation()) {
            throw std::runtime_error("benchmark warmup operation failed");
        }
    }

    std::vector<double> elapsed;
    elapsed.reserve(samples);
    for (std::size_t index = 0; index < samples; ++index) {
        const auto start = Clock::now();
        if (!operation()) {
            throw std::runtime_error("benchmark measured operation failed");
        }
        const auto stop = Clock::now();
        elapsed.push_back(
            std::chrono::duration<double, std::micro>(stop - start).count());
    }
    return summarize(std::move(elapsed));
}

void printResult(
    std::string_view operation,
    std::size_t semantic_entities,
    std::size_t provider_objects,
    std::size_t warmups,
    const Stats& stats,
    std::string_view workload = "lines") {
    std::cout
        << std::fixed << std::setprecision(3)
        << "E2_RESULT"
        << " operation=" << operation
        << " workload=" << workload
        << " semantic_entities=" << semantic_entities
        << " provider_objects=" << provider_objects
        << " warmups=" << warmups
        << " samples=" << stats.samples
        << " median_us=" << stats.median_us
        << " p95_us=" << stats.p95_us
        << " max_us=" << stats.max_us
        << '\n';
}

[[nodiscard]] Fixture makeFixture(std::size_t entity_count) {
    const auto support =
        part::partSketchSupportForBuiltinPlane(
            core::BuiltinReferenceRole::xy_plane);
    if (!support) {
        throw std::runtime_error("missing XY Sketch support");
    }
    const auto placement = part::sketchPlacementForSupport(*support);
    if (!placement) {
        throw std::runtime_error("missing XY Sketch placement");
    }

    sketch::SketchModel model;
    std::vector<sketch::EntityId> line_ids;
    line_ids.reserve(entity_count);
    for (std::size_t index = 0; index < entity_count; ++index) {
        const double u = static_cast<double>(index) * 1.5;
        line_ids.push_back(
            model.addLine({u, 0.0}, {u + 1.0, 0.5}));
    }

    const auto sketch_id = sketch::SketchId::generate();
    part::PartAuthoredState state;
    state.sketches.push_back(
        part::PartSketch{
            sketch_id,
            *support,
            *placement,
            true,
            std::move(model)});

    auto restored = part::PartDocument::restore(
        core::DocumentId::generate(),
        std::move(state));
    if (!restored.ok()) {
        throw std::runtime_error("Part fixture reconstruction failed");
    }

    return Fixture{
        application::DocumentSession{
            std::filesystem::path{"e2-presentation-benchmark.ss2part"},
            std::move(*restored.document)},
        sketch_id,
        std::move(line_ids)};
}

[[nodiscard]] viewer::SketchScene makeNativeScene(
    std::size_t entity_count,
    bool mixed) {
    viewer::SketchScene scene;
    scene.origin =
        viewer::SketchOriginPresentation{
            viewer::Point3{0.0, 0.0, 0.0}};

    for (std::size_t index = 0; index < entity_count; ++index) {
        const auto token = viewer::PresentationToken{
            0x10000U + static_cast<std::uint64_t>(index) + 1U};
        const double u = static_cast<double>(index) * 1.5;

        if (!mixed || index % 3U == 0U) {
            scene.lines.push_back(
                viewer::SketchLinePresentation{
                    token,
                    {u, 0.0, 0.0},
                    {u + 1.0, 0.5, 0.0}});
            continue;
        }

        viewer::SketchCurvePresentation curve;
        curve.token = token;
        const bool circle = index % 3U == 1U;
        const std::size_t segment_count = circle ? 96U : 24U;
        const double sweep = circle
            ? 2.0 * std::numbers::pi_v<double>
            : std::numbers::pi_v<double> * 0.5;
        const double v = circle ? 3.0 : 6.0;

        curve.points.reserve(segment_count + 1U);
        for (std::size_t point = 0; point <= segment_count; ++point) {
            const double fraction =
                static_cast<double>(point) /
                static_cast<double>(segment_count);
            const double angle = sweep * fraction;
            curve.points.push_back(
                viewer::Point3{
                    u + 0.75 * std::cos(angle),
                    v + 0.75 * std::sin(angle),
                    0.0});
        }
        scene.curves.push_back(std::move(curve));
    }

    if (!scene.valid()) {
        throw std::runtime_error("generated native scene is invalid");
    }
    return scene;
}

[[nodiscard]] std::size_t providerObjectCount(
    const viewer::SketchScene& scene) {
    std::size_t result = scene.origin ? 1U : 0U;
    result += scene.lines.size();
    for (const auto& curve : scene.curves) {
        result += curve.points.size() - 1U;
    }
    return result;
}

[[nodiscard]] std::size_t sampleCount(
    std::size_t entity_count) noexcept {
    if (entity_count >= 5000U) return 3U;
    if (entity_count >= 1000U) return 5U;
    return 7U;
}

void waitForNativeWindow() {
    QEventLoop loop;
    QTimer::singleShot(150, &loop, &QEventLoop::quit);
    loop.exec();
}

void measureControllerPaths(
    viewer_qt_occt::QtOcctViewerWidget& viewport,
    std::size_t entity_count) {
    auto fixture = makeFixture(entity_count);
    QTreeWidget tree;
    ui::PartDocumentTreeController tree_controller{tree};
    ui::PartViewportController controller{tree_controller, &viewport};

    controller.setDocumentSession(&fixture.session);
    controller.setSketchEditSketch(fixture.sketch_id);

    bool flip = false;
    const auto mutate = [&]() {
        const double offset = flip ? 0.10 : 0.20;
        flip = !flip;
        const auto result = fixture.session.execute(
            application::UpdateSketchGeometryCommand{
                fixture.sketch_id,
                fixture.session.document().revision(),
                {
                    application::SketchLineGeometryUpdate{
                        fixture.line_ids.front(),
                        {offset, 0.0},
                        {offset + 1.0, 0.5}},
                },
                {},
                {}});
        if (!result.ok() || !result.changed) {
            throw std::runtime_error("benchmark semantic mutation failed");
        }
    };

    constexpr std::size_t warmups = 1U;
    const auto samples = sampleCount(entity_count);
    const auto authored = measure(
        warmups,
        samples,
        [&]() {
            mutate();
            controller.refreshPresentation();
            return true;
        });
    printResult(
        "authored_refresh",
        entity_count,
        entity_count + 1U,
        warmups,
        authored);

    const auto* hosted =
        fixture.session.document().findSketch(fixture.sketch_id);
    if (hosted == nullptr) {
        throw std::runtime_error("active benchmark Sketch disappeared");
    }

    sketch::SketchTransformGeometry geometry;
    geometry.lines = hosted->model.state().lines;
    const auto transform = measure(
        warmups,
        samples,
        [&]() {
            return controller.setSketchGeometryPreview(geometry);
        });
    printResult(
        "transform_preview",
        entity_count,
        geometry.lines.size(),
        warmups,
        transform);

    if (entity_count == 100U) {
        const auto single = measure(
            warmups,
            samples,
            [&]() {
                return controller.setSketchPreview(
                    {
                        ui::SketchPreviewLine2D{
                            {0.0, 0.0},
                            {10.0, 1.0}},
                    });
            });
        printResult(
            "single_line_preview",
            1U,
            1U,
            warmups,
            single);
    }

    controller.clear();
    QApplication::processEvents();
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication app{argc, argv};

    try {
        viewer_qt_occt::QtOcctViewerWidget viewport;
        viewport.resize(900, 640);
        viewport.show();
        waitForNativeWindow();
        QApplication::processEvents();

        constexpr std::size_t warmups = 1U;
        for (const std::size_t entity_count :
             {100U, 1000U, 5000U}) {
            measureControllerPaths(viewport, entity_count);

            const auto scene = makeNativeScene(entity_count, false);
            const auto native = measure(
                warmups,
                sampleCount(entity_count),
                [&]() {
                    return viewport.setSketchScene(scene);
                });
            printResult(
                "native_scene_replace",
                entity_count,
                providerObjectCount(scene),
                warmups,
                native);
        }

        const auto mixed = makeNativeScene(100U, true);
        const auto mixed_stats = measure(
            warmups,
            5U,
            [&]() {
                return viewport.setSketchScene(mixed);
            });
        printResult(
            "native_scene_replace",
            100U,
            providerObjectCount(mixed),
            warmups,
            mixed_stats,
            "mixed_line_circle_arc");

        viewport.close();
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr
            << "E2_BENCHMARK_FAILURE "
            << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
