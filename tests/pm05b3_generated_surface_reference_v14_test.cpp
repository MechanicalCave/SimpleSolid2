#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/part/datum.hpp>
#include <simplesolid2/part/feature.hpp>
#include <simplesolid2/part/part_document_store.hpp>
#include <simplesolid2/part/part_sketch.hpp>
#include <simplesolid2/persistence/native_document_container.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-05B3 generated-Surface CHECK failed at line "
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
            ("simplesolid2_pm05b3_" +
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

void writeBytes(
    const std::filesystem::path& path,
    const std::string& bytes) {
    std::ofstream out{
        path,
        std::ios::binary | std::ios::trunc};
    CHECK(static_cast<bool>(out));
    out.write(
        bytes.data(),
        static_cast<std::streamsize>(
            bytes.size()));
    CHECK(static_cast<bool>(out));
}

std::string repackage(
    const persistence::NativeDocumentPackage& package,
    std::string authored_json) {
    const auto built =
        persistence::buildNativeDocumentContainer(
            persistence::NativeDocumentDescriptor{
                "part",
                package.descriptor.document_id,
                14},
            std::move(authored_json));
    CHECK(built.ok());
    return std::move(*built.bytes);
}

part::FeatureSurfaceAddress capSurface(
    part::FeatureId producer) {
    part::FeatureSurfaceAddress result{
        producer,
        part::FeatureSurfaceRoleKind::extent_cap,
        std::nullopt,
        0U,
        0U,
        false};
    CHECK(result.valid());
    return result;
}

part::FeatureSurfaceAddress sideSurface(
    part::FeatureId producer,
    sketch::EntityId source,
    std::uint32_t use_index) {
    part::FeatureSurfaceAddress result{
        producer,
        part::FeatureSurfaceRoleKind::side,
        source,
        0U,
        use_index,
        false};
    CHECK(result.valid());
    return result;
}

part::FeatureCurveAddress curveBetween(
    part::FeatureId producer,
    part::FeatureCurveRoleKind role,
    part::FeatureSurfaceAddress first,
    part::FeatureSurfaceAddress second) {
    std::vector<part::FeatureSurfaceAddress>
        surfaces{
            std::move(first),
            std::move(second)};
    std::sort(
        surfaces.begin(),
        surfaces.end());
    part::FeatureCurveAddress result{
        producer,
        role,
        std::move(surfaces)};
    CHECK(result.valid());
    return result;
}

part::FeaturePointAddress pointAt(
    part::FeatureId producer,
    part::FeatureSurfaceAddress first,
    part::FeatureSurfaceAddress second,
    part::FeatureSurfaceAddress third) {
    std::vector<part::FeatureSurfaceAddress>
        surfaces{
            std::move(first),
            std::move(second),
            std::move(third)};
    std::sort(
        surfaces.begin(),
        surfaces.end());
    part::FeaturePointAddress result{
        producer,
        std::move(surfaces)};
    CHECK(result.valid());
    return result;
}

part::MaterialEdgeReference singularEdge(
    part::FeatureId stage,
    part::FeatureCurveAddress curve) {
    part::MaterialEdgeReference result{
        {
            part::BodyStageKind::after_feature,
            stage},
        std::move(curve),
        part::SingularAtAuthoredStage{}};
    CHECK(result.valid());
    return result;
}

nlohmann::json* findSurfaceByRole(
    nlohmann::json& value,
    const std::string& role) {
    if (value.is_object()) {
        if (value.contains("role") &&
            value["role"].is_string() &&
            value["role"].get<std::string>() ==
                role) {
            return &value;
        }
        for (auto& [key, child] :
             value.items()) {
            (void)key;
            if (auto* found =
                    findSurfaceByRole(
                        child,
                        role)) {
                return found;
            }
        }
    } else if (value.is_array()) {
        for (auto& child : value) {
            if (auto* found =
                    findSurfaceByRole(
                        child,
                        role)) {
                return found;
            }
        }
    }
    return nullptr;
}

struct Fixture final {
    part::PartDocument document;
    part::FeatureId base_id;
    part::FeatureId fillet_id;
    part::FeatureId chamfer_id;
    part::FeatureSurfaceAddress fillet_surface;
    part::FeatureSurfaceAddress corner_surface;
    part::FeatureSurfaceAddress chamfer_surface;
};

Fixture makeFixture() {
    auto source =
        part::PartDocument::create(
            core::DocumentId::generate());
    application::DocumentSession session{
        {},
        std::move(source)};

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
    const auto regions =
        sketch::analyzeRegions(
            sketch->model);
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
    CHECK(profile.ok() && profile.profile_id);

    auto state =
        session.document().state();
    const auto base_id =
        state.body.next_feature_id.allocate();
    const auto fillet_id =
        state.body.next_feature_id.allocate();
    const auto chamfer_id =
        state.body.next_feature_id.allocate();
    CHECK(base_id && fillet_id && chamfer_id);

    state.body.features.push_back(
        part::PartFeature{
            *base_id,
            "Base",
            false,
            part::ExtrudeFeature{
                *profile.profile_id,
                part::ExtrudeOperation::add,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{10.0},
                    false}}});

    const auto cap =
        capSurface(*base_id);
    const auto side0 =
        sideSurface(
            *base_id,
            rectangle.entity_ids.at(0),
            0U);
    const auto side1 =
        sideSurface(
            *base_id,
            rectangle.entity_ids.at(1),
            1U);
    const auto side2 =
        sideSurface(
            *base_id,
            rectangle.entity_ids.at(2),
            2U);

    auto edge0 =
        singularEdge(
            *base_id,
            curveBetween(
                *base_id,
                part::FeatureCurveRoleKind::cap_side,
                cap,
                side0));
    auto edge1 =
        singularEdge(
            *base_id,
            curveBetween(
                *base_id,
                part::FeatureCurveRoleKind::cap_side,
                cap,
                side1));

    std::vector<part::MaterialEdgeReference>
        fillet_edges{edge1, edge0};
    std::sort(
        fillet_edges.begin(),
        fillet_edges.end());

    state.body.features.push_back(
        part::PartFeature{
            *fillet_id,
            "Fillet001",
            false,
            part::FilletFeature{
                fillet_edges,
                core::LengthValue{2.0}}});

    part::FeatureSurfaceAddress
        fillet_surface{
            *fillet_id,
            part::FeatureSurfaceRoleKind::
                fillet_surface,
            std::nullopt,
            0U,
            0U,
            false};
    fillet_surface.source_edges =
        {edge0};
    CHECK(fillet_surface.valid());

    auto corner_point =
        pointAt(
            *base_id,
            cap,
            side0,
            side1);

    part::FeatureSurfaceAddress
        corner_surface{
            *fillet_id,
            part::FeatureSurfaceRoleKind::
                corner_transition,
            std::nullopt,
            0U,
            0U,
            false};
    corner_surface.source_edges =
        fillet_edges;
    corner_surface.source_points =
        {corner_point};
    CHECK(corner_surface.valid());

    auto generated_edge =
        singularEdge(
            *fillet_id,
            curveBetween(
                *fillet_id,
                part::FeatureCurveRoleKind::
                    edge_feature_boundary,
                fillet_surface,
                side0));
    auto corner_boundary =
        singularEdge(
            *fillet_id,
            curveBetween(
                *fillet_id,
                part::FeatureCurveRoleKind::
                    edge_feature_boundary,
                fillet_surface,
                corner_surface));

    std::vector<part::MaterialEdgeReference>
        chamfer_edges{
            generated_edge,
            corner_boundary};
    std::sort(
        chamfer_edges.begin(),
        chamfer_edges.end());

    state.body.features.push_back(
        part::PartFeature{
            *chamfer_id,
            "Chamfer001",
            false,
            part::ChamferFeature{
                chamfer_edges,
                core::LengthValue{1.0}}});

    part::FeatureSurfaceAddress
        chamfer_surface{
            *chamfer_id,
            part::FeatureSurfaceRoleKind::
                chamfer_surface,
            std::nullopt,
            0U,
            0U,
            false};
    chamfer_surface.source_edges =
        {generated_edge};
    CHECK(chamfer_surface.valid());

    const part::SurfaceReference
        generated_plane{
            {
                part::BodyStageKind::after_feature,
                *chamfer_id},
            chamfer_surface};
    CHECK(generated_plane.valid());

    const auto generated_support =
        part::partSketchSupportForBodyPlanarSurface(
            generated_plane);
    CHECK(generated_support.has_value());
    state.sketches.push_back(
        part::PartSketch{
            sketch::SketchId::generate(),
            *generated_support,
            true,
            {}});

    const auto datum_id =
        state.next_datum_id.allocate();
    CHECK(datum_id.has_value());
    state.datum_planes.push_back(
        part::OffsetDatumPlane{
            *datum_id,
            part::PlaneReference{
                part::BodyPlanarSurfacePlaneReference{
                    generated_plane}},
            core::LengthValue{5.0},
            true});

    auto restored =
        part::PartDocument::restore(
            session.document().documentId(),
            std::move(state),
            session.document().revision());
    CHECK(restored.ok());

    return {
        std::move(*restored.document),
        *base_id,
        *fillet_id,
        *chamfer_id,
        std::move(fillet_surface),
        std::move(corner_surface),
        std::move(chamfer_surface)};
}

} // namespace

