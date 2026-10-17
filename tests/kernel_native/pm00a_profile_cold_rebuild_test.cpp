#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel/evidence.hpp>
#include <simplesolid2/kernel/profile_input.hpp>
#include <simplesolid2/kernel_occt/profile_face_evidence.hpp>
#include <simplesolid2/kernel_occt/solid_modeling_kernel.hpp>
#include <simplesolid2/part/part_document_store.hpp>
#include <simplesolid2/part/profile_kernel_input.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-00A cold rebuild CHECK failed at line "
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
            ("simplesolid2_pm00a_cold_rebuild_" +
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

void appendPoint(
    std::ostringstream& out,
    const kernel::Point2& point) {
    out << point.u << ',' << point.v;
}

void appendCurve(
    std::ostringstream& out,
    const kernel::Curve2& curve) {
    std::visit(
        [&out](const auto& value) {
            using T =
                std::decay_t<decltype(value)>;
            if constexpr (
                std::is_same_v<T, kernel::Line2>) {
                out << "L(";
                appendPoint(out, value.start);
                out << "->";
                appendPoint(out, value.end);
                out << ')';
            } else if constexpr (
                std::is_same_v<T, kernel::Circle2>) {
                out << "C(";
                appendPoint(out, value.center);
                out << ";r=" << value.radius << ')';
            } else {
                out << "A(";
                appendPoint(out, value.center);
                out << ";r=" << value.radius
                    << ";a=" << value.start_angle
                    << ";s=" << value.sweep_angle
                    << ')';
            }
        },
        curve);
}

std::string fingerprint(
    const kernel::PlanarProfileInput& input) {
    std::ostringstream out;
    out << std::setprecision(17);
    out << "O="
        << input.frame.origin.x << ','
        << input.frame.origin.y << ','
        << input.frame.origin.z
        << ";U="
        << input.frame.u_axis.x << ','
        << input.frame.u_axis.y << ','
        << input.frame.u_axis.z
        << ";V="
        << input.frame.v_axis.x << ','
        << input.frame.v_axis.y << ','
        << input.frame.v_axis.z
        << ";N="
        << input.frame.normal.x << ','
        << input.frame.normal.y << ','
        << input.frame.normal.z;

    const auto append_loop =
        [&out](const kernel::ProfileLoopInput& loop) {
            out << '[';
            for (const auto& use : loop.boundary) {
                out << '{'
                    << use.provenance.source_entity
                    << ':'
                    << use.provenance.loop_index
                    << ':'
                    << use.provenance.use_index
                    << ':'
                    << use.provenance.hole
                    << ';';
                appendCurve(out, use.curve);
                out << ";p="
                    << use.start_parameter
                    << ','
                    << use.end_parameter
                    << ";f="
                    << use.follows_source_direction
                    << ";seam="
                    << use.crosses_closed_seam
                    << ";whole="
                    << use.whole_closed_curve
                    << '}';
            }
            out << ']';
        };

    out << ";outer=";
    append_loop(input.outer);
    out << ";holes=" << input.holes.size();
    for (const auto& hole : input.holes) {
        append_loop(hole);
    }
    return out.str();
}

struct EvidenceSnapshot final {
    std::size_t face_count{};
    std::size_t wire_count{};
    std::size_t edge_count{};
    std::vector<kernel::BoundaryLineageEvidence>
        lineage;

    friend bool operator==(
        const EvidenceSnapshot&,
        const EvidenceSnapshot&) = default;
};

EvidenceSnapshot snapshot(
    const kernel::ShapeEvidence& evidence) {
    return {
        evidence.face_count,
        evidence.wire_count,
        evidence.edge_count,
        evidence.boundary_lineage,
    };
}

// Native S1/F2 witness: two exact interior Line/Circle tangencies form
// a durable lower pocket. Release all runtime Sketch/Profile/OCCT handles,
// then cold-rebuild the Profile Face from the authored .ss2part payload.
void verifyF2TangentPocketColdRebuild(
    bool near_contact, bool contact_interior = false) {
    TempDirectory temp;
    const auto path = temp.path /
        (contact_interior
            ? "PM00AN1InteriorTangentPocket.ss2part"
            : (near_contact
                ? "PM00AN1NearTangentPocket.ss2part"
                : "PM00AF2TangentPocket.ss2part"));
    part::PartDocumentStore store;
    part::ProfileId pocket_profile;
    std::optional<sketch::SketchId> sketch_id;
    std::string before_fingerprint;
    EvidenceSnapshot before_evidence;

    {
        auto document = part::PartDocument::create(
            core::DocumentId::generate());
        const auto published = store.createNew(path, document);
        CHECK(published.ok());
        application::DocumentSession session{
            path, std::move(document), *published.checkpoint};

        const auto created = session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
        CHECK(created.ok() && created.sketch_id.has_value());
        sketch_id = *created.sketch_id;

        const sketch::Point2 center =
            near_contact
                ? sketch::Point2{
                    190.34517723877235, 67.52296383563574}
                : sketch::Point2{0.0, 0.0};
        const double radius = near_contact ? 30.0 : 5.0;
        const double x_left = near_contact
            ? std::nextafter(center.u - radius,
                             -std::numeric_limits<double>::infinity())
            : -5.0;
        const double x_right = near_contact
            ? std::nextafter(center.u + radius,
                             std::numeric_limits<double>::infinity())
            : 5.0;
        const double y_base =
            near_contact ? center.v - 60.0 : -9.0;
        const double y_top = near_contact
            ? center.v + (contact_interior ? 20.0 : 0.0)
            : 7.0;
        const auto circle = session.execute(
            application::AddSketchCircleCommand{
                *sketch_id, center, radius});
        CHECK(circle.ok() && circle.entity_id.has_value());
        const auto left = session.execute(
            application::AddSketchLineCommand{
                *sketch_id,
                {x_left, y_base}, {x_left, y_top}});
        const auto right = session.execute(
            application::AddSketchLineCommand{
                *sketch_id,
                {x_right, y_base}, {x_right, y_top}});
        const auto base = session.execute(
            application::AddSketchLineCommand{
                *sketch_id,
                {x_left, y_base}, {x_right, y_base}});
        CHECK(left.ok() && right.ok() && base.ok());

        const auto* source =
            session.document().findSketch(*sketch_id);
        CHECK(source != nullptr);
        const auto regions = sketch::analyzeRegions(source->model);
        CHECK(regions.regions.size() == 2U);
        const auto picked = sketch::pickRegion(
            source->model, regions,
            {center.u, near_contact ? center.v - 50.0 : -8.0});
        CHECK(picked.region_index.has_value());
        const auto region = std::find_if(
            regions.regions.begin(), regions.regions.end(),
            [&picked](const auto& candidate) {
                return candidate.region_index ==
                       *picked.region_index;
            });
        CHECK(region != regions.regions.end());
        const auto intent =
            part::makeProfileRegionIntent(*region);
        CHECK(intent.has_value());
        const auto replay = part::resolveProfileRegionIntent(
            source->model, *intent);
        CHECK(replay.valid());

        const auto profile = session.execute(
            application::CreateProfileCommand{
                *sketch_id, session.document().revision(), *intent});
        CHECK(profile.ok() && profile.profile_id.has_value());
        pocket_profile = *profile.profile_id;

        const auto input = part::makeKernelProfileInput(
            session.document(), pocket_profile);
        CHECK(input.has_value() && input->valid());
        const auto face =
            kernel_occt::buildProfileFaceEvidence(*input);
        CHECK(face.ok());
        CHECK(face.face_count == 1U);
        CHECK(face.wire_count == 1U);

        // True native OCCT solid modeling, not a WorkflowKernel mock.
        kernel_occt::OcctSolidModelingKernel native_provider;
        kernel::LinearExtrudeInput extrusion;
        extrusion.profile = *input;
        extrusion.start_offset_mm = 0.0;
        extrusion.end_offset_mm = 10.0;
        CHECK(extrusion.valid());
        const auto solid = native_provider.extrude(extrusion);
        CHECK(solid.status == kernel::SolidModelingStatus::ok);
        CHECK(solid.brep_valid);
        CHECK(solid.solid_count == 1U);
        CHECK(solid.solid != nullptr);

        before_fingerprint = fingerprint(*input);
        before_evidence = snapshot(face);
        CHECK(session.save().ok());
    }

    // A new document load and entirely fresh OCCT B-Rep evidence:
    // no cached fragment, OCCT face, profile or session survives.
    const auto loaded = store.load(path);
    CHECK(loaded.ok());
    CHECK(loaded.document->findSketch(*sketch_id) != nullptr);
    CHECK(loaded.document->findProfile(pocket_profile) != nullptr);
    const auto input = part::makeKernelProfileInput(
        *loaded.document, pocket_profile);
    CHECK(input.has_value() && input->valid());
    CHECK(fingerprint(*input) == before_fingerprint);
    const auto cold_face =
        kernel_occt::buildProfileFaceEvidence(*input);
    CHECK(cold_face.ok());
    CHECK(snapshot(cold_face) == before_evidence);
    // The same authored Profile produces a fresh native Body after reopen.
    kernel_occt::OcctSolidModelingKernel cold_provider;
    kernel::LinearExtrudeInput cold_extrusion;
    cold_extrusion.profile = *input;
    cold_extrusion.start_offset_mm = 0.0;
    cold_extrusion.end_offset_mm = 10.0;
    CHECK(cold_extrusion.valid());
    const auto cold_solid = cold_provider.extrude(cold_extrusion);
    CHECK(cold_solid.status == kernel::SolidModelingStatus::ok);
    CHECK(cold_solid.brep_valid);
    CHECK(cold_solid.solid_count == 1U);
    CHECK(cold_solid.solid != nullptr);

    std::cout << "PM00A_F2_TANGENT_POCKET_COLD_PASS"
              << " native_occt_face=1 native_occt_solid=1"
              << " cold_reopen=1\n";
}

// S1/N1 D2-A real native acceptance: Circle 9 with two near-secant
// diagonals (authored Extend Both shared apex). Two honest intersection
// branches survive; unsupported microlenses are explicitly diagnostic,
// but the lower macroregion must create a durable OCCT Face and solid.
void verifyD2AOwnerExtendedSecantColdRebuild() {
    TempDirectory temp;
    const auto path =
        temp.path / "PM00AD2AOwnerExtendedSecants.ss2part";
    part::PartDocumentStore store;
    part::ProfileId selected_profile;
    std::optional<sketch::SketchId> sketch_id;
    std::string before_fingerprint;
    EvidenceSnapshot before_evidence;
    const sketch::Point2 center{
        89.16841975020736, 67.52296383563574};
    const double left_x = 49.16841975020738;
    const double right_x = 129.16841975020736;
    const double top_y = 60.507216219488555;
    const sketch::Point2 apex{
        89.16841975020735, 123.46646180728104};
    {
        auto document = part::PartDocument::create(
            core::DocumentId::generate());
        const auto created_file =
            store.createNew(path, document);
        CHECK(created_file.ok());
        application::DocumentSession session{
            path, std::move(document),
            *created_file.checkpoint};
        const auto sketch_create = session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
        CHECK(sketch_create.ok() &&
              sketch_create.sketch_id.has_value());
        sketch_id = *sketch_create.sketch_id;
        const auto circle = session.execute(
            application::AddSketchCircleCommand{
                *sketch_id, center, 30.0});
        CHECK(circle.ok() && circle.entity_id.has_value());
        const auto add_line =
            [&session, &sketch_id](
                sketch::Point2 first, sketch::Point2 second) {
                return session.execute(
                    application::AddSketchLineCommand{
                        *sketch_id, first, second});
            };
        CHECK(add_line({left_x, 0.0},
                       {right_x, 0.0}).ok());
        CHECK(add_line({left_x, 0.0},
                       {left_x, top_y}).ok());
        CHECK(add_line({left_x, top_y}, apex).ok());
        CHECK(add_line({right_x, 0.0},
                       {right_x, top_y}).ok());
        CHECK(add_line({right_x, top_y}, apex).ok());

        const auto* source =
            session.document().findSketch(*sketch_id);
        CHECK(source != nullptr);
        const auto analysis =
            sketch::analyzeRegions(source->model);
        CHECK(!analysis.complete());
        const auto picked = sketch::pickRegion(
            source->model, analysis, {center.u, 20.0});
        CHECK(picked.region_index.has_value());
        const auto region = std::find_if(
            analysis.regions.begin(), analysis.regions.end(),
            [&picked](const auto& candidate) {
                return candidate.region_index ==
                    *picked.region_index;
            });
        CHECK(region != analysis.regions.end());
        const auto intent =
            part::makeProfileRegionIntent(*region);
        CHECK(intent.has_value());
        CHECK(part::resolveProfileRegionIntent(
            source->model, *intent).valid());
        const auto profile = session.execute(
            application::CreateProfileCommand{
                *sketch_id, session.document().revision(),
                *intent});
        CHECK(profile.ok() && profile.profile_id.has_value());
        selected_profile = *profile.profile_id;

        const auto input = part::makeKernelProfileInput(
            session.document(), selected_profile);
        CHECK(input.has_value() && input->valid());
        const auto face =
            kernel_occt::buildProfileFaceEvidence(*input);
        CHECK(face.ok());
        CHECK(face.face_count == 1U);
        CHECK(face.wire_count == 1U);

        kernel_occt::OcctSolidModelingKernel provider;
        kernel::LinearExtrudeInput extrusion;
        extrusion.profile = *input;
        extrusion.start_offset_mm = 0.0;
        extrusion.end_offset_mm = 10.0;
        CHECK(extrusion.valid());
        const auto solid = provider.extrude(extrusion);
        CHECK(solid.status == kernel::SolidModelingStatus::ok);
        CHECK(solid.brep_valid);
        CHECK(solid.solid_count == 1U);

        before_fingerprint = fingerprint(*input);
        before_evidence = snapshot(face);
        CHECK(session.save().ok());
    }
    const auto cold = store.load(path);
    CHECK(cold.ok());
    CHECK(cold.document->findSketch(*sketch_id) != nullptr);
    CHECK(cold.document->findProfile(selected_profile) != nullptr);
    const auto input = part::makeKernelProfileInput(
        *cold.document, selected_profile);
    CHECK(input.has_value() && input->valid());
    CHECK(fingerprint(*input) == before_fingerprint);
    const auto face =
        kernel_occt::buildProfileFaceEvidence(*input);
    CHECK(face.ok());
    CHECK(snapshot(face) == before_evidence);
    kernel_occt::OcctSolidModelingKernel cold_provider;
    kernel::LinearExtrudeInput cold_extrusion;
    cold_extrusion.profile = *input;
    cold_extrusion.start_offset_mm = 0.0;
    cold_extrusion.end_offset_mm = 10.0;
    CHECK(cold_extrusion.valid());
    const auto solid = cold_provider.extrude(cold_extrusion);
    CHECK(solid.status == kernel::SolidModelingStatus::ok);
    CHECK(solid.brep_valid);
    CHECK(solid.solid_count == 1U);
    std::cout << "PM00A_D2A_OWNER_SECTANTS_NATIVE_COLD_PASS"
              << " source_crossings_distinct=1"
              << " profile_face=1 extrude=1 cold_reopen=1\n";
}

} // namespace

