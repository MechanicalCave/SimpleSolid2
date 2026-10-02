#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel/evidence.hpp>
#include <simplesolid2/kernel/profile_input.hpp>
#include <simplesolid2/kernel_occt/profile_face_evidence.hpp>
#include <simplesolid2/part/part_document_store.hpp>
#include <simplesolid2/part/profile_kernel_input.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <algorithm>
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

} // namespace

int main() {
    TempDirectory temp;
    const auto path =
        temp.path / "PM00AColdRebuild.ss2part";

    part::PartDocumentStore store;
    part::ProfileId profile_id;
    sketch::SketchId sketch_id;
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
                    sketch_id,
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
                    sketch_id,
                    {20.0, 15.0},
                    5.0});
        CHECK(
            circle.ok() &&
            circle.entity_id.has_value());

        const auto* source =
            session.document().findSketch(
                sketch_id);
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
                    sketch_id,
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
            sketch_id) != nullptr);
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
