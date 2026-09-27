#include <simplesolid2/persistence/file_guard.hpp>

#include <simplesolid2/persistence/file_snapshot.hpp>

#include <algorithm>
#include <cerrno>
#include <cwctype>
#include <string>
#include <system_error>
#include <utility>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#endif

namespace simplesolid2::persistence {
namespace {

SaveGuardResult failure(
    SaveGuardErrorCode code,
    std::string message,
    std::filesystem::path path) {
    return SaveGuardResult{
        std::nullopt,
        SaveGuardDiagnostic{
            code,
            std::move(message),
            std::move(path),
        },
    };
}

std::string pathDigest(
    const std::filesystem::path& target) {
    std::error_code ec;
    auto canonical =
        std::filesystem::weakly_canonical(
            target,
            ec);
    if (ec) {
        ec.clear();
        canonical =
            std::filesystem::absolute(
                target,
                ec);
        if (ec) {
            canonical = target.lexically_normal();
        }
    }

#if defined(_WIN32)
    auto text = canonical.wstring();
    std::transform(
        text.begin(),
        text.end(),
        text.begin(),
        [](wchar_t ch) {
            return static_cast<wchar_t>(
                std::towlower(ch));
        });
    const std::string bytes{
        reinterpret_cast<const char*>(
            text.data()),
        text.size() * sizeof(wchar_t)};
#else
    const auto text = canonical.generic_string();
    const std::string bytes{text};
#endif
    return sha256Hex(sha256(bytes));
}

} // namespace

CooperativeSaveGuard::CooperativeSaveGuard(
    CooperativeSaveGuard&& other) noexcept
    : native_handle_{
          std::exchange(
              other.native_handle_,
              -1)} {}

CooperativeSaveGuard&
CooperativeSaveGuard::operator=(
    CooperativeSaveGuard&& other) noexcept {
    if (this == &other) return *this;
    release();
    native_handle_ =
        std::exchange(
            other.native_handle_,
            -1);
    return *this;
}

CooperativeSaveGuard::~CooperativeSaveGuard() {
    release();
}

void CooperativeSaveGuard::release() noexcept {
    if (native_handle_ < 0) return;
#if defined(_WIN32)
    auto handle =
        reinterpret_cast<HANDLE>(
            native_handle_);
    static_cast<void>(::ReleaseMutex(handle));
    ::CloseHandle(handle);
#else
    const int fd =
        static_cast<int>(native_handle_);
    static_cast<void>(::flock(fd, LOCK_UN));
    ::close(fd);
#endif
    native_handle_ = -1;
}

SaveGuardResult acquireCooperativeSaveGuard(
    const std::filesystem::path& target) {
    const auto digest = pathDigest(target);

#if defined(_WIN32)
    std::wstring name =
        L"Local\\SimpleSolid2.Save.";
    name.append(
        digest.begin(),
        digest.end());

    HANDLE handle =
        ::CreateMutexW(
            nullptr,
            FALSE,
            name.c_str());
    if (handle == nullptr) {
        return failure(
            SaveGuardErrorCode::io_failure,
            "Unable to create cooperative Save guard",
            target);
    }

    const auto wait =
        ::WaitForSingleObject(handle, 0U);
    if (wait == WAIT_OBJECT_0 ||
        wait == WAIT_ABANDONED) {
        return SaveGuardResult{
            CooperativeSaveGuard{
                reinterpret_cast<std::intptr_t>(
                    handle)},
            SaveGuardDiagnostic{},
        };
    }

    ::CloseHandle(handle);
    if (wait == WAIT_TIMEOUT) {
        return failure(
            SaveGuardErrorCode::busy,
            "Another SS2 Save owns the target guard",
            target);
    }
    return failure(
        SaveGuardErrorCode::io_failure,
        "Unable to acquire cooperative Save guard",
        target);
#else
    auto lock_path =
        std::filesystem::temp_directory_path();
    lock_path /=
        ".simplesolid2-save-" +
        digest + ".lock";

    const int fd =
        ::open(
            lock_path.c_str(),
            O_CREAT | O_RDWR,
            0600);
    if (fd < 0) {
        return failure(
            SaveGuardErrorCode::io_failure,
            "Unable to create cooperative Save guard",
            target);
    }
    if (::flock(fd, LOCK_EX | LOCK_NB) != 0) {
        ::close(fd);
        return failure(
            errno == EWOULDBLOCK
                ? SaveGuardErrorCode::busy
                : SaveGuardErrorCode::io_failure,
            errno == EWOULDBLOCK
                ? "Another SS2 Save owns the target guard"
                : "Unable to acquire cooperative Save guard",
            target);
    }
    return SaveGuardResult{
        CooperativeSaveGuard{
            static_cast<std::intptr_t>(fd)},
        SaveGuardDiagnostic{},
    };
#endif
}

} // namespace simplesolid2::persistence
