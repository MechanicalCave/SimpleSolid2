#include <simplesolid2/persistence/file_snapshot.hpp>

#include <algorithm>
#include <cerrno>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <limits>
#include <sstream>
#include <system_error>
#include <vector>

#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace simplesolid2::persistence {
namespace {

constexpr std::array<std::uint32_t, 64> k{
    0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U,
    0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
    0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U,
    0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
    0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU,
    0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
    0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U,
    0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
    0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U,
    0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
    0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U,
    0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
    0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U,
    0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
    0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U,
    0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U,
};

FileSnapshotResult failure(
    FileSnapshotErrorCode code,
    std::string message,
    std::filesystem::path path) {
    return FileSnapshotResult{
        std::nullopt,
        FileSnapshotDiagnostic{
            code,
            std::move(message),
            std::move(path),
        },
    };
}

#if defined(_WIN32)
class Handle final {
public:
    explicit Handle(HANDLE value) noexcept
        : value_{value} {}
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    ~Handle() {
        if (value_ != INVALID_HANDLE_VALUE) {
            ::CloseHandle(value_);
        }
    }
    [[nodiscard]] HANDLE get() const noexcept {
        return value_;
    }

private:
    HANDLE value_{INVALID_HANDLE_VALUE};
};

std::optional<FileIdentity> fileIdentity(
    HANDLE handle) {
    FILE_ID_INFO info{};
    if (!::GetFileInformationByHandleEx(
            handle,
            FileIdInfo,
            &info,
            sizeof(info))) {
        return std::nullopt;
    }
    FileIdentity result;
    result.volume =
        static_cast<std::uint64_t>(
            info.VolumeSerialNumber);
    std::memcpy(
        result.object.data(),
        info.FileId.Identifier,
        result.object.size());
    return result;
}
#endif

} // namespace

Sha256Digest sha256(
    std::string_view bytes) noexcept {
    std::array<std::uint32_t, 8> h{
        0x6a09e667U, 0xbb67ae85U,
        0x3c6ef372U, 0xa54ff53aU,
        0x510e527fU, 0x9b05688cU,
        0x1f83d9abU, 0x5be0cd19U,
    };

    std::vector<std::uint8_t> data;
    data.reserve(bytes.size() + 72U);
    for (const unsigned char ch : bytes) {
        data.push_back(ch);
    }
    const auto bit_length =
        static_cast<std::uint64_t>(
            bytes.size()) * 8U;
    data.push_back(0x80U);
    while ((data.size() % 64U) != 56U) {
        data.push_back(0U);
    }
    for (int shift = 56; shift >= 0; shift -= 8) {
        data.push_back(
            static_cast<std::uint8_t>(
                bit_length >> shift));
    }

    for (std::size_t offset = 0U;
         offset < data.size();
         offset += 64U) {
        std::array<std::uint32_t, 64> w{};
        for (std::size_t i = 0U; i < 16U; ++i) {
            const auto p = offset + i * 4U;
            w[i] =
                (static_cast<std::uint32_t>(data[p]) << 24U) |
                (static_cast<std::uint32_t>(data[p + 1U]) << 16U) |
                (static_cast<std::uint32_t>(data[p + 2U]) << 8U) |
                static_cast<std::uint32_t>(data[p + 3U]);
        }
        for (std::size_t i = 16U; i < 64U; ++i) {
            const auto s0 =
                std::rotr(w[i - 15U], 7) ^
                std::rotr(w[i - 15U], 18) ^
                (w[i - 15U] >> 3U);
            const auto s1 =
                std::rotr(w[i - 2U], 17) ^
                std::rotr(w[i - 2U], 19) ^
                (w[i - 2U] >> 10U);
            w[i] =
                w[i - 16U] + s0 +
                w[i - 7U] + s1;
        }

        auto a = h[0];
        auto b = h[1];
        auto c = h[2];
        auto d = h[3];
        auto e = h[4];
        auto f = h[5];
        auto g = h[6];
        auto hh = h[7];

        for (std::size_t i = 0U; i < 64U; ++i) {
            const auto s1 =
                std::rotr(e, 6) ^
                std::rotr(e, 11) ^
                std::rotr(e, 25);
            const auto ch =
                (e & f) ^ ((~e) & g);
            const auto temp1 =
                hh + s1 + ch + k[i] + w[i];
            const auto s0 =
                std::rotr(a, 2) ^
                std::rotr(a, 13) ^
                std::rotr(a, 22);
            const auto maj =
                (a & b) ^ (a & c) ^ (b & c);
            const auto temp2 = s0 + maj;

            hh = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }

        h[0] += a;
        h[1] += b;
        h[2] += c;
        h[3] += d;
        h[4] += e;
        h[5] += f;
        h[6] += g;
        h[7] += hh;
    }

    Sha256Digest digest{};
    for (std::size_t i = 0U; i < h.size(); ++i) {
        digest[i * 4U] =
            static_cast<std::uint8_t>(h[i] >> 24U);
        digest[i * 4U + 1U] =
            static_cast<std::uint8_t>(h[i] >> 16U);
        digest[i * 4U + 2U] =
            static_cast<std::uint8_t>(h[i] >> 8U);
        digest[i * 4U + 3U] =
            static_cast<std::uint8_t>(h[i]);
    }
    return digest;
}

