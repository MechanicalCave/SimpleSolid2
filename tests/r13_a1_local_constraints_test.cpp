#if defined(SS2_R13_A1_PLANEGCS_NATIVE_EVIDENCE) || \
    defined(SS2_R13_A1_PLANEGCS_OCCT_EVIDENCE)
#include <simplesolid2/sketch/planegcs_dev_solver.hpp>
#endif
#if defined(SS2_R13_A1_OCCT_EVIDENCE) || \
    defined(SS2_R13_A1_PLANEGCS_OCCT_EVIDENCE)
#include <simplesolid2/kernel_occt/profile_face_evidence.hpp>
#include <simplesolid2/kernel_occt/solid_modeling_kernel.hpp>
#include <simplesolid2/part/part_document_store.hpp>
#include <numbers>
#endif

// The original-source native Core-only target cannot depend on the
// Application/Part headers. The normal Sketch/Part/OCCT tests retain them.
#if !defined(SS2_R13_A1_PLANEGCS_NATIVE_EVIDENCE)
#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/part/profile_kernel_input.hpp>
#include <simplesolid2/part/effective_sketch_projection.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>
#endif
#include <simplesolid2/sketch/sketch_model.hpp>
#include <simplesolid2/sketch/local_solver.hpp>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <set>
#include <utility>

namespace sk = simplesolid2::sketch;

