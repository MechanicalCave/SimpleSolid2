#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/kernel/face_boundary.hpp>
#include <simplesolid2/kernel_occt/solid_modeling_kernel.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <map>
#include <set>
#include <cmath>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-02J R2 Add Surface continuation CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

kernel::BoundaryUse2D lineUse(
    kernel::Point2 start,
    kernel::Point2 end,
    std::string source,
    std::uint32_t use_index) {
    return {
        kernel::Line2{start, end},
        0.0,
        1.0,
        true,
        false,
        false,
        kernel::BoundaryUseProvenance{
            std::move(source),
            0U,
            use_index,
            false},
    };
}

kernel::PlanarProfileInput rectangle(
    kernel::Frame3 frame,
    double u0,
    double v0,
    double u1,
    double v1,
    std::string_view prefix) {
    kernel::PlanarProfileInput profile;
    profile.frame = frame;
    profile.outer.boundary = {
        lineUse(
            {u0, v0},
            {u1, v0},
            std::string{prefix} + "-bottom",
            0U),
        lineUse(
            {u1, v0},
            {u1, v1},
            std::string{prefix} + "-right",
            1U),
        lineUse(
            {u1, v1},
            {u0, v1},
            std::string{prefix} + "-top",
            2U),
        lineUse(
            {u0, v1},
            {u0, v0},
            std::string{prefix} + "-left",
            3U),
    };
    CHECK(profile.valid());
    return profile;
}

kernel::LinearExtrudeInput add(
    kernel::PlanarProfileInput profile,
    double distance) {
    kernel::LinearExtrudeInput input;
    input.profile = std::move(profile);
    input.start_offset_mm = 0.0;
    input.end_offset_mm = distance;
    input.start_cap_role =
        kernel::ExtrudeCapRole::profile_cap;
    input.end_cap_role =
        kernel::ExtrudeCapRole::extent_cap;
    input.operation =
        kernel::SolidBooleanOperation::add;
    CHECK(input.valid());
    return input;
}

const kernel::NewSurfaceLineage*
newCap(
    const kernel::SolidModelingResult& result,
    kernel::ExtrudeCapRole role) {
    const auto found =
        std::find_if(
            result.new_surfaces.begin(),
            result.new_surfaces.end(),
            [role](const auto& surface) {
                return surface.role.kind ==
                           kernel::ExtrudeGeneratedFaceRoleKind::cap &&
                       surface.role.cap_role ==
                           std::optional<kernel::ExtrudeCapRole>{role};
            });
    return found == result.new_surfaces.end()
        ? nullptr
        : &*found;
}

const kernel::NewSurfaceLineage*
newSide(
    const kernel::SolidModelingResult& result,
    std::string_view source) {
    const auto found =
        std::find_if(
            result.new_surfaces.begin(),
            result.new_surfaces.end(),
            [source](const auto& surface) {
                return surface.role.kind ==
                           kernel::ExtrudeGeneratedFaceRoleKind::side &&
                       surface.role.side_provenance &&
                       surface.role.side_provenance->source_entity ==
                           source;
            });
    return found == result.new_surfaces.end()
        ? nullptr
        : &*found;
}

const kernel::InheritedSurfaceLineage*
inheritedSurface(
    const kernel::SolidModelingResult& result,
    kernel::RuntimeSurfaceToken token) {
    const auto found =
        std::find_if(
            result.inherited_surfaces.begin(),
            result.inherited_surfaces.end(),
            [token](const auto& surface) {
                return surface.token == token;
            });
    return found == result.inherited_surfaces.end()
        ? nullptr
        : &*found;
}

sketch::Point2 projectToFrame(
    const kernel::Frame3& frame,
    kernel::Point3 point) {
    const kernel::Point3 delta{
        point.x - frame.origin.x,
        point.y - frame.origin.y,
        point.z - frame.origin.z};
    return {
        delta.x * frame.u_axis.x +
            delta.y * frame.u_axis.y +
            delta.z * frame.u_axis.z,
        delta.x * frame.v_axis.x +
            delta.y * frame.v_axis.y +
            delta.z * frame.v_axis.z,
    };
}

part::ProfileId createRectangleProfile(
    application::DocumentSession& session,
    sketch::SketchId sketch_id,
    sketch::Point2 first,
    sketch::Point2 opposite) {
    const auto rectangle =
        session.execute(
            application::AddSketchRectangleCommand{
                sketch_id,
                session.document().revision(),
                first,
                opposite,
                sketch::EntityRole::regular,
                false});
    CHECK(rectangle.ok());

    const auto* hosted =
        session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    const auto regions =
        sketch::analyzeRegions(hosted->model);
    CHECK(regions.complete());
    CHECK(regions.regions.size() == 1U);
    const auto intent =
        part::makeProfileRegionIntent(
            regions.regions.front());
    CHECK(intent.has_value());

    const auto profile =
        session.execute(
            application::CreateProfileCommand{
                sketch_id,
                session.document().revision(),
                *intent});
    CHECK(profile.ok());
    CHECK(profile.profile_id.has_value());
    return *profile.profile_id;
}