std::string sha256Hex(
    const Sha256Digest& digest) {
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (const auto byte : digest) {
        out << std::setw(2)
            << static_cast<unsigned>(byte);
    }
    return out.str();
}

FileSnapshotResult readFileSnapshot(
    const std::filesystem::path& path,
    std::uintmax_t maximum_bytes) {
#if defined(_WIN32)
    Handle handle{
        ::CreateFileW(
            path.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr)};
    if (handle.get() == INVALID_HANDLE_VALUE) {
        const auto error = ::GetLastError();
        return failure(
            error == ERROR_FILE_NOT_FOUND ||
                    error == ERROR_PATH_NOT_FOUND
                ? FileSnapshotErrorCode::not_found
                : FileSnapshotErrorCode::io_failure,
            error == ERROR_FILE_NOT_FOUND ||
                    error == ERROR_PATH_NOT_FOUND
                ? "File does not exist"
                : "Unable to open file snapshot",
            path);
    }

    FILE_STANDARD_INFO info{};
    if (!::GetFileInformationByHandleEx(
            handle.get(),
            FileStandardInfo,
            &info,
            sizeof(info))) {
        return failure(
            FileSnapshotErrorCode::io_failure,
            "Unable to inspect file snapshot",
            path);
    }
    if (info.Directory) {
        return failure(
            FileSnapshotErrorCode::not_regular,
            "Path is not a regular file",
            path);
    }
    if (info.EndOfFile.QuadPart < 0 ||
        static_cast<std::uint64_t>(
            info.EndOfFile.QuadPart) >
            maximum_bytes) {
        return failure(
            FileSnapshotErrorCode::too_large,
            "File exceeds supported snapshot size",
            path);
    }

    std::string bytes(
        static_cast<std::size_t>(
            info.EndOfFile.QuadPart),
        '\0');
    std::size_t offset = 0U;
    while (offset < bytes.size()) {
        const auto remaining =
            bytes.size() - offset;
        const DWORD request =
            static_cast<DWORD>(
                std::min<std::size_t>(
                    remaining,
                    1U << 20U));
        DWORD read = 0U;
        if (!::ReadFile(
                handle.get(),
                bytes.data() + offset,
                request,
                &read,
                nullptr) ||
            read == 0U) {
            return failure(
                FileSnapshotErrorCode::io_failure,
                "Unable to read file snapshot",
                path);
        }
        offset += read;
    }

    const auto identity =
        fileIdentity(handle.get());
    if (!identity) {
        return failure(
            FileSnapshotErrorCode::io_failure,
            "Unable to capture file identity",
            path);
    }

    return FileSnapshotResult{
        FileSnapshot{
            bytes,
            *identity,
            sha256(bytes)},
        FileSnapshotDiagnostic{},
    };
#else
    const int fd =
        ::open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        return failure(
            errno == ENOENT
                ? FileSnapshotErrorCode::not_found
                : FileSnapshotErrorCode::io_failure,
            errno == ENOENT
                ? "File does not exist"
                : "Unable to open file snapshot",
            path);
    }

    struct CloseFd final {
        int fd{-1};
        ~CloseFd() {
            if (fd >= 0) ::close(fd);
        }
    } close{fd};

    struct stat info {};
    if (::fstat(fd, &info) != 0) {
        return failure(
            FileSnapshotErrorCode::io_failure,
            "Unable to inspect file snapshot",
            path);
    }
    if (!S_ISREG(info.st_mode)) {
        return failure(
            FileSnapshotErrorCode::not_regular,
            "Path is not a regular file",
            path);
    }
    if (info.st_size < 0 ||
        static_cast<std::uintmax_t>(
            info.st_size) > maximum_bytes) {
        return failure(
            FileSnapshotErrorCode::too_large,
            "File exceeds supported snapshot size",
            path);
    }

    std::string bytes(
        static_cast<std::size_t>(info.st_size),
        '\0');
    std::size_t offset = 0U;
    while (offset < bytes.size()) {
        const auto read =
            ::read(
                fd,
                bytes.data() + offset,
                bytes.size() - offset);
        if (read <= 0) {
            return failure(
                FileSnapshotErrorCode::io_failure,
                "Unable to read file snapshot",
                path);
        }
        offset += static_cast<std::size_t>(read);
    }

    FileIdentity identity;
    identity.volume =
        static_cast<std::uint64_t>(info.st_dev);
    const auto inode =
        static_cast<std::uint64_t>(info.st_ino);
    for (std::size_t i = 0U; i < 8U; ++i) {
        identity.object[i] =
            static_cast<std::uint8_t>(
                inode >> (i * 8U));
    }

    return FileSnapshotResult{
        FileSnapshot{
            bytes,
            identity,
            sha256(bytes)},
        FileSnapshotDiagnostic{},
    };
#endif
}

} // namespace simplesolid2::persistence