int main() {
    verifyD2AOwnerExtendedSecantColdRebuild();
    verifyF2TangentPocketColdRebuild(false);
    verifyF2TangentPocketColdRebuild(true);
    verifyF2TangentPocketColdRebuild(true, true);
    TempDirectory temp;
    const auto path =
        temp.path / "PM00AColdRebuild.ss2part";

    part::PartDocumentStore store;
    part::ProfileId profile_id;
    std::optional<sketch::SketchId> sketch_id;
    std::string before_fingerprint;
    EvidenceSnapshot before_evidence;

    {
        auto document =
            part::PartDocument::create(
                core::DocumentId::generate());
        const auto published =
            store.createNew(path, document);
        CHECK(published.ok());

        application::DocumentSession session{
            path,
            std::move(document),
            *published.checkpoint};

        const auto sketch_created =
            session.execute(
                application::CreatePartSketchCommand{
                    core::BuiltinReferenceRole::xy_plane});
        CHECK(
            sketch_created.ok() &&
            sketch_created.sketch_id.has_value());
        sketch_id =
            *sketch_created.sketch_id;

        const auto rectangle =
            session.execute(
                application::AddSketchRectangleCommand{
                    *sketch_id,
                    session.document().revision(),
                    {0.0, 0.0},
                    {40.0, 30.0},
                    sketch::EntityRole::regular,
                    false});
        CHECK(rectangle.ok());
        CHECK(rectangle.changed);
        CHECK(rectangle.entity_ids.size() == 4U);

        const auto circle =
            session.execute(
                application::AddSketchCircleCommand{
                    *sketch_id,
                    {20.0, 15.0},
                    5.0});
        CHECK(
            circle.ok() &&
            circle.entity_id.has_value());

        const auto* source =
            session.document().findSketch(
                *sketch_id);
        CHECK(source != nullptr);

        const auto analysis =
            sketch::analyzeRegions(
                source->model);
        CHECK(analysis.complete());

        const auto pick =
            sketch::pickRegion(
                source->model,
                analysis,
                {2.0, 2.0});
        CHECK(
            pick.region_index.has_value());

        const auto region =
            std::find_if(
                analysis.regions.begin(),
                analysis.regions.end(),
                [&pick](
                    const sketch::RegionCandidate2D&
                        candidate) {
                    return candidate.region_index ==
                           *pick.region_index;
                });
        CHECK(region != analysis.regions.end());
        CHECK(region->holes.size() == 1U);

        const auto intent =
            part::makeProfileRegionIntent(
                *region);
        CHECK(intent.has_value());

        const auto created =
            session.execute(
                application::CreateProfileCommand{
                    *sketch_id,
                    session.document().revision(),
                    *intent});
        CHECK(
            created.ok() &&
            created.profile_id.has_value());
        profile_id =
            *created.profile_id;

        const auto input =
            part::makeKernelProfileInput(
                session.document(),
                profile_id);
        CHECK(input.has_value());
        CHECK(input->valid());
        CHECK(input->outer.boundary.size() == 4U);
        CHECK(input->holes.size() == 1U);
        CHECK(
            input->holes.front()
                .boundary.size() == 1U);

        const auto evidence =
            kernel_occt::buildProfileFaceEvidence(
                *input);
        CHECK(evidence.ok());
        CHECK(evidence.face_count == 1U);
        CHECK(evidence.wire_count == 2U);
        CHECK(evidence.edge_count == 5U);
        CHECK(
            evidence.boundary_lineage.size() ==
            5U);
        CHECK(
            std::all_of(
                evidence.boundary_lineage.begin(),
                evidence.boundary_lineage.end(),
                [](const auto& item) {
                    return item.generated_edge_count ==
                           1U &&
                           !item.provenance
                                .source_entity
                                .empty();
                }));

        before_fingerprint =
            fingerprint(*input);
        before_evidence =
            snapshot(evidence);

        CHECK(session.save().ok());
        CHECK(!session.needsSave());
    }

    // Everything above this point that can carry runtime DocumentSession,
    // evaluated Profile, OCCT B-Rep/provider object or transient Kernel input
    // has gone out of scope. Rebuild only from the native authored document.
    const auto loaded =
        store.load(path);
    CHECK(loaded.ok());
    CHECK(
        loaded.document->findSketch(
            *sketch_id) != nullptr);
    CHECK(
        loaded.document->findProfile(
            profile_id) != nullptr);

    const auto rebuilt_input =
        part::makeKernelProfileInput(
            *loaded.document,
            profile_id);
    CHECK(rebuilt_input.has_value());
    CHECK(rebuilt_input->valid());
    CHECK(
        fingerprint(*rebuilt_input) ==
        before_fingerprint);

    const auto rebuilt_evidence =
        kernel_occt::buildProfileFaceEvidence(
            *rebuilt_input);
    CHECK(rebuilt_evidence.ok());
    CHECK(
        snapshot(rebuilt_evidence) ==
        before_evidence);

    return EXIT_SUCCESS;
}
