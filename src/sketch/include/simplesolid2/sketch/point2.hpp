#pragma once

#include <cmath>

namespace simplesolid2::sketch {

struct Point2 final {
    double u{};
    double v{};

    [[nodiscard]] bool finite() const noexcept {
        return std::isfinite(u) && std::isfinite(v);
    }

    friend bool operator==(const Point2&, const Point2&) = default;
};

} // namespace simplesolid2::sketch
