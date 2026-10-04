#include <simplesolid2/part/feature_evaluation.hpp>

#include <simplesolid2/part/feature.hpp>
#include <simplesolid2/part/part_document.hpp>
#include <simplesolid2/part/profile_kernel_input.hpp>

#include <algorithm>
#include <map>
#include <utility>

namespace simplesolid2::part {
namespace {

[[nodiscard]] FeatureEvaluationDiagnosticCode
diagnosticForKernel(
    kernel::SolidModelingStatus status) noexcept {
    switch (status) {
    case kernel::SolidModelingStatus::ok:
        return FeatureEvaluationDiagnosticCode::none;
    case kernel::SolidModelingStatus::invalid_input:
        return FeatureEvaluationDiagnosticCode::
            kernel_invalid_input;
    case kernel::SolidModelingStatus::missing_upstream:
        return FeatureEvaluationDiagnosticCode::
            missing_upstream_body;
    case kernel::SolidModelingStatus::provider_mismatch:
        return FeatureEvaluationDiagnosticCode::
            kernel_provider_mismatch;
    case kernel::SolidModelingStatus::provider_failure:
        return FeatureEvaluationDiagnosticCode::
            kernel_provider_failure;
    case kernel::SolidModelingStatus::invalid_brep:
        return FeatureEvaluationDiagnosticCode::
            invalid_brep;
    case kernel::SolidModelingStatus::detached_add:
        return FeatureEvaluationDiagnosticCode::
            detached_add;
    case kernel::SolidModelingStatus::no_effect:
        return FeatureEvaluationDiagnosticCode::
            no_effect;
    case kernel::SolidModelingStatus::empty_result:
        return FeatureEvaluationDiagnosticCode::
            empty_result;
    case kernel::SolidModelingStatus::multi_solid:
        return FeatureEvaluationDiagnosticCode::
            multi_solid;
    }
    return FeatureEvaluationDiagnosticCode::
        kernel_provider_failure;
}

[[nodiscard]] kernel::SolidBooleanOperation
kernelOperation(
    ExtrudeOperation operation) noexcept {
    return operation == ExtrudeOperation::cut
        ? kernel::SolidBooleanOperation::cut
        : kernel::SolidBooleanOperation::add;
}

} // namespace

std::optional<kernel::LinearExtrudeInput>
makeKernelExtrudeInput(
    const PartDocument& document,
    const ExtrudeFeature& feature) {
    auto profile =
        makeKernelProfileInput(
            document,
            feature.profile_id);
    if (!profile) {
        return std::nullopt;
    }

    kernel::LinearExtrudeInput result;
    result.profile = std::move(*profile);
    result.operation =
        kernelOperation(feature.operation);

    if (const auto* one_sided =
            std::get_if<OneSidedExtrudeExtent>(
                &feature.extent)) {
        const double distance =
            one_sided->distance.millimetres;
        if (one_sided->reversed) {
            result.start_offset_mm =
                -distance;
            result.end_offset_mm = 0.0;
            result.start_cap_role =
                kernel::ExtrudeCapRole::
                    extent_cap;
            result.end_cap_role =
                kernel::ExtrudeCapRole::
                    profile_cap;
        } else {
            result.start_offset_mm = 0.0;
            result.end_offset_mm =
                distance;
            result.start_cap_role =
                kernel::ExtrudeCapRole::
                    profile_cap;
            result.end_cap_role =
                kernel::ExtrudeCapRole::
                    extent_cap;
        }
        return result.valid()
            ? std::optional<
                  kernel::LinearExtrudeInput>{
                  std::move(result)}
            : std::nullopt;
    }

    const auto* midplane =
        std::get_if<MidplaneExtrudeExtent>(
            &feature.extent);
    if (midplane == nullptr) {
        return std::nullopt;
    }

    const double half =
        midplane->total_distance
            .millimetres *
        0.5;
    result.start_offset_mm = -half;
    result.end_offset_mm = half;
    result.start_cap_role =
        kernel::ExtrudeCapRole::
            negative_cap;
    result.end_cap_role =
        kernel::ExtrudeCapRole::
            positive_cap;
    return result.valid()
        ? std::optional<
              kernel::LinearExtrudeInput>{
              std::move(result)}
        : std::nullopt;
}

namespace {

[[nodiscard]] FeatureFaceRoleKind
partCapRole(
    kernel::ExtrudeCapRole role) noexcept {
    switch (role) {
    case kernel::ExtrudeCapRole::profile_cap:
        return FeatureFaceRoleKind::
            profile_cap;
    case kernel::ExtrudeCapRole::extent_cap:
        return FeatureFaceRoleKind::
            extent_cap;
    case kernel::ExtrudeCapRole::negative_cap:
        return FeatureFaceRoleKind::
            negative_cap;
    case kernel::ExtrudeCapRole::positive_cap:
        return FeatureFaceRoleKind::
            positive_cap;
    }
    return FeatureFaceRoleKind::side;
}

[[nodiscard]] FeatureSurfaceRoleKind
partSurfaceCapRole(
    kernel::ExtrudeCapRole role) noexcept {
    switch (role) {
    case kernel::ExtrudeCapRole::profile_cap:
        return FeatureSurfaceRoleKind::
            profile_cap;
    case kernel::ExtrudeCapRole::extent_cap:
        return FeatureSurfaceRoleKind::
            extent_cap;
    case kernel::ExtrudeCapRole::negative_cap:
        return FeatureSurfaceRoleKind::
            negative_cap;
    case kernel::ExtrudeCapRole::positive_cap:
        return FeatureSurfaceRoleKind::
            positive_cap;
    }
    return FeatureSurfaceRoleKind::side;
}

[[nodiscard]] FeatureFaceResolution
convertNewFace(
    FeatureId producer,
    const kernel::NewFaceLineage& source) {
    FeatureFaceResolution result;
    result.address.producer_feature_id =
        producer;
    result.status = source.status;
    result.candidate_count =
        source.candidate_count;
    result.runtime_token =
        source.resolved_token;

    if (source.role.kind ==
        kernel::ExtrudeGeneratedFaceRoleKind::cap) {
        if (!source.role.cap_role) {
            result.status =
                kernel::ReferenceStatus::
                    unsupported;
            result.runtime_token.reset();
            return result;
        }
        result.address.role =
            partCapRole(
                *source.role.cap_role);
        return result;
    }

    result.address.role =
        FeatureFaceRoleKind::side;
    if (!source.role.side_provenance) {
        result.status =
            kernel::ReferenceStatus::
                unsupported;
        result.runtime_token.reset();
        return result;
    }

    const auto& provenance =
        *source.role.side_provenance;
    const auto entity =
        sketch::EntityId::parse(
            provenance.source_entity);
    if (!entity) {
        result.status =
            kernel::ReferenceStatus::
                unsupported;
        result.runtime_token.reset();
        return result;
    }

    result.address.source_entity =
        *entity;
    result.address.loop_index =
        provenance.loop_index;
    result.address.use_index =
        provenance.use_index;
    result.address.hole =
        provenance.hole;
    return result;
}

[[nodiscard]] FeatureSurfaceResolution
convertNewSurface(
    FeatureId producer,
    const kernel::NewSurfaceLineage& source) {
    FeatureSurfaceResolution result;
    result.address.producer_feature_id =
        producer;
    result.status =
        source.surface_status;
    result.strict_face_status =
        source.strict_face_status;
    result.candidate_face_count =
        source.candidate_face_count;
    result.surface_kind =
        source.surface_kind;
    result.canonical_frame =
        source.canonical_frame;
    result.runtime_token =
        source.resolved_token;
    result.current_faces =
        source.current_faces;

    if (source.role.kind ==
        kernel::ExtrudeGeneratedFaceRoleKind::cap) {
        if (!source.role.cap_role) {
            result.status =
                kernel::ReferenceStatus::
                    unsupported;
            result.strict_face_status =
                kernel::ReferenceStatus::
                    unsupported;
            result.runtime_token.reset();
            result.current_faces.clear();
            result.candidate_face_count = 0U;
            return result;
        }
        result.address.role =
            partSurfaceCapRole(
                *source.role.cap_role);
        return result;
    }

    result.address.role =
        FeatureSurfaceRoleKind::side;
    if (!source.role.side_provenance) {
        result.status =
            kernel::ReferenceStatus::
                unsupported;
        result.strict_face_status =
            kernel::ReferenceStatus::
                unsupported;
        result.runtime_token.reset();
        result.current_faces.clear();
        result.candidate_face_count = 0U;
        return result;
    }

    const auto& provenance =
        *source.role.side_provenance;
    const auto entity =
        sketch::EntityId::parse(
            provenance.source_entity);
    if (!entity) {
        result.status =
            kernel::ReferenceStatus::
                unsupported;
        result.strict_face_status =
            kernel::ReferenceStatus::
                unsupported;
        result.runtime_token.reset();
        result.current_faces.clear();
        result.candidate_face_count = 0U;
        return result;
    }

    result.address.source_entity =
        *entity;
    result.address.loop_index =
        provenance.loop_index;
    result.address.use_index =
        provenance.use_index;
    result.address.hole =
        provenance.hole;
    return result;
}

void propagateCurrentReferences(
    std::vector<FeatureFaceResolution>& references,
    const std::vector<
        kernel::InheritedFaceLineage>& inherited) {
    std::map<std::uint64_t, std::size_t>
        by_token;
    for (std::size_t index = 0U;
         index < references.size();
         ++index) {
        if (references[index].status ==
                kernel::ReferenceStatus::
                    resolved &&
            references[index].runtime_token) {
            by_token.emplace(
                references[index]
                    .runtime_token->value,
                index);
        }
    }

    std::vector<bool> seen(
        references.size(),
        false);
    for (const auto& item : inherited) {
        const auto found =
            by_token.find(
                item.token.value);
        if (found == by_token.end()) {
            continue;
        }

        auto& reference =
            references[found->second];
        reference.status =
            item.status;
        reference.candidate_count =
            item.candidate_count;
        seen[found->second] = true;
        if (item.status !=
            kernel::ReferenceStatus::
                resolved) {
            reference.runtime_token.reset();
        }
    }

    // Every previously Resolved reference must be reported by the provider.
    // Missing propagation evidence is fail-closed, never assumed Resolved.
    for (const auto& [token, index] :
         by_token) {
        static_cast<void>(token);
        if (!seen[index]) {
            references[index].status =
                kernel::ReferenceStatus::
                    unsupported;
            references[index]
                .candidate_count = 0U;
            references[index]
                .runtime_token.reset();
        }
    }
}

[[nodiscard]] bool propagateCurrentSurfaces(
    std::vector<FeatureSurfaceResolution>& references,
    const std::vector<
        kernel::InheritedSurfaceLineage>& inherited) {
    std::map<std::uint64_t, std::size_t>
        by_token;
    for (std::size_t index = 0U;
         index < references.size();
         ++index) {
        if (references[index].status !=
                kernel::ReferenceStatus::resolved ||
            !references[index].runtime_token) {
            continue;
        }
        const auto [it, inserted] =
            by_token.emplace(
                references[index]
                    .runtime_token->value,
                index);
        static_cast<void>(it);
        if (!inserted) {
            return false;
        }
    }

    std::vector<bool> seen(
        references.size(),
        false);
    for (const auto& item : inherited) {
        if (!item.token.valid()) {
            return false;
        }
        const auto found =
            by_token.find(item.token.value);
        if (found == by_token.end()) {
            return false;
        }

        auto& reference =
            references[found->second];
        if (reference.surface_kind !=
            item.surface_kind) {
            return false;
        }
        if (item.surface_status ==
                kernel::ReferenceStatus::resolved &&
            reference.canonical_frame !=
                item.canonical_frame) {
            return false;
        }

        reference.status =
            item.surface_status;
        reference.strict_face_status =
            item.strict_face_status;
        reference.candidate_face_count =
            item.candidate_face_count;
        reference.canonical_frame =
            item.canonical_frame;
        reference.current_faces =
            item.current_faces;
        seen[found->second] = true;

        if (item.surface_status !=
            kernel::ReferenceStatus::resolved) {
            reference.runtime_token.reset();
        }
    }

    for (const auto& [token, index] :
         by_token) {
        static_cast<void>(token);
        if (!seen[index]) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] bool sameSurfaceAddressSet(
    const std::vector<FeatureSurfaceAddress>& first,
    const std::vector<FeatureSurfaceAddress>& second) {
    if (first.size() != second.size()) return false;
    return std::all_of(
        first.begin(),
        first.end(),
        [&second](const FeatureSurfaceAddress& address) {
            return std::find(
                       second.begin(),
                       second.end(),
                       address) != second.end();
        });
}

[[nodiscard]] bool uniqueSurfaceAddressSet(
    const std::vector<FeatureSurfaceAddress>& addresses,
    std::size_t expected_size) {
    if (addresses.size() != expected_size) return false;
    for (std::size_t i = 0U; i < addresses.size(); ++i) {
        if (!addresses[i].valid()) return false;
        for (std::size_t j = i + 1U; j < addresses.size(); ++j) {
            if (addresses[i] == addresses[j]) return false;
        }
    }
    return true;
}

[[nodiscard]] std::optional<std::vector<FeatureSurfaceAddress>>
surfaceAddressesForTokens(
    const std::vector<kernel::RuntimeSurfaceToken>& tokens,
    const std::vector<FeatureSurfaceResolution>& surfaces) {
    std::vector<FeatureSurfaceAddress> result;
    result.reserve(tokens.size());
    for (const auto token : tokens) {
        if (!token.valid()) return std::nullopt;
        const FeatureSurfaceResolution* matched = nullptr;
        for (const auto& surface : surfaces) {
            if (surface.status != kernel::ReferenceStatus::resolved ||
                !surface.runtime_token ||
                *surface.runtime_token != token) {
                continue;
            }
            if (matched != nullptr) return std::nullopt;
            matched = &surface;
        }
        if (matched == nullptr || !matched->address.valid()) {
            return std::nullopt;
        }
        if (std::find(result.begin(), result.end(), matched->address) !=
            result.end()) {
            return std::nullopt;
        }
        result.push_back(matched->address);
    }
    return result;
}

[[nodiscard]] std::optional<FeatureId> commonSurfaceProducer(
    const std::vector<FeatureSurfaceAddress>& surfaces) {
    if (surfaces.empty()) return std::nullopt;
    const auto producer = surfaces.front().producer_feature_id;
    if (!producer.valid()) return std::nullopt;
    for (const auto& surface : surfaces) {
        if (surface.producer_feature_id != producer) {
            return std::nullopt;
        }
    }
    return producer;
}

struct CurrentEdgeCandidate final {
    kernel::RuntimeEdgeToken token;
    FeatureEdgeAddress address;
    kernel::CurveKind curve_kind{kernel::CurveKind::other};
    bool produced_by_current_operation{false};
};

struct CurrentPointCandidate final {
    kernel::RuntimeVertexToken token;
    FeaturePointAddress address;
    std::optional<kernel::Point3> point;
    bool produced_by_current_operation{false};
};

[[nodiscard]] const FeatureEdgeResolution* findEdgeResolution(
    const std::vector<FeatureEdgeResolution>& references,
    const FeatureEdgeAddress& address) {
    const auto found = std::find_if(
        references.begin(),
        references.end(),
        [&address](const FeatureEdgeResolution& item) {
            return item.address == address;
        });
    return found == references.end() ? nullptr : &*found;
}

[[nodiscard]] const FeaturePointResolution* findPointResolution(
    const std::vector<FeaturePointResolution>& references,
    const FeaturePointAddress& address) {
    const auto found = std::find_if(
        references.begin(),
        references.end(),
        [&address](const FeaturePointResolution& item) {
            return item.address == address;
        });
    return found == references.end() ? nullptr : &*found;
}

[[nodiscard]] std::optional<std::vector<CurrentEdgeCandidate>>
convertCurrentEdgeCandidates(
    const kernel::SolidModelingResult& kernel_result,
    const std::vector<FeatureSurfaceResolution>& surfaces) {
    if (kernel_result.current_edge_semantics.size() !=
        kernel_result.current_edges.size()) {
        return std::nullopt;
    }

    std::vector<CurrentEdgeCandidate> result;
    result.reserve(kernel_result.current_edge_semantics.size());
    for (const auto& semantic : kernel_result.current_edge_semantics) {
        if (!semantic.token.valid() ||
            std::count(
                kernel_result.current_edges.begin(),
                kernel_result.current_edges.end(),
                semantic.token) != 1) {
            return std::nullopt;
        }
        if (semantic.integrity_failure) return std::nullopt;

        if (semantic.representation_artifact ||
            semantic.role == kernel::EdgeSemanticRoleKind::periodic_seam ||
            semantic.role == kernel::EdgeSemanticRoleKind::unsupported ||
            semantic.status == kernel::ReferenceStatus::unsupported) {
            continue;
        }
        if (semantic.curve_kind == kernel::CurveKind::other) {
            return std::nullopt;
        }

        const auto mapped =
            surfaceAddressesForTokens(
                semantic.adjacent_surfaces,
                surfaces);
        if (!mapped || !uniqueSurfaceAddressSet(*mapped, 2U)) {
            return std::nullopt;
        }

        FeatureEdgeAddress address;
        address.role = semantic.role;
        address.adjacent_surfaces = *mapped;
        if (!address.valid()) return std::nullopt;

        result.push_back(
            {
                semantic.token,
                std::move(address),
                semantic.curve_kind,
                semantic.produced_by_current_operation,
            });
    }
    return result;
}

[[nodiscard]] std::optional<std::vector<CurrentPointCandidate>>
convertCurrentPointCandidates(
    const kernel::SolidModelingResult& kernel_result,
    const std::vector<FeatureSurfaceResolution>& surfaces) {
    if (kernel_result.current_vertex_semantics.size() !=
        kernel_result.current_vertices.size()) {
        return std::nullopt;
    }

    std::vector<CurrentPointCandidate> result;
    result.reserve(kernel_result.current_vertex_semantics.size());
    for (const auto& semantic : kernel_result.current_vertex_semantics) {
        if (!semantic.token.valid() ||
            std::count(
                kernel_result.current_vertices.begin(),
                kernel_result.current_vertices.end(),
                semantic.token) != 1) {
            return std::nullopt;
        }
        if (semantic.integrity_failure) return std::nullopt;
        if (semantic.status == kernel::ReferenceStatus::unsupported) {
            continue;
        }

        const auto mapped =
            surfaceAddressesForTokens(
                semantic.adjacent_surfaces,
                surfaces);
        if (!mapped ||
            !uniqueSurfaceAddressSet(*mapped, 3U) ||
            semantic.incident_material_edge_count != 3U) {
            return std::nullopt;
        }

        FeaturePointAddress address;
        address.adjacent_surfaces = *mapped;
        if (!address.valid()) return std::nullopt;

        result.push_back(
            {
                semantic.token,
                std::move(address),
                semantic.provider_point,
                semantic.produced_by_current_operation,
            });
    }
    return result;
}

[[nodiscard]] std::optional<std::vector<FeatureEdgeResolution>>
resolveCurrentEdges(
    FeatureId current_feature,
    const kernel::SolidModelingResult& kernel_result,
    const std::vector<FeatureSurfaceResolution>& surfaces,
    const std::vector<FeatureEdgeResolution>& previous) {
    const auto candidates =
        convertCurrentEdgeCandidates(
            kernel_result,
            surfaces);
    if (!candidates) return std::nullopt;

    std::vector<FeatureEdgeAddress> addresses;
    addresses.reserve(previous.size() + candidates->size());
    for (const auto& old : previous) {
        if (!old.address.valid()) return std::nullopt;
        if (std::find(addresses.begin(), addresses.end(), old.address) ==
            addresses.end()) {
            addresses.push_back(old.address);
        }
    }
    for (const auto& candidate : *candidates) {
        if (std::find(
                addresses.begin(),
                addresses.end(),
                candidate.address) == addresses.end()) {
            addresses.push_back(candidate.address);
        }
    }

    std::vector<FeatureEdgeResolution> result;
    result.reserve(addresses.size());
    for (const auto& address : addresses) {
        FeatureEdgeResolution resolved;
        resolved.address = address;

        if (const auto* old = findEdgeResolution(previous, address)) {
            resolved.producer_feature_id = old->producer_feature_id;
            resolved.curve_kind = old->curve_kind;
        }

        bool produced_now = false;
        for (const auto& candidate : *candidates) {
            if (!(candidate.address == address)) continue;
            resolved.current_edges.push_back(candidate.token);
            produced_now =
                produced_now ||
                candidate.produced_by_current_operation;
            if (resolved.curve_kind == kernel::CurveKind::other) {
                resolved.curve_kind = candidate.curve_kind;
            } else if (resolved.curve_kind != candidate.curve_kind) {
                return std::nullopt;
            }
        }

        resolved.candidate_count = resolved.current_edges.size();
        resolved.status =
            resolved.candidate_count == 0U
                ? kernel::ReferenceStatus::missing
                : resolved.candidate_count == 1U
                    ? kernel::ReferenceStatus::resolved
                    : kernel::ReferenceStatus::ambiguous;

        if (resolved.curve_kind == kernel::CurveKind::other) {
            return std::nullopt;
        }

        if (!resolved.producer_feature_id) {
            resolved.producer_feature_id =
                commonSurfaceProducer(address.adjacent_surfaces);
            if (!resolved.producer_feature_id && produced_now) {
                resolved.producer_feature_id = current_feature;
            }
        }

        if (!resolved.valid()) return std::nullopt;
        result.push_back(std::move(resolved));
    }
    return result;
}

[[nodiscard]] std::optional<std::vector<FeaturePointResolution>>
resolveCurrentPoints(
    FeatureId current_feature,
    const kernel::SolidModelingResult& kernel_result,
    const std::vector<FeatureSurfaceResolution>& surfaces,
    const std::vector<FeaturePointResolution>& previous) {
    const auto candidates =
        convertCurrentPointCandidates(
            kernel_result,
            surfaces);
    if (!candidates) return std::nullopt;

    std::vector<FeaturePointAddress> addresses;
    addresses.reserve(previous.size() + candidates->size());
    for (const auto& old : previous) {
        if (!old.address.valid()) return std::nullopt;
        if (std::find(addresses.begin(), addresses.end(), old.address) ==
            addresses.end()) {
            addresses.push_back(old.address);
        }
    }
    for (const auto& candidate : *candidates) {
        if (std::find(
                addresses.begin(),
                addresses.end(),
                candidate.address) == addresses.end()) {
            addresses.push_back(candidate.address);
        }
    }

    std::vector<FeaturePointResolution> result;
    result.reserve(addresses.size());
    for (const auto& address : addresses) {
        FeaturePointResolution resolved;
        resolved.address = address;

        if (const auto* old = findPointResolution(previous, address)) {
            resolved.producer_feature_id = old->producer_feature_id;
        }

        bool produced_now = false;
        std::optional<kernel::Point3> singular_point;
        for (const auto& candidate : *candidates) {
            if (!(candidate.address == address)) continue;
            resolved.current_vertices.push_back(candidate.token);
            produced_now =
                produced_now ||
                candidate.produced_by_current_operation;
            if (!singular_point) {
                singular_point = candidate.point;
            }
        }

        resolved.candidate_count =
            resolved.current_vertices.size();
        resolved.status =
            resolved.candidate_count == 0U
                ? kernel::ReferenceStatus::missing
                : resolved.candidate_count == 1U
                    ? kernel::ReferenceStatus::resolved
                    : kernel::ReferenceStatus::ambiguous;

        if (resolved.status == kernel::ReferenceStatus::resolved) {
            if (!singular_point) return std::nullopt;
            resolved.current_point = singular_point;
        }

        if (!resolved.producer_feature_id) {
            resolved.producer_feature_id =
                commonSurfaceProducer(address.adjacent_surfaces);
            if (!resolved.producer_feature_id && produced_now) {
                resolved.producer_feature_id = current_feature;
            }
        }

        if (!resolved.valid()) return std::nullopt;
        result.push_back(std::move(resolved));
    }
    return result;
}

template <typename Token>
[[nodiscard]] bool uniqueValidTokens(
    const std::vector<Token>& tokens) noexcept {
    for (std::size_t index = 0U;
         index < tokens.size();
         ++index) {
        if (!tokens[index].valid()) {
            return false;
        }
        for (std::size_t other = index + 1U;
             other < tokens.size();
             ++other) {
            if (tokens[index] == tokens[other]) {
                return false;
            }
        }
    }
    return true;
}

[[nodiscard]] std::optional<BodyStageTopologyCatalog>
makeBodyStageTopologyCatalog(
    FeatureId feature_id,
    const kernel::SolidModelingResult& kernel_result,
    const std::vector<FeatureFaceResolution>&
        semantic_faces,
    const std::vector<FeatureSurfaceResolution>&
        semantic_surfaces) {
    if (!feature_id.valid() ||
        !kernel_result.ok() ||
        kernel_result.face_count !=
            kernel_result.current_faces.size() ||
        kernel_result.edge_count !=
            kernel_result.current_edges.size() ||
        kernel_result.vertex_count !=
            kernel_result.current_vertices.size() ||
        !uniqueValidTokens(
            kernel_result.current_faces) ||
        !uniqueValidTokens(
            kernel_result.current_edges) ||
        !uniqueValidTokens(
            kernel_result.current_vertices)) {
        return std::nullopt;
    }

    BodyStageTopologyCatalog result;
    result.stage.kind =
        BodyStageKind::after_feature;
    result.stage.feature_id = feature_id;
    result.surfaces = semantic_surfaces;

    for (const auto& surface :
         result.surfaces) {
        if (!surface.valid()) {
            return std::nullopt;
        }
        for (const auto token :
             surface.current_faces) {
            if (std::count(
                    kernel_result.current_faces.begin(),
                    kernel_result.current_faces.end(),
                    token) != 1) {
                return std::nullopt;
            }
        }
    }

    result.faces.reserve(
        kernel_result.current_faces.size());
    for (const auto token :
         kernel_result.current_faces) {
        std::size_t strict_claim_count = 0U;
        std::optional<FeatureFaceAddress>
            strict_address;

        for (const auto& reference :
             semantic_faces) {
            if (reference.status !=
                    kernel::ReferenceStatus::
                        resolved ||
                !reference.runtime_token ||
                *reference.runtime_token != token) {
                continue;
            }
            ++strict_claim_count;
            strict_address =
                reference.address;
        }

        if (strict_claim_count > 1U) {
            return std::nullopt;
        }

        BodyFaceTopologyRecord record;
        record.runtime_token = token;
        record.semantic_address =
            std::move(strict_address);

        for (const auto& surface :
             semantic_surfaces) {
            if (surface.status ==
                    kernel::ReferenceStatus::
                        missing ||
                surface.status ==
                    kernel::ReferenceStatus::
                        unsupported) {
                continue;
            }
            if (std::find(
                    surface.current_faces.begin(),
                    surface.current_faces.end(),
                    token) ==
                surface.current_faces.end()) {
                continue;
            }
            if (std::find(
                    record.surface_candidates.begin(),
                    record.surface_candidates.end(),
                    surface.address) ==
                record.surface_candidates.end()) {
                record.surface_candidates.push_back(
                    surface.address);
            }
        }

        if (!record.surface_candidates.empty()) {
            record.accounting_class =
                TopologyAccountingClass::
                    referenceable;
        } else if (record.semantic_address) {
            // A strict semantic Face without a carrier is an incomplete
            // PM-02B semantic claim, not a valid fallback.
            return std::nullopt;
        } else {
            record.accounting_class =
                TopologyAccountingClass::
                    semantically_unsupported;
        }

        result.faces.push_back(
            std::move(record));
    }

    // Every current supported Extrude Add/Cut Face must map to at least one
    // semantic Surface carrier. Multiple candidates are explicit ambiguity.
    if (!result.faces.empty() &&
        std::any_of(
            result.faces.begin(),
            result.faces.end(),
            [](const BodyFaceTopologyRecord& face) {
                return face.surface_candidates.empty();
            })) {
        return std::nullopt;
    }

    // Every semantic Face still reported Resolved must belong to exactly one
    // current provider Face in this same stage inventory.
    for (const auto& reference :
         semantic_faces) {
        if (reference.status !=
            kernel::ReferenceStatus::resolved) {
            continue;
        }
        if (!reference.runtime_token ||
            std::count(
                kernel_result.current_faces.begin(),
                kernel_result.current_faces.end(),
                *reference.runtime_token) != 1) {
            return std::nullopt;
        }
    }

    result.edges.reserve(
        kernel_result.current_edges.size());
    for (const auto token :
         kernel_result.current_edges) {
        result.edges.push_back(
            BodyEdgeTopologyRecord{
                token,
                TopologyAccountingClass::
                    semantically_unsupported});
    }

    result.vertices.reserve(
        kernel_result.current_vertices.size());
    for (const auto token :
         kernel_result.current_vertices) {
        result.vertices.push_back(
            BodyVertexTopologyRecord{
                token,
                TopologyAccountingClass::
                    semantically_unsupported});
    }

    return result.complete()
        ? std::optional<BodyStageTopologyCatalog>{
              std::move(result)}
        : std::nullopt;
}

} // namespace

bool BodyStageRef::valid() const noexcept {
    switch (kind) {
    case BodyStageKind::empty_body:
        return !feature_id.has_value();
    case BodyStageKind::after_feature:
        return feature_id.has_value() &&
               feature_id->valid();
    }
    return false;
}

bool FeatureFaceAddress::valid() const noexcept {
    if (!producer_feature_id.valid()) {
        return false;
    }
    if (role == FeatureFaceRoleKind::side) {
        return source_entity.has_value() &&
               source_entity->valid();
    }
    return !source_entity.has_value();
}

bool FeatureSurfaceAddress::valid() const noexcept {
    if (!producer_feature_id.valid()) {
        return false;
    }
    if (role == FeatureSurfaceRoleKind::side) {
        return source_entity.has_value() &&
               source_entity->valid();
    }
    return !source_entity.has_value();
}

bool FeatureSurfaceResolution::valid() const noexcept {
    if (!address.valid() ||
        candidate_face_count !=
            current_faces.size()) {
        return false;
    }

    for (std::size_t index = 0U;
         index < current_faces.size();
         ++index) {
        if (!current_faces[index].valid()) {
            return false;
        }
        for (std::size_t other = index + 1U;
             other < current_faces.size();
             ++other) {
            if (current_faces[index] ==
                current_faces[other]) {
                return false;
            }
        }
    }

    if (status ==
        kernel::ReferenceStatus::resolved) {
        if (surface_kind ==
            kernel::SurfaceKind::plane) {
            if (!canonical_frame ||
                !canonical_frame->valid()) {
                return false;
            }
        } else if (canonical_frame) {
            return false;
        }
    } else if (canonical_frame) {
        return false;
    }

    switch (status) {
    case kernel::ReferenceStatus::resolved:
        if (!runtime_token ||
            !runtime_token->valid() ||
            candidate_face_count == 0U) {
            return false;
        }
        return candidate_face_count == 1U
            ? strict_face_status ==
                  kernel::ReferenceStatus::
                      resolved
            : strict_face_status ==
                  kernel::ReferenceStatus::
                      ambiguous;
    case kernel::ReferenceStatus::missing:
        return !runtime_token &&
               candidate_face_count == 0U &&
               strict_face_status ==
                   kernel::ReferenceStatus::
                       missing;
    case kernel::ReferenceStatus::ambiguous:
        return !runtime_token &&
               candidate_face_count > 0U &&
               strict_face_status ==
                   kernel::ReferenceStatus::
                       ambiguous;
    case kernel::ReferenceStatus::unsupported:
        return !runtime_token &&
               candidate_face_count == 0U &&
               current_faces.empty() &&
               strict_face_status ==
                   kernel::ReferenceStatus::
                       unsupported;
    }
    return false;
}

bool BodyFaceTopologyRecord::valid() const noexcept {
    if (!runtime_token.valid()) {
        return false;
    }

    if (semantic_address &&
        !semantic_address->valid()) {
        return false;
    }
    for (std::size_t index = 0U;
         index < surface_candidates.size();
         ++index) {
        if (!surface_candidates[index].valid()) {
            return false;
        }
        for (std::size_t other = index + 1U;
             other < surface_candidates.size();
             ++other) {
            if (surface_candidates[index] ==
                surface_candidates[other]) {
                return false;
            }
        }
    }

    switch (accounting_class) {
    case TopologyAccountingClass::referenceable:
        return !surface_candidates.empty();
    case TopologyAccountingClass::
        known_representation_artifact:
    case TopologyAccountingClass::
        semantically_unsupported:
        return !semantic_address.has_value() &&
               surface_candidates.empty();
    case TopologyAccountingClass::
        integrity_failure:
        return false;
    }
    return false;
}

bool BodyEdgeTopologyRecord::valid() const noexcept {
    return runtime_token.valid() &&
           accounting_class !=
               TopologyAccountingClass::
                   integrity_failure;
}

bool BodyVertexTopologyRecord::valid() const noexcept {
    return runtime_token.valid() &&
           accounting_class !=
               TopologyAccountingClass::
                   integrity_failure;
}

bool BodyStageTopologyCatalog::valid() const noexcept {
    if (!stage.valid()) {
        return false;
    }

    if (stage.kind ==
        BodyStageKind::empty_body) {
        return faces.empty() &&
               edges.empty() &&
               vertices.empty() &&
               surfaces.empty();
    }

    for (std::size_t index = 0U;
         index < faces.size();
         ++index) {
        if (!faces[index].valid()) {
            return false;
        }
        for (std::size_t other = index + 1U;
             other < faces.size();
             ++other) {
            if (faces[index].runtime_token ==
                faces[other].runtime_token) {
                return false;
            }
        }
    }
    for (std::size_t index = 0U;
         index < edges.size();
         ++index) {
        if (!edges[index].valid()) {
            return false;
        }
        for (std::size_t other = index + 1U;
             other < edges.size();
             ++other) {
            if (edges[index].runtime_token ==
                edges[other].runtime_token) {
                return false;
            }
        }
    }
    for (std::size_t index = 0U;
         index < vertices.size();
         ++index) {
        if (!vertices[index].valid()) {
            return false;
        }
        for (std::size_t other = index + 1U;
             other < vertices.size();
             ++other) {
            if (vertices[index].runtime_token ==
                vertices[other].runtime_token) {
                return false;
            }
        }
    }

    for (std::size_t index = 0U;
         index < surfaces.size();
         ++index) {
        if (!surfaces[index].valid()) {
            return false;
        }
        for (std::size_t other = index + 1U;
             other < surfaces.size();
             ++other) {
            if (surfaces[index].address ==
                surfaces[other].address) {
                return false;
            }
            if (surfaces[index].runtime_token &&
                surfaces[other].runtime_token &&
                surfaces[index].runtime_token ==
                    surfaces[other].runtime_token) {
                return false;
            }
        }
    }

    return true;
}

bool BodyStageTopologyCatalog::complete() const noexcept {
    return valid();
}

const FeatureEvaluation*
PartEvaluation::findFeature(
    FeatureId id) const noexcept {
    const auto found =
        std::find_if(
            features.begin(),
            features.end(),
            [id](const FeatureEvaluation& item) {
                return item.feature_id == id;
            });
    return found == features.end()
        ? nullptr
        : &*found;
}

PartEvaluation evaluatePart(
    const PartDocument& document,
    kernel::ISolidModelingKernel& modeling_kernel) {
    PartEvaluation result;
    result.source_revision =
        document.revision();
    result.features.reserve(
        document.body().features.size());

    kernel::RuntimeSolidHandle current_solid;
    std::optional<BodyStageTopologyCatalog>
        current_topology;
    std::vector<FeatureFaceResolution>
        current_references;
    std::vector<FeatureSurfaceResolution>
        current_surfaces;
    bool chain_broken = false;

    for (const auto& authored :
         document.body().features) {
        FeatureEvaluation evaluated;
        evaluated.feature_id = authored.id;

        if (authored.suppressed) {
            evaluated.status =
                FeatureEvaluationStatus::
                    suppressed;
            result.features.push_back(
                std::move(evaluated));
            continue;
        }

        if (chain_broken) {
            evaluated.status =
                FeatureEvaluationStatus::
                    blocked;
            evaluated.diagnostic =
                FeatureEvaluationDiagnosticCode::
                    upstream_unavailable;
            result.features.push_back(
                std::move(evaluated));
            continue;
        }

        const auto* extrude =
            std::get_if<ExtrudeFeature>(
                &authored.definition);
        if (extrude == nullptr) {
            evaluated.status =
                FeatureEvaluationStatus::
                    failed;
            evaluated.diagnostic =
                FeatureEvaluationDiagnosticCode::
                    kernel_invalid_input;
            chain_broken = true;
            // Keep the current-revision upstream result available only as a
            // presentation prefix. chain_broken prevents all later active
            // Features from consuming it as Body truth.
            current_references.clear();
            result.features.push_back(
                std::move(evaluated));
            continue;
        }

        const auto* profile =
            document.findProfile(
                extrude->profile_id);
        if (profile == nullptr) {
            evaluated.status =
                FeatureEvaluationStatus::
                    blocked;
            evaluated.diagnostic =
                FeatureEvaluationDiagnosticCode::
                    missing_profile;
            chain_broken = true;
            // Keep the current-revision upstream result available only as a
            // presentation prefix. chain_broken prevents all later active
            // Features from consuming it as Body truth.
            current_references.clear();
            result.features.push_back(
                std::move(evaluated));
            continue;
        }

        if (!document.evaluateProfile(
                 profile->id)
                 .value_or(
                     ResolvedProfileRegion{})
                 .valid()) {
            evaluated.status =
                FeatureEvaluationStatus::
                    blocked;
            evaluated.diagnostic =
                FeatureEvaluationDiagnosticCode::
                    unresolved_profile;
            chain_broken = true;
            // Keep the current-revision upstream result available only as a
            // presentation prefix. chain_broken prevents all later active
            // Features from consuming it as Body truth.
            current_references.clear();
            result.features.push_back(
                std::move(evaluated));
            continue;
        }

        if (extrude->operation ==
                ExtrudeOperation::cut &&
            current_solid == nullptr) {
            evaluated.status =
                FeatureEvaluationStatus::
                    blocked;
            evaluated.diagnostic =
                FeatureEvaluationDiagnosticCode::
                    missing_upstream_body;
            chain_broken = true;
            current_references.clear();
            result.features.push_back(
                std::move(evaluated));
            continue;
        }

        auto input =
            makeKernelExtrudeInput(
                document,
                *extrude);
        if (!input) {
            evaluated.status =
                FeatureEvaluationStatus::
                    failed;
            evaluated.diagnostic =
                FeatureEvaluationDiagnosticCode::
                    kernel_invalid_input;
            chain_broken = true;
            // Keep the current-revision upstream result available only as a
            // presentation prefix. chain_broken prevents all later active
            // Features from consuming it as Body truth.
            current_references.clear();
            result.features.push_back(
                std::move(evaluated));
            continue;
        }

        auto kernel_result =
            modeling_kernel.extrude(
                *input,
                current_solid);
        evaluated.kernel_status =
            kernel_result.status;
        if (!kernel_result.ok()) {
            evaluated.status =
                FeatureEvaluationStatus::
                    failed;
            evaluated.diagnostic =
                diagnosticForKernel(
                    kernel_result.status);
            chain_broken = true;
            // Keep the current-revision upstream result available only as a
            // presentation prefix. chain_broken prevents all later active
            // Features from consuming it as Body truth.
            current_references.clear();
            result.features.push_back(
                std::move(evaluated));
            continue;
        }

        auto candidate_references =
            current_references;
        propagateCurrentReferences(
            candidate_references,
            kernel_result.inherited_faces);

        evaluated.produced_faces.reserve(
            kernel_result.new_faces.size());
        for (const auto& face :
             kernel_result.new_faces) {
            auto converted =
                convertNewFace(
                    authored.id,
                    face);
            evaluated.produced_faces.push_back(
                converted);
            candidate_references.push_back(
                std::move(converted));
        }

        auto candidate_surfaces =
            current_surfaces;
        if (!propagateCurrentSurfaces(
                candidate_surfaces,
                kernel_result.inherited_surfaces)) {
            evaluated.status =
                FeatureEvaluationStatus::
                    failed;
            evaluated.diagnostic =
                FeatureEvaluationDiagnosticCode::
                    topology_integrity_failure;
            chain_broken = true;
            current_references.clear();
            current_surfaces.clear();
            result.features.push_back(
                std::move(evaluated));
            continue;
        }

        evaluated.produced_surfaces.reserve(
            kernel_result.new_surfaces.size());
        for (const auto& surface :
             kernel_result.new_surfaces) {
            auto converted =
                convertNewSurface(
                    authored.id,
                    surface);
            evaluated.produced_surfaces.push_back(
                converted);
            candidate_surfaces.push_back(
                std::move(converted));
        }

        auto candidate_topology =
            makeBodyStageTopologyCatalog(
                authored.id,
                kernel_result,
                candidate_references,
                candidate_surfaces);
        if (!candidate_topology) {
            evaluated.status =
                FeatureEvaluationStatus::
                    failed;
            evaluated.diagnostic =
                FeatureEvaluationDiagnosticCode::
                    topology_integrity_failure;
            chain_broken = true;
            current_references.clear();
            current_surfaces.clear();
            result.features.push_back(
                std::move(evaluated));
            continue;
        }

        evaluated.status =
            FeatureEvaluationStatus::
                up_to_date;
        evaluated.diagnostic =
            FeatureEvaluationDiagnosticCode::
                none;
        evaluated.result_solid =
            kernel_result.solid;
        evaluated.result_topology =
            *candidate_topology;
        current_references =
            std::move(candidate_references);
        current_surfaces =
            std::move(candidate_surfaces);
        current_topology =
            std::move(candidate_topology);
        current_solid =
            std::move(kernel_result.solid);
        result.features.push_back(
            std::move(evaluated));
    }

    if (chain_broken) {
        result.body_status =
            BodyEvaluationStatus::
                unavailable;
        result.body_solid.reset();
        result.resolved_prefix_solid =
            std::move(current_solid);
        result.resolved_prefix_topology =
            std::move(current_topology);
        result.current_topology.reset();
        result.current_face_references.clear();
        return result;
    }

    if (current_solid != nullptr) {
        result.body_status =
            BodyEvaluationStatus::
                up_to_date;
        result.body_solid =
            std::move(current_solid);
        result.current_topology =
            std::move(current_topology);
        result.current_face_references =
            std::move(current_references);
        result.current_surface_references =
            std::move(current_surfaces);
        return result;
    }

    result.body_status =
        BodyEvaluationStatus::empty;
    return result;
}

} // namespace simplesolid2::part
