#include <simplesolid2/application/project_session.hpp>
#include <simplesolid2/application/project_workspace_metadata.hpp>
#include <simplesolid2/kernel/evidence.hpp>

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <utility>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-00A E09 CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

struct TempDirectory final {
    std::filesystem::path path;

    TempDirectory() {
        path =
            std::filesystem::temp_directory_path() /
            ("simplesolid2_pm00a_e09_" +
             std::to_string(
                 std::filesystem::file_time_type::clock::now()
                     .time_since_epoch()
                     .count()));
        std::filesystem::create_directories(path);
    }

    ~TempDirectory() {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};

// Evidence-only PM-00A prototype. This is deliberately local to the
// regression: PM-00B still owns the production publication contract.
struct PublicationTicket final {
    core::DocumentId document_id;
    core::DocumentRevision revision;
    std::uint64_t session_generation{};
    std::uint64_t request_generation{};
};

enum class RequestState {
    active,
    cancelled,
    failed,
};

enum class PublicationOutcome {
    published,
    stale_revision,
    stale_session,
    stale_generation,
    cancelled,
    failed,
};

class EvidencePublicationGate final {
public:
    [[nodiscard]] bool bindCanonical(
        const application::ProjectSession& project,
        const core::DocumentId& document_id) {
        const auto* session =
            project.documentSession(document_id);
        if (session == nullptr ||
            session->documentId() != document_id) {
            return false;
        }

        document_id_ = document_id;
        current_session_ = session;
        ++session_generation_;
        request_generation_ = 0U;
        request_states_.clear();
        published_.reset();
        published_request_generation_.reset();
        return true;
    }

    [[nodiscard]] std::optional<PublicationTicket>
    begin(
        const application::ProjectSession& project,
        const core::DocumentId& document_id) {
        const auto* session =
            project.documentSession(document_id);
        if (!document_id_.has_value() ||
            *document_id_ != document_id ||
            session == nullptr ||
            session != current_session_) {
            return std::nullopt;
        }

        ++request_generation_;
        request_states_[request_generation_] =
            RequestState::active;

        return PublicationTicket{
            document_id,
            session->document().revision(),
            session_generation_,
            request_generation_,
        };
    }

    [[nodiscard]] bool cancel(
        const PublicationTicket& ticket) {
        return markTerminal(
            ticket,
            RequestState::cancelled);
    }

    [[nodiscard]] bool fail(
        const PublicationTicket& ticket) {
        return markTerminal(
            ticket,
            RequestState::failed);
    }

    [[nodiscard]] PublicationOutcome publish(
        const application::ProjectSession& project,
        const PublicationTicket& ticket,
        kernel::ShapeEvidence payload) {
        const auto* session =
            project.documentSession(
                ticket.document_id);

        if (!document_id_.has_value() ||
            *document_id_ != ticket.document_id ||
            session == nullptr ||
            session != current_session_ ||
            ticket.session_generation !=
                session_generation_) {
            return PublicationOutcome::stale_session;
        }

        if (ticket.revision !=
            session->document().revision()) {
            return PublicationOutcome::stale_revision;
        }

        const auto found =
            request_states_.find(
                ticket.request_generation);
        if (found == request_states_.end()) {
            return PublicationOutcome::stale_generation;
        }
        if (found->second ==
            RequestState::cancelled) {
            return PublicationOutcome::cancelled;
        }
        if (found->second ==
            RequestState::failed) {
            return PublicationOutcome::failed;
        }

        if (ticket.request_generation !=
            request_generation_) {
            return PublicationOutcome::stale_generation;
        }

        published_ = std::move(payload);
        published_request_generation_ =
            ticket.request_generation;
        return PublicationOutcome::published;
    }

    [[nodiscard]] const std::optional<
        kernel::ShapeEvidence>&
    published() const noexcept {
        return published_;
    }

    [[nodiscard]] std::optional<std::uint64_t>
    publishedRequestGeneration() const noexcept {
        return published_request_generation_;
    }

private:
    [[nodiscard]] bool markTerminal(
        const PublicationTicket& ticket,
        RequestState state) {
        if (ticket.session_generation !=
            session_generation_) {
            return false;
        }

        const auto found =
            request_states_.find(
                ticket.request_generation);
        if (found == request_states_.end() ||
            found->second != RequestState::active) {
            return false;
        }

        found->second = state;
        return true;
    }

    std::optional<core::DocumentId> document_id_;
    const application::DocumentSession*
        current_session_{};
    std::uint64_t session_generation_{};
    std::uint64_t request_generation_{};
    std::map<std::uint64_t, RequestState>
        request_states_;
    std::optional<kernel::ShapeEvidence> published_;
    std::optional<std::uint64_t>
        published_request_generation_;
};

[[nodiscard]] kernel::ShapeEvidence
shapeEvidence(std::size_t face_count) {
    kernel::ShapeEvidence result;
    result.status = kernel::EvidenceStatus::ok;
    result.brep_valid = true;
    result.solid_count = 1U;
    result.face_count = face_count;
    result.wire_count = 1U;
    result.edge_count = 12U;
    return result;
}

} // namespace

