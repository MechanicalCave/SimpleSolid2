#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void check(
    bool value,
    const char* expression,
    int line) {
    if (!value) {
        std::cerr
            << "PM-00A E07 CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

struct ReplayCase final {
    std::string_view row;
    std::string_view executable;
};

[[nodiscard]] std::filesystem::path
siblingExecutable(
    const std::filesystem::path& self,
    std::string_view stem) {
    const auto directory =
        std::filesystem::absolute(self)
            .parent_path();

    auto name = std::string{stem};
#ifdef _WIN32
    name += ".exe";
#endif
    return directory / name;
}

[[nodiscard]] int
runFreshProcess(
    const std::filesystem::path& executable) {
    CHECK(std::filesystem::exists(executable));

    std::string command;
    command.reserve(
        executable.string().size() + 4U);
    command += '"';
    command += executable.string();
    command += '"';

    return std::system(command.c_str());
}

void verifyFreshProcessParity(
    const std::filesystem::path& self,
    const ReplayCase& item) {
    const auto executable =
        siblingExecutable(
            self,
            item.executable);

    // E07 requires previous runtime/provider state to be unavailable. Two
    // independent child-process executions are stronger than a warm in-process
    // retry: the second invocation cannot observe DocumentSession, B-Rep,
    // OCCT operation objects, provider handles or lineage caches from the
    // first process.
    const int first =
        runFreshProcess(executable);
    CHECK(first == 0);

    const int cold =
        runFreshProcess(executable);
    CHECK(cold == 0);

    std::cout
        << "E07_REPLAY row="
        << item.row
        << " first=PASS cold_process=PASS\n";
}

} // namespace

int main(int argc, char** argv) {
    CHECK(argc > 0);
    CHECK(argv != nullptr);
    CHECK(argv[0] != nullptr);

    const std::filesystem::path self{
        argv[0]};

    const std::vector<ReplayCase> cases{
        {
            "E07-01",
            "pm00a_e01_extrude_evidence_test",
        },
        {
            "E07-02",
            "pm00a_e02_multistage_lineage_test",
        },
        {
            "E07-03+E07-04",
            "pm00a_e03_e04_cardinality_evidence_test",
        },
        {
            "E07-05",
            "pm00a_e05_similarity_evidence_test",
        },
    };

    for (const auto& item : cases) {
        verifyFreshProcessParity(
            self,
            item);
    }

    // E07-06 intentionally remains outside this executable until E06 proves
    // the supported full-Revolve semantic-side behavior. PM-00A must not
    // manufacture Revolve semantics merely to make the E07 table look done.
    std::cout
        << "PM00A_E07_PARTIAL_PASS rows=E07-01..E07-05"
        << " pending=E07-06"
        << " false_resolved=0\n";
    return EXIT_SUCCESS;
}
