#include <simplesolid2/part/part_document.hpp>

#include <cstdlib>
#include <iostream>
#include <limits>

using namespace simplesolid2;

namespace {
void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr << "PART-01 transaction CHECK failed at line "
                  << line << ": " << expression << '\n';
        std::abort();
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)
}

int main() {
    auto document = part::PartDocument::create(core::DocumentId::generate());
    CHECK(document.revision().value() == 0U);
    CHECK(document.properties().title.empty());
    CHECK(document.builtinReferenceVisible(core::BuiltinReferenceRole::origin_point));
    CHECK(document.builtinReferenceVisible(core::BuiltinReferenceRole::x_axis));
    CHECK(!document.builtinReferenceVisible(core::BuiltinReferenceRole::xy_plane));

    {
        part::PartDocumentTransaction transaction{document};
        auto properties = document.properties();
        properties.title = "Drive Shaft";
        transaction.setProperties(properties);

        CHECK(document.properties().title.empty());
        CHECK(transaction.stagedState().properties.title == "Drive Shaft");

        const auto committed = transaction.commit();
        CHECK(committed.ok());
        CHECK(committed.changed);
    }

    CHECK(document.properties().title == "Drive Shaft");
    CHECK(document.revision().value() == 1U);

    {
        part::PartDocumentTransaction transaction{document};
        transaction.setProperties(document.properties());
        const auto committed = transaction.commit();
        CHECK(committed.ok());
        CHECK(!committed.changed);
    }
    CHECK(document.revision().value() == 1U);

    {
        part::PartDocumentTransaction transaction{document};
        auto properties = document.properties();
        properties.number = "12-04-117";
        transaction.setProperties(properties);
        transaction.rollback();
    }
    CHECK(document.properties().number.empty());
    CHECK(document.revision().value() == 1U);

    {
        part::PartDocumentTransaction transaction{document};
        CHECK(transaction.setBuiltinReferenceVisible(
            core::BuiltinReferenceRole::xy_plane,
            true));
        CHECK(!document.builtinReferenceVisible(
            core::BuiltinReferenceRole::xy_plane));
        CHECK(transaction.stagedState().presentation.builtin_references.visible(
            core::BuiltinReferenceRole::xy_plane));

        const auto committed = transaction.commit();
        CHECK(committed.ok());
        CHECK(committed.changed);
    }

    CHECK(document.builtinReferenceVisible(core::BuiltinReferenceRole::xy_plane));
    CHECK(document.revision().value() == 2U);

    {
        part::PartDocumentTransaction transaction{document};
        CHECK(!transaction.setBuiltinReferenceVisible(
            core::BuiltinReferenceRole::xy_plane,
            true));
        const auto committed = transaction.commit();
        CHECK(committed.ok());
        CHECK(!committed.changed);
    }
    CHECK(document.revision().value() == 2U);

    auto exhausted = part::PartDocument::restore(
        core::DocumentId::generate(),
        part::PartAuthoredState{},
        core::DocumentRevision{std::numeric_limits<std::uint64_t>::max()});
    part::PartDocumentTransaction transaction{exhausted};
    auto properties = exhausted.properties();
    properties.title = "Must not commit";
    transaction.setProperties(properties);
    const auto rejected = transaction.commit();
    CHECK(!rejected.ok());
    CHECK(rejected.code == part::PartCommitErrorCode::revision_exhausted);
    CHECK(exhausted.properties().title.empty());
    CHECK(exhausted.revision().value() ==
          std::numeric_limits<std::uint64_t>::max());

    return EXIT_SUCCESS;
}
