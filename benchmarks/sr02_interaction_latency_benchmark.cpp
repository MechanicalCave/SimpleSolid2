#include "part_document_tree_controller.hpp"
#include "part_sketch_interaction_controller.hpp"
#include "part_viewport_controller.hpp"

#include <simplesolid2/application/cad_input.hpp>
#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/part/part_document.hpp>
#include <simplesolid2/part/part_sketch.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>
#include <simplesolid2/viewer_qt_occt/qt_occt_viewer_widget.hpp>

#include <QApplication>
#include <QEventLoop>
#include <QTimer>
#include <QTreeWidget>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <iostream>
#include <numbers>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

using namespace simplesolid2;

namespace {

using Clock = std::chrono::steady_clock;

struct Stats final {
    double p50_us{};
    double p95_us{};
    double max_us{};
    std::size_t samples{};
};

struct Fixture final {
    application::DocumentSession session;
    sketch::SketchId sketch_id;
    std::vector<sketch::EntityId> line_ids;
    std::size_t semantic_entities{};
};

struct RuntimeCounters final {
    std::size_t resolver_calls{};
    std::uint64_t region_analysis_calls{};
    viewer_qt_occt::QtOcctRuntimeDiagnostics provider;
};

[[nodiscard]] Stats summarize(std::vector<double> samples) {
    if (samples.empty()) {
        throw std::runtime_error("measurement produced no samples");
    }
    std::sort(samples.begin(), samples.end());
    const auto percentile =
        [&samples](double value) {
            const auto index = std::min(
                samples.size() - 1U,
                static_cast<std::size_t>(
                    std::ceil(
                        value *
                        static_cast<double>(samples.size()))) -
                    1U);
            return samples[index];
        };
    return {
        percentile(0.50),
        percentile(0.95),
        samples.back(),
        samples.size()};
}

template <typename Operation>
[[nodiscard]] Stats measure(
    std::size_t warmups,
    std::size_t samples,
    Operation&& operation) {
    for (std::size_t index = 0U; index < warmups; ++index) {
        if (!operation(index)) {
            throw std::runtime_error("benchmark warmup operation failed");
        }
    }

    std::vector<double> elapsed;
    elapsed.reserve(samples);
    for (std::size_t index = 0U; index < samples; ++index) {
        const auto start = Clock::now();
        if (!operation(index)) {
            throw std::runtime_error("benchmark measured operation failed");
        }
        QApplication::processEvents(QEventLoop::AllEvents);
        const auto stop = Clock::now();
        elapsed.push_back(
            std::chrono::duration<double, std::micro>(
                stop - start).count());
    }
    return summarize(std::move(elapsed));
}

[[nodiscard]] std::size_t sampleCount(
    std::size_t entity_count) noexcept {
    if (entity_count >= 5000U) return 12U;
    if (entity_count >= 1000U) return 20U;
    return 30U;
}

void printResult(
    std::string_view scenario,
    std::string_view workload,
    std::size_t semantic_entities,
    std::size_t derived_objects,
    const Stats& stats,
    const RuntimeCounters& counters) {
    std::cout
        << std::fixed << std::setprecision(3)
        << "SR02_RESULT"
        << " scenario=" << scenario
        << " workload=" << workload
        << " semantic_entities=" << semantic_entities
        << " derived_objects=" << derived_objects
        << " samples=" << stats.samples
        << " p50_us=" << stats.p50_us
        << " p95_us=" << stats.p95_us
        << " max_us=" << stats.max_us
        << " resolver_calls=" << counters.resolver_calls
        << " region_analysis_calls="
        << counters.region_analysis_calls
        << " rectangle_queries="
        << counters.provider.sketch_rectangle_queries
        << " rectangle_segments="
        << counters.provider.sketch_rectangle_segments
        << " token_lookups="
        << counters.provider.sketch_rectangle_token_lookups
        << " token_comparisons="
        << counters.provider.sketch_rectangle_token_comparisons
        << " update_current_viewer_calls="
        << counters.provider.update_current_viewer_calls
        << " redraw_calls="
        << counters.provider.redraw_calls
        << " native_objects_current="
        << counters.provider.sketch_native_objects_current
        << '\n';
}

[[nodiscard]] part::PartSketchSupport makeSupport() {
    const auto support =
        part::partSketchSupportForBuiltinPlane(
            core::BuiltinReferenceRole::xy_plane);
    if (!support) {
        throw std::runtime_error("missing XY Sketch support");
    }
    return *support;
}

[[nodiscard]] Fixture makeFixture(
    sketch::SketchModel model,
    std::vector<sketch::EntityId> line_ids) {
    const auto support = makeSupport();
    const auto placement =
        part::sketchPlacementForSupport(support);
    if (!placement) {
        throw std::runtime_error("missing XY Sketch placement");
    }

    const auto semantic_entities = model.entityCount();
    const auto sketch_id = sketch::SketchId::generate();

    part::PartAuthoredState state;
    state.sketches.push_back(
        part::PartSketch{
            sketch_id,
            support,
            *placement,
            true,
            std::move(model)});

    auto restored = part::PartDocument::restore(
        core::DocumentId::generate(),
        std::move(state));
    if (!restored.ok()) {
        throw std::runtime_error(
            "Part fixture reconstruction failed");
    }

    return Fixture{
        application::DocumentSession{
            std::filesystem::path{
                "sr02-latency-benchmark.ss2part"},
            std::move(*restored.document)},
        sketch_id,
        std::move(line_ids),
        semantic_entities};
}

[[nodiscard]] Fixture makeLineFixture(
    std::size_t entity_count) {
    sketch::SketchModel model;
    std::vector<sketch::EntityId> line_ids;
    line_ids.reserve(entity_count);

    const auto side = static_cast<std::size_t>(
        std::ceil(std::sqrt(
            static_cast<double>(entity_count))));
    for (std::size_t index = 0U;
         index < entity_count;
         ++index) {
        const auto column = index % side;
        const auto row = index / side;
        const double u =
            static_cast<double>(column) * 2.0;
        const double v =
            static_cast<double>(row) * 2.0;
        line_ids.push_back(
            model.addLine(
                {u, v},
                {u + 1.0, v + 0.5}));
    }
    return makeFixture(
        std::move(model),
        std::move(line_ids));
}

[[nodiscard]] Fixture makeDenseIntersectionFixture(
    std::size_t entity_count) {
    sketch::SketchModel model;
    std::vector<sketch::EntityId> line_ids;
    line_ids.reserve(entity_count);

    constexpr double radius = 40.0;
    for (std::size_t index = 0U;
         index < entity_count;
         ++index) {
        const double angle =
            std::numbers::pi_v<double> *
            static_cast<double>(index) /
            static_cast<double>(entity_count);
        const sketch::Point2 direction{
            std::cos(angle),
            std::sin(angle)};
        line_ids.push_back(
            model.addLine(
                {-radius * direction.u,
                 -radius * direction.v},
                {radius * direction.u,
                 radius * direction.v}));
    }
    return makeFixture(
        std::move(model),
        std::move(line_ids));
}

[[nodiscard]] Fixture makeMixedFixture(
    std::size_t entity_count) {
    sketch::SketchModel model;
    std::vector<sketch::EntityId> line_ids;

    const auto side = static_cast<std::size_t>(
        std::ceil(std::sqrt(
            static_cast<double>(entity_count))));
    for (std::size_t index = 0U;
         index < entity_count;
         ++index) {
        const auto column = index % side;
        const auto row = index / side;
        const double u =
            static_cast<double>(column) * 4.0;
        const double v =
            static_cast<double>(row) * 4.0;

        switch (index % 3U) {
        case 0U:
            line_ids.push_back(
                model.addLine(
                    {u, v},
                    {u + 1.5, v + 0.75}));
            break;
        case 1U:
            static_cast<void>(
                model.addCircle(
                    {u + 1.0, v + 1.0},
                    0.75));
            break;
        default:
            static_cast<void>(
                model.addArc(
                    {u + 1.0, v + 1.0},
                    0.75,
                    0.0,
                    std::numbers::pi_v<double> * 0.75));
            break;
        }
    }
    return makeFixture(
        std::move(model),
        std::move(line_ids));
}

[[nodiscard]] Fixture makeProfileFixture(
    std::size_t rectangle_count) {
    sketch::SketchModel model;
    std::vector<sketch::EntityId> line_ids;
    line_ids.reserve(rectangle_count * 4U);

    const auto side = static_cast<std::size_t>(
        std::ceil(std::sqrt(
            static_cast<double>(rectangle_count))));
    for (std::size_t index = 0U;
         index < rectangle_count;
         ++index) {
        const auto column = index % side;
        const auto row = index / side;
        const double u =
            static_cast<double>(column) * 12.0;
        const double v =
            static_cast<double>(row) * 12.0;
        const sketch::Point2 a{u, v};
        const sketch::Point2 b{u + 8.0, v};
        const sketch::Point2 c{u + 8.0, v + 8.0};
        const sketch::Point2 d{u, v + 8.0};
        line_ids.push_back(model.addLine(a, b));
        line_ids.push_back(model.addLine(b, c));
        line_ids.push_back(model.addLine(c, d));
        line_ids.push_back(model.addLine(d, a));
    }
    return makeFixture(
        std::move(model),
        std::move(line_ids));
}

void waitForNativeWindow() {
    QEventLoop loop;
    QTimer::singleShot(
        150,
        &loop,
        &QEventLoop::quit);
    loop.exec();
}

[[nodiscard]] ui::SketchPointerInput pointerInput(
    ui::PartViewportController& controller,
    sketch::SketchId sketch_id,
    viewer::SpatialPointerPhase phase,
    sketch::Point2 position) {
    const auto projected =
        controller.projectSketchPointToViewport(position);
    if (!projected) {
        throw std::runtime_error(
            "could not project benchmark Sketch point");
    }
    return {
        sketch_id,
        phase,
        *projected,
        position,
        false};
}

void resetCounters(
    viewer_qt_occt::QtOcctViewerWidget& viewport,
    ui::PartSketchInteractionController* interaction = nullptr) {
    viewport.resetRuntimeDiagnostics();
    sketch::resetRegionAnalysisInvocationCount();
    if (interaction != nullptr) {
        interaction->resetLatencyDiagnostics();
    }
}

[[nodiscard]] RuntimeCounters counters(
    const viewer_qt_occt::QtOcctViewerWidget& viewport,
    const ui::PartSketchInteractionController* interaction = nullptr) {
    RuntimeCounters result;
    result.provider = viewport.runtimeDiagnostics();
    result.region_analysis_calls =
        sketch::regionAnalysisInvocationCount();
    if (interaction != nullptr) {
        result.resolver_calls =
            interaction->pointerResolutionCount();
    }
    return result;
}

void configureInteraction(
    ui::PartSketchInteractionController& interaction) {
    interaction.setCadInteractionSettingsProvider(
        [] {
            application::CadInteractionSettings settings;
            return settings;
        });
}

void measureInteractivePreviewPaths(
    viewer_qt_occt::QtOcctViewerWidget& viewport,
    Fixture& fixture,
    std::string_view workload) {
    QTreeWidget tree;
    ui::PartDocumentTreeController tree_controller{tree};
    ui::PartViewportController controller{
        tree_controller,
        &viewport};
    ui::PartSketchInteractionController interaction{
        controller};

    controller.setDocumentSession(&fixture.session);
    controller.setSketchEditSketch(fixture.sketch_id);
    viewport.fitAll();
    QApplication::processEvents();
    configureInteraction(interaction);
    interaction.begin(
        fixture.session,
        fixture.sketch_id);

    const auto samples =
        sampleCount(fixture.semantic_entities);

    const auto run_preview =
        [&](std::string_view scenario,
            const std::function<void()>& arm,
            sketch::Point2 base,
            sketch::Point2 target) {
            interaction.activateSelect();
            arm();
            interaction.onPointer(
                pointerInput(
                    controller,
                    fixture.sketch_id,
                    viewer::SpatialPointerPhase::move,
                    target));
            resetCounters(viewport, &interaction);

            const auto stats = measure(
                0U,
                samples,
                [&](std::size_t index) {
                    const double delta =
                        static_cast<double>(index % 5U) *
                        0.002;
                    interaction.onPointer(
                        pointerInput(
                            controller,
                            fixture.sketch_id,
                            viewer::SpatialPointerPhase::move,
                            {target.u + delta,
                             target.v + delta * 0.5}));
                    return true;
                });

            printResult(
                scenario,
                workload,
                fixture.semantic_entities,
                fixture.semantic_entities,
                stats,
                counters(viewport, &interaction));
            static_cast<void>(base);
        };

    run_preview(
        "line_preview_event_to_redraw",
        [&] {
            interaction.activateLine();
            interaction.onPointer(
                pointerInput(
                    controller,
                    fixture.sketch_id,
                    viewer::SpatialPointerPhase::primary_press,
                    {1.25, 1.25}));
            if (interaction.lineStage() !=
                sketch::LineStage::await_next_point) {
                throw std::runtime_error(
                    "Line benchmark did not reach preview stage");
            }
        },
        {1.25, 1.25},
        {3.25, 2.75});

    run_preview(
        "circle_preview_event_to_redraw",
        [&] {
            interaction.activateCircle();
            interaction.onPointer(
                pointerInput(
                    controller,
                    fixture.sketch_id,
                    viewer::SpatialPointerPhase::primary_press,
                    {5.25, 5.25}));
            if (interaction.circleStage() !=
                sketch::CircleStage::await_radius) {
                throw std::runtime_error(
                    "Circle benchmark did not reach preview stage");
            }
        },
        {5.25, 5.25},
        {7.25, 5.75});

    run_preview(
        "arc_preview_event_to_redraw",
        [&] {
            interaction.activateArc();
            interaction.onPointer(
                pointerInput(
                    controller,
                    fixture.sketch_id,
                    viewer::SpatialPointerPhase::primary_press,
                    {9.25, 9.25}));
            interaction.onPointer(
                pointerInput(
                    controller,
                    fixture.sketch_id,
                    viewer::SpatialPointerPhase::primary_press,
                    {13.25, 9.25}));
            if (interaction.arcStage() !=
                sketch::ArcStage::await_arc_point) {
                throw std::runtime_error(
                    "Arc benchmark did not reach preview stage");
            }
        },
        {9.25, 9.25},
        {11.25, 12.25});

    interaction.activateSelect();
    if (fixture.line_ids.empty()) {
        throw std::runtime_error(
            "transform benchmark requires a Line");
    }

    const auto* hosted =
        fixture.session.document().findSketch(
            fixture.sketch_id);
    const auto* first_line =
        hosted != nullptr
            ? hosted->model.findLine(
                  fixture.line_ids.front())
            : nullptr;
    if (first_line == nullptr) {
        throw std::runtime_error(
            "transform benchmark Line missing");
    }
    const sketch::Point2 midpoint{
        (first_line->start().u + first_line->end().u) * 0.5,
        (first_line->start().v + first_line->end().v) * 0.5};

    const auto select_pointer =
        pointerInput(
            controller,
            fixture.sketch_id,
            viewer::SpatialPointerPhase::primary_press,
            midpoint);
    interaction.onPointer(select_pointer);
    auto release_pointer = select_pointer;
    release_pointer.phase =
        viewer::SpatialPointerPhase::primary_release;
    interaction.onPointer(release_pointer);
    if (interaction.selectedCount() == 0U) {
        throw std::runtime_error(
            "transform benchmark could not select Line");
    }
    if (!interaction.activateMove()) {
        throw std::runtime_error(
            "transform benchmark MOVE activation failed");
    }
    interaction.onPointer(
        pointerInput(
            controller,
            fixture.sketch_id,
            viewer::SpatialPointerPhase::primary_press,
            midpoint));
    if (interaction.commonTransformStage() !=
        sketch::CommonTransformStage::await_destination) {
        throw std::runtime_error(
            "transform benchmark did not reach destination stage");
    }

    const sketch::Point2 transform_target{
        midpoint.u + 2.0,
        midpoint.v + 1.5};
    interaction.onPointer(
        pointerInput(
            controller,
            fixture.sketch_id,
            viewer::SpatialPointerPhase::move,
            transform_target));
    resetCounters(viewport, &interaction);
    const auto transform_stats = measure(
        0U,
        samples,
        [&](std::size_t index) {
            const double delta =
                static_cast<double>(index % 5U) *
                0.002;
            interaction.onPointer(
                pointerInput(
                    controller,
                    fixture.sketch_id,
                    viewer::SpatialPointerPhase::move,
                    {transform_target.u + delta,
                     transform_target.v}));
            return true;
        });
    printResult(
        "move_preview_event_to_redraw",
        workload,
        fixture.semantic_entities,
        fixture.semantic_entities,
        transform_stats,
        counters(viewport, &interaction));

    interaction.end();
    controller.clear();
    QApplication::processEvents();
}

void measureRectangleQuery(
    viewer_qt_occt::QtOcctViewerWidget& viewport,
    Fixture& fixture,
    std::string_view workload) {
    QTreeWidget tree;
    ui::PartDocumentTreeController tree_controller{tree};
    ui::PartViewportController controller{
        tree_controller,
        &viewport};

    controller.setDocumentSession(&fixture.session);
    controller.setSketchEditSketch(fixture.sketch_id);
    viewport.fitAll();
    QApplication::processEvents();

    const viewer::ViewportRect2 full_view{
        {0.0, 0.0},
        {static_cast<double>(viewport.width()),
         static_cast<double>(viewport.height())}};

    static_cast<void>(
        controller.querySketchEntities(
            full_view,
            viewer::SketchRectangleSelectionRule::crossing));
    resetCounters(viewport);

    const auto stats = measure(
        0U,
        sampleCount(fixture.semantic_entities),
        [&](std::size_t) {
            const auto result =
                controller.querySketchEntities(
                    full_view,
                    viewer::SketchRectangleSelectionRule::crossing);
            return result.completed;
        });

    printResult(
        "rectangle_crossing_query",
        workload,
        fixture.semantic_entities,
        fixture.semantic_entities,
        stats,
        counters(viewport));

    controller.clear();
    QApplication::processEvents();
}

void measureAuthoredRefresh(
    viewer_qt_occt::QtOcctViewerWidget& viewport,
    Fixture& fixture,
    std::string_view workload) {
    QTreeWidget tree;
    ui::PartDocumentTreeController tree_controller{tree};
    ui::PartViewportController controller{
        tree_controller,
        &viewport};

    controller.setDocumentSession(&fixture.session);
    controller.setSketchEditSketch(fixture.sketch_id);
    viewport.fitAll();
    QApplication::processEvents();

    controller.refreshPresentation();
    resetCounters(viewport);
    const auto stats = measure(
        0U,
        sampleCount(fixture.semantic_entities),
        [&](std::size_t) {
            controller.refreshPresentation();
            return !controller.presentationDegraded();
        });

    printResult(
        "authored_refresh_to_redraw",
        workload,
        fixture.semantic_entities,
        fixture.semantic_entities,
        stats,
        counters(viewport));

    controller.clear();
    QApplication::processEvents();
}

void measureProfileHover(
    viewer_qt_occt::QtOcctViewerWidget& viewport) {
    auto fixture = makeProfileFixture(25U);
    QTreeWidget tree;
    ui::PartDocumentTreeController tree_controller{tree};
    ui::PartViewportController controller{
        tree_controller,
        &viewport};
    ui::PartSketchInteractionController interaction{
        controller};

    controller.setDocumentSession(&fixture.session);
    controller.setSketchEditSketch(fixture.sketch_id);
    viewport.fitAll();
    QApplication::processEvents();
    configureInteraction(interaction);
    interaction.begin(
        fixture.session,
        fixture.sketch_id);

    if (!interaction.activateProfileCreate()) {
        throw std::runtime_error(
            "Profile benchmark activation failed");
    }

    const sketch::Point2 first_center{4.0, 4.0};
    const sketch::Point2 second_center{16.0, 4.0};

    resetCounters(viewport, &interaction);
    const auto cold = measure(
        0U,
        1U,
        [&](std::size_t) {
            interaction.onPointer(
                pointerInput(
                    controller,
                    fixture.sketch_id,
                    viewer::SpatialPointerPhase::move,
                    first_center));
            return interaction.profileHoverStatus().has_value();
        });
    printResult(
        "profile_hover_cold",
        "25_closed_regions",
        fixture.semantic_entities,
        fixture.semantic_entities,
        cold,
        counters(viewport, &interaction));

    resetCounters(viewport, &interaction);
    const auto warm = measure(
        0U,
        30U,
        [&](std::size_t) {
            interaction.onPointer(
                pointerInput(
                    controller,
                    fixture.sketch_id,
                    viewer::SpatialPointerPhase::move,
                    first_center));
            return interaction.profileHoverStatus().has_value();
        });
    printResult(
        "profile_hover_warm_no_draft",
        "25_closed_regions",
        fixture.semantic_entities,
        fixture.semantic_entities,
        warm,
        counters(viewport, &interaction));

    interaction.onPointer(
        pointerInput(
            controller,
            fixture.sketch_id,
            viewer::SpatialPointerPhase::primary_press,
            first_center));
    if (!interaction.profileDraftIntent()) {
        throw std::runtime_error(
            "Profile benchmark draft was not created");
    }

    interaction.onPointer(
        pointerInput(
            controller,
            fixture.sketch_id,
            viewer::SpatialPointerPhase::move,
            second_center));
    resetCounters(viewport, &interaction);
    const auto draft = measure(
        0U,
        30U,
        [&](std::size_t) {
            interaction.onPointer(
                pointerInput(
                    controller,
                    fixture.sketch_id,
                    viewer::SpatialPointerPhase::move,
                    second_center));
            return interaction.profileHoverStatus().has_value();
        });
    printResult(
        "profile_hover_warm_with_draft",
        "25_closed_regions",
        fixture.semantic_entities,
        fixture.semantic_entities,
        draft,
        counters(viewport, &interaction));

    interaction.cancelProfile();
    interaction.end();
    controller.clear();
    QApplication::processEvents();
}

void measureFinishSketch(
    viewer_qt_occt::QtOcctViewerWidget& viewport) {
    auto fixture = makeLineFixture(1000U);
    QTreeWidget tree;
    ui::PartDocumentTreeController tree_controller{tree};
    ui::PartViewportController controller{
        tree_controller,
        &viewport};
    ui::PartSketchInteractionController interaction{
        controller};

    controller.setDocumentSession(&fixture.session);
    controller.setSketchEditSketch(fixture.sketch_id);
    viewport.fitAll();
    QApplication::processEvents();
    configureInteraction(interaction);
    interaction.begin(
        fixture.session,
        fixture.sketch_id);

    const auto arm_preview = [&] {
        interaction.activateLine();
        interaction.onPointer(
            pointerInput(
                controller,
                fixture.sketch_id,
                viewer::SpatialPointerPhase::primary_press,
                {1.25, 1.25}));
        interaction.onPointer(
            pointerInput(
                controller,
                fixture.sketch_id,
                viewer::SpatialPointerPhase::move,
                {3.25, 2.75}));
    };

    arm_preview();
    constexpr std::size_t samples = 20U;
    std::vector<double> elapsed;
    elapsed.reserve(samples);
    viewer_qt_occt::QtOcctRuntimeDiagnostics aggregate;

    for (std::size_t index = 0U;
         index < samples;
         ++index) {
        resetCounters(viewport, &interaction);
        const auto start = Clock::now();

        // Mirrors the presentation-relevant ordering in
        // CadWorkbench::clearSketchRuntimeContext().
        controller.clearSketchDynamicInputOverlay();
        interaction.end();
        controller.setSketchEditSketch(std::nullopt);
        QApplication::processEvents(QEventLoop::AllEvents);

        const auto stop = Clock::now();
        elapsed.push_back(
            std::chrono::duration<double, std::micro>(
                stop - start).count());

        const auto sample_metrics =
            viewport.runtimeDiagnostics();
        aggregate.update_current_viewer_calls +=
            sample_metrics.update_current_viewer_calls;
        aggregate.redraw_calls +=
            sample_metrics.redraw_calls;
        aggregate.sketch_native_objects_current =
            sample_metrics.sketch_native_objects_current;
        aggregate.sketch_rectangle_queries +=
            sample_metrics.sketch_rectangle_queries;
        aggregate.sketch_rectangle_segments +=
            sample_metrics.sketch_rectangle_segments;
        aggregate.sketch_rectangle_token_lookups +=
            sample_metrics.sketch_rectangle_token_lookups;
        aggregate.sketch_rectangle_token_comparisons +=
            sample_metrics.sketch_rectangle_token_comparisons;

        if (index + 1U < samples) {
            controller.setSketchEditSketch(
                fixture.sketch_id);
            interaction.begin(
                fixture.session,
                fixture.sketch_id);
            arm_preview();
        }
    }

    RuntimeCounters result;
    result.provider = aggregate;
    printResult(
        "finish_sketch_to_stable_non_edit",
        "1000_lines_active_preview",
        fixture.semantic_entities,
        fixture.semantic_entities,
        summarize(std::move(elapsed)),
        result);

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

        for (const std::size_t entity_count :
             {100U, 1000U, 5000U}) {
            auto fixture =
                makeLineFixture(entity_count);
            measureInteractivePreviewPaths(
                viewport,
                fixture,
                "lines");
            measureRectangleQuery(
                viewport,
                fixture,
                "lines");
            measureAuthoredRefresh(
                viewport,
                fixture,
                "lines");
        }

        auto dense =
            makeDenseIntersectionFixture(80U);
        measureInteractivePreviewPaths(
            viewport,
            dense,
            "dense_intersections");

        auto mixed =
            makeMixedFixture(300U);
        measureRectangleQuery(
            viewport,
            mixed,
            "mixed_line_circle_arc");
        measureAuthoredRefresh(
            viewport,
            mixed,
            "mixed_line_circle_arc");

        measureProfileHover(viewport);
        measureFinishSketch(viewport);

        viewport.close();
        std::cout << "SR02_BENCHMARK_PASS\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr
            << "SR02_BENCHMARK_FAILURE "
            << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
