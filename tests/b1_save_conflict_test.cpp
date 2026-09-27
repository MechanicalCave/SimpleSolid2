#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/part/part_document_store.hpp>
#include <simplesolid2/persistence/file_guard.hpp>
#include <simplesolid2/persistence/file_snapshot.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <thread>
#include <string>

using namespace simplesolid2;

namespace {

void check(
    bool value,
    const char* expression,
    int line) {
    if (!value) {
        std::cerr
            << "B1 Save conflict CHECK failed at line "
            << line << ": "
            << expression << '\n';
        std::abort();
    }
}
#define CHECK(expr) \
    check(static_cast<bool>(expr), #expr, __LINE__)

struct TempDirectory final {
    std::filesystem::path path;

    TempDirectory() {
        path =
            std::filesystem::temp_directory_path() /
            ("simplesolid2_b1_" +
             std::to_string(
                 std::filesystem::
                     file_time_type::clock::now()
                         .time_since_epoch()
                         .count()));
        std::filesystem::create_directories(path);
    }

    ~TempDirectory() {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};

std::string readBytes(
    const std::filesystem::path& path) {
    std::ifstream in{
        path,
        std::ios::binary};
    CHECK(static_cast<bool>(in));
    return std::string{
        std::istreambuf_iterator<char>{in},
        std::istreambuf_iterator<char>{}};
}

void writeBytesInPlace(
    const std::filesystem::path& path,
    const std::string& bytes) {
    std::ofstream out{
        path,
        std::ios::binary |
            std::ios::in |
            std::ios::out |
            std::ios::trunc};
    CHECK(static_cast<bool>(out));
    out.write(
        bytes.data(),
        static_cast<std::streamsize>(
            bytes.size()));
    CHECK(static_cast<bool>(out));
}

void setTitle(
    application::DocumentSession& session,
    std::string value) {
    auto properties =
        session.document().properties();
    properties.title = std::move(value);
    const auto result =
        session.execute(
            application::
                SetDocumentPropertiesCommand{
                    std::move(properties)});
    CHECK(result.ok());
    CHECK(result.changed);
}

application::DocumentSession openSession(
    const std::filesystem::path& path) {
    part::PartDocumentStore store;
    auto loaded = store.load(path);
    CHECK(loaded.ok());
    return application::DocumentSession{
        path,
        std::move(*loaded.document),
        *loaded.checkpoint};
}

} // namespace

int main() {
    CHECK(
        persistence::sha256Hex(
            persistence::sha256("abc")) ==
        "ba7816bf8f01cfea414140de5dae2223"
        "b00361a396177a9cb410ff61f20015ad");

    TempDirectory temp;
    part::PartDocumentStore store;

    // Sequential two-session lost-update prevention.
    const auto shared_path =
        temp.path / "Shared.ss2part";
    auto seed =
        part::PartDocument::create(
            core::DocumentId::generate());
    const auto created =
        store.createNew(shared_path, seed);
    CHECK(created.ok());

    auto first = openSession(shared_path);
    auto second = openSession(shared_path);

    setTitle(first, "Writer A");
    CHECK(first.save().ok());
    const auto durable_after_first =
        readBytes(shared_path);

    setTitle(second, "Writer B");
    const auto second_state =
        second.document().state();
    const auto second_revision =
        second.document().revision();
    const auto second_undo =
        second.undoDepth();
    const auto second_redo =
        second.redoDepth();
    const auto second_checkpoint =
        second.fileCheckpoint();
    CHECK(second_checkpoint.has_value());

    const auto stale_save = second.save();
    CHECK(!stale_save.ok());
    CHECK(
        stale_save.diagnostic.code ==
        application::DocumentSessionErrorCode::
            save_conflict);
    CHECK(
        stale_save.diagnostic.store_code ==
        part::PartStoreErrorCode::
            save_conflict_file_replaced);
    CHECK(
        readBytes(shared_path) ==
        durable_after_first);
    CHECK(
        second.document().state() ==
        second_state);
    CHECK(
        second.document().revision() ==
        second_revision);
    CHECK(second.undoDepth() == second_undo);
    CHECK(second.redoDepth() == second_redo);
    CHECK(
        second.fileCheckpoint() ==
        second_checkpoint);
    CHECK(second.needsSave());

    // Guard ownership is fail-fast and cannot publish. Hold the
    // cooperative guard on another thread because a Windows mutex is
    // recursive for its owning thread.
    {
        auto guarded = openSession(shared_path);
        setTitle(guarded, "Guarded writer");
        const auto before =
            readBytes(shared_path);

        std::promise<bool> acquired_promise;
        auto acquired =
            acquired_promise.get_future();
        std::promise<void> release_promise;
        auto release =
            release_promise.get_future();

        std::thread holder{
            [&] {
                auto guard =
                    persistence::
                        acquireCooperativeSaveGuard(
                            shared_path);
                acquired_promise.set_value(
                    guard.ok());
                if (guard.ok()) {
                    release.wait();
                }
            }};
        CHECK(acquired.get());

        const auto busy = guarded.save();
        CHECK(!busy.ok());
        CHECK(
            busy.diagnostic.store_code ==
            part::PartStoreErrorCode::
                save_conflict_busy);
        CHECK(readBytes(shared_path) == before);
        CHECK(guarded.needsSave());

        release_promise.set_value();
        holder.join();
    }

    // Same file object, same byte length, changed bytes -> content conflict.
    const auto content_path =
        temp.path / "Content.ss2part";
    auto content_doc =
        part::PartDocument::create(
            core::DocumentId::generate());
    CHECK(
        store.createNew(
            content_path,
            content_doc)
            .ok());
    auto content_session =
        openSession(content_path);
    setTitle(content_session, "Local content");
    auto changed_bytes =
        readBytes(content_path);
    CHECK(!changed_bytes.empty());
    changed_bytes[
        changed_bytes.size() / 2U] ^=
        static_cast<char>(0x01);
    writeBytesInPlace(
        content_path,
        changed_bytes);
    const auto content_conflict =
        content_session.save();
    CHECK(!content_conflict.ok());
    CHECK(
        content_conflict.diagnostic.store_code ==
        part::PartStoreErrorCode::
            save_conflict_content_changed);
    CHECK(content_session.needsSave());

    // Byte-identical replacement with same DocumentId is still replacement.
    const auto replaced_path =
        temp.path / "Replaced.ss2part";
    auto replaced_doc =
        part::PartDocument::create(
            core::DocumentId::generate());
    CHECK(
        store.createNew(
            replaced_path,
            replaced_doc)
            .ok());
    auto replaced_session =
        openSession(replaced_path);
    setTitle(replaced_session, "Local replacement");
    const auto identical =
        readBytes(replaced_path);
    const auto replacement =
        temp.path / "Replacement.tmp";
    writeBytesInPlace(
        replacement,
        identical);
    CHECK(std::filesystem::remove(
        replaced_path));
    std::filesystem::rename(
        replacement,
        replaced_path);
    const auto replaced_conflict =
        replaced_session.save();
    CHECK(!replaced_conflict.ok());
    CHECK(
        replaced_conflict.diagnostic.store_code ==
        part::PartStoreErrorCode::
            save_conflict_file_replaced);

    // Valid native replacement with another DocumentId reports identity change.
    const auto identity_path =
        temp.path / "Identity.ss2part";
    auto identity_doc =
        part::PartDocument::create(
            core::DocumentId::generate());
    CHECK(
        store.createNew(
            identity_path,
            identity_doc)
            .ok());
    auto identity_session =
        openSession(identity_path);
    setTitle(identity_session, "Local identity");

    const auto other_path =
        temp.path / "Other.ss2part";
    auto other_doc =
        part::PartDocument::create(
            core::DocumentId::generate());
    CHECK(
        store.createNew(
            other_path,
            other_doc)
            .ok());
    CHECK(std::filesystem::remove(
        identity_path));
    std::filesystem::rename(
        other_path,
        identity_path);
    const auto identity_conflict =
        identity_session.save();
    CHECK(!identity_conflict.ok());
    CHECK(
        identity_conflict.diagnostic.store_code ==
        part::PartStoreErrorCode::
            save_conflict_document_identity_changed);

    // Missing target is not recreated by ordinary Save.
    const auto missing_path =
        temp.path / "Missing.ss2part";
    auto missing_doc =
        part::PartDocument::create(
            core::DocumentId::generate());
    CHECK(
        store.createNew(
            missing_path,
            missing_doc)
            .ok());
    auto missing_session =
        openSession(missing_path);
    setTitle(missing_session, "Local missing");
    CHECK(std::filesystem::remove(
        missing_path));
    const auto missing =
        missing_session.save();
    CHECK(!missing.ok());
    CHECK(
        missing.diagnostic.store_code ==
        part::PartStoreErrorCode::
            save_conflict_target_missing);
    CHECK(!std::filesystem::exists(
        missing_path));
    CHECK(missing_session.needsSave());

    // Successful Save updates checkpoint; explicit repeated Save remains safe.
    const auto normal_path =
        temp.path / "Normal.ss2part";
    auto normal_doc =
        part::PartDocument::create(
            core::DocumentId::generate());
    CHECK(
        store.createNew(
            normal_path,
            normal_doc)
            .ok());
    auto normal_session =
        openSession(normal_path);
    const auto old_checkpoint =
        normal_session.fileCheckpoint();
    CHECK(old_checkpoint.has_value());
    setTitle(normal_session, "Normal Save");
    CHECK(normal_session.save().ok());
    CHECK(!normal_session.needsSave());
    CHECK(
        normal_session.fileCheckpoint() !=
        old_checkpoint);
    CHECK(normal_session.save().ok());
    CHECK(!normal_session.needsSave());

    auto reopened =
        store.load(normal_path);
    CHECK(reopened.ok());
    CHECK(
        reopened.document->properties().title ==
        "Normal Save");

    return EXIT_SUCCESS;
}
