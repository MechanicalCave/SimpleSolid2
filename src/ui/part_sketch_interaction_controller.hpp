#pragma once

#include "part_viewport_controller.hpp"

#include <simplesolid2/application/cad_input.hpp>
#include <simplesolid2/application/cad_input_semantics.hpp>
#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/part/profile.hpp>
#include <simplesolid2/sketch/interaction_state.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <utility>

namespace simplesolid2::ui {

enum class ProfileToolSessionKind : std::uint8_t {
    create,
    edit,
};

struct ProfileToolOptions final {
    bool detect_islands{true};
    bool highlight_on_hover{true};
    bool show_region_boundaries{false};
    bool show_problems{true};

    friend bool operator==(
        const ProfileToolOptions&,
        const ProfileToolOptions&) = default;
};

class PartSketchInteractionController final
    : public application::ISketchCadInputSemanticTarget {
public:
    using StateChangedHandler = std::function<void()>;
    using StatusHandler = std::function<void(const std::string&)>;

    explicit PartSketchInteractionController(
        PartViewportController& viewport_controller);

    void begin(
        application::DocumentSession& session,
        sketch::SketchId sketch_id);
    void end();

    [[nodiscard]] bool active() const noexcept;
    [[nodiscard]] sketch::SketchTool tool() const noexcept;
    [[nodiscard]] std::optional<sketch::LineStage>
    lineStage() const noexcept;
    [[nodiscard]] std::optional<sketch::CircleStage>
    circleStage() const noexcept;
    [[nodiscard]] std::optional<sketch::ArcStage>
    arcStage() const noexcept;
    [[nodiscard]] std::optional<sketch::MoveStage>
    moveStage() const noexcept;
    [[nodiscard]] std::optional<sketch::CommonTransformStage>
    commonTransformStage() const noexcept;
    [[nodiscard]] std::optional<sketch::PointRequest>
    activePointRequest() const noexcept;
    [[nodiscard]] application::CadInputContextGeneration
    cadInputContextGeneration() const noexcept {
        return cad_input_context_generation_;
    }
    [[nodiscard]] bool submitDirectDistance(double distance);

    [[nodiscard]] bool cadInputSemanticActive() const noexcept override {
        return active();
    }
    [[nodiscard]] std::optional<sketch::PointRequest>
    cadInputSemanticPointRequest() const noexcept override {
        return activePointRequest();
    }
    [[nodiscard]] bool activateCadInputSemanticTool(
        sketch::SketchTool tool) override;
    [[nodiscard]] bool submitCadInputSemanticDirectDistance(
        double distance) override {
        return submitDirectDistance(distance);
    }
    [[nodiscard]] application::CadInputSubmitResult
    submitCadInputSemanticProfileCommand(
        const application::ProfileCadInputCommand&
            command) override;

    [[nodiscard]] std::size_t selectedCount() const noexcept;
    [[nodiscard]] bool directManipulationActive()
        const noexcept;
    [[nodiscard]] std::optional<sketch::DirectEditMode>
    directEditMode() const noexcept;
    [[nodiscard]] bool cycleDirectEditMode();
    [[nodiscard]] std::optional<sketch::SketchTool>
    lastRepeatableCommand() const noexcept {
        return last_repeatable_command_;
    }
    [[nodiscard]] bool repeatLastCommand();

    [[nodiscard]] bool profileToolActive() const noexcept {
        return profile_session_.has_value();
    }
    [[nodiscard]] std::optional<ProfileToolSessionKind>
    profileToolSessionKind() const noexcept;
    [[nodiscard]] part::ProfileAreaEditMode
    profileAreaMode() const noexcept;
    [[nodiscard]] ProfileToolOptions
    profileToolOptions() const noexcept;
    [[nodiscard]] bool setProfileToolOptions(
        ProfileToolOptions options);
    [[nodiscard]] bool setProfileAreaMode(
        part::ProfileAreaEditMode mode);
    [[nodiscard]] std::optional<part::ProfileId>
    editedProfileId() const noexcept;
    [[nodiscard]] std::optional<part::ProfileRegionIntent>
    profileDraftIntent() const;
    [[nodiscard]] std::optional<std::uint32_t>
    profileHoveredRegion() const noexcept;
    [[nodiscard]] std::optional<part::ProfileAreaEditStatus>
    profileHoverStatus() const noexcept;
    [[nodiscard]] std::optional<sketch::RegionCandidate2D>
    profileHoverPreview() const;
    [[nodiscard]] std::size_t
    profileAnalysisBuildCount() const noexcept {
        return profile_analysis_build_count_;
    }

    [[nodiscard]] bool activateProfileCreate();
    [[nodiscard]] bool activateProfileEdit(
        part::ProfileId profile_id);
    [[nodiscard]] bool finishProfile();
    void cancelProfile();

    // Line/Circle/Arc are adapters to the same semantic
    // SketchInteractionState and resolved-input path.
    void activateSelect();
    void activateLine();
    void activateCircle();
    void activateArc();
    [[nodiscard]] bool activateMove();
    [[nodiscard]] bool activateCopy();
    [[nodiscard]] bool activateRotate();
    [[nodiscard]] bool activateScale();
    [[nodiscard]] bool activateMirror();
    [[nodiscard]] bool completeTransformSelection();
    [[nodiscard]] bool commitTransform();

    // SK-07A source compatibility.
    [[nodiscard]] bool completeMoveSelection();
    [[nodiscard]] bool commitMove();
    void finishLine();
    void cancelLine();
    [[nodiscard]] bool escape();

    [[nodiscard]] bool deleteSelection();
    [[nodiscard]] bool commitDirectManipulation();

    void cancelForHistory();
    [[nodiscard]] bool reconcileAfterHistory();

    void onPointer(const SketchPointerInput& input);

    void setStateChangedHandler(StateChangedHandler handler) {
        state_changed_handler_ = std::move(handler);
    }

    void setStatusHandler(StatusHandler handler) {
        status_handler_ = std::move(handler);
    }

private:
    static constexpr double drag_threshold_pixels = 4.0;

    [[nodiscard]] const part::PartSketch*
    activeSketch() const noexcept;

    void handleSelectPointer(const SketchPointerInput& input);
    void handleLinePointer(const SketchPointerInput& input);
    void handleCirclePointer(const SketchPointerInput& input);
    void handleArcPointer(const SketchPointerInput& input);
    void handleCommonTransformPointer(
        const SketchPointerInput& input);
    void handleProfilePointer(
        const SketchPointerInput& input);
    [[nodiscard]] bool ensureProfileAnalysis();
    void updateProfileHover(sketch::Point2 point);
    void resetProfileRuntime() noexcept;
    [[nodiscard]] bool acceptLineResolvedPoint(
        sketch::ResolvedSketchInput input);
    void updateRectangleOverlay(viewer::ViewportPoint2 current);
    void updateHover(viewer::ViewportPoint2 point);
    [[nodiscard]] bool beginDirectManipulation(
        sketch::SketchGripRef grip);
    void updateDirectManipulationPreview(
        sketch::Point2 raw_input);
    void updateCommonTransformPreview(
        sketch::Point2 raw_input);
    [[nodiscard]] application::DocumentSessionResult
    executeGeometryUpdate(
        const sketch::SketchTransformGeometry& geometry,
        core::DocumentRevision expected_revision);

    void projectSelection();
    void projectInteraction();
    void configureForCurrentTool();
    struct CadInputContextFingerprint final {
        bool active{};
        std::optional<sketch::SketchId> sketch_id;
        sketch::SketchTool tool{sketch::SketchTool::select};
        std::optional<sketch::LineStage> line_stage;
        std::optional<sketch::CircleStage> circle_stage;
        std::optional<sketch::ArcStage> arc_stage;
        std::optional<sketch::CommonTransformStage>
            transform_stage;
        bool direct_manipulation_active{};
        std::optional<sketch::DirectEditMode>
            direct_edit_mode;
        bool profile_active{};
        std::optional<ProfileToolSessionKind>
            profile_session_kind;
        part::ProfileAreaEditMode profile_area_mode{
            part::ProfileAreaEditMode::add_area};
        std::optional<sketch::Point2> point_base;
        bool direct_distance_enabled{};
        std::optional<core::DocumentRevision>
            document_revision;

        friend bool operator==(
            const CadInputContextFingerprint&,
            const CadInputContextFingerprint&) = default;
    };

    [[nodiscard]] CadInputContextFingerprint
    currentCadInputContextFingerprint() const noexcept;
    void refreshCadInputContextGeneration();
    void notifyStateChanged();
    void reportStatus(std::string message);

    PartViewportController* viewport_controller_{};
    application::DocumentSession* session_{};
    std::optional<sketch::SketchId> sketch_id_;
    sketch::SketchInteractionState interaction_;

    std::optional<viewer::ViewportPoint2> press_anchor_;
    bool rectangle_drag_active_{};
    std::optional<core::DocumentRevision>
        manipulation_revision_;
    std::optional<core::DocumentRevision>
        transform_revision_;
    std::optional<sketch::SketchTool>
        last_repeatable_command_;

    struct ProfileToolSession final {
        ProfileToolSessionKind kind{
            ProfileToolSessionKind::create};
        part::ProfileAreaEditMode area_mode{
            part::ProfileAreaEditMode::add_area};
        ProfileToolOptions options;
        std::optional<part::ProfileId> profile_id;
        std::optional<part::ProfileRegionIntent>
            draft_intent;
        std::optional<std::uint32_t>
            hovered_region;
        std::optional<part::ProfileAreaEditResult>
            hover_result;
        core::DocumentRevision expected_revision;
    };

    struct ProfileAnalysisCache final {
        sketch::SketchModelState model_state;
        sketch::RegionAnalysis2D analysis;
    };

    std::optional<ProfileToolSession>
        profile_session_;
    std::optional<ProfileAnalysisCache>
        profile_analysis_cache_;
    std::size_t profile_analysis_build_count_{};

    application::CadInputContextGeneration
        cad_input_context_generation_{};
    std::optional<CadInputContextFingerprint>
        cad_input_context_fingerprint_;

    StateChangedHandler state_changed_handler_;
    StatusHandler status_handler_;
};

} // namespace simplesolid2::ui
