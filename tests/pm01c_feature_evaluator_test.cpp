#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/part/profile.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-01C evaluator CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

template <typename Token>
void appendUniqueRuntimeToken(
    std::vector<Token>& tokens,
    Token token) {
    if (std::find(
            tokens.begin(),
            tokens.end(),
            token) ==
        tokens.end()) {
        tokens.push_back(token);
    }
}

class FakeSolid final
    : public kernel::RuntimeSolid {
public:
    struct Edge final {
        kernel::RuntimeEdgeToken token;
        std::vector<kernel::RuntimeSurfaceToken>
            surfaces;
        kernel::CurveKind kind{
            kernel::CurveKind::line};
        bool seam{false};
    };

    struct Vertex final {
        kernel::RuntimeVertexToken token;
        std::vector<kernel::RuntimeSurfaceToken>
            surfaces;
        std::vector<kernel::RuntimeEdgeToken>
            incident_edges;
        kernel::Point3 point;
    };

    std::vector<kernel::RuntimeFaceToken>
        tokens;
    std::vector<kernel::RuntimeSurfaceToken>
        surface_tokens;
    std::vector<Edge> edges;
    std::vector<Vertex> vertices;
    std::uint64_t next_token{1U};
    std::uint64_t next_surface_token{1U};
    std::uint64_t next_edge_token{1U};
    std::uint64_t next_vertex_token{1U};
};

