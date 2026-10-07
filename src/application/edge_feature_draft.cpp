#include <simplesolid2/application/edge_feature_draft.hpp>

#include <simplesolid2/application/document_session.hpp>

#include <algorithm>
#include <limits>
#include <utility>

namespace simplesolid2::application {
namespace {

[[nodiscard]] std::optional<part::BodyStageRef>
requiredStageForCreate(
    const DocumentSession& session) {
    const auto& features =
        session.document().body().features;
    if (features.empty()) {
        return std::nullopt;
    }
    return part::BodyStageRef{
        part::BodyStageKind::after_feature,
        features.back().id};
}

[[nodiscard]] bool canonicalizeEdges(
    std::vector<part::MaterialEdgeReference>&
        edges,
    const std::optional<part::BodyStageRef>&
        required_stage) {
    if (edges.empty()) {
        return true;
    }
    if (!required_stage) {
        return false;
    }

    for (const auto& edge : edges) {
        if (!edge.valid() ||
            edge.stage != *required_stage) {
            return false;
        }
    }

    std::sort(
        edges.begin(),
        edges.end());
    return std::adjacent_find(
               edges.begin(),
               edges.end()) ==
           edges.end();
}

[[nodiscard]] bool validPositiveLength(
    core::LengthValue value) noexcept {
    return value.finite() &&
           value.millimetres > 0.0;
}

} // namespace

bool CreateFilletFeatureCommand::valid()
    const noexcept {
    return part::filletFeatureStructurallyValid(
        part::FilletFeature{
            edges,
            radius});
}

bool CreateChamferFeatureCommand::valid()
    const noexcept {
    return part::chamferFeatureStructurallyValid(
        part::ChamferFeature{
            edges,
            distance});
}

FilletDraft::FilletDraft(
    core::DocumentId document_id,
    core::DocumentRevision source_revision,
    std::optional<part::BodyStageRef>
        required_stage)
    : document_id_{std::move(document_id)},
      source_revision_{source_revision},
      required_stage_{std::move(
          required_stage)} {}

FilletDraft
FilletDraft::beginCreate(
    const DocumentSession& session) {
    return FilletDraft{
        session.documentId(),
        session.document().revision(),
        requiredStageForCreate(session)};
}

std::optional<FilletDraft>
FilletDraft::beginCreate(
    const DocumentSession& session,
    std::vector<part::MaterialEdgeReference>
        edges) {
    auto draft =
        beginCreate(session);
    if (!draft.setEdges(
            std::move(edges))) {
        return std::nullopt;
    }
    return draft;
}

bool FilletDraft::valid() const noexcept {
    if (!radius_ ||
        !required_stage_ ||
        edges_.empty()) {
        return false;
    }
    return part::filletFeatureStructurallyValid(
               part::FilletFeature{
                   edges_,
                   *radius_}) &&
           edges_.front().stage ==
               *required_stage_;
}

bool FilletDraft::currentFor(
    const DocumentSession& session) const noexcept {
    return session.documentId() ==
               document_id_ &&
           session.document().revision() ==
               source_revision_;
}

bool FilletDraft::canAdvance() const noexcept {
    return generation_ !=
           std::numeric_limits<
               EdgeFeatureDraftGeneration>::max();
}

void FilletDraft::advance() noexcept {
    ++generation_;
}

bool FilletDraft::setEdges(
    std::vector<part::MaterialEdgeReference>
        edges) {
    if (!canonicalizeEdges(
            edges,
            required_stage_)) {
        return false;
    }
    if (edges_ == edges) {
        return true;
    }
    if (!canAdvance()) {
        return false;
    }
    edges_ = std::move(edges);
    advance();
    return true;
}

bool FilletDraft::setRadius(
    core::LengthValue radius) noexcept {
    if (!validPositiveLength(radius)) {
        return false;
    }
    if (radius_ &&
        *radius_ == radius) {
        return true;
    }
    if (!canAdvance()) {
        return false;
    }
    radius_ = radius;
    advance();
    return true;
}

bool FilletDraft::setName(
    std::string name) {
    if (name_ == name) {
        return true;
    }
    if (!canAdvance()) {
        return false;
    }
    name_ = std::move(name);
    advance();
    return true;
}

std::optional<CreateFilletFeatureCommand>
FilletDraft::command() const {
    if (!valid()) {
        return std::nullopt;
    }
    CreateFilletFeatureCommand result{
        edges_,
        source_revision_,
        *radius_,
        name_};
    return result.valid()
        ? std::optional<
              CreateFilletFeatureCommand>{
              std::move(result)}
        : std::nullopt;
}

ChamferDraft::ChamferDraft(
    core::DocumentId document_id,
    core::DocumentRevision source_revision,
    std::optional<part::BodyStageRef>
        required_stage)
    : document_id_{std::move(document_id)},
      source_revision_{source_revision},
      required_stage_{std::move(
          required_stage)} {}

ChamferDraft
ChamferDraft::beginCreate(
    const DocumentSession& session) {
    return ChamferDraft{
        session.documentId(),
        session.document().revision(),
        requiredStageForCreate(session)};
}

std::optional<ChamferDraft>
ChamferDraft::beginCreate(
    const DocumentSession& session,
    std::vector<part::MaterialEdgeReference>
        edges) {
    auto draft =
        beginCreate(session);
    if (!draft.setEdges(
            std::move(edges))) {
        return std::nullopt;
    }
    return draft;
}

bool ChamferDraft::valid() const noexcept {
    if (!distance_ ||
        !required_stage_ ||
        edges_.empty()) {
        return false;
    }
    return part::chamferFeatureStructurallyValid(
               part::ChamferFeature{
                   edges_,
                   *distance_}) &&
           edges_.front().stage ==
               *required_stage_;
}

bool ChamferDraft::currentFor(
    const DocumentSession& session) const noexcept {
    return session.documentId() ==
               document_id_ &&
           session.document().revision() ==
               source_revision_;
}

bool ChamferDraft::canAdvance() const noexcept {
    return generation_ !=
           std::numeric_limits<
               EdgeFeatureDraftGeneration>::max();
}

void ChamferDraft::advance() noexcept {
    ++generation_;
}

bool ChamferDraft::setEdges(
    std::vector<part::MaterialEdgeReference>
        edges) {
    if (!canonicalizeEdges(
            edges,
            required_stage_)) {
        return false;
    }
    if (edges_ == edges) {
        return true;
    }
    if (!canAdvance()) {
        return false;
    }
    edges_ = std::move(edges);
    advance();
    return true;
}

bool ChamferDraft::setDistance(
    core::LengthValue distance) noexcept {
    if (!validPositiveLength(distance)) {
        return false;
    }
    if (distance_ &&
        *distance_ == distance) {
        return true;
    }
    if (!canAdvance()) {
        return false;
    }
    distance_ = distance;
    advance();
    return true;
}

bool ChamferDraft::setName(
    std::string name) {
    if (name_ == name) {
        return true;
    }
    if (!canAdvance()) {
        return false;
    }
    name_ = std::move(name);
    advance();
    return true;
}

std::optional<CreateChamferFeatureCommand>
ChamferDraft::command() const {
    if (!valid()) {
        return std::nullopt;
    }
    CreateChamferFeatureCommand result{
        edges_,
        source_revision_,
        *distance_,
        name_};
    return result.valid()
        ? std::optional<
              CreateChamferFeatureCommand>{
              std::move(result)}
        : std::nullopt;
}


EdgeFeatureDraftFinishResult
finishFilletDraft(
    DocumentSession& session,
    const FilletDraft& draft,
    const EdgeFeatureDraftEvaluationResult& evaluation,
    kernel::ISolidModelingKernel& modeling_kernel) {
    if (session.documentId() !=
            draft.documentId() ||
        session.document().revision() !=
            draft.sourceRevision()) {
        return {
            EdgeFeatureDraftFinishStatus::stale_context,
            false,
            std::nullopt,
            "Fillet draft context is stale."};
    }

    const auto command = draft.command();
    if (!command || !draft.radius()) {
        return {
            EdgeFeatureDraftFinishStatus::invalid_draft,
            false,
            std::nullopt,
            "Fillet draft is incomplete or invalid."};
    }

    const bool exact_evaluation =
        evaluation.committable() &&
        evaluation.document_id &&
        *evaluation.document_id ==
            draft.documentId() &&
        evaluation.source_revision ==
            draft.sourceRevision() &&
        evaluation.draft_generation ==
            draft.generation() &&
        evaluation.operation ==
            kernel::EdgeFeatureOperation::fillet &&
        evaluation.edges ==
            draft.edges() &&
        evaluation.parameter ==
            draft.radius();

    if (!exact_evaluation) {
        return {
            EdgeFeatureDraftFinishStatus::stale_evaluation,
            false,
            std::nullopt,
            "Fillet Finish requires the current successful draft evaluation."};
    }

    const auto result =
        session.execute(
            *command,
            modeling_kernel);
    if (!result.ok()) {
        return {
            EdgeFeatureDraftFinishStatus::rejected,
            false,
            std::nullopt,
            result.diagnostic.message};
    }
    return {
        EdgeFeatureDraftFinishStatus::committed,
        result.changed,
        result.feature_id,
        {}};
}

EdgeFeatureDraftFinishResult
finishChamferDraft(
    DocumentSession& session,
    const ChamferDraft& draft,
    const EdgeFeatureDraftEvaluationResult& evaluation,
    kernel::ISolidModelingKernel& modeling_kernel) {
    if (session.documentId() !=
            draft.documentId() ||
        session.document().revision() !=
            draft.sourceRevision()) {
        return {
            EdgeFeatureDraftFinishStatus::stale_context,
            false,
            std::nullopt,
            "Chamfer draft context is stale."};
    }

    const auto command = draft.command();
    if (!command || !draft.distance()) {
        return {
            EdgeFeatureDraftFinishStatus::invalid_draft,
            false,
            std::nullopt,
            "Chamfer draft is incomplete or invalid."};
    }

    const bool exact_evaluation =
        evaluation.committable() &&
        evaluation.document_id &&
        *evaluation.document_id ==
            draft.documentId() &&
        evaluation.source_revision ==
            draft.sourceRevision() &&
        evaluation.draft_generation ==
            draft.generation() &&
        evaluation.operation ==
            kernel::EdgeFeatureOperation::chamfer &&
        evaluation.edges ==
            draft.edges() &&
        evaluation.parameter ==
            draft.distance();

    if (!exact_evaluation) {
        return {
            EdgeFeatureDraftFinishStatus::stale_evaluation,
            false,
            std::nullopt,
            "Chamfer Finish requires the current successful draft evaluation."};
    }

    const auto result =
        session.execute(
            *command,
            modeling_kernel);
    if (!result.ok()) {
        return {
            EdgeFeatureDraftFinishStatus::rejected,
            false,
            std::nullopt,
            result.diagnostic.message};
    }
    return {
        EdgeFeatureDraftFinishStatus::committed,
        result.changed,
        result.feature_id,
        {}};
}

} // namespace simplesolid2::application
