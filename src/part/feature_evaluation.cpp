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

} // namespace

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
    std::vector<FeatureFaceResolution>
        current_references;
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
            current_solid.reset();
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
            current_solid.reset();
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
            current_solid.reset();
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
            current_solid.reset();
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
            current_solid.reset();
            current_references.clear();
            result.features.push_back(
                std::move(evaluated));
            continue;
        }

        propagateCurrentReferences(
            current_references,
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
            current_references.push_back(
                std::move(converted));
        }

        evaluated.status =
            FeatureEvaluationStatus::
                up_to_date;
        evaluated.diagnostic =
            FeatureEvaluationDiagnosticCode::
                none;
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
        result.current_face_references.clear();
        return result;
    }

    if (current_solid != nullptr) {
        result.body_status =
            BodyEvaluationStatus::
                up_to_date;
        result.body_solid =
            std::move(current_solid);
        result.current_face_references =
            std::move(current_references);
        return result;
    }

    result.body_status =
        BodyEvaluationStatus::empty;
    return result;
}

} // namespace simplesolid2::part
