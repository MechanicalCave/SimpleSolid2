#include <simplesolid2/persistence/atomic_file.hpp>

#include <chrono>
#include <fstream>
#include <random>
#include <sstream>
#include <system_error>

#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#endif

namespace simplesolid2::persistence {
namespace {

AtomicWriteResult failure(
    AtomicWriteErrorCode code,
    std::string message,
    std::filesystem::path path) {
    return AtomicWriteResult{
        AtomicWriteDiagnostic{code, std::move(message), std::move(path)},
    };
}

std::string temporarySuffix() {
    std::random_device rd;
    std::mt19937_64 engine{
        static_cast<std::mt19937_64::result_type>(
            std::chrono::high_resolution_clock::now().time_since_epoch().count()) ^
        (static_cast<std::mt19937_64::result_type>(rd()) << 1U)};
    std::ostringstream out;
    out << ".tmp-" << std::hex << engine();
    return out.str();
}

void cleanup(const std::filesystem::path& path) noexcept {
    std::error_code ec;
    std::filesystem::remove(path, ec);
}

bool publishReplacing(
    const std::filesystem::path& temporary,
    const std::filesystem::path& target) noexcept {
#if defined(_WIN32)
    return ::MoveFileExW(
               temporary.c_str(),
               target.c_str(),
               MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    std::error_code ec;
    std::filesystem::rename(temporary, target, ec);
    return !ec;
#endif
}

} // namespace

AtomicWriteResult writeFileAtomically(
    const std::filesystem::path& path,
    std::string_view content,
    AtomicWriteMode mode) {
    if (path.empty() || path.filename().empty()) {
        return failure(
            AtomicWriteErrorCode::invalid_parent,
            "Target file path must not be empty",
            path);
    }

    const auto parent = path.parent_path().empty()
                            ? std::filesystem::current_path()
                            : path.parent_path();

    std::error_code ec;
    if (!std::filesystem::is_directory(parent, ec) || ec) {
        return failure(
            AtomicWriteErrorCode::invalid_parent,
            "Target parent directory does not exist or is not accessible",
            parent);
    }

    const bool exists = std::filesystem::exists(path, ec);
    if (ec) {
        return failure(
            AtomicWriteErrorCode::io_failure,
            "Unable to inspect target file",
            path);
    }
    if (mode == AtomicWriteMode::create_new && exists) {
        return failure(
            AtomicWriteErrorCode::target_exists,
            "Target file already exists",
            path);
    }
    if (exists && !std::filesystem::is_regular_file(path, ec)) {
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
    for (unsigned attempt = 0; attempt < 64U; ++attempt) {
        temporary = path;
        temporary += temporarySuffix() + "-" + std::to_string(attempt);
        ec.clear();
        if (!std::filesystem::exists(temporary, ec) && !ec) {
            reserved = true;
            break;
        }
        if (ec) {
            return failure(
                AtomicWriteErrorCode::io_failure,
                "Unable to reserve temporary file name",
                temporary);
        }
    }
    if (!reserved) {
        return failure(
            AtomicWriteErrorCode::io_failure,
            "Unable to reserve temporary file name",
            path);
    }

    {
        std::ofstream out{temporary, std::ios::binary | std::ios::trunc};
        if (!out) {
            return failure(
                AtomicWriteErrorCode::io_failure,
                "Unable to create temporary file",
                temporary);
        }
        out.write(content.data(), static_cast<std::streamsize>(content.size()));
        out.flush();
        if (!out) {
            cleanup(temporary);
            return failure(
                AtomicWriteErrorCode::io_failure,
                "Unable to write temporary file",
                temporary);
        }
    }

    if (mode == AtomicWriteMode::create_new) {
        ec.clear();
        if (std::filesystem::exists(path, ec)) {
            cleanup(temporary);
            if (ec) {
                return failure(
                    AtomicWriteErrorCode::io_failure,
                    "Unable to recheck target file before publish",
                    path);
            }
            return failure(
                AtomicWriteErrorCode::target_exists,
                "Target file appeared while the new file was being created",
                path);
        }
        ec.clear();
        std::filesystem::rename(temporary, path, ec);
        if (ec) {
            cleanup(temporary);
            return failure(
                AtomicWriteErrorCode::io_failure,
                "Unable to publish new file",
                path);
        }
        return AtomicWriteResult{};
    }

    if (!publishReplacing(temporary, path)) {
        cleanup(temporary);
        return failure(
            AtomicWriteErrorCode::io_failure,
            "Unable to atomically replace target file",
            path);
    }

    return AtomicWriteResult{};
}

} // namespace simplesolid2::persistence