int main() {
    TempDirectory temp;

    application::ProjectWorkspaceMetadataService
        metadata_service;
    const auto initialized =
        metadata_service.initialize(
            temp.path,
            "PM-00A E09");
    CHECK(initialized.ok());

    auto opened =
        application::ProjectSession::open(
            temp.path);
    CHECK(opened.ok());

    auto& project = *opened.session;
    const auto created =
        project.createPart("E09.ss2part");
    CHECK(created.ok());
    CHECK(created.session != nullptr);
    CHECK(!created.reused_session);

    auto* session_a = created.session;
    const auto document_id =
        session_a->documentId();

    EvidencePublicationGate gate;
    CHECK(
        gate.bindCanonical(
            project,
            document_id));

    // E09-01: a result started at revision R has no publication authority
    // after an authored mutation advances the canonical document to R+1.
    const auto revision_ticket =
        gate.begin(
            project,
            document_id);
    CHECK(revision_ticket.has_value());

    auto properties =
        session_a->document().properties();
    properties.title =
        "Revision advanced while evidence was running";
    const auto advanced =
        session_a->execute(
            application::SetDocumentPropertiesCommand{
                properties});
    CHECK(advanced.ok());
    CHECK(advanced.changed);
    CHECK(
        session_a->document().revision() !=
        revision_ticket->revision);

    CHECK(
        gate.publish(
            project,
            *revision_ticket,
            shapeEvidence(6U)) ==
        PublicationOutcome::stale_revision);
    CHECK(!gate.published().has_value());

    const auto current_after_revision =
        gate.begin(
            project,
            document_id);
    CHECK(current_after_revision.has_value());
    CHECK(
        gate.publish(
            project,
            *current_after_revision,
            shapeEvidence(6U)) ==
        PublicationOutcome::published);
    CHECK(gate.published().has_value());

    CHECK(session_a->save().ok());
    CHECK(!session_a->needsSave());

    // E09-02: Session A can be replaced by canonical Session B for the same
    // durable DocumentId. Results owned by A cannot publish into B.
    const auto session_a_ticket =
        gate.begin(
            project,
            document_id);
    CHECK(session_a_ticket.has_value());

    CHECK(
        project.closeDocument(
            document_id,
            false));
    CHECK(
        project.documentSession(
            document_id) == nullptr);

    const auto reopened =
        project.openDocument(
            document_id);
    CHECK(reopened.ok());
    CHECK(!reopened.reused_session);
    CHECK(reopened.session != nullptr);

    auto* session_b = reopened.session;
    CHECK(
        session_b->documentId() ==
        document_id);
    CHECK(
        gate.bindCanonical(
            project,
            document_id));

    CHECK(
        gate.publish(
            project,
            *session_a_ticket,
            shapeEvidence(6U)) ==
        PublicationOutcome::stale_session);
    CHECK(!gate.published().has_value());

    const auto session_b_ticket =
        gate.begin(
            project,
            document_id);
    CHECK(session_b_ticket.has_value());
    CHECK(
        gate.publish(
            project,
            *session_b_ticket,
            shapeEvidence(6U)) ==
        PublicationOutcome::published);

    // E09-03: geometric equality does not grant authority. Two requests at
    // the same revision produce byte-for-byte equal neutral provider evidence,
    // but only the newest request generation may publish.
    const auto older_identical =
        gate.begin(
            project,
            document_id);
    CHECK(older_identical.has_value());
    const auto newer_identical =
        gate.begin(
            project,
            document_id);
    CHECK(newer_identical.has_value());
    CHECK(
        older_identical->revision ==
        newer_identical->revision);

    const auto identical_payload =
        shapeEvidence(8U);
    CHECK(
        gate.publish(
            project,
            *newer_identical,
            identical_payload) ==
        PublicationOutcome::published);
    CHECK(
        gate.published().has_value() &&
        *gate.published() ==
            identical_payload);
    CHECK(
        gate.publishedRequestGeneration() ==
        std::optional<std::uint64_t>{
            newer_identical->
                request_generation});

    CHECK(
        gate.publish(
            project,
            *older_identical,
            identical_payload) ==
        PublicationOutcome::stale_generation);
    CHECK(
        gate.publishedRequestGeneration() ==
        std::optional<std::uint64_t>{
            newer_identical->
                request_generation});

    // E09-04: cancelled and failed requests remain terminal. Even if their
    // completion arrives after a newer request has published, they cannot
    // replace current evidence.
    const auto cancelled =
        gate.begin(
            project,
            document_id);
    CHECK(cancelled.has_value());
    CHECK(gate.cancel(*cancelled));

    const auto after_cancel =
        gate.begin(
            project,
            document_id);
    CHECK(after_cancel.has_value());
    const auto after_cancel_payload =
        shapeEvidence(9U);
    CHECK(
        gate.publish(
            project,
            *after_cancel,
            after_cancel_payload) ==
        PublicationOutcome::published);

    CHECK(
        gate.publish(
            project,
            *cancelled,
            shapeEvidence(99U)) ==
        PublicationOutcome::cancelled);
    CHECK(
        gate.published().has_value() &&
        *gate.published() ==
            after_cancel_payload);
    CHECK(
        gate.publishedRequestGeneration() ==
        std::optional<std::uint64_t>{
            after_cancel->
                request_generation});

    const auto failed =
        gate.begin(
            project,
            document_id);
    CHECK(failed.has_value());
    CHECK(gate.fail(*failed));

    const auto after_failure =
        gate.begin(
            project,
            document_id);
    CHECK(after_failure.has_value());
    const auto after_failure_payload =
        shapeEvidence(10U);
    CHECK(
        gate.publish(
            project,
            *after_failure,
            after_failure_payload) ==
        PublicationOutcome::published);

    CHECK(
        gate.publish(
            project,
            *failed,
            shapeEvidence(100U)) ==
        PublicationOutcome::failed);
    CHECK(
        gate.published().has_value() &&
        *gate.published() ==
            after_failure_payload);
    CHECK(
        gate.publishedRequestGeneration() ==
        std::optional<std::uint64_t>{
            after_failure->
                request_generation});

    std::cout
        << "PM00A_E09_PASS rows=E09-01..E09-04"
        << " stale_publications=0\n";
    return EXIT_SUCCESS;
}