class FakeKernel final
    : public kernel::ISolidModelingKernel {
public:
    bool saw_reverse{false};
    bool saw_midplane{false};
    bool corrupt_topology_inventory{false};
    bool split_first_inherited_surface{false};
    bool alias_first_two_inherited_surfaces{false};
    bool first_survives_second_missing{false};
    bool emit_semantic_subshapes{false};
    bool split_first_inherited_edge{false};
    bool replace_first_inherited_edge{false};
    bool replace_first_inherited_vertex_same_point{false};
    bool duplicate_first_new_edge_branch{false};

    kernel::SolidModelingResult extrude(
        const kernel::LinearExtrudeInput& input,
        kernel::RuntimeSolidHandle upstream) noexcept override {
        kernel::SolidModelingResult result;
        if (!input.valid()) {
            result.status =
                kernel::SolidModelingStatus::
                    invalid_input;
            return result;
        }
        if (input.operation ==
                kernel::SolidBooleanOperation::cut &&
            upstream == nullptr) {
            result.status =
                kernel::SolidModelingStatus::
                    missing_upstream;
            return result;
        }

        if (input.start_offset_mm < 0.0 &&
            input.end_offset_mm == 0.0 &&
            input.start_cap_role ==
                kernel::ExtrudeCapRole::
                    extent_cap &&
            input.end_cap_role ==
                kernel::ExtrudeCapRole::
                    profile_cap) {
            saw_reverse = true;
        }
        if (input.start_offset_mm < 0.0 &&
            input.end_offset_mm > 0.0 &&
            input.start_cap_role ==
                kernel::ExtrudeCapRole::
                    negative_cap &&
            input.end_cap_role ==
                kernel::ExtrudeCapRole::
                    positive_cap) {
            saw_midplane = true;
        }

        const double span =
            input.end_offset_mm -
            input.start_offset_mm;
        if (span == 99.0) {
            result.status =
                kernel::SolidModelingStatus::
                    no_effect;
            return result;
        }

        auto runtime =
            std::make_shared<FakeSolid>();
        std::vector<kernel::RuntimeFaceToken>
            inventory_faces;
        if (upstream) {
            const auto* existing =
                dynamic_cast<
                    const FakeSolid*>(
                    upstream.get());
            if (!existing) {
                result.status =
                    kernel::SolidModelingStatus::
                        provider_mismatch;
                return result;
            }
            runtime->next_token =
                existing->next_token;
            runtime->next_surface_token =
                existing->next_surface_token;
            runtime->next_edge_token =
                existing->next_edge_token;
            runtime->next_vertex_token =
                existing->next_vertex_token;
            CHECK(
                existing->tokens.size() ==
                existing->surface_tokens.size());

            std::optional<kernel::RuntimeFaceToken>
                alias_face;
            if (alias_first_two_inherited_surfaces) {
                CHECK(existing->tokens.size() >= 2U);
                alias_face =
                    kernel::RuntimeFaceToken{
                        runtime->next_token++};
                inventory_faces.push_back(
                    *alias_face);
            }

            for (std::size_t index = 0U;
                 index < existing->tokens.size();
                 ++index) {
                const auto face_token =
                    existing->tokens[index];
                const auto surface_token =
                    existing->surface_tokens[index];

                if (alias_first_two_inherited_surfaces &&
                    index < 2U) {
                    CHECK(alias_face.has_value());
                    result.inherited_faces.push_back(
                        {
                            face_token,
                            kernel::ReferenceStatus::
                                ambiguous,
                            1U,
                        });
                    result.inherited_surfaces.push_back(
                        {
                            surface_token,
                            kernel::ReferenceStatus::
                                ambiguous,
                            kernel::ReferenceStatus::
                                ambiguous,
                            1U,
                            kernel::SurfaceKind::plane,
                            std::nullopt,
                            {*alias_face},
                        });
                    continue;
                }

                if (first_survives_second_missing &&
                    index == 1U) {
                    result.inherited_faces.push_back(
                        {
                            face_token,
                            kernel::ReferenceStatus::
                                missing,
                            0U,
                        });
                    result.inherited_surfaces.push_back(
                        {
                            surface_token,
                            kernel::ReferenceStatus::
                                missing,
                            kernel::ReferenceStatus::
                                missing,
                            0U,
                            kernel::SurfaceKind::plane,
                            std::nullopt,
                            {},
                        });
                    continue;
                }

                if (split_first_inherited_surface &&
                    index == 0U) {
                    const kernel::RuntimeFaceToken
                        first_fragment{
                            runtime->next_token++};
                    const kernel::RuntimeFaceToken
                        second_fragment{
                            runtime->next_token++};
                    inventory_faces.push_back(
                        first_fragment);
                    inventory_faces.push_back(
                        second_fragment);

                    result.inherited_faces.push_back(
                        {
                            face_token,
                            kernel::ReferenceStatus::
                                ambiguous,
                            2U,
                        });
                    result.inherited_surfaces.push_back(
                        {
                            surface_token,
                            kernel::ReferenceStatus::
                                resolved,
                            kernel::ReferenceStatus::
                                ambiguous,
                            2U,
                            kernel::SurfaceKind::plane,
                            kernel::Frame3{},
                            {
                                first_fragment,
                                second_fragment,
                            },
                        });
                    continue;
                }

                runtime->tokens.push_back(
                    face_token);
                runtime->surface_tokens.push_back(
                    surface_token);
                inventory_faces.push_back(
                    face_token);
                result.inherited_faces.push_back(
                    {
                        face_token,
                        kernel::ReferenceStatus::
                            resolved,
                        1U,
                    });
                result.inherited_surfaces.push_back(
                    {
                        surface_token,
                        kernel::ReferenceStatus::
                            resolved,
                        kernel::ReferenceStatus::
                            resolved,
                        1U,
                        kernel::SurfaceKind::plane,
                        kernel::Frame3{},
                        {face_token},
                    });
            }
        }

        auto publish =
            [&result, &runtime, &inventory_faces](
                kernel::ExtrudeFaceRole role) {
                const kernel::RuntimeFaceToken face_token{
                    runtime->next_token++};
                const kernel::RuntimeSurfaceToken
                    surface_token{
                        runtime->next_surface_token++};
                runtime->tokens.push_back(face_token);
                runtime->surface_tokens.push_back(
                    surface_token);
                inventory_faces.push_back(
                    face_token);
                result.new_faces.push_back(
                    {
                        role,
                        kernel::ReferenceStatus::
                            resolved,
                        1U,
                        face_token,
                    });
                result.new_surfaces.push_back(
                    {
                        std::move(role),
                        kernel::ReferenceStatus::
                            resolved,
                        kernel::ReferenceStatus::
                            resolved,
                        1U,
                        kernel::SurfaceKind::plane,
                        kernel::Frame3{},
                        surface_token,
                        {face_token},
                    });
            };

        publish(
            {
                kernel::ExtrudeGeneratedFaceRoleKind::cap,
                input.start_cap_role,
                std::nullopt,
            });
        publish(
            {
                kernel::ExtrudeGeneratedFaceRoleKind::cap,
                input.end_cap_role,
                std::nullopt,
            });
        for (const auto& use :
             input.profile.outer.boundary) {
            publish(
                {
                    kernel::ExtrudeGeneratedFaceRoleKind::
                        side,
                    std::nullopt,
                    use.provenance,
                });
        }

        if (emit_semantic_subshapes) {
            const FakeSolid* existing =
                upstream
                    ? dynamic_cast<
                          const FakeSolid*>(
                          upstream.get())
                    : nullptr;
            if (upstream && existing == nullptr) {
                result.status =
                    kernel::SolidModelingStatus::
                        provider_mismatch;
                return result;
            }

            std::map<std::uint64_t,
                     kernel::RuntimeEdgeToken>
                edge_descendant;

            std::optional<FakeSolid::Edge>
                replacement_edge;
            if (existing != nullptr) {
                for (std::size_t index = 0U;
                     index < existing->edges.size();
                     ++index) {
                    const auto& source =
                        existing->edges[index];

                    if (replace_first_inherited_edge &&
                        index == 0U) {
                        result.inherited_edge_realizations
                            .push_back(
                                {
                                    source.token,
                                    kernel::ReferenceStatus::
                                        missing,
                                    0U,
                                    {},
                                });
                        replacement_edge = source;
                        continue;
                    }

                    if (split_first_inherited_edge &&
                        index == 0U) {
                        const kernel::RuntimeEdgeToken
                            first{
                                runtime->next_edge_token++};
                        const kernel::RuntimeEdgeToken
                            second{
                                runtime->next_edge_token++};
                        runtime->edges.push_back(
                            {
                                first,
                                source.surfaces,
                                source.kind,
                                source.seam,
                            });
                        runtime->edges.push_back(
                            {
                                second,
                                source.surfaces,
                                source.kind,
                                source.seam,
                            });
                        edge_descendant.emplace(
                            source.token.value,
                            first);
                        result.inherited_edge_realizations
                            .push_back(
                                {
                                    source.token,
                                    kernel::ReferenceStatus::
                                        ambiguous,
                                    2U,
                                    {first, second},
                                });
                        continue;
                    }

                    const kernel::RuntimeEdgeToken token{
                        runtime->next_edge_token++};
                    runtime->edges.push_back(
                        {
                            token,
                            source.surfaces,
                            source.kind,
                            source.seam,
                        });
                    edge_descendant.emplace(
                        source.token.value,
                        token);
                    result.inherited_edge_realizations
                        .push_back(
                            {
                                source.token,
                                kernel::ReferenceStatus::
                                    resolved,
                                1U,
                                {token},
                            });
                }
            }

            const auto add_edge =
                [&runtime](
                    kernel::RuntimeSurfaceToken first,
                    kernel::RuntimeSurfaceToken second) {
                    const kernel::RuntimeEdgeToken token{
                        runtime->next_edge_token++};
                    runtime->edges.push_back(
                        {
                            token,
                            {first, second},
                            kernel::CurveKind::line,
                            false,
                        });
                    return token;
                };

            CHECK(result.new_surfaces.size() == 6U);
            std::vector<kernel::RuntimeSurfaceToken>
                feature_surfaces;
            feature_surfaces.reserve(6U);
            for (const auto& surface :
                 result.new_surfaces) {
                CHECK(surface.resolved_token.has_value());
                feature_surfaces.push_back(
                    *surface.resolved_token);
            }

            const auto s0 = feature_surfaces[0];
            const auto s1 = feature_surfaces[1];
            const auto s2 = feature_surfaces[2];
            const auto s3 = feature_surfaces[3];
            const auto s4 = feature_surfaces[4];
            const auto s5 = feature_surfaces[5];

            std::vector<kernel::RuntimeEdgeToken>
                new_edges = {
                    add_edge(s0, s2),
                    add_edge(s0, s3),
                    add_edge(s0, s4),
                    add_edge(s0, s5),
                    add_edge(s1, s2),
                    add_edge(s1, s3),
                    add_edge(s1, s4),
                    add_edge(s1, s5),
                    add_edge(s2, s3),
                    add_edge(s3, s4),
                    add_edge(s4, s5),
                    add_edge(s5, s2),
                };

            if (duplicate_first_new_edge_branch) {
                const auto& first =
                    runtime->edges[
                        runtime->edges.size() -
                        new_edges.size()];
                add_edge(
                    first.surfaces[0],
                    first.surfaces[1]);
            }

            if (replacement_edge) {
                const kernel::RuntimeEdgeToken token{
                    runtime->next_edge_token++};
                edge_descendant.emplace(
                    replacement_edge->token.value,
                    token);
                runtime->edges.push_back(
                    {
                        token,
                        replacement_edge->surfaces,
                        replacement_edge->kind,
                        replacement_edge->seam,
                    });
            }

            std::optional<FakeSolid::Vertex>
                replacement_vertex;
            if (existing != nullptr) {
                for (std::size_t index = 0U;
                     index < existing->vertices.size();
                     ++index) {
                    const auto& source =
                        existing->vertices[index];
                    if (replace_first_inherited_vertex_same_point &&
                        index == 0U) {
                        result.inherited_vertex_realizations
                            .push_back(
                                {
                                    source.token,
                                    kernel::ReferenceStatus::
                                        missing,
                                    0U,
                                    {},
                                });
                        replacement_vertex = source;
                        continue;
                    }

                    const kernel::RuntimeVertexToken token{
                        runtime->next_vertex_token++};
                    std::vector<kernel::RuntimeEdgeToken>
                        incident;
                    for (const auto old_edge :
                         source.incident_edges) {
                        const auto mapped =
                            edge_descendant.find(
                                old_edge.value);
                        if (mapped !=
                            edge_descendant.end()) {
                            appendUniqueRuntimeToken(
                                incident,
                                mapped->second);
                        }
                    }
                    runtime->vertices.push_back(
                        {
                            token,
                            source.surfaces,
                            std::move(incident),
                            source.point,
                        });
                    result.inherited_vertex_realizations
                        .push_back(
                            {
                                source.token,
                                kernel::ReferenceStatus::
                                    resolved,
                                1U,
                                {token},
                            });
                }
            }

            const auto add_vertex =
                [&runtime](
                    std::vector<kernel::RuntimeSurfaceToken>
                        surfaces,
                    std::vector<kernel::RuntimeEdgeToken>
                        edges,
                    kernel::Point3 point) {
                    const kernel::RuntimeVertexToken token{
                        runtime->next_vertex_token++};
                    runtime->vertices.push_back(
                        {
                            token,
                            std::move(surfaces),
                            std::move(edges),
                            point,
                        });
                    return token;
                };

            const double z0 = input.start_offset_mm;
            const double z1 = input.end_offset_mm;
            add_vertex(
                {s0, s2, s5},
                {new_edges[0], new_edges[3], new_edges[11]},
                {0.0, 0.0, z0});
            add_vertex(
                {s0, s2, s3},
                {new_edges[0], new_edges[1], new_edges[8]},
                {1.0, 0.0, z0});
            add_vertex(
                {s0, s3, s4},
                {new_edges[1], new_edges[2], new_edges[9]},
                {1.0, 1.0, z0});
            add_vertex(
                {s0, s4, s5},
                {new_edges[2], new_edges[3], new_edges[10]},
                {0.0, 1.0, z0});
            add_vertex(
                {s1, s2, s5},
                {new_edges[4], new_edges[7], new_edges[11]},
                {0.0, 0.0, z1});
            add_vertex(
                {s1, s2, s3},
                {new_edges[4], new_edges[5], new_edges[8]},
                {1.0, 0.0, z1});
            add_vertex(
                {s1, s3, s4},
                {new_edges[5], new_edges[6], new_edges[9]},
                {1.0, 1.0, z1});
            add_vertex(
                {s1, s4, s5},
                {new_edges[6], new_edges[7], new_edges[10]},
                {0.0, 1.0, z1});

            if (replacement_vertex) {
                const kernel::RuntimeVertexToken token{
                    runtime->next_vertex_token++};
                std::vector<kernel::RuntimeEdgeToken>
                    incident;
                for (const auto old_edge :
                     replacement_vertex->incident_edges) {
                    const auto mapped =
                        edge_descendant.find(
                            old_edge.value);
                    if (mapped !=
                        edge_descendant.end()) {
                        appendUniqueRuntimeToken(
                            incident,
                            mapped->second);
                    }
                }
                runtime->vertices.push_back(
                    {
                        token,
                        replacement_vertex->surfaces,
                        std::move(incident),
                        // Deliberately exact same XYZ. Semantic provenance,
                        // not coordinates, decides replacement identity.
                        replacement_vertex->point,
                    });
            }

            result.current_edges.reserve(
                runtime->edges.size());
            result.current_edge_semantics.reserve(
                runtime->edges.size());
            for (const auto& edge :
                 runtime->edges) {
                result.current_edges.push_back(
                    edge.token);
                result.current_edge_semantics.push_back(
                    {
                        edge.token,
                        edge.kind,
                        edge.seam,
                        edge.surfaces,
                    });
            }

            result.current_vertices.reserve(
                runtime->vertices.size());
            result.current_vertex_semantics.reserve(
                runtime->vertices.size());
            for (const auto& vertex :
                 runtime->vertices) {
                result.current_vertices.push_back(
                    vertex.token);
                result.current_vertex_semantics.push_back(
                    {
                        vertex.token,
                        vertex.surfaces,
                        vertex.incident_edges,
                        vertex.point,
                    });
            }

            result.edge_count =
                result.current_edges.size();
            result.vertex_count =
                result.current_vertices.size();
        }

        result.status =
            kernel::SolidModelingStatus::ok;
        result.brep_valid = true;
        result.solid_count = 1U;
        result.current_faces =
            std::move(inventory_faces);
        result.face_count =
            result.current_faces.size();
        result.edge_count =
            result.current_edges.size();
        result.vertex_count =
            result.current_vertices.size();
        if (corrupt_topology_inventory &&
            !result.current_faces.empty()) {
            result.current_faces.pop_back();
        }
        result.solid =
            std::move(runtime);
        return result;
    }
};

