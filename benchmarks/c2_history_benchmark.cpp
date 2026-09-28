#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/core/document.hpp>
#include <simplesolid2/part/part_sketch.hpp>
#include <simplesolid2/sketch/line.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#include <psapi.h>
#endif

using namespace simplesolid2;

namespace {

struct Options final {
    std::size_t entities{0U};
    std::size_t depth{0U};
    std::size_t samples{20U};
    std::size_t warmup{3U};
    std::string scenario{"all"};
    std::filesystem::path output;
    std::string git_sha{"unknown"};
    bool self_test{false};
};

struct MemoryCounters final {
    std::uint64_t working_set_bytes{0U};
    std::uint64_t peak_working_set_bytes{0U};
    std::uint64_t private_usage_bytes{0U};
};

struct Statistics final {
    double median{};
    double p95{};
    double maximum{};
};

struct SampleMeasurement final {
    double setup_ms{};
    double latency_us{};
    MemoryCounters post_setup;
};

struct ScenarioResult final {
    std::string name;
    std::vector<double> latency_us;
    std::vector<double> setup_ms;
    Statistics latency_statistics;
    bool percentile_limited{false};
    std::uint64_t max_post_setup_working_set_bytes{0U};
    std::uint64_t max_post_setup_private_usage_bytes{0U};
};

struct Fixture final {
    application::DocumentSession session;
    sketch::SketchId sketch_id;
    std::vector<sketch::EntityId> entity_ids;
};

[[noreturn]] void fail(const std::string& message) {
    throw std::runtime_error{message};
}

void require(bool condition, const std::string& message) {
    if (!condition) {
        fail(message);
    }
}

std::size_t parseSize(std::string_view text, const char* name) {
    if (text.empty()) {
        fail(std::string{name} + " requires a non-empty unsigned integer");
    }

    std::size_t consumed = 0U;
    unsigned long long value = 0U;
    try {
        value = std::stoull(std::string{text}, &consumed, 10);
    } catch (const std::exception&) {
        fail(std::string{name} + " requires an unsigned integer");
    }

    if (consumed != text.size() ||
        value > static_cast<unsigned long long>(
            std::numeric_limits<std::size_t>::max())) {
        fail(std::string{name} + " is out of range");
    }

    return static_cast<std::size_t>(value);
}

bool validScenario(std::string_view scenario) {
    return scenario == "all" ||
           scenario == "add" ||
           scenario == "edit" ||
           scenario == "transform" ||
           scenario == "undo" ||
           scenario == "redo" ||
           scenario == "branch" ||
           scenario == "branch_half";
}

Options parseOptions(int argc, char** argv) {
    Options options;

    for (int index = 1; index < argc; ++index) {
        const std::string_view arg{argv[index]};
        if (arg == "--self-test") {
            options.self_test = true;
            continue;
        }

        const auto value = [&]() -> std::string_view {
            if (index + 1 >= argc) {
                fail(std::string{arg} + " requires a value");
            }
            return argv[++index];
        };

        if (arg == "--entities") {
            options.entities = parseSize(value(), "--entities");
        } else if (arg == "--depth") {
            options.depth = parseSize(value(), "--depth");
        } else if (arg == "--samples") {
            options.samples = parseSize(value(), "--samples");
        } else if (arg == "--warmup") {
            options.warmup = parseSize(value(), "--warmup");
        } else if (arg == "--scenario") {
            options.scenario = std::string{value()};
        } else if (arg == "--output") {
            options.output = std::filesystem::path{value()};
        } else if (arg == "--git-sha") {
            options.git_sha = std::string{value()};
        } else {
            fail("Unknown argument: " + std::string{arg});
        }
    }

    if (options.self_test) {
        return options;
    }

    require(
        options.entities > 0U,
        "--entities must be greater than zero");
    require(
        options.depth > 0U,
        "--depth must be greater than zero");
    require(
        options.samples > 0U,
        "--samples must be greater than zero");
    require(
        validScenario(options.scenario),
        "Unknown --scenario value");

    return options;
}

Statistics calculateStatistics(
    const std::vector<double>& values) {
    require(
        !values.empty(),
        "Cannot calculate statistics for an empty sample set");

    auto sorted = values;
    std::sort(sorted.begin(), sorted.end());

    const auto size = sorted.size();
    const double median =
        (size % 2U) == 0U
            ? (sorted[size / 2U - 1U] +
               sorted[size / 2U]) /
                  2.0
            : sorted[size / 2U];

    const auto nearest_rank =
        static_cast<std::size_t>(
            std::ceil(
                0.95 *
                static_cast<double>(size)));
    const auto p95_index =
        std::max<std::size_t>(
            1U,
            nearest_rank) -
        1U;

    return Statistics{
        median,
        sorted[p95_index],
        sorted.back()};
}

MemoryCounters readMemoryCounters() noexcept {
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS_EX counters{};
    counters.cb = sizeof(counters);
    if (::GetProcessMemoryInfo(
            ::GetCurrentProcess(),
            reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(
                &counters),
            sizeof(counters)) != FALSE) {
        return MemoryCounters{
            static_cast<std::uint64_t>(
                counters.WorkingSetSize),
            static_cast<std::uint64_t>(
                counters.PeakWorkingSetSize),
            static_cast<std::uint64_t>(
                counters.PrivateUsage)};
    }
#endif
    return {};
}

std::string compilerDescription() {
#if defined(_MSC_VER)
    return "MSVC " +
           std::to_string(_MSC_VER) +
           " full=" +
           std::to_string(_MSC_FULL_VER);
#elif defined(__clang__)
    return "Clang " +
           std::string{__clang_version__};
#elif defined(__GNUC__)
    return "GCC " +
           std::string{__VERSION__};
#else
    return "unknown";
#endif
}

std::string buildConfiguration() {
#if defined(NDEBUG)
    return "Release";
#else
    return "Debug";
#endif
}

std::string jsonEscape(std::string_view input) {
    std::ostringstream out;

    for (const char ch : input) {
        switch (ch) {
        case '"':
            out << "\\\"";
            break;
        case '\\':
            out << "\\\\";
            break;
        case '\b':
            out << "\\b";
            break;
        case '\f':
            out << "\\f";
            break;
        case '\n':
            out << "\\n";
            break;
        case '\r':
            out << "\\r";
            break;
        case '\t':
            out << "\\t";
            break;
        default:
            if (static_cast<unsigned char>(ch) < 0x20U) {
                out << "\\u"
                    << std::hex
                    << std::setw(4)
                    << std::setfill('0')
                    << static_cast<int>(
                           static_cast<unsigned char>(ch))
                    << std::dec
                    << std::setfill(' ');
            } else {
                out << ch;
            }
        }
    }

    return out.str();
}

const part::PartSketch& requireSketch(
    const application::DocumentSession& session,
    const sketch::SketchId& sketch_id) {
    const auto* hosted =
        session.document().findSketch(sketch_id);
    require(
        hosted != nullptr,
        "Benchmark Sketch is missing");
    return *hosted;
}

std::size_t entityCount(
    const application::DocumentSession& session,
    const sketch::SketchId& sketch_id) {
    return requireSketch(
               session,
               sketch_id)
        .model.entityCount();
}

Fixture makeFixture(
    std::size_t entities,
    std::size_t depth) {
    const auto support =
        part::partSketchSupportForBuiltinPlane(
            core::BuiltinReferenceRole::xy_plane);
    require(
        support.has_value(),
        "Unable to create XY Sketch support");

    const auto placement =
        part::sketchPlacementForSupport(*support);
    require(
        placement.has_value(),
        "Unable to create XY Sketch placement");

    auto sketch_id =
        sketch::SketchId::generate();
    sketch::SketchModel model;
    std::vector<sketch::EntityId> entity_ids;
    entity_ids.reserve(entities);

    constexpr std::size_t row_width = 250U;
    for (std::size_t index = 0U;
         index < entities;
         ++index) {
        const auto column =
            index % row_width;
        const auto row =
            index / row_width;
        const double u =
            static_cast<double>(column) * 2.0;
        const double v =
            static_cast<double>(row) * 2.0;

        entity_ids.push_back(
            model.addLine(
                sketch::Point2{u, v},
                sketch::Point2{
                    u + 1.0,
                    v + 0.25}));
    }

    part::PartAuthoredState state;
    state.sketches.push_back(
        part::PartSketch{
            sketch_id,
            *support,
            *placement,
            true,
            std::move(model)});

    auto restored =
        part::PartDocument::restore(
            core::DocumentId::generate(),
            std::move(state));
    require(
        restored.ok(),
        "Unable to restore benchmark Part state");

    application::DocumentSession session{
        {},
        std::move(*restored.document)};

    for (std::size_t index = 0U;
         index < depth;
         ++index) {
        auto properties =
            session.document().properties();
        properties.title =
            "C2 history " +
            std::to_string(index + 1U);

        const auto result =
            session.execute(
                application::
                    SetDocumentPropertiesCommand{
                        std::move(properties)});
        require(
            result.ok() && result.changed,
            "Unable to prepare requested history depth");
    }

    require(
        session.undoDepth() == depth &&
            session.redoDepth() == 0U,
        "Prepared history depth does not match request");
    require(
        entityCount(
            session,
            sketch_id) == entities,
        "Prepared benchmark entity count does not match request");

    return Fixture{
        std::move(session),
        std::move(sketch_id),
        std::move(entity_ids)};
}

application::UpdateSketchGeometryCommand
makeLineUpdate(
    const Fixture& fixture,
    std::size_t count,
    double du,
    double dv) {
    const auto& hosted =
        requireSketch(
            fixture.session,
            fixture.sketch_id);

    std::vector<
        application::SketchLineGeometryUpdate>
        updates;

    const auto bounded =
        std::min(
            count,
            fixture.entity_ids.size());
    updates.reserve(bounded);

    for (std::size_t index = 0U;
         index < bounded;
         ++index) {
        const auto id =
            fixture.entity_ids[index];
        const auto* line =
            hosted.model.findLine(id);
        require(
            line != nullptr,
            "Benchmark Line is missing");

        updates.push_back(
            application::SketchLineGeometryUpdate{
                id,
                {
                    line->start().u + du,
                    line->start().v + dv,
                },
                {
                    line->end().u + du,
                    line->end().v + dv,
                }});
    }

    return application::
        UpdateSketchGeometryCommand{
            fixture.sketch_id,
            fixture.session.document().revision(),
            std::move(updates),
            {},
            {}};
}

template <typename Operation>
double timedMicroseconds(
    Operation&& operation) {
    const auto started =
        std::chrono::steady_clock::now();
    operation();
    const auto finished =
        std::chrono::steady_clock::now();

    return std::chrono::duration<
               double,
               std::micro>(
               finished - started)
        .count();
}

SampleMeasurement runSample(
    const Options& options,
    std::string_view scenario,
    std::size_t ordinal) {
    const auto setup_started =
        std::chrono::steady_clock::now();

    auto fixture =
        makeFixture(
            options.entities,
            options.depth);

    auto observeSetup =
        [&]() {
            const auto setup_finished =
                std::chrono::steady_clock::now();
            return std::pair{
                std::chrono::duration<
                    double,
                    std::milli>(
                    setup_finished -
                    setup_started)
                    .count(),
                readMemoryCounters()};
        };

    if (scenario == "add") {
        const auto before_count =
            entityCount(
                fixture.session,
                fixture.sketch_id);
        std::optional<sketch::EntityId>
            created;

        const auto [setup_ms, post_setup] =
            observeSetup();

        const double latency =
            timedMicroseconds(
                [&] {
                    const auto result =
                        fixture.session.execute(
                            application::
                                AddSketchLineCommand{
                                    fixture.sketch_id,
                                    {
                                        100000.0 +
                                            static_cast<double>(
                                                ordinal),
                                        10.0,
                                    },
                                    {
                                        100001.0 +
                                            static_cast<double>(
                                                ordinal),
                                        10.5,
                                    }});
                    require(
                        result.ok() &&
                            result.changed &&
                            result.entity_id,
                        "Timed Add failed");
                    created =
                        result.entity_id;
                });

        require(
            entityCount(
                fixture.session,
                fixture.sketch_id) ==
                before_count + 1U,
            "Timed Add produced wrong entity count");
        require(
            requireSketch(
                fixture.session,
                fixture.sketch_id)
                .model.contains(*created),
            "Timed Add result identity is missing");

        return SampleMeasurement{
            setup_ms,
            latency,
            post_setup};
    }

    if (scenario == "edit" ||
        scenario == "transform") {
        const std::size_t update_count =
            scenario == "edit"
                ? 1U
                : std::min<std::size_t>(
                      100U,
                      options.entities);

        const auto first_id =
            fixture.entity_ids.front();
        const auto* before_line =
            requireSketch(
                fixture.session,
                fixture.sketch_id)
                .model.findLine(first_id);
        require(
            before_line != nullptr,
            "Benchmark Line is missing before update");

        const auto before_start =
            before_line->start();
        constexpr double du = 1.0;
        constexpr double dv = 1.0;

        const auto command =
            makeLineUpdate(
                fixture,
                update_count,
                du,
                dv);

        const auto [setup_ms, post_setup] =
            observeSetup();

        application::DocumentSessionResult
            result;
        const double latency =
            timedMicroseconds(
                [&] {
                    result =
                        fixture.session.execute(
                            command);
                });

        require(
            result.ok() && result.changed,
            scenario == "edit"
                ? "Timed single-entity edit failed"
                : "Timed 10-percent transform failed");

        const auto* changed_line =
            requireSketch(
                fixture.session,
                fixture.sketch_id)
                .model.findLine(first_id);
        require(
            changed_line != nullptr &&
                changed_line->start() ==
                    sketch::Point2{
                        before_start.u + du,
                        before_start.v + dv},
            "Timed geometry update produced wrong geometry");

        return SampleMeasurement{
            setup_ms,
            latency,
            post_setup};
    }

    if (scenario == "undo") {
        const auto before_depth =
            fixture.session.undoDepth();
        const auto before_title =
            fixture.session.document()
                .properties()
                .title;

        const auto [setup_ms, post_setup] =
            observeSetup();

        application::DocumentSessionResult
            result;
        const double latency =
            timedMicroseconds(
                [&] {
                    result =
                        fixture.session.undo();
                });

        require(
            result.ok() && result.changed,
            "Timed Undo failed");
        require(
            fixture.session.undoDepth() + 1U ==
                before_depth,
            "Timed Undo produced wrong cursor depth");
        require(
            fixture.session.document()
                    .properties()
                    .title !=
                before_title,
            "Timed Undo did not change authored state");

        return SampleMeasurement{
            setup_ms,
            latency,
            post_setup};
    }

    if (scenario == "redo") {
        const auto target_depth =
            fixture.session.undoDepth();
        const auto target_title =
            fixture.session.document()
                .properties()
                .title;

        const auto prepared =
            fixture.session.undo();
        require(
            prepared.ok() &&
                prepared.changed,
            "Redo preparation Undo failed");

        const auto [setup_ms, post_setup] =
            observeSetup();

        application::DocumentSessionResult
            result;
        const double latency =
            timedMicroseconds(
                [&] {
                    result =
                        fixture.session.redo();
                });

        require(
            result.ok() && result.changed,
            "Timed Redo failed");
        require(
            fixture.session.undoDepth() ==
                    target_depth &&
                fixture.session.document()
                        .properties()
                        .title ==
                    target_title,
            "Timed Redo did not restore expected state");

        return SampleMeasurement{
            setup_ms,
            latency,
            post_setup};
    }

    if (scenario == "branch" ||
        scenario == "branch_half") {
        const auto undo_count =
            scenario == "branch"
                ? 1U
                : std::max<std::size_t>(
                      1U,
                      options.depth / 2U);

        for (std::size_t index = 0U;
             index < undo_count;
             ++index) {
            const auto undone =
                fixture.session.undo();
            require(
                undone.ok() &&
                    undone.changed,
                "Branch preparation Undo failed");
        }

        const auto base_depth =
            fixture.session.undoDepth();
        require(
            fixture.session.redoDepth() ==
                undo_count,
            "Branch preparation did not create expected Redo suffix");

        auto properties =
            fixture.session.document()
                .properties();
        properties.description =
            "C2 branch " +
            std::to_string(ordinal);

        const auto [setup_ms, post_setup] =
            observeSetup();

        application::DocumentSessionResult
            result;
        const double latency =
            timedMicroseconds(
                [&] {
                    result =
                        fixture.session.execute(
                            application::
                                SetDocumentPropertiesCommand{
                                    properties});
                });

        require(
            result.ok() && result.changed,
            "Timed branch command failed");
        require(
            fixture.session.redoDepth() == 0U &&
                fixture.session.undoDepth() ==
                    base_depth + 1U,
            "Timed branch command produced wrong history shape");

        return SampleMeasurement{
            setup_ms,
            latency,
            post_setup};
    }

    fail(
        "Unsupported benchmark scenario: " +
        std::string{scenario});
}

ScenarioResult runRepeated(
    const Options& options,
    std::string name) {
    for (std::size_t index = 0U;
         index < options.warmup;
         ++index) {
        static_cast<void>(
            runSample(
                options,
                name,
                index));
    }

    ScenarioResult result;
    result.name = std::move(name);
    result.latency_us.reserve(
        options.samples);
    result.setup_ms.reserve(
        options.samples);

    for (std::size_t index = 0U;
         index < options.samples;
         ++index) {
        const auto sample =
            runSample(
                options,
                result.name,
                options.warmup + index);

        result.latency_us.push_back(
            sample.latency_us);
        result.setup_ms.push_back(
            sample.setup_ms);

        result.max_post_setup_working_set_bytes =
            std::max(
                result
                    .max_post_setup_working_set_bytes,
                sample
                    .post_setup
                    .working_set_bytes);
        result.max_post_setup_private_usage_bytes =
            std::max(
                result
                    .max_post_setup_private_usage_bytes,
                sample
                    .post_setup
                    .private_usage_bytes);
    }

    result.latency_statistics =
        calculateStatistics(
            result.latency_us);

    return result;
}

std::vector<ScenarioResult>
runScenarios(
    const Options& options) {
    static constexpr std::string_view
        scenarios[]{
            "add",
            "edit",
            "transform",
            "undo",
            "redo",
            "branch",
        };

    std::vector<ScenarioResult> results;

    if (options.scenario == "all") {
        results.reserve(
            std::size(scenarios) + 1U);
        for (const auto scenario :
             scenarios) {
            results.push_back(
                runRepeated(
                    options,
                    std::string{scenario}));
        }

        auto supplemental_options =
            options;
        supplemental_options.samples = 1U;
        supplemental_options.warmup = 0U;
        auto supplemental =
            runRepeated(
                supplemental_options,
                "branch_half");
        supplemental.percentile_limited = true;
        results.push_back(
            std::move(supplemental));
    } else {
        results.push_back(
            runRepeated(
                options,
                options.scenario));
    }

    return results;
}

void writeNumberArray(
    std::ostream& out,
    const std::vector<double>& values) {
    out << "[";
    for (std::size_t index = 0U;
         index < values.size();
         ++index) {
        if (index != 0U) {
            out << ", ";
        }
        out << values[index];
    }
    out << "]";
}

void writeJson(
    std::ostream& out,
    const Options& options,
    const MemoryCounters& final_memory,
    const std::vector<ScenarioResult>& results) {
    std::vector<double> all_setup_ms;
    std::uint64_t max_post_setup_working_set = 0U;
    std::uint64_t max_post_setup_private_usage = 0U;

    for (const auto& result :
         results) {
        all_setup_ms.insert(
            all_setup_ms.end(),
            result.setup_ms.begin(),
            result.setup_ms.end());

        max_post_setup_working_set =
            std::max(
                max_post_setup_working_set,
                result
                    .max_post_setup_working_set_bytes);
        max_post_setup_private_usage =
            std::max(
                max_post_setup_private_usage,
                result
                    .max_post_setup_private_usage_bytes);
    }

    const auto setup_statistics =
        calculateStatistics(
            all_setup_ms);

    out << std::fixed
        << std::setprecision(3);
    out << "{\n";
    out << "  \"status\": \"completed\",\n";
    out << "  \"git_sha\": \""
        << jsonEscape(options.git_sha)
        << "\",\n";
    out << "  \"build_configuration\": \""
        << buildConfiguration()
        << "\",\n";
    out << "  \"compiler\": \""
        << jsonEscape(
               compilerDescription())
        << "\",\n";
    out << "  \"entities\": "
        << options.entities
        << ",\n";
    out << "  \"history_depth\": "
        << options.depth
        << ",\n";
    out << "  \"samples_requested\": "
        << options.samples
        << ",\n";
    out << "  \"warmup_samples\": "
        << options.warmup
        << ",\n";
    out << "  \"sample_fixture_policy\": "
           "\"fresh-equivalent-semantic-baseline\",\n";
    out << "  \"transform_entity_count\": "
        << std::min<std::size_t>(
               100U,
               options.entities)
        << ",\n";
    out << "  \"setup_ms\": "
        << setup_statistics.median
        << ",\n";
    out << "  \"setup_p95_ms\": "
        << setup_statistics.p95
        << ",\n";
    out << "  \"setup_max_ms\": "
        << setup_statistics.maximum
        << ",\n";
    out << "  \"post_setup_working_set_bytes\": "
        << max_post_setup_working_set
        << ",\n";
    out << "  \"post_setup_private_usage_bytes\": "
        << max_post_setup_private_usage
        << ",\n";
    out << "  \"peak_working_set_bytes\": "
        << final_memory.peak_working_set_bytes
        << ",\n";
    out << "  \"final_private_usage_bytes\": "
        << final_memory.private_usage_bytes
        << ",\n";
    out << "  \"results\": [\n";

    for (std::size_t result_index = 0U;
         result_index < results.size();
         ++result_index) {
        const auto& result =
            results[result_index];
        const auto setup_stats =
            calculateStatistics(
                result.setup_ms);

        out << "    {\n";
        out << "      \"scenario\": \""
            << jsonEscape(result.name)
            << "\",\n";
        out << "      \"samples_completed\": "
            << result.latency_us.size()
            << ",\n";
        out << "      \"percentile_limited\": "
            << (result.percentile_limited
                    ? "true"
                    : "false")
            << ",\n";
        out << "      \"median_us\": "
            << result
                   .latency_statistics
                   .median
            << ",\n";
        out << "      \"p95_us\": "
            << result
                   .latency_statistics
                   .p95
            << ",\n";
        out << "      \"max_us\": "
            << result
                   .latency_statistics
                   .maximum
            << ",\n";
        out << "      \"setup_median_ms\": "
            << setup_stats.median
            << ",\n";
        out << "      \"setup_p95_ms\": "
            << setup_stats.p95
            << ",\n";
        out << "      \"setup_max_ms\": "
            << setup_stats.maximum
            << ",\n";
        out << "      \"max_post_setup_working_set_bytes\": "
            << result
                   .max_post_setup_working_set_bytes
            << ",\n";
        out << "      \"max_post_setup_private_usage_bytes\": "
            << result
                   .max_post_setup_private_usage_bytes
            << ",\n";
        out << "      \"latency_us\": ";
        writeNumberArray(
            out,
            result.latency_us);
        out << ",\n";
        out << "      \"setup_ms\": ";
        writeNumberArray(
            out,
            result.setup_ms);
        out << "\n";
        out << "    }";

        if (result_index + 1U !=
            results.size()) {
            out << ",";
        }
        out << "\n";
    }

    out << "  ]\n";
    out << "}\n";
}

int selfTest() {
    const auto odd =
        calculateStatistics(
            std::vector<double>{
                5.0,
                1.0,
                3.0,
                4.0,
                2.0});
    require(
        odd.median == 3.0,
        "Odd median self-test failed");
    require(
        odd.p95 == 5.0,
        "Odd p95 self-test failed");
    require(
        odd.maximum == 5.0,
        "Odd max self-test failed");

    const auto even =
        calculateStatistics(
            std::vector<double>{
                4.0,
                1.0,
                3.0,
                2.0});
    require(
        even.median == 2.5,
        "Even median self-test failed");
    require(
        even.p95 == 4.0,
        "Even p95 self-test failed");

    const auto escaped =
        jsonEscape(
            "a\"b\\c\n");
    require(
        escaped ==
            "a\\\"b\\\\c\\n",
        "JSON escape self-test failed");

    require(
        validScenario("branch") &&
            validScenario("branch_half"),
        "Scenario contract self-test failed");

    std::cout
        << "[c2-benchmark] self-test passed\n";
    return EXIT_SUCCESS;
}

} // namespace

int main(int argc, char** argv) {
    try {
        const auto options =
            parseOptions(
                argc,
                argv);

        if (options.self_test) {
            return selfTest();
        }

        const auto results =
            runScenarios(
                options);

        const auto final_memory =
            readMemoryCounters();

        std::ostringstream json;
        writeJson(
            json,
            options,
            final_memory,
            results);

        std::cout
            << json.str();

        if (!options.output.empty()) {
            const auto parent =
                options.output.parent_path();
            if (!parent.empty()) {
                std::filesystem::
                    create_directories(parent);
            }

            std::ofstream file{
                options.output,
                std::ios::binary |
                    std::ios::trunc};
            require(
                static_cast<bool>(file),
                "Unable to open benchmark output file");
            file << json.str();
            require(
                static_cast<bool>(file),
                "Unable to write benchmark output file");
        }

        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr
            << "[c2-benchmark] ERROR: "
            << error.what()
            << '\n';
        return 2;
    }
}
