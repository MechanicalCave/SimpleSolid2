#include <simplesolid2/part/part_document.hpp>

#include <cstdlib>
#include <iostream>
#include <limits>
#include <utility>

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

part::PartSketch validHostedSketch(
    sketch::SketchId id) {
    const auto support =
        part::partSketchSupportForBuiltinPlane(
            core::BuiltinReferenceRole::xy_plane);
    CHECK(support.has_value());
    const auto placement =
        part::sketchPlacementForSupport(*support);
    CHECK(placement.has_value());

    return part::PartSketch{
        std::move(id),
        *support,
        *placement,
        true,
        sketch::SketchModel{}};
}

void checkInactive(
    part::PartDocumentTransaction& transaction) {
    const auto repeated = transaction.commit();
    CHECK(!repeated.ok());
    CHECK(
        repeated.code ==
        part::PartCommitErrorCode::inactive_transaction);
    CHECK(!repeated.changed);
}
}

int main() {
    auto document =
        part::PartDocument::create(
            core::DocumentId::generate());
    CHECK(document.revision().value() == 0U);
    CHECK(document.properties().title.empty());
    CHECK(document.builtinReferenceVisible(
        core::BuiltinReferenceRole::origin_point));
    CHECK(document.builtinReferenceVisible(
        core::BuiltinReferenceRole::x_axis));
    CHECK(!document.builtinReferenceVisible(
        core::BuiltinReferenceRole::xy_plane));

    // Fresh changed commit increments exactly once and is terminal.
    {
        part::PartDocumentTransaction transaction{
            document};
        auto properties = document.properties();
        properties.title = "Drive Shaft";
        transaction.setProperties(properties);

        CHECK(document.properties().title.empty());
        CHECK(
            transaction.stagedState()
                .properties.title ==
            "Drive Shaft");

        const auto committed =
            transaction.commit();
        CHECK(committed.ok());
        CHECK(committed.changed);
        checkInactive(transaction);
    }

    CHECK(
        document.properties().title ==
        "Drive Shaft");
    CHECK(document.revision().value() == 1U);

    // Fresh no-op is successful, does not increment, and is terminal.
    {
        part::PartDocumentTransaction transaction{
            document};
        transaction.setProperties(
            document.properties());
        const auto committed =
            transaction.commit();
        CHECK(committed.ok());
        CHECK(!committed.changed);
        CHECK(document.revision().value() == 1U);
        checkInactive(transaction);
    }

    // Rollback is terminal/idempotent and leaves authored state unchanged.
    {
        part::PartDocumentTransaction transaction{
            document};
        auto properties = document.properties();
        properties.number = "12-04-117";
        transaction.setProperties(properties);
        transaction.rollback();
        transaction.rollback();
        checkInactive(transaction);
    }
    CHECK(document.properties().number.empty());
    CHECK(document.revision().value() == 1U);

    // Existing visibility mutation semantics remain unchanged.
    {
        part::PartDocumentTransaction transaction{
            document};
        CHECK(
            transaction.setBuiltinReferenceVisible(
                core::BuiltinReferenceRole::xy_plane,
                true));
        CHECK(!document.builtinReferenceVisible(
            core::BuiltinReferenceRole::xy_plane));

        const auto committed =
            transaction.commit();
        CHECK(committed.ok());
        CHECK(committed.changed);
    }

    CHECK(document.builtinReferenceVisible(
        core::BuiltinReferenceRole::xy_plane));
    CHECK(document.revision().value() == 2U);

    // B2: overlapping transactions are bound to their base revision.
    {
        part::PartDocumentTransaction first{
            document};
        part::PartDocumentTransaction stale{
            document};

        auto first_properties =
            document.properties();
        first_properties.description =
            "Accepted by T1";
        first.setProperties(first_properties);

        auto stale_properties =
            document.properties();
        stale_properties.number =
            "STALE-T2";
        stale.setProperties(stale_properties);

        const auto before =
            document.revision();
        const auto first_result =
            first.commit();
        CHECK(first_result.ok());
        CHECK(first_result.changed);
        CHECK(
            document.revision().value() ==
            before.value() + 1U);
        CHECK(
            document.properties().description ==
            "Accepted by T1");

        const auto stale_result =
            stale.commit();
        CHECK(!stale_result.ok());
        CHECK(
            stale_result.code ==
            part::PartCommitErrorCode::
                stale_transaction);
        CHECK(!stale_result.changed);
        CHECK(
            document.properties().description ==
            "Accepted by T1");
        CHECK(
            document.properties().number.empty());
        CHECK(
            document.revision().value() ==
            before.value() + 1U);
        checkInactive(stale);
    }

    // Freshness precedes no-op equivalence.
    {
        part::PartDocumentTransaction stale{
            document};
        part::PartDocumentTransaction winner{
            document};

        auto properties =
            document.properties();
        properties.description =
            "Winner revision";
        winner.setProperties(properties);
        CHECK(winner.commit().changed);

        stale.replaceState(document.state());
        const auto revision =
            document.revision();
        const auto rejected =
            stale.commit();
        CHECK(!rejected.ok());
        CHECK(
            rejected.code ==
            part::PartCommitErrorCode::
                stale_transaction);
        CHECK(document.revision() == revision);
        checkInactive(stale);
    }

    // Complete state validation rejects duplicate hosted Sketch identity.
    {
        const auto before_state =
            document.state();
        const auto before_revision =
            document.revision();

        auto invalid = before_state;
        const auto duplicate_id =
            sketch::SketchId::generate();
        invalid.sketches.push_back(
            validHostedSketch(duplicate_id));
        invalid.sketches.push_back(
            validHostedSketch(duplicate_id));

        part::PartDocumentTransaction transaction{
            document};
        transaction.replaceState(
            std::move(invalid));
        const auto rejected =
            transaction.commit();
        CHECK(!rejected.ok());
        CHECK(
            rejected.code ==
            part::PartCommitErrorCode::
                invalid_state);
        CHECK(!rejected.changed);
        CHECK(document.state() == before_state);
        CHECK(
            document.revision() ==
            before_revision);
        checkInactive(transaction);
    }

    // Valid reconstruction preserves the supplied technical revision.
    const auto max_revision =
        core::DocumentRevision{
            std::numeric_limits<std::uint64_t>::max()};
    auto reconstructed =
        part::PartDocument::restore(
            core::DocumentId::generate(),
            part::PartAuthoredState{},
            max_revision);
    CHECK(reconstructed.ok());
    CHECK(reconstructed.document.has_value());
    CHECK(
        reconstructed.document->revision() ==
        max_revision);

    auto exhausted =
        std::move(*reconstructed.document);

    // At max revision a fresh no-op still succeeds and is terminal.
    {
        part::PartDocumentTransaction no_op{
            exhausted};
        const auto result = no_op.commit();
        CHECK(result.ok());
        CHECK(!result.changed);
        CHECK(
            exhausted.revision() ==
            max_revision);
        checkInactive(no_op);
    }

    // A fresh changed commit at max revision is rejected and terminal.
    {
        part::PartDocumentTransaction transaction{
            exhausted};
        auto properties =
            exhausted.properties();
        properties.title =
            "Must not commit";
        transaction.setProperties(properties);
        const auto rejected =
            transaction.commit();
        CHECK(!rejected.ok());
        CHECK(
            rejected.code ==
            part::PartCommitErrorCode::
                revision_exhausted);
        CHECK(
            exhausted.properties()
                .title.empty());
        CHECK(
            exhausted.revision() ==
            max_revision);
        checkInactive(transaction);
    }

    // Reconstruction itself uses the same Part authored-state validator.
    {
        part::PartAuthoredState invalid;
        const auto duplicate_id =
            sketch::SketchId::generate();
        invalid.sketches.push_back(
            validHostedSketch(duplicate_id));
        invalid.sketches.push_back(
            validHostedSketch(duplicate_id));

        auto rejected =
            part::PartDocument::restore(
                core::DocumentId::generate(),
                std::move(invalid));
        CHECK(!rejected.ok());
        CHECK(!rejected.document.has_value());
        CHECK(
            rejected.code ==
            part::PartReconstructErrorCode::
                invalid_state);
    }

    return EXIT_SUCCESS;
}
