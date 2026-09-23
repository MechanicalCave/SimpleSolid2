#include <simplesolid2/core/document.hpp>

#include <cstdlib>
#include <iostream>
#include <string>
#include <type_traits>

using namespace simplesolid2::core;

namespace {
void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr << "PART-01 document primitives CHECK failed at line "
                  << line << ": " << expression << '\n';
        std::abort();
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)
}

int main() {
    static_assert(!std::is_same_v<DocumentId, std::string>);

    const auto first = DocumentId::generate();
    const auto second = DocumentId::generate();
    CHECK(first != second);
    CHECK(DocumentId::parse(first.value()).has_value());
    CHECK(!DocumentId::parse("not-a-document-id").has_value());

    DocumentRevision revision;
    CHECK(revision.value() == 0U);
    const auto next = revision.next();
    CHECK(next.has_value());
    CHECK(next->value() == 1U);

    DocumentProperties a;
    DocumentProperties b;
    CHECK(a == b);
    b.title = "Drive Shaft";
    CHECK(a != b);
    return EXIT_SUCCESS;
}
