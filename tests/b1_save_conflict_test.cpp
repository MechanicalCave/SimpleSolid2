#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/part/part_document_store.hpp>
#include <simplesolid2/persistence/file_guard.hpp>
#include <simplesolid2/persistence/file_snapshot.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <optional>
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

struct SessionInvariantSnapshot final {
    part::PartAuthoredState state;
    core::DocumentRevision revision;
    std::size_t undo_depth{};
    std::size_t redo_depth{};
    std::optional<part::PartFileCheckpoint>
        checkpoint;
};

SessionInvariantSnapshot captureSession(
    const application::DocumentSession& session) {
    return SessionInvariantSnapshot{
        session.document().state(),
        session.document().revision(),
        session.undoDepth(),
        session.redoDepth(),
        session.fileCheckpoint(),
    };
}

void checkSessionUnchanged(
    const application::DocumentSession& session,
    const SessionInvariantSnapshot& before) {
    CHECK(session.document().state() == before.state);
    CHECK(session.document().revision() == before.revision);
    CHECK(session.undoDepth() == before.undo_depth);
    CHECK(session.redoDepth() == before.redo_depth);
    CHECK(session.fileCheckpoint() == before.checkpoint);
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

    // Controlled concurrent cooperating Save from the same checkpoint.
    // Both real sessions are released together; exactly one may publish.
    {
        const auto concurrent_path =
            temp.path / "Concurrent.ss2part";
        auto concurrent_doc =
            part::PartDocument::create(
                core::DocumentId::generate());
        CHECK(
            store.createNew(
                concurrent_path,
                concurrent_doc)
                .ok());

        auto left = openSession(concurrent_path);
        auto right = openSession(concurrent_path);
        setTitle(left, "Concurrent A");
        setTitle(right, "Concurrent B");
        const auto left_before =
            captureSession(left);
        const auto right_before =
            captureSession(right);

        std::promise<void> left_ready_promise;
        std::promise<void> right_ready_promise;
        auto left_ready =
            left_ready_promise.get_future();
        auto right_ready =
            right_ready_promise.get_future();
        std::promise<void> start_promise;
        auto start =
            start_promise.get_future().share();

        auto left_future =
            std::async(
                std::launch::async,
                [&] {
                    left_ready_promise.set_value();
                    start.wait();
                    return left.save();
                });
        auto right_future =
            std::async(
                std::launch::async,
                [&] {
                    right_ready_promise.set_value();
                    start.wait();
                    return right.save();
                });

        left_ready.wait();
        right_ready.wait();
        start_promise.set_value();

        const auto left_result =
            left_future.get();
        const auto right_result =
            right_future.get();
        CHECK(
            left_result.ok() !=
            right_result.ok());

        const bool left_won =
            left_result.ok();
        const auto& loser_result =
            left_won
                ? right_result
                : left_result;
        auto& loser =
            left_won ? right : left;
        const auto& loser_before =
            left_won
                ? right_before
                : left_before;

        CHECK(!loser_result.ok());
        CHECK(
            loser_result.diagnostic.code ==
            application::DocumentSessionErrorCode::
                save_conflict);
        CHECK(
            loser_result.diagnostic.store_code ==
                part::PartStoreErrorCode::
                    save_conflict_busy ||
            loser_result.diagnostic.store_code ==
                part::PartStoreErrorCode::
                    save_conflict_file_replaced);
        checkSessionUnchanged(
            loser,
            loser_before);
        CHECK(loser.needsSave());

        auto& winner =
            left_won ? left : right;
        CHECK(!winner.needsSave());

        const auto persisted =
            store.load(concurrent_path);
        CHECK(persisted.ok());
        CHECK(
            persisted.document->properties().title ==
            (left_won
                 ? "Concurrent A"
                 : "Concurrent B"));
    }

    // Guard ownership is fail-fast and cannot publish. Hold the
    // cooperative guard on another thread because a Windows mutex is
    // recursive for its owning thread.
    {
        auto guarded = openSession(shared_path);
        setTitle(guarded, "Guarded writer");
        const auto guarded_before =
            captureSession(guarded);
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
        checkSessionUnchanged(
            guarded,
            guarded_before);
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
    const auto content_before =
        captureSession(content_session);
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
    checkSessionUnchanged(
        content_session,
        content_before);
    CHECK(
        readBytes(content_path) ==
        changed_bytes);
    CHECK(content_session.needsSave());

    // Explicit Save still checks the file checkpoint when authored state is clean.
    {
        const auto clean_path =
            temp.path / "CleanExternal.ss2part";
        auto clean_doc =
            part::PartDocument::create(
                core::DocumentId::generate());
        CHECK(
            store.createNew(
                clean_path,
                clean_doc)
                .ok());
        auto clean_session =
            openSession(clean_path);
        CHECK(!clean_session.needsSave());
        const auto clean_before =
            captureSession(clean_session);

        auto clean_changed =
            readBytes(clean_path);
        CHECK(!clean_changed.empty());
        clean_changed[
            clean_changed.size() / 2U] ^=
            static_cast<char>(0x01);
        writeBytesInPlace(
            clean_path,
            clean_changed);

        const auto clean_conflict =
            clean_session.save();
        CHECK(!clean_conflict.ok());
        CHECK(
            clean_conflict.diagnostic.code ==
            application::DocumentSessionErrorCode::
                save_conflict);
        CHECK(
            clean_conflict.diagnostic.store_code ==
            part::PartStoreErrorCode::
                save_conflict_content_changed);
        checkSessionUnchanged(
            clean_session,
            clean_before);
        CHECK(!clean_session.needsSave());
        CHECK(
            readBytes(clean_path) ==
            clean_changed);
    }

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
    const auto replaced_before =
        captureSession(replaced_session);
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
    checkSessionUnchanged(
        replaced_session,
        replaced_before);
    CHECK(replaced_session.needsSave());
    CHECK(
        readBytes(replaced_path) ==
        identical);

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
    const auto identity_before =
        captureSession(identity_session);

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
    const auto other_bytes =
        readBytes(other_path);
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
    checkSessionUnchanged(
        identity_session,
        identity_before);
    CHECK(identity_session.needsSave());
    CHECK(
        readBytes(identity_path) ==
        other_bytes);

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
    const auto missing_before =
        captureSession(missing_session);
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
    checkSessionUnchanged(
        missing_session,
        missing_before);
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

    // Competing create-new publications cannot both claim one target.
    {
        const auto create_path =
            temp.path / "ConcurrentCreate.ss2part";
        auto create_a =
            part::PartDocument::create(
                core::DocumentId::generate());
        auto create_b =
            part::PartDocument::create(
                core::DocumentId::generate());
        const auto id_a =
            create_a.documentId();
        const auto id_b =
            create_b.documentId();

        std::promise<void> a_ready_promise;
        std::promise<void> b_ready_promise;
        auto a_ready =
            a_ready_promise.get_future();
        auto b_ready =
            b_ready_promise.get_future();
        std::promise<void> start_promise;
        auto start =
            start_promise.get_future().share();

        auto a_future =
            std::async(
                std::launch::async,
                [&] {
                    a_ready_promise.set_value();
                    start.wait();
                    part::PartDocumentStore local_store;
                    return local_store.createNew(
                        create_path,
                        create_a);
                });
        auto b_future =
            std::async(
                std::launch::async,
                [&] {
                    b_ready_promise.set_value();
                    start.wait();
                    part::PartDocumentStore local_store;
                    return local_store.createNew(
                        create_path,
                        create_b);
                });

        a_ready.wait();
        b_ready.wait();
        start_promise.set_value();

        const auto a_result =
            a_future.get();
        const auto b_result =
            b_future.get();
        CHECK(a_result.ok() != b_result.ok());

        const auto& loser =
            a_result.ok()
                ? b_result
                : a_result;
        CHECK(
            loser.diagnostic.code ==
            part::PartStoreErrorCode::
                target_exists);

        const auto created_race =
            store.load(create_path);
        CHECK(created_race.ok());
        CHECK(
            created_race.document->documentId() ==
            (a_result.ok() ? id_a : id_b));
    }

    return EXIT_SUCCESS;
}
