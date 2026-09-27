#include <simplesolid2/persistence/atomic_file.hpp>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <random>
#include <sstream>
#include <system_error>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace simplesolid2::persistence {
namespace {

AtomicWriteResult failure(
    AtomicWriteErrorCode code,
    std::string message,
    std::filesystem::path path) {
    return AtomicWriteResult{
        std::nullopt,
        AtomicWriteDiagnostic{
            code,
            std::move(message),
            std::move(path)},
    };
}

std::string temporarySuffix() {
    std::random_device rd;
    std::mt19937_64 engine{
        static_cast<std::mt19937_64::result_type>(
            std::chrono::high_resolution_clock::now()
                .time_since_epoch().count()) ^
        (static_cast<std::mt19937_64::result_type>(
             rd()) << 1U)};
    std::ostringstream out;
    out << ".tmp-" << std::hex << engine();
    return out.str();
}

void cleanup(
    const std::filesystem::path& path) noexcept {
    std::error_code ec;
    std::filesystem::remove(path, ec);
}

enum class ExclusiveWriteResult {
    success,
    exists,
    failure,
};

ExclusiveWriteResult writeExclusive(
    const std::filesystem::path& path,
    std::string_view content) {
#if defined(_WIN32)
    HANDLE handle =
        ::CreateFileW(
            path.c_str(),
            GENERIC_WRITE,
            0U,
            nullptr,
            CREATE_NEW,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        return ::GetLastError() ==
                       ERROR_FILE_EXISTS ||
                   ::GetLastError() ==
                       ERROR_ALREADY_EXISTS
            ? ExclusiveWriteResult::exists
            : ExclusiveWriteResult::failure;
    }

    std::size_t offset = 0U;
    bool ok = true;
    while (offset < content.size()) {
        const auto remaining =
            content.size() - offset;
        const DWORD request =
            static_cast<DWORD>(
                std::min<std::size_t>(
                    remaining,
                    1U << 20U));
        DWORD written = 0U;
        if (!::WriteFile(
                handle,
                content.data() + offset,
                request,
                &written,
                nullptr) ||
            written == 0U) {
            ok = false;
            break;
        }
        offset += written;
    }
    if (ok) {
        ok = ::FlushFileBuffers(handle) != 0;
    }
    ::CloseHandle(handle);
    return ok
        ? ExclusiveWriteResult::success
        : ExclusiveWriteResult::failure;
#else
    const int fd =
        ::open(
            path.c_str(),
            O_WRONLY | O_CREAT | O_EXCL,
            0600);
    if (fd < 0) {
        return errno == EEXIST
            ? ExclusiveWriteResult::exists
            : ExclusiveWriteResult::failure;
    }

    std::size_t offset = 0U;
    bool ok = true;
    while (offset < content.size()) {
        const auto written =
            ::write(
                fd,
                content.data() + offset,
                content.size() - offset);
        if (written <= 0) {
            ok = false;
            break;
        }
        offset +=
            static_cast<std::size_t>(
                written);
    }
    if (::close(fd) != 0) ok = false;
    return ok
        ? ExclusiveWriteResult::success
        : ExclusiveWriteResult::failure;
#endif
}

bool publishReplacing(
    const std::filesystem::path& temporary,
    const std::filesystem::path& target) noexcept {
#if defined(_WIN32)
    return ::MoveFileExW(
               temporary.c_str(),
               target.c_str(),
               MOVEFILE_REPLACE_EXISTING |
                   MOVEFILE_WRITE_THROUGH) != 0;
#else
    std::error_code ec;
    std::filesystem::rename(
        temporary,
        target,
        ec);
    return !ec;
#endif
}

bool publishNew(
    const std::filesystem::path& temporary,
    const std::filesystem::path& target) noexcept {
#if defined(_WIN32)
    return ::MoveFileExW(
               temporary.c_str(),
               target.c_str(),
               MOVEFILE_WRITE_THROUGH) != 0;
#else
    if (::link(
            temporary.c_str(),
            target.c_str()) != 0) {
        return false;
    }
    return ::unlink(
               temporary.c_str()) == 0;
#endif
}

} // namespace

AtomicWriteResult writeFileAtomically(
    const std::filesystem::path& path,
    std::string_view content,
    AtomicWriteMode mode) {
    if (path.empty() ||
        path.filename().empty()) {
        return failure(
            AtomicWriteErrorCode::invalid_parent,
            "Target file path must not be empty",
            path);
    }

    const auto parent =
        path.parent_path().empty()
            ? std::filesystem::current_path()
            : path.parent_path();

    std::error_code ec;
    if (!std::filesystem::is_directory(
            parent,
            ec) ||
        ec) {
        return failure(
            AtomicWriteErrorCode::invalid_parent,
            "Target parent directory does not exist or is not accessible",
            parent);
    }

    const bool exists =
        std::filesystem::exists(path, ec);
    if (ec) {
        return failure(
            AtomicWriteErrorCode::io_failure,
            "Unable to inspect target file",
            path);
    }
    if (mode == AtomicWriteMode::create_new &&
        exists) {
        return failure(
            AtomicWriteErrorCode::target_exists,
            "Target file already exists",
            path);
    }
    if (exists &&
        !std::filesystem::is_regular_file(
            path,
            ec)) {
        return failure(
            AtomicWriteErrorCode::target_not_regular,
            "Target path exists but is not a regular file",
            path);
    }
    if (ec) {
        return failure(
            AtomicWriteErrorCode::io_failure,
            "Unable to inspect target file type",
            path);
    }

    std::filesystem::path temporary;
    bool reserved = false;
    for (unsigned attempt = 0U;
         attempt < 64U;
         ++attempt) {
        temporary = path;
        temporary +=
            temporarySuffix() + "-" +
            std::to_string(attempt);
        const auto written =
            writeExclusive(
                temporary,
                content);
        if (written ==
            ExclusiveWriteResult::success) {
            reserved = true;
            break;
        }
        if (written ==
            ExclusiveWriteResult::failure) {
            cleanup(temporary);
            return failure(
                AtomicWriteErrorCode::io_failure,
                "Unable to create exclusive temporary file",
                temporary);
        }
    }

    if (!reserved) {
        return failure(
            AtomicWriteErrorCode::io_failure,
            "Unable to reserve exclusive temporary file",
            path);
    }

    const auto temporary_snapshot =
        readFileSnapshot(
            temporary,
            content.size());
    if (!temporary_snapshot.ok() ||
        temporary_snapshot.snapshot->bytes !=
            content) {
        cleanup(temporary);
        return failure(
            AtomicWriteErrorCode::io_failure,
            "Unable to verify temporary publication file",
            temporary);
    }
    const auto identity =
        temporary_snapshot.snapshot->identity;

    const bool published =
        mode == AtomicWriteMode::create_new
            ? publishNew(
                  temporary,
                  path)
            : publishReplacing(
                  temporary,
                  path);
    if (!published) {
        const auto existed =
            mode ==
                AtomicWriteMode::create_new &&
            std::filesystem::exists(
                path,
                ec) &&
            !ec;
        cleanup(temporary);
        return failure(
            existed
                ? AtomicWriteErrorCode::target_exists
                : AtomicWriteErrorCode::io_failure,
            existed
                ? "Target file appeared before create-new publication"
                : "Unable to atomically publish target file",
            path);
    }

    return AtomicWriteResult{
        identity,
        AtomicWriteDiagnostic{},
    };
}

} // namespace simplesolid2::persistence
