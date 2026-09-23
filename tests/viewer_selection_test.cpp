#include <simplesolid2/viewer/selection.hpp>

#include <cstdlib>
#include <iostream>

using namespace simplesolid2::viewer;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr << "WB-01 Viewer selection CHECK failed at line "
                  << line << ": " << expression << '\n';
        std::abort();
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

} // namespace

int main() {
    const PresentationToken first{1U};
    const PresentationToken second{2U};

    PresentationSelection selection;
    CHECK(selection.valid());

    selection.selected = {first, second};
    selection.primary = second;
    CHECK(selection.valid());

    selection.selected.push_back(first);
    CHECK(!selection.valid());

    selection.selected = {first};
    selection.primary = second;
    CHECK(!selection.valid());

    SelectionIntent intent{
        first,
        SelectionIntentMode::replace};
    CHECK(intent.valid());

    intent.token = {};
    CHECK(!intent.valid());

    return EXIT_SUCCESS;
}
