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

bool EditFilletFeatureCommand::valid()
    const noexcept {
    return feature_id.valid() &&
           part::filletFeatureStructurallyValid(
               part::FilletFeature{
                   edges,
                   radius});
}

bool EditChamferFeatureCommand::valid()
    const noexcept {
    return feature_id.valid() &&
           part::chamferFeatureStructurallyValid(
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

std::optional<FilletDraft>
FilletDraft::beginEdit(
    const DocumentSession& session,
    part::FeatureId feature_id) {
    const auto* feature =
        session.document().findFeature(feature_id);
    if (feature == nullptr || feature->suppressed) {
        return std::nullopt;
    }
    const auto* fillet =
        std::get_if<part::FilletFeature>(
            &feature->definition);
    if (fillet == nullptr || fillet->edges.empty()) {
        return std::nullopt;
    }

    auto draft = FilletDraft{
        session.documentId(),
        session.document().revision(),
        fillet->edges.front().stage};
    draft.mode_ = EdgeFeatureDraftMode::edit;
    draft.feature_id_ = feature_id;
    draft.edges_ = fillet->edges;
    draft.radius_ = fillet->radius;
    draft.name_ = feature->name;
    return draft.valid()
        ? std::optional<FilletDraft>{std::move(draft)}
        : std::nullopt;
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
    if (mode_ != EdgeFeatureDraftMode::create ||
        !valid()) {
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

std::optional<EditFilletFeatureCommand>
FilletDraft::editCommand() const {
    if (mode_ != EdgeFeatureDraftMode::edit ||
        !feature_id_ ||
        !valid()) {
        return std::nullopt;
    }
    EditFilletFeatureCommand result{
        *feature_id_,
        edges_,
        source_revision_,
        *radius_,
        name_};
    return result.valid()
        ? std::optional<EditFilletFeatureCommand>{
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

std::optional<ChamferDraft>
ChamferDraft::beginEdit(
    const DocumentSession& session,
    part::FeatureId feature_id) {
    const auto* feature =
        session.document().findFeature(feature_id);
    if (feature == nullptr || feature->suppressed) {
        return std::nullopt;
    }
    const auto* chamfer =
        std::get_if<part::ChamferFeature>(
            &feature->definition);
    if (chamfer == nullptr || chamfer->edges.empty()) {
        return std::nullopt;
    }

    auto draft = ChamferDraft{
        session.documentId(),
        session.document().revision(),
        chamfer->edges.front().stage};
    draft.mode_ = EdgeFeatureDraftMode::edit;
    draft.feature_id_ = feature_id;
    draft.edges_ = chamfer->edges;
    draft.distance_ = chamfer->distance;
    draft.name_ = feature->name;
    return draft.valid()
        ? std::optional<ChamferDraft>{std::move(draft)}
        : std::nullopt;
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
    if (mode_ != EdgeFeatureDraftMode::create ||
        !valid()) {
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

std::optional<EditChamferFeatureCommand>
ChamferDraft::editCommand() const {
    if (mode_ != EdgeFeatureDraftMode::edit ||
        !feature_id_ ||
        !valid()) {
        return std::nullopt;
    }
    EditChamferFeatureCommand result{
        *feature_id_,
        edges_,
        source_revision_,
        *distance_,
        name_};
    return result.valid()
        ? std::optional<EditChamferFeatureCommand>{
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

    if (!draft.valid() || !draft.radius()) {
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
        evaluation.mode ==
            draft.mode() &&
        evaluation.feature_id ==
            draft.featureId() &&
        evaluation.operation ==
            kernel::EdgeFeatureOperation::fillet &&
        evaluation.edges ==
            draft.edges() &&
        evaluation.parameter ==
            draft.radius() &&
        evaluation.name ==
            draft.name();

    if (!exact_evaluation) {
        return {
            EdgeFeatureDraftFinishStatus::stale_evaluation,
            false,
            std::nullopt,
            "Fillet Finish requires the current successful draft evaluation."};
    }

    if (draft.mode() ==
        EdgeFeatureDraftMode::create) {
        const auto command = draft.command();
        if (!command) {
            return {
                EdgeFeatureDraftFinishStatus::invalid_draft,
                false,
                std::nullopt,
                "Fillet create draft has no valid command."};
        }
        const auto result =
            session.execute(*command, modeling_kernel);
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

    const auto command = draft.editCommand();
    if (!command || !draft.featureId()) {
        return {
            EdgeFeatureDraftFinishStatus::invalid_draft,
            false,
            std::nullopt,
            "Fillet edit draft has no valid FeatureId/command."};
    }
    const auto result =
        session.execute(*command, modeling_kernel);
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
        draft.featureId(),
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

    if (!draft.valid() || !draft.distance()) {
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
        evaluation.mode ==
            draft.mode() &&
        evaluation.feature_id ==
            draft.featureId() &&
        evaluation.operation ==
            kernel::EdgeFeatureOperation::chamfer &&
        evaluation.edges ==
            draft.edges() &&
        evaluation.parameter ==
            draft.distance() &&
        evaluation.name ==
            draft.name();

    if (!exact_evaluation) {
        return {
            EdgeFeatureDraftFinishStatus::stale_evaluation,
            false,
            std::nullopt,
            "Chamfer Finish requires the current successful draft evaluation."};
    }

    if (draft.mode() ==
        EdgeFeatureDraftMode::create) {
        const auto command = draft.command();
        if (!command) {
            return {
                EdgeFeatureDraftFinishStatus::invalid_draft,
                false,
                std::nullopt,
                "Chamfer create draft has no valid command."};
        }
        const auto result =
            session.execute(*command, modeling_kernel);
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

    const auto command = draft.editCommand();
    if (!command || !draft.featureId()) {
        return {
            EdgeFeatureDraftFinishStatus::invalid_draft,
            false,
            std::nullopt,
            "Chamfer edit draft has no valid FeatureId/command."};
    }
    const auto result =
        session.execute(*command, modeling_kernel);
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
        draft.featureId(),
        {}};
}

} // namespace simplesolid2::application