int main() {
    CHECK(
        part::PartDocumentStore::current_schema_version ==
        14);

    const auto fixture =
        makeFixture();

    TempDirectory temp;
    part::PartDocumentStore store;
    const auto path =
        temp.path /
        "GeneratedSurfaceReferences.ss2part";
    CHECK(
        store.createNew(
            path,
            fixture.document)
            .ok());

    const auto package =
        persistence::readNativeDocumentContainer(
            path);
    CHECK(package.ok());
    CHECK(
        package.package->descriptor
            .domain_schema_version == 15);

    auto authored =
        nlohmann::json::parse(
            package.package->authored_json);

    auto* fillet_surface =
        findSurfaceByRole(
            authored,
            "fillet_surface");
    auto* corner_surface =
        findSurfaceByRole(
            authored,
            "corner_transition");
    auto* chamfer_surface =
        findSurfaceByRole(
            authored,
            "chamfer_surface");
    CHECK(fillet_surface != nullptr);
    CHECK(corner_surface != nullptr);
    CHECK(chamfer_surface != nullptr);

    CHECK(
        fillet_surface->contains(
            "source_edge"));
    CHECK(
        corner_surface->contains(
            "source_point"));
    CHECK(
        corner_surface->contains(
            "incident_edges"));
    CHECK(
        (*corner_surface)["incident_edges"]
            .size() == 2U);
    CHECK(
        chamfer_surface->contains(
            "source_edge"));

    CHECK(
        package.package->authored_json.find(
            "runtime_token") ==
        std::string::npos);
    CHECK(
        package.package->authored_json.find(
            "TopoDS") ==
        std::string::npos);

    const auto loaded =
        store.load(path);
    CHECK(loaded.ok());
    CHECK(
        loaded.document->state() ==
        fixture.document.state());

    // Both body-planar Sketch support and Datum Plane source keep the full
    // generated-Surface provenance through current schema-v15 persistence.
    CHECK(
        loaded.document->sketches().size() ==
        fixture.document.sketches().size());
    CHECK(
        loaded.document->datumPlanes().size() ==
        fixture.document.datumPlanes().size());

    // P3 incident authored Edge set is canonical and deduplicated.
    {
        auto bad = authored;
        auto* corner =
            findSurfaceByRole(
                bad,
                "corner_transition");
        CHECK(corner != nullptr);
        (*corner)["incident_edges"].push_back(
            (*corner)["incident_edges"].at(0));
        auto text = bad.dump(2);
        text.push_back('\n');
        const auto bad_path =
            temp.path /
            "DuplicateCornerIncidentEdge.ss2part";
        writeBytes(
            bad_path,
            repackage(
                *package.package,
                std::move(text)));
        CHECK(!store.load(bad_path).ok());
    }

    // A transition Surface cannot claim an Edge from its own/future stage.
    {
        auto bad = authored;
        auto* blend =
            findSurfaceByRole(
                bad,
                "fillet_surface");
        CHECK(blend != nullptr);
        (*blend)["source_edge"]["stage"]
                ["feature_id"] =
            fixture.fillet_id.serialized();
        auto text = bad.dump(2);
        text.push_back('\n');
        const auto bad_path =
            temp.path /
            "SelfStageGeneratedSurface.ss2part";
        writeBytes(
            bad_path,
            repackage(
                *package.package,
                std::move(text)));
        CHECK(!store.load(bad_path).ok());
    }

    // Missing accepted P2 provenance fails closed.
    {
        auto bad = authored;
        auto* blend =
            findSurfaceByRole(
                bad,
                "fillet_surface");
        CHECK(blend != nullptr);
        blend->erase("source_edge");
        auto text = bad.dump(2);
        text.push_back('\n');
        const auto bad_path =
            temp.path /
            "MissingGeneratedSurfaceSource.ss2part";
        writeBytes(
            bad_path,
            repackage(
                *package.package,
                std::move(text)));
        CHECK(!store.load(bad_path).ok());
    }

    std::cout
        << "PM05B3_GENERATED_SURFACE_REFERENCE_V14_PASS"
        << " p2=1"
        << " p3=1"
        << " chained_edge_intent=1"
        << " sketch_support_roundtrip=1"
        << " datum_support_roundtrip=1"
        << " provider_identity_persisted=0\n";
    return EXIT_SUCCESS;
}
