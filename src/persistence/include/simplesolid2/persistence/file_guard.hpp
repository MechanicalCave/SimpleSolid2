#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

namespace simplesolid2::persistence {

enum class SaveGuardErrorCode {
    none,
    busy,
    io_failure,
};

struct SaveGuardDiagnostic final {
    SaveGuardErrorCode code{
        SaveGuardErrorCode::none};
    std::string message;
    std::filesystem::path path;
};

class CooperativeSaveGuard final {
public:
    CooperativeSaveGuard() = default;
    CooperativeSaveGuard(
        const CooperativeSaveGuard&) = delete;
    CooperativeSaveGuard& operator=(
        const CooperativeSaveGuard&) = delete;
    CooperativeSaveGuard(
        CooperativeSaveGuard&& other) noexcept;
    CooperativeSaveGuard& operator=(
        CooperativeSaveGuard&& other) noexcept;
    ~CooperativeSaveGuard();

private:
    friend struct SaveGuardResult;
    friend SaveGuardResult
    acquireCooperativeSaveGuard(
        const std::filesystem::path&);

    explicit CooperativeSaveGuard(
        std::intptr_t native_handle) noexcept
        : native_handle_{native_handle} {}

    void release() noexcept;

    std::intptr_t native_handle_{-1};
};

struct SaveGuardResult final {
    std::optional<CooperativeSaveGuard> guard;
    SaveGuardDiagnostic diagnostic;

    [[nodiscard]] bool ok() const noexcept {
        return guard.has_value();
    }
};

[[nodiscard]] SaveGuardResult
acquireCooperativeSaveGuard(
    const std::filesystem::path& target);

} // namespace simplesolid2::persistence