struct Fixture final {
    part::PartDocument document;
    part::ProfileId profile_id;

    Fixture(
        part::PartDocument&& source,
        part::ProfileId profile)
        : document{std::move(source)},
          profile_id{profile} {}
};

Fixture makeFixture() {
    auto document =
        part::PartDocument::create(
            core::DocumentId::generate());
    application::DocumentSession session{
        {},
        std::move(document)};

    const auto sketch_created =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::
                    xy_plane});
    CHECK(
        sketch_created.ok() &&
        sketch_created.sketch_id);
    const auto sketch_id =
        *sketch_created.sketch_id;

    const auto rectangle =
        session.execute(
            application::AddSketchRectangleCommand{
                sketch_id,
                session.document().revision(),
                {0.0, 0.0},
                {40.0, 30.0},
                sketch::EntityRole::regular,
                false});
    CHECK(rectangle.ok());
    CHECK(rectangle.entity_ids.size() == 4U);

    const auto* sketch =
        session.document().findSketch(
            sketch_id);
    CHECK(sketch != nullptr);
    const auto analysis =
        sketch::analyzeRegions(
            sketch->model);
    CHECK(analysis.complete());
    CHECK(analysis.regions.size() == 1U);
    const auto intent =
        part::makeProfileRegionIntent(
            analysis.regions.front());
    CHECK(intent.has_value());

    const auto profile =
        session.execute(
            application::CreateProfileCommand{
                sketch_id,
                session.document().revision(),
                *intent});
    CHECK(
        profile.ok() &&
        profile.profile_id);

    auto restored =
        part::PartDocument::restore(
            session.document().documentId(),
            session.document().state(),
            session.document().revision());
    CHECK(restored.ok());
    return Fixture{
        std::move(*restored.document),
        *profile.profile_id};
}

