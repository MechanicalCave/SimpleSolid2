#include <simplesolid2/application/document_workspace.hpp>
#include <simplesolid2/part/part_document_store.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

using namespace simplesolid2;

namespace {
void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr << "PART-01 discovery CHECK failed at line "
                  << line << ": " << expression << '\n';
        std::abort();
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

struct TempDirectory final {
    std::filesystem::path path;
    TempDirectory() {
        path = std::filesystem::temp_directory_path() /
               ("simplesolid2_document_discovery_" +
                std::to_string(
                    std::filesystem::file_time_type::clock::now()
                        .time_since_epoch().count()));
        std::filesystem::create_directories(path);
    }
    ~TempDirectory() {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};

const application::DocumentIndexEntry* findById(
    const application::DocumentWorkspaceIndex& index,
    const core::DocumentId& id) {
    for (const auto& entry : index.entries()) {
        if (entry.document_id && *entry.document_id == id) return &entry;
    }
    return nullptr;
}
}

int main() {
    TempDirectory temp;
    std::filesystem::create_directories(temp.path / "A");
    std::filesystem::create_directories(temp.path / "B");

    part::PartDocumentStore store;
    const auto id = core::DocumentId::generate();
    auto document = part::PartDocument::create(id);
    CHECK(store.createNew(temp.path / "A" / "Gear.ss2part", document).ok());

    application::DocumentWorkspaceIndex index;
    CHECK(index.refresh(temp.path).ok());
    const auto resolved = index.resolve(id);
    CHECK(resolved.ok());
    CHECK(resolved.relative_paths.size() == 1U);
    CHECK(resolved.relative_paths.front() ==
          std::filesystem::path{"A"} / "Gear.ss2part");

    std::filesystem::copy_file(
        temp.path / "A" / "Gear.ss2part",
        temp.path / "B" / "Gear.ss2part");
    CHECK(index.refresh(temp.path).ok());

    const auto conflict = index.resolve(id);
    CHECK(!conflict.ok());
    CHECK(conflict.state == application::DocumentResolutionState::identity_conflict);
    CHECK(conflict.relative_paths.size() == 2U);
    const auto* conflict_entry = findById(index, id);
    CHECK(conflict_entry != nullptr);
    CHECK(conflict_entry->state ==
          application::DocumentIndexState::identity_conflict);

    const auto independent_id = core::DocumentId::generate();
    auto independent = part::PartDocument::create(independent_id);
    CHECK(store.createNew(temp.path / "Independent.ss2part", independent).ok());

    {
        std::ofstream out{temp.path / "Broken.ss2part", std::ios::binary};
        out << "broken\n";
    }

    CHECK(index.refresh(temp.path).ok());
    CHECK(index.resolve(independent_id).ok());

    bool found_invalid = false;
    for (const auto& entry : index.entries()) {
        if (entry.state == application::DocumentIndexState::invalid) {
            found_invalid = true;
            CHECK(entry.relative_paths.size() == 1U);
            CHECK(entry.relative_paths.front() == "Broken.ss2part");
        }
    }
    CHECK(found_invalid);

    return EXIT_SUCCESS;
}
