#include <simplesolid2/core/document_reference.hpp>

#include <cstdlib>
#include <iostream>

using namespace simplesolid2::core;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr << "WB-01 built-in reference CHECK failed at line "
                  << line << ": " << expression << '\n';
        std::abort();
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

} // namespace

int main() {
    CHECK(builtin_reference_roles.size() == 7U);

    BuiltinReferenceVisibility visibility;
    CHECK(visibility.mask() == BuiltinReferenceVisibility::default_mask);
    CHECK(visibility.visible(BuiltinReferenceRole::origin_point));
    CHECK(visibility.visible(BuiltinReferenceRole::x_axis));
    CHECK(visibility.visible(BuiltinReferenceRole::y_axis));
    CHECK(visibility.visible(BuiltinReferenceRole::z_axis));
    CHECK(!visibility.visible(BuiltinReferenceRole::xy_plane));
    CHECK(!visibility.visible(BuiltinReferenceRole::xz_plane));
    CHECK(!visibility.visible(BuiltinReferenceRole::yz_plane));

    CHECK(visibility.setVisible(BuiltinReferenceRole::xy_plane, true));
    CHECK(visibility.visible(BuiltinReferenceRole::xy_plane));
    CHECK(!visibility.setVisible(BuiltinReferenceRole::xy_plane, true));

    const auto restored =
        BuiltinReferenceVisibility::fromMask(visibility.mask());
    CHECK(restored.has_value());
    CHECK(*restored == visibility);

    CHECK(!BuiltinReferenceVisibility::fromMask(0x80U).has_value());
    CHECK(!isBuiltinReferenceRole(
        static_cast<BuiltinReferenceRole>(255U)));

    return EXIT_SUCCESS;
}