part::PartDocument withFeatures(
    const part::PartDocument& source,
    std::vector<part::PartFeature> features,
    std::optional<part::FeatureIdCursor>
        cursor = std::nullopt) {
    auto state = source.state();
    if (cursor) {
        state.body.next_feature_id =
            *cursor;
    }
    state.body.features =
        std::move(features);
    auto restored =
        part::PartDocument::restore(
            source.documentId(),
            std::move(state),
            source.revision());
    CHECK(restored.ok());
    return std::move(*restored.document);
}

part::PartFeature featureWithExtent(
    part::FeatureId id,
    part::ProfileId profile,
    part::ExtrudeOperation operation,
    part::ExtrudeExtent extent,
    bool suppressed = false) {
    return {
        id,
        "Extrude" + id.serialized(),
        suppressed,
        part::ExtrudeFeature{
            profile,
            operation,
            std::move(extent)},
    };
}

part::PartFeature feature(
    part::FeatureId id,
    part::ProfileId profile,
    part::ExtrudeOperation operation,
    double distance,
    bool suppressed = false) {
    return featureWithExtent(
        id,
        profile,
        operation,
        part::OneSidedExtrudeExtent{
            core::LengthValue{distance},
            false},
        suppressed);
}

} // namespace

int main() {
    FakeKernel kernel;
    auto fixture = makeFixture();

    const auto cursor =
        *part::FeatureIdCursor::parse("4");
    const auto id1 =
        *part::FeatureId::parse("1");
    const auto id2 =
        *part::FeatureId::parse("2");
    const auto id3 =
        *part::FeatureId::parse("3");

    // Add -> Cut is a valid ordered one-Body chain.
    auto valid =
        withFeatures(
            fixture.document,
            {
                feature(
                    id1,
                    fixture.profile_id,
                    part::ExtrudeOperation::add,
                    10.0),
                feature(
                    id2,
                    fixture.profile_id,
                    part::ExtrudeOperation::cut,
                    5.0),
            },
            cursor);
    const auto valid_eval =
        part::evaluatePart(
            valid,
            kernel);
    CHECK(
        valid_eval.body_status ==
        part::BodyEvaluationStatus::
            up_to_date);
    CHECK(valid_eval.body_solid != nullptr);
    CHECK(valid_eval.features.size() == 2U);
    CHECK(
        valid_eval.features[0].status ==
        part::FeatureEvaluationStatus::
            up_to_date);
    CHECK(
        valid_eval.features[1].status ==
        part::FeatureEvaluationStatus::
            up_to_date);
    CHECK(
        valid_eval.current_face_references.size() ==
        12U);
    CHECK(valid_eval.current_topology.has_value());
    CHECK(
        valid_eval.current_topology->stage.kind ==
        part::BodyStageKind::after_feature);
    CHECK(
        valid_eval.current_topology->stage.feature_id ==
        std::optional<part::FeatureId>{id2});
    CHECK(valid_eval.current_topology->faces.size() == 12U);
    CHECK(valid_eval.current_topology->surfaces.size() == 12U);
    CHECK(valid_eval.current_surface_references.size() == 12U);
    CHECK(valid_eval.current_topology->edges.empty());
    CHECK(valid_eval.current_topology->vertices.empty());
    CHECK(valid_eval.features[0].result_solid != nullptr);
    CHECK(valid_eval.features[0].result_topology.has_value());
    CHECK(
        valid_eval.features[0].result_topology
            ->stage.feature_id ==
        std::optional<part::FeatureId>{id1});
    CHECK(valid_eval.features[0].result_topology->faces.size() == 6U);
    CHECK(valid_eval.features[1].result_solid != nullptr);
    CHECK(valid_eval.features[1].result_topology.has_value());
    for (const auto& face :
         valid_eval.current_topology->faces) {
        CHECK(face.valid());
        CHECK(
            face.accounting_class ==
            part::TopologyAccountingClass::
                referenceable);
        CHECK(face.semantic_address.has_value());
    }
    for (const auto& surface :
         valid_eval.current_surface_references) {
        CHECK(surface.valid());
        CHECK(
            surface.status ==
            kernel::ReferenceStatus::resolved);
        CHECK(
            surface.strict_face_status ==
            kernel::ReferenceStatus::resolved);
        CHECK(surface.runtime_token.has_value());
        CHECK(surface.current_faces.size() == 1U);
        CHECK(
            surface.surface_kind ==
            kernel::SurfaceKind::plane);
        CHECK(surface.canonical_frame.has_value());
    }
    for (const auto& reference :
         valid_eval.current_face_references) {
        CHECK(reference.address.valid());
        CHECK(
            reference.status ==
            kernel::ReferenceStatus::resolved);
        CHECK(reference.runtime_token);
    }

    // PM-02B semantic split: one strict bounded Face becomes two current
    // realizations while its semantic Surface remains singularly Resolved.
    kernel.split_first_inherited_surface = true;
    const auto split_eval =
        part::evaluatePart(
            valid,
            kernel);
    kernel.split_first_inherited_surface = false;

    CHECK(
        split_eval.body_status ==
        part::BodyEvaluationStatus::
            up_to_date);
    CHECK(split_eval.current_topology.has_value());

    const part::FeatureSurfaceAddress
        split_surface_address{
            id1,
            part::FeatureSurfaceRoleKind::
                profile_cap,
            std::nullopt,
            0U,
            0U,
            false};
    const auto split_surface =
        std::find_if(
            split_eval.current_surface_references.begin(),
            split_eval.current_surface_references.end(),
            [&split_surface_address](const auto& surface) {
                return surface.address ==
                       split_surface_address;
            });
    CHECK(
        split_surface !=
        split_eval.current_surface_references.end());
    CHECK(split_surface->valid());
    CHECK(
        split_surface->status ==
        kernel::ReferenceStatus::resolved);
    CHECK(
        split_surface->strict_face_status ==
        kernel::ReferenceStatus::ambiguous);
    CHECK(split_surface->candidate_face_count == 2U);
    CHECK(split_surface->current_faces.size() == 2U);

    const part::FeatureFaceAddress
        split_face_address{
            id1,
            part::FeatureFaceRoleKind::
                profile_cap,
            std::nullopt,
            0U,
            0U,
            false};
    const auto split_face =
        std::find_if(
            split_eval.current_face_references.begin(),
            split_eval.current_face_references.end(),
            [&split_face_address](const auto& face) {
                return face.address ==
                       split_face_address;
            });
    CHECK(
        split_face !=
        split_eval.current_face_references.end());
    CHECK(
        split_face->status ==
        kernel::ReferenceStatus::ambiguous);
    CHECK(split_face->candidate_count == 2U);
    CHECK(!split_face->runtime_token.has_value());

    const auto fragment_count =
        static_cast<std::size_t>(
            std::count_if(
                split_eval.current_topology->faces.begin(),
                split_eval.current_topology->faces.end(),
                [&split_surface_address](const auto& face) {
                    return !face.semantic_address &&
                           std::find(
                               face.surface_candidates.begin(),
                               face.surface_candidates.end(),
                               split_surface_address) !=
                               face.surface_candidates.end();
                }));
    CHECK(fragment_count == 2U);

    // Two prior semantic Surface claims collapse onto one current provider
    // Face with no independent semantic winner. Both remain Ambiguous; Part
    // exposes both carrier candidates on the one current Face and no frame.
    kernel.alias_first_two_inherited_surfaces = true;
    const auto alias_eval =
        part::evaluatePart(
            valid,
            kernel);
    kernel.alias_first_two_inherited_surfaces = false;
    CHECK(
        alias_eval.body_status ==
        part::BodyEvaluationStatus::
            up_to_date);
    CHECK(alias_eval.current_topology.has_value());

    const part::FeatureSurfaceAddress
        first_alias_address{
            id1,
            part::FeatureSurfaceRoleKind::
                profile_cap,
            std::nullopt,
            0U,
            0U,
            false};
    const part::FeatureSurfaceAddress
        second_alias_address{
            id1,
            part::FeatureSurfaceRoleKind::
                extent_cap,
            std::nullopt,
            0U,
            0U,
            false};

    const auto find_surface =
        [](const part::PartEvaluation& evaluation,
           const part::FeatureSurfaceAddress& address) {
            return std::find_if(
                evaluation
                    .current_surface_references.begin(),
                evaluation
                    .current_surface_references.end(),
                [&address](const auto& surface) {
                    return surface.address == address;
                });
        };

    const auto first_alias =
        find_surface(
            alias_eval,
            first_alias_address);
    const auto second_alias =
        find_surface(
            alias_eval,
            second_alias_address);
    CHECK(
        first_alias !=
        alias_eval.current_surface_references.end());
    CHECK(
        second_alias !=
        alias_eval.current_surface_references.end());
    CHECK(first_alias->valid());
    CHECK(second_alias->valid());
    CHECK(
        first_alias->status ==
        kernel::ReferenceStatus::ambiguous);
    CHECK(
        second_alias->status ==
        kernel::ReferenceStatus::ambiguous);
    CHECK(!first_alias->canonical_frame.has_value());
    CHECK(!second_alias->canonical_frame.has_value());
    CHECK(first_alias->current_faces.size() == 1U);
    CHECK(second_alias->current_faces.size() == 1U);
    CHECK(
        first_alias->current_faces.front() ==
        second_alias->current_faces.front());

    const auto shared_alias_face =
        std::find_if(
            alias_eval.current_topology->faces.begin(),
            alias_eval.current_topology->faces.end(),
            [&first_alias_address,
             &second_alias_address](const auto& face) {
                return
                    std::find(
                        face.surface_candidates.begin(),
                        face.surface_candidates.end(),
                        first_alias_address) !=
                        face.surface_candidates.end() &&
                    std::find(
                        face.surface_candidates.begin(),
                        face.surface_candidates.end(),
                        second_alias_address) !=
                        face.surface_candidates.end();
            });
    CHECK(
        shared_alias_face !=
        alias_eval.current_topology->faces.end());
    CHECK(!shared_alias_face->semantic_address.has_value());
    CHECK(shared_alias_face->surface_candidates.size() == 2U);

    // If independent provenance says one semantic claim survives and the
    // other is deleted, no provider-history asymmetry invents a second
    // winner: survivor stays Resolved, removed meaning is Missing/no frame.
    kernel.first_survives_second_missing = true;
    const auto winner_eval =
        part::evaluatePart(
            valid,
            kernel);
    kernel.first_survives_second_missing = false;
    CHECK(
        winner_eval.body_status ==
        part::BodyEvaluationStatus::
            up_to_date);

    const auto survivor =
        find_surface(
            winner_eval,
            first_alias_address);
    const auto removed =
        find_surface(
            winner_eval,
            second_alias_address);
    CHECK(
        survivor !=
        winner_eval.current_surface_references.end());
    CHECK(
        removed !=
        winner_eval.current_surface_references.end());
    CHECK(survivor->valid());
    CHECK(removed->valid());
    CHECK(
        survivor->status ==
        kernel::ReferenceStatus::resolved);
    CHECK(survivor->canonical_frame.has_value());
    CHECK(
        removed->status ==
        kernel::ReferenceStatus::missing);
    CHECK(
        removed->strict_face_status ==
        kernel::ReferenceStatus::missing);
    CHECK(removed->candidate_face_count == 0U);
    CHECK(removed->current_faces.empty());
    CHECK(!removed->runtime_token.has_value());
    CHECK(!removed->canonical_frame.has_value());

    // PM-02C production semantics in the provider-neutral evaluator:
    // complete material Edge/Vertex promotion, Curve carrier continuity across
    // strict Edge split, multi-branch ambiguity, and Point identity that never
    // falls back to XYZ.
    kernel.emit_semantic_subshapes = true;

    auto semantic_base =
        withFeatures(
            fixture.document,
            {
                feature(
                    id1,
                    fixture.profile_id,
                    part::ExtrudeOperation::add,
                    10.0),
            },
            cursor);
    const auto semantic_base_eval =
        part::evaluatePart(
            semantic_base,
            kernel);
    CHECK(
        semantic_base_eval.body_status ==
        part::BodyEvaluationStatus::up_to_date);
    CHECK(semantic_base_eval.current_topology.has_value());
    CHECK(
        semantic_base_eval.current_topology->edges.size() ==
        12U);
    CHECK(
        semantic_base_eval.current_topology->vertices.size() ==
        8U);
    CHECK(
        semantic_base_eval.current_curve_references.size() ==
        12U);
    CHECK(
        semantic_base_eval.current_point_references.size() ==
        8U);
    for (const auto& curve :
         semantic_base_eval.current_curve_references) {
        CHECK(curve.valid());
        CHECK(
            curve.status ==
            kernel::ReferenceStatus::resolved);
        CHECK(
            curve.strict_edge_status ==
            kernel::ReferenceStatus::resolved);
        CHECK(curve.candidate_edge_count == 1U);
        CHECK(
            curve.curve_kind ==
            kernel::CurveKind::line);
    }
    for (const auto& point :
         semantic_base_eval.current_point_references) {
        CHECK(point.valid());
        CHECK(
            point.status ==
            kernel::ReferenceStatus::resolved);
        CHECK(point.candidate_vertex_count == 1U);
        CHECK(point.address.adjacent_surfaces.size() == 3U);
    }
    for (const auto& edge :
         semantic_base_eval.current_topology->edges) {
        CHECK(edge.valid());
        CHECK(
            edge.accounting_class ==
            part::TopologyAccountingClass::referenceable);
        CHECK(
            edge.referenceability ==
            kernel::ReferenceStatus::resolved);
        CHECK(!edge.periodic_seam);
        CHECK(edge.curve_candidates.size() == 1U);
    }
    for (const auto& vertex :
         semantic_base_eval.current_topology->vertices) {
        CHECK(vertex.valid());
        CHECK(
            vertex.accounting_class ==
            part::TopologyAccountingClass::referenceable);
        CHECK(
            vertex.referenceability ==
            kernel::ReferenceStatus::resolved);
        CHECK(vertex.point_candidates.size() == 1U);
        CHECK(vertex.incident_material_edge_count == 3U);
        CHECK(vertex.provider_point.has_value());
    }

    // One previously singular bounded Edge splits into two provider
    // descendants. Explicit lineage keeps one semantic Curve carrier
    // Resolved while the strict Edge selector becomes Ambiguous.
    kernel.split_first_inherited_edge = true;
    const auto split_edge_eval =
        part::evaluatePart(
            valid,
            kernel);
    kernel.split_first_inherited_edge = false;
    CHECK(
        split_edge_eval.body_status ==
        part::BodyEvaluationStatus::up_to_date);
    const auto split_curve_count =
        static_cast<std::size_t>(
            std::count_if(
                split_edge_eval.current_curve_references.begin(),
                split_edge_eval.current_curve_references.end(),
                [id1](const auto& curve) {
                    return curve.address
                               .producer_feature_id == id1 &&
                           curve.status ==
                               kernel::ReferenceStatus::
                                   resolved &&
                           curve.strict_edge_status ==
                               kernel::ReferenceStatus::
                                   ambiguous &&
                           curve.candidate_edge_count == 2U &&
                           curve.current_edges.size() == 2U;
                }));
    CHECK(split_curve_count == 1U);

    // A newly observed Surface-pair relation with two disconnected branches
    // has no branch provenance discriminator. Neither branch may win: the
    // Curve itself and the strict Edge selector are both Ambiguous.
    kernel.duplicate_first_new_edge_branch = true;
    const auto multi_branch_eval =
        part::evaluatePart(
            semantic_base,
            kernel);
    kernel.duplicate_first_new_edge_branch = false;
    CHECK(
        multi_branch_eval.body_status ==
        part::BodyEvaluationStatus::up_to_date);
    const auto ambiguous_new_curve =
        std::find_if(
            multi_branch_eval.current_curve_references.begin(),
            multi_branch_eval.current_curve_references.end(),
            [id1](const auto& curve) {
                return curve.address.producer_feature_id == id1 &&
                       curve.status ==
                           kernel::ReferenceStatus::ambiguous &&
                       curve.strict_edge_status ==
                           kernel::ReferenceStatus::ambiguous &&
                       curve.candidate_edge_count == 2U;
            });
    CHECK(
        ambiguous_new_curve !=
        multi_branch_eval.current_curve_references.end());
    CHECK(ambiguous_new_curve->valid());
    CHECK(multi_branch_eval.current_topology.has_value());
    const auto ambiguous_edge_records =
        static_cast<std::size_t>(
            std::count_if(
                multi_branch_eval.current_topology->edges.begin(),
                multi_branch_eval.current_topology->edges.end(),
                [&ambiguous_new_curve](const auto& edge) {
                    return edge.referenceability ==
                               kernel::ReferenceStatus::
                                   ambiguous &&
                           std::find(
                               edge.curve_candidates.begin(),
                               edge.curve_candidates.end(),
                               ambiguous_new_curve->address) !=
                               edge.curve_candidates.end();
                }));
    CHECK(ambiguous_edge_records == 2U);

    // Deleting an inherited Edge meaning and creating a replacement with the
    // same semantic Surface relation does not revive the old Curve identity:
    // provider lineage says the old bounded realization is gone, so a new
    // Curve is produced by the current Feature.
    kernel.replace_first_inherited_edge = true;
    const auto replace_edge_eval =
        part::evaluatePart(
            valid,
            kernel);
    kernel.replace_first_inherited_edge = false;
    CHECK(
        replace_edge_eval.body_status ==
        part::BodyEvaluationStatus::up_to_date);
    const auto old_missing_curve =
        std::find_if(
            replace_edge_eval.current_curve_references.begin(),
            replace_edge_eval.current_curve_references.end(),
            [id1](const auto& curve) {
                return curve.address.producer_feature_id == id1 &&
                       curve.status ==
                           kernel::ReferenceStatus::missing;
            });
    CHECK(
        old_missing_curve !=
        replace_edge_eval.current_curve_references.end());
    CHECK(
        old_missing_curve->strict_edge_status ==
        kernel::ReferenceStatus::missing);
    CHECK(old_missing_curve->current_edges.empty());
    const auto replacement_curve =
        std::find_if(
            replace_edge_eval.current_curve_references.begin(),
            replace_edge_eval.current_curve_references.end(),
            [id2, &old_missing_curve](const auto& curve) {
                return curve.address.producer_feature_id == id2 &&
                       curve.address.role ==
                           old_missing_curve->address.role &&
                       curve.address.adjacent_surfaces ==
                           old_missing_curve->address
                               .adjacent_surfaces &&
                       curve.status ==
                           kernel::ReferenceStatus::resolved;
            });
    CHECK(
        replacement_curve !=
        replace_edge_eval.current_curve_references.end());
    CHECK(
        replacement_curve->strict_edge_status ==
        kernel::ReferenceStatus::resolved);

    // Exact same XYZ is deliberately insufficient to preserve Point
    // identity. Capture one old semantic Point and its diagnostic provider
    // coordinate, then remove its lineage and emit a new current Vertex at
    // exactly the same point.
    CHECK(!semantic_base_eval.current_topology->vertices.empty());
    const auto old_vertex_token =
        semantic_base_eval.current_topology
            ->vertices.front().runtime_token;
    const auto old_point =
        std::find_if(
            semantic_base_eval.current_point_references.begin(),
            semantic_base_eval.current_point_references.end(),
            [old_vertex_token](const auto& point) {
                return std::find(
                           point.current_vertices.begin(),
                           point.current_vertices.end(),
                           old_vertex_token) !=
                       point.current_vertices.end();
            });
    CHECK(
        old_point !=
        semantic_base_eval.current_point_references.end());
    const auto old_vertex_record =
        std::find_if(
            semantic_base_eval.current_topology->vertices.begin(),
            semantic_base_eval.current_topology->vertices.end(),
            [old_vertex_token](const auto& vertex) {
                return vertex.runtime_token ==
                       old_vertex_token;
            });
    CHECK(
        old_vertex_record !=
        semantic_base_eval.current_topology->vertices.end());
    CHECK(old_vertex_record->provider_point.has_value());

    kernel.replace_first_inherited_vertex_same_point = true;
    const auto replace_point_eval =
        part::evaluatePart(
            valid,
            kernel);
    kernel.replace_first_inherited_vertex_same_point = false;
    CHECK(
        replace_point_eval.body_status ==
        part::BodyEvaluationStatus::up_to_date);
    CHECK(replace_point_eval.current_topology.has_value());

    const auto missing_old_point =
        std::find_if(
            replace_point_eval.current_point_references.begin(),
            replace_point_eval.current_point_references.end(),
            [&old_point](const auto& point) {
                return point.address ==
                           old_point->address &&
                       point.status ==
                           kernel::ReferenceStatus::missing;
            });
    CHECK(
        missing_old_point !=
        replace_point_eval.current_point_references.end());
    CHECK(missing_old_point->current_vertices.empty());

    const auto new_same_xyz_point =
        std::find_if(
            replace_point_eval.current_point_references.begin(),
            replace_point_eval.current_point_references.end(),
            [id2, &old_point](const auto& point) {
                return point.address.producer_feature_id == id2 &&
                       point.address.adjacent_surfaces ==
                           old_point->address.adjacent_surfaces &&
                       point.status ==
                           kernel::ReferenceStatus::resolved &&
                       point.current_vertices.size() == 1U;
            });
    CHECK(
        new_same_xyz_point !=
        replace_point_eval.current_point_references.end());
    const auto replacement_vertex_token =
        new_same_xyz_point->current_vertices.front();
    const auto replacement_vertex_record =
        std::find_if(
            replace_point_eval.current_topology->vertices.begin(),
            replace_point_eval.current_topology->vertices.end(),
            [replacement_vertex_token](const auto& vertex) {
                return vertex.runtime_token ==
                       replacement_vertex_token;
            });
    CHECK(
        replacement_vertex_record !=
        replace_point_eval.current_topology->vertices.end());
    CHECK(replacement_vertex_record->provider_point.has_value());
    CHECK(
        replacement_vertex_record->provider_point->x ==
        old_vertex_record->provider_point->x);
    CHECK(
        replacement_vertex_record->provider_point->y ==
        old_vertex_record->provider_point->y);
    CHECK(
        replacement_vertex_record->provider_point->z ==
        old_vertex_record->provider_point->z);
    CHECK(
        new_same_xyz_point->address !=
        old_point->address);

    kernel.emit_semantic_subshapes = false;

    // Part -> Kernel translation preserves Reverse OneSide semantics.
    kernel.saw_reverse = false;
    auto reversed =
        withFeatures(
            fixture.document,
            {
                featureWithExtent(
                    id1,
                    fixture.profile_id,
                    part::ExtrudeOperation::add,
                    part::OneSidedExtrudeExtent{
                        core::LengthValue{8.0},
                        true}),
            },
            cursor);
    const auto reversed_evaluation =
        part::evaluatePart(
            reversed,
            kernel);
    CHECK(
        reversed_evaluation.body_status ==
        part::BodyEvaluationStatus::
            up_to_date);
    CHECK(kernel.saw_reverse);

    // Midplane uses total distance split equally and negative/positive caps.
    kernel.saw_midplane = false;
    auto centered =
        withFeatures(
            fixture.document,
            {
                featureWithExtent(
                    id1,
                    fixture.profile_id,
                    part::ExtrudeOperation::add,
                    part::MidplaneExtrudeExtent{
                        core::LengthValue{10.0}}),
            },
            cursor);
    const auto centered_evaluation =
        part::evaluatePart(
            centered,
            kernel);
    CHECK(
        centered_evaluation.body_status ==
        part::BodyEvaluationStatus::
            up_to_date);
    CHECK(kernel.saw_midplane);

    bool negative_cap = false;
    bool positive_cap = false;
    for (const auto& face :
         centered_evaluation.features[0]
             .produced_faces) {
        negative_cap =
            negative_cap ||
            face.address.role ==
                part::FeatureFaceRoleKind::
                    negative_cap;
        positive_cap =
            positive_cap ||
            face.address.role ==
                part::FeatureFaceRoleKind::
                    positive_cap;
    }
    CHECK(negative_cap);
    CHECK(positive_cap);

    // A first Cut is Blocked, and a later Add cannot silently restart history.
    auto blocked =
        withFeatures(
            fixture.document,
            {
                feature(
                    id1,
                    fixture.profile_id,
                    part::ExtrudeOperation::cut,
                    5.0),
                feature(
                    id2,
                    fixture.profile_id,
                    part::ExtrudeOperation::add,
                    10.0),
            },
            cursor);
    const auto blocked_eval =
        part::evaluatePart(
            blocked,
            kernel);
    CHECK(
        blocked_eval.body_status ==
        part::BodyEvaluationStatus::
            unavailable);
    CHECK(blocked_eval.body_solid == nullptr);
    CHECK(blocked_eval.resolved_prefix_solid == nullptr);
    CHECK(
        blocked_eval.features[0].status ==
        part::FeatureEvaluationStatus::
            blocked);
    CHECK(
        blocked_eval.features[0].diagnostic ==
        part::FeatureEvaluationDiagnosticCode::
            missing_upstream_body);
    CHECK(
        blocked_eval.features[1].status ==
        part::FeatureEvaluationStatus::
            blocked);
    CHECK(
        blocked_eval.features[1].diagnostic ==
        part::FeatureEvaluationDiagnosticCode::
            upstream_unavailable);

    // Suppressed history does not contribute and does not poison a later
    // first successful Add.
    auto suppressed =
        withFeatures(
            fixture.document,
            {
                feature(
                    id1,
                    fixture.profile_id,
                    part::ExtrudeOperation::cut,
                    5.0,
                    true),
                feature(
                    id2,
                    fixture.profile_id,
                    part::ExtrudeOperation::add,
                    10.0),
            },
            cursor);
    const auto suppressed_eval =
        part::evaluatePart(
            suppressed,
            kernel);
    CHECK(
        suppressed_eval.features[0].status ==
        part::FeatureEvaluationStatus::
            suppressed);
    CHECK(
        suppressed_eval.features[1].status ==
        part::FeatureEvaluationStatus::
            up_to_date);
    CHECK(
        suppressed_eval.body_status ==
        part::BodyEvaluationStatus::
            up_to_date);

    // A geometric/kernel failure invalidates final Body truth and blocks all
    // later active Features. H7 may retain only the same-revision upstream
    // prefix for presentation; it is not final Body truth and is never
    // consumed downstream.
    auto failed =
        withFeatures(
            fixture.document,
            {
                feature(
                    id1,
                    fixture.profile_id,
                    part::ExtrudeOperation::add,
                    10.0),
                feature(
                    id2,
                    fixture.profile_id,
                    part::ExtrudeOperation::add,
                    99.0),
                feature(
                    id3,
                    fixture.profile_id,
                    part::ExtrudeOperation::cut,
                    2.0),
            },
            cursor);
    const auto failed_eval =
        part::evaluatePart(
            failed,
            kernel);
    CHECK(
        failed_eval.features[0].status ==
        part::FeatureEvaluationStatus::
            up_to_date);
    CHECK(
        failed_eval.features[1].status ==
        part::FeatureEvaluationStatus::
            failed);
    CHECK(
        failed_eval.features[1].diagnostic ==
        part::FeatureEvaluationDiagnosticCode::
            no_effect);
    CHECK(
        failed_eval.features[2].status ==
        part::FeatureEvaluationStatus::
            blocked);
    CHECK(
        failed_eval.body_status ==
        part::BodyEvaluationStatus::
            unavailable);
    CHECK(failed_eval.body_solid == nullptr);
    CHECK(failed_eval.resolved_prefix_solid != nullptr);
    CHECK(failed_eval.resolved_prefix_topology.has_value());
    CHECK(
        failed_eval.resolved_prefix_topology->stage.feature_id ==
        std::optional<part::FeatureId>{id1});
    CHECK(!failed_eval.current_topology.has_value());
    CHECK(
        failed_eval.current_face_references.empty());

    // PM-02A fail-closed admission: a provider result whose declared
    // unique Face count does not match its current Face inventory cannot
    // become Body truth even when the modeling operation itself returned ok.
    kernel.corrupt_topology_inventory = true;
    auto corrupt_topology =
        withFeatures(
            fixture.document,
            {
                feature(
                    id1,
                    fixture.profile_id,
                    part::ExtrudeOperation::add,
                    10.0),
            },
            cursor);
    const auto corrupt_eval =
        part::evaluatePart(
            corrupt_topology,
            kernel);
    CHECK(
        corrupt_eval.body_status ==
        part::BodyEvaluationStatus::
            unavailable);
    CHECK(corrupt_eval.body_solid == nullptr);
    CHECK(!corrupt_eval.current_topology.has_value());
    CHECK(corrupt_eval.resolved_prefix_solid == nullptr);
    CHECK(!corrupt_eval.resolved_prefix_topology.has_value());
    CHECK(
        corrupt_eval.features.size() == 1U);
    CHECK(
        corrupt_eval.features[0].status ==
        part::FeatureEvaluationStatus::
            failed);
    CHECK(
        corrupt_eval.features[0].diagnostic ==
        part::FeatureEvaluationDiagnosticCode::
            topology_integrity_failure);
    CHECK(corrupt_eval.features[0].result_solid == nullptr);
    CHECK(!corrupt_eval.features[0].result_topology.has_value());
    kernel.corrupt_topology_inventory = false;

    // A deleted Profile leaves repairable authored Feature intent but blocks
    // evaluation rather than corrupting the Part.
    auto missing_state =
        valid.state();
    missing_state.profiles.clear();
    auto missing_restore =
        part::PartDocument::restore(
            valid.documentId(),
            std::move(missing_state),
            valid.revision());
    CHECK(missing_restore.ok());
    const auto missing_eval =
        part::evaluatePart(
            *missing_restore.document,
            kernel);
    CHECK(
        missing_eval.features[0].status ==
        part::FeatureEvaluationStatus::
            blocked);
    CHECK(
        missing_eval.features[0].diagnostic ==
        part::FeatureEvaluationDiagnosticCode::
            missing_profile);
    CHECK(
        missing_eval.features[1].status ==
        part::FeatureEvaluationStatus::
            blocked);
    CHECK(missing_eval.body_solid == nullptr);
    CHECK(missing_eval.resolved_prefix_solid == nullptr);

    std::cout
        << "PM01C_EVALUATOR_PASS"
        << " stale_last_good=0"
        << " resolved_prefix_presentation=1"
        << " topology_catalog=1"
        << " split_face_ambiguous_surface_resolved=1"
        << " alias_no_winner=ambiguous"
        << " independent_winner=resolved_missing"
        << " unresolved_surface_frame=none"
        << " curve_carrier_split=resolved_edge_ambiguous"
        << " new_pair_multibranch=ambiguous"
        << " edge_replacement=no_rebind"
        << " same_xyz_point_replacement=distinct"
        << " topology_integrity_fail_closed=1"
        << " restart_after_failure=0\n";
    return EXIT_SUCCESS;
}
