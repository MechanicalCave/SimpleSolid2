#pragma once

#include <simplesolid2/core/document.hpp>
#include <simplesolid2/core/units.hpp>
#include <simplesolid2/part/feature.hpp>
#include <simplesolid2/part/semantic_topology_reference.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace simplesolid2::application {

class DocumentSession;

using EdgeFeatureDraftGeneration = std::uint64_t;

struct CreateFilletFeatureCommand final {
    std::vector<part::MaterialEdgeReference> edges;
    core::DocumentRevision expected_revision;
    core::LengthValue radius;
    std::string name;

    [[nodiscard]] bool valid() const noexcept;
};

struct CreateChamferFeatureCommand final {
    std::vector<part::MaterialEdgeReference> edges;
    core::DocumentRevision expected_revision;
    core::LengthValue distance;
    std::string name;

    [[nodiscard]] bool valid() const noexcept;
};

class FilletDraft final {
public:
    [[nodiscard]] static FilletDraft
    beginCreate(const DocumentSession& session);

    [[nodiscard]] static std::optional<FilletDraft>
    beginCreate(
        const DocumentSession& session,
        std::vector<part::MaterialEdgeReference>
            edges);

    [[nodiscard]] const core::DocumentId&
    documentId() const noexcept {
        return document_id_;
    }

    [[nodiscard]] core::DocumentRevision
    sourceRevision() const noexcept {
        return source_revision_;
    }

    [[nodiscard]] EdgeFeatureDraftGeneration
    generation() const noexcept {
        return generation_;
    }

    [[nodiscard]] const std::optional<
        part::BodyStageRef>&
    requiredStage() const noexcept {
        return required_stage_;
    }

    [[nodiscard]] const std::vector<
        part::MaterialEdgeReference>&
    edges() const noexcept {
        return edges_;
    }

    [[nodiscard]] const std::optional<
        core::LengthValue>&
    radius() const noexcept {
        return radius_;
    }

    [[nodiscard]] const std::string&
    name() const noexcept {
        return name_;
    }

    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] bool currentFor(
        const DocumentSession& session) const noexcept;

    [[nodiscard]] bool setEdges(
        std::vector<part::MaterialEdgeReference>
            edges);
    [[nodiscard]] bool setRadius(
        core::LengthValue radius) noexcept;
    [[nodiscard]] bool setName(
        std::string name);

    [[nodiscard]] std::optional<
        CreateFilletFeatureCommand>
    command() const;

private:
    FilletDraft(
        core::DocumentId document_id,
        core::DocumentRevision source_revision,
        std::optional<part::BodyStageRef>
            required_stage);

    [[nodiscard]] bool canAdvance() const noexcept;
    void advance() noexcept;

    core::DocumentId document_id_;
    core::DocumentRevision source_revision_;
    EdgeFeatureDraftGeneration generation_{1U};
    std::optional<part::BodyStageRef>
        required_stage_;
    std::vector<part::MaterialEdgeReference>
        edges_;
    std::optional<core::LengthValue> radius_;
    std::string name_;
};

class ChamferDraft final {
public:
    [[nodiscard]] static ChamferDraft
    beginCreate(const DocumentSession& session);

    [[nodiscard]] static std::optional<ChamferDraft>
    beginCreate(
        const DocumentSession& session,
        std::vector<part::MaterialEdgeReference>
            edges);

    [[nodiscard]] const core::DocumentId&
    documentId() const noexcept {
        return document_id_;
    }

    [[nodiscard]] core::DocumentRevision
    sourceRevision() const noexcept {
        return source_revision_;
    }

    [[nodiscard]] EdgeFeatureDraftGeneration
    generation() const noexcept {
        return generation_;
    }

    [[nodiscard]] const std::optional<
        part::BodyStageRef>&
    requiredStage() const noexcept {
        return required_stage_;
    }

    [[nodiscard]] const std::vector<
        part::MaterialEdgeReference>&
    edges() const noexcept {
        return edges_;
    }

    [[nodiscard]] const std::optional<
        core::LengthValue>&
    distance() const noexcept {
        return distance_;
    }

    [[nodiscard]] const std::string&
    name() const noexcept {
        return name_;
    }

    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] bool currentFor(
        const DocumentSession& session) const noexcept;

    [[nodiscard]] bool setEdges(
        std::vector<part::MaterialEdgeReference>
            edges);
    [[nodiscard]] bool setDistance(
        core::LengthValue distance) noexcept;
    [[nodiscard]] bool setName(
        std::string name);

    [[nodiscard]] std::optional<
        CreateChamferFeatureCommand>
    command() const;

private:
    ChamferDraft(
        core::DocumentId document_id,
        core::DocumentRevision source_revision,
        std::optional<part::BodyStageRef>
            required_stage);

    [[nodiscard]] bool canAdvance() const noexcept;
    void advance() noexcept;

    core::DocumentId document_id_;
    core::DocumentRevision source_revision_;
    EdgeFeatureDraftGeneration generation_{1U};
    std::optional<part::BodyStageRef>
        required_stage_;
    std::vector<part::MaterialEdgeReference>
        edges_;
    std::optional<core::LengthValue> distance_;
    std::string name_;
};

} // namespace simplesolid2::application