void verifyPartIntegration() {
    kernel_occt::OcctSolidModelingKernel provider;
    auto document =
        part::PartDocument::create(
            core::DocumentId::generate());
    application::DocumentSession session{
        std::filesystem::path{
            "pm02jr2-add-continuation.ss2part"},
        std::move(document)};

    const auto base_sketch =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
    CHECK(base_sketch.ok());
    CHECK(base_sketch.sketch_id.has_value());

    const auto base_profile =
        createRectangleProfile(
            session,
            *base_sketch.sketch_id,
            {0.0, 0.0},
            {40.0, 30.0});

    const auto base =
        session.execute(
            application::CreateExtrudeFeatureCommand{
                base_profile,
                session.document().revision(),
                part::ExtrudeOperation::add,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{10.0},
                    false},
                "Base"},
            provider);
    CHECK(base.ok());
    CHECK(base.feature_id.has_value());

    const auto base_eval =
        part::evaluatePart(
            session.document(),
            provider);
    CHECK(
        base_eval.body_status ==
        part::BodyEvaluationStatus::up_to_date);
    CHECK(base_eval.current_topology.has_value());

    const auto& base_topology =
        *base_eval.current_topology;
    const auto top =
        std::find_if(
            base_topology.surfaces.begin(),
            base_topology.surfaces.end(),
            [id = *base.feature_id](
                const auto& surface) {
                return surface.status ==
                           kernel::ReferenceStatus::resolved &&
                       surface.address.producer_feature_id ==
                           id &&
                       surface.address.role ==
                           part::FeatureSurfaceRoleKind::
                               extent_cap &&
                       surface.surface_kind ==
                           kernel::SurfaceKind::plane &&
                       surface.canonical_frame;
            });
    CHECK(top != base_topology.surfaces.end());
    CHECK(top->canonical_frame.has_value());

    const auto continued_side =
        std::find_if(
            base_topology.surfaces.begin(),
            base_topology.surfaces.end(),
            [id = *base.feature_id](
                const auto& surface) {
                return surface.status ==
                           kernel::ReferenceStatus::resolved &&
                       surface.address.producer_feature_id ==
                           id &&
                       surface.address.role ==
                           part::FeatureSurfaceRoleKind::side &&
                       surface.surface_kind ==
                           kernel::SurfaceKind::plane &&
                       surface.canonical_frame &&
                       surface.canonical_frame->normal.x > 0.9;
            });
    CHECK(continued_side !=
          base_topology.surfaces.end());

    const part::SurfaceReference top_ref{
        base_topology.stage,
        top->address};
    const auto top_support =
        part::partSketchSupportForBodyPlanarSurface(
            top_ref);
    CHECK(top_support.has_value());

    const auto boss_sketch =
        session.execute(
            application::CreatePartSketchOnSupportCommand{
                *top_support,
                session.document().revision()},
            &provider);
    CHECK(boss_sketch.ok());
    CHECK(boss_sketch.sketch_id.has_value());

    const auto& top_frame =
        *top->canonical_frame;
    const std::vector<kernel::Point3> boss_corners{
        {20.0, 5.0, 10.0},
        {40.0, 5.0, 10.0},
        {20.0, 25.0, 10.0},
        {40.0, 25.0, 10.0},
    };
    std::vector<sketch::Point2> uv;
    uv.reserve(boss_corners.size());
    for (const auto& point : boss_corners) {
        uv.push_back(
            projectToFrame(
                top_frame,
                point));
    }
    double min_u = uv.front().u;
    double max_u = uv.front().u;
    double min_v = uv.front().v;
    double max_v = uv.front().v;
    for (const auto point : uv) {
        min_u = std::min(min_u, point.u);
        max_u = std::max(max_u, point.u);
        min_v = std::min(min_v, point.v);
        max_v = std::max(max_v, point.v);
    }
    CHECK(max_u - min_u > 1.0);
    CHECK(max_v - min_v > 1.0);

    const auto boss_profile =
        createRectangleProfile(
            session,
            *boss_sketch.sketch_id,
            {min_u, min_v},
            {max_u, max_v});

    const auto boss =
        session.execute(
            application::CreateExtrudeFeatureCommand{
                boss_profile,
                session.document().revision(),
                part::ExtrudeOperation::add,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{10.0},
                    false},
                "Boss"},
            provider);
    CHECK(boss.ok());
    CHECK(boss.feature_id.has_value());

    const auto final_eval =
        part::evaluatePart(
            session.document(),
            provider);
    CHECK(
        final_eval.body_status ==
        part::BodyEvaluationStatus::up_to_date);
    CHECK(final_eval.current_topology.has_value());
    const auto& topology =
        *final_eval.current_topology;

    const auto current_side =
        std::find_if(
            topology.surfaces.begin(),
            topology.surfaces.end(),
            [address = continued_side->address](
                const auto& surface) {
                return surface.address == address;
            });
    CHECK(current_side != topology.surfaces.end());
    CHECK(
        current_side->status ==
        kernel::ReferenceStatus::resolved);
    CHECK(current_side->current_faces.size() >= 2U);
    CHECK(
        current_side->strict_face_status ==
        kernel::ReferenceStatus::ambiguous);

    for (const auto face_token :
         current_side->current_faces) {
        const auto face =
            std::find_if(
                topology.faces.begin(),
                topology.faces.end(),
                [face_token](const auto& record) {
                    return record.runtime_token ==
                           face_token;
                });
        CHECK(face != topology.faces.end());
        CHECK(face->surface_candidates.size() == 1U);
        CHECK(
            face->surface_candidates.front() ==
            current_side->address);
    }

    const auto contribution =
        part::currentFeatureContribution(
            topology,
            *boss.feature_id);
    CHECK(contribution.valid());
    CHECK(!contribution.faces.empty());

    const auto partition_count =
        std::count_if(
            topology.edges.begin(),
            topology.edges.end(),
            [](const auto& edge) {
                return edge.representation_partition &&
                       edge.accounting_class ==
                           part::TopologyAccountingClass::
                               known_representation_artifact &&
                       edge.referenceability ==
                           kernel::ReferenceStatus::
                               unsupported;
            });
    CHECK(partition_count >= 1);

    // PG-01D D1: the Surface carrier remains one semantic Surface even
    // where it spans two distinct current bounded Face realizations.
    // Read *each* native Face's real oriented wires independently; do
    // not reinterpret the carrier as one guessed Face perimeter.
    CHECK(final_eval.body_solid);
    std::vector<kernel::RuntimeFaceToken> observed_faces;
    std::vector<part::FeatureFaceAddress> strict_bounded_addresses;
    std::size_t carrier_only_faces = 0U;
    std::size_t admitted_strict_faces = 0U;
    std::size_t boundary_material_uses = 0U;
    std::size_t boundary_partition_uses = 0U;
    std::size_t boundary_other_nonmaterial_uses = 0U;
    // E0 Face Boundary is still a read-only native incidence
    // experiment: one exact same-Surface carrier, several bounded
    // Faces, current-scoped real provider Edge tokens only.
    std::map<std::uint64_t, std::size_t>
        grouped_same_surface_native_edge_uses;
    // Native per-Face oriented uses retained separately: opposite
    // orientation is necessary before same-Surface cancellation.
    std::map<std::uint64_t, std::vector<bool>>
        grouped_same_surface_native_edge_orientations;
    std::size_t blocked_face_admissions = 0U;
    std::size_t accepted_face_admissions = 0U;
    for (const auto token : current_side->current_faces) {
        CHECK(std::find(
            observed_faces.begin(),
            observed_faces.end(), token) == observed_faces.end());
        observed_faces.push_back(token);
        const auto* face = [&]() -> const part::BodyFaceTopologyRecord* {
            for (const auto& record : topology.faces) {
                if (record.runtime_token == token) return &record;
            }
            return nullptr;
        }();
        CHECK(face != nullptr);
        CHECK(face->surface_candidates.size() == 1U);
        CHECK(face->surface_candidates.front() ==
              current_side->address);
        if (face->semantic_address) {
            // Each bounded Face is independent current semantic meaning
            // even when the shared Surface carrier cannot name one Face.
            CHECK(std::find(
                strict_bounded_addresses.begin(),
                strict_bounded_addresses.end(),
                *face->semantic_address) ==
                strict_bounded_addresses.end());
            strict_bounded_addresses.push_back(
                *face->semantic_address);
            ++admitted_strict_faces;
        } else {
            // Surface-carrier support alone does not authorize a Face.
            ++carrier_only_faces;
        }
        const auto scoped =
            provider.bindFaceToBody(final_eval.body_solid, token);
        CHECK(scoped && scoped->valid());
        const auto native =
            provider.queryFaceBoundary(final_eval.body_solid, *scoped);
        CHECK(native.ok());
        std::size_t outer_wires = 0U;
        bool all_members_material = true;
        for (const auto& wire : native.wires) {
            CHECK(wire.valid());
            if (wire.outer) ++outer_wires;
            for (const auto& occurrence : wire.edges) {
                CHECK(occurrence.valid());
                ++grouped_same_surface_native_edge_uses[
                    occurrence.edge.value];
                grouped_same_surface_native_edge_orientations[
                    occurrence.edge.value].push_back(
                        occurrence.reversed);
                const auto edge = std::find_if(
                    topology.edges.begin(),
                    topology.edges.end(),
                    [&occurrence](const auto& record) {
                        return record.runtime_token ==
                               occurrence.edge;
                    });
                CHECK(edge != topology.edges.end());
                const auto author =
                    part::authorMaterialEdgeReference(
                        topology, occurrence.edge);
                if (edge->representation_partition ||
                    edge->periodic_seam) {
                    // Native wires contain representation artifacts.
                    // These are never material Sketch source references.
                    CHECK(!author.ok());
                    all_members_material = false;
                    ++boundary_partition_uses;
                } else if (author.ok()) {
                    CHECK(author.reference &&
                          author.reference->stage == topology.stage);
                    ++boundary_material_uses;
                } else {
                    all_members_material = false;
                    ++boundary_other_nonmaterial_uses;
                }
            }
        }
        CHECK(outer_wires == 1U);

        const auto inspected =
            part::inspectMaterialFaceBoundary(
                final_eval.features.back(), token, provider);
        if (!face->semantic_address) {
            CHECK(!inspected.ok());
            CHECK(inspected.status ==
                  part::MaterialFaceBoundaryStatus::face_not_strict);
            ++blocked_face_admissions;
        } else if (!all_members_material) {
            // Do not partly author a bounded Face whose native wire
            // contains a nonmaterial partition/seam occurrence.
            CHECK(!inspected.ok());
            CHECK(inspected.status ==
                  part::MaterialFaceBoundaryStatus::
                      material_edge_unavailable);
            ++blocked_face_admissions;
        } else {
            CHECK(inspected.ok());
            CHECK(inspected.bounded_face ==
                  face->semantic_address);
            CHECK(inspected.wires.size() ==
                  native.wires.size());
            for (std::size_t index = 0U;
                 index < native.wires.size(); ++index) {
                CHECK(inspected.wires[index].outer ==
                      native.wires[index].outer);
                CHECK(inspected.wires[index].edges.size() ==
                      native.wires[index].edges.size());
            }
            ++accepted_face_admissions;
        }
    }
    CHECK(observed_faces.size() ==
          current_side->current_faces.size());
    CHECK(carrier_only_faces + admitted_strict_faces ==
          observed_faces.size());
    CHECK(strict_bounded_addresses.size() ==
          admitted_strict_faces);
    // The Surface reference is ambiguous for bounded Face selection,
    // but individual Face addresses can still be strict and distinct.
    CHECK(admitted_strict_faces >= 2U);
    CHECK(boundary_material_uses > 0U);
    std::cout
        << "PG01D_D1_SPLIT_FACE_BOUNDARY_PASS"
        << " face_realizations=" << observed_faces.size()
        << " carrier_only=" << carrier_only_faces
        << " strict_face=" << admitted_strict_faces
        << " material_uses=" << boundary_material_uses
        << " rejected_partition_or_seam="
        << boundary_partition_uses
        << " other_nonmaterial="
        << boundary_other_nonmaterial_uses
        << " blocked_admissions="
        << blocked_face_admissions
        << " accepted_admissions="
        << accepted_face_admissions
        << '\n';
    // The former shared-Surface partition Edge is an actual native
    // boundary use and must never be promoted into material identity.
    CHECK(boundary_partition_uses >= 1U);
    CHECK(blocked_face_admissions >= 1U);

    // Face Boundary E0: cancel ONLY certified internal representation
    // edges from this one strictly-equal Surface carrier. Every such
    // partition must occur exactly twice in the real oriented Face
    // wire ledger. Every remaining source must be an individually
    // referenceable material Edge occurring once on this region's
    // perimeter. This is not yet a contour/wire reconstruction or
    // an authorization to merge other coplanar Surfaces. The
    // surviving set still needs an exact single-cycle proof below;
    // outer/hole classification for general regions is not implied.
    std::size_t certified_internal_partitions = 0U;
    std::vector<part::MaterialEdgeReference>
        certified_perimeter_sources;
    std::vector<kernel::RuntimeEdgeToken>
        certified_perimeter_tokens;
    for (const auto& [value, occurrences] :
         grouped_same_surface_native_edge_uses) {
        const auto token = kernel::RuntimeEdgeToken{value};
        const auto found = std::find_if(
            topology.edges.begin(), topology.edges.end(),
            [token](const auto& edge) {
                return edge.runtime_token == token;
            });
        CHECK(found != topology.edges.end());
        if (found->representation_partition) {
            CHECK(found->accounting_class ==
                  part::TopologyAccountingClass::
                      known_representation_artifact);
            CHECK(occurrences == 2U);
            const auto oriented =
                grouped_same_surface_native_edge_orientations.find(
                    token.value);
            CHECK(oriented !=
                  grouped_same_surface_native_edge_orientations.end());
            CHECK(oriented->second.size() == 2U);
            CHECK(oriented->second[0] != oriented->second[1]);
            CHECK(!part::authorMaterialEdgeReference(
                topology, token).ok());
            ++certified_internal_partitions;
            continue;
        }
        // A periodic seam is a separate representation artifact and
        // requires its own positive incidence proof on curved faces.
        CHECK(!found->periodic_seam);
        const auto material =
            part::authorMaterialEdgeReference(
                topology, token);
        CHECK(material.ok());
        CHECK(material.reference);
        CHECK(occurrences == 1U);
        const auto oriented =
            grouped_same_surface_native_edge_orientations.find(
                token.value);
        CHECK(oriented !=
              grouped_same_surface_native_edge_orientations.end());
        CHECK(oriented->second.size() == 1U);
        certified_perimeter_sources.push_back(
            *material.reference);
        certified_perimeter_tokens.push_back(token);
    }
    CHECK(certified_internal_partitions >= 1U);
    CHECK(!certified_perimeter_sources.empty());
    std::sort(
        certified_perimeter_sources.begin(),
        certified_perimeter_sources.end());
    CHECK(std::adjacent_find(
        certified_perimeter_sources.begin(),
        certified_perimeter_sources.end()) ==
        certified_perimeter_sources.end());
    // E0 positive proof for THIS planar split-carrier fixture only:
    // retain exact current native provider Vertex/Edge incidences; never
    // pair coordinates or infer missing geometry. Degree two alone could
    // allow several DISCONNECTED cycles, which is insufficient for this
    // fixture's expected one contiguous material perimeter.
    std::map<std::uint64_t, std::vector<std::uint64_t>>
        perimeter_vertex_edges;
    std::map<std::uint64_t, std::vector<std::uint64_t>>
        perimeter_edge_vertices;
    for (const auto& vertex : topology.vertices) {
        std::vector<std::uint64_t> incident;
        for (const auto token : certified_perimeter_tokens) {
            if (std::find(
                    vertex.incident_material_edges.begin(),
                    vertex.incident_material_edges.end(),
                    token) != vertex.incident_material_edges.end()) {
                incident.push_back(token.value);
            }
        }
        if (incident.empty()) continue;
        CHECK(vertex.runtime_token.value != 0U);
        CHECK(incident.size() == 2U);
        CHECK(incident[0] != incident[1]);
        for (const auto edge : incident) {
            perimeter_edge_vertices[edge].push_back(
                vertex.runtime_token.value);
        }
        CHECK(perimeter_vertex_edges.emplace(
            vertex.runtime_token.value,
            std::move(incident)).second);
    }
    CHECK(!perimeter_edge_vertices.empty());
    CHECK(perimeter_edge_vertices.size() ==
          certified_perimeter_tokens.size());
    CHECK(perimeter_vertex_edges.size() ==
          certified_perimeter_tokens.size());
    for (const auto& [edge, endpoints] :
         perimeter_edge_vertices) {
        CHECK(endpoints.size() == 2U);
        CHECK(endpoints[0] != endpoints[1]);
        (void)edge;
    }

    // Reconstruct and exhaust the SINGLE closed cycle starting from the
    // lowest current Edge token. A disconnected second cycle would cause
    // this walk to return to its start too early and fail closed.
    const auto start_edge =
        perimeter_edge_vertices.begin()->first;
    const auto start_vertex =
        perimeter_edge_vertices.begin()->second.front();
    auto current_edge = start_edge;
    auto current_vertex = start_vertex;
    std::set<std::uint64_t> visited_edges;
    for (std::size_t walk = 0U;
         walk < perimeter_edge_vertices.size();
         ++walk) {
        CHECK(visited_edges.insert(current_edge).second);
        const auto edge =
            perimeter_edge_vertices.find(current_edge);
        CHECK(edge != perimeter_edge_vertices.end());
        const auto& endpoints = edge->second;
        CHECK(endpoints[0] == current_vertex ||
              endpoints[1] == current_vertex);
        const auto next_vertex =
            endpoints[0] == current_vertex
                ? endpoints[1]
                : endpoints[0];
        const auto vertex =
            perimeter_vertex_edges.find(next_vertex);
        CHECK(vertex != perimeter_vertex_edges.end());
        const auto& neighbors = vertex->second;
        CHECK(neighbors.size() == 2U);
        CHECK(neighbors[0] == current_edge ||
              neighbors[1] == current_edge);
        const auto next_edge =
            neighbors[0] == current_edge
                ? neighbors[1]
                : neighbors[0];
        if (walk + 1U == perimeter_edge_vertices.size()) {
            CHECK(next_vertex == start_vertex);
            CHECK(next_edge == start_edge);
        } else {
            CHECK(next_edge != start_edge);
        }
        current_vertex = next_vertex;
        current_edge = next_edge;
    }
    CHECK(visited_edges.size() ==
          certified_perimeter_tokens.size());
    // E0 negative boundary distinction for this real Add-continuation
    // Body: a native Edge shared with a Face of ANOTHER semantic Surface
    // must remain a material perimeter member of the selected carrier.
    // Do not cancel it as a representation partition merely because
    // OCCT reports the same Edge on two adjacent Face wires.
    // This is current-stage equality of explicit Surface addresses,
    // not an inference from normal/coplanarity/curve geometry.
    std::size_t foreign_carrier_shared_material_edges = 0U;
    std::vector<part::FeatureSurfaceAddress>
        observed_distinct_neighbor_carriers;
    std::set<std::uint64_t> observed_neighbor_material_tokens;
    for (const auto& foreign_face : topology.faces) {
        if (foreign_face.surface_candidates.size() != 1U ||
            foreign_face.surface_candidates.front() ==
                current_side->address) {
            continue;
        }
        const auto& other_carrier =
            foreign_face.surface_candidates.front();
        const auto scoped = provider.bindFaceToBody(
            final_eval.body_solid, foreign_face.runtime_token);
        CHECK(scoped && scoped->valid());
        const auto raw = provider.queryFaceBoundaryAnySurface(
            final_eval.body_solid, *scoped);
        CHECK(raw.ok());
        for (const auto& wire : raw.wires) {
            for (const auto& use : wire.edges) {
                if (std::find(
                        certified_perimeter_tokens.begin(),
                        certified_perimeter_tokens.end(),
                        use.edge) ==
                    certified_perimeter_tokens.end()) {
                    continue;
                }
                const auto in_selected =
                    grouped_same_surface_native_edge_uses.find(
                        use.edge.value);
                CHECK(in_selected !=
                      grouped_same_surface_native_edge_uses.end());
                CHECK(in_selected->second == 1U);
                const auto material =
                    part::authorMaterialEdgeReference(
                        topology, use.edge);
                CHECK(material.ok());
                const auto edge = std::find_if(
                    topology.edges.begin(),
                    topology.edges.end(),
                    [&use](const auto& record) {
                        return record.runtime_token == use.edge;
                    });
                CHECK(edge != topology.edges.end());
                CHECK(!edge->representation_partition);
                CHECK(!edge->periodic_seam);
                if (observed_neighbor_material_tokens.insert(
                        use.edge.value).second) {
                    ++foreign_carrier_shared_material_edges;
                }
                if (std::find(
                        observed_distinct_neighbor_carriers.begin(),
                        observed_distinct_neighbor_carriers.end(),
                        other_carrier) ==
                    observed_distinct_neighbor_carriers.end()) {
                    observed_distinct_neighbor_carriers.push_back(
                        other_carrier);
                }
            }
        }
    }
    CHECK(foreign_carrier_shared_material_edges >= 1U);
    CHECK(!observed_distinct_neighbor_carriers.empty());
    for (const auto& candidate :
         observed_distinct_neighbor_carriers) {
        CHECK(candidate != current_side->address);
    }
    std::cout
        << "PG01D_FACE_BOUNDARY_E0_SPLIT_CARRIER_PERIMETER_PASS"
        << " same_semantic_surface=1"
        << " split_faces=" << observed_faces.size()
        << " cancelled_internal_partitions="
        << certified_internal_partitions
        << " partition_opposite_oriented_uses=1"
        << " distinct_material_perimeter_edges="
        << certified_perimeter_sources.size()
        << " single_closed_component=1"
        << " foreign_surface_carriers="
        << observed_distinct_neighbor_carriers.size()
        << " foreign_carrier_shared_material_edges="
        << foreign_carrier_shared_material_edges
        << " source_proximity_guessing=0"
        << '\\n';
    const auto invalid_face =
        part::inspectMaterialFaceBoundary(
            final_eval.features.back(),
            kernel::RuntimeFaceToken{}, provider);
    CHECK(invalid_face.status ==
          part::MaterialFaceBoundaryStatus::face_unavailable);
    auto detached_stage = final_eval.features.back();
    detached_stage.result_solid.reset();
    const auto invalid_stage =
        part::inspectMaterialFaceBoundary(
            detached_stage,
            observed_faces.front(), provider);
    CHECK(invalid_stage.status ==
          part::MaterialFaceBoundaryStatus::invalid_stage);

    const part::SurfaceReference final_side_ref{
        topology.stage,
        current_side->address};
    const auto side_support =
        part::partSketchSupportForBodyPlanarSurface(
            final_side_ref);
    CHECK(side_support.has_value());

    const auto new_sketch =
        session.execute(
            application::CreatePartSketchOnSupportCommand{
                *side_support,
                session.document().revision()},
            &provider);
    CHECK(new_sketch.ok());
    CHECK(new_sketch.sketch_id.has_value());

    // PG-01D E0: actual Cut through the LOWER portion of the same
    // previously split +X material Surface. This is deliberately not
    // a guessed stitched Face: source Sketch is explicitly supported by
    // the inherited same-stage semantic Surface, and OCCT re-evaluates
    // the full Part with a real circular hole in one bounded fragment.
    CHECK(current_side->canonical_frame.has_value());
    CHECK(current_side->canonical_frame->normal.x > 0.9);
    const auto hole_uv = projectToFrame(
        *current_side->canonical_frame,
        kernel::Point3{40.0, 15.0, 5.0});
    const auto hole_circle = session.execute(
        application::AddSketchCircleCommand{
            *new_sketch.sketch_id,
            hole_uv,
            2.5,
            sketch::EntityRole::regular});
    CHECK(hole_circle.ok());
    const auto* hole_host =
        session.document().findSketch(*new_sketch.sketch_id);
    CHECK(hole_host);
    const auto hole_regions =
        sketch::analyzeRegions(hole_host->model);
    CHECK(hole_regions.complete());
    CHECK(hole_regions.regions.size() == 1U);
    const auto hole_intent =
        part::makeProfileRegionIntent(
            hole_regions.regions.front());
    CHECK(hole_intent.has_value());
    const auto hole_profile = session.execute(
        application::CreateProfileCommand{
            *new_sketch.sketch_id,
            session.document().revision(),
            *hole_intent});
    CHECK(hole_profile.ok() && hole_profile.profile_id);
    const auto hole_cut = session.execute(
        application::CreateExtrudeFeatureCommand{
            *hole_profile.profile_id,
            session.document().revision(),
            part::ExtrudeOperation::cut,
            part::OneSidedExtrudeExtent{
                core::LengthValue{45.0},
                true},
            "PG01D split-carrier lower side through-hole"},
        provider);
    CHECK(hole_cut.ok() && hole_cut.feature_id);

    const auto drilled =
        part::evaluatePart(session.document(), provider);
    CHECK(drilled.body_status ==
          part::BodyEvaluationStatus::up_to_date);
    CHECK(drilled.body_solid && drilled.current_topology);
    CHECK(drilled.current_topology->complete());
    const auto& drilled_catalog = *drilled.current_topology;
    CHECK(drilled_catalog.stage.feature_id &&
          *drilled_catalog.stage.feature_id == *hole_cut.feature_id);
    const auto drilled_side = std::find_if(
        drilled_catalog.surfaces.begin(),
        drilled_catalog.surfaces.end(),
        [address = continued_side->address](
            const auto& surface) {
            return surface.address == address;
        });
    CHECK(drilled_side != drilled_catalog.surfaces.end());
    CHECK(drilled_side->status ==
          kernel::ReferenceStatus::resolved);
    CHECK(drilled_side->current_faces.size() >= 2U);

    // Native oriented wire accounting within the ONE certified carrier.
    // A real hole is identified by the OCCT Face's INNER wire, not by
    // geometry proximity, Circle radius or viewer outline. Partition
    // cancellation is allowed only for exactly two opposite-oriented
    // native uses on different fragments of this same Surface.
    struct OrientedRegionUse final {
        bool reversed{};
        bool inner{};
    };
    std::map<std::uint64_t, std::vector<OrientedRegionUse>>
        drilled_native_uses;
    std::size_t drilled_native_outer_wires = 0U;
    std::size_t drilled_native_hole_wires = 0U;
    std::size_t drilled_faces_with_holes = 0U;
    for (const auto token : drilled_side->current_faces) {
        const auto face = std::find_if(
            drilled_catalog.faces.begin(),
            drilled_catalog.faces.end(),
            [token](const auto& item) {
                return item.runtime_token == token;
            });
        CHECK(face != drilled_catalog.faces.end());
        CHECK(face->surface_candidates.size() == 1U);
        CHECK(face->surface_candidates.front() ==
              drilled_side->address);
        const auto scoped =
            provider.bindFaceToBody(drilled.body_solid, token);
        CHECK(scoped && scoped->valid());
        const auto native = provider.queryFaceBoundaryAnySurface(
            drilled.body_solid, *scoped);
        CHECK(native.ok());
        std::size_t face_outer_wires = 0U;
        std::size_t face_hole_wires = 0U;
        for (const auto& wire : native.wires) {
            if (wire.outer) {
                ++face_outer_wires;
                ++drilled_native_outer_wires;
            } else {
                ++face_hole_wires;
                ++drilled_native_hole_wires;
            }
            for (const auto& use : wire.edges) {
                CHECK(use.valid());
                drilled_native_uses[use.edge.value].push_back(
                    OrientedRegionUse{
                        use.reversed,
                        !wire.outer});
            }
        }
        CHECK(face_outer_wires == 1U);
        if (face_hole_wires != 0U) {
            ++drilled_faces_with_holes;
        }
    }
    CHECK(drilled_native_outer_wires ==
          drilled_side->current_faces.size());
    CHECK(drilled_native_hole_wires == 1U);
    CHECK(drilled_faces_with_holes == 1U);

    std::size_t drilled_partition_edges = 0U;
    std::size_t drilled_outer_material = 0U;
    std::size_t drilled_hole_material = 0U;
    std::vector<part::MaterialEdgeReference>
        drilled_unique_material_sources;
    for (const auto& [value, uses] : drilled_native_uses) {
        const kernel::RuntimeEdgeToken token{value};
        const auto edge = std::find_if(
            drilled_catalog.edges.begin(),
            drilled_catalog.edges.end(),
            [token](const auto& item) {
                return item.runtime_token == token;
            });
        CHECK(edge != drilled_catalog.edges.end());
        const auto material = part::authorMaterialEdgeReference(
            drilled_catalog, token);
        if (edge->representation_partition) {
            CHECK(edge->accounting_class ==
                  part::TopologyAccountingClass::
                      known_representation_artifact);
            CHECK(!edge->periodic_seam);
            CHECK(!material.ok());
            CHECK(uses.size() == 2U);
            CHECK(uses[0].reversed != uses[1].reversed);
            CHECK(!uses[0].inner && !uses[1].inner);
            ++drilled_partition_edges;
            continue;
        }
        // Any periodic seam or unknown nonmaterial source is a STOP
        // in this planar case, never an omitted partial material link.
        CHECK(!edge->periodic_seam);
        CHECK(material.ok() && material.reference);
        CHECK(material.reference->stage ==
              drilled_catalog.stage);
        CHECK(uses.size() == 1U);
        drilled_unique_material_sources.push_back(
            *material.reference);
        if (uses.front().inner) {
            ++drilled_hole_material;
        } else {
            ++drilled_outer_material;
        }
    }
    CHECK(drilled_partition_edges >= 1U);
    CHECK(drilled_hole_material >= 1U);
    CHECK(drilled_outer_material >= 1U);
    std::sort(
        drilled_unique_material_sources.begin(),
        drilled_unique_material_sources.end());
    CHECK(std::adjacent_find(
        drilled_unique_material_sources.begin(),
        drilled_unique_material_sources.end()) ==
        drilled_unique_material_sources.end());
    std::cout
        << "PG01D_FACE_BOUNDARY_E0_SPLIT_CARRIER_HOLE_PASS"
        << " same_semantic_surface=1"
        << " native_faces=" << drilled_side->current_faces.size()
        << " native_outer_wires=" << drilled_native_outer_wires
        << " native_hole_wires=" << drilled_native_hole_wires
        << " cancelled_partitions=" << drilled_partition_edges
        << " strict_outer_material_edges=" << drilled_outer_material
        << " strict_hole_material_edges=" << drilled_hole_material
        << " source_geometry_guessing=0"
        << '\\n';
}

} // namespace