namespace {
void check(bool condition, const char* expression, int line) {
    if (!condition) {
        std::cerr << "R13-A1 Horizontal CHECK failed at " << line
                  << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(x) check(static_cast<bool>(x), #x, __LINE__)
} // namespace

#if defined(SS2_R13_A1_PLANEGCS_NATIVE_EVIDENCE)
int main() {
    using K = sk::ConstraintKind;
    using E = sk::ConstraintElement;
    constexpr E L=E::line, S=E::line_start, T=E::line_end;
    const sk::LocalSolverAcceptancePolicy policy{1.0e-7,1.0e-6};
    sk::PlaneGcsDevSolver native;
    unsigned passed = 0;

    // R13-A2.0 real original-source PlaneGCS numerical preflight:
    // one click may prepare, but never publish an unsolved Part relation.
    {
        sk::SketchModel current;
        const auto line = current.addLine({0.0,0.0},{5.0,2.0});
        const auto baseline = current.state();
        const auto first = sk::ConstraintReference{line,L};
        const auto preview = sk::preflightLocalRelation(
            baseline, K::horizontal, first, {},
            {}, policy, native);
        CHECK(preview.status ==
              sk::LocalRelationPreflightStatus::candidate_accepted);
        CHECK(preview.provisional_constraint_id);
        CHECK(preview.proposed_authored);
        CHECK(preview.solve.candidate);
        CHECK(preview.solve.status ==
              sk::LocalSolveGatewayStatus::candidate_accepted);
        CHECK(preview.proposed_authored->constraints.size() == 1U);
        CHECK(preview.proposed_authored->constraints.front().id ==
              *preview.provisional_constraint_id);
        CHECK(preview.proposed_authored->constraints.front().kind ==
              K::horizontal);
        CHECK(std::abs(preview.solve.candidate->lines.front().start.v -
                       preview.solve.candidate->lines.front().end.v) <
              policy.distance_tolerance);
        CHECK(current.state() == baseline);
        CHECK(baseline.constraints.empty());
        CHECK(baseline.next_constraint_id == current.state().next_constraint_id);

        // A semantic duplicate is rejected before the backend is invoked.
        const auto duplicate = sk::preflightLocalRelation(
            *preview.proposed_authored, K::horizontal, first, {},
            {}, policy, native);
        CHECK(duplicate.status ==
              sk::LocalRelationPreflightStatus::invalid_intent);
        CHECK(!duplicate.provisional_constraint_id);
        CHECK(!duplicate.proposed_authored);
        CHECK(!duplicate.solve.candidate);

        // A horizontal equation on an immovable skew Line is impossible.
        // A naive residual or "backend says success" shortcut must not
        // authorize this candidate, nor alter original authored seeds.
        const std::array<sk::EntityId,1> fixed{line};
        const auto conflict = sk::preflightLocalRelation(
            baseline, K::horizontal, first, {},
            fixed, policy, native);
        CHECK(conflict.status ==
              sk::LocalRelationPreflightStatus::solver_rejected);
        CHECK(!conflict.provisional_constraint_id);
        CHECK(!conflict.proposed_authored);
        CHECK(!conflict.solve.candidate);
        CHECK(current.state() == baseline);

        // Invalid non-existing Line semantic identity never publishes a
        // candidate, even before the solver sees the proposal.
        const auto absent = sk::EntityId::parse("99");
        CHECK(absent);
        const auto invalid = sk::preflightLocalRelation(
            baseline, K::horizontal, {*absent,L}, {},
            {}, policy, native);
        CHECK(invalid.status ==
              sk::LocalRelationPreflightStatus::invalid_intent);
        CHECK(!invalid.provisional_constraint_id);
        CHECK(current.state() == baseline);
    }

    for (const auto kind : {K::horizontal,K::vertical}) {
        sk::SketchModel model;
        const auto line = model.addLine({0.0,0.0},{5.0,2.0});
        CHECK(model.addConstraint(kind,{line,L}));
        const auto authored = model.state();
        const auto problem = sk::prepareLocalSolveProblem(authored);
        CHECK(problem && problem->equations.size() == 1U);
        const auto r = sk::evaluateLocalSolve(*problem,policy,native);
        CHECK(r.status == sk::LocalSolveGatewayStatus::candidate_accepted);
        CHECK(r.candidate);
        const auto& s = r.candidate->lines.front();
        CHECK(kind == K::horizontal
            ? std::abs(s.start.v-s.end.v) < policy.distance_tolerance
            : std::abs(s.start.u-s.end.u) < policy.distance_tolerance);
        CHECK(model.state() == authored);
        ++passed;
    }

    for (const auto kind : {K::parallel,K::perpendicular}) {
        sk::SketchModel model;
        const auto base = model.addLine({0.0,0.0},{5.0,0.0});
        const auto line = model.addLine({1.0,2.0},{4.0,4.0});
        CHECK(model.addConstraint(kind,{base,L},{line,L}));
        const auto authored = model.state();
        const std::array<sk::EntityId,1> fixed{base};
        const auto problem = sk::prepareLocalSolveProblem(authored,fixed);
        CHECK(problem && problem->fixed_lines.size() == 1U);
        const auto r = sk::evaluateLocalSolve(*problem,policy,native);
        CHECK(r.status == sk::LocalSolveGatewayStatus::candidate_accepted);
        CHECK(r.candidate && r.candidate->lines[0] == authored.lines[0]);
        CHECK(model.state() == authored);
        ++passed;
    }

    {
        sk::SketchModel model;
        const auto a = model.addLine({0.0,0.0},{2.0,2.0});
        const auto b = model.addLine({2.1,2.1},{6.0,4.0});
        CHECK(model.addConstraint(K::coincident,{a,T},{b,S}));
        const auto authored = model.state();
        const std::array<sk::EntityId,1> fixed{a};
        const auto problem = sk::prepareLocalSolveProblem(authored,fixed);
        CHECK(problem);
        const auto r = sk::evaluateLocalSolve(*problem,policy,native);
        CHECK(r.status == sk::LocalSolveGatewayStatus::candidate_accepted);
        CHECK(r.candidate);
        CHECK(r.candidate->lines[0].end == r.candidate->lines[1].start);
        CHECK(r.candidate->lines[0] == authored.lines[0]);
        CHECK(model.state() == authored);
        ++passed;
    }

    {
        sk::SketchModel model;
        const auto a = model.addLine({0.0,0.0},{5.0,2.0});
        CHECK(model.addConstraint(K::fix,{a,L}));
        const auto authored = model.state();
        const auto problem = sk::prepareLocalSolveProblem(authored);
        CHECK(problem && problem->equations.empty() &&
              problem->fixed_lines.size() == 1U);
        const auto r = sk::evaluateLocalSolve(*problem,policy,native);
        CHECK(r.status == sk::LocalSolveGatewayStatus::candidate_accepted);
        CHECK(r.candidate && r.candidate->lines[0] == authored.lines[0]);
        CHECK(!model.addConstraint(K::horizontal,{a,L}));
        CHECK(model.state() == authored);
        ++passed;
    }

    // Original native PlaneGCS must solve all seven equations at once,
    // returning a disposable and precisely closed four-Line rectangle.
    {
        sk::SketchModel model;
        const auto b=model.addLine({0.0,0.0},{4.0,0.0});
        const auto r=model.addLine({4.2,0.1},{4.1,3.1});
        const auto t=model.addLine({4.3,3.0},{-0.2,3.2});
        const auto l=model.addLine({0.1,3.1},{0.1,0.2});
        CHECK(model.addConstraint(K::coincident,{b,T},{r,S}));
        CHECK(model.addConstraint(K::coincident,{r,T},{t,S}));
        CHECK(model.addConstraint(K::coincident,{t,T},{l,S}));
        CHECK(model.addConstraint(K::coincident,{l,T},{b,S}));
        CHECK(model.addConstraint(K::vertical,{r,L}));
        CHECK(model.addConstraint(K::horizontal,{t,L}));
        CHECK(model.addConstraint(K::vertical,{l,L}));
        const auto authored=model.state();
        const std::array<sk::EntityId,1> fixed{b};
        const auto problem=sk::prepareLocalSolveProblem(authored,fixed);
        CHECK(problem && problem->equations.size()==7U);
        CHECK(problem->fixed_lines.size()==1U);
        const auto solved=sk::evaluateLocalSolve(*problem,policy,native);
        CHECK(solved.status==sk::LocalSolveGatewayStatus::candidate_accepted);
        CHECK(solved.candidate);
        CHECK(solved.degrees_of_freedom && *solved.degrees_of_freedom==1U);
        CHECK(solved.residuals.constraints.size()==7U);
        const auto& lines=solved.candidate->lines;
        CHECK(lines[0]==authored.lines[0]);
        CHECK(lines[0].end==lines[1].start);
        CHECK(lines[1].end==lines[2].start);
        CHECK(lines[2].end==lines[3].start);
        CHECK(lines[3].end==lines[0].start);
        CHECK(model.state()==authored);
        ++passed;
    }

    // The original solver can report Success on an impossible fixed
    // diagonal + Horizontal. The real SS2 gateway must still reject it.
    {
        sk::SketchModel model;
        const auto a=model.addLine({0.0,0.0},{5.0,2.0});
        CHECK(model.addConstraint(K::horizontal,{a,L}));
        const auto authored=model.state();
        const std::array<sk::EntityId,1> fixed{a};
        const auto problem=sk::prepareLocalSolveProblem(authored,fixed);
        CHECK(problem);
        const auto refused=sk::evaluateLocalSolve(*problem,policy,native);
        CHECK(refused.status != sk::LocalSolveGatewayStatus::candidate_accepted);
        CHECK(!refused.candidate);
        CHECK(model.state()==authored);
        ++passed;
    }

    // Two projected PG-01 sources can be mutually immovable even when
    // their authored Coincident relation is unsatisfiable. The original
    // pinned PlaneGCS diagnose()/solve() returns Success, DoF=0 and no
    // conflict for the 0.25-unit gap; only the SS2 semantic residual
    // checker detects it. Never move either source or publish a candidate.
    {
        sk::SketchModel model;
        const auto first=model.addLine({0.0,0.0},{4.0,0.0});
        const auto second=model.addLine({4.25,0.0},{4.25,3.0});
        const auto seam=model.addConstraint(
            K::coincident,{first,T},{second,S});
        CHECK(seam);
        const auto authored=model.state();
        const std::array<sk::EntityId,2> frozen{first,second};
        const auto problem=sk::prepareLocalSolveProblem(authored,frozen);
        CHECK(problem && problem->fixed_lines.size()==2U);
        CHECK(problem->equations.size()==1U);
        const auto rejected=sk::evaluateLocalSolve(*problem,policy,native);
        CHECK(rejected.status==
            sk::LocalSolveGatewayStatus::residual_violation);
        CHECK(!rejected.candidate);
        CHECK(rejected.residuals.constraints.size()==1U);
        CHECK(rejected.residuals.constraints.front().constraint_id==*seam);
        CHECK(rejected.residuals.constraints.front().residual==0.25);
        CHECK(!rejected.residuals.constraints.front().accepted);
        CHECK(model.state()==authored);
        ++passed;
    }

    // Original PlaneGCS 0.8.0 identifies the second equation as redundant:
    // Vertical on a free Line duplicates Perpendicular to a fixed
    // Horizontal source. The bridge must translate its native tag back
    // to the precise SS2 ConstraintId and refuse to publish a candidate.
    {
        sk::SketchModel model;
        const auto base=model.addLine({0.0,0.0},{5.0,0.0});
        const auto freeLine=model.addLine({1.0,2.0},{4.0,4.0});
        CHECK(model.addConstraint(K::vertical,{freeLine,L}));
        const auto extra=model.addConstraint(
            K::perpendicular,{base,L},{freeLine,L});
        CHECK(extra);
        const auto authored=model.state();
        const std::array<sk::EntityId,1> projected{base};
        const auto problem=sk::prepareLocalSolveProblem(authored,projected);
        CHECK(problem && problem->equations.size()==2U);
        const auto rejected=sk::evaluateLocalSolve(*problem,policy,native);
        CHECK(rejected.status==
            sk::LocalSolveGatewayStatus::redundant_constraint);
        CHECK(!rejected.candidate);
        CHECK(rejected.diagnostics.size()==1U);
        CHECK(rejected.diagnostics.front().kind==
            sk::LocalBackendDiagnosticKind::redundant_constraint);
        CHECK(rejected.diagnostics.front().constraint==*extra);
        CHECK(model.state()==authored);
        ++passed;
    }

    CHECK(passed==10U);
    std::cout<<"R13_A1_ORIGINAL_PLANEGCS_NATIVE_PASS cases="<<passed<<'\n';
    return EXIT_SUCCESS;
}

#elif defined(SS2_R13_A1_PLANEGCS_OCCT_EVIDENCE)
// This is the ORIGINAL numeric PlaneGCS backend, the actual production
// Part stage and real OCCT, not a scripted Sketch/Feature candidate.
int main() {
    namespace app=simplesolid2::application;
    namespace part=simplesolid2::part;
    namespace core=simplesolid2::core;
    namespace occt=simplesolid2::kernel_occt;
    using E=sk::ConstraintElement;
    using K=sk::ConstraintKind;
    occt::OcctSolidModelingKernel provider;
    sk::PlaneGcsDevSolver native;
    app::DocumentSession session{
        {},part::PartDocument::create(core::DocumentId::generate())};
    const auto sketch=session.execute(
        app::CreatePartSketchCommand{core::BuiltinReferenceRole::xy_plane});
    CHECK(sketch.ok() && sketch.sketch_id);
    const auto rectangle=session.execute(app::AddSketchRectangleCommand{
        *sketch.sketch_id,session.document().revision(),
        {5.0,0.0},{15.0,8.0},sk::EntityRole::regular,false});
    CHECK(rectangle.ok() && rectangle.entity_ids.size()==4U);
    const auto initial=session.document().findSketch(
        *sketch.sketch_id)->model.state();
    CHECK(initial.lines.size()==4U);
    // A skewed, CLOSED authored profile, with x=15.4 and x=4.5 at top,
    // must be numerically solved to the fixed base x=5..15.
    const auto edit=session.execute(app::UpdateSketchGeometryCommand{
        *sketch.sketch_id,session.document().revision(),
        {
            {initial.lines[1].id,initial.lines[1].start,{15.4,8.2}},
            {initial.lines[2].id,{15.4,8.2},{4.5,8.1}},
            {initial.lines[3].id,{4.5,8.1},initial.lines[3].end}
        },{},{}}); 
    CHECK(edit.ok() && edit.changed);
    const auto skew=session.document().findSketch(
        *sketch.sketch_id)->model.state();
    CHECK(skew.lines.size()==4U && skew.lines[0]==initial.lines[0]);
    CHECK(skew.lines[1].end.u==15.4 && skew.lines[2].end.u==4.5);
    const auto regions=sk::analyzeRegions(
        session.document().findSketch(*sketch.sketch_id)->model);
    CHECK(regions.complete() && regions.regions.size()==1U);
    const auto intent=part::makeProfileRegionIntent(regions.regions.front());
    CHECK(intent);
    const auto profile=session.execute(app::CreateProfileCommand{
        *sketch.sketch_id,session.document().revision(),*intent});
    CHECK(profile.ok() && profile.profile_id);
    const auto add=[&](K kind,sk::ConstraintReference first,
                       sk::ConstraintReference second={}) {
        const auto rel=session.execute(app::AddSketchRelationCommand{
            *sketch.sketch_id,session.document().revision(),kind,first,second});
        CHECK(rel.ok() && rel.constraint_id);
    };
    const auto& L=skew.lines;
    add(K::fix,{L[0].id,E::line});
    add(K::coincident,{L[0].id,E::line_end},{L[1].id,E::line_start});
    add(K::coincident,{L[1].id,E::line_end},{L[2].id,E::line_start});
    add(K::coincident,{L[2].id,E::line_end},{L[3].id,E::line_start});
    add(K::coincident,{L[3].id,E::line_end},{L[0].id,E::line_start});
    add(K::vertical,{L[1].id,E::line});
    add(K::horizontal,{L[2].id,E::line});
    add(K::vertical,{L[3].id,E::line});
    const auto authored=session.document().findSketch(
        *sketch.sketch_id)->model.state();
    CHECK(authored.constraints.size()==8U && authored.lines==skew.lines);
    part::PartEvaluation prefix;
    prefix.source_document_id=session.document().documentId();
    prefix.source_revision=session.document().revision();
        prefix.source_incarnation = session.document().incarnationToken();
    const auto input=part::prepareStageLocalSolveProblem(
        session.document(),*sketch.sketch_id,prefix,provider);
    CHECK(input && input->constraints.size()==8U);
    CHECK(input->equations.size()==7U);
    CHECK(input->fixed_lines.size()==1U && input->fixed_lines[0]==L[0].id);
    const sk::LocalSolverAcceptancePolicy policy{1.0e-7,1.0e-6};
    const auto accepted=sk::evaluateLocalSolve(*input,policy,native);
    CHECK(accepted.status==sk::LocalSolveGatewayStatus::candidate_accepted);
    CHECK(accepted.candidate);
    CHECK(accepted.degrees_of_freedom && *accepted.degrees_of_freedom==1U);
    const auto& solved=accepted.candidate->lines;
    CHECK(solved.size()==4U && solved[0]==authored.lines[0]);
    CHECK(solved[0].end==solved[1].start);
    CHECK(solved[1].end==solved[2].start);
    CHECK(solved[2].end==solved[3].start);
    CHECK(solved[3].end==solved[0].start);
    CHECK(std::abs(solved[1].end.u-15.0)<1.0e-5);
    CHECK(std::abs(solved[2].end.u-5.0)<1.0e-5);
    CHECK(solved[1].end.u!=authored.lines[1].end.u);
    CHECK(!part::resolveKernelProfileInput(
        session.document(),*profile.profile_id).ok());
    const auto resolved=part::resolveStageSolvedProfileKernelInput(
        session.document(),*profile.profile_id,prefix,provider,policy,native);
    CHECK(resolved.ok() && resolved.input);
    CHECK(resolved.input->outer.boundary.size()==4U);
    CHECK(occt::buildProfileFaceEvidence(*resolved.input).brep_valid);
    auto state=session.document().state();
    const auto feature=state.body.next_feature_id.allocate();
    CHECK(feature);
    state.body.features.push_back(part::PartFeature{
        *feature,"PlaneGCSRealExtrude",false,
        part::ExtrudeFeature{*profile.profile_id,part::ExtrudeOperation::add,
            part::OneSidedExtrudeExtent{core::LengthValue{10.0},false}}});
    const auto owned=part::PartDocument::restore(
        session.document().documentId(),std::move(state),
        session.document().revision());
    CHECK(owned.ok() && owned.document);
    const auto legacy=part::evaluatePart(*owned.document,provider);
    CHECK(legacy.features.size()==1U);
    CHECK(legacy.features.front().status==part::FeatureEvaluationStatus::blocked);
    const auto partEval=part::evaluatePartWithLocalSolve(
        *owned.document,provider,policy,native);
    CHECK(partEval.body_status==part::BodyEvaluationStatus::up_to_date);
    CHECK(partEval.features.size()==1U);
    CHECK(partEval.features.front().status==part::FeatureEvaluationStatus::up_to_date);
    CHECK(partEval.body_solid!=nullptr);
    const auto mesh=provider.presentationMesh(partEval.body_solid);
    CHECK(mesh.ok());
    double xmin=std::numeric_limits<double>::infinity();
    double xmax=-std::numeric_limits<double>::infinity();
    for(const auto& tri:mesh.mesh.triangles) {
        for(const auto& vertex:{tri.first,tri.second,tri.third}) {
            xmin=std::min(xmin,vertex.x);
            xmax=std::max(xmax,vertex.x);
        }
    }
    CHECK(std::isfinite(xmin) && std::isfinite(xmax));
    CHECK(std::abs(xmin-5.0)<0.01 && std::abs(xmax-15.0)<0.01);
    CHECK(owned.document->findSketch(*sketch.sketch_id)->model.state()==authored);
    CHECK(session.document().findSketch(*sketch.sketch_id)->model.state()==authored);

    // D2-P21-1/P21-06: valid authored intent is NOT the same as a
    // numerically feasible profile. Take the verified real OCCT Extrude
    // and introduce one more legitimate, nonduplicate Sketch relation:
    // fixed horizontal base PARALLEL to a vertical right wall. This
    // cannot produce a nondegenerate four-Line closed rectangle, but
    // remains a valid v21 authored relation that MUST cold-load intact.
    {
        auto impossibleState = owned.document->state();
        CHECK(impossibleState.sketches.size() == 1U);
        auto& impossibleModel = impossibleState.sketches.front().model;
        const auto impossibleRelation = impossibleModel.addConstraint(
            K::parallel, {L[0].id,E::line}, {L[1].id,E::line});
        CHECK(impossibleRelation);
        CHECK(impossibleRelation->serialized() == "9");
        const auto impossibleAuthored = impossibleModel.state();
        CHECK(impossibleAuthored.constraints.size() == 9U);
        CHECK(impossibleAuthored.next_constraint_id.serialized() == "10");
        const auto impossibleOwned = part::PartDocument::restore(
            owned.document->documentId(), std::move(impossibleState),
            owned.document->revision());
        CHECK(impossibleOwned.ok() && impossibleOwned.document);
        const auto impossiblePath = std::filesystem::temp_directory_path() /
            ("r13a1_p21_numerically_unsatisfied_" +
             std::string{owned.document->documentId().value()} +
             ".ss2part");
        part::PartDocumentStore impossibleStore;
        CHECK(impossibleStore.createNew(
            impossiblePath, *impossibleOwned.document).ok());
        const auto impossibleCold = impossibleStore.load(impossiblePath);
        CHECK(impossibleCold.ok() && impossibleCold.document);
        CHECK(impossibleCold.document->state() ==
              impossibleOwned.document->state());
        CHECK(impossibleCold.document->findSketch(*sketch.sketch_id)
            ->model.state() == impossibleAuthored);
        CHECK(impossibleCold.document->findProfile(*profile.profile_id));
        CHECK(impossibleCold.document->state().body.features.size() == 1U);
        CHECK(impossibleCold.document->state().body.features.front().id ==
              *feature);

        occt::OcctSolidModelingKernel impossibleProvider;
        part::PartEvaluation impossiblePrefix;
        impossiblePrefix.source_document_id =
            impossibleCold.document->documentId();
        impossiblePrefix.source_revision =
            impossibleCold.document->revision();
        impossiblePrefix.source_incarnation = impossibleCold.document->incarnationToken();
        const auto impossibleProblem = part::prepareStageLocalSolveProblem(
            *impossibleCold.document, *sketch.sketch_id,
            impossiblePrefix, impossibleProvider);
        CHECK(impossibleProblem);
        CHECK(impossibleProblem->constraints.size() == 9U);
        CHECK(impossibleProblem->equations.size() == 8U);
        const auto impossibleResult =
            sk::evaluateLocalSolve(*impossibleProblem, policy, native);
        CHECK(impossibleResult.status !=
              sk::LocalSolveGatewayStatus::candidate_accepted);
        CHECK(!impossibleResult.candidate);
        CHECK(impossibleCold.document->findSketch(*sketch.sketch_id)
            ->model.state() == impossibleAuthored);
        const auto impossibleDefault =
            part::evaluatePart(*impossibleCold.document, impossibleProvider);
        CHECK(impossibleDefault.features.size() == 1U);
        CHECK(impossibleDefault.features.front().status ==
              part::FeatureEvaluationStatus::blocked);
        CHECK(impossibleDefault.body_solid == nullptr);
        const auto impossibleEvaluated = part::evaluatePartWithLocalSolve(
            *impossibleCold.document, impossibleProvider, policy, native);
        CHECK(impossibleEvaluated.features.size() == 1U);
        CHECK(impossibleEvaluated.features.front().feature_id == *feature);
        CHECK(impossibleEvaluated.features.front().status !=
              part::FeatureEvaluationStatus::up_to_date);
        CHECK(impossibleEvaluated.body_status !=
              part::BodyEvaluationStatus::up_to_date);
        CHECK(impossibleEvaluated.body_solid == nullptr);
        CHECK(impossibleCold.document->findSketch(*sketch.sketch_id)
            ->model.state() == impossibleAuthored);
        CHECK(std::filesystem::remove(impossiblePath));
    }

    // A1.3 native lifecycle: erase an authored constraint through the
    // revision-bound DocumentSession, then Undo/Redo/Undo its SAME semantic
    // ConstraintId. A previous Part-stage prefix must never be reused.
    const auto horizontal = std::find_if(
        authored.constraints.begin(), authored.constraints.end(),
        [](const auto& relation) { return relation.kind == K::horizontal; });
    CHECK(horizontal != authored.constraints.end());
    const auto horizontal_id = horizontal->id;
    const auto erased = session.execute(app::EraseSketchConstraintCommand{
        *sketch.sketch_id, session.document().revision(), horizontal_id});
    CHECK(erased.ok() && erased.changed);
    CHECK(session.document().findProfile(*profile.profile_id) != nullptr);
    const auto* afterErase = session.document().findSketch(*sketch.sketch_id);
    CHECK(afterErase && afterErase->model.constraintCount() == 7U);
    CHECK(afterErase->model.findConstraint(horizontal_id) == nullptr);
    CHECK(afterErase->model.state().lines == authored.lines);
    CHECK(!part::prepareStageLocalSolveProblem(
        session.document(), *sketch.sketch_id, prefix, provider));
    CHECK(!part::resolveStageSolvedProfileKernelInput(
        session.document(), *profile.profile_id, prefix,
        provider, policy, native).ok());

    CHECK(session.undo().changed);
    CHECK(session.document().findSketch(*sketch.sketch_id)
        ->model.findConstraint(horizontal_id) != nullptr);
    CHECK(session.document().findSketch(*sketch.sketch_id)
        ->model.state() == authored);
    CHECK(session.redo().changed);
    CHECK(session.document().findSketch(*sketch.sketch_id)
        ->model.findConstraint(horizontal_id) == nullptr);
    CHECK(session.document().findProfile(*profile.profile_id) != nullptr);
    CHECK(session.undo().changed);
    CHECK(session.document().findSketch(*sketch.sketch_id)
        ->model.state() == authored);

    // Re-evaluate the original real PlaneGCS → Profile → OCCT solid on
    // the NEW revision. The persisted authored intent and ProfileId are
    // unchanged; only the disposable numerical candidate is rebuilt.
    part::PartEvaluation replayPrefix;
    replayPrefix.source_document_id = session.document().documentId();
    replayPrefix.source_revision = session.document().revision();
        replayPrefix.source_incarnation = session.document().incarnationToken();
    const auto replayProfile = part::resolveStageSolvedProfileKernelInput(
        session.document(), *profile.profile_id, replayPrefix,
        provider, policy, native);
    CHECK(replayProfile.ok() && replayProfile.input);
    CHECK(occt::buildProfileFaceEvidence(*replayProfile.input).brep_valid);
    auto replayState = session.document().state();
    const auto replayFeatureId = replayState.body.next_feature_id.allocate();
    CHECK(replayFeatureId && *replayFeatureId == *feature);
    replayState.body.features.push_back(part::PartFeature{
        *replayFeatureId, "PlaneGCSRealExtrude", false,
        part::ExtrudeFeature{*profile.profile_id, part::ExtrudeOperation::add,
            part::OneSidedExtrudeExtent{core::LengthValue{10.0}, false}}});
    const auto replayOwned = part::PartDocument::restore(
        session.document().documentId(), std::move(replayState),
        session.document().revision());
    CHECK(replayOwned.ok() && replayOwned.document);
    const auto replayEvaluated = part::evaluatePartWithLocalSolve(
        *replayOwned.document, provider, policy, native);
    CHECK(replayEvaluated.body_status ==
        part::BodyEvaluationStatus::up_to_date);
    CHECK(replayEvaluated.features.size() == 1U);
    CHECK(replayEvaluated.features.front().status ==
        part::FeatureEvaluationStatus::up_to_date);
    CHECK(replayEvaluated.body_solid != nullptr);
    CHECK(provider.presentationMesh(replayEvaluated.body_solid).ok());
    CHECK(replayOwned.document->findSketch(*sketch.sketch_id)
        ->model.state() == authored);

    // P21-06: real native Part file cold reopen of the SAME eight-constraint
    // authored Sketch, durable ProfileId and Part-owned Extrude FeatureId.
    // No cached solver candidate/BRep may be restored as current state.
    part::PartDocumentStore coldStore;
    const auto coldBase = std::filesystem::temp_directory_path() /
        ("r13a1_v21_real_part_" +
         std::string{session.document().documentId().value()});
    const auto goodPath = coldBase.string() + "_good.ss2part";
    CHECK(coldStore.createNew(goodPath, *replayOwned.document).ok());
    const auto goodLoaded = coldStore.load(goodPath);
    CHECK(goodLoaded.ok());
    CHECK(goodLoaded.document->documentId() ==
          replayOwned.document->documentId());
    CHECK(goodLoaded.document->findSketch(*sketch.sketch_id)
        ->model.state() == authored);
    CHECK(goodLoaded.document->findProfile(*profile.profile_id) != nullptr);
    CHECK(goodLoaded.document->state().body.features.size() == 1U);
    CHECK(goodLoaded.document->state().body.features.front().id == *feature);
    CHECK(goodLoaded.document->state() == replayOwned.document->state());
    const auto coldLegacy = part::evaluatePart(*goodLoaded.document, provider);
    CHECK(coldLegacy.features.size() == 1U);
    CHECK(coldLegacy.features.front().status ==
        part::FeatureEvaluationStatus::blocked);
    CHECK(coldLegacy.body_solid == nullptr);
    const auto coldSolved = part::evaluatePartWithLocalSolve(
        *goodLoaded.document, provider, policy, native);
    CHECK(coldSolved.body_status == part::BodyEvaluationStatus::up_to_date);
    CHECK(coldSolved.features.size() == 1U);
    CHECK(coldSolved.features.front().feature_id == *feature);
    CHECK(coldSolved.features.front().status ==
        part::FeatureEvaluationStatus::up_to_date);
    CHECK(coldSolved.body_solid != nullptr);
    CHECK(provider.presentationMesh(coldSolved.body_solid).ok());
    CHECK(goodLoaded.document->findSketch(*sketch.sketch_id)
        ->model.state() == authored);
    CHECK(std::filesystem::remove(goodPath));

    // The SAME original PlaneGCS 0.8.0 adapter must also produce a real
    // Revolve through the Part-owned stage and existing feature scheduler.
    // A controlled fake backend or a naked provider.revolve() is not proof.
    auto revolveState = session.document().state();
    const auto revolveId = revolveState.body.next_feature_id.allocate();
    CHECK(revolveId && *revolveId == *feature);
    revolveState.body.features.push_back(part::PartFeature{
        *revolveId, "PlaneGCSRealRevolve", false,
        part::RevolveFeature{
            *profile.profile_id,
            part::AxisReference{part::BuiltinOriginAxisReference{
                core::BuiltinReferenceRole::y_axis}},
            part::RevolveOperation::add,
            part::OneSidedRevolveExtent{
                core::AngleValue{2.0 * std::numbers::pi_v<double>},
                false}}});
    const auto revolveOwned = part::PartDocument::restore(
        session.document().documentId(), std::move(revolveState),
        session.document().revision());
    CHECK(revolveOwned.ok() && revolveOwned.document);
    CHECK(revolveOwned.document->findProfile(*profile.profile_id) != nullptr);
    const auto unsolvedRevolve =
        part::evaluatePart(*revolveOwned.document, provider);
    CHECK(unsolvedRevolve.features.size() == 1U);
    CHECK(unsolvedRevolve.features.front().status ==
        part::FeatureEvaluationStatus::blocked);
    const auto realRevolve = part::evaluatePartWithLocalSolve(
        *revolveOwned.document, provider, policy, native);
    CHECK(realRevolve.source_revision == revolveOwned.document->revision());
    CHECK(realRevolve.body_status == part::BodyEvaluationStatus::up_to_date);
    CHECK(realRevolve.features.size() == 1U);
    CHECK(realRevolve.features.front().feature_id == *revolveId);
    CHECK(realRevolve.features.front().status ==
        part::FeatureEvaluationStatus::up_to_date);
    CHECK(realRevolve.features.front().result_topology);
    CHECK(realRevolve.features.front().result_topology->complete());
    CHECK(realRevolve.body_solid != nullptr);
    CHECK(realRevolve.current_topology);
    CHECK(realRevolve.current_topology->complete());

    // The Y-axis stays OUTSIDE the fixed 5..15 mm rectangle: a true
    // annular Revolve must have the expected inner and outer radii.
    // Presentation-mesh bounds are an independent geometry witness,
    // never a replacement for stable semantic identity or residuals.
    const auto revolveMesh = provider.presentationMesh(realRevolve.body_solid);
    CHECK(revolveMesh.ok());
    double inner = std::numeric_limits<double>::infinity();
    double outer = -std::numeric_limits<double>::infinity();
    for (const auto& tri : revolveMesh.mesh.triangles) {
        for (const auto& vertex : {tri.first, tri.second, tri.third}) {
            const double radius = std::hypot(vertex.x, vertex.z);
            inner = std::min(inner, radius);
            outer = std::max(outer, radius);
        }
    }
    CHECK(std::isfinite(inner) && std::isfinite(outer));
    CHECK(std::abs(inner - 5.0) < 0.1);
    CHECK(std::abs(outer - 15.0) < 0.1);
    CHECK(revolveOwned.document->findSketch(*sketch.sketch_id)
        ->model.state() == authored);
    CHECK(session.document().findSketch(*sketch.sketch_id)
        ->model.state() == authored);

    // P21-06 native Revolve cold cycle: the authored Sketch/Profile/Axis/
    // Feature is persistent, never the transient PlaneGCS candidate or
    // OCCT BRep. Recompute on the reopened revision and check real topology.
    const auto revolvePath = coldBase.string() + "_revolve.ss2part";
    CHECK(coldStore.createNew(revolvePath, *revolveOwned.document).ok());
    const auto revolveLoaded = coldStore.load(revolvePath);
    CHECK(revolveLoaded.ok());
    CHECK(revolveLoaded.document->documentId() ==
          revolveOwned.document->documentId());
    CHECK(revolveLoaded.document->state() == revolveOwned.document->state());
    CHECK(revolveLoaded.document->findProfile(*profile.profile_id) != nullptr);
    CHECK(revolveLoaded.document->findSketch(*sketch.sketch_id)
        ->model.state() == authored);
    CHECK(revolveLoaded.document->state().body.features.size() == 1U);
    CHECK(revolveLoaded.document->state().body.features.front().id ==
          *revolveId);
    const auto unsolvedColdRevolve =
        part::evaluatePart(*revolveLoaded.document, provider);
    CHECK(unsolvedColdRevolve.features.size() == 1U);
    CHECK(unsolvedColdRevolve.features.front().status ==
          part::FeatureEvaluationStatus::blocked);
    CHECK(unsolvedColdRevolve.body_solid == nullptr);
    const auto solvedColdRevolve = part::evaluatePartWithLocalSolve(
        *revolveLoaded.document, provider, policy, native);
    CHECK(solvedColdRevolve.source_revision ==
          revolveLoaded.document->revision());
    CHECK(solvedColdRevolve.body_status ==
          part::BodyEvaluationStatus::up_to_date);
    CHECK(solvedColdRevolve.features.size() == 1U);
    CHECK(solvedColdRevolve.features.front().feature_id == *revolveId);
    CHECK(solvedColdRevolve.features.front().status ==
          part::FeatureEvaluationStatus::up_to_date);
    CHECK(solvedColdRevolve.features.front().result_topology);
    CHECK(solvedColdRevolve.features.front().result_topology->complete());
    CHECK(solvedColdRevolve.current_topology);
    CHECK(solvedColdRevolve.current_topology->complete());
    CHECK(solvedColdRevolve.body_solid != nullptr);
    const auto coldRevolveMesh =
        provider.presentationMesh(solvedColdRevolve.body_solid);
    CHECK(coldRevolveMesh.ok());
    double coldInner = std::numeric_limits<double>::infinity();
    double coldOuter = -std::numeric_limits<double>::infinity();
    for (const auto& tri : coldRevolveMesh.mesh.triangles) {
        for (const auto& vertex : {tri.first, tri.second, tri.third}) {
            const double radius = std::hypot(vertex.x, vertex.z);
            coldInner = std::min(coldInner, radius);
            coldOuter = std::max(coldOuter, radius);
        }
    }
    CHECK(std::abs(coldInner - 5.0) < 0.1);
    CHECK(std::abs(coldOuter - 15.0) < 0.1);
    CHECK(revolveLoaded.document->findSketch(*sketch.sketch_id)
        ->model.state() == authored);
    CHECK(std::filesystem::remove(revolvePath));

    // A1.3 geometry-delete cascade: deleting the top Line retires its
    // Horizontal and two Coincident intents in ONE DocumentSession command.
    // Dependent durable Profile/Feature identities must survive, but
    // their derived BRep must become blocked, never stale-current.
    const auto eraseTop = session.execute(app::EraseSketchEntityCommand{
        *sketch.sketch_id, L[2].id});
    CHECK(eraseTop.ok() && eraseTop.changed);
    CHECK(session.document().findProfile(*profile.profile_id) != nullptr);
    const auto* withoutTop =
        session.document().findSketch(*sketch.sketch_id);
    CHECK(withoutTop != nullptr);
    CHECK(withoutTop->model.state().lines.size() == 3U);
    CHECK(withoutTop->model.constraintCount() == 5U);
    CHECK(withoutTop->model.findConstraint(horizontal_id) == nullptr);
    CHECK(!part::prepareStageLocalSolveProblem(
        session.document(), *sketch.sketch_id, replayPrefix, provider));

    const auto evaluateCurrentExtrude = [&]() {
        auto featureState = session.document().state();
        const auto durableFeatureId = featureState.body.next_feature_id.allocate();
        CHECK(durableFeatureId && *durableFeatureId == *feature);
        featureState.body.features.push_back(part::PartFeature{
            *durableFeatureId, "PlaneGCSRealExtrude", false,
            part::ExtrudeFeature{*profile.profile_id,
                part::ExtrudeOperation::add,
                part::OneSidedExtrudeExtent{core::LengthValue{10.0}, false}}});
        const auto staged = part::PartDocument::restore(
            session.document().documentId(), std::move(featureState),
            session.document().revision());
        CHECK(staged.ok() && staged.document);
        CHECK(staged.document->findProfile(*profile.profile_id) != nullptr);
        return part::evaluatePartWithLocalSolve(
            *staged.document, provider, policy, native);
    };
    const auto brokenBody = evaluateCurrentExtrude();
    CHECK(brokenBody.features.size() == 1U);
    CHECK(brokenBody.features.front().feature_id == *feature);
    CHECK(brokenBody.features.front().status !=
        part::FeatureEvaluationStatus::up_to_date);
    CHECK(brokenBody.body_status !=
        part::BodyEvaluationStatus::up_to_date);
    CHECK(brokenBody.body_solid == nullptr);

    // P21-07: persist the BROKEN current Part as a real v21 document.
    // Cold reopen must preserve authored ProfileId/FeatureId and the
    // cascade-retired relation IDs, without resurrecting a stale solid.
    auto brokenState = session.document().state();
    const auto brokenFeatureId =
        brokenState.body.next_feature_id.allocate();
    CHECK(brokenFeatureId && *brokenFeatureId == *feature);
    brokenState.body.features.push_back(part::PartFeature{
        *brokenFeatureId, "PlaneGCSRealExtrude", false,
        part::ExtrudeFeature{*profile.profile_id,
            part::ExtrudeOperation::add,
            part::OneSidedExtrudeExtent{core::LengthValue{10.0}, false}}});
    const auto brokenOwned = part::PartDocument::restore(
        session.document().documentId(), std::move(brokenState),
        session.document().revision());
    CHECK(brokenOwned.ok() && brokenOwned.document);
    const auto brokenPath = coldBase.string() + "_broken.ss2part";
    CHECK(coldStore.createNew(brokenPath, *brokenOwned.document).ok());
    const auto coldBroken = coldStore.load(brokenPath);
    CHECK(coldBroken.ok());
    CHECK(coldBroken.document->state() == brokenOwned.document->state());
    CHECK(coldBroken.document->findProfile(*profile.profile_id) != nullptr);
    CHECK(coldBroken.document->state().body.features.size() == 1U);
    CHECK(coldBroken.document->state().body.features.front().id == *feature);
    CHECK(coldBroken.document->findSketch(*sketch.sketch_id)
        ->model.constraintCount() == 5U);
    const auto reopenedBroken = part::evaluatePartWithLocalSolve(
        *coldBroken.document, provider, policy, native);
    CHECK(reopenedBroken.features.size() == 1U);
    CHECK(reopenedBroken.features.front().feature_id == *feature);
    CHECK(reopenedBroken.features.front().status !=
        part::FeatureEvaluationStatus::up_to_date);
    CHECK(reopenedBroken.body_status !=
        part::BodyEvaluationStatus::up_to_date);
    CHECK(reopenedBroken.body_solid == nullptr);

    CHECK(session.undo().changed);
    CHECK(session.document().findSketch(*sketch.sketch_id)
        ->model.state() == authored);
    CHECK(session.document().findProfile(*profile.profile_id) != nullptr);

    // P21-07: repair the same previously cold-opened BROKEN native Part
    // through a real successful DocumentSession Undo history command.
    // Publish that recovered authored state to the EXISTING native file
    // using its cold-read optimistic checkpoint, then cold-reopen again.
    // The Profile and Feature are stable owner identities; no prior BRep
    // or numerical PlaneGCS candidate is treated as current.
    auto repairedState = session.document().state();
    const auto repairedFeatureId =
        repairedState.body.next_feature_id.allocate();
    CHECK(repairedFeatureId && *repairedFeatureId == *feature);
    repairedState.body.features.push_back(part::PartFeature{
        *repairedFeatureId, "PlaneGCSRealExtrude", false,
        part::ExtrudeFeature{*profile.profile_id,
            part::ExtrudeOperation::add,
            part::OneSidedExtrudeExtent{core::LengthValue{10.0}, false}}});
    const auto repairedOwned = part::PartDocument::restore(
        session.document().documentId(), std::move(repairedState),
        session.document().revision());
    CHECK(repairedOwned.ok() && repairedOwned.document);
    CHECK(repairedOwned.document->state().body.features.front().id ==
        *feature);
    const auto repairSaved = coldStore.save(
        brokenPath, *repairedOwned.document, *coldBroken.checkpoint);
    CHECK(repairSaved.ok());
    const auto coldRepaired = coldStore.load(brokenPath);
    CHECK(coldRepaired.ok());
    CHECK(coldRepaired.document->documentId() ==
        coldBroken.document->documentId());
    CHECK(coldRepaired.document->state() ==
        repairedOwned.document->state());
    CHECK(coldRepaired.document->findSketch(*sketch.sketch_id)
        ->model.state() == authored);
    CHECK(coldRepaired.document->findProfile(*profile.profile_id) != nullptr);
    CHECK(coldRepaired.document->state().body.features.size() == 1U);
    CHECK(coldRepaired.document->state().body.features.front().id ==
        *feature);
    // This decoder currently resets an in-memory DocumentRevision on
    // each cold Load. A raw PartEvaluation from a different cold
    // incarnation could coincidentally have the SAME numeric revision:
    // that cross-session epoch guarantee is NOT proved here. An explicit
    // wrong-revision prefix, however, must still be rejected.
    occt::OcctSolidModelingKernel recoveredProvider;
    auto invalidRevisionPrefix = reopenedBroken;
    invalidRevisionPrefix.source_revision =
        core::DocumentRevision{std::numeric_limits<std::uint64_t>::max()};
    CHECK(invalidRevisionPrefix.source_revision !=
          coldRepaired.document->revision());
    CHECK(!part::prepareStageLocalSolveProblem(
        *coldRepaired.document, *sketch.sketch_id,
        invalidRevisionPrefix, recoveredProvider));
    const auto repairedLegacy = part::evaluatePart(
        *coldRepaired.document, recoveredProvider);
    CHECK(repairedLegacy.features.size() == 1U);
    CHECK(repairedLegacy.features.front().status ==
        part::FeatureEvaluationStatus::blocked);
    CHECK(repairedLegacy.body_solid == nullptr);
    const auto repairedNative = part::evaluatePartWithLocalSolve(
        *coldRepaired.document, recoveredProvider, policy, native);
    CHECK(repairedNative.source_revision ==
        coldRepaired.document->revision());
    CHECK(repairedNative.features.size() == 1U);
    CHECK(repairedNative.features.front().feature_id == *feature);
    CHECK(repairedNative.features.front().status ==
        part::FeatureEvaluationStatus::up_to_date);
    CHECK(repairedNative.body_status ==
        part::BodyEvaluationStatus::up_to_date);
    CHECK(repairedNative.body_solid != nullptr);
    CHECK(repairedNative.current_topology);
    CHECK(repairedNative.current_topology->complete());
    CHECK(recoveredProvider.presentationMesh(
        repairedNative.body_solid).ok());
    CHECK(coldRepaired.document->findSketch(*sketch.sketch_id)
        ->model.state() == authored);
    CHECK(std::filesystem::remove(brokenPath));

    const auto revivedBody = evaluateCurrentExtrude();
    CHECK(revivedBody.features.size() == 1U);
    CHECK(revivedBody.features.front().feature_id == *feature);
    CHECK(revivedBody.features.front().status ==
        part::FeatureEvaluationStatus::up_to_date);
    CHECK(revivedBody.body_status ==
        part::BodyEvaluationStatus::up_to_date);
    CHECK(revivedBody.body_solid != nullptr);
    CHECK(provider.presentationMesh(revivedBody.body_solid).ok());

    CHECK(session.redo().changed);
    CHECK(session.document().findProfile(*profile.profile_id) != nullptr);
    CHECK(session.document().findSketch(*sketch.sketch_id)
        ->model.constraintCount() == 5U);
    const auto rebrokenBody = evaluateCurrentExtrude();
    CHECK(rebrokenBody.features.size() == 1U);
    CHECK(rebrokenBody.features.front().feature_id == *feature);
    CHECK(rebrokenBody.features.front().status !=
        part::FeatureEvaluationStatus::up_to_date);
    CHECK(rebrokenBody.body_status !=
        part::BodyEvaluationStatus::up_to_date);
    CHECK(rebrokenBody.body_solid == nullptr);
    std::cout<<"R13_A1_ORIGINAL_PLANEGCS_OCCT_PASS real_solver=1"
             <<" profile_face=1 extrude_solid=1 fixed_line=1"
             <<" corrected_geometry=1 source_unchanged=1\n";
    return EXIT_SUCCESS;
}
#elif !defined(SS2_R13_A1_OCCT_EVIDENCE)
int main() {
    // Native MSVC RED: a caller-owned solver may drive the actual Part
    // feature scheduler, without changing the legacy evaluatePart API.
    CHECK(&simplesolid2::part::evaluatePartWithLocalSolve != nullptr);
    CHECK(!sk::ConstraintId::parse("0"));
    CHECK(!sk::ConstraintId::parse("01"));
    CHECK(!sk::ConstraintId::parse("-1"));
    CHECK(!sk::ConstraintId::parse("18446744073709551616"));

    // R13-A2.0 registered FAST proof: caller-injected backend cannot
    // mutate the authoritative current-stage Sketch during preflight.
    // An echo backend exercises the stage-neutral gateway, NOT an
    // installed/redistributable numerical product solver.
    {
        struct EchoBackend final : sk::ILocalSketchSolver {
            int calls{};
            sk::LocalBackendSolveResult solve(
                const sk::LocalSolveProblem& problem) override {
                ++calls;
                return {sk::LocalBackendStatus::converged,
                        problem.authored, {}, {}, 3U};
            }
        } backend;
        const sk::LocalSolverAcceptancePolicy policy{1.0e-8, 1.0e-8};
        sk::SketchModel flat;
        const auto line = flat.addLine({0.0, 3.0}, {5.0, 3.0});
        const auto before = flat.state();
        const sk::ConstraintReference ref{
            line, sk::ConstraintElement::line};
        const auto accepted = sk::preflightLocalRelation(
            before, sk::ConstraintKind::horizontal,
            ref, {}, {}, policy, backend);
        CHECK(accepted.status ==
              sk::LocalRelationPreflightStatus::candidate_accepted);
        CHECK(accepted.provisional_constraint_id);
        CHECK(accepted.proposed_authored);
        CHECK(accepted.proposed_authored->constraints.size() == 1U);
        CHECK(accepted.solve.candidate);
        CHECK(accepted.solve.degrees_of_freedom == 3U);
        CHECK(flat.state() == before);
        CHECK(backend.calls == 1);

        // RED (R13-A2.0): a backend cannot claim more free scalar
        // parameters than the Line-only solver problem actually owns.
        // One unfixed Line has four endpoint coordinates, not five.
        // This must fail closed even when the backend says Converged
        // and all independent geometric residuals are satisfied.
        struct ImpossibleDofBackend final : sk::ILocalSketchSolver {
            std::size_t claimed_dof{5U};
            sk::LocalBackendSolveResult solve(
                const sk::LocalSolveProblem& problem) override {
                return {sk::LocalBackendStatus::converged,
                        problem.authored, {}, {}, claimed_dof};
            }
        } impossible_dof;
        const auto invalid_dof = sk::preflightLocalRelation(
            before, sk::ConstraintKind::horizontal,
            ref, {}, {}, policy, impossible_dof);
        CHECK(invalid_dof.status ==
              sk::LocalRelationPreflightStatus::solver_rejected);
        CHECK(invalid_dof.solve.status ==
              sk::LocalSolveGatewayStatus::invalid_backend_response);
        CHECK(!invalid_dof.proposed_authored);
        CHECK(!invalid_dof.provisional_constraint_id);
        CHECK(!invalid_dof.solve.candidate);
        CHECK(flat.state() == before);

        // A fixed Line has no editable scalar parameters at all,
        // regardless of how many coordinates its geometry contains.
        impossible_dof.claimed_dof = 1U;
        const std::array<sk::EntityId, 1> stage_fixed{line};
        const auto invalid_fixed_dof = sk::preflightLocalRelation(
            before, sk::ConstraintKind::horizontal,
            ref, {}, stage_fixed, policy, impossible_dof);
        CHECK(invalid_fixed_dof.status ==
              sk::LocalRelationPreflightStatus::solver_rejected);
        CHECK(invalid_fixed_dof.solve.status ==
              sk::LocalSolveGatewayStatus::invalid_backend_response);
        CHECK(!invalid_fixed_dof.proposed_authored);
        CHECK(!invalid_fixed_dof.provisional_constraint_id);
        CHECK(!invalid_fixed_dof.solve.candidate);
        CHECK(flat.state() == before);

        const auto duplicate = sk::preflightLocalRelation(
            *accepted.proposed_authored, sk::ConstraintKind::horizontal,
            ref, {}, {}, policy, backend);
        CHECK(duplicate.status ==
              sk::LocalRelationPreflightStatus::invalid_intent);
        CHECK(!duplicate.provisional_constraint_id);
        CHECK(backend.calls == 1);

        const auto nonexistent = sk::EntityId::parse("9999");
        CHECK(nonexistent);
        const auto missing = sk::preflightLocalRelation(
            before, sk::ConstraintKind::horizontal,
            {*nonexistent, sk::ConstraintElement::line}, {},
            {}, policy, backend);
        CHECK(missing.status ==
              sk::LocalRelationPreflightStatus::invalid_intent);
        CHECK(backend.calls == 1);

        const std::array<sk::EntityId, 2> invalid_fixed{line, line};
        const auto bad_stage = sk::preflightLocalRelation(
            before, sk::ConstraintKind::horizontal,
            ref, {}, invalid_fixed, policy, backend);
        CHECK(bad_stage.status ==
              sk::LocalRelationPreflightStatus::invalid_problem);
        CHECK(!bad_stage.provisional_constraint_id);
        CHECK(!bad_stage.proposed_authored);
        CHECK(backend.calls == 1);

        sk::SketchModel skew;
        const auto skew_line = skew.addLine(
            {0.0, 0.0}, {5.0, 2.0});
        const auto skew_before = skew.state();
        const auto rejected = sk::preflightLocalRelation(
            skew_before, sk::ConstraintKind::horizontal,
            {skew_line, sk::ConstraintElement::line}, {},
            {}, policy, backend);
        CHECK(rejected.status ==
              sk::LocalRelationPreflightStatus::solver_rejected);
        CHECK(rejected.solve.status ==
              sk::LocalSolveGatewayStatus::residual_violation);
        CHECK(!rejected.provisional_constraint_id);
        CHECK(!rejected.proposed_authored);
        CHECK(!rejected.solve.candidate);
        CHECK(skew.state() == skew_before);
        CHECK(backend.calls == 2);
    }


    // A2.0 RED: an untrustworthy solver's silence about redundancy
    // cannot authorize a new relation already implied by existing
    // semantic relations. An implied request must reject BEFORE calling
    // the backend and must not expose a provisional ConstraintId.
    {
        struct SilentRedundancy final : sk::ILocalSketchSolver {
            int calls{};
            sk::LocalBackendSolveResult solve(
                const sk::LocalSolveProblem& problem) override {
                ++calls;
                return {sk::LocalBackendStatus::converged,
                        problem.authored, {}, {}, std::nullopt};
            }
        } solver;
        const sk::LocalSolverAcceptancePolicy policy{1.0e-8, 1.0e-8};
        using CK = sk::ConstraintKind;
        using CE = sk::ConstraintElement;

        sk::SketchModel parallel_chain;
        const auto a = parallel_chain.addLine({0.0, 0.0}, {5.0, 0.0});
        const auto b = parallel_chain.addLine({0.0, 3.0}, {5.0, 3.0});
        const auto c = parallel_chain.addLine({0.0, 6.0}, {5.0, 6.0});
        const sk::ConstraintReference ar{a, CE::line};
        const sk::ConstraintReference br{b, CE::line};
        const sk::ConstraintReference cr{c, CE::line};
        CHECK(parallel_chain.addConstraint(CK::parallel, ar, br));
        CHECK(parallel_chain.addConstraint(CK::parallel, br, cr));
        const auto unchanged_parallel = parallel_chain.state();
        const auto transitive = sk::preflightLocalRelation(
            unchanged_parallel, CK::parallel, ar, cr, {}, policy, solver);
        CHECK(transitive.status ==
              sk::LocalRelationPreflightStatus::invalid_intent);
        CHECK(!transitive.provisional_constraint_id);
        CHECK(!transitive.proposed_authored);
        CHECK(solver.calls == 0);
        CHECK(parallel_chain.state() == unchanged_parallel);

        // Two perpendicular turns give a parallel relation; this is
        // implied by relation parity, not by seeded geometry alone.
        sk::SketchModel parity_chain;
        const auto p = parity_chain.addLine({0.0, 0.0}, {5.0, 0.0});
        const auto q = parity_chain.addLine({0.0, 2.0}, {0.0, 7.0});
        const auto r = parity_chain.addLine({0.0, 9.0}, {5.0, 9.0});
        const sk::ConstraintReference pr{p, CE::line};
        const sk::ConstraintReference qr{q, CE::line};
        const sk::ConstraintReference rr{r, CE::line};
        CHECK(parity_chain.addConstraint(CK::perpendicular, pr, qr));
        CHECK(parity_chain.addConstraint(CK::perpendicular, qr, rr));
        const auto parity_before = parity_chain.state();
        const auto double_perpendicular = sk::preflightLocalRelation(
            parity_before, CK::parallel, pr, rr, {}, policy, solver);
        CHECK(double_perpendicular.status ==
              sk::LocalRelationPreflightStatus::invalid_intent);
        CHECK(!double_perpendicular.provisional_constraint_id);
        CHECK(solver.calls == 0);
        CHECK(parity_chain.state() == parity_before);

        // Horizontal(A) + Parallel(A,B) already establishes horizontal B.
        sk::SketchModel anchored;
        const auto horizontal_a = anchored.addLine({0.0, 1.0}, {5.0, 1.0});
        const auto horizontal_b = anchored.addLine({0.0, 3.0}, {5.0, 3.0});
        const sk::ConstraintReference ha{horizontal_a, CE::line};
        const sk::ConstraintReference hb{horizontal_b, CE::line};
        CHECK(anchored.addConstraint(CK::horizontal, ha, {}));
        CHECK(anchored.addConstraint(CK::parallel, ha, hb));
        const auto anchored_before = anchored.state();
        const auto inherited_axis = sk::preflightLocalRelation(
            anchored_before, CK::horizontal, hb, {}, {}, policy, solver);
        CHECK(inherited_axis.status ==
              sk::LocalRelationPreflightStatus::invalid_intent);
        CHECK(!inherited_axis.provisional_constraint_id);
        CHECK(solver.calls == 0);
        CHECK(anchored.state() == anchored_before);

        // Coincident endpoint closure is transitive across three distinct
        // Lines and must use endpoint roles, not whole-Line identity.
        sk::SketchModel coincident_chain;
        const auto ca = coincident_chain.addLine({0.0, 0.0}, {5.0, 0.0});
        const auto cb = coincident_chain.addLine({5.0, 0.0}, {8.0, 1.0});
        const auto cc = coincident_chain.addLine({5.0, 0.0}, {7.0, 2.0});
        const sk::ConstraintReference ca_end{ca, CE::line_end};
        const sk::ConstraintReference cb_start{cb, CE::line_start};
        const sk::ConstraintReference cc_start{cc, CE::line_start};
        CHECK(coincident_chain.addConstraint(CK::coincident, ca_end, cb_start));
        CHECK(coincident_chain.addConstraint(CK::coincident, cb_start, cc_start));
        const auto coincident_before = coincident_chain.state();
        const auto endpoint_closure = sk::preflightLocalRelation(
            coincident_before, CK::coincident, ca_end, cc_start,
            {}, policy, solver);
        CHECK(endpoint_closure.status ==
              sk::LocalRelationPreflightStatus::invalid_intent);
        CHECK(!endpoint_closure.provisional_constraint_id);
        CHECK(solver.calls == 0);
        CHECK(coincident_chain.state() == coincident_before);

        // RED: a contradictory authored parity cycle is NOT evidence
        // of a redundant request. The source model allows these
        // independently legal manual relations; only the solver can
        // reject the globally impossible system. Do not silently label
        // Vertical(C) as redundant merely because ONE path from C to
        // the horizontal axis happens to have odd parity.
        sk::SketchModel contradictory;
        const auto bad_a = contradictory.addLine(
            {0.0, 0.0}, {5.0, 0.0});
        const auto bad_b = contradictory.addLine(
            {0.0, 3.0}, {5.0, 3.0});
        const auto bad_c = contradictory.addLine(
            {0.0, 6.0}, {5.0, 6.0});
        const sk::ConstraintReference bad_ar{bad_a, CE::line};
        const sk::ConstraintReference bad_br{bad_b, CE::line};
        const sk::ConstraintReference bad_cr{bad_c, CE::line};
        CHECK(contradictory.addConstraint(
            CK::horizontal, bad_ar, {}));
        CHECK(contradictory.addConstraint(
            CK::parallel, bad_ar, bad_br));
        CHECK(contradictory.addConstraint(
            CK::parallel, bad_br, bad_cr));
        CHECK(contradictory.addConstraint(
            CK::perpendicular, bad_ar, bad_cr));
        const auto contradictory_before = contradictory.state();
        const auto prior_solver_calls = solver.calls;
        const auto conflicting_cycle = sk::preflightLocalRelation(
            contradictory_before, CK::vertical, bad_cr, {},
            {}, policy, solver);
        CHECK(conflicting_cycle.status ==
              sk::LocalRelationPreflightStatus::solver_rejected);
        CHECK(conflicting_cycle.solve.status ==
              sk::LocalSolveGatewayStatus::residual_violation);
        CHECK(!conflicting_cycle.proposed_authored);
        CHECK(!conflicting_cycle.provisional_constraint_id);
        CHECK(solver.calls == prior_solver_calls + 1);
        CHECK(contradictory.state() == contradictory_before);

        // A CONSISTENT parity chain carrying the opposite value must
        // also reach the solver instead of masquerading as redundancy.
        // Horizontal(A), Perpendicular(A,B), Parallel(B,C) imply that
        // A and C are perpendicular, not parallel. This new conflicting
        // Parallel(A,C) should be diagnosed by the numeric residual.
        sk::SketchModel coherent_opposite;
        const auto good_a = coherent_opposite.addLine(
            {0.0, 0.0}, {5.0, 0.0});
        const auto good_b = coherent_opposite.addLine(
            {0.0, 3.0}, {0.0, 8.0});
        const auto good_c = coherent_opposite.addLine(
            {3.0, 3.0}, {3.0, 8.0});
        const sk::ConstraintReference good_ar{good_a, CE::line};
        const sk::ConstraintReference good_br{good_b, CE::line};
        const sk::ConstraintReference good_cr{good_c, CE::line};
        CHECK(coherent_opposite.addConstraint(
            CK::horizontal, good_ar, {}));
        CHECK(coherent_opposite.addConstraint(
            CK::perpendicular, good_ar, good_br));
        CHECK(coherent_opposite.addConstraint(
            CK::parallel, good_br, good_cr));
        const auto coherent_before = coherent_opposite.state();
        const auto coherent_call_count = solver.calls;
        const auto conflicting_intent = sk::preflightLocalRelation(
            coherent_before, CK::parallel, good_ar, good_cr,
            {}, policy, solver);
        CHECK(conflicting_intent.status ==
              sk::LocalRelationPreflightStatus::solver_rejected);
        CHECK(conflicting_intent.solve.status ==
              sk::LocalSolveGatewayStatus::residual_violation);
        CHECK(!conflicting_intent.provisional_constraint_id);
        CHECK(!conflicting_intent.proposed_authored);
        CHECK(solver.calls == coherent_call_count + 1);
        CHECK(coherent_opposite.state() == coherent_before);

        // No false rejection merely because the seed lines happen to be
        // parallel: no authored relation connects Line C yet.
        sk::SketchModel free_third;
        const auto fa = free_third.addLine({0.0, 0.0}, {5.0, 0.0});
        const auto fb = free_third.addLine({0.0, 3.0}, {5.0, 3.0});
        const auto fc = free_third.addLine({0.0, 6.0}, {5.0, 6.0});
        const sk::ConstraintReference far{fa, CE::line};
        const sk::ConstraintReference fbr{fb, CE::line};
        const sk::ConstraintReference fcr{fc, CE::line};
        CHECK(free_third.addConstraint(CK::parallel, far, fbr));
        const auto free_before = free_third.state();
        const auto before_independent_calls = solver.calls;
        const auto independent = sk::preflightLocalRelation(
            free_before, CK::parallel, far, fcr, {}, policy, solver);
        CHECK(independent.status ==
              sk::LocalRelationPreflightStatus::candidate_accepted);
        CHECK(independent.provisional_constraint_id);
        CHECK(independent.proposed_authored);
        CHECK(solver.calls == before_independent_calls + 1);
        CHECK(free_third.state() == free_before);
    }

    // The general solver gateway returns disposable derived geometry and
    // can legitimately translate underconstrained lines. Only an AUTHORING
    // command that captures this candidate must guard unrelated seeds.
    {
        struct MoveUnrelatedLine final : sk::ILocalSketchSolver {
            int calls{};
            sk::LocalBackendSolveResult solve(
                const sk::LocalSolveProblem& problem) override {
                ++calls;
                auto candidate = problem.authored;
                CHECK(candidate.lines.size() == 2U);
                candidate.lines.back().start.u += 1.0;
                candidate.lines.back().end.u += 1.0;
                return {sk::LocalBackendStatus::converged,
                        candidate, {}, {}, 7U};
            }
        } drift;
        const sk::LocalSolverAcceptancePolicy policy{1.0e-8, 1.0e-8};
        sk::SketchModel authored;
        const auto constrained = authored.addLine({0.0, 0.0}, {5.0, 0.0});
        const auto unrelated = authored.addLine({10.0, 2.0}, {15.0, 2.0});
        CHECK(constrained.valid() && unrelated.valid());
        CHECK(authored.addConstraint(
            sk::ConstraintKind::horizontal,
            {constrained, sk::ConstraintElement::line}));
        const auto before = authored.state();
        const auto problem = sk::prepareLocalSolveProblem(before);
        CHECK(problem && problem->equations.size() == 1U);
        const auto wrong = sk::evaluateLocalSolve(*problem, policy, drift);
        CHECK(drift.calls == 1);
        CHECK(wrong.status == sk::LocalSolveGatewayStatus::candidate_accepted);
        CHECK(wrong.candidate);
        CHECK(authored.state() == before);

        // No equations means zero geometric residuals; this is still a
        // transient candidate, not permission to author changed seeds.
        sk::SketchModel no_constraints;
        const auto free_a =
            no_constraints.addLine({0.0, 0.0}, {5.0, 0.0});
        const auto free_b =
            no_constraints.addLine({10.0, 2.0}, {15.0, 2.0});
        CHECK(free_a.valid() && free_b.valid());
        const auto free_state = no_constraints.state();
        const auto empty_problem = sk::prepareLocalSolveProblem(free_state);
        CHECK(empty_problem && empty_problem->equations.empty());
        const auto wrong_empty =
            sk::evaluateLocalSolve(*empty_problem, policy, drift);
        CHECK(drift.calls == 2);
        CHECK(wrong_empty.status ==
              sk::LocalSolveGatewayStatus::candidate_accepted);
        CHECK(wrong_empty.candidate);
        CHECK(no_constraints.state() == free_state);
    }

    // A2.0 RED: the new numerically solved relation is permitted to
    // correct its own connected component, NOT translate an independent
    // Horizontal Line when all residuals happen to remain satisfied.
    // The create writer must fail closed before a Part transaction.
    {
        namespace app = simplesolid2::application;
        namespace part = simplesolid2::part;
        namespace core = simplesolid2::core;
        struct CreatedRelationDrift final : sk::ILocalSketchSolver {
            bool drift_unrelated{true};
            int calls{};
            sk::LocalBackendSolveResult solve(
                const sk::LocalSolveProblem& problem) override {
                ++calls;
                auto candidate = problem.authored;
                CHECK(candidate.lines.size() == 3U);
                // The authored first Line is skew; the backend solves
                // the requested Horizontal(A) equation legitimately.
                candidate.lines.front().end.v =
                    candidate.lines.front().start.v;
                if (drift_unrelated) {
                    // C has its own unrelated Horizontal constraint.
                    // Its 2 mm translation still satisfies residuals.
                    candidate.lines.back().start.u += 2.0;
                    candidate.lines.back().end.u += 2.0;
                }
                return {sk::LocalBackendStatus::converged,
                        candidate, {}, {}, std::nullopt};
            }
        } backend;
        const sk::LocalSolverAcceptancePolicy policy{1.0e-8, 1.0e-8};
        app::DocumentSession session{
            {}, part::PartDocument::create(core::DocumentId::generate())};
        const auto made = session.execute(app::CreatePartSketchCommand{
            core::BuiltinReferenceRole::xy_plane});
        CHECK(made.ok() && made.sketch_id);
        const auto sid = *made.sketch_id;
        const auto a = session.execute(app::AddSketchLineCommand{
            sid, {0.0, 0.0}, {5.0, 1.0}});
        const auto b = session.execute(app::AddSketchLineCommand{
            sid, {10.0, 3.0}, {15.0, 3.0}});
        const auto c = session.execute(app::AddSketchLineCommand{
            sid, {20.0, 7.0}, {25.0, 7.0}});
        CHECK(a.ok() && a.entity_id);
        CHECK(b.ok() && b.entity_id);
        CHECK(c.ok() && c.entity_id);
        const auto fixed_c = session.execute(app::AddSketchRelationCommand{
            sid, session.document().revision(),
            sk::ConstraintKind::horizontal,
            {*c.entity_id, sk::ConstraintElement::line}, {}});
        CHECK(fixed_c.ok() && fixed_c.constraint_id);
        const auto original =
            session.document().findSketch(sid)->model.state();
        const auto original_rev = session.document().revision();
        const auto original_undo = session.undoDepth();
        const app::AddSolvedSketchRelationCommand create_a{
            sid, session.document().documentId(), original_rev,
            session.document().incarnationToken(),
            sk::ConstraintKind::horizontal,
            {*a.entity_id, sk::ConstraintElement::line}, {}};

        // A generic current-stage solver may return this geometry,
        // but an explicit relation authoring command cannot accept
        // it as new authoritative relational intent.
        const auto bad = session.execute(create_a, policy, backend);
        CHECK(!bad.ok() && !bad.changed && !bad.constraint_id);
        CHECK(backend.calls == 1);
        CHECK(session.document().revision() == original_rev);
        CHECK(session.undoDepth() == original_undo);
        CHECK(session.document().findSketch(sid)->model.state() == original);

        // Positive control: same numerical correction of A with
        // unrelated B/C exactly fixed remains legal. Solved geometry
        // is ephemeral; ONLY the new semantic relation is authored.
        backend.drift_unrelated = false;
        const auto accepted = session.execute(create_a, policy, backend);
        CHECK(accepted.ok() && accepted.changed && accepted.constraint_id);
        CHECK(backend.calls == 2);
        CHECK(session.undoDepth() == original_undo + 1U);
        const auto after =
            session.document().findSketch(sid)->model.state();
        CHECK(after.constraints.size() == original.constraints.size() + 1U);
        CHECK(after.lines == original.lines);
        CHECK(after.constraints.front() == original.constraints.front());
        CHECK(session.undo().ok());
        const auto undone =
            session.document().findSketch(sid)->model.state();
        CHECK(undone.lines == original.lines);
        CHECK(undone.constraints == original.constraints);
        CHECK(undone.next_constraint_id == after.next_constraint_id);
        CHECK(session.redo().ok());
        CHECK(session.document().findSketch(sid)->model.findConstraint(
            *accepted.constraint_id) != nullptr);
    }

    // A2.0 connected-component proof: a newly created A--B relation
    // extends an existing B--C component, so C is part of the legal
    // proposed graph even though it was not reachable from A before
    // this create command. Independent constrained D is never part
    // of that graph. No backend-derived coordinates are persisted.
    {
        namespace app = simplesolid2::application;
        namespace part = simplesolid2::part;
        namespace core = simplesolid2::core;
        struct ExtendedCreateComponent final : sk::ILocalSketchSolver {
            bool shift_unrelated{true};
            int calls{};
            sk::LocalBackendSolveResult solve(
                const sk::LocalSolveProblem& problem) override {
                ++calls;
                auto candidate = problem.authored;
                CHECK(candidate.lines.size() == 4U);
                auto& line = shift_unrelated
                    ? candidate.lines.back()
                    : candidate.lines[2];
                line.start.u += 2.0;
                line.end.u += 2.0;
                return {sk::LocalBackendStatus::converged,
                        candidate, {}, {}, std::nullopt};
            }
        } backend;
        const sk::LocalSolverAcceptancePolicy policy{1.0e-8, 1.0e-8};
        app::DocumentSession session{
            {}, part::PartDocument::create(core::DocumentId::generate())};
        const auto made = session.execute(app::CreatePartSketchCommand{
            core::BuiltinReferenceRole::xy_plane});
        CHECK(made.ok() && made.sketch_id);
        const auto sid = *made.sketch_id;
        const auto a = session.execute(app::AddSketchLineCommand{
            sid, {0.0, 0.0}, {5.0, 0.0}});
        const auto b = session.execute(app::AddSketchLineCommand{
            sid, {10.0, 2.0}, {15.0, 2.0}});
        const auto c = session.execute(app::AddSketchLineCommand{
            sid, {20.0, 4.0}, {25.0, 4.0}});
        const auto d = session.execute(app::AddSketchLineCommand{
            sid, {30.0, 6.0}, {35.0, 6.0}});
        CHECK(a.ok() && a.entity_id);
        CHECK(b.ok() && b.entity_id);
        CHECK(c.ok() && c.entity_id);
        CHECK(d.ok() && d.entity_id);
        using CK = sk::ConstraintKind;
        using CE = sk::ConstraintElement;
        const auto bc = session.execute(app::AddSketchRelationCommand{
            sid, session.document().revision(), CK::parallel,
            {*b.entity_id, CE::line}, {*c.entity_id, CE::line}});
        const auto d_h = session.execute(app::AddSketchRelationCommand{
            sid, session.document().revision(), CK::horizontal,
            {*d.entity_id, CE::line}, {}});
        CHECK(bc.ok() && bc.constraint_id);
        CHECK(d_h.ok() && d_h.constraint_id);
        const auto original = session.document().findSketch(sid)->model.state();
        const auto revision = session.document().revision();
        const auto undo = session.undoDepth();
        const app::AddSolvedSketchRelationCommand ab{
            sid, session.document().documentId(), revision,
            session.document().incarnationToken(), CK::parallel,
            {*a.entity_id, CE::line}, {*b.entity_id, CE::line}};

        const auto rejected = session.execute(ab, policy, backend);
        CHECK(!rejected.ok() && !rejected.changed && !rejected.constraint_id);
        CHECK(backend.calls == 1);
        CHECK(session.document().revision() == revision);
        CHECK(session.undoDepth() == undo);
        CHECK(session.document().findSketch(sid)->model.state() == original);

        // The new A--B edge joins the existing B--C edge. Translating
        // C is legal as a derived solution; translating D was not.
        backend.shift_unrelated = false;
        const auto accepted = session.execute(ab, policy, backend);
        CHECK(accepted.ok() && accepted.changed && accepted.constraint_id);
        CHECK(backend.calls == 2);
        CHECK(session.undoDepth() == undo + 1U);
        const auto after = session.document().findSketch(sid)->model.state();
        CHECK(after.lines == original.lines);
        CHECK(after.constraints.size() == original.constraints.size() + 1U);
        CHECK(session.undo().ok());
        const auto undone = session.document().findSketch(sid)->model.state();
        CHECK(undone.lines == original.lines);
        CHECK(undone.constraints == original.constraints);
        CHECK(undone.next_constraint_id == after.next_constraint_id);
        CHECK(session.redo().ok());
        CHECK(session.document().findSketch(sid)->model.findConstraint(
            *accepted.constraint_id) != nullptr);
    }

    // A2.0 command-level RED: explicit authoring is not allowed to turn
    // an unrelated free Line's solver-only translation into authored seeds.
    // The generic numerical gateway deliberately remains permissive for
    // current-stage derived geometry (Profile/Kernel tests above).
    {
        namespace app = simplesolid2::application;
        namespace part = simplesolid2::part;
        namespace core = simplesolid2::core;
        struct ShiftFreeLine final : sk::ILocalSketchSolver {
            int calls{};
            sk::LocalBackendSolveResult solve(
                const sk::LocalSolveProblem& problem) override {
                ++calls;
                auto candidate = problem.authored;
                CHECK(!candidate.lines.empty());
                if (calls == 1) {
                    auto& moved = candidate.lines.back();
                    moved.start.u += 2.0;
                    moved.end.u += 2.0;
                }
                return {sk::LocalBackendStatus::converged,
                        candidate, {}, {}, std::nullopt};
            }
        } backend;
        const sk::LocalSolverAcceptancePolicy policy{1.0e-8, 1.0e-8};
        app::DocumentSession session{
            {}, part::PartDocument::create(core::DocumentId::generate())};
        const auto made = session.execute(app::CreatePartSketchCommand{
            core::BuiltinReferenceRole::xy_plane});
        CHECK(made.ok() && made.sketch_id);
        const auto sid = *made.sketch_id;
        const auto first = session.execute(
            app::AddSketchLineCommand{
                sid, {0.0, 0.0}, {5.0, 0.0}});
        CHECK(first.ok() && first.entity_id);
        const auto original = session.document().findSketch(sid)->model.state();
        const auto fix_revision = session.document().revision();
        const auto fix_undo = session.undoDepth();
        const app::FixCurrentSketchLineCommand fix{
            sid, session.document().documentId(), fix_revision,
            session.document().incarnationToken(), *first.entity_id};
        const auto wrong_fix = session.execute(fix, policy, backend);
        CHECK(!wrong_fix.ok() && !wrong_fix.changed);
        CHECK(!wrong_fix.constraint_id);
        CHECK(backend.calls == 1);
        CHECK(session.document().revision() == fix_revision);
        CHECK(session.undoDepth() == fix_undo);
        CHECK(session.document().findSketch(sid)->model.state() == original);

        const auto extra = session.execute(
            app::AddSketchLineCommand{
                sid, {10.0, 2.0}, {15.0, 2.0}});
        CHECK(extra.ok() && extra.entity_id);
        const auto relation = session.execute(
            app::AddSketchRelationCommand{
                sid, session.document().revision(),
                sk::ConstraintKind::horizontal,
                {*first.entity_id, sk::ConstraintElement::line}, {}});
        CHECK(relation.ok() && relation.constraint_id);
        // Re-arm one malicious initial candidate; the second solver
        // would preserve any staged seeds, so without an authoring
        // guard the transaction would incorrectly commit the jump.
        backend.calls = 0;
        const auto before_delete =
            session.document().findSketch(sid)->model.state();
        const auto delete_revision = session.document().revision();
        const auto delete_undo = session.undoDepth();
        const app::EraseSolvedSketchConstraintCommand erase{
            sid, session.document().documentId(), delete_revision,
            session.document().incarnationToken(), *relation.constraint_id};
        const auto wrong_delete = session.execute(erase, policy, backend);
        CHECK(!wrong_delete.ok() && !wrong_delete.changed);
        CHECK(backend.calls == 1);
        CHECK(session.document().revision() == delete_revision);
        CHECK(session.undoDepth() == delete_undo);
        CHECK(session.document().findSketch(sid)->model.state()
              == before_delete);

        // Positive control: the same unconstrained secondary Line is
        // legal when its exact solved endpoints remain unchanged.
        struct StableBackend final : sk::ILocalSketchSolver {
            int calls{};
            sk::LocalBackendSolveResult solve(
                const sk::LocalSolveProblem& problem) override {
                ++calls;
                return {sk::LocalBackendStatus::converged,
                        problem.authored, {}, {}, std::nullopt};
            }
        } stable;
        const auto stable_fix = session.execute(
            app::FixCurrentSketchLineCommand{
                sid, session.document().documentId(),
                session.document().revision(),
                session.document().incarnationToken(), *first.entity_id},
            policy, stable);
        CHECK(stable_fix.ok() && stable_fix.changed);
        CHECK(stable_fix.constraint_id);
        CHECK(stable.calls == 2);
        CHECK(session.document().findSketch(sid)
              ->model.isFixedLine(*first.entity_id));
        const auto stable_erase = session.execute(
            app::EraseSolvedSketchConstraintCommand{
                sid, session.document().documentId(),
                session.document().revision(),
                session.document().incarnationToken(), *relation.constraint_id},
            policy, stable);
        CHECK(stable_erase.ok() && stable_erase.changed);
        CHECK(stable.calls == 4);
        const auto& after_erase = session.document().findSketch(sid)->model;
        CHECK(after_erase.findConstraint(*relation.constraint_id) == nullptr);
        CHECK(after_erase.isFixedLine(*first.entity_id));
        CHECK(after_erase.findLine(*extra.entity_id) != nullptr);
        CHECK((after_erase.findLine(*extra.entity_id)->start() ==
               sk::Point2{10.0, 2.0}));
        CHECK((after_erase.findLine(*extra.entity_id)->end() ==
               sk::Point2{15.0, 2.0}));
    }

    // R13-A2.0 application-level atomic numeric transaction prototype.
    // The backend is injected by the test. Nothing is linked to the
    // unapproved consumer PlaneGCS package or Qt authoring path.
    {
        namespace app = simplesolid2::application;
        namespace part = simplesolid2::part;
        namespace core = simplesolid2::core;
        struct EchoNumeric final : sk::ILocalSketchSolver {
            int calls{};
            std::size_t reported_dof{3U};
            app::DocumentSession* reenter{};
            std::optional<sk::SketchId> sketch_id;
            bool reentered{};
            sk::LocalBackendSolveResult solve(
                const sk::LocalSolveProblem& problem) override {
                ++calls;
                if (reenter && !reentered) {
                    reentered = true;
                    CHECK(sketch_id);
                    const auto changed = reenter->execute(
                        app::AddSketchLineCommand{
                            *sketch_id, {30.0, 30.0}, {34.0, 31.0}});
                    CHECK(changed.ok() && changed.changed);
                }
                return {sk::LocalBackendStatus::converged,
                        problem.authored, {}, {}, reported_dof};
            }
        } backend;
        const sk::LocalSolverAcceptancePolicy policy{1.0e-8, 1.0e-8};
        app::DocumentSession session{
            {}, part::PartDocument::create(core::DocumentId::generate())};
        const auto created = session.execute(
            app::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
        CHECK(created.ok() && created.sketch_id);
        const auto sid = *created.sketch_id;
        const auto line_result = session.execute(
            app::AddSketchLineCommand{
                sid, {0.0, 4.0}, {7.0, 4.0}});
        CHECK(line_result.ok() && line_result.entity_id);
        const auto id = *line_result.entity_id;
        const auto authored_before =
            session.document().findSketch(sid)->model.state();
        const auto starting_rev = session.document().revision();
        const auto starting_undo = session.undoDepth();
        const app::AddSolvedSketchRelationCommand command{
            sid, session.document().documentId(), starting_rev,
            session.document().incarnationToken(),
            sk::ConstraintKind::horizontal,
            {id, sk::ConstraintElement::line}, {}};
        auto accepted = session.execute(command, policy, backend);
        CHECK(accepted.ok() && accepted.changed &&
              accepted.constraint_id);
        CHECK(accepted.preflight_status ==
              sk::LocalRelationPreflightStatus::candidate_accepted);
        CHECK(accepted.solver_status ==
              sk::LocalSolveGatewayStatus::candidate_accepted);
        CHECK(backend.calls == 1);
        CHECK(session.undoDepth() == starting_undo + 1U);
        CHECK(session.document().findSketch(sid)
              ->model.constraintCount() == 1U);
        CHECK(session.document().findSketch(sid)
              ->model.state().lines == authored_before.lines);
        const auto committed_state =
            session.document().findSketch(sid)->model.state();
        CHECK(session.undo().ok());
        const auto after_undo =
            session.document().findSketch(sid)->model.state();
        CHECK(after_undo.lines == authored_before.lines);
        CHECK(after_undo.circles == authored_before.circles);
        CHECK(after_undo.arcs == authored_before.arcs);
        CHECK(after_undo.constraints == authored_before.constraints);
        // Undo removes authorship, never rewinds the semantic high-water
        // ConstraintId cursor: identity must never be recycled.
        CHECK(after_undo.next_constraint_id ==
              committed_state.next_constraint_id);
        CHECK(session.redo().ok());
        CHECK(session.document().findSketch(sid)
              ->model.findConstraint(*accepted.constraint_id));

        // Reusing the previous revision cannot re-execute a command.
        auto stale = session.execute(command, policy, backend);
        CHECK(!stale.ok() && !stale.changed);
        CHECK(backend.calls == 1);

        // Duplicate semantic intent cannot consume another constraint ID.
        const app::AddSolvedSketchRelationCommand repeat{
            sid, session.document().documentId(),
            session.document().revision(),
            session.document().incarnationToken(),
            sk::ConstraintKind::horizontal,
            {id, sk::ConstraintElement::line}, {}};
        const auto undo_before_repeat = session.undoDepth();
        auto duplicate = session.execute(repeat, policy, backend);
        CHECK(!duplicate.ok() && !duplicate.changed);
        CHECK(duplicate.preflight_status ==
              sk::LocalRelationPreflightStatus::invalid_intent);
        CHECK(!duplicate.constraint_id && backend.calls == 1);
        CHECK(session.undoDepth() == undo_before_repeat);

        // Fix Current Position must be implemented with current
        // *effective* solved geometry, not this raw-authored endpoint path.
        const app::AddSolvedSketchRelationCommand fix{
            sid, session.document().documentId(),
            session.document().revision(),
            session.document().incarnationToken(),
            sk::ConstraintKind::fix,
            {id, sk::ConstraintElement::line}, {}};
        CHECK(!session.execute(fix, policy, backend).ok());
        CHECK(backend.calls == 1);

        // A forged weak incarnation from a separate cold reconstruction
        // of the same DocumentId/revision is never enough to commit.
        const auto cold = part::PartDocument::restore(
            session.document().documentId(), session.document().state(),
            session.document().revision());
        CHECK(cold.ok() && cold.document);
        const app::AddSolvedSketchRelationCommand wrong_owner{
            sid, session.document().documentId(),
            session.document().revision(),
            cold.document->incarnationToken(),
            sk::ConstraintKind::vertical,
            {id, sk::ConstraintElement::line}, {}};
        CHECK(!session.execute(wrong_owner, policy, backend).ok());
        CHECK(backend.calls == 1);

        // Reusing valid identifiers in another document is not enough.
        const app::AddSolvedSketchRelationCommand wrong_document{
            sid, core::DocumentId::generate(),
            session.document().revision(),
            session.document().incarnationToken(),
            sk::ConstraintKind::vertical,
            {id, sk::ConstraintElement::line}, {}};
        const auto wrong_document_undo = session.undoDepth();
        CHECK(!session.execute(wrong_document, policy, backend).ok());
        CHECK(backend.calls == 1);
        CHECK(session.undoDepth() == wrong_document_undo);

        // Numerically impossible horizontal request on a skew free Line:
        // echo backend falsely claims convergence. Independent residual
        // review must reject it with typed diagnostics and ZERO Part change.
        const auto skew_line = session.execute(
            app::AddSketchLineCommand{
                sid, {10.0, 10.0}, {15.0, 12.0}});
        CHECK(skew_line.ok() && skew_line.entity_id);
        const auto skew_before =
            session.document().findSketch(sid)->model.state();
        const auto skew_rev = session.document().revision();
        const auto skew_undo = session.undoDepth();
        const app::AddSolvedSketchRelationCommand impossible{
            sid, session.document().documentId(), skew_rev,
            session.document().incarnationToken(),
            sk::ConstraintKind::horizontal,
            {*skew_line.entity_id, sk::ConstraintElement::line}, {}};
        const auto refused_solve =
            session.execute(impossible, policy, backend);
        CHECK(!refused_solve.ok() && !refused_solve.changed);
        CHECK(!refused_solve.constraint_id);
        CHECK(refused_solve.preflight_status ==
              sk::LocalRelationPreflightStatus::solver_rejected);
        CHECK(refused_solve.solver_status ==
              sk::LocalSolveGatewayStatus::residual_violation);
        CHECK(backend.calls == 2);
        CHECK(session.document().revision() == skew_rev);
        CHECK(session.undoDepth() == skew_undo);
        CHECK(session.document().findSketch(sid)->model.state() ==
              skew_before);

        // Stale backend result re-entry cannot overwrite a newer Part.
        const auto next_line = session.execute(
            app::AddSketchLineCommand{
                sid, {5.0, 7.0}, {8.0, 7.0}});
        CHECK(next_line.ok() && next_line.entity_id);
        const auto extra = *next_line.entity_id;
        const auto before_reentry = session.document().revision();
        const auto undo_before_reentry = session.undoDepth();
        const app::AddSolvedSketchRelationCommand reentrant{
            sid, session.document().documentId(),
            before_reentry, session.document().incarnationToken(),
            sk::ConstraintKind::horizontal,
            {extra, sk::ConstraintElement::line}, {}};
        // Even an authoritative command must reject impossible backend
        // DoF without consuming an ID, mutating Part, or creating Undo.
        // Three free Lines expose at most 12 scalar endpoint parameters.
        const auto no_write_snapshot =
            session.document().findSketch(sid)->model.state();
        backend.reported_dof = 13U;
        const auto bad_dof_write =
            session.execute(reentrant, policy, backend);
        CHECK(!bad_dof_write.ok() && !bad_dof_write.changed);
        CHECK(!bad_dof_write.constraint_id);
        CHECK(bad_dof_write.preflight_status ==
              sk::LocalRelationPreflightStatus::solver_rejected);
        CHECK(bad_dof_write.solver_status ==
              sk::LocalSolveGatewayStatus::invalid_backend_response);
        CHECK(session.undoDepth() == undo_before_reentry);
        CHECK(session.document().revision() == before_reentry);
        CHECK(session.document().findSketch(sid)->model.state() ==
              no_write_snapshot);

        backend.reported_dof = 3U;
        backend.reenter = &session;
        backend.sketch_id = sid;
        const auto refused = session.execute(reentrant, policy, backend);
        CHECK(!refused.ok() && !refused.changed);
        CHECK(!refused.constraint_id);
        CHECK(backend.reentered);
        CHECK(session.document().revision() != before_reentry);
        CHECK(session.undoDepth() == undo_before_reentry + 1U);
        CHECK(session.document().findSketch(sid)
              ->model.findConstraint(*accepted.constraint_id) != nullptr);
        CHECK(session.document().findSketch(sid)
              ->model.constraintCount() == 1U);
        CHECK(session.document().findSketch(sid)
              ->model.findLine(extra) != nullptr);
    }

    // A2.0 actual Part writer proof: an implicitly redundant new
    // relation cannot spend a ConstraintId, change DocumentRevision, or
    // create Undo even when the injected backend would say Converged.
    // A genuinely independent relation must still commit normally.
    {
        namespace app = simplesolid2::application;
        namespace part = simplesolid2::part;
        namespace core = simplesolid2::core;
        struct SilentRedundancy final : sk::ILocalSketchSolver {
            int calls{};
            sk::LocalBackendSolveResult solve(
                const sk::LocalSolveProblem& problem) override {
                ++calls;
                return {sk::LocalBackendStatus::converged,
                        problem.authored, {}, {}, std::nullopt};
            }
        } backend;
        const sk::LocalSolverAcceptancePolicy policy{1.0e-8, 1.0e-8};
        app::DocumentSession session{
            {}, part::PartDocument::create(core::DocumentId::generate())};
        const auto made = session.execute(app::CreatePartSketchCommand{
            core::BuiltinReferenceRole::xy_plane});
        CHECK(made.ok() && made.sketch_id);
        const auto sid = *made.sketch_id;
        const auto a = session.execute(app::AddSketchLineCommand{
            sid, {0.0, 0.0}, {5.0, 0.0}});
        const auto b = session.execute(app::AddSketchLineCommand{
            sid, {0.0, 2.0}, {5.0, 2.0}});
        const auto c = session.execute(app::AddSketchLineCommand{
            sid, {0.0, 4.0}, {5.0, 4.0}});
        const auto d = session.execute(app::AddSketchLineCommand{
            sid, {0.0, 6.0}, {5.0, 6.0}});
        CHECK(a.ok() && a.entity_id);
        CHECK(b.ok() && b.entity_id);
        CHECK(c.ok() && c.entity_id);
        CHECK(d.ok() && d.entity_id);
        using CK = sk::ConstraintKind;
        using CE = sk::ConstraintElement;
        const sk::ConstraintReference ar{*a.entity_id, CE::line};
        const sk::ConstraintReference br{*b.entity_id, CE::line};
        const sk::ConstraintReference cr{*c.entity_id, CE::line};
        const sk::ConstraintReference dr{*d.entity_id, CE::line};
        const auto ab = session.execute(app::AddSketchRelationCommand{
            sid, session.document().revision(), CK::parallel, ar, br});
        const auto bc = session.execute(app::AddSketchRelationCommand{
            sid, session.document().revision(), CK::parallel, br, cr});
        CHECK(ab.ok() && ab.constraint_id);
        CHECK(bc.ok() && bc.constraint_id);
        const auto snapshot = session.document().findSketch(sid)->model.state();
        const auto before_rev = session.document().revision();
        const auto before_undo = session.undoDepth();
        const auto wrong = session.execute(
            app::AddSolvedSketchRelationCommand{
                sid, session.document().documentId(), before_rev,
                session.document().incarnationToken(),
                CK::parallel, ar, cr}, policy, backend);
        CHECK(!wrong.ok() && !wrong.changed);
        CHECK(!wrong.constraint_id);
        CHECK(wrong.preflight_status ==
              sk::LocalRelationPreflightStatus::invalid_intent);
        CHECK(backend.calls == 0);
        CHECK(session.document().revision() == before_rev);
        CHECK(session.undoDepth() == before_undo);
        CHECK(session.document().findSketch(sid)->model.state() == snapshot);

        const auto valid = session.execute(
            app::AddSolvedSketchRelationCommand{
                sid, session.document().documentId(), before_rev,
                session.document().incarnationToken(),
                CK::parallel, ar, dr}, policy, backend);
        CHECK(valid.ok() && valid.changed && valid.constraint_id);
        CHECK(backend.calls == 1);
        CHECK(session.undoDepth() == before_undo + 1U);
        const auto after = session.document().findSketch(sid)->model.state();
        CHECK(after.constraints.size() == 3U);
        CHECK(after.next_constraint_id != snapshot.next_constraint_id);
        CHECK(after.lines == snapshot.lines);
        CHECK(session.undo().ok());
        const auto undone = session.document().findSketch(sid)->model.state();
        CHECK(undone.constraints == snapshot.constraints);
        CHECK(undone.lines == snapshot.lines);
        CHECK(undone.next_constraint_id == after.next_constraint_id);
        CHECK(session.redo().ok());
        CHECK(session.document().findSketch(sid)->model.findConstraint(
            *valid.constraint_id) != nullptr);
    }

    // A2.0 RED: a second, independently constrained component must not
    // drift during Fix or solver-backed Delete. Every residual is valid:
    // both lines are horizontal, yet only component A is being edited.
    {
        namespace app = simplesolid2::application;
        namespace part = simplesolid2::part;
        namespace core = simplesolid2::core;
        struct DisconnectedHorizontalDrift final : sk::ILocalSketchSolver {
            bool shift_every_solve{};
            int calls{};
            sk::LocalBackendSolveResult solve(
                const sk::LocalSolveProblem& problem) override {
                ++calls;
                auto candidate = problem.authored;
                CHECK(candidate.lines.size() == 2U);
                if (shift_every_solve || calls == 1) {
                    candidate.lines[1].start.u += 2.0;
                    candidate.lines[1].end.u += 2.0;
                }
                return {sk::LocalBackendStatus::converged,
                        candidate, {}, {}, std::nullopt};
            }
        } backend;
        const sk::LocalSolverAcceptancePolicy policy{1.0e-8, 1.0e-8};
        app::DocumentSession session{
            {}, part::PartDocument::create(core::DocumentId::generate())};
        const auto made = session.execute(app::CreatePartSketchCommand{
            core::BuiltinReferenceRole::xy_plane});
        CHECK(made.ok() && made.sketch_id);
        const auto sid = *made.sketch_id;
        const auto line_a = session.execute(app::AddSketchLineCommand{
            sid, {0.0, 0.0}, {5.0, 0.0}});
        const auto line_b = session.execute(app::AddSketchLineCommand{
            sid, {10.0, 2.0}, {15.0, 2.0}});
        CHECK(line_a.ok() && line_a.entity_id);
        CHECK(line_b.ok() && line_b.entity_id);
        const auto horizontal_a = session.execute(
            app::AddSketchRelationCommand{
                sid, session.document().revision(),
                sk::ConstraintKind::horizontal,
                {*line_a.entity_id, sk::ConstraintElement::line}, {}});
        const auto horizontal_b = session.execute(
            app::AddSketchRelationCommand{
                sid, session.document().revision(),
                sk::ConstraintKind::horizontal,
                {*line_b.entity_id, sk::ConstraintElement::line}, {}});
        CHECK(horizontal_a.ok() && horizontal_a.constraint_id);
        CHECK(horizontal_b.ok() && horizontal_b.constraint_id);
        const auto original =
            session.document().findSketch(sid)->model.state();
        CHECK(original.constraints.size() == 2U);
        const auto revision_before = session.document().revision();
        const auto undo_before = session.undoDepth();

        // Re-solving after a provisional Fix returns exactly the same
        // apparently valid drift of component B. This must not authorize
        // an extra Fix transaction or publication of this candidate.
        backend.shift_every_solve = true;
        const app::FixCurrentSketchLineCommand fix{
            sid, session.document().documentId(), revision_before,
            session.document().incarnationToken(), *line_a.entity_id};
        const auto bad_fix = session.execute(fix, policy, backend);
        CHECK(!bad_fix.ok() && !bad_fix.changed);
        CHECK(!bad_fix.constraint_id);
        CHECK(backend.calls == 1);
        CHECK(session.document().revision() == revision_before);
        CHECK(session.undoDepth() == undo_before);
        CHECK(session.document().findSketch(sid)->model.state() == original);

        // Delete(A) previously materialized the full solved candidate;
        // component B then jumped 2 mm despite having an independent
        // horizontal relation and no selected target in that component.
        backend.calls = 0;
        backend.shift_every_solve = false;
        const app::EraseSolvedSketchConstraintCommand erase{
            sid, session.document().documentId(), revision_before,
            session.document().incarnationToken(), *horizontal_a.constraint_id};
        const auto bad_delete = session.execute(erase, policy, backend);
        CHECK(!bad_delete.ok() && !bad_delete.changed);
        CHECK(backend.calls == 1);
        CHECK(session.document().revision() == revision_before);
        CHECK(session.undoDepth() == undo_before);
        CHECK(session.document().findSketch(sid)->model.state() == original);
    }

    // Positive connected-component control: Delete(A--B) is allowed to
    // preserve the accepted effective position of C when B--C has its
    // own surviving relation. Traversal must be TRANSITIVE, not only
    // the two directly selected Line ids.
    {
        namespace app = simplesolid2::application;
        namespace part = simplesolid2::part;
        namespace core = simplesolid2::core;
        struct ConnectedEffective final : sk::ILocalSketchSolver {
            int calls{};
            sk::LocalBackendSolveResult solve(
                const sk::LocalSolveProblem& problem) override {
                ++calls;
                auto solved = problem.authored;
                CHECK(solved.lines.size() == 3U);
                if (calls == 1) {
                    solved.lines.back().start.u += 2.0;
                    solved.lines.back().end.u += 2.0;
                }
                return {sk::LocalBackendStatus::converged,
                        solved, {}, {}, std::nullopt};
            }
        } backend;
        const sk::LocalSolverAcceptancePolicy policy{1.0e-8, 1.0e-8};
        app::DocumentSession session{
            {}, part::PartDocument::create(core::DocumentId::generate())};
        const auto made = session.execute(app::CreatePartSketchCommand{
            core::BuiltinReferenceRole::xy_plane});
        CHECK(made.ok() && made.sketch_id);
        const auto sid = *made.sketch_id;
        const auto a = session.execute(app::AddSketchLineCommand{
            sid, {0.0, 0.0}, {5.0, 0.0}});
        const auto b = session.execute(app::AddSketchLineCommand{
            sid, {10.0, 2.0}, {15.0, 2.0}});
        const auto c = session.execute(app::AddSketchLineCommand{
            sid, {20.0, 4.0}, {25.0, 4.0}});
        CHECK(a.ok() && a.entity_id);
        CHECK(b.ok() && b.entity_id);
        CHECK(c.ok() && c.entity_id);
        const auto a_b = session.execute(app::AddSketchRelationCommand{
            sid, session.document().revision(),
            sk::ConstraintKind::parallel,
            {*a.entity_id, sk::ConstraintElement::line},
            {*b.entity_id, sk::ConstraintElement::line}});
        const auto b_c = session.execute(app::AddSketchRelationCommand{
            sid, session.document().revision(),
            sk::ConstraintKind::parallel,
            {*b.entity_id, sk::ConstraintElement::line},
            {*c.entity_id, sk::ConstraintElement::line}});
        CHECK(a_b.ok() && a_b.constraint_id);
        CHECK(b_c.ok() && b_c.constraint_id);
        const auto before = session.document().findSketch(sid)->model.state();
        const auto old_undo = session.undoDepth();
        const app::EraseSolvedSketchConstraintCommand erase{
            sid, session.document().documentId(),
            session.document().revision(),
            session.document().incarnationToken(), *a_b.constraint_id};
        const auto deleted = session.execute(erase, policy, backend);
        CHECK(deleted.ok() && deleted.changed);
        CHECK(backend.calls == 2);
        CHECK(session.undoDepth() == old_undo + 1U);
        const auto& current = session.document().findSketch(sid)->model;
        CHECK(current.findConstraint(*a_b.constraint_id) == nullptr);
        CHECK(current.findConstraint(*b_c.constraint_id) != nullptr);
        CHECK((current.findLine(*c.entity_id)->start() ==
               sk::Point2{22.0, 4.0}));
        CHECK((current.findLine(*c.entity_id)->end() ==
               sk::Point2{27.0, 4.0}));
        CHECK(session.undo().ok());
        const auto undone = session.document().findSketch(sid)->model.state();
        CHECK(undone.lines == before.lines);
        CHECK(undone.constraints == before.constraints);
        CHECK(session.redo().ok());
        CHECK((session.document().findSketch(sid)->model
                   .findLine(*c.entity_id)->start() ==
               sk::Point2{22.0, 4.0}));
    }

    // A2.0 mixed-primitive regression: the Line-only native bridge
    // must never reinterpret Circle/Arc as solver-editable geometry.
    // Fix/Delete remain atomic with exact, unrelated primitive seeds.
    {
        namespace app = simplesolid2::application;
        namespace part = simplesolid2::part;
        namespace core = simplesolid2::core;
        struct MixedPrimitiveBackend final : sk::ILocalSketchSolver {
            enum class Mode { echo, move_circle, resize_arc };
            Mode mode{Mode::echo};
            int calls{};
            sk::LocalBackendSolveResult solve(
                const sk::LocalSolveProblem& problem) override {
                ++calls;
                auto candidate = problem.authored;
                CHECK(candidate.circles.size() == 1U);
                CHECK(candidate.arcs.size() == 1U);
                if (mode == Mode::move_circle) {
                    candidate.circles.front().center.u += 1.0;
                } else if (mode == Mode::resize_arc) {
                    candidate.arcs.front().radius += 0.5;
                }
                return {sk::LocalBackendStatus::converged,
                        candidate, {}, {}, std::nullopt};
            }
        } backend;
        const sk::LocalSolverAcceptancePolicy policy{1.0e-8, 1.0e-8};
        app::DocumentSession session{
            {}, part::PartDocument::create(core::DocumentId::generate())};
        const auto made = session.execute(app::CreatePartSketchCommand{
            core::BuiltinReferenceRole::xy_plane});
        CHECK(made.ok() && made.sketch_id);
        const auto sid = *made.sketch_id;
        const auto line = session.execute(app::AddSketchLineCommand{
            sid, {0.0, 0.0}, {5.0, 0.0}});
        const auto circle = session.execute(app::AddSketchCircleCommand{
            sid, {12.0, 3.0}, 2.5});
        const auto arc = session.execute(app::AddSketchArcCommand{
            sid, {20.0, 3.0}, 1.5, 0.0, 1.2});
        CHECK(line.ok() && line.entity_id);
        CHECK(circle.ok() && circle.entity_id);
        CHECK(arc.ok() && arc.entity_id);
        const auto horizontal = session.execute(
            app::AddSketchRelationCommand{
                sid, session.document().revision(),
                sk::ConstraintKind::horizontal,
                {*line.entity_id, sk::ConstraintElement::line}, {}});
        CHECK(horizontal.ok() && horizontal.constraint_id);
        const auto original =
            session.document().findSketch(sid)->model.state();
        CHECK(original.circles.size() == 1U && original.arcs.size() == 1U);
        const auto revision = session.document().revision();
        const auto undo = session.undoDepth();
        const app::FixCurrentSketchLineCommand fix{
            sid, session.document().documentId(), revision,
            session.document().incarnationToken(), *line.entity_id};
        const app::EraseSolvedSketchConstraintCommand erase{
            sid, session.document().documentId(), revision,
            session.document().incarnationToken(), *horizontal.constraint_id};

        // Even a Converged answer with satisfied Line residuals must
        // not publish a Circle/Arc change through a Line-only solver.
        backend.mode = MixedPrimitiveBackend::Mode::move_circle;
        const auto bad_fix = session.execute(fix, policy, backend);
        CHECK(!bad_fix.ok() && !bad_fix.changed && !bad_fix.constraint_id);
        CHECK(backend.calls == 1);
        CHECK(session.document().revision() == revision);
        CHECK(session.undoDepth() == undo);
        CHECK(session.document().findSketch(sid)->model.state() == original);

        backend.mode = MixedPrimitiveBackend::Mode::resize_arc;
        const auto bad_delete = session.execute(erase, policy, backend);
        CHECK(!bad_delete.ok() && !bad_delete.changed);
        CHECK(backend.calls == 2);
        CHECK(session.document().revision() == revision);
        CHECK(session.undoDepth() == undo);
        CHECK(session.document().findSketch(sid)->model.state() == original);

        // Legal unchanged non-Line primitives survive Fix + Delete.
        backend.mode = MixedPrimitiveBackend::Mode::echo;
        const auto good_fix = session.execute(fix, policy, backend);
        CHECK(good_fix.ok() && good_fix.changed && good_fix.constraint_id);
        CHECK(backend.calls == 4);
        const auto good_erase = session.execute(
            app::EraseSolvedSketchConstraintCommand{
                sid, session.document().documentId(),
                session.document().revision(),
                session.document().incarnationToken(),
                *horizontal.constraint_id},
            policy, backend);
        CHECK(good_erase.ok() && good_erase.changed);
        CHECK(backend.calls == 6);
        const auto after = session.document().findSketch(sid)->model.state();
        CHECK(after.circles == original.circles);
        CHECK(after.arcs == original.arcs);
        CHECK(after.lines == original.lines);
        CHECK(after.constraints.size() == 1U);
        CHECK(session.document().findSketch(sid)->model.isFixedLine(
            *line.entity_id));
        CHECK(session.undo().ok());
        CHECK(session.document().findSketch(sid)->model.state().circles ==
              original.circles);
        CHECK(session.document().findSketch(sid)->model.state().arcs ==
              original.arcs);
        CHECK(session.redo().ok());
        CHECK(session.document().findSketch(sid)->model.state().circles ==
              original.circles);
        CHECK(session.document().findSketch(sid)->model.state().arcs ==
              original.arcs);
    }

    // A2.0 RED: Fix Current Position must capture current NUMERIC
    // solution, rather than the historical skew authored seed.
    // Source is intentionally local/XY, never a projected or Body-stage Line.
    {
        namespace app = simplesolid2::application;
        namespace part = simplesolid2::part;
        namespace core = simplesolid2::core;
        struct SolveToHorizontal final : sk::ILocalSketchSolver {
            int calls{};
            sk::LocalBackendSolveResult solve(
                const sk::LocalSolveProblem& problem) override {
                ++calls;
                auto candidate = problem.authored;
                for (auto& line : candidate.lines) {
                    line.end.v = line.start.v;
                }
                return {sk::LocalBackendStatus::converged,
                        candidate, {}, {},
                        problem.fixed_lines.empty() ? 3U : 0U};
            }
        } backend;
        const sk::LocalSolverAcceptancePolicy policy{1.0e-8, 1.0e-8};
        app::DocumentSession session{
            {}, part::PartDocument::create(core::DocumentId::generate())};
        const auto created = session.execute(
            app::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
        CHECK(created.ok() && created.sketch_id);
        const auto sid = *created.sketch_id;
        const auto line = session.execute(
            app::AddSketchLineCommand{
                sid, {0.0, 0.0}, {5.0, 2.0}});
        CHECK(line.ok() && line.entity_id);
        const auto id = *line.entity_id;
        const auto horizontal = session.execute(
            app::AddSketchRelationCommand{
                sid, session.document().revision(),
                sk::ConstraintKind::horizontal,
                {id, sk::ConstraintElement::line}, {}});
        CHECK(horizontal.ok() && horizontal.constraint_id);
        const auto before_fix =
            session.document().findSketch(sid)->model.state();
        const auto rev = session.document().revision();
        const auto undo = session.undoDepth();
        const app::FixCurrentSketchLineCommand command{
            sid, session.document().documentId(), rev,
            session.document().incarnationToken(), id};
        const auto frozen = session.execute(command, policy, backend);
        CHECK(frozen.ok() && frozen.changed && frozen.constraint_id);
        CHECK(backend.calls == 2);
        CHECK(session.document().revision() != rev);
        CHECK(session.undoDepth() == undo + 1U);
        const auto& model =
            session.document().findSketch(sid)->model;
        CHECK(model.isFixedLine(id));
        CHECK(model.findLine(id) != nullptr);
        CHECK((model.findLine(id)->start() == sk::Point2{0.0, 0.0}));
        CHECK((model.findLine(id)->end() == sk::Point2{5.0, 0.0}));
        CHECK(model.constraintCount() == 2U);
        CHECK(session.undo().ok());
        CHECK(session.document().findSketch(sid)->model.state().lines ==
              before_fix.lines);
        CHECK(!session.document().findSketch(sid)->model.isFixedLine(id));
        CHECK(session.redo().ok());
        CHECK(session.document().findSketch(sid)->model.isFixedLine(id));
        CHECK((session.document().findSketch(sid)->model.findLine(id)->end() ==
              sk::Point2{5.0, 0.0}));
        const auto fixed_rev = session.document().revision();
        const auto fixed_undo = session.undoDepth();
        const auto again = session.execute(
            app::FixCurrentSketchLineCommand{
                sid, session.document().documentId(), fixed_rev,
                session.document().incarnationToken(), id},
            policy, backend);
        CHECK(!again.ok() && !again.changed);
        CHECK(!again.constraint_id);
        CHECK(session.document().revision() == fixed_rev);
        CHECK(session.undoDepth() == fixed_undo);
        CHECK(backend.calls == 2);
    }

    // Fix Current Position must never accept a second solve that
    // independently moves other geometry, even when all residuals pass.
    {
        namespace app = simplesolid2::application;
        namespace part = simplesolid2::part;
        namespace core = simplesolid2::core;
        struct AlternateSolution final : sk::ILocalSketchSolver {
            int calls{};
            app::DocumentSession* reenter{};
            std::optional<sk::SketchId> sketch_id;
            bool reentered{};
            sk::LocalBackendSolveResult solve(
                const sk::LocalSolveProblem& problem) override {
                ++calls;
                auto candidate = problem.authored;
                if (calls == 2 && reenter) {
                    reentered = true;
                    CHECK(sketch_id);
                    const auto changed = reenter->execute(
                        app::AddSketchLineCommand{
                            *sketch_id, {30.0, 0.0}, {34.0, 0.0}});
                    CHECK(changed.ok() && changed.changed);
                } else if (calls == 2) {
                    auto& other = candidate.lines.back();
                    other.start.u += 1.0;
                    other.end.u += 1.0;
                }
                return {sk::LocalBackendStatus::converged,
                        candidate, {}, {},
                        problem.fixed_lines.empty() ? 8U : 4U};
            }
        } backend;
        const sk::LocalSolverAcceptancePolicy policy{1.0e-8, 1.0e-8};
        app::DocumentSession session{
            {}, part::PartDocument::create(core::DocumentId::generate())};
        const auto created = session.execute(
            app::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
        CHECK(created.ok() && created.sketch_id);
        const auto sid = *created.sketch_id;
        const auto a = session.execute(
            app::AddSketchLineCommand{
                sid, {0.0, 0.0}, {5.0, 0.0}});
        const auto b = session.execute(
            app::AddSketchLineCommand{
                sid, {10.0, 0.0}, {15.0, 0.0}});
        CHECK(a.ok() && a.entity_id && b.ok() && b.entity_id);
        const auto prior = session.document().findSketch(sid)->model.state();
        const auto rev = session.document().revision();
        const auto undo = session.undoDepth();
        const app::FixCurrentSketchLineCommand command{
            sid, session.document().documentId(), rev,
            session.document().incarnationToken(), *a.entity_id};
        const auto alternative = session.execute(command, policy, backend);
        CHECK(!alternative.ok() && !alternative.changed);
        CHECK(!alternative.constraint_id);
        CHECK(backend.calls == 2);
        CHECK(session.document().findSketch(sid)->model.state() == prior);
        CHECK(session.document().revision() == rev);
        CHECK(session.undoDepth() == undo);

        // An old command is rejected before any additional backend call.
        const auto changed = session.execute(
            app::AddSketchLineCommand{
                sid, {20.0, 0.0}, {25.0, 0.0}});
        CHECK(changed.ok() && changed.changed);
        CHECK(!session.execute(command, policy, backend).ok());
        CHECK(backend.calls == 2);

        // A backend that re-enters DocumentSession during the second
        // validation invalidates the original command without hiding
        // its own independently committed change or consuming a Fix ID.
        backend.calls = 0;
        backend.reenter = &session;
        backend.sketch_id = sid;
        const auto current_rev = session.document().revision();
        const auto current_undo = session.undoDepth();
        const auto current_constraints =
            session.document().findSketch(sid)->model.constraintCount();
        const app::FixCurrentSketchLineCommand reentrant{
            sid, session.document().documentId(), current_rev,
            session.document().incarnationToken(), *a.entity_id};
        const auto stale_solve = session.execute(
            reentrant, policy, backend);
        CHECK(!stale_solve.ok() && !stale_solve.changed);
        CHECK(!stale_solve.constraint_id);
        CHECK(backend.calls == 2 && backend.reentered);
        CHECK(session.document().revision() != current_rev);
        CHECK(session.undoDepth() == current_undo + 1U);
        CHECK(session.document().findSketch(sid)->model.constraintCount()
              == current_constraints);
        CHECK(!session.document().findSketch(sid)->model.isFixedLine(
            *a.entity_id));
    }

    // A2.0 RED: Delete relation must retain last accepted effective
    // geometry as authored seeds instead of snapping back to old inputs.
    {
        namespace app = simplesolid2::application;
        namespace part = simplesolid2::part;
        namespace core = simplesolid2::core;
        struct HorizontalEffective final : sk::ILocalSketchSolver {
            int calls{};
            sk::LocalBackendSolveResult solve(
                const sk::LocalSolveProblem& problem) override {
                ++calls;
                auto candidate = problem.authored;
                if (!problem.equations.empty()) {
                    for (auto& line : candidate.lines) {
                        line.end.v = line.start.v;
                    }
                }
                return {sk::LocalBackendStatus::converged,
                        candidate, {}, {},
                        problem.equations.empty() ? 4U : 3U};
            }
        } backend;
        const sk::LocalSolverAcceptancePolicy policy{1.0e-8, 1.0e-8};
        app::DocumentSession session{
            {}, part::PartDocument::create(core::DocumentId::generate())};
        const auto created = session.execute(
            app::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
        CHECK(created.ok() && created.sketch_id);
        const auto sid = *created.sketch_id;
        const auto line = session.execute(
            app::AddSketchLineCommand{
                sid, {0.0, 0.0}, {5.0, 2.0}});
        CHECK(line.ok() && line.entity_id);
        const auto horizontal = session.execute(
            app::AddSketchRelationCommand{
                sid, session.document().revision(),
                sk::ConstraintKind::horizontal,
                {*line.entity_id, sk::ConstraintElement::line}, {}});
        CHECK(horizontal.ok() && horizontal.constraint_id);
        const auto original =
            session.document().findSketch(sid)->model.state();
        const auto original_rev = session.document().revision();
        const auto original_undo = session.undoDepth();
        const app::EraseSolvedSketchConstraintCommand command{
            sid, session.document().documentId(), original_rev,
            session.document().incarnationToken(), *horizontal.constraint_id};
        const auto erased = session.execute(command, policy, backend);
        CHECK(erased.ok() && erased.changed);
        CHECK(backend.calls == 2);
        const auto& after = session.document().findSketch(sid)->model;
        CHECK(after.constraintCount() == 0U);
        CHECK((after.findLine(*line.entity_id)->end() ==
               sk::Point2{5.0, 0.0}));
        CHECK(after.state().next_constraint_id ==
              original.next_constraint_id);
        CHECK(session.undoDepth() == original_undo + 1U);
        CHECK(session.undo().ok());
        const auto undo_state =
            session.document().findSketch(sid)->model.state();
        CHECK(undo_state.lines == original.lines);
        CHECK(undo_state.constraints == original.constraints);
        CHECK(session.redo().ok());
        CHECK(session.document().findSketch(sid)->model.constraintCount() == 0U);
        CHECK((session.document().findSketch(sid)->model
                   .findLine(*line.entity_id)->end() ==
               sk::Point2{5.0, 0.0}));
        const auto no_op_rev = session.document().revision();
        const auto no_op_undo = session.undoDepth();
        const auto removed_twice = session.execute(
            app::EraseSolvedSketchConstraintCommand{
                sid, session.document().documentId(), no_op_rev,
                session.document().incarnationToken(), *horizontal.constraint_id},
            policy, backend);
        CHECK(!removed_twice.ok() && !removed_twice.changed);
        CHECK(session.document().revision() == no_op_rev);
        CHECK(session.undoDepth() == no_op_undo);
        CHECK(backend.calls == 2);
    }

    // Solved Delete must fail closed if the remaining solver shifts an
    // otherwise unconstrained Line, or mutates the owning Part mid-solve.
    {
        namespace app = simplesolid2::application;
        namespace part = simplesolid2::part;
        namespace core = simplesolid2::core;
        struct DivertOrReenter final : sk::ILocalSketchSolver {
            int calls{};
            app::DocumentSession* reenter{};
            std::optional<sk::SketchId> sketch_id;
            bool reentered{};
            sk::LocalBackendSolveResult solve(
                const sk::LocalSolveProblem& problem) override {
                ++calls;
                auto candidate = problem.authored;
                if (calls == 2 && reenter) {
                    reentered = true;
                    CHECK(sketch_id);
                    const auto extra = reenter->execute(
                        app::AddSketchLineCommand{
                            *sketch_id, {30.0, 0.0}, {35.0, 0.0}});
                    CHECK(extra.ok() && extra.changed);
                } else if (calls == 2) {
                    candidate.lines.back().start.u += 1.0;
                    candidate.lines.back().end.u += 1.0;
                }
                return {sk::LocalBackendStatus::converged,
                        candidate, {}, {},
                        problem.equations.empty() ? 8U : 7U};
            }
        } backend;
        const sk::LocalSolverAcceptancePolicy policy{1.0e-8,1.0e-8};
        app::DocumentSession session{
            {}, part::PartDocument::create(core::DocumentId::generate())};
        const auto created = session.execute(
            app::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
        CHECK(created.ok() && created.sketch_id);
        const auto sid = *created.sketch_id;
        const auto line = session.execute(
            app::AddSketchLineCommand{
                sid, {0.0, 0.0}, {5.0, 0.0}});
        const auto other = session.execute(
            app::AddSketchLineCommand{
                sid, {10.0, 0.0}, {15.0, 0.0}});
        CHECK(line.ok() && line.entity_id &&
              other.ok() && other.entity_id);
        const auto added = session.execute(
            app::AddSketchRelationCommand{
                sid, session.document().revision(),
                sk::ConstraintKind::horizontal,
                {*line.entity_id, sk::ConstraintElement::line}, {}});
        CHECK(added.ok() && added.constraint_id);
        const auto before =
            session.document().findSketch(sid)->model.state();
        const auto rev = session.document().revision();
        const auto undo = session.undoDepth();
        const app::EraseSolvedSketchConstraintCommand command{
            sid, session.document().documentId(), rev,
            session.document().incarnationToken(), *added.constraint_id};
        const auto divergent = session.execute(
            command, policy, backend);
        CHECK(!divergent.ok() && !divergent.changed);
        CHECK(backend.calls == 2);
        CHECK(session.document().findSketch(sid)->model.state() == before);
        CHECK(session.document().revision() == rev);
        CHECK(session.undoDepth() == undo);

        backend.calls = 0;
        backend.reenter = &session;
        backend.sketch_id = sid;
        const auto changing = session.execute(
            command, policy, backend);
        CHECK(!changing.ok() && !changing.changed);
        CHECK(backend.calls == 2 && backend.reentered);
        CHECK(session.document().revision() != rev);
        CHECK(session.undoDepth() == undo + 1U);
        const auto afterReentry =
            session.document().findSketch(sid)->model.state();
        CHECK(afterReentry.constraints == before.constraints);
        CHECK(afterReentry.lines.size() == before.lines.size() + 1U);
        CHECK(!session.execute(command, policy, backend).ok());
        CHECK(backend.calls == 2);
    }

    sk::SketchModel model;
    const auto line = model.addLine({1.0, 2.0}, {5.0, 4.0});
    const auto seed = model.state();
    CHECK(seed.constraints.empty());
    CHECK(model.constraintCount() == 0U);

    // Invalid and duplicate requests must not consume an ID or mutate state.
    const auto cursor0 = model.constraintIdCursor();
    CHECK(!model.addHorizontal({}));
    CHECK(model.constraintIdCursor() == cursor0);
    const auto id = model.addHorizontal(line);
    CHECK(id && id->valid() && id->serialized() == "1");
    CHECK(sk::ConstraintId::parse(id->serialized()) == id);
    CHECK(model.constraintCount() == 1U);
    CHECK(model.findConstraint(*id) != nullptr);
    CHECK(!model.addHorizontal(line));
    CHECK(model.constraintCount() == 1U);
    CHECK(model.state().constraints.front().kind == sk::ConstraintKind::horizontal);
    CHECK(model.state().constraints.front().first.entity == line);
    CHECK(model.state().constraints.front().first.element == sk::ConstraintElement::line);
    CHECK(model.state().constraints.front().second == sk::ConstraintReference{});
    CHECK(model.state().next_constraint_id.serialized() == "2");

    // Creating a relation is authored design intent, not a geometry rewrite.
    CHECK(model.findLine(line)->start() == seed.lines.front().start);
    CHECK(model.findLine(line)->end() == seed.lines.front().end);
    CHECK(model.state() != seed);

    // State copy / restore carries exact IDs and semantics; cursor is monotonic.
    const auto snapshot = model.state();
    auto restored = sk::SketchModel::restore(snapshot);
    CHECK(restored.has_value());
    CHECK(restored->state() == snapshot);
    CHECK(*restored == model);
    CHECK(restored->eraseConstraint(*id));
    CHECK(!restored->eraseConstraint(*id));
    CHECK(restored->findConstraint(*id) == nullptr);
    CHECK(restored->constraintIdCursor().serialized() == "2");
    const auto id2 = restored->addHorizontal(line);
    CHECK(id2 && id2->serialized() == "2");
    CHECK(model.findConstraint(*id) != nullptr);

    // Deleted geometry cascades local semantic dependants without reusing IDs.
    CHECK(model.erase(line));
    CHECK(model.constraintCount() == 0U);
    CHECK(model.findConstraint(*id) == nullptr);
    CHECK(model.constraintIdCursor().serialized() == "2");
    const auto newLine = model.addLine({0.0, 0.0}, {3.0, 1.0});
    const auto newConstraint = model.addHorizontal(newLine);
    CHECK(newConstraint && newConstraint->serialized() == "2");

    // Fail-closed restoration: missing target, duplicate ID, bad role, bad cursor.
    auto broken = snapshot;
    broken.constraints.front().first.entity = *sk::EntityId::parse("9999");
    CHECK(!sk::SketchModel::restore(broken));
    broken = snapshot;
    broken.constraints.push_back(broken.constraints.front());
    CHECK(!sk::SketchModel::restore(broken));
    broken = snapshot;
    broken.constraints.front().first.element = sk::ConstraintElement::line_start;
    CHECK(!sk::SketchModel::restore(broken));
    broken = snapshot;
    broken.next_constraint_id = *sk::ConstraintIdCursor::parse("1");
    CHECK(!sk::SketchModel::restore(broken));

    // Native DocumentSession: one atomic command, revision guard and exact Undo/Redo.
    {
        namespace app = simplesolid2::application;
        namespace part = simplesolid2::part;
        namespace core = simplesolid2::core;
        app::DocumentSession session{
            {}, part::PartDocument::create(core::DocumentId::generate())};
        const auto made = session.execute(
            app::CreatePartSketchCommand{core::BuiltinReferenceRole::xy_plane});
        CHECK(made.ok() && made.sketch_id);
        const auto lineMade = session.execute(
            app::AddSketchLineCommand{*made.sketch_id, {2.0, 3.0}, {7.0, 5.0}});
        CHECK(lineMade.ok() && lineMade.entity_id);
        const auto rev = session.document().revision();
        const auto historyBefore = session.undoDepth();
        const auto created = session.execute(app::AddHorizontalSketchConstraintCommand{
            *made.sketch_id, rev, *lineMade.entity_id});
        CHECK(created.ok() && created.changed && created.constraint_id);
        CHECK(created.constraint_id->serialized() == "1");
        CHECK(session.undoDepth() == historyBefore + 1U);
        CHECK(session.needsSave());
        // v21 persists authored relations; cold reopen proves identity.
        part::PartDocumentStore store;
        const auto path = std::filesystem::temp_directory_path() /
            ("r13a1_v21_" +
                std::string{session.document().documentId().value()} +
                ".ss2part");
        const auto saved = store.createNew(path, session.document());
        CHECK(saved.ok());
        const auto loaded = store.load(path);
        CHECK(loaded.ok());
        const auto* coldSketch = loaded.document->findSketch(*made.sketch_id);
        CHECK(coldSketch != nullptr);
        CHECK(coldSketch->model.state() ==
              session.document().findSketch(*made.sketch_id)->model.state());
        CHECK(coldSketch->model.findConstraint(*created.constraint_id) != nullptr);
        CHECK(coldSketch->model.constraintIdCursor().serialized() == "2");
        CHECK(std::filesystem::remove(path));
        CHECK(session.document().findSketch(*made.sketch_id)->model
            .findConstraint(*created.constraint_id) != nullptr);

        const auto duplicate = session.execute(app::AddHorizontalSketchConstraintCommand{
            *made.sketch_id, session.document().revision(), *lineMade.entity_id});
        CHECK(!duplicate.ok() && !duplicate.changed);
        CHECK(duplicate.diagnostic.code == app::DocumentSessionErrorCode::invalid_command);
        CHECK(session.undoDepth() == historyBefore + 1U);

        const auto stale = session.execute(app::AddHorizontalSketchConstraintCommand{
            *made.sketch_id, rev, *lineMade.entity_id});
        CHECK(!stale.ok());
        CHECK(stale.diagnostic.code == app::DocumentSessionErrorCode::revision_diverged);

        CHECK(session.undo().changed);
        CHECK(session.document().findSketch(*made.sketch_id)->model.constraintCount() == 0U);
        CHECK(session.redo().changed);
        CHECK(session.document().findSketch(*made.sketch_id)->model
            .findConstraint(*created.constraint_id) != nullptr);

        CHECK(session.undo().changed);
        // A new branch after Undo must not recycle even a retired local ConstraintId.
        const auto forked = session.execute(app::AddHorizontalSketchConstraintCommand{
            *made.sketch_id, session.document().revision(), *lineMade.entity_id});
        CHECK(forked.ok() && forked.changed && forked.constraint_id);
        CHECK(forked.constraint_id->serialized() == "2");
        CHECK(!session.canRedo());

        const auto deleted = session.execute(app::EraseSketchConstraintCommand{
            *made.sketch_id, session.document().revision(), *forked.constraint_id});
        CHECK(deleted.ok() && deleted.changed);
        CHECK(session.document().findSketch(*made.sketch_id)->model.constraintCount() == 0U);
        CHECK(session.undo().changed);
        CHECK(session.document().findSketch(*made.sketch_id)->model
            .findConstraint(*forked.constraint_id) != nullptr);
        CHECK(session.redo().changed);
        CHECK(session.document().findSketch(*made.sketch_id)->model.constraintCount() == 0U);
    }

    // A1.1 has no accepted solver yet: an authored Horizontal on a
    // geometrically closed rectangle MUST NOT publish seed geometry to
    // Profile/Kernel as if it had passed a constraint solve.
    {
        namespace app = simplesolid2::application;
        namespace part = simplesolid2::part;
        namespace core = simplesolid2::core;
        app::DocumentSession session{
            {}, part::PartDocument::create(core::DocumentId::generate())};
        const auto made = session.execute(
            app::CreatePartSketchCommand{core::BuiltinReferenceRole::xy_plane});
        CHECK(made.ok() && made.sketch_id);
        const auto rectangle = session.execute(
            app::AddSketchRectangleCommand{
                *made.sketch_id, session.document().revision(),
                {0.0, 0.0}, {10.0, 8.0}, sk::EntityRole::regular, false});
        CHECK(rectangle.ok() && rectangle.entity_ids.size() == 4U);
        const auto* sketchBefore = session.document().findSketch(*made.sketch_id);
        CHECK(sketchBefore != nullptr);
        const auto regions = sk::analyzeRegions(sketchBefore->model);
        CHECK(regions.complete() && regions.regions.size() == 1U);
        const auto intent = part::makeProfileRegionIntent(regions.regions.front());
        CHECK(intent.has_value());
        const auto profile = session.execute(
            app::CreateProfileCommand{
                *made.sketch_id, session.document().revision(), *intent});
        CHECK(profile.ok() && profile.profile_id);
        CHECK(session.document().evaluateProfile(*profile.profile_id)->valid());
        CHECK(part::resolveKernelProfileInput(
            session.document(), *profile.profile_id).ok());

        const auto linked = session.execute(
            app::AddHorizontalSketchConstraintCommand{
                *made.sketch_id, session.document().revision(),
                rectangle.entity_ids.front()});
        CHECK(linked.ok() && linked.changed && linked.constraint_id);
        CHECK(!session.document().evaluateProfile(*profile.profile_id).has_value());
        const auto unresolved = part::resolveKernelProfileInput(
            session.document(), *profile.profile_id);
        CHECK(!unresolved.ok());
        CHECK(unresolved.status == part::ProfileKernelInputStatus::unresolved_profile);
        CHECK(session.document().findProfile(*profile.profile_id) != nullptr);

        // RED A1.3: a REAL Part-owned same-stage solver gateway must let
        // the EXISTING region-intent/Kernel Profile resolver consume its
        // accepted, derived geometry; no second Profile implementation.
        struct EmptyProjectionQuery final : simplesolid2::kernel::IEdgeProjectionQuery {
            std::optional<simplesolid2::kernel::ScopedProjectionEdge>
            bindEdgeToBody(
                simplesolid2::kernel::RuntimeSolidHandle,
                simplesolid2::kernel::RuntimeEdgeToken) noexcept override {
                return std::nullopt;
            }
            simplesolid2::kernel::EdgeProjectionResult
            projectEdgeToPlane(
                simplesolid2::kernel::RuntimeSolidHandle,
                const simplesolid2::kernel::ScopedProjectionEdge&,
                const simplesolid2::kernel::Frame3&) noexcept override {
                return {};
            }
        } query;
        struct ProfileSolver final : sk::ILocalSketchSolver {
            unsigned calls{};
            bool fail{false};
            double translation_u{};
            std::optional<app::DocumentSession*> edit_session;
            std::optional<sk::SketchId> edit_sketch;
            sk::LocalBackendSolveResult solve(
                const sk::LocalSolveProblem& problem) override {
                ++calls;
                if (edit_session && edit_sketch) {
                    const auto edit = (*edit_session)->execute(
                        app::AddSketchLineCommand{
                            *edit_sketch, {20.0, 20.0}, {21.0, 21.0}});
                    CHECK(edit.ok() && edit.changed);
                }
                if (fail) {
                    return {sk::LocalBackendStatus::failed,
                        std::nullopt, {}, {}, std::nullopt};
                }
                // Scripted native test double: deliberately return a
                // different, still-valid solved rectangle so that the
                // Kernel Profile proves it consumes DERIVED coordinates.
                auto geometry = problem.authored;
                for (auto& line : geometry.lines) {
                    line.start.u += translation_u;
                    line.end.u += translation_u;
                }
                return {sk::LocalBackendStatus::converged,
                    std::move(geometry), {}, {}, 1U};
            }
        } backend;
        const sk::LocalSolverAcceptancePolicy accurate{1.0e-12, 1.0e-12};
        part::PartEvaluation prefix;
        prefix.source_document_id = session.document().documentId();
        prefix.source_revision = session.document().revision();
        prefix.source_incarnation = session.document().incarnationToken();
        const auto proved = part::resolveStageSolvedProfileKernelInput(
            session.document(), *profile.profile_id, prefix, query,
            accurate, backend);
        CHECK(proved.ok());
        CHECK(proved.input->outer.boundary.size() == 4U);
        CHECK(proved.input->holes.empty());
        CHECK(backend.calls == 1U);

        // Same durable ProfileIntent, but solver-produced coordinates
        // differ from the authored seed by 2 mm. This is the existing
        // Profile/Kernel adapter, not a second region resolver.
        backend.translation_u = 2.0;
        const auto shifted = part::resolveStageSolvedProfileKernelInput(
            session.document(), *profile.profile_id, prefix, query,
            accurate, backend);
        CHECK(shifted.ok());
        CHECK(backend.calls == 2U);
        CHECK(shifted.input != proved.input);
        CHECK(shifted.input->outer.boundary.size() ==
            proved.input->outer.boundary.size());
        for (std::size_t i = 0; i < proved.input->outer.boundary.size(); ++i) {
            const auto& before = std::get<simplesolid2::kernel::Line2>(
                proved.input->outer.boundary[i].curve);
            const auto& after = std::get<simplesolid2::kernel::Line2>(
                shifted.input->outer.boundary[i].curve);
            CHECK(after.start.u == before.start.u + 2.0);
            CHECK(after.end.u == before.end.u + 2.0);
            CHECK(after.start.v == before.start.v);
            CHECK(after.end.v == before.end.v);
        }
        backend.translation_u = 0.0;
        CHECK(!session.document().evaluateProfile(*profile.profile_id));
        // Existing untrusted entrypoint remains strictly fenced even if a
        // caller passes its own arbitrary model that looks solved.
        const auto untrusted = sk::SketchModel::restore(
            session.document().findSketch(*made.sketch_id)->model.state());
        CHECK(untrusted);
        CHECK(part::resolveKernelProfileInput(
            session.document(), *profile.profile_id,
            nullptr, nullptr, &*untrusted).status ==
            part::ProfileKernelInputStatus::unresolved_profile);
        auto foreign = prefix;
        foreign.source_document_id = core::DocumentId::generate();
        CHECK(!part::resolveStageSolvedProfileKernelInput(
            session.document(), *profile.profile_id, foreign, query,
            accurate, backend).ok());
        CHECK(backend.calls == 2U);
        backend.fail = true;
        CHECK(!part::resolveStageSolvedProfileKernelInput(
            session.document(), *profile.profile_id, prefix, query,
            accurate, backend).ok());
        CHECK(backend.calls == 3U);
        backend.fail = false;
        backend.edit_session = &session;
        backend.edit_sketch = *made.sketch_id;
        CHECK(!part::resolveStageSolvedProfileKernelInput(
            session.document(), *profile.profile_id, prefix, query,
            accurate, backend).ok());
        CHECK(backend.calls == 4U);
        CHECK(prefix.source_revision != session.document().revision());
        CHECK(!part::resolveStageSolvedProfileKernelInput(
            session.document(), *profile.profile_id, prefix, query,
            accurate, backend).ok());
        CHECK(backend.calls == 4U);
        // First undo only the reentrant backend test edit; the next Undo
        // must still target the original Horizontal relation.
        CHECK(session.undo().changed);

        // Undo revives the previously valid Profile; Redo blocks it again
        // without mutating the authored Profile or Sketch line coordinates.
        CHECK(session.undo().changed);
        CHECK(session.document().evaluateProfile(*profile.profile_id)->valid());
        CHECK(part::resolveKernelProfileInput(
            session.document(), *profile.profile_id).ok());
        CHECK(session.redo().changed);
        CHECK(!session.document().evaluateProfile(*profile.profile_id).has_value());
        CHECK(part::resolveKernelProfileInput(
            session.document(), *profile.profile_id).status ==
            part::ProfileKernelInputStatus::unresolved_profile);
    }


    // R13-A1: the production Part -> accepted numerical candidate ->
    // EXISTING ProfileRegionIntent/Kernel bridge must close an explicit
    // Coincident at 1e-12 precision BEFORE region and wire conversion.
    // This test uses a scripted backend, not a native PlaneGCS solver.
    {
        namespace app = simplesolid2::application;
        namespace part = simplesolid2::part;
        namespace core = simplesolid2::core;
        namespace kernel = simplesolid2::kernel;
        struct NoProjection final : kernel::IEdgeProjectionQuery {
            std::optional<kernel::ScopedProjectionEdge> bindEdgeToBody(
                kernel::RuntimeSolidHandle,
                kernel::RuntimeEdgeToken) noexcept override {
                return std::nullopt;
            }
            kernel::EdgeProjectionResult projectEdgeToPlane(
                kernel::RuntimeSolidHandle,
                const kernel::ScopedProjectionEdge&,
                const kernel::Frame3&) noexcept override {
                return {};
            }
        } projection;
        struct SeamBackend final : sk::ILocalSketchSolver {
            double offset{};
            unsigned calls{};
            sk::LocalBackendSolveResult solve(
                const sk::LocalSolveProblem& problem) override {
                ++calls;
                auto candidate = problem.authored;
                CHECK(candidate.lines.size() == 4U);
                candidate.lines[1].start.u += offset;
                return {sk::LocalBackendStatus::converged,
                        std::move(candidate), {}, {}, 3U};
            }
        } backend;
        app::DocumentSession session{
            {}, part::PartDocument::create(core::DocumentId::generate())};
        const auto made = session.execute(
            app::CreatePartSketchCommand{core::BuiltinReferenceRole::xy_plane});
        CHECK(made.ok() && made.sketch_id);
        const auto rectangle = session.execute(
            app::AddSketchRectangleCommand{
                *made.sketch_id, session.document().revision(),
                {0.0, 0.0}, {10.0, 8.0},
                sk::EntityRole::regular, false});
        CHECK(rectangle.ok() && rectangle.entity_ids.size() == 4U);
        const auto before = session.document()
            .findSketch(*made.sketch_id)->model.state();
        CHECK(before.lines.size() == 4U);
        CHECK(before.lines[0].end == before.lines[1].start);
        const auto regions = sk::analyzeRegions(
            session.document().findSketch(*made.sketch_id)->model);
        CHECK(regions.complete() && regions.regions.size() == 1U);
        const auto intent = part::makeProfileRegionIntent(
            regions.regions.front());
        CHECK(intent);
        const auto profile = session.execute(
            app::CreateProfileCommand{
                *made.sketch_id, session.document().revision(), *intent});
        CHECK(profile.ok() && profile.profile_id);
        const auto relation = session.execute(app::AddSketchRelationCommand{
            *made.sketch_id, session.document().revision(),
            sk::ConstraintKind::coincident,
            {before.lines[0].id, sk::ConstraintElement::line_end},
            {before.lines[1].id, sk::ConstraintElement::line_start}});
        CHECK(relation.ok() && relation.constraint_id);
        CHECK(!session.document().evaluateProfile(*profile.profile_id));
        CHECK(part::resolveKernelProfileInput(
            session.document(), *profile.profile_id).status ==
            part::ProfileKernelInputStatus::unresolved_profile);

        const sk::LocalSolverAcceptancePolicy precise{1.0e-12, 1.0e-12};
        part::PartEvaluation prefix;
        prefix.source_document_id = session.document().documentId();
        prefix.source_revision = session.document().revision();
        prefix.source_incarnation = session.document().incarnationToken();
        backend.offset = 5.0e-13;
        const auto stage = part::evaluateStageLocalSolve(
            session.document(), *made.sketch_id, prefix,
            projection, precise, backend);
        CHECK(stage);
        CHECK(stage->solution.status ==
            sk::LocalSolveGatewayStatus::candidate_accepted);
        CHECK(stage->solution.candidate);
        CHECK(stage->solution.candidate->lines[0].end ==
            stage->solution.candidate->lines[1].start);
        CHECK(stage->solution.candidate->lines[1].start ==
            before.lines[1].start);
        const auto acceptedProfile =
            part::resolveStageSolvedProfileKernelInput(
                session.document(), *profile.profile_id, prefix,
                projection, precise, backend);
        CHECK(acceptedProfile.ok());
        CHECK(acceptedProfile.input->outer.boundary.size() == 4U);
        CHECK(acceptedProfile.input->holes.empty());
        // The existing Kernel adapter must carry the EXACT same canonical
        // shared point, not reintroduce an epsilon-sized seam in the OCCT
        // wire's source curves. Use semantic provenance, not loop order.
        const kernel::Line2* first_curve = nullptr;
        const kernel::Line2* second_curve = nullptr;
        for (const auto& use : acceptedProfile.input->outer.boundary) {
            if (use.provenance.source_entity ==
                before.lines[0].id.serialized()) {
                first_curve = std::get_if<kernel::Line2>(&use.curve);
            }
            if (use.provenance.source_entity ==
                before.lines[1].id.serialized()) {
                second_curve = std::get_if<kernel::Line2>(&use.curve);
            }
        }
        CHECK(first_curve != nullptr && second_curve != nullptr);
        CHECK(first_curve->end == second_curve->start);
        CHECK(first_curve->end.u == before.lines[0].end.u);
        CHECK(first_curve->end.v == before.lines[0].end.v);
        CHECK(backend.calls == 2U);
        // The observed Profile remains authored, with stable identity.
        CHECK(session.document().findProfile(*profile.profile_id));
        CHECK(session.document().findSketch(*made.sketch_id)->model
            .state().lines == before.lines);

        // Out-of-policy local Coincident gaps must never be promoted into
        // a repaired Profile via the new staged wrapper.
        backend.offset = 5.0e-12;
        const auto refused = part::resolveStageSolvedProfileKernelInput(
            session.document(), *profile.profile_id, prefix,
            projection, precise, backend);
        CHECK(!refused.ok());
        CHECK(refused.status ==
            part::ProfileKernelInputStatus::unresolved_profile);
        CHECK(backend.calls == 3U);
        CHECK(session.document().findProfile(*profile.profile_id));
    }

    // R13-A1: explicit five-relation authored semantics (not numeric solve).
    {
        sk::SketchModel relations;
        const auto a = relations.addLine({0.0, 0.0}, {2.0, 3.0});
        const auto b = relations.addLine({4.0, 5.0}, {6.0, 7.0});
        const auto c = relations.addLine({1.0, 1.0}, {4.0, 4.0});
        using K = sk::ConstraintKind;
        using E = sk::ConstraintElement;
        const sk::ConstraintReference la{a, E::line};
        const sk::ConstraintReference lb{b, E::line};
        const sk::ConstraintReference lc{c, E::line};
        const sk::ConstraintReference as{a, E::line_start};
        const sk::ConstraintReference ae{a, E::line_end};
        const sk::ConstraintReference bs{b, E::line_start};
        const auto horizontal = relations.addConstraint(K::horizontal, la);
        const auto vertical = relations.addConstraint(K::vertical, lc);
        const auto coincident = relations.addConstraint(K::coincident, ae, bs);
        const auto parallel = relations.addConstraint(K::parallel, la, lb);
        const auto perpendicular = relations.addConstraint(K::perpendicular, lb, lc);
        CHECK(horizontal && vertical && coincident && parallel && perpendicular);
        CHECK(horizontal->serialized() == "1");
        CHECK(vertical->serialized() == "2");
        CHECK(coincident->serialized() == "3");
        CHECK(parallel->serialized() == "4");
        CHECK(perpendicular->serialized() == "5");
        const auto beforeInvalid = relations.state();
        CHECK(!relations.addConstraint(K::horizontal, la));
        CHECK(!relations.addConstraint(K::vertical, lc)); // duplicate
        CHECK(!relations.addConstraint(K::vertical, la)); // conflicts with Horizontal(a)
        CHECK(!relations.addConstraint(K::coincident, bs, ae));
        CHECK(!relations.addConstraint(K::parallel, lb, la));
        CHECK(!relations.addConstraint(K::perpendicular, lc, lb));
        CHECK(!relations.addConstraint(K::perpendicular, la, lb)); // conflicts with Parallel(a,b)
        CHECK(!relations.addConstraint(K::parallel, lb, lc)); // conflicts with Perpendicular(b,c)
        CHECK(!relations.addConstraint(K::coincident, as, ae)); // same Line
        CHECK(!relations.addConstraint(K::coincident, as, as));
        CHECK(!relations.addConstraint(K::parallel, la, la));
        CHECK(!relations.addConstraint(K::perpendicular, la, la));
        CHECK(!relations.addConstraint(K::vertical, as));
        CHECK(!relations.addConstraint(K::horizontal, la, lb));
        CHECK(!relations.addConstraint(K::coincident, la, bs));
        CHECK(!relations.addConstraint(K::parallel, as, lb));
        CHECK(!relations.addConstraint(K::perpendicular, la, bs));
        CHECK(!relations.addConstraint(static_cast<K>(255), la));
        CHECK(!relations.addConstraint(K::vertical, { *sk::EntityId::parse("9999"), E::line }));
        CHECK(relations.state() == beforeInvalid);
        auto roundTrip = sk::SketchModel::restore(beforeInvalid);
        CHECK(roundTrip && roundTrip->state() == beforeInvalid);
        const sk::Point2 originalStart{0.0, 0.0};
        CHECK(relations.findLine(a)->start() == originalStart);
        auto invalidState = beforeInvalid;
        invalidState.constraints.push_back(invalidState.constraints[3]);
        invalidState.constraints.back().id = *sk::ConstraintId::parse("6");
        invalidState.next_constraint_id = *sk::ConstraintIdCursor::parse("7");
        invalidState.constraints.back().first = lb;
        invalidState.constraints.back().second = la;
        CHECK(!sk::SketchModel::restore(invalidState));
        invalidState = beforeInvalid;
        invalidState.constraints[2].first = la;
        CHECK(!sk::SketchModel::restore(invalidState));
        invalidState = beforeInvalid;
        invalidState.constraints[4].second.entity = *sk::EntityId::parse("9999");
        CHECK(!sk::SketchModel::restore(invalidState));
        invalidState = beforeInvalid;
        invalidState.constraints[1].kind = static_cast<K>(255);
        CHECK(!sk::SketchModel::restore(invalidState));
        CHECK(relations.erase(b));
        CHECK(relations.constraintCount() == 2U); // Horizontal(a), Vertical(a)
        CHECK(relations.findConstraint(*horizontal) != nullptr);
        CHECK(relations.findConstraint(*vertical) != nullptr);
        CHECK(relations.findConstraint(*coincident) == nullptr);
        CHECK(relations.findConstraint(*parallel) == nullptr);
        CHECK(relations.findConstraint(*perpendicular) == nullptr);
        CHECK(relations.constraintIdCursor().serialized() == "6");
        const auto newParallel = relations.addConstraint(K::parallel, la, lc);
        CHECK(newParallel && newParallel->serialized() == "6");
        CHECK(relations.state().constraints.size() == 3U);
        // Current solved geometry is not published for any of these intents.
    }

    // All five manual relation kinds are wired to the real revision-bound
    // DocumentSession transaction path and undoable as one command each.
    {
        namespace app = simplesolid2::application;
        namespace part = simplesolid2::part;
        namespace core = simplesolid2::core;
        using K = sk::ConstraintKind;
        using E = sk::ConstraintElement;
        app::DocumentSession session{
            {}, part::PartDocument::create(core::DocumentId::generate())};
        const auto made = session.execute(
            app::CreatePartSketchCommand{core::BuiltinReferenceRole::xy_plane});
        CHECK(made.ok() && made.sketch_id);
        const auto first = session.execute(
            app::AddSketchLineCommand{*made.sketch_id, {1.0, 2.0}, {5.0, 6.0}});
        const auto second = session.execute(
            app::AddSketchLineCommand{*made.sketch_id, {6.0, 1.0}, {8.0, 2.0}});
        const auto third = session.execute(
            app::AddSketchLineCommand{*made.sketch_id, {10.0, 2.0}, {12.0, 4.0}});
        CHECK(first.ok() && first.entity_id && second.ok() && second.entity_id);
        CHECK(third.ok() && third.entity_id);
        const sk::ConstraintReference firstLine{*first.entity_id, E::line};
        const sk::ConstraintReference secondLine{*second.entity_id, E::line};
        const sk::ConstraintReference thirdLine{*third.entity_id, E::line};
        const sk::ConstraintReference firstEnd{*first.entity_id, E::line_end};
        const sk::ConstraintReference secondStart{*second.entity_id, E::line_start};
        const auto oldRevision = session.document().revision();
        const auto add = [&](K kind, sk::ConstraintReference lhs,
                             sk::ConstraintReference rhs = {}) {
            return session.execute(app::AddSketchRelationCommand{
                *made.sketch_id, session.document().revision(), kind, lhs, rhs});
        };
        CHECK(add(K::horizontal, firstLine).changed);
        CHECK(add(K::vertical, secondLine).changed);
        const auto coincident = add(K::coincident, firstEnd, secondStart);
        CHECK(coincident.changed && coincident.constraint_id);
        CHECK(add(K::parallel, firstLine, secondLine).changed);
        CHECK(add(K::perpendicular, secondLine, thirdLine).changed);
        const auto* hosted = session.document().findSketch(*made.sketch_id);
        CHECK(hosted && hosted->model.constraintCount() == 5U);
        const auto undoDepth = session.undoDepth();
        const auto duplicate = add(K::coincident, secondStart, firstEnd);
        CHECK(!duplicate.changed && !duplicate.ok());
        CHECK(session.undoDepth() == undoDepth);
        CHECK(!add(K::vertical, firstLine).ok()); // conflicts with Horizontal
        CHECK(!add(K::perpendicular, firstLine, secondLine).ok()); // conflicts with Parallel
        CHECK(session.undoDepth() == undoDepth);
        const auto stale = session.execute(app::AddSketchRelationCommand{
            *made.sketch_id, oldRevision,
            K::parallel, firstLine, secondLine});
        CHECK(!stale.ok());
        CHECK(stale.diagnostic.code == app::DocumentSessionErrorCode::revision_diverged);
        CHECK(session.undo().changed);
        CHECK(session.document().findSketch(*made.sketch_id)->model.constraintCount() == 4U);
        CHECK(session.redo().changed);
        CHECK(session.document().findSketch(*made.sketch_id)->model.constraintCount() == 5U);
        const auto deleted = session.execute(app::EraseSketchConstraintCommand{
            *made.sketch_id, session.document().revision(), *coincident.constraint_id});
        CHECK(deleted.ok() && deleted.changed);
        CHECK(session.document().findSketch(*made.sketch_id)->model.constraintCount() == 4U);
        CHECK(session.undo().changed);
        CHECK(session.document().findSketch(*made.sketch_id)->model.constraintCount() == 5U);
    }


    // R13-A1.2 RED: donor-aligned, pure numeric acceptance bridge.
    // It checks candidate geometry; it does not solve equations or publish Part.
    {
        using K = sk::ConstraintKind;
        using E = sk::ConstraintElement;
        sk::SketchModel authored;
        const auto a = authored.addLine({0.0, 0.0}, {4.0, 0.0});
        const auto b = authored.addLine({4.0, 0.0}, {4.0, 3.0});
        const auto c = authored.addLine({1.0, 2.0}, {3.0, 2.0});
        CHECK(authored.addConstraint(K::horizontal, {a, E::line}));
        CHECK(authored.addConstraint(K::vertical, {b, E::line}));
        CHECK(authored.addConstraint(K::coincident,
            {a, E::line_end}, {b, E::line_start}));
        CHECK(authored.addConstraint(K::parallel,
            {a, E::line}, {c, E::line}));
        CHECK(authored.addConstraint(K::perpendicular,
            {b, E::line}, {c, E::line}));

        const std::array<sk::EntityId, 1> fixed{b};
        const auto authoredState = authored.state();
        const auto problem = sk::prepareLocalSolveProblem(authoredState, fixed);
        CHECK(problem.has_value());
        CHECK(problem->authored == authoredState);
        CHECK(problem->lines.size() == 3U);
        CHECK(problem->constraints.size() == 5U);
        CHECK(problem->fixed_lines.size() == 1U);
        CHECK(problem->fixed_lines.front() == b);
        CHECK(problem->lines[1].fixed);
        CHECK(!problem->lines[0].fixed);
        CHECK(!problem->lines[2].fixed);
        const sk::LocalSolverAcceptancePolicy policy{1.0e-7, 1.0e-6};
        CHECK(policy.valid());
        const auto good = sk::reviewLocalSolveCandidate(
            *problem, authoredState, policy);
        CHECK(good.status == sk::LocalResidualStatus::residuals_satisfied);
        CHECK(good.constraints.size() == 5U);
        for (const auto& result : good.constraints) {
            CHECK(result.constraint_id.valid());
            CHECK(result.accepted);
            CHECK(result.residual == 0.0);
        }
        CHECK(good.constraints[0].unit == sk::LocalResidualUnit::model_distance);
        CHECK(good.constraints[1].unit == sk::LocalResidualUnit::model_distance);
        CHECK(good.constraints[2].unit == sk::LocalResidualUnit::model_distance);
        CHECK(good.constraints[3].unit == sk::LocalResidualUnit::radians);
        CHECK(good.constraints[4].unit == sk::LocalResidualUnit::radians);

        auto badGeometry = authoredState;
        badGeometry.lines[0].end.v = 0.1; // horizontal, coincident, parallel fail
        const auto bad = sk::reviewLocalSolveCandidate(
            *problem, badGeometry, policy);
        CHECK(bad.status == sk::LocalResidualStatus::residual_violation);
        CHECK(!bad.constraints[0].accepted);
        CHECK(!bad.constraints[2].accepted);
        CHECK(!bad.constraints[3].accepted);
        CHECK(authored.state() == authoredState); // no authored mutation

        auto fixedChanged = authoredState;
        fixedChanged.lines[1].start.u += 1.0e-8;
        CHECK(sk::reviewLocalSolveCandidate(
            *problem, fixedChanged, policy).status ==
            sk::LocalResidualStatus::fixed_entity_changed);

        auto corrupt = authoredState;
        corrupt.constraints.clear();
        CHECK(sk::reviewLocalSolveCandidate(
            *problem, corrupt, policy).status ==
            sk::LocalResidualStatus::stale_semantics);
        corrupt = authoredState;
        corrupt.lines[0].role = sk::EntityRole::construction;
        CHECK(sk::reviewLocalSolveCandidate(
            *problem, corrupt, policy).status ==
            sk::LocalResidualStatus::stale_semantics);
        corrupt = authoredState;
        corrupt.lines[0].end.u = std::numeric_limits<double>::infinity();
        CHECK(sk::reviewLocalSolveCandidate(
            *problem, corrupt, policy).status ==
            sk::LocalResidualStatus::invalid_candidate);

        CHECK((!sk::LocalSolverAcceptancePolicy{0.0, 1.0e-6}.valid()));
        CHECK((!sk::LocalSolverAcceptancePolicy{1.0e-7, 0.0}.valid()));
        CHECK((!sk::LocalSolverAcceptancePolicy{
            std::numeric_limits<double>::infinity(), 1.0e-6}.valid()));
        CHECK(sk::reviewLocalSolveCandidate(
            *problem, authoredState,
            sk::LocalSolverAcceptancePolicy{0.0, 1.0e-6}).status ==
            sk::LocalResidualStatus::invalid_policy);

        const std::array<sk::EntityId, 1> missing{
            *sk::EntityId::parse("9999")};
        const std::array<sk::EntityId, 2> repeated{b, b};
        CHECK(!sk::prepareLocalSolveProblem(authoredState, missing));
        CHECK(!sk::prepareLocalSolveProblem(authoredState, repeated));
        CHECK(!sk::prepareLocalSolveProblem(
            sk::SketchModelState{}, fixed));
    }

    // SS1-style backend gateway RED: do not mistake a backend's declared
    // convergence for a trustworthy solved Sketch or Part authority.
    // Scripted backend tests the boundary; it is NOT a production solver.
    {
        struct ScriptedBackend final : sk::ILocalSketchSolver {
            sk::LocalBackendStatus status{sk::LocalBackendStatus::converged};
            std::optional<sk::SketchModelState> candidate;
            std::vector<sk::ConstraintId> conflicts;
            std::vector<sk::LocalBackendDiagnostic> diagnostics;
            std::optional<std::size_t> degrees_of_freedom;
            bool throw_on_solve{false};
            unsigned calls{};

            sk::LocalBackendSolveResult solve(
                const sk::LocalSolveProblem&) override {
                ++calls;
                if (throw_on_solve) {
                    throw std::runtime_error{"synthetic backend failure"};
                }
                return {status, candidate, conflicts, diagnostics, degrees_of_freedom};
            }
        };

        sk::SketchModel seed;
        const auto line = seed.addLine({0.0, 0.0}, {10.0, 0.0});
        const auto relation = seed.addHorizontal(line);
        CHECK(relation);
        const auto state = seed.state();
        const auto input = sk::prepareLocalSolveProblem(state);
        CHECK(input);
        const sk::LocalSolverAcceptancePolicy policy{1.0e-8, 1.0e-6};
        ScriptedBackend backend;
        backend.candidate = state;
        backend.degrees_of_freedom = 3U; // underconstrained is legal
        const auto accepted = sk::evaluateLocalSolve(*input, policy, backend);
        CHECK(accepted.status == sk::LocalSolveGatewayStatus::candidate_accepted);
        CHECK(accepted.candidate && *accepted.candidate == state);
        CHECK(accepted.residuals.status == sk::LocalResidualStatus::residuals_satisfied);
        CHECK(accepted.degrees_of_freedom == 3U);
        CHECK(accepted.diagnostics.empty());
        CHECK(backend.calls == 1U);
        CHECK(seed.state() == state);

        backend.candidate->lines.front().end.v = 0.25;
        const auto residualRejected = sk::evaluateLocalSolve(*input, policy, backend);
        CHECK(residualRejected.status == sk::LocalSolveGatewayStatus::residual_violation);
        CHECK(!residualRejected.candidate);
        CHECK(residualRejected.residuals.status == sk::LocalResidualStatus::residual_violation);
        CHECK(!residualRejected.diagnostics.empty());
        CHECK(residualRejected.diagnostics.front().kind ==
            sk::LocalBackendDiagnosticKind::residual_violation);
        CHECK(residualRejected.diagnostics.front().constraint == relation);

        // The original PlaneGCS 0.8.0 fully fixed diagonal false-success
        // witness: raw native Success and DoF=0 can be returned for a
        // 0,0 -> 5,2 Line with a Horizontal equation, even though its
        // 2 mm residual is impossible to eliminate without moving the
        // caller's projected source. The SS2 gateway, not the backend
        // status, must reject that disposable candidate.
        sk::SketchModel sourceAnchored;
        const auto diagonal = sourceAnchored.addLine(
            {0.0, 0.0}, {5.0, 2.0});
        const auto diagonalHorizontal = sourceAnchored.addHorizontal(diagonal);
        CHECK(diagonalHorizontal);
        const auto diagonalAuthored = sourceAnchored.state();
        const std::array<sk::EntityId, 1> projectedDiagonal{diagonal};
        const auto impossibleInput = sk::prepareLocalSolveProblem(
            diagonalAuthored, projectedDiagonal);
        CHECK(impossibleInput);
        CHECK(impossibleInput->fixed_lines.size() == 1U);
        CHECK(impossibleInput->lines.front().fixed);
        ScriptedBackend upstreamFalseSuccess;
        upstreamFalseSuccess.candidate = diagonalAuthored;
        upstreamFalseSuccess.degrees_of_freedom = 0U;
        const auto falselyConverged = sk::evaluateLocalSolve(
            *impossibleInput, policy, upstreamFalseSuccess);
        CHECK(upstreamFalseSuccess.calls == 1U);
        CHECK(falselyConverged.status ==
            sk::LocalSolveGatewayStatus::residual_violation);
        CHECK(!falselyConverged.candidate);
        CHECK(falselyConverged.residuals.status ==
            sk::LocalResidualStatus::residual_violation);
        CHECK(falselyConverged.residuals.constraints.size() == 1U);
        CHECK(falselyConverged.residuals.constraints.front().constraint_id ==
            diagonalHorizontal);
        CHECK(falselyConverged.residuals.constraints.front().unit ==
            sk::LocalResidualUnit::model_distance);
        CHECK(falselyConverged.residuals.constraints.front().residual == 2.0);
        CHECK(!falselyConverged.residuals.constraints.front().accepted);
        CHECK(falselyConverged.diagnostics.size() == 1U);
        CHECK(falselyConverged.diagnostics.front().kind ==
            sk::LocalBackendDiagnosticKind::residual_violation);
        CHECK(falselyConverged.diagnostics.front().constraint ==
            diagonalHorizontal);
        CHECK(sourceAnchored.state() == diagonalAuthored);

        // A false-success backend may attempt to satisfy the Horizontal
        // equation by moving the projected, immutable PG-01 source.
        // Residual 0 is NOT sufficient: the fixed anchor must win and
        // the gateway must reject the purportedly converged candidate.
        upstreamFalseSuccess.candidate = diagonalAuthored;
        upstreamFalseSuccess.candidate->lines.front().end.v = 0.0;
        CHECK(upstreamFalseSuccess.candidate->lines.front().start.v ==
              upstreamFalseSuccess.candidate->lines.front().end.v);
        const auto movedProjectedSource = sk::evaluateLocalSolve(
            *impossibleInput, policy, upstreamFalseSuccess);
        CHECK(upstreamFalseSuccess.calls == 2U);
        CHECK(movedProjectedSource.status ==
            sk::LocalSolveGatewayStatus::fixed_entity_changed);
        CHECK(movedProjectedSource.residuals.status ==
            sk::LocalResidualStatus::fixed_entity_changed);
        CHECK(!movedProjectedSource.candidate);
        CHECK(sourceAnchored.state() == diagonalAuthored);

        // Upstream PlaneGCS can also report numeric Success for
        // Horizontal+Vertical while collapsing the same Line to a point.
        // Such an authored H+V pair is rejected earlier by SketchModel;
        // nevertheless a single valid Horizontal problem must be robust
        // against a backend that proposes a collapsed (zero-length) Line
        // whose equation residual misleadingly equals exactly zero.
        sk::SketchModel collapseModel;
        const auto collapsible = collapseModel.addLine(
            {0.0, 0.0}, {5.0, 2.0});
        const auto collapsedHorizontal = collapseModel.addHorizontal(
            collapsible);
        CHECK(collapsedHorizontal);
        const auto collapseAuthored = collapseModel.state();
        const auto collapseInput = sk::prepareLocalSolveProblem(
            collapseAuthored);
        CHECK(collapseInput);
        ScriptedBackend collapsingNative;
        collapsingNative.candidate = collapseAuthored;
        collapsingNative.candidate->lines.front().end =
            collapsingNative.candidate->lines.front().start;
        collapsingNative.degrees_of_freedom = 0U;
        const auto collapsedReview = sk::reviewLocalSolveCandidate(
            *collapseInput, *collapsingNative.candidate, policy);
        CHECK(collapsedReview.status ==
            sk::LocalResidualStatus::invalid_candidate);
        const auto collapsedSolve = sk::evaluateLocalSolve(
            *collapseInput, policy, collapsingNative);
        CHECK(collapsingNative.calls == 1U);
        CHECK(collapsedSolve.status ==
            sk::LocalSolveGatewayStatus::invalid_candidate);
        CHECK(collapsedSolve.residuals.status ==
            sk::LocalResidualStatus::invalid_candidate);
        CHECK(!collapsedSolve.candidate);
        CHECK(collapseModel.state() == collapseAuthored);

        // Auto-registered SS2 rectangle regression: four lines, four
        // Coincident seams, right/left Vertical and top Horizontal.
        // The bottom is a fixed projected source, never a solver unknown.
        // This tests real SS2 model/gateway/CTest behavior with a scripted
        // candidate; it is NOT a real PlaneGCS native backend test.
        sk::SketchModel rectangle;
        const auto bottom = rectangle.addLine(
            {0.0, 0.0}, {4.0, 0.0});
        const auto right = rectangle.addLine(
            {4.2, 0.1}, {4.1, 3.1});
        const auto top = rectangle.addLine(
            {4.3, 3.0}, {-0.2, 3.2});
        const auto left = rectangle.addLine(
            {0.1, 3.1}, {0.1, 0.2});
        using Ref = sk::ConstraintElement;
        CHECK(rectangle.addConstraint(sk::ConstraintKind::coincident,
            {bottom, Ref::line_end}, {right, Ref::line_start}));
        const auto rightTop = rectangle.addConstraint(
            sk::ConstraintKind::coincident,
            {right, Ref::line_end}, {top, Ref::line_start});
        CHECK(rightTop);
        CHECK(rectangle.addConstraint(sk::ConstraintKind::coincident,
            {top, Ref::line_end}, {left, Ref::line_start}));
        CHECK(rectangle.addConstraint(sk::ConstraintKind::coincident,
            {left, Ref::line_end}, {bottom, Ref::line_start}));
        CHECK(rectangle.addConstraint(sk::ConstraintKind::vertical,
            {right, Ref::line}));
        CHECK(rectangle.addConstraint(sk::ConstraintKind::horizontal,
            {top, Ref::line}));
        CHECK(rectangle.addConstraint(sk::ConstraintKind::vertical,
            {left, Ref::line}));
        const auto rectangleAuthored = rectangle.state();
        const std::array<sk::EntityId, 1> rectangleFixed{bottom};
        const auto rectangleInput = sk::prepareLocalSolveProblem(
            rectangleAuthored, rectangleFixed);
        CHECK(rectangleInput);
        CHECK(rectangleInput->constraints.size() == 7U);
        CHECK(rectangleInput->equations.size() == 7U);
        CHECK(rectangleInput->fixed_lines == std::vector<sk::EntityId>{bottom});
        CHECK(rectangleInput->lines.size() == 4U);
        CHECK(rectangleInput->lines.front().fixed);
        for (std::size_t i = 1; i < rectangleInput->lines.size(); ++i) {
            CHECK(!rectangleInput->lines[i].fixed);
        }

        auto rectangleCandidate = rectangleAuthored;
        rectangleCandidate.lines[1].start = {4.0, 0.0};
        rectangleCandidate.lines[1].end = {4.0, 3.0};
        rectangleCandidate.lines[2].start = {4.0, 3.0};
        rectangleCandidate.lines[2].end = {0.0, 3.0};
        rectangleCandidate.lines[3].start = {0.0, 3.0};
        rectangleCandidate.lines[3].end = {0.0, 0.0};
        ScriptedBackend rectangleBackend;
        rectangleBackend.candidate = rectangleCandidate;
        rectangleBackend.degrees_of_freedom = 1U;
        const auto rectangleGateway = sk::evaluateLocalSolve(
            *rectangleInput, policy, rectangleBackend);
        CHECK(rectangleBackend.calls == 1U);
        CHECK(rectangleGateway.status ==
            sk::LocalSolveGatewayStatus::candidate_accepted);
        CHECK(rectangleGateway.candidate);
        CHECK(rectangleGateway.degrees_of_freedom == 1U);
        CHECK(rectangleGateway.residuals.status ==
            sk::LocalResidualStatus::residuals_satisfied);
        CHECK(rectangleGateway.residuals.constraints.size() == 7U);
        for (const auto& residual : rectangleGateway.residuals.constraints) {
            CHECK(residual.accepted);
        }
        const auto& rectangleLines = rectangleGateway.candidate->lines;
        CHECK(rectangleLines.size() == 4U);
        CHECK(rectangleLines[0] == rectangleAuthored.lines[0]);
        CHECK(rectangleLines[0].end == rectangleLines[1].start);
        CHECK(rectangleLines[1].end == rectangleLines[2].start);
        CHECK(rectangleLines[2].end == rectangleLines[3].start);
        CHECK(rectangleLines[3].end == rectangleLines[0].start);
        CHECK(rectangleLines[1].start.u == rectangleLines[1].end.u);
        CHECK(rectangleLines[2].start.v == rectangleLines[2].end.v);
        CHECK(rectangleLines[3].start.u == rectangleLines[3].end.u);
        CHECK(rectangle.state() == rectangleAuthored);

        // A backend's spurious Success must not publish an open seam.
        rectangleBackend.candidate = rectangleCandidate;
        rectangleBackend.candidate->lines[1].end.v += 0.25;
        const auto openRectangle = sk::evaluateLocalSolve(
            *rectangleInput, policy, rectangleBackend);
        CHECK(rectangleBackend.calls == 2U);
        CHECK(openRectangle.status ==
            sk::LocalSolveGatewayStatus::residual_violation);
        CHECK(!openRectangle.candidate);
        CHECK(openRectangle.residuals.constraints.size() == 7U);
        CHECK(!openRectangle.residuals.constraints[1].accepted);
        CHECK(openRectangle.residuals.constraints[1].constraint_id == rightTop);
        CHECK(rectangle.state() == rectangleAuthored);

        backend.candidate = state;
        backend.diagnostics = {{sk::LocalBackendDiagnosticKind::redundant_constraint, relation}};
        const auto redundancy = sk::evaluateLocalSolve(*input, policy, backend);
        CHECK(redundancy.status == sk::LocalSolveGatewayStatus::redundant_constraint);
        CHECK(!redundancy.candidate);
        CHECK(redundancy.diagnostics == backend.diagnostics);
        backend.diagnostics.clear();

        backend.diagnostics = {{sk::LocalBackendDiagnosticKind::conflicting_constraint, relation}};
        CHECK(sk::evaluateLocalSolve(*input, policy, backend).status ==
              sk::LocalSolveGatewayStatus::invalid_backend_response);
        backend.diagnostics.clear();
        backend.diagnostics = {{sk::LocalBackendDiagnosticKind::redundant_constraint,
                               *sk::ConstraintId::parse("9999")}};
        CHECK(sk::evaluateLocalSolve(*input, policy, backend).status ==
              sk::LocalSolveGatewayStatus::invalid_backend_response);
        backend.diagnostics.clear();
        backend.diagnostics = {{static_cast<sk::LocalBackendDiagnosticKind>(255), relation}};
        CHECK(sk::evaluateLocalSolve(*input, policy, backend).status ==
              sk::LocalSolveGatewayStatus::invalid_backend_response);
        backend.diagnostics.clear();
        backend.diagnostics = {
            {sk::LocalBackendDiagnosticKind::redundant_constraint, relation},
            {sk::LocalBackendDiagnosticKind::redundant_constraint, relation}};
        CHECK(sk::evaluateLocalSolve(*input, policy, backend).status ==
              sk::LocalSolveGatewayStatus::invalid_backend_response);
        backend.diagnostics.clear();

        backend.status = sk::LocalBackendStatus::conflict;
        backend.candidate.reset();
        backend.conflicts = {*relation};
        const auto conflict = sk::evaluateLocalSolve(*input, policy, backend);
        CHECK(conflict.status == sk::LocalSolveGatewayStatus::conflict);
        CHECK(!conflict.candidate);
        CHECK(conflict.conflicting_constraints == backend.conflicts);

        // A2.0 RED: a backend may not publish contradictory semantic
        // conflict provenance. The vector consumed by UI highlights and
        // the detailed diagnostics must agree on exact ConstraintIds.
        backend.diagnostics = {{
            sk::LocalBackendDiagnosticKind::conflicting_constraint,
            relation}};
        backend.conflicts.clear();
        const auto diagnostic_without_owner = sk::evaluateLocalSolve(
            *input, policy, backend);
        CHECK(diagnostic_without_owner.status ==
              sk::LocalSolveGatewayStatus::invalid_backend_response);
        CHECK(!diagnostic_without_owner.candidate);
        CHECK(diagnostic_without_owner.conflicting_constraints.empty());
        backend.conflicts = {*relation};
        const auto consistent_conflict = sk::evaluateLocalSolve(
            *input, policy, backend);
        CHECK(consistent_conflict.status ==
              sk::LocalSolveGatewayStatus::conflict);
        CHECK(consistent_conflict.conflicting_constraints ==
              backend.conflicts);
        CHECK(consistent_conflict.diagnostics == backend.diagnostics);
        backend.diagnostics.clear();

        // Cross-linked detail from a different, existing ConstraintId
        // is equally unsafe; checking IDs only for existence is not
        // enough. A partial diagnostic list cannot contradict the
        // conflict-owner inventory.
        sk::SketchModel multi;
        const auto first_line =
            multi.addLine({0.0, 0.0}, {5.0, 0.0});
        const auto second_line =
            multi.addLine({0.0, 2.0}, {5.0, 2.0});
        const auto first_relation = multi.addHorizontal(first_line);
        const auto second_relation = multi.addHorizontal(second_line);
        CHECK(first_relation && second_relation);
        const auto multi_problem =
            sk::prepareLocalSolveProblem(multi.state());
        CHECK(multi_problem);
        backend.conflicts = {*first_relation};
        backend.diagnostics = {{
            sk::LocalBackendDiagnosticKind::conflicting_constraint,
            *second_relation}};
        const auto mismatched_conflict = sk::evaluateLocalSolve(
            *multi_problem, policy, backend);
        CHECK(mismatched_conflict.status ==
              sk::LocalSolveGatewayStatus::invalid_backend_response);
        CHECK(!mismatched_conflict.candidate);
        CHECK(mismatched_conflict.conflicting_constraints.empty());

        // The detailed inventory cannot be an incomplete subset of
        // the actual conflict owners either. Exact complete coverage
        // of two independent IDs remains accepted.
        backend.conflicts = {*first_relation, *second_relation};
        backend.diagnostics = {{
            sk::LocalBackendDiagnosticKind::conflicting_constraint,
            *first_relation}};
        const auto missing_detail = sk::evaluateLocalSolve(
            *multi_problem, policy, backend);
        CHECK(missing_detail.status ==
              sk::LocalSolveGatewayStatus::invalid_backend_response);
        CHECK(!missing_detail.candidate);
        backend.diagnostics.push_back({
            sk::LocalBackendDiagnosticKind::conflicting_constraint,
            *second_relation});
        const auto full_details = sk::evaluateLocalSolve(
            *multi_problem, policy, backend);
        CHECK(full_details.status == sk::LocalSolveGatewayStatus::conflict);
        CHECK(!full_details.candidate);
        CHECK(full_details.conflicting_constraints == backend.conflicts);
        CHECK(full_details.diagnostics == backend.diagnostics);
        backend.diagnostics.clear();

        backend.conflicts = {*sk::ConstraintId::parse("9999")};
        CHECK(sk::evaluateLocalSolve(*input, policy, backend).status ==
              sk::LocalSolveGatewayStatus::invalid_backend_response);
        backend.conflicts.clear();
        backend.candidate = state;
        CHECK(sk::evaluateLocalSolve(*input, policy, backend).status ==
              sk::LocalSolveGatewayStatus::invalid_backend_response);
        backend.candidate.reset();
        backend.status = sk::LocalBackendStatus::unsupported;
        const auto unsupported = sk::evaluateLocalSolve(*input, policy, backend);
        CHECK(unsupported.status == sk::LocalSolveGatewayStatus::unsupported);
        CHECK(!unsupported.candidate);
        backend.candidate = state;
        CHECK(sk::evaluateLocalSolve(*input, policy, backend).status ==
              sk::LocalSolveGatewayStatus::invalid_backend_response);
        backend.candidate.reset();
        backend.status = sk::LocalBackendStatus::failed;
        CHECK(sk::evaluateLocalSolve(*input, policy, backend).status ==
              sk::LocalSolveGatewayStatus::backend_failure);

        backend.status = sk::LocalBackendStatus::converged;
        backend.candidate.reset();
        CHECK(sk::evaluateLocalSolve(*input, policy, backend).status ==
              sk::LocalSolveGatewayStatus::invalid_backend_response);
        backend.candidate = state;
        backend.conflicts = {*relation};
        CHECK(sk::evaluateLocalSolve(*input, policy, backend).status ==
              sk::LocalSolveGatewayStatus::invalid_backend_response);
        backend.conflicts.clear();

        backend.throw_on_solve = true;
        CHECK(sk::evaluateLocalSolve(*input, policy, backend).status ==
              sk::LocalSolveGatewayStatus::backend_failure);
        backend.throw_on_solve = false;
        const auto calls = backend.calls;
        CHECK(sk::evaluateLocalSolve(
            *input, sk::LocalSolverAcceptancePolicy{0.0, 1.0e-6},
            backend).status == sk::LocalSolveGatewayStatus::invalid_policy);
        CHECK(backend.calls == calls);

        auto spoofed = *input;
        spoofed.constraints.clear();
        CHECK(sk::evaluateLocalSolve(spoofed, policy, backend).status ==
              sk::LocalSolveGatewayStatus::invalid_problem);
        CHECK(backend.calls == calls);
        spoofed = *input;
        spoofed.equations.clear();
        CHECK(sk::evaluateLocalSolve(spoofed, policy, backend).status ==
              sk::LocalSolveGatewayStatus::invalid_problem);
        CHECK(backend.calls == calls);

        // Explicit Coincident uses a caller-owned 1e-12 model-distance
        // policy. A numerically converged seam must have one EXACT shared
        // evaluated vertex, not two almost-equal Profile coordinates.
        sk::SketchModel seamModel;
        const auto s1 = seamModel.addLine({0.0, 0.0}, {2.0, 0.0});
        const auto s2 = seamModel.addLine({2.0, 0.0}, {2.0, 4.0});
        const auto seamId = seamModel.addConstraint(
            sk::ConstraintKind::coincident,
            {s1, sk::ConstraintElement::line_end},
            {s2, sk::ConstraintElement::line_start});
        CHECK(seamId);
        const auto seamAuthor = seamModel.state();
        const auto seamProblem = sk::prepareLocalSolveProblem(seamAuthor);
        CHECK(seamProblem);
        const sk::LocalSolverAcceptancePolicy precise{1.0e-12, 1.0e-12};
        ScriptedBackend seamBackend;
        seamBackend.candidate = seamAuthor;
        seamBackend.candidate->lines[1].start.u += 5.0e-13;
        const auto seamNear = sk::evaluateLocalSolve(
            *seamProblem, precise, seamBackend);
        CHECK(seamNear.status ==
            sk::LocalSolveGatewayStatus::candidate_accepted);
        CHECK(seamNear.candidate);
        CHECK(seamNear.candidate->lines[0].end ==
            seamNear.candidate->lines[1].start);
        CHECK(seamNear.candidate->lines[0].end == seamAuthor.lines[0].end);
        CHECK(seamModel.state() == seamAuthor); // authored never welded

        // Real gaps outside the caller-owned precision remain open.
        seamBackend.candidate = seamAuthor;
        seamBackend.candidate->lines[1].start.u += 5.0e-12;
        const auto seamTooWide = sk::evaluateLocalSolve(
            *seamProblem, precise, seamBackend);
        CHECK(seamTooWide.status ==
            sk::LocalSolveGatewayStatus::residual_violation);
        CHECK(!seamTooWide.candidate);

        // An exactly equal disposable candidate remains valid.
        seamBackend.candidate = seamAuthor;
        const auto exactlyClosed = sk::evaluateLocalSolve(
            *seamProblem, precise, seamBackend);
        CHECK(exactlyClosed.status ==
            sk::LocalSolveGatewayStatus::candidate_accepted);
        CHECK(exactlyClosed.candidate);

        // Fixed sources are authoritative, even when the free seed is
        // only numerically Coincident. Never move a Fix/PG-01 anchor.
        sk::SketchModel anchored;
        const auto anchor = anchored.addLine({0.0, 0.0}, {2.0, 0.0});
        const auto free = anchored.addLine({2.0 + 5.0e-13, 0.0}, {2.0, 4.0});
        CHECK(anchored.addConstraint(sk::ConstraintKind::fix,
            {anchor, sk::ConstraintElement::line}));
        CHECK(anchored.addConstraint(sk::ConstraintKind::coincident,
            {anchor, sk::ConstraintElement::line_end},
            {free, sk::ConstraintElement::line_start}));
        const auto anchoredState = anchored.state();
        const auto anchoredInput = sk::prepareLocalSolveProblem(anchoredState);
        CHECK(anchoredInput);
        ScriptedBackend anchoredBackend;
        anchoredBackend.candidate = anchoredState;
        const auto anchoredResult = sk::evaluateLocalSolve(
            *anchoredInput, precise, anchoredBackend);
        CHECK(anchoredResult.status ==
            sk::LocalSolveGatewayStatus::candidate_accepted);
        CHECK(anchoredResult.candidate);
        CHECK(anchoredResult.candidate->lines[0] == anchoredState.lines[0]);
        CHECK(anchoredResult.candidate->lines[1].start ==
            anchoredState.lines[0].end);
        CHECK(anchored.state() == anchoredState);

        // A multi-relation chain cannot accumulate pairwise tolerances
        // into a larger displacement from the canonical vertex.
        sk::SketchModel chained;
        const auto ca = chained.addLine({0.0, 0.0}, {2.0, 0.0});
        const auto cb = chained.addLine({2.0 + 8.0e-13, 0.0}, {3.0, 1.0});
        const auto cc = chained.addLine({2.0 + 1.6e-12, 0.0}, {4.0, 2.0});
        CHECK(chained.addConstraint(sk::ConstraintKind::coincident,
            {ca, sk::ConstraintElement::line_end},
            {cb, sk::ConstraintElement::line_start}));
        CHECK(chained.addConstraint(sk::ConstraintKind::coincident,
            {cb, sk::ConstraintElement::line_start},
            {cc, sk::ConstraintElement::line_start}));
        const auto chainedState = chained.state();
        const auto chainedInput = sk::prepareLocalSolveProblem(chainedState);
        CHECK(chainedInput);
        ScriptedBackend chainedBackend;
        chainedBackend.candidate = chainedState;
        const auto chainedRejected = sk::evaluateLocalSolve(
            *chainedInput, precise, chainedBackend);
        CHECK(chainedRejected.status ==
            sk::LocalSolveGatewayStatus::coincident_not_closed);
        CHECK(!chainedRejected.candidate);
        CHECK(chained.state() == chainedState);

        // No authored Coincident means no implicit global gap welding,
        // even though the gap is inside the same numerical error budget.
        sk::SketchModel separate;
        const auto sep1 = separate.addLine({0.0, 0.0}, {2.0, 0.0});
        separate.addLine({2.0 + 5.0e-13, 0.0}, {3.0, 1.0});
        CHECK(separate.addHorizontal(sep1));
        const auto separateState = separate.state();
        const auto separateInput = sk::prepareLocalSolveProblem(separateState);
        CHECK(separateInput);
        ScriptedBackend separateBackend;
        separateBackend.candidate = separateState;
        const auto separateResult = sk::evaluateLocalSolve(
            *separateInput, precise, separateBackend);
        CHECK(separateResult.status ==
            sk::LocalSolveGatewayStatus::candidate_accepted);
        CHECK(separateResult.candidate);
        CHECK(separateResult.candidate->lines[0].end !=
            separateResult.candidate->lines[1].start);

        // A local snapping decision must not break a previously accepted
        // Vertical residual: the ENTIRE candidate is checked again.
        sk::SketchModel coupled;
        const auto cs = coupled.addLine({0.0, 0.0}, {2.0, 0.0});
        const auto ct = coupled.addLine(
            {2.0 + 5.0e-13, 0.0}, {2.0 + 1.2e-12, 4.0});
        CHECK(coupled.addConstraint(sk::ConstraintKind::coincident,
            {cs, sk::ConstraintElement::line_end},
            {ct, sk::ConstraintElement::line_start}));
        CHECK(coupled.addConstraint(sk::ConstraintKind::vertical,
            {ct, sk::ConstraintElement::line}));
        const auto coupledState = coupled.state();
        const auto coupledInput = sk::prepareLocalSolveProblem(coupledState);
        CHECK(coupledInput);
        ScriptedBackend coupledBackend;
        coupledBackend.candidate = coupledState;
        const auto coupledResult = sk::evaluateLocalSolve(
            *coupledInput, precise, coupledBackend);
        CHECK(coupledResult.status ==
            sk::LocalSolveGatewayStatus::residual_violation);
        CHECK(!coupledResult.candidate);
        CHECK(coupled.state() == coupledState);

        // Same authored semantic constraints in different record order
        // must produce the SAME evaluated vertex, not a history-dependent
        // sequence of pairwise 1e-12 tolerance welds.
        sk::SketchModel ordered;
        const auto oa = ordered.addLine({0.0, 0.0}, {10.0, 0.0});
        const auto ob = ordered.addLine({10.0 + 4.0e-13, 0.0}, {11.0, 1.0});
        const auto oc = ordered.addLine({10.0 + 8.0e-13, 0.0}, {12.0, 2.0});
        CHECK(ordered.addConstraint(sk::ConstraintKind::coincident,
            {oa, sk::ConstraintElement::line_end},
            {ob, sk::ConstraintElement::line_start}));
        CHECK(ordered.addConstraint(sk::ConstraintKind::coincident,
            {ob, sk::ConstraintElement::line_start},
            {oc, sk::ConstraintElement::line_start}));
        const auto orderedState = ordered.state();
        auto reversedState = orderedState;
        std::reverse(reversedState.constraints.begin(),
                     reversedState.constraints.end());
        CHECK(sk::SketchModel::restore(reversedState));
        const auto inputOrdered = sk::prepareLocalSolveProblem(orderedState);
        const auto inputReversed = sk::prepareLocalSolveProblem(reversedState);
        CHECK(inputOrdered && inputReversed);
        ScriptedBackend orderedBackend;
        ScriptedBackend reversedBackend;
        orderedBackend.candidate = orderedState;
        reversedBackend.candidate = reversedState;
        const auto solvedOrdered = sk::evaluateLocalSolve(
            *inputOrdered, precise, orderedBackend);
        const auto solvedReversed = sk::evaluateLocalSolve(
            *inputReversed, precise, reversedBackend);
        CHECK(solvedOrdered.status ==
            sk::LocalSolveGatewayStatus::candidate_accepted);
        CHECK(solvedReversed.status ==
            sk::LocalSolveGatewayStatus::candidate_accepted);
        CHECK(solvedOrdered.candidate && solvedReversed.candidate);
        CHECK(solvedOrdered.candidate->lines ==
            solvedReversed.candidate->lines);
        CHECK(solvedOrdered.candidate->lines[0].end ==
            solvedOrdered.candidate->lines[1].start);
        CHECK(solvedOrdered.candidate->lines[1].start ==
            solvedOrdered.candidate->lines[2].start);
        CHECK(ordered.state() == orderedState);

        // A stage may supply two independently frozen projected Lines.
        // A near-but-unequal authored Coincident cannot move EITHER
        // projected source even when both residues fit the numeric budget.
        sk::SketchModel twoProjected;
        const auto p1 = twoProjected.addLine({0.0, 0.0}, {2.0, 0.0});
        const auto p2 = twoProjected.addLine(
            {2.0 + 5.0e-13, 0.0}, {2.0 + 5.0e-13, 3.0});
        CHECK(twoProjected.addConstraint(sk::ConstraintKind::coincident,
            {p1, sk::ConstraintElement::line_end},
            {p2, sk::ConstraintElement::line_start}));
        const auto twoProjectedState = twoProjected.state();
        const std::array<sk::EntityId, 2> projectedFixed{p1, p2};
        const auto twoProjectedInput = sk::prepareLocalSolveProblem(
            twoProjectedState, projectedFixed);
        CHECK(twoProjectedInput);
        ScriptedBackend twoProjectedBackend;
        twoProjectedBackend.candidate = twoProjectedState;
        const auto incompatibleFixed = sk::evaluateLocalSolve(
            *twoProjectedInput, precise, twoProjectedBackend);
        CHECK(incompatibleFixed.status ==
            sk::LocalSolveGatewayStatus::coincident_not_closed);
        CHECK(!incompatibleFixed.candidate);
        CHECK(twoProjected.state() == twoProjectedState);

        // At 100 metres one binary64 ULP is > 1e-12 mm. Reject a
        // representably separate candidate under a strict 1e-12 budget;
        // do not pretend that absolute target is attainable at any scale.
        // A distinct, explicit caller-owned budget may accept one ULP.
        const double large = 100000.0;
        const double oneUlp = std::nextafter(
            large, std::numeric_limits<double>::infinity()) - large;
        CHECK(oneUlp > precise.distance_tolerance);
        sk::SketchModel largeModel;
        const auto largeA = largeModel.addLine(
            {large - 1.0, 0.0}, {large, 0.0});
        const auto largeB = largeModel.addLine(
            {large + oneUlp, 0.0}, {large + 1.0, 1.0});
        CHECK(largeModel.addConstraint(sk::ConstraintKind::coincident,
            {largeA, sk::ConstraintElement::line_end},
            {largeB, sk::ConstraintElement::line_start}));
        const auto largeAuthored = largeModel.state();
        const auto largeInput = sk::prepareLocalSolveProblem(largeAuthored);
        CHECK(largeInput);
        ScriptedBackend largeBackend;
        largeBackend.candidate = largeAuthored;
        const auto strictLarge = sk::evaluateLocalSolve(
            *largeInput, precise, largeBackend);
        CHECK(strictLarge.status ==
            sk::LocalSolveGatewayStatus::residual_violation);
        CHECK(!strictLarge.candidate);
        const sk::LocalSolverAcceptancePolicy explicitOneUlp{
            2.0 * oneUlp, precise.angular_tolerance_radians};
        const auto calibratedLarge = sk::evaluateLocalSolve(
            *largeInput, explicitOneUlp, largeBackend);
        CHECK(calibratedLarge.status ==
            sk::LocalSolveGatewayStatus::candidate_accepted);
        CHECK(calibratedLarge.candidate);
        CHECK(calibratedLarge.candidate->lines[0].end ==
            calibratedLarge.candidate->lines[1].start);
        CHECK(largeModel.state() == largeAuthored);

    }


    // R13-A1.2 Part-owned native bridge: a solver problem must come
    // from the exact PartDocument owner/revision and current stage;
    // PG-01 projection must never fall back to stale authored seeds.
    {
        namespace app = simplesolid2::application;
        namespace part = simplesolid2::part;
        namespace core = simplesolid2::core;
        namespace kernel = simplesolid2::kernel;
        struct RefusingProjectionProvider final : kernel::IEdgeProjectionQuery {
            std::optional<kernel::ScopedProjectionEdge> bindEdgeToBody(
                kernel::RuntimeSolidHandle,
                kernel::RuntimeEdgeToken) noexcept override {
                return std::nullopt;
            }
            kernel::EdgeProjectionResult projectEdgeToPlane(
                kernel::RuntimeSolidHandle,
                const kernel::ScopedProjectionEdge&,
                const kernel::Frame3&) noexcept override {
                return {};
            }
        } provider;
        app::DocumentSession session{
            {}, part::PartDocument::create(core::DocumentId::generate())};
        const auto made = session.execute(
            app::CreatePartSketchCommand{core::BuiltinReferenceRole::xy_plane});
        CHECK(made.ok() && made.sketch_id);
        const auto line = session.execute(
            app::AddSketchLineCommand{*made.sketch_id, {1.0, 2.0}, {3.0, 2.0}});
        CHECK(line.ok() && line.entity_id);
        const auto relation = session.execute(
            app::AddHorizontalSketchConstraintCommand{
                *made.sketch_id, session.document().revision(), *line.entity_id});
        CHECK(relation.ok() && relation.constraint_id);

        part::PartEvaluation prefix;
        prefix.source_document_id = session.document().documentId();
        prefix.source_revision = session.document().revision();
        prefix.source_incarnation = session.document().incarnationToken();
        const auto input = part::prepareStageLocalSolveProblem(
            session.document(), *made.sketch_id, prefix, provider);
        CHECK(input && input->constraints.size() == 1U);
        CHECK(input->authored == session.document().findSketch(*made.sketch_id)->model.state());
        CHECK(input->fixed_lines.empty()); // no projected bindings on this Sketch

        auto invalidOwner = prefix;
        invalidOwner.source_document_id = core::DocumentId::generate();
        CHECK(!part::prepareStageLocalSolveProblem(
            session.document(), *made.sketch_id, invalidOwner, provider));
        auto invalidRevision = prefix;
        invalidRevision.source_revision = {};
        CHECK(!part::prepareStageLocalSolveProblem(
            session.document(), *made.sketch_id, invalidRevision, provider));
        invalidOwner = prefix;
        invalidOwner.source_document_id.reset();
        CHECK(!part::prepareStageLocalSolveProblem(
            session.document(), *made.sketch_id, invalidOwner, provider));

        // Subsequent real Part mutation retires the previous prefix.
        const auto added = session.execute(
            app::AddSketchLineCommand{*made.sketch_id, {10.0, 1.0}, {12.0, 1.0}});
        CHECK(added.ok());
        CHECK(!part::prepareStageLocalSolveProblem(
            session.document(), *made.sketch_id, prefix, provider));
        prefix.source_revision = session.document().revision();
        prefix.source_incarnation = session.document().incarnationToken();
        const auto updated = part::prepareStageLocalSolveProblem(
            session.document(), *made.sketch_id, prefix, provider);
        CHECK(updated && updated->lines.size() == 2U);
        CHECK(updated->constraints.size() == 1U);
        CHECK(!part::prepareStageLocalSolveProblem(
            session.document(), sk::SketchId::generate(), prefix, provider));

        // Manual Fix needs NO projected edge or upstream Body at all.
        // The same production Part bridge must supply a frozen Line.
        const auto fix = session.execute(app::AddSketchRelationCommand{
            *made.sketch_id, session.document().revision(),
            sk::ConstraintKind::fix,
            {*line.entity_id, sk::ConstraintElement::line}, {}});
        CHECK(fix.ok() && fix.constraint_id);
        CHECK(!part::prepareStageLocalSolveProblem(
            session.document(), *made.sketch_id, prefix, provider));
        prefix.source_revision = session.document().revision();
        prefix.source_incarnation = session.document().incarnationToken();
        const auto withFix = part::prepareStageLocalSolveProblem(
            session.document(), *made.sketch_id, prefix, provider);
        CHECK(withFix && withFix->fixed_lines.size() == 1U);
        CHECK(withFix->fixed_lines.front() == *line.entity_id);
        CHECK(withFix->lines.front().fixed);
        CHECK(!withFix->lines.back().fixed);
        CHECK(withFix->constraints.size() == 2U);
        CHECK(withFix->constraints.back().kind == sk::ConstraintKind::fix);
        CHECK(withFix->equations.size() == 1U);
        CHECK(withFix->equations.front().kind == sk::ConstraintKind::horizontal);

        // A1.2 Part-owned, current-stage evaluation gateway: never call
        // a backend for stale/foreign Part evidence and never advertise
        // a candidate whose source revision changed DURING solve.
        struct StageBackend final : sk::ILocalSketchSolver {
            unsigned calls{};
            app::DocumentSession* mutate_session{};
            std::optional<sk::SketchId> mutate_sketch;

            sk::LocalBackendSolveResult solve(
                const sk::LocalSolveProblem& problem) override {
                ++calls;
                if (mutate_session && mutate_sketch) {
                    const auto edit = mutate_session->execute(
                        app::AddSketchLineCommand{
                            *mutate_sketch, {15.0, 1.0}, {17.0, 1.0}});
                    CHECK(edit.ok() && edit.changed);
                }
                return {sk::LocalBackendStatus::converged,
                    problem.authored, {}, {}, 2U};
            }
        } stageBackend;

        const sk::LocalSolverAcceptancePolicy tight{1.0e-12, 1.0e-12};
        const auto stageSolved = part::evaluateStageLocalSolve(
            session.document(), *made.sketch_id, prefix, provider,
            tight, stageBackend);
        CHECK(stageSolved);
        CHECK(stageSolved->source_document_id ==
              session.document().documentId());
        CHECK(stageSolved->source_revision == session.document().revision());
        CHECK(stageSolved->source_sketch_id == *made.sketch_id);
        CHECK(!stageSolved->consuming_feature);
        CHECK(stageSolved->solution.status ==
            sk::LocalSolveGatewayStatus::candidate_accepted);
        CHECK(stageSolved->solution.candidate);
        CHECK(stageSolved->solution.candidate->lines.size() == 2U);
        CHECK(stageSolved->solution.candidate->lines.front() ==
            withFix->authored.lines.front());
        CHECK(stageBackend.calls == 1U);

        // CP1 RED: two independent restores of the SAME authored state have
        // the same durable DocumentId and numeric revision zero, but must
        // never accept each other's disposable PartEvaluation. No provider
        // callback or numerical backend may run on the foreign incarnation.
        const auto coldFirst = part::PartDocument::restore(
            session.document().documentId(), session.document().state());
        const auto coldSecond = part::PartDocument::restore(
            session.document().documentId(), session.document().state());
        CHECK(coldFirst.ok() && coldSecond.ok());
        CHECK(coldFirst.document->documentId() ==
              coldSecond.document->documentId());
        CHECK(coldFirst.document->revision() ==
              coldSecond.document->revision());
        CHECK(coldFirst.document->incarnationToken() !=
              coldSecond.document->incarnationToken());
        part::PartEvaluation oldCold;
        oldCold.source_document_id = coldFirst.document->documentId();
        oldCold.source_revision = coldFirst.document->revision();
        oldCold.source_incarnation = coldFirst.document->incarnationToken();
        CHECK(!part::ownsCurrentPartEvaluation(*coldSecond.document, oldCold));
        CHECK(!part::prepareStageLocalSolveProblem(
            *coldSecond.document, *made.sketch_id, oldCold, provider));
        CHECK(!part::evaluateEffectiveSketchProjection(
            *coldSecond.document, *made.sketch_id, oldCold, provider));
        StageBackend rejectedBackend;
        CHECK(!part::evaluateStageLocalSolve(
            *coldSecond.document, *made.sketch_id, oldCold, provider,
            tight, rejectedBackend));
        CHECK(rejectedBackend.calls == 0U);
        auto absentToken = oldCold;
        absentToken.source_incarnation.reset();
        CHECK(!part::ownsCurrentPartEvaluation(
            *coldFirst.document, absentToken));
        CHECK(!part::prepareStageLocalSolveProblem(
            *coldFirst.document, *made.sketch_id, absentToken, provider));

        part::PartEvaluation newCold;
        newCold.source_document_id = coldSecond.document->documentId();
        newCold.source_revision = coldSecond.document->revision();
        newCold.source_incarnation = coldSecond.document->incarnationToken();
        CHECK(part::ownsCurrentPartEvaluation(*coldSecond.document, newCold));
        CHECK(part::prepareStageLocalSolveProblem(
            *coldSecond.document, *made.sketch_id, newCold, provider));
        CHECK(part::evaluateEffectiveSketchProjection(
            *coldSecond.document, *made.sketch_id, newCold, provider));
        const auto newColdSolve = part::evaluateStageLocalSolve(
            *coldSecond.document, *made.sketch_id, newCold, provider,
            tight, rejectedBackend);
        CHECK(newColdSolve);
        CHECK(rejectedBackend.calls == 1U);
        CHECK(newColdSolve->solution.status ==
              sk::LocalSolveGatewayStatus::candidate_accepted);

        auto wrongStage = prefix;
        wrongStage.source_document_id = core::DocumentId::generate();
        CHECK(!part::evaluateStageLocalSolve(
            session.document(), *made.sketch_id, wrongStage, provider,
            tight, stageBackend));
        CHECK(!part::evaluateStageLocalSolve(
            session.document(), sk::SketchId::generate(), prefix, provider,
            tight, stageBackend));
        CHECK(!part::evaluateStageLocalSolve(
            session.document(), *made.sketch_id, prefix, provider,
            sk::LocalSolverAcceptancePolicy{0.0, tight.angular_tolerance_radians},
            stageBackend));
        CHECK(stageBackend.calls == 1U);

        // A real DocumentSession mutation during a reentrant adapter call
        // retires this stage even if that backend declares convergence.
        stageBackend.mutate_session = &session;
        stageBackend.mutate_sketch = *made.sketch_id;
        CHECK(!part::evaluateStageLocalSolve(
            session.document(), *made.sketch_id, prefix, provider,
            tight, stageBackend));
        CHECK(stageBackend.calls == 2U);
        CHECK(session.document().revision() != prefix.source_revision);
        CHECK(!part::evaluateStageLocalSolve(
            session.document(), *made.sketch_id, prefix, provider,
            tight, stageBackend));
        CHECK(stageBackend.calls == 2U);
    }

    // Owner-approved sixth relation: Fix(Line) locks both endpoints in the
    // authored Sketch. No projected Body, implicit origin anchor or solver
    // is needed to ground ordinary sketch lines.
    {
        using K = sk::ConstraintKind;
        using E = sk::ConstraintElement;
        sk::SketchModel model;
        const auto locked = model.addLine({1.0, 3.0}, {5.0, 7.0});
        const auto freeLine = model.addLine({0.0, 1.0}, {8.0, 2.0});
        const auto untouched = model.state();
        CHECK(!model.isFixedLine(locked));
        const auto fix = model.addConstraint(K::fix, {locked, E::line});
        CHECK(fix && fix->serialized() == "1");
        CHECK(model.isFixedLine(locked));
        CHECK(!model.isFixedLine(freeLine));
        const auto frozen = model.state();
        CHECK(frozen.constraints.size() == 1U);
        CHECK(frozen.constraints.front().kind == K::fix);
        CHECK(frozen.lines == untouched.lines);
        CHECK(!model.addConstraint(K::fix, {locked, E::line}));
        CHECK(!model.addConstraint(K::fix, {locked, E::line_start}));
        CHECK(!model.addConstraint(K::fix, {locked, E::line_end}));
        CHECK(!model.addConstraint(K::fix, {locked, E::line},
                                   {freeLine, E::line}));
        CHECK(model.state() == frozen); // no duplicate ID consumed
        CHECK(!model.updateLine(locked, {8.0, 9.0}, {5.0, 7.0}));
        CHECK(!model.updateLine(locked, {1.0, 3.0}, {5.0, 7.0}));
        CHECK(model.findLine(locked)->start() == frozen.lines[0].start);
        CHECK(model.findLine(locked)->end() == frozen.lines[0].end);
        CHECK(model.updateLine(freeLine, {0.0, 2.0}, {8.0, 3.0}));

        const auto solvedInput = sk::prepareLocalSolveProblem(model.state());
        CHECK(solvedInput && solvedInput->lines.size() == 2U);
        CHECK(solvedInput->constraints.size() == 1U);
        CHECK(solvedInput->equations.empty()); // Fix is not a solver equation
        CHECK(solvedInput->fixed_lines.size() == 1U);
        CHECK(solvedInput->fixed_lines.front() == locked);
        CHECK(solvedInput->lines.front().fixed);
        CHECK(!solvedInput->lines.back().fixed);
        const sk::LocalSolverAcceptancePolicy policy{1.0e-7, 1.0e-6};
        const auto fixedOnlyReview = sk::reviewLocalSolveCandidate(
            *solvedInput, model.state(), policy);
        CHECK(fixedOnlyReview.status ==
            sk::LocalResidualStatus::residuals_satisfied);
        CHECK(fixedOnlyReview.constraints.empty()); // Fix has no equation residual
        auto movedFixed = model.state();
        movedFixed.lines.front().start.u += 1.0e-9;
        CHECK(sk::reviewLocalSolveCandidate(
            *solvedInput, movedFixed, policy).status ==
            sk::LocalResidualStatus::fixed_entity_changed);
        auto restored = sk::SketchModel::restore(model.state());
        CHECK(restored && restored->isFixedLine(locked));
        CHECK(!restored->updateLine(locked, {7.0, 7.0}, {9.0, 9.0}));
        CHECK(restored->eraseConstraint(*fix));
        CHECK(!restored->isFixedLine(locked));
        CHECK(restored->updateLine(locked, {7.0, 7.0}, {9.0, 9.0}));
        CHECK(model.erase(locked)); // dependent Fix is removed with line
        CHECK(model.constraintCount() == 0U);
        CHECK(!model.isFixedLine(locked));
    }

    // Contradictions between immutable Fix coordinates and new geometric
    // intent must reject before allocating an ID or calling a solver.
    // Restore also rejects malformed/impossible authored combinations.
    {
        using K = sk::ConstraintKind;
        using E = sk::ConstraintElement;
        sk::SketchModel blocked;
        const auto diagonal = blocked.addLine({0.0, 0.0}, {3.0, 4.0});
        const auto parallel = blocked.addLine({1.0, 1.0}, {7.0, 8.0});
        const auto offset = blocked.addLine({0.0, 1.0}, {3.0, 5.0});
        const auto af = blocked.addConstraint(K::fix, {diagonal, E::line});
        CHECK(af);
        const auto preserved = blocked.state();
        CHECK(!blocked.addConstraint(K::horizontal, {diagonal, E::line}));
        CHECK(!blocked.addConstraint(K::vertical, {diagonal, E::line}));
        CHECK(blocked.state() == preserved);
        CHECK(blocked.constraintIdCursor().serialized() == "2");
        CHECK(blocked.addConstraint(K::fix, {parallel, E::line}));
        const auto twoFixed = blocked.state();
        CHECK(!blocked.addConstraint(
            K::parallel, {diagonal, E::line}, {parallel, E::line}));
        CHECK(!blocked.addConstraint(
            K::perpendicular, {diagonal, E::line}, {parallel, E::line}));
        CHECK(!blocked.addConstraint(
            K::coincident, {diagonal, E::line_start}, {parallel, E::line_start}));
        CHECK(blocked.state() == twoFixed);
        auto corrupt = twoFixed;
        corrupt.constraints.push_back(sk::SketchConstraintState{
            *sk::ConstraintId::parse("3"), K::horizontal,
            {diagonal, E::line}, {}});
        corrupt.next_constraint_id = *sk::ConstraintIdCursor::parse("4");
        CHECK(!sk::SketchModel::restore(corrupt));
        corrupt = twoFixed;
        corrupt.constraints.push_back(sk::SketchConstraintState{
            *sk::ConstraintId::parse("3"), K::parallel,
            {diagonal, E::line}, {parallel, E::line}});
        corrupt.next_constraint_id = *sk::ConstraintIdCursor::parse("4");
        CHECK(!sk::SketchModel::restore(corrupt));
        CHECK(!blocked.isFixedLine(offset));
        const auto earlierIntent = blocked.addConstraint(
            K::horizontal, {offset, E::line});
        CHECK(earlierIntent); // free Line can be solved later
        const auto beforeImpossibleFix = blocked.state();
        CHECK(!blocked.addConstraint(K::fix, {offset, E::line}));
        CHECK(blocked.state() == beforeImpossibleFix);
    }

    // Do not confuse a genuinely compatible set of fixed references
    // with a contradiction; the backend still owns DoF/redundancy.
    {
        using K = sk::ConstraintKind;
        using E = sk::ConstraintElement;
        sk::SketchModel model;
        const auto horizontal = model.addLine({0.0, 0.0}, {4.0, 0.0});
        const auto vertical = model.addLine({4.0, 0.0}, {4.0, 5.0});
        CHECK(model.addConstraint(K::fix, {horizontal, E::line}));
        CHECK(model.addConstraint(K::fix, {vertical, E::line}));
        CHECK(model.addConstraint(K::horizontal, {horizontal, E::line}));
        CHECK(model.addConstraint(K::vertical, {vertical, E::line}));
        CHECK(model.addConstraint(K::coincident,
            {horizontal, E::line_end}, {vertical, E::line_start}));
        CHECK(model.addConstraint(K::perpendicular,
            {horizontal, E::line}, {vertical, E::line}));
        const auto current = model.state();
        const auto restored = sk::SketchModel::restore(current);
        CHECK(restored && restored->state() == current);
        const auto prepared = sk::prepareLocalSolveProblem(current);
        CHECK(prepared && prepared->fixed_lines.size() == 2U);
        CHECK(prepared->lines[0].fixed && prepared->lines[1].fixed);
        const sk::LocalSolverAcceptancePolicy policy{1.0e-8, 1.0e-6};
        const auto checked = sk::reviewLocalSolveCandidate(*prepared, current, policy);
        CHECK(checked.status == sk::LocalResidualStatus::residuals_satisfied);
    }

    // Exact fixed geometry must not become a false conflict because
    // atan2/remainder accumulates a binary64 angle-roundoff. These
    // non-axis-aligned integer directions have exact zero dot/cross.
    {
        using K = sk::ConstraintKind;
        using E = sk::ConstraintElement;
        sk::SketchModel model;
        const auto first = model.addLine({0.0, 0.0}, {-20.0, 1.0});
        const auto perpendicular = model.addLine({0.0, 0.0}, {3.0, 60.0});
        const auto parallel = model.addLine({0.0, 0.0}, {-60.0, 3.0});
        const auto nearPerpendicular = model.addLine({0.0, 0.0}, {3.0, 59.0});
        CHECK(model.addConstraint(K::fix, {first, E::line}));
        CHECK(model.addConstraint(K::fix, {perpendicular, E::line}));
        CHECK(model.addConstraint(K::fix, {parallel, E::line}));
        CHECK(model.addConstraint(K::fix, {nearPerpendicular, E::line}));
        CHECK(model.addConstraint(K::perpendicular,
            {first, E::line}, {perpendicular, E::line}));
        CHECK(model.addConstraint(K::parallel,
            {first, E::line}, {parallel, E::line}));
        const auto accepted = model.state();
        CHECK(!model.addConstraint(K::perpendicular,
            {first, E::line}, {nearPerpendicular, E::line}));
        CHECK(model.state() == accepted);
        const auto restored = sk::SketchModel::restore(accepted);
        CHECK(restored && restored->state() == accepted);
    }

    // Fix is an ordinary undoable revision-bound DocumentSession relation;
    // attempts to mutate it must fail atomically, without dirty/history
    // changes, and become possible only after explicit Unfix.
    {
        namespace app = simplesolid2::application;
        namespace part = simplesolid2::part;
        namespace core = simplesolid2::core;
        using K = sk::ConstraintKind;
        using E = sk::ConstraintElement;
        app::DocumentSession session{
            {}, part::PartDocument::create(core::DocumentId::generate())};
        const auto made = session.execute(
            app::CreatePartSketchCommand{core::BuiltinReferenceRole::xy_plane});
        CHECK(made.ok() && made.sketch_id);
        const auto line = session.execute(
            app::AddSketchLineCommand{*made.sketch_id, {2.0, 3.0}, {7.0, 8.0}});
        CHECK(line.ok() && line.entity_id);
        const auto initialRevision = session.document().revision();
        const auto initialDepth = session.undoDepth();
        const auto fixed = session.execute(app::AddSketchRelationCommand{
            *made.sketch_id, initialRevision, K::fix,
            {*line.entity_id, E::line}, {}});
        CHECK(fixed.ok() && fixed.changed && fixed.constraint_id);
        CHECK(session.document().findSketch(*made.sketch_id)->
            model.isFixedLine(*line.entity_id));
        CHECK(session.undoDepth() == initialDepth + 1U);
        CHECK(session.needsSave());
        const auto depth = session.undoDepth();
        const auto afterFixRevision = session.document().revision();
        const auto contradictory = session.execute(app::AddSketchRelationCommand{
            *made.sketch_id, afterFixRevision, K::horizontal,
            {*line.entity_id, E::line}, {}});
        CHECK(!contradictory.ok() && !contradictory.changed);
        CHECK(contradictory.diagnostic.code ==
            app::DocumentSessionErrorCode::invalid_command);
        CHECK(session.document().revision() == afterFixRevision);
        CHECK(session.undoDepth() == depth);
        const auto attempted = session.execute(app::UpdateSketchGeometryCommand{
            *made.sketch_id, afterFixRevision,
            {{*line.entity_id, {2.0, 3.0}, {10.0, 8.0}}}, {}, {}});
        CHECK(!attempted.ok() && !attempted.changed);
        CHECK(attempted.diagnostic.code ==
            app::DocumentSessionErrorCode::invalid_command);
        CHECK(session.document().revision() == afterFixRevision);
        CHECK(session.undoDepth() == depth);
        const auto other = session.execute(app::AddSketchLineCommand{
            *made.sketch_id, {3.0, 0.0}, {3.0, 12.0}});
        CHECK(other.ok() && other.entity_id);
        const auto fixedDepth = session.undoDepth();
        const auto fixedRevision = session.document().revision();
        const auto trimBlocked = session.execute(app::TrimSketchCommand{
            *made.sketch_id, fixedRevision, *line.entity_id,
            {*other.entity_id}, {4.0, 5.0}});
        CHECK(!trimBlocked.changed && !trimBlocked.ok());
        CHECK(trimBlocked.diagnostic.code ==
            app::DocumentSessionErrorCode::invalid_command);
        const auto extendBlocked = session.execute(app::ExtendSketchCommand{
            *made.sketch_id, fixedRevision, *line.entity_id,
            {*other.entity_id}, sk::StructuralEndpointRole::end});
        CHECK(!extendBlocked.changed);
        CHECK(extendBlocked.diagnostic.code ==
            app::DocumentSessionErrorCode::invalid_command);
        const auto extendBothBlocked =
            session.execute(app::ExtendBothSketchLinesCommand{
                *made.sketch_id, fixedRevision,
                *line.entity_id, *other.entity_id});
        CHECK(!extendBothBlocked.changed);
        CHECK(extendBothBlocked.diagnostic.code ==
            app::DocumentSessionErrorCode::invalid_command);
        CHECK(session.undoDepth() == fixedDepth);
        CHECK(session.document().revision() == fixedRevision);
        CHECK(!session.execute(app::AddSketchRelationCommand{
            *made.sketch_id, initialRevision, K::fix,
            {*line.entity_id, E::line}, {}}).ok());
        CHECK(session.undo().changed); // retire unrelated second Line first
        CHECK(session.document().findSketch(*made.sketch_id)->
            model.isFixedLine(*line.entity_id));
        CHECK(session.undo().changed); // then undo Fix itself
        CHECK(!session.document().findSketch(*made.sketch_id)->
            model.isFixedLine(*line.entity_id));
        CHECK(session.redo().changed);
        CHECK(session.document().findSketch(*made.sketch_id)->
            model.isFixedLine(*line.entity_id));
        CHECK(session.execute(app::EraseSketchConstraintCommand{
            *made.sketch_id, session.document().revision(), *fixed.constraint_id}).changed);
        CHECK(!session.document().findSketch(*made.sketch_id)->
            model.isFixedLine(*line.entity_id));
        CHECK(session.execute(app::UpdateSketchGeometryCommand{
            *made.sketch_id, session.document().revision(),
            {{*line.entity_id, {2.0, 3.0}, {10.0, 8.0}}}, {}, {}}).changed);
    }

    std::cout << "R13-A1 Horizontal authored-state test PASS\n";
}

#else

// Real Windows kernel-native evidence: existing authored ProfileRegionIntent,
// actual Part-stage numerical acceptance and the REAL OCCT solid provider.
// A scripted backend stands in for the still-unlicensed PlaneGCS solver.
// This test does exercise the production Feature scheduler with an explicit
// TEST-ONLY backend. It does not install PlaneGCS, alter default evaluation,
// write a native file or provide a vNext persistence proof.
int main() {
    namespace app = simplesolid2::application;
    namespace part = simplesolid2::part;
    namespace core = simplesolid2::core;
    namespace kernel = simplesolid2::kernel;
    namespace occt = simplesolid2::kernel_occt;

    struct ControlledSolver final : sk::ILocalSketchSolver {
        double seam_offset{5.0e-13};
        double translated_u{};
        unsigned calls{};

        sk::LocalBackendSolveResult solve(
            const sk::LocalSolveProblem& problem) override {
            ++calls;
            auto solved = problem.authored;
            CHECK(solved.lines.size() == 4U);
            // The explicit Coincident tolerates the residual, but
            // the downstream Profile/OCCT needs ONE exact vertex.
            // The opt-in Feature scheduler must ALSO consume material
            // solver changes, not just accept the old authored seed.
            for (auto& line : solved.lines) {
                line.start.u += translated_u;
                line.end.u += translated_u;
            }
            solved.lines[1].start.u += seam_offset;
            return {sk::LocalBackendStatus::converged,
                    std::move(solved), {}, {}, 3U};
        }
    } backend;
    occt::OcctSolidModelingKernel provider;
    app::DocumentSession session{
        {}, part::PartDocument::create(core::DocumentId::generate())};
    const auto sketch = session.execute(
        app::CreatePartSketchCommand{core::BuiltinReferenceRole::xy_plane});
    CHECK(sketch.ok() && sketch.sketch_id);

    // All x > 0 permits both a normal OCCT Extrude and full Revolve
    // around the unmodified global Y-axis (an annular solid).
    const auto rectangle = session.execute(app::AddSketchRectangleCommand{
        *sketch.sketch_id, session.document().revision(),
        {5.0, 0.0}, {15.0, 8.0}, sk::EntityRole::regular, false});
    CHECK(rectangle.ok() && rectangle.entity_ids.size() == 4U);

    const auto* original = session.document().findSketch(*sketch.sketch_id);
    CHECK(original != nullptr);
    const auto seed = original->model.state();
    CHECK(seed.lines.size() == 4U);
    CHECK(seed.lines[0].end == seed.lines[1].start);
    const auto analysis = sk::analyzeRegions(original->model);
    CHECK(analysis.complete() && analysis.regions.size() == 1U);
    const auto intent = part::makeProfileRegionIntent(analysis.regions.front());
    CHECK(intent);
    const auto profile = session.execute(app::CreateProfileCommand{
        *sketch.sketch_id, session.document().revision(), *intent});
    CHECK(profile.ok() && profile.profile_id);
    const auto coincide = session.execute(app::AddSketchRelationCommand{
        *sketch.sketch_id, session.document().revision(),
        sk::ConstraintKind::coincident,
        {seed.lines[0].id, sk::ConstraintElement::line_end},
        {seed.lines[1].id, sk::ConstraintElement::line_start}});
    CHECK(coincide.ok() && coincide.constraint_id);

    CHECK(!session.document().evaluateProfile(*profile.profile_id));
    CHECK(part::resolveKernelProfileInput(
        session.document(), *profile.profile_id).status ==
        part::ProfileKernelInputStatus::unresolved_profile);

    part::PartEvaluation current;
    current.source_document_id = session.document().documentId();
    current.source_revision = session.document().revision();
        current.source_incarnation = session.document().incarnationToken();
    const sk::LocalSolverAcceptancePolicy policy{1.0e-12, 1.0e-12};
    const auto solved = part::resolveStageSolvedProfileKernelInput(
        session.document(), *profile.profile_id, current,
        provider, policy, backend);
    CHECK(solved.ok() && solved.input);
    CHECK(backend.calls == 1U);
    CHECK(solved.input->outer.boundary.size() == 4U);

    // Verify genuine Profile→OCCT Face construction before Extrude.
    const auto face = occt::buildProfileFaceEvidence(*solved.input);
    CHECK(face.brep_valid);
    CHECK(face.solid_count == 0U); // planar Face, not already a Body

    kernel::LinearExtrudeInput extrude_input;
    extrude_input.profile = *solved.input;
    extrude_input.start_offset_mm = 0.0;
    extrude_input.end_offset_mm = 10.0;
    CHECK(extrude_input.valid());
    const auto extruded = provider.extrude(extrude_input);
    CHECK(extruded.ok());
    CHECK(extruded.solid_count == 1U);
    CHECK(extruded.face_count >= 6U);

    // Provenance survives the actual OCCT prism, not just the neutral
    // Profile adapter: each authored boundary yields ONE resolvable side.
    const auto extrude_evidence =
        occt::buildProfileExtrudeEvidence(*solved.input, 10.0);
    CHECK(extrude_evidence.ok());
    CHECK(extrude_evidence.shape.brep_valid);
    CHECK(extrude_evidence.shape.solid_count == 1U);
    CHECK(extrude_evidence.sides.size() == 4U);
    std::set<std::string> expected_sources;
    for (const auto& line : seed.lines) {
        CHECK(expected_sources.insert(line.id.serialized()).second);
    }
    std::set<std::string> extruded_sources;
    for (const auto& side : extrude_evidence.sides) {
        CHECK(side.status == kernel::ReferenceStatus::resolved);
        CHECK(side.candidate_face_count == 1U);
        CHECK(side.provenance);
        CHECK(!side.provenance->hole);
        CHECK(extruded_sources.insert(
            side.provenance->source_entity).second);
    }
    CHECK(extruded_sources == expected_sources);

    kernel::AngularRevolveInput revolve_input;
    revolve_input.profile = *solved.input;
    revolve_input.axis.origin = {0.0, 0.0, 0.0};
    revolve_input.axis.direction = {0.0, 1.0, 0.0};
    revolve_input.start_angle_radians = 0.0;
    revolve_input.end_angle_radians =
        2.0 * std::numbers::pi_v<double>;
    CHECK(revolve_input.valid());
    const auto revolved = provider.revolve(revolve_input);
    CHECK(revolved.ok());
    CHECK(revolved.solid_count == 1U);
    CHECK(revolved.brep_valid);

    // The *existing* OCCT revolve evidence must map each semantic edge
    // back to the same durable EntityId. Native periodic seams are not
    // substitute authored edges and may not be silently used as such.
    const auto revolve_evidence = occt::buildProfileFullRevolveEvidence(
        *solved.input, {0.0, 0.0}, {0.0, 1.0});
    CHECK(revolve_evidence.ok());
    CHECK(revolve_evidence.shape.brep_valid);
    CHECK(revolve_evidence.shape.solid_count == 1U);
    CHECK(revolve_evidence.boundary_faces.size() == 4U);
    std::set<std::string> revolved_sources;
    for (const auto& boundary : revolve_evidence.boundary_faces) {
        CHECK(boundary.status == kernel::ReferenceStatus::resolved);
        CHECK(boundary.candidate_face_count == 1U);
        CHECK(revolved_sources.insert(
            boundary.provenance.source_entity).second);
    }
    CHECK(revolved_sources == expected_sources);

    // The numerical bridge alone authorizes the provider calls above.
    // A representably open seam outside 1e-12 must never reach OCCT.
    backend.seam_offset = 5.0e-12;
    const auto rejected = part::resolveStageSolvedProfileKernelInput(
        session.document(), *profile.profile_id, current,
        provider, policy, backend);
    CHECK(rejected.status ==
        part::ProfileKernelInputStatus::unresolved_profile);
    CHECK(!rejected.input);
    CHECK(backend.calls == 2U);
    CHECK(session.document().findProfile(*profile.profile_id));
    CHECK(session.document().findSketch(*sketch.sketch_id)
        ->model.state().lines == seed.lines);

    // A refused candidate must not poison either the authored intent or
    // the old Profile/OCCT path. Undo the ONLY new Coincident relation;
    // the exact same ProfileId must immediately work through the original
    // public resolver and produce a genuine native solid again.
    CHECK(session.undo().changed);
    const auto* unconstrained =
        session.document().findProfile(*profile.profile_id);
    CHECK(unconstrained != nullptr);
    CHECK(session.document().findSketch(*sketch.sketch_id)
        ->model.state().constraints.empty());
    const auto original_profile =
        part::resolveKernelProfileInput(session.document(),
                                       *profile.profile_id);
    CHECK(original_profile.ok());
    kernel::LinearExtrudeInput after_undo = extrude_input;
    after_undo.profile = *original_profile.input;
    const auto restored_solid = provider.extrude(after_undo);
    CHECK(restored_solid.ok());
    CHECK(restored_solid.brep_valid);
    CHECK(restored_solid.solid_count == 1U);

    // Redo restores the exact authored relation identity and fail-closed
    // unsolved Profile state; no stale OCCT or solver result is reused.
    CHECK(session.redo().changed);
    CHECK(session.document().findProfile(*profile.profile_id) != nullptr);
    const auto after_redo =
        session.document().findSketch(*sketch.sketch_id)->model.state();
    CHECK(after_redo.lines == seed.lines);
    CHECK(after_redo.constraints.size() == 1U);
    CHECK(after_redo.constraints.front().id == *coincide.constraint_id);
    CHECK(part::resolveKernelProfileInput(
        session.document(), *profile.profile_id).status ==
        part::ProfileKernelInputStatus::unresolved_profile);
    CHECK(!session.document().evaluateProfile(*profile.profile_id));
    CHECK(backend.calls == 2U);

    // A1.3 RED->GREEN: from SAME persisted ProfileIntent/ConstraintId into the
    // REAL existing Part Feature scheduler (not a naked OCCT adapter).
    // The original product evaluatePart must remain fail-closed until a
    // native numeric solver is explicitly supplied.
    auto staged = session.document().state();
    const auto feature_id = staged.body.next_feature_id.allocate();
    CHECK(feature_id);
    staged.body.features.push_back(part::PartFeature{
        *feature_id,
        "R13StageExtrude",
        false,
        part::ExtrudeFeature{
            *profile.profile_id,
            part::ExtrudeOperation::add,
            part::OneSidedExtrudeExtent{
                core::LengthValue{10.0}, false}}});
    auto owning = part::PartDocument::restore(
        session.document().documentId(),
        std::move(staged), session.document().revision());
    CHECK(owning.ok());
    CHECK(owning.document);
    const auto legacy_body = part::evaluatePart(*owning.document, provider);
    CHECK(legacy_body.features.size() == 1U);
    CHECK(legacy_body.features.front().status ==
        part::FeatureEvaluationStatus::blocked);
    backend.seam_offset = 5.0e-13;
    const auto actual_body = part::evaluatePartWithLocalSolve(
        *owning.document, provider, policy, backend);
    CHECK(actual_body.source_document_id == owning.document->documentId());
    CHECK(actual_body.source_revision == owning.document->revision());
    CHECK(actual_body.body_status ==
        part::BodyEvaluationStatus::up_to_date);
    CHECK(actual_body.features.size() == 1U);
    CHECK(actual_body.features.front().feature_id == *feature_id);
    CHECK(actual_body.features.front().status ==
        part::FeatureEvaluationStatus::up_to_date);
    CHECK(actual_body.features.front().result_topology);
    CHECK(actual_body.features.front().result_topology->complete());
    CHECK(actual_body.body_solid != nullptr);
    CHECK(actual_body.current_topology);
    CHECK(actual_body.current_topology->complete());
    CHECK(backend.calls == 3U);

    // Nontrivial derived geometry must reach the actual runtime OCCT
    // solid generated by the SAME Part Feature scheduler. A caller-owned
    // backend shifts all 4 Line endpoints by +2 mm in Sketch U; the
    // Feature Body X bounds (not merely neutral Profile input) must shift
    // by exactly that amount within mesh numeric accuracy.
    const auto original_mesh = provider.presentationMesh(
        actual_body.body_solid);
    CHECK(original_mesh.ok());
    const auto x_bounds = [](const kernel::SolidPresentationMesh& mesh) {
        double low = std::numeric_limits<double>::infinity();
        double high = -std::numeric_limits<double>::infinity();
        for (const auto& triangle : mesh.triangles) {
            for (const auto& point :
                 {triangle.first, triangle.second, triangle.third}) {
                low = std::min(low, point.x);
                high = std::max(high, point.x);
            }
        }
        return std::pair{low, high};
    };
    const auto original_bounds = x_bounds(original_mesh.mesh);
    CHECK(std::isfinite(original_bounds.first));
    CHECK(std::isfinite(original_bounds.second));
    CHECK(std::abs(original_bounds.first - 5.0) < 0.01);
    CHECK(std::abs(original_bounds.second - 15.0) < 0.01);
    backend.translated_u = 2.0;
    const auto translated_body = part::evaluatePartWithLocalSolve(
        *owning.document, provider, policy, backend);
    CHECK(translated_body.body_status ==
        part::BodyEvaluationStatus::up_to_date);
    CHECK(translated_body.features.size() == 1U);
    CHECK(translated_body.features.front().status ==
        part::FeatureEvaluationStatus::up_to_date);
    CHECK(translated_body.body_solid != nullptr);
    const auto translated_mesh = provider.presentationMesh(
        translated_body.body_solid);
    CHECK(translated_mesh.ok());
    const auto translated_bounds = x_bounds(translated_mesh.mesh);
    CHECK(std::abs(translated_bounds.first -
        original_bounds.first - 2.0) < 0.01);
    CHECK(std::abs(translated_bounds.second -
        original_bounds.second - 2.0) < 0.01);
    CHECK(backend.calls == 4U);
    backend.translated_u = 0.0;
    CHECK(owning.document->findProfile(*profile.profile_id));
    CHECK(owning.document->findSketch(*sketch.sketch_id)->model
        .state().constraints.front().id == *coincide.constraint_id);

    // Violated residual and obsolete revision cannot reach Feature/OCCT.
    backend.seam_offset = 5.0e-12;
    const auto rejected_body = part::evaluatePartWithLocalSolve(
        *owning.document, provider, policy, backend);
    CHECK(rejected_body.features.size() == 1U);
    CHECK(rejected_body.features.front().status ==
        part::FeatureEvaluationStatus::blocked);
    CHECK(rejected_body.body_status !=
        part::BodyEvaluationStatus::up_to_date);
    CHECK(backend.calls == 5U);

    // The SAME Part Feature scheduler and SAME region resolver must also
    // execute a REAL OCCT full Revolve. The built-in Y axis lies outside
    // the 5..15 mm rectangle, so this is an annular, one-solid body.
    auto revolve_state = session.document().state();
    const auto revolve_id = revolve_state.body.next_feature_id.allocate();
    CHECK(revolve_id);
    revolve_state.body.features.push_back(part::PartFeature{
        *revolve_id,
        "R13StageRevolve",
        false,
        part::RevolveFeature{
            *profile.profile_id,
            part::AxisReference{
                part::BuiltinOriginAxisReference{
                    core::BuiltinReferenceRole::y_axis}},
            part::RevolveOperation::add,
            part::OneSidedRevolveExtent{
                core::AngleValue{2.0 * std::numbers::pi_v<double>},
                false}}});
    const auto revolve_document = part::PartDocument::restore(
        session.document().documentId(),
        std::move(revolve_state), session.document().revision());
    CHECK(revolve_document.ok() && revolve_document.document);
    const auto legacy_revolve =
        part::evaluatePart(*revolve_document.document, provider);
    CHECK(legacy_revolve.features.size() == 1U);
    CHECK(legacy_revolve.features.front().status ==
        part::FeatureEvaluationStatus::blocked);
    backend.seam_offset = 5.0e-13;
    const auto evaluated_revolve = part::evaluatePartWithLocalSolve(
        *revolve_document.document, provider, policy, backend);
    CHECK(evaluated_revolve.source_revision ==
        revolve_document.document->revision());
    CHECK(evaluated_revolve.body_status ==
        part::BodyEvaluationStatus::up_to_date);
    CHECK(evaluated_revolve.features.size() == 1U);
    CHECK(evaluated_revolve.features.front().feature_id == *revolve_id);
    CHECK(evaluated_revolve.features.front().status ==
        part::FeatureEvaluationStatus::up_to_date);
    CHECK(evaluated_revolve.features.front().result_topology);
    CHECK(evaluated_revolve.features.front().result_topology->complete());
    CHECK(evaluated_revolve.body_solid != nullptr);
    CHECK(evaluated_revolve.current_topology);
    CHECK(evaluated_revolve.current_topology->complete());
    CHECK(backend.calls == 6U);

    // Extrude's +2 mm X shift alone is insufficient: a full Revolve
    // around the built-in Y axis must change BOTH the annulus's inner
    // AND outer radii in the actual OCCT Body, not just report success.
    // Sample the provider's runtime presentation only as an independent
    // output-location witness; triangulation is NOT CAD identity and its
    // looser measurement tolerance never modifies the 1e-12 solver
    // acceptance budget or the exact Coincident vertex.
    const auto radial_bounds = [](const kernel::SolidPresentationMesh& mesh) {
        double inner = std::numeric_limits<double>::infinity();
        double outer = -std::numeric_limits<double>::infinity();
        for (const auto& triangle : mesh.triangles) {
            for (const auto& point :
                 {triangle.first, triangle.second, triangle.third}) {
                const double radius = std::hypot(point.x, point.z);
                inner = std::min(inner, radius);
                outer = std::max(outer, radius);
            }
        }
        return std::pair{inner, outer};
    };
    const auto base_revolve_mesh =
        provider.presentationMesh(evaluated_revolve.body_solid);
    CHECK(base_revolve_mesh.ok());
    const auto initial_radii = radial_bounds(base_revolve_mesh.mesh);
    CHECK(std::isfinite(initial_radii.first));
    CHECK(std::isfinite(initial_radii.second));
    CHECK(std::abs(initial_radii.first - 5.0) < 0.1);
    CHECK(std::abs(initial_radii.second - 15.0) < 0.1);

    // D2-P21-06: unlike a restored in-memory Part, this is a real native
    // v21 Save -> Close -> Reopen of a constrained Sketch/owned Profile
    // and a full Revolve Feature. Derived solver candidates, BRep and
    // topology are NOT persisted. This OCCT test has an explicit TEST-
    // ONLY ControlledSolver; original upstream PlaneGCS is exercised in
    // its separate independently gated Windows native evidence tests.
    {
        const auto authoredRevolve =
            revolve_document.document->findSketch(*sketch.sketch_id)
                ->model.state();
        CHECK(authoredRevolve.constraints.size() == 1U);
        CHECK(authoredRevolve.constraints.front().id ==
              *coincide.constraint_id);
        const auto coldPath = std::filesystem::temp_directory_path() /
            ("r13_a1_p21_native_revolve_" +
             std::string{revolve_document.document->documentId().value()} +
             ".ss2part");
        part::PartDocumentStore coldStore;
        const auto firstSave =
            coldStore.createNew(coldPath, *revolve_document.document);
        CHECK(firstSave.ok());
        auto cold = coldStore.load(coldPath);
        CHECK(cold.ok() && cold.document && cold.checkpoint);
        CHECK(cold.document->documentId() ==
              revolve_document.document->documentId());
        CHECK(cold.document->state() ==
              revolve_document.document->state());
        const auto* coldSketch =
            cold.document->findSketch(*sketch.sketch_id);
        CHECK(coldSketch != nullptr);
        CHECK(coldSketch->model.state() == authoredRevolve);
        CHECK(coldSketch->model.findConstraint(*coincide.constraint_id));
        CHECK(cold.document->findProfile(*profile.profile_id));
        CHECK(cold.document->state().body.features.size() == 1U);
        CHECK(cold.document->state().body.features.front().id == *revolve_id);

        occt::OcctSolidModelingKernel coldProvider;
        const auto noSolver =
            part::evaluatePart(*cold.document, coldProvider);
        CHECK(noSolver.features.size() == 1U);
        CHECK(noSolver.features.front().status ==
              part::FeatureEvaluationStatus::blocked);
        CHECK(noSolver.body_solid == nullptr);
        ControlledSolver coldSolver;
        const auto coldResult = part::evaluatePartWithLocalSolve(
            *cold.document, coldProvider, policy, coldSolver);
        CHECK(coldResult.source_document_id == cold.document->documentId());
        CHECK(coldResult.source_revision == cold.document->revision());
        CHECK(coldResult.features.size() == 1U);
        CHECK(coldResult.features.front().feature_id == *revolve_id);
        CHECK(coldResult.features.front().status ==
              part::FeatureEvaluationStatus::up_to_date);
        CHECK(coldResult.body_status ==
              part::BodyEvaluationStatus::up_to_date);
        CHECK(coldResult.body_solid != nullptr);
        CHECK(coldResult.current_topology);
        CHECK(coldResult.current_topology->complete());
        CHECK(coldSolver.calls == 1U);
        const auto coldMesh =
            coldProvider.presentationMesh(coldResult.body_solid);
        CHECK(coldMesh.ok());
        const auto coldRadii = radial_bounds(coldMesh.mesh);
        CHECK(std::abs(coldRadii.first - initial_radii.first) < 0.1);
        CHECK(std::abs(coldRadii.second - initial_radii.second) < 0.1);
        CHECK(cold.document->findSketch(*sketch.sketch_id)
            ->model.state() == authoredRevolve);

        // A second explicit Save and independent Load must not persist
        // either previous OCCT body or the numerical solved endpoints.
        CHECK(coldStore.save(
            coldPath, *cold.document, *cold.checkpoint).ok());
        const auto again = coldStore.load(coldPath);
        CHECK(again.ok() && again.document);
        CHECK(again.document->documentId() ==
              revolve_document.document->documentId());
        CHECK(again.document->state() ==
              revolve_document.document->state());
        CHECK(again.document->findSketch(*sketch.sketch_id)
            ->model.state() == authoredRevolve);
        occt::OcctSolidModelingKernel againProvider;
        ControlledSolver againSolver;
        const auto recomputed = part::evaluatePartWithLocalSolve(
            *again.document, againProvider, policy, againSolver);
        CHECK(recomputed.features.size() == 1U);
        CHECK(recomputed.features.front().feature_id == *revolve_id);
        CHECK(recomputed.features.front().status ==
              part::FeatureEvaluationStatus::up_to_date);
        CHECK(recomputed.body_status ==
              part::BodyEvaluationStatus::up_to_date);
        CHECK(recomputed.body_solid != nullptr);
        CHECK(recomputed.current_topology);
        CHECK(recomputed.current_topology->complete());
        CHECK(againSolver.calls == 1U);
        CHECK(std::filesystem::remove(coldPath));
    }

    backend.translated_u = 2.0;
    const auto shifted_revolve = part::evaluatePartWithLocalSolve(
        *revolve_document.document, provider, policy, backend);
    CHECK(shifted_revolve.body_status ==
        part::BodyEvaluationStatus::up_to_date);
    CHECK(shifted_revolve.features.size() == 1U);
    CHECK(shifted_revolve.features.front().feature_id == *revolve_id);
    CHECK(shifted_revolve.features.front().status ==
        part::FeatureEvaluationStatus::up_to_date);
    CHECK(shifted_revolve.body_solid != nullptr);
    CHECK(shifted_revolve.current_topology);
    CHECK(shifted_revolve.current_topology->complete());
    const auto shifted_revolve_mesh =
        provider.presentationMesh(shifted_revolve.body_solid);
    CHECK(shifted_revolve_mesh.ok());
    const auto derived_radii = radial_bounds(shifted_revolve_mesh.mesh);
    CHECK(std::abs(derived_radii.first -
        initial_radii.first - 2.0) < 0.1);
    CHECK(std::abs(derived_radii.second -
        initial_radii.second - 2.0) < 0.1);
    CHECK(backend.calls == 7U);
    CHECK(revolve_document.document->findSketch(*sketch.sketch_id)
        ->model.state().lines == seed.lines);
    backend.translated_u = 0.0;
    backend.seam_offset = 5.0e-12;
    const auto rejected_revolve = part::evaluatePartWithLocalSolve(
        *revolve_document.document, provider, policy, backend);
    CHECK(rejected_revolve.features.size() == 1U);
    CHECK(rejected_revolve.features.front().status ==
        part::FeatureEvaluationStatus::blocked);
    CHECK(rejected_revolve.body_status !=
        part::BodyEvaluationStatus::up_to_date);
    CHECK(backend.calls == 8U);

    // Two scheduled Feature stages in ONE native Part evaluation:
    // first Extrude uses an ordinary unconstrained Profile; the second
    // is a constrained inner rectangle cut through the live upstream
    // OCCT solid. Only the second stage is authorized to call the
    // numerical backend. Rejecting that solve may not publish the
    // downstream Cut, erase the valid prefix or mutate either Sketch.
    {
        app::DocumentSession chain_session{
            {}, part::PartDocument::create(core::DocumentId::generate())};
        const auto parent_sketch = chain_session.execute(
            app::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
        CHECK(parent_sketch.ok() && parent_sketch.sketch_id);
        const auto parent_rectangle = chain_session.execute(
            app::AddSketchRectangleCommand{
                *parent_sketch.sketch_id,
                chain_session.document().revision(),
                {5.0, 0.0}, {15.0, 8.0},
                sk::EntityRole::regular, false});
        CHECK(parent_rectangle.ok());
        const auto parent_regions = sk::analyzeRegions(
            chain_session.document().findSketch(
                *parent_sketch.sketch_id)->model);
        CHECK(parent_regions.complete());
        CHECK(parent_regions.regions.size() == 1U);
        const auto parent_intent =
            part::makeProfileRegionIntent(parent_regions.regions.front());
        CHECK(parent_intent);
        const auto parent_profile = chain_session.execute(
            app::CreateProfileCommand{
                *parent_sketch.sketch_id,
                chain_session.document().revision(), *parent_intent});
        CHECK(parent_profile.ok() && parent_profile.profile_id);

        const auto cut_sketch = chain_session.execute(
            app::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
        CHECK(cut_sketch.ok() && cut_sketch.sketch_id);
        const auto cut_rectangle = chain_session.execute(
            app::AddSketchRectangleCommand{
                *cut_sketch.sketch_id,
                chain_session.document().revision(),
                {7.0, 2.0}, {9.0, 6.0},
                sk::EntityRole::regular, false});
        CHECK(cut_rectangle.ok());
        const auto cut_model = chain_session.document()
            .findSketch(*cut_sketch.sketch_id)->model.state();
        CHECK(cut_model.lines.size() == 4U);
        const auto cut_regions = sk::analyzeRegions(
            chain_session.document().findSketch(
                *cut_sketch.sketch_id)->model);
        CHECK(cut_regions.complete());
        CHECK(cut_regions.regions.size() == 1U);
        const auto cut_intent =
            part::makeProfileRegionIntent(cut_regions.regions.front());
        CHECK(cut_intent);
        const auto cut_profile = chain_session.execute(
            app::CreateProfileCommand{
                *cut_sketch.sketch_id,
                chain_session.document().revision(), *cut_intent});
        CHECK(cut_profile.ok() && cut_profile.profile_id);
        const auto cut_constraint = chain_session.execute(
            app::AddSketchRelationCommand{
                *cut_sketch.sketch_id,
                chain_session.document().revision(),
                sk::ConstraintKind::coincident,
                {cut_model.lines[0].id, sk::ConstraintElement::line_end},
                {cut_model.lines[1].id, sk::ConstraintElement::line_start}});
        CHECK(cut_constraint.ok() && cut_constraint.constraint_id);

        auto chained_state = chain_session.document().state();
        const auto parent_feature_id =
            chained_state.body.next_feature_id.allocate();
        const auto cut_feature_id =
            chained_state.body.next_feature_id.allocate();
        CHECK(parent_feature_id && cut_feature_id);
        chained_state.body.features.push_back(part::PartFeature{
            *parent_feature_id, "NativePrefixExtrude", false,
            part::ExtrudeFeature{
                *parent_profile.profile_id, part::ExtrudeOperation::add,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{10.0}, false}}});
        chained_state.body.features.push_back(part::PartFeature{
            *cut_feature_id, "NativeStageCut", false,
            part::ExtrudeFeature{
                *cut_profile.profile_id, part::ExtrudeOperation::cut,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{10.0}, false}}});
        const auto chained_document = part::PartDocument::restore(
            chain_session.document().documentId(),
            std::move(chained_state),
            chain_session.document().revision());
        CHECK(chained_document.ok() && chained_document.document);
        const auto& authored_chain = *chained_document.document;
        const auto legacy_chain = part::evaluatePart(authored_chain, provider);
        CHECK(legacy_chain.features.size() == 2U);
        CHECK(legacy_chain.features[0].feature_id == *parent_feature_id);
        CHECK(legacy_chain.features[0].status ==
            part::FeatureEvaluationStatus::up_to_date);
        CHECK(legacy_chain.features[1].feature_id == *cut_feature_id);
        CHECK(legacy_chain.features[1].status ==
            part::FeatureEvaluationStatus::blocked);
        CHECK(legacy_chain.body_status !=
            part::BodyEvaluationStatus::up_to_date);
        CHECK(legacy_chain.resolved_prefix_solid != nullptr);
        CHECK(legacy_chain.resolved_prefix_topology);
        CHECK(legacy_chain.resolved_prefix_topology->complete());

        ControlledSolver chain_backend;
        const auto solved_chain = part::evaluatePartWithLocalSolve(
            authored_chain, provider, policy, chain_backend);
        CHECK(solved_chain.source_document_id == authored_chain.documentId());
        CHECK(solved_chain.source_revision == authored_chain.revision());
        CHECK(solved_chain.body_status ==
            part::BodyEvaluationStatus::up_to_date);
        CHECK(solved_chain.features.size() == 2U);
        CHECK(solved_chain.features[0].feature_id == *parent_feature_id);
        CHECK(solved_chain.features[0].status ==
            part::FeatureEvaluationStatus::up_to_date);
        CHECK(solved_chain.features[1].feature_id == *cut_feature_id);
        CHECK(solved_chain.features[1].status ==
            part::FeatureEvaluationStatus::up_to_date);
        CHECK(solved_chain.features[1].result_topology);
        CHECK(solved_chain.features[1].result_topology->complete());
        CHECK(solved_chain.current_topology);
        CHECK(solved_chain.current_topology->complete());
        CHECK(solved_chain.body_solid != nullptr);
        CHECK(chain_backend.calls == 1U);
        const auto cut_mesh = provider.presentationMesh(
            solved_chain.body_solid);
        CHECK(cut_mesh.ok());
        bool has_cut_inner_corner = false;
        for (const auto& triangle : cut_mesh.mesh.triangles) {
            for (const auto& point :
                 {triangle.first, triangle.second, triangle.third}) {
                has_cut_inner_corner |=
                    std::abs(point.x - 7.0) < 0.01 &&
                    std::abs(point.y - 2.0) < 0.01;
            }
        }
        CHECK(has_cut_inner_corner);

        chain_backend.seam_offset = 5.0e-12;
        const auto bad_chain = part::evaluatePartWithLocalSolve(
            authored_chain, provider, policy, chain_backend);
        CHECK(bad_chain.features.size() == 2U);
        CHECK(bad_chain.features[0].status ==
            part::FeatureEvaluationStatus::up_to_date);
        CHECK(bad_chain.features[1].status ==
            part::FeatureEvaluationStatus::blocked);
        CHECK(bad_chain.body_status !=
            part::BodyEvaluationStatus::up_to_date);
        CHECK(bad_chain.body_solid == nullptr);
        CHECK(bad_chain.current_topology == std::nullopt);
        CHECK(bad_chain.resolved_prefix_solid != nullptr);
        CHECK(bad_chain.resolved_prefix_topology);
        CHECK(bad_chain.resolved_prefix_topology->complete());
        CHECK(bad_chain.resolved_prefix_topology->stage.feature_id ==
            *parent_feature_id);
        CHECK(chain_backend.calls == 2U);
        CHECK(authored_chain.findSketch(*cut_sketch.sketch_id)
            ->model.state().lines == cut_model.lines);
        CHECK(authored_chain.findSketch(*cut_sketch.sketch_id)
            ->model.state().constraints.size() == 1U);
        CHECK(authored_chain.findSketch(*cut_sketch.sketch_id)
            ->model.state().constraints.front().id ==
            *cut_constraint.constraint_id);
        CHECK(authored_chain.findProfile(*parent_profile.profile_id));
        CHECK(authored_chain.findProfile(*cut_profile.profile_id));

        // A legitimate PG-01 projected Line triggers an external provider call
    // *inside* prepareStageLocalSolveProblem. Reentering DocumentSession
    // through that provider must not leave any borrowed authored pointers
    // dangling or allow an obsolete problem to escape standalone prepare.
    {
        auto cloned = part::PartDocument::restore(
            authored_chain.documentId(), authored_chain.state(),
            authored_chain.revision());
        CHECK(cloned.ok() && cloned.document);
        app::DocumentSession projected_session{
            {}, std::move(*cloned.document)};
        const auto projected_sketch = projected_session.execute(
            app::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
        CHECK(projected_sketch.ok() && projected_sketch.sketch_id);
        auto current_prefix = part::evaluatePart(
            projected_session.document(), provider);
        CHECK(current_prefix.features.size() == 2U);
        CHECK(current_prefix.features.front().status ==
            part::FeatureEvaluationStatus::up_to_date);
        CHECK(current_prefix.features.front().result_solid);
        CHECK(current_prefix.features.front().result_topology);
        const auto frame = part::resolveCurrentProjectionSketchFrame(
            projected_session.document(), *projected_sketch.sketch_id,
            current_prefix);
        CHECK(frame);

        std::optional<part::MaterialEdgeReference> source;
        for (const auto& edge :
             current_prefix.features.front().result_topology->edges) {
            const auto material = part::authorMaterialEdgeReference(
                *current_prefix.features.front().result_topology,
                edge.runtime_token);
            if (!material.ok()) continue;
            const auto bound = provider.bindEdgeToBody(
                current_prefix.features.front().result_solid,
                edge.runtime_token);
            if (!bound) continue;
            const auto projected_edge = provider.projectEdgeToPlane(
                current_prefix.features.front().result_solid,
                *bound, *frame);
            if (projected_edge.ok() &&
                std::holds_alternative<kernel::Line2>(
                    *projected_edge.curve)) {
                source = *material.reference;
                break;
            }
        }
        CHECK(source);
        const auto linked = projected_session.execute(
            app::CreateProjectedSketchEdgesCommand{
                *projected_sketch.sketch_id,
                projected_session.document().revision(), {*source},
                sk::EntityRole::regular},
            provider);
        CHECK(linked.ok());
        CHECK(linked.entity_ids.size() == 1U);
        current_prefix = part::evaluatePart(
            projected_session.document(), provider);
        const auto valid_problem = part::prepareStageLocalSolveProblem(
            projected_session.document(), *projected_sketch.sketch_id,
            current_prefix, provider);
        CHECK(valid_problem);
        CHECK(valid_problem->fixed_lines.size() == 1U);
        CHECK(valid_problem->fixed_lines.front() == linked.entity_ids.front());

        // P21-08 current-stage PG-01: persist the SAME real OCCT Edge
        // binding with an authored A1 relation on a different local Line,
        // then re-evaluate with a NEW provider after native cold reopen.
        // A deliberately stale projected seed makes any fallback visible.
        {
            auto coldState = projected_session.document().state();
            const auto target = std::find_if(
                coldState.sketches.begin(), coldState.sketches.end(),
                [&](const part::PartSketch& item) {
                    return item.id == *projected_sketch.sketch_id;
                });
            CHECK(target != coldState.sketches.end());
            CHECK(target->projection_bindings.size() == 1U);
            const auto oldBindings = target->projection_bindings;
            const auto projectedId = linked.entity_ids.front();
            CHECK(target->model.updateLine(
                projectedId, {180.0, 180.0}, {190.0, 180.0}));
            auto restoredDoc = part::PartDocument::restore(
                projected_session.document().documentId(),
                std::move(coldState),
                projected_session.document().revision());
            CHECK(restoredDoc.ok() && restoredDoc.document);
            app::DocumentSession authoredPg{
                {}, std::move(*restoredDoc.document)};
            const auto userLine = authoredPg.execute(
                app::AddSketchLineCommand{
                    *projected_sketch.sketch_id,
                    {21.0, 22.0}, {27.0, 23.0}});
            CHECK(userLine.ok() && userLine.entity_id);
            const auto relation = authoredPg.execute(
                app::AddHorizontalSketchConstraintCommand{
                    *projected_sketch.sketch_id,
                    authoredPg.document().revision(), *userLine.entity_id});
            CHECK(relation.ok() && relation.constraint_id);
            const auto* authoredPgSketch = authoredPg.document().findSketch(
                *projected_sketch.sketch_id);
            CHECK(authoredPgSketch != nullptr);
            CHECK(authoredPgSketch->projection_bindings == oldBindings);
            const auto authoredPgState = authoredPg.document().state();
            const auto pgPath =
                std::filesystem::temp_directory_path() /
                ("r13a1_p21_pg01_stage_" +
                 std::string{authoredPg.document().documentId().value()} +
                 ".ss2part");
            part::PartDocumentStore pgStore;
            CHECK(pgStore.createNew(pgPath, authoredPg.document()).ok());
            const auto pgCold = pgStore.load(pgPath);
            CHECK(pgCold.ok() && pgCold.document);
            CHECK(pgCold.document->documentId() ==
                  authoredPg.document().documentId());
            CHECK(pgCold.document->state() == authoredPgState);
            const auto* coldPgSketch = pgCold.document->findSketch(
                *projected_sketch.sketch_id);
            CHECK(coldPgSketch != nullptr);
            CHECK(coldPgSketch->projection_bindings == oldBindings);
            CHECK(coldPgSketch->model.findConstraint(
                *relation.constraint_id) != nullptr);
            CHECK(coldPgSketch->model.constraintIdCursor().serialized() ==
                  "2");
            CHECK(coldPgSketch->model.findLine(projectedId)->start() ==
                  (sk::Point2{180.0, 180.0}));

            // Fresh OCCT ownership and runtime Edge tokens: only the new
            // evaluated prefix at the cold document's exact revision can
            // resolve these durable PG-01 material semantic references.
            occt::OcctSolidModelingKernel freshPgProvider;
            const auto freshPrefix =
                part::evaluatePart(*pgCold.document, freshPgProvider);
            CHECK(freshPrefix.source_revision ==
                  pgCold.document->revision());
            CHECK(freshPrefix.features.size() == 2U);
            CHECK(freshPrefix.features.front().status ==
                  part::FeatureEvaluationStatus::up_to_date);
            CHECK(freshPrefix.features.front().result_solid);
            CHECK(!part::prepareStageLocalSolveProblem(
                *pgCold.document, *projected_sketch.sketch_id,
                current_prefix, freshPgProvider));
            auto foreign = freshPrefix;
            foreign.source_document_id = core::DocumentId::generate();
            CHECK(!part::prepareStageLocalSolveProblem(
                *pgCold.document, *projected_sketch.sketch_id,
                foreign, freshPgProvider));
            auto stale = freshPrefix;
            // Cold Part revision is zero: default revision is CURRENT,
            // not stale. Use an explicitly different numeric revision.
            stale.source_revision = core::DocumentRevision{
                std::numeric_limits<std::uint64_t>::max()};
            CHECK(stale.source_revision != pgCold.document->revision());
            CHECK(!part::prepareStageLocalSolveProblem(
                *pgCold.document, *projected_sketch.sketch_id,
                stale, freshPgProvider));
            const auto effective = part::evaluateEffectiveSketchProjection(
                *pgCold.document, *projected_sketch.sketch_id,
                freshPrefix, freshPgProvider);
            CHECK(effective && effective->allResolved());
            CHECK(effective->outcomes.size() == 1U);
            CHECK(effective->outcomes.front().target_entity == projectedId);
            const auto effectiveLine = effective->model.findLine(projectedId);
            CHECK(effectiveLine);
            CHECK(effectiveLine->start() !=
                  (sk::Point2{180.0, 180.0}));

            const auto freshProblem = part::prepareStageLocalSolveProblem(
                *pgCold.document, *projected_sketch.sketch_id,
                freshPrefix, freshPgProvider);
            CHECK(freshProblem);
            // The solver receives disposable CURRENT projected geometry,
            // not the stale authored Line seed saved in the native file.
            // The immutable Part source retains its own complete seed state.
            CHECK(freshProblem->authored == effective->model.state());
            CHECK(freshProblem->authored != coldPgSketch->model.state());
            CHECK(coldPgSketch->model.findLine(projectedId)->start() ==
                  (sk::Point2{180.0, 180.0}));
            CHECK(freshProblem->lines.size() == 2U);
            CHECK(freshProblem->fixed_lines.size() == 1U);
            CHECK(freshProblem->fixed_lines.front() == projectedId);
            const auto projectedSolverLine = std::find_if(
                freshProblem->lines.cbegin(), freshProblem->lines.cend(),
                [projectedId](const sk::LocalSolveLineInput& line) {
                    return line.id == projectedId;
                });
            CHECK(projectedSolverLine != freshProblem->lines.cend());
            CHECK(projectedSolverLine->fixed);
            CHECK(projectedSolverLine->start == effectiveLine->start());
            CHECK(projectedSolverLine->end == effectiveLine->end());
            CHECK(freshProblem->constraints.size() == 1U);
            CHECK(freshProblem->constraints.front().id ==
                  *relation.constraint_id);
            CHECK(freshProblem->constraints.front().kind ==
                  sk::ConstraintKind::horizontal);
            CHECK(freshProblem->equations.size() == 1U);
            CHECK(freshProblem->equations.front().first.entity ==
                  *userLine.entity_id);
            CHECK(coldPgSketch->model.findLine(projectedId)->start() ==
                  (sk::Point2{180.0, 180.0}));

            // P21-08 negative cold-stage controls: even on an EXACT
            // source document/revision and a valid persisted binding,
            // the runtime OCCT provider can lose the scoped Edge while
            // binding or refuse the geometric projection afterwards.
            // Neither path may substitute authored seed coordinates,
            // permit solver admission or change the durable model/IDs.
            struct DenyingColdProjection final
                : kernel::IEdgeProjectionQuery {
                occt::OcctSolidModelingKernel& native;
                bool deny_on_bind{};
                unsigned binds{};
                unsigned projections{};

                DenyingColdProjection(
                    occt::OcctSolidModelingKernel& provider,
                    bool deny_binding)
                    : native{provider}, deny_on_bind{deny_binding} {}

                std::optional<kernel::ScopedProjectionEdge> bindEdgeToBody(
                    kernel::RuntimeSolidHandle solid,
                    kernel::RuntimeEdgeToken token) noexcept override {
                    ++binds;
                    if (deny_on_bind) return std::nullopt;
                    return native.bindEdgeToBody(std::move(solid), token);
                }

                kernel::EdgeProjectionResult projectEdgeToPlane(
                    kernel::RuntimeSolidHandle,
                    const kernel::ScopedProjectionEdge&,
                    const kernel::Frame3&) noexcept override {
                    ++projections;
                    // Return a typed provider failure, not a stale curve
                    // masquerading as live OCCT geometry.
                    return {};
                }
            };
            const auto authoredBeforeFailure = coldPgSketch->model.state();
            const auto partBeforeFailure = pgCold.document->state();
            DenyingColdProjection unavailable{
                freshPgProvider, true};
            const auto noBinding = part::evaluateEffectiveSketchProjection(
                *pgCold.document, *projected_sketch.sketch_id,
                freshPrefix, unavailable);
            CHECK(!noBinding || !noBinding->allResolved());
            CHECK(unavailable.binds == 1U);
            CHECK(unavailable.projections == 0U);
            CHECK(!part::prepareStageLocalSolveProblem(
                *pgCold.document, *projected_sketch.sketch_id,
                freshPrefix, unavailable));
            CHECK(unavailable.projections == 0U);

            DenyingColdProjection cannotProject{
                freshPgProvider, false};
            const auto noProjection = part::evaluateEffectiveSketchProjection(
                *pgCold.document, *projected_sketch.sketch_id,
                freshPrefix, cannotProject);
            CHECK(!noProjection || !noProjection->allResolved());
            CHECK(cannotProject.binds == 1U);
            CHECK(cannotProject.projections == 1U);
            CHECK(!part::prepareStageLocalSolveProblem(
                *pgCold.document, *projected_sketch.sketch_id,
                freshPrefix, cannotProject));
            CHECK(cannotProject.projections >= 2U);
            CHECK(coldPgSketch->model.state() == authoredBeforeFailure);
            CHECK(pgCold.document->state() == partBeforeFailure);
            CHECK(coldPgSketch->model.findConstraint(
                *relation.constraint_id) != nullptr);
            CHECK(coldPgSketch->model.findLine(projectedId)->start() ==
                  (sk::Point2{180.0, 180.0}));

            // Once the live provider is available again on the SAME
            // revision, recompute succeeds without repairing the seed.
            const auto afterProviderRecovery =
                part::prepareStageLocalSolveProblem(
                    *pgCold.document, *projected_sketch.sketch_id,
                    freshPrefix, freshPgProvider);
            CHECK(afterProviderRecovery);
            CHECK(afterProviderRecovery->fixed_lines.size() == 1U);
            CHECK(afterProviderRecovery->fixed_lines.front() == projectedId);
            CHECK(afterProviderRecovery->constraints.size() == 1U);
            CHECK(afterProviderRecovery->constraints.front().id ==
                  *relation.constraint_id);
            CHECK(pgCold.document->state() == partBeforeFailure);
            CHECK(std::filesystem::remove(pgPath));
        }

        struct ReentrantQuery final : kernel::IEdgeProjectionQuery {
            occt::OcctSolidModelingKernel& native;
            app::DocumentSession& session;
            sk::SketchId sketch_id;
            unsigned bindings{};
            unsigned projections{};
            bool mutated{};
            bool mutate_on_bind{};

            ReentrantQuery(
                occt::OcctSolidModelingKernel& native_provider,
                app::DocumentSession& owner,
                sk::SketchId target_id)
                : native{native_provider}, session{owner},
                  sketch_id{target_id} {}

            std::optional<kernel::ScopedProjectionEdge> bindEdgeToBody(
                kernel::RuntimeSolidHandle body,
                kernel::RuntimeEdgeToken edge) noexcept override {
                ++bindings;
                auto bound = native.bindEdgeToBody(
                    std::move(body), edge);
                if (mutate_on_bind && bound && bound->valid()) {
                    const auto edit = session.execute(
                        app::AddSketchLineCommand{
                            sketch_id, {27.0, 2.0}, {31.0, 2.0}});
                    CHECK(edit.ok() && edit.changed);
                    mutated = true;
                }
                return bound;
            }
            kernel::EdgeProjectionResult projectEdgeToPlane(
                kernel::RuntimeSolidHandle body,
                const kernel::ScopedProjectionEdge& bound,
                const kernel::Frame3& target) noexcept override {
                ++projections;
                auto value = native.projectEdgeToPlane(
                    std::move(body), bound, target);
                if (value.ok()) {
                    const auto edit = session.execute(
                        app::AddSketchLineCommand{
                            sketch_id, {21.0, 2.0}, {24.0, 2.0}});
                    CHECK(edit.ok() && edit.changed);
                    mutated = true;
                }
                return value;
            }
        } reentrant{provider, projected_session,
                    *projected_sketch.sketch_id};

        CHECK(!part::prepareStageLocalSolveProblem(
            projected_session.document(), *projected_sketch.sketch_id,
            current_prefix, reentrant));
        CHECK(reentrant.projections == 1U);
        CHECK(reentrant.mutated);
        CHECK(current_prefix.source_revision !=
            projected_session.document().revision());
        CHECK(!part::prepareStageLocalSolveProblem(
            projected_session.document(), *projected_sketch.sketch_id,
            current_prefix, provider));
        current_prefix = part::evaluatePart(
            projected_session.document(), provider);
        const auto restored = part::prepareStageLocalSolveProblem(
            projected_session.document(), *projected_sketch.sketch_id,
            current_prefix, provider);
        CHECK(restored && restored->lines.size() == 2U);
        CHECK(restored->fixed_lines.size() == 1U);

        // A mutation while BINDING the real native Edge must abort
        // BEFORE the second projection callback. Even a valid scoped
        // OCCT edge cannot authorize use of an obsolete Part revision.
        ReentrantQuery bind_reentrant{
            provider, projected_session, *projected_sketch.sketch_id};
        bind_reentrant.mutate_on_bind = true;
        CHECK(!part::prepareStageLocalSolveProblem(
            projected_session.document(), *projected_sketch.sketch_id,
            current_prefix, bind_reentrant));
        CHECK(bind_reentrant.bindings == 1U);
        CHECK(bind_reentrant.projections == 0U);
        CHECK(bind_reentrant.mutated);
        CHECK(current_prefix.source_revision !=
            projected_session.document().revision());
        const auto after_binding_edit = part::evaluatePart(
            projected_session.document(), provider);
        const auto accepted_again = part::prepareStageLocalSolveProblem(
            projected_session.document(), *projected_sketch.sketch_id,
            after_binding_edit, provider);
        CHECK(accepted_again && accepted_again->lines.size() == 3U);
        CHECK(accepted_again->fixed_lines.size() == 1U);

        // The DOCUMENT SESSION command is a different publication
        // boundary from prepareStageLocalSolveProblem. An older outer
        // Project Edge request must not overwrite the newer document
        // committed by an adversarial callback during the native query.
        const auto command_sketch = projected_session.execute(
            app::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
        CHECK(command_sketch.ok() && command_sketch.sketch_id);
        const app::CreateProjectedSketchEdgesCommand request{
            *command_sketch.sketch_id,
            projected_session.document().revision(), {*source},
            sk::EntityRole::regular};

        struct CommandReentrantKernel final
            : kernel::ISolidModelingKernel,
              kernel::IEdgeProjectionQuery {
            enum class Hook { bind, project };

            occt::OcctSolidModelingKernel& native;
            app::DocumentSession& session;
            sk::SketchId sketch_id;
            Hook hook;
            sk::EntityRole nested_role{sk::EntityRole::regular};
            unsigned binds{};
            unsigned projections{};
            bool mutated{};

            CommandReentrantKernel(
                occt::OcctSolidModelingKernel& provider,
                app::DocumentSession& owner,
                sk::SketchId target,
                Hook phase)
                : native{provider}, session{owner},
                  sketch_id{target}, hook{phase} {}

            kernel::SolidModelingResult extrude(
                const kernel::LinearExtrudeInput& input,
                kernel::RuntimeSolidHandle upstream = {}) noexcept override {
                return native.extrude(input, std::move(upstream));
            }
            kernel::SolidModelingResult revolve(
                const kernel::AngularRevolveInput& input,
                kernel::RuntimeSolidHandle upstream = {}) noexcept override {
                return native.revolve(input, std::move(upstream));
            }
            kernel::SolidModelingResult edgeFeature(
                const kernel::EdgeFeatureInput& input,
                kernel::RuntimeSolidHandle upstream = {}) noexcept override {
                return native.edgeFeature(input, std::move(upstream));
            }
            kernel::BodyPresentationResult bodyPresentation(
                kernel::RuntimeSolidHandle body) noexcept override {
                return native.bodyPresentation(std::move(body));
            }
            kernel::SolidPresentationResult presentationMesh(
                kernel::RuntimeSolidHandle body) noexcept override {
                return native.presentationMesh(std::move(body));
            }
            void editNested() noexcept {
                CHECK(!mutated);
                const auto edit = session.execute(
                    app::AddSketchLineCommand{
                        sketch_id, {27.0, 3.0}, {30.0, 3.0},
                        nested_role});
                CHECK(edit.ok() && edit.changed);
                mutated = true;
            }
            std::optional<kernel::ScopedProjectionEdge> bindEdgeToBody(
                kernel::RuntimeSolidHandle body,
                kernel::RuntimeEdgeToken edge) noexcept override {
                ++binds;
                auto bound = native.bindEdgeToBody(
                    std::move(body), edge);
                if (hook == Hook::bind && bound && bound->valid()) {
                    editNested();
                }
                return bound;
            }
            kernel::EdgeProjectionResult projectEdgeToPlane(
                kernel::RuntimeSolidHandle body,
                const kernel::ScopedProjectionEdge& bound,
                const kernel::Frame3& target) noexcept override {
                ++projections;
                auto result = native.projectEdgeToPlane(
                    std::move(body), bound, target);
                if (hook == Hook::project && result.ok()) {
                    editNested();
                }
                return result;
            }
        };

        // First, reenter at the scoped binding step. The second provider
        // hook must never run. The nested edit survives; the old outer
        // command must not create a binding or consume a second Undo.
        const auto baseline_undo = projected_session.undoDepth();
        CommandReentrantKernel bind_command{
            provider, projected_session, *command_sketch.sketch_id,
            CommandReentrantKernel::Hook::bind};
        const auto stale_bind = projected_session.execute(
            request, bind_command);
        CHECK(!stale_bind.ok() && !stale_bind.changed);
        CHECK(stale_bind.diagnostic.code ==
            app::DocumentSessionErrorCode::revision_diverged);
        CHECK(bind_command.mutated && bind_command.binds == 1U);
        CHECK(bind_command.projections == 0U);
        CHECK(projected_session.undoDepth() == baseline_undo + 1U);
        const auto* first_after =
            projected_session.document().findSketch(
                *command_sketch.sketch_id);
        CHECK(first_after != nullptr);
        CHECK(first_after->model.state().lines.size() == 1U);
        CHECK(first_after->projection_bindings.empty());

        // The next valid request uses the newly committed revision.
        // Reenter DURING the geometric projection this time; again no
        // obsolete outer commit, no lost nested edit or invented history.
        const app::CreateProjectedSketchEdgesCommand next_request{
            *command_sketch.sketch_id,
            projected_session.document().revision(), {*source},
            sk::EntityRole::regular};
        const auto second_undo = projected_session.undoDepth();
        CommandReentrantKernel project_command{
            provider, projected_session, *command_sketch.sketch_id,
            CommandReentrantKernel::Hook::project};
        const auto stale_project = projected_session.execute(
            next_request, project_command);
        CHECK(!stale_project.ok() && !stale_project.changed);
        CHECK(stale_project.diagnostic.code ==
            app::DocumentSessionErrorCode::revision_diverged);
        CHECK(project_command.mutated &&
            project_command.binds == 1U &&
            project_command.projections == 1U);
        CHECK(projected_session.undoDepth() == second_undo + 1U);
        const auto* second_after =
            projected_session.document().findSketch(
                *command_sketch.sketch_id);
        CHECK(second_after != nullptr);
        CHECK(second_after->model.state().lines.size() == 2U);
        CHECK(second_after->projection_bindings.empty());

        // A normal, same-revision native OCCT call must still commit
        // exactly one target/binding as one Undoable transaction.
        const auto clean_revision = projected_session.document().revision();
        const auto clean_undo = projected_session.undoDepth();
        const auto clean_project = projected_session.execute(
            app::CreateProjectedSketchEdgesCommand{
                *command_sketch.sketch_id, clean_revision, {*source},
                sk::EntityRole::regular},
            provider);
        CHECK(clean_project.ok() && clean_project.changed);
        CHECK(clean_project.entity_ids.size() == 1U);
        CHECK(projected_session.undoDepth() == clean_undo + 1U);
        const auto* clean_after =
            projected_session.document().findSketch(
                *command_sketch.sketch_id);
        CHECK(clean_after != nullptr);
        CHECK(clean_after->model.state().lines.size() == 3U);
        CHECK(clean_after->projection_bindings.size() == 1U);
        CHECK(clean_after->projection_bindings.front().target_entity ==
            clean_project.entity_ids.front());

        // Break Link is ALSO a revision-bound provider command. The
        // recompute may reenter through native BIND or PROJECT. Its older
        // outer transaction must not detach the live link, erase a nested
        // Line or consume another Undo slot. Both hooks are exercised on
        // real OCCT-backed geometry, not artificial pick tokens.
        const auto linked_id = clean_project.entity_ids.front();
        const auto check_stale_break = [&](CommandReentrantKernel::Hook hook) {
            const auto expected = projected_session.document().revision();
            const auto undo_before = projected_session.undoDepth();
            const auto* before = projected_session.document().findSketch(
                *command_sketch.sketch_id);
            CHECK(before != nullptr);
            const auto line_count = before->model.state().lines.size();
            CommandReentrantKernel reentry{
                provider, projected_session, *command_sketch.sketch_id,
                hook};
            const auto broken = projected_session.execute(
                app::BreakProjectedEdgeLinksCommand{
                    *command_sketch.sketch_id, {linked_id}, expected},
                reentry);
            CHECK(!broken.ok() && !broken.changed);
            CHECK(broken.status ==
                app::BreakProjectedEdgeLinkStatus::stale_revision);
            CHECK(reentry.mutated && reentry.binds == 1U);
            CHECK(reentry.projections ==
                (hook == CommandReentrantKernel::Hook::bind ? 0U : 1U));
            CHECK(projected_session.undoDepth() == undo_before + 1U);
            const auto* live = projected_session.document().findSketch(
                *command_sketch.sketch_id);
            CHECK(live != nullptr);
            CHECK(live->model.state().lines.size() == line_count + 1U);
            CHECK(live->projection_bindings.size() == 1U);
            CHECK(live->projection_bindings.front().target_entity == linked_id);
        };
        check_stale_break(CommandReentrantKernel::Hook::bind);
        check_stale_break(CommandReentrantKernel::Hook::project);

        // A fresh ordinary native Break Link remains exactly one atomic
        // Undoable edit. It retains the latest resolved local Line/EntityId
        // while removing only its external PG-01 dependency.
        const auto final_undo = projected_session.undoDepth();
        const auto* before_break = projected_session.document().findSketch(
            *command_sketch.sketch_id);
        CHECK(before_break != nullptr);
        const auto final_lines = before_break->model.state().lines.size();
        const auto normal_break = projected_session.execute(
            app::BreakProjectedEdgeLinksCommand{
                *command_sketch.sketch_id, {linked_id},
                projected_session.document().revision()},
            provider);
        CHECK(normal_break.ok() && normal_break.changed);
        CHECK(projected_session.undoDepth() == final_undo + 1U);
        const auto* detached = projected_session.document().findSketch(
            *command_sketch.sketch_id);
        CHECK(detached != nullptr);
        CHECK(detached->projection_bindings.empty());
        CHECK(detached->model.findLine(linked_id) != nullptr);
        CHECK(detached->model.state().lines.size() == final_lines);

        // PG-01 Create/Edit Profile are revision-bound semantic Commands
        // too. A provider callback may reenter the session after the
        // commands captured their prospective PartAuthoredState. A stale
        // original command must return revision_diverged, not silently
        // report an unrelated broken RegionIntent or publish obsolete
        // Profile state over the genuine nested edit.
        const auto profile_sketch = projected_session.execute(
            app::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
        CHECK(profile_sketch.ok() && profile_sketch.sketch_id);
        const auto profile_linked = projected_session.execute(
            app::CreateProjectedSketchEdgesCommand{
                *profile_sketch.sketch_id,
                projected_session.document().revision(), {*source},
                sk::EntityRole::construction},
            provider);
        CHECK(profile_linked.ok());
        CHECK(profile_linked.entity_ids.size() == 1U);
        const auto rectangle = projected_session.execute(
            app::AddSketchRectangleCommand{
                *profile_sketch.sketch_id,
                projected_session.document().revision(),
                {50.0, 20.0}, {56.0, 26.0},
                sk::EntityRole::regular, false});
        CHECK(rectangle.ok());
        const auto* profile_model = projected_session.document()
            .findSketch(*profile_sketch.sketch_id);
        CHECK(profile_model != nullptr);
        const auto profile_regions = sk::analyzeRegions(profile_model->model);
        CHECK(profile_regions.complete());
        CHECK(profile_regions.regions.size() == 1U);
        const auto profile_intent = part::makeProfileRegionIntent(
            profile_regions.regions.front());
        CHECK(profile_intent);

        const auto check_stale_profile_create = [&](CommandReentrantKernel::Hook hook) {
            const auto original_revision = projected_session.document().revision();
            const auto undo_before = projected_session.undoDepth();
            const auto profile_count = projected_session.document().state().profiles.size();
            const auto before_count = projected_session.document()
                .findSketch(*profile_sketch.sketch_id)->model.state().lines.size();
            CommandReentrantKernel reentry{
                provider, projected_session, *profile_sketch.sketch_id, hook};
            reentry.nested_role = sk::EntityRole::construction;
            const auto created = projected_session.execute(
                app::CreateProfileCommand{
                    *profile_sketch.sketch_id, original_revision,
                    *profile_intent},
                reentry);
            CHECK(!created.ok() && !created.changed);
            CHECK(created.diagnostic.code ==
                app::DocumentSessionErrorCode::revision_diverged);
            CHECK(reentry.mutated && reentry.binds == 1U);
            CHECK(reentry.projections ==
                (hook == CommandReentrantKernel::Hook::bind ? 0U : 1U));
            CHECK(projected_session.undoDepth() == undo_before + 1U);
            CHECK(projected_session.document().state().profiles.size() ==
                profile_count);
            const auto* live = projected_session.document().findSketch(
                *profile_sketch.sketch_id);
            CHECK(live != nullptr);
            CHECK(live->model.state().lines.size() == before_count + 1U);
            CHECK(live->projection_bindings.size() == 1U);
        };
        check_stale_profile_create(CommandReentrantKernel::Hook::bind);
        check_stale_profile_create(CommandReentrantKernel::Hook::project);

        const auto good_profile = projected_session.execute(
            app::CreateProfileCommand{
                *profile_sketch.sketch_id,
                projected_session.document().revision(), *profile_intent},
            provider);
        CHECK(good_profile.ok() && good_profile.profile_id);
        CHECK(projected_session.document().findProfile(*good_profile.profile_id));

        const auto check_stale_profile_edit = [&](CommandReentrantKernel::Hook hook) {
            const auto original_revision = projected_session.document().revision();
            const auto undo_before = projected_session.undoDepth();
            const auto before_count = projected_session.document()
                .findSketch(*profile_sketch.sketch_id)->model.state().lines.size();
            const auto before_intent = projected_session.document()
                .findProfile(*good_profile.profile_id)->region_intent;
            CommandReentrantKernel reentry{
                provider, projected_session, *profile_sketch.sketch_id, hook};
            reentry.nested_role = sk::EntityRole::construction;
            const auto changed = projected_session.execute(
                app::ReplaceProfileRegionIntentCommand{
                    *good_profile.profile_id, original_revision,
                    *profile_intent},
                reentry);
            CHECK(!changed.ok() && !changed.changed);
            CHECK(changed.diagnostic.code ==
                app::DocumentSessionErrorCode::revision_diverged);
            CHECK(reentry.mutated && reentry.binds == 1U);
            CHECK(reentry.projections ==
                (hook == CommandReentrantKernel::Hook::bind ? 0U : 1U));
            CHECK(projected_session.undoDepth() == undo_before + 1U);
            const auto* live = projected_session.document().findSketch(
                *profile_sketch.sketch_id);
            CHECK(live != nullptr);
            CHECK(live->model.state().lines.size() == before_count + 1U);
            CHECK(projected_session.document().findProfile(
                *good_profile.profile_id)->region_intent == before_intent);
        };
        check_stale_profile_edit(CommandReentrantKernel::Hook::bind);
        check_stale_profile_edit(CommandReentrantKernel::Hook::project);

        // Positive same-revision Edit Profile remains a no-op when the
        // authored RegionIntent is unchanged; no spurious Undo/dirty write.
        const auto fresh_revision = projected_session.document().revision();
        const auto edit_undo = projected_session.undoDepth();
        const auto clean_edit = projected_session.execute(
            app::ReplaceProfileRegionIntentCommand{
                *good_profile.profile_id, fresh_revision,
                *profile_intent},
            provider);
        CHECK(clean_edit.ok() && !clean_edit.changed);
        CHECK(projected_session.document().revision() == fresh_revision);
        CHECK(projected_session.undoDepth() == edit_undo);
        }
    }

    // Fixed-base constrained rectangle through actual OCCT Part evaluation.
    // This exercises eight authored local relations (seven numeric equations
    // plus Fix(Line)) as ONE live Sketch -> Profile -> Extrude Feature, with
    // the user-fixed bottom untouched and the free height changed by an
    // injected test backend. It is NOT a native PlaneGCS backend claim.
    {
        app::DocumentSession rect_session{
            {}, part::PartDocument::create(core::DocumentId::generate())};
        const auto rect_sketch = rect_session.execute(
            app::CreatePartSketchCommand{core::BuiltinReferenceRole::xy_plane});
        CHECK(rect_sketch.ok() && rect_sketch.sketch_id);
        const auto rectangle = rect_session.execute(
            app::AddSketchRectangleCommand{
                *rect_sketch.sketch_id, rect_session.document().revision(),
                {5.0, 0.0}, {15.0, 8.0},
                sk::EntityRole::regular, false});
        CHECK(rectangle.ok() && rectangle.entity_ids.size() == 4U);
        const auto original_rect = rect_session.document().findSketch(
            *rect_sketch.sketch_id)->model.state();
        CHECK(original_rect.lines.size() == 4U);
        const auto regions = sk::analyzeRegions(
            rect_session.document().findSketch(*rect_sketch.sketch_id)->model);
        CHECK(regions.complete() && regions.regions.size() == 1U);
        const auto intent = part::makeProfileRegionIntent(regions.regions.front());
        CHECK(intent);
        const auto profile = rect_session.execute(
            app::CreateProfileCommand{
                *rect_sketch.sketch_id,
                rect_session.document().revision(), *intent});
        CHECK(profile.ok() && profile.profile_id);

        using E = sk::ConstraintElement;
        const auto add_relation = [&](
            sk::ConstraintKind kind,
            sk::ConstraintReference first,
            sk::ConstraintReference second = {}) {
            const auto result = rect_session.execute(
                app::AddSketchRelationCommand{
                    *rect_sketch.sketch_id,
                    rect_session.document().revision(),
                    kind, first, second});
            CHECK(result.ok() && result.constraint_id);
        };
        const auto& a = original_rect.lines;
        add_relation(sk::ConstraintKind::fix, {a[0].id, E::line});
        add_relation(sk::ConstraintKind::coincident,
            {a[0].id, E::line_end}, {a[1].id, E::line_start});
        add_relation(sk::ConstraintKind::coincident,
            {a[1].id, E::line_end}, {a[2].id, E::line_start});
        add_relation(sk::ConstraintKind::coincident,
            {a[2].id, E::line_end}, {a[3].id, E::line_start});
        add_relation(sk::ConstraintKind::coincident,
            {a[3].id, E::line_end}, {a[0].id, E::line_start});
        add_relation(sk::ConstraintKind::vertical, {a[1].id, E::line});
        add_relation(sk::ConstraintKind::horizontal, {a[2].id, E::line});
        add_relation(sk::ConstraintKind::vertical, {a[3].id, E::line});

        const auto authored = rect_session.document().findSketch(
            *rect_sketch.sketch_id)->model.state();
        CHECK(authored.constraints.size() == 8U);
        CHECK(authored.lines == original_rect.lines);
        part::PartEvaluation prefix;
        prefix.source_document_id = rect_session.document().documentId();
        prefix.source_revision = rect_session.document().revision();
        prefix.source_incarnation = rect_session.document().incarnationToken();
        const auto input = part::prepareStageLocalSolveProblem(
            rect_session.document(), *rect_sketch.sketch_id, prefix, provider);
        CHECK(input);
        CHECK(input->constraints.size() == 8U);
        CHECK(input->equations.size() == 7U);
        CHECK(input->fixed_lines.size() == 1U);
        CHECK(input->fixed_lines.front() == a[0].id);
        CHECK(input->lines.front().fixed);
        for (std::size_t i = 1; i < input->lines.size(); ++i) {
            CHECK(!input->lines[i].fixed);
        }
        CHECK(!part::resolveKernelProfileInput(
            rect_session.document(), *profile.profile_id).ok());

        struct FixedBaseRectangleSolver final : sk::ILocalSketchSolver {
            bool move_fixed{};
            unsigned calls{};

            sk::LocalBackendSolveResult solve(
                const sk::LocalSolveProblem& problem) override {
                ++calls;
                CHECK(problem.constraints.size() == 8U);
                CHECK(problem.equations.size() == 7U);
                CHECK(problem.fixed_lines.size() == 1U);
                CHECK(problem.lines.size() == 4U);
                auto candidate = problem.authored;
                candidate.lines[1].end.v = 10.0;
                candidate.lines[2].start.v = 10.0;
                candidate.lines[2].end.v = 10.0;
                candidate.lines[3].start.v = 10.0;
                if (move_fixed) candidate.lines[0].end.u += 0.5;
                return {sk::LocalBackendStatus::converged,
                        std::move(candidate), {}, {}, 1U};
            }
        } rect_backend;

        const sk::LocalSolverAcceptancePolicy rect_policy{1.0e-12, 1.0e-12};
        const auto solved_input = part::resolveStageSolvedProfileKernelInput(
            rect_session.document(), *profile.profile_id, prefix,
            provider, rect_policy, rect_backend);
        CHECK(solved_input.ok() && solved_input.input);
        CHECK(solved_input.input->outer.boundary.size() == 4U);
        CHECK(rect_backend.calls == 1U);
        CHECK(occt::buildProfileFaceEvidence(*solved_input.input).brep_valid);

        auto with_feature = rect_session.document().state();
        const auto feature_id = with_feature.body.next_feature_id.allocate();
        CHECK(feature_id);
        with_feature.body.features.push_back(part::PartFeature{
            *feature_id, "R13EightRelationsExtrude", false,
            part::ExtrudeFeature{
                *profile.profile_id, part::ExtrudeOperation::add,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{10.0}, false}}});
        const auto owning = part::PartDocument::restore(
            rect_session.document().documentId(), std::move(with_feature),
            rect_session.document().revision());
        CHECK(owning.ok() && owning.document);
        const auto blocked = part::evaluatePart(*owning.document, provider);
        CHECK(blocked.features.size() == 1U);
        CHECK(blocked.features.front().status ==
            part::FeatureEvaluationStatus::blocked);
        const auto extruded = part::evaluatePartWithLocalSolve(
            *owning.document, provider, rect_policy, rect_backend);
        CHECK(extruded.body_status ==
            part::BodyEvaluationStatus::up_to_date);
        CHECK(extruded.features.size() == 1U);
        CHECK(extruded.features.front().status ==
            part::FeatureEvaluationStatus::up_to_date);
        CHECK(extruded.body_solid != nullptr);
        CHECK(rect_backend.calls == 2U);
        const auto mesh = provider.presentationMesh(extruded.body_solid);
        CHECK(mesh.ok());
        double max_y = -std::numeric_limits<double>::infinity();
        for (const auto& triangle : mesh.mesh.triangles) {
            for (const auto& point :
                 {triangle.first, triangle.second, triangle.third}) {
                max_y = std::max(max_y, point.y);
            }
        }
        CHECK(std::isfinite(max_y));
        CHECK(std::abs(max_y - 10.0) < 0.01);
        CHECK(owning.document->findSketch(*rect_sketch.sketch_id)
            ->model.state() == authored);

        // Even a zero numeric residual cannot license movement of the
        // authored fixed base; no current OCCT Body may be published.
        rect_backend.move_fixed = true;
        const auto refused = part::evaluatePartWithLocalSolve(
            *owning.document, provider, rect_policy, rect_backend);
        CHECK(refused.body_status !=
            part::BodyEvaluationStatus::up_to_date);
        CHECK(refused.features.size() == 1U);
        CHECK(refused.features.front().status ==
            part::FeatureEvaluationStatus::blocked);
        CHECK(rect_backend.calls == 3U);
        CHECK(owning.document->findSketch(*rect_sketch.sketch_id)
            ->model.state() == authored);
    }

    std::cout << "R13_A1_OCCT_PASS exact_seam=1"
              << " extrude_solid=1 revolve_solid=1"
              << " stale_profile_publication=0"
              << " derived_extrude_x_shift=2mm"
              << " derived_revolve_radii_shift=2mm"
              << " stage_prefix_cut=1"
              << " pg01_reentrant_stale=0"
              << " pg01_reentrant_bind_stale=0"
              << " pg01_command_atomic_reentry=1"
              << " pg01_break_atomic_reentry=1"
              << " pg01_profile_command_reentry=1\n";
    return EXIT_SUCCESS;
}
#endif
