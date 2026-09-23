#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace simplesolid2::persistence {

enum class AtomicWriteMode {
    create_new,
    replace,
};

enum class AtomicWriteErrorCode {
    none,
    invalid_parent,
    target_exists,
    target_not_regular,
    io_failure,
};

struct AtomicWriteDiagnostic final {
    AtomicWriteErrorCode code{AtomicWriteErrorCode::none};
    std::string message;
    std::filesystem::path path;
};

struct AtomicWriteResult final {
    AtomicWriteDiagnostic diagnostic;

    [[nodiscard]] bool ok() const noexcept {
        return diagnostic.code == AtomicWriteErrorCode::none;
    }
};

[[nodiscard]] AtomicWriteResult writeFileAtomically(
    const std::filesystem::path& path,
    std::string_view content,
    AtomicWriteMode mode);

} // namespace simplesolid2::persistence