int main() {
    kernel_occt::OcctSolidModelingKernel provider;

    kernel::Frame3 xy;
    const auto base =
        provider.extrude(
            add(
                rectangle(
                    xy,
                    0.0,
                    0.0,
                    40.0,
                    30.0,
                    "base"),
                10.0));
    CHECK(base.ok());

    const auto* base_right =
        newSide(
            base,
            "base-right");
    CHECK(base_right != nullptr);
    CHECK(
        base_right->surface_status ==
        kernel::ReferenceStatus::resolved);
    CHECK(base_right->resolved_token.has_value());

    // Boss from the top plane touches the +X outer boundary. Its generated
    // "boss-right" side extends the existing "base-right" engineering
    // Surface upward. Any retained z=10 boundary is a representation
    // partition, not a second semantic Surface.
    kernel::Frame3 top_frame;
    top_frame.origin = {0.0, 0.0, 10.0};
    CHECK(top_frame.valid());

    const auto extended =
        provider.extrude(
            add(
                rectangle(
                    top_frame,
                    20.0,
                    5.0,
                    40.0,
                    25.0,
                    "boss"),
                10.0),
            base.solid);
    CHECK(extended.ok());

    const auto* inherited_right =
        inheritedSurface(
            extended,
            *base_right->resolved_token);
    CHECK(inherited_right != nullptr);
    CHECK(
        inherited_right->surface_status ==
        kernel::ReferenceStatus::resolved);
    CHECK(
        inherited_right->canonical_frame.has_value());

    const auto* created_right =
        newSide(
            extended,
            "boss-right");
    CHECK(created_right != nullptr);
    CHECK(created_right->continued_into.has_value());
    CHECK(
        *created_right->continued_into ==
        *base_right->resolved_token);
    CHECK(
        created_right->surface_status ==
        kernel::ReferenceStatus::unsupported);
    CHECK(!created_right->resolved_token.has_value());
    CHECK(created_right->current_faces.empty());
    CHECK(!created_right->contribution_faces.empty());

    CHECK(inherited_right->current_faces.size() >= 2U);
    CHECK(
        inherited_right->strict_face_status ==
        kernel::ReferenceStatus::ambiguous);

    for (const auto token :
         created_right->contribution_faces) {
        CHECK(
            std::find(
                inherited_right->current_faces.begin(),
                inherited_right->current_faces.end(),
                token) !=
            inherited_right->current_faces.end());
    }

    std::size_t partition_edges = 0U;
    for (const auto& edge :
         extended.current_edge_semantics) {
        if (!edge.same_surface_partition) {
            continue;
        }
        ++partition_edges;
        CHECK(!edge.periodic_seam);
        CHECK(edge.adjacent_surfaces.size() == 1U);
        CHECK(
            edge.adjacent_surfaces.front() ==
            *base_right->resolved_token);
    }
    CHECK(partition_edges >= 1U);

    verifyPartIntegration();

    std::cout
        << "PM02JR2_ADD_SURFACE_CONTINUATION_PASS"
        << " inherited_side_faces="
        << inherited_right->current_faces.size()
        << " contribution_faces="
        << created_right->contribution_faces.size()
        << " partition_edges="
        << partition_edges
        << " geometry_similarity_authority=0"
        << '\n';

    return EXIT_SUCCESS;
}
