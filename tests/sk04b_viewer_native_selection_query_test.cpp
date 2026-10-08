#include <simplesolid2/viewer_qt_occt/qt_occt_viewer_widget.hpp>

#include <QApplication>
#include <QMouseEvent>
#include <QTest>
#include <QWidget>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <vector>

using namespace simplesolid2;

namespace {

void check(
    bool value,
    const char* expression,
    int line) {
    if (!value) {
        std::cerr
            << "SK-04B native selection query CHECK failed at line "
            << line << ": "
            << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(expr) \
    check(static_cast<bool>(expr), #expr, __LINE__)

bool contains(
    const std::vector<viewer::PresentationToken>& tokens,
    viewer::PresentationToken token) {
    return std::find(
               tokens.begin(),
               tokens.end(),
               token) != tokens.end();
}

void sendMouseMove(
    QWidget& widget,
    viewer::ViewportPoint2 point) {
    QMouseEvent event{
        QEvent::MouseMove,
        QPointF{point.x, point.y},
        Qt::NoButton,
        Qt::NoButton,
        Qt::NoModifier};
    // sendEvent() is synchronous and drives the actual QWidget
    // mouseMoveEvent directly. Do not process the global event queue here:
    // that can immediately deliver a real OS-cursor move at an unrelated
    // screen position and overwrite the deterministic synthetic hover state.
    CHECK(QApplication::sendEvent(
        &widget,
        &event));
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication app{argc, argv};

    viewer_qt_occt::QtOcctViewerWidget widget;
    widget.resize(801, 601);
    widget.show();
    QApplication::processEvents();
    CHECK(widget.hasMouseTracking());

    const viewer::CameraState camera{
        viewer::Point3{0.0, 0.0, 100.0},
        viewer::Point3{0.0, 0.0, 0.0},
        viewer::Vec3{0.0, 1.0, 0.0},
        viewer::CameraProjection::orthographic,
        100.0};
    CHECK(widget.setCameraState(camera));

    // PM-02D2: Body topology query and View Style are projections of one
    // installed generation-scoped BodyScene. Front topology is returned;
    // occluded and non-material representation artifacts are not.
    const viewer::PresentationToken
        body_face_token{0x5001U};
    const viewer::PresentationToken
        body_edge_token{0x5002U};
    const viewer::PresentationToken
        hidden_edge_token{0x5003U};
    const viewer::PresentationToken
        body_vertex_token{0x5004U};
    const viewer::PresentationToken
        hidden_vertex_token{0x5005U};
    const viewer::PresentationToken
        seam_token{0x5006U};

    viewer::BodyScene body_scene;
    body_scene.generation = {77U};
    body_scene.purpose =
        viewer::BodyScenePurpose::current_body;
    body_scene.triangles = {
        {
            {-20.0, -20.0, 0.0},
            {20.0, -20.0, 0.0},
            {20.0, 20.0, 0.0},
            {0.0, 0.0, 1.0},
            {0.0, 0.0, 1.0},
            {0.0, 0.0, 1.0}},
        {
            {-20.0, -20.0, 0.0},
            {20.0, 20.0, 0.0},
            {-20.0, 20.0, 0.0},
            {0.0, 0.0, 1.0},
            {0.0, 0.0, 1.0},
            {0.0, 0.0, 1.0}},
    };
    body_scene.faces = {
        {
            body_face_token,
            0U,
            2U},
    };
    body_scene.edges = {
        {
            body_edge_token,
            {
                {-20.0, 0.0, 0.0},
                {20.0, 0.0, 0.0},
            },
            true,
            true},
        {
            hidden_edge_token,
            {
                {-20.0, 0.0, -20.0},
                {20.0, 0.0, -20.0},
            },
            true,
            true},
        {
            seam_token,
            {
                {0.0, -20.0, 0.0},
                {0.0, 20.0, 0.0},
            },
            false,
            false},
    };
    body_scene.vertices = {
        {
            body_vertex_token,
            {0.0, 0.0, 0.0},
            true},
        {
            hidden_vertex_token,
            {0.0, 0.0, -20.0},
            true},
    };
    CHECK(body_scene.valid());
    CHECK(widget.setBodyScene(body_scene));

    CHECK(
        widget.viewStyle() ==
        viewer::ViewStyle::shaded);
    CHECK(widget.setViewStyle(
        viewer::ViewStyle::shaded_with_edges));
    CHECK(
        widget.viewStyle() ==
        viewer::ViewStyle::shaded_with_edges);
    CHECK(widget.setViewStyle(
        viewer::ViewStyle::
            shaded_with_hidden_edges));
    CHECK(
        widget.viewStyle() ==
        viewer::ViewStyle::
            shaded_with_hidden_edges);

    const viewer::ViewportPoint2
        body_center{
            static_cast<double>(widget.width()) / 2.0,
            static_cast<double>(widget.height()) / 2.0};

    const auto body_all =
        widget.queryBodyTopology(
            body_center);
    CHECK(body_all.valid());
    CHECK(body_all.completed);
    CHECK(
        body_all.generation ==
        body_scene.generation);
    CHECK(
        std::count_if(
            body_all.candidates.begin(),
            body_all.candidates.end(),
            [body_face_token](const auto& item) {
                return item.token ==
                       body_face_token;
            }) == 1);
    CHECK(
        std::count_if(
            body_all.candidates.begin(),
            body_all.candidates.end(),
            [body_edge_token](const auto& item) {
                return item.token ==
                       body_edge_token;
            }) == 1);
    CHECK(
        std::count_if(
            body_all.candidates.begin(),
            body_all.candidates.end(),
            [body_vertex_token](const auto& item) {
                return item.token ==
                       body_vertex_token;
            }) == 1);
    CHECK(
        std::none_of(
            body_all.candidates.begin(),
            body_all.candidates.end(),
            [hidden_edge_token,
             hidden_vertex_token,
             seam_token](const auto& item) {
                return item.token ==
                           hidden_edge_token ||
                       item.token ==
                           hidden_vertex_token ||
                       item.token ==
                           seam_token;
            }));

    // PM-02D4: Feature Contribution is a deactivated presentation
    // overlay over the already-installed BodyScene token. It must not create
    // another hit target or perturb the neutral topology query.
    viewer::BodyTopologyOverlayScene
        feature_contribution_overlay;
    feature_contribution_overlay.generation =
        body_scene.generation;
    feature_contribution_overlay.groups.push_back(
        {
            viewer::BodyTopologyOverlayRole::
                feature_contribution_selected,
            {
                body_face_token,
                body_edge_token,
                body_vertex_token,
            }});
    CHECK(feature_contribution_overlay.valid());
    CHECK(widget.setBodyTopologyOverlayScene(
        feature_contribution_overlay));

    const auto body_with_overlay =
        widget.queryBodyTopology(
            body_center);
    CHECK(body_with_overlay.valid());
    CHECK(body_with_overlay.completed);
    CHECK(
        body_with_overlay.generation ==
        body_all.generation);
    CHECK(
        body_with_overlay.candidates ==
        body_all.candidates);

    auto stale_feature_overlay =
        feature_contribution_overlay;
    stale_feature_overlay.generation = {76U};
    CHECK(!widget.setBodyTopologyOverlayScene(
        stale_feature_overlay));
    CHECK(widget.setBodyTopologyOverlayScene(
        viewer::BodyTopologyOverlayScene{}));

    const auto face_only =
        widget.queryBodyTopology(
            body_center,
            viewer::BodyTopologyPickFilter{
                true,
                false,
                false});
    CHECK(face_only.valid());
    CHECK(face_only.completed);
    CHECK(face_only.candidates.size() == 1U);
    CHECK(
        face_only.candidates.front().token ==
        body_face_token);
    CHECK(
        face_only.candidates.front().kind ==
        viewer::BodyTopologyPresentationKind::
            face);

    const auto edge_only =
        widget.queryBodyTopology(
            body_center,
            viewer::BodyTopologyPickFilter{
                false,
                true,
                false});
    CHECK(edge_only.valid());
    CHECK(edge_only.completed);
    CHECK(edge_only.candidates.size() == 1U);
    CHECK(
        edge_only.candidates.front().token ==
        body_edge_token);

    const auto vertex_only =
        widget.queryBodyTopology(
            body_center,
            viewer::BodyTopologyPickFilter{
                false,
                false,
                true});
    CHECK(vertex_only.valid());
    CHECK(vertex_only.completed);
    CHECK(vertex_only.candidates.size() == 1U);
    CHECK(
        vertex_only.candidates.front().token ==
        body_vertex_token);

    // PM-02D3: hover emits the neutral Body query; provider does not rank
    // semantic priority. A controller-like callback may project the chosen
    // PresentationToken back as preselection.
    int body_preselection_intents = 0;
    int body_preselection_current_intents = 0;
    int body_preselection_clear_intents = 0;
    viewer::BodyTopologyPickQueryResult
        last_body_preselection_query;
    viewer::ViewportPoint2
        last_body_preselection_point;
    widget.setBodyTopologyPreselectionIntentHandler(
        [&widget,
         &body_preselection_intents,
         &body_preselection_current_intents,
         &body_preselection_clear_intents,
         &last_body_preselection_query,
         &last_body_preselection_point,
         body_vertex_token](
            const viewer::BodyTopologyPickQueryResult& query,
            viewer::ViewportPoint2 point) {
            ++body_preselection_intents;
            CHECK(query.valid());
            const bool current_pick_intent =
                query.completed &&
                query.generation.valid() &&
                point.valid();
            if (current_pick_intent) {
                ++body_preselection_current_intents;
            } else {
                ++body_preselection_clear_intents;
            }
            last_body_preselection_query = query;
            last_body_preselection_point = point;
            if (query.valid() &&
                query.completed &&
                std::any_of(
                    query.candidates.begin(),
                    query.candidates.end(),
                    [body_vertex_token](const auto& candidate) {
                        return candidate.token ==
                               body_vertex_token;
                    })) {
                CHECK(
                    widget.setBodyTopologyPreselection(
                        body_vertex_token));
            } else {
                CHECK(
                    widget.setBodyTopologyPreselection(
                        std::nullopt));
            }
        });

    int cycle_forward = 0;
    int cycle_reverse = 0;
    widget.setBodyTopologyCycleIntentHandler(
        [&cycle_forward, &cycle_reverse](bool reverse) {
            if (reverse) {
                ++cycle_reverse;
            } else {
                ++cycle_forward;
            }
        });

    sendMouseMove(widget, body_center);
    CHECK(body_preselection_intents >= 1);
    CHECK(last_body_preselection_query.valid());
    CHECK(last_body_preselection_query.completed);
    CHECK(
        last_body_preselection_query.generation ==
        body_scene.generation);
    CHECK(
        last_body_preselection_point.x ==
        body_center.x);
    CHECK(
        last_body_preselection_point.y ==
        body_center.y);

    // Focus and drain unrelated OS/Qt events *before* arming the
    // deterministic hover. A queued real OS cursor move may otherwise
    // clear this preselection before Tab reaches QWidget::event().
    widget.setFocus(Qt::MouseFocusReason);
    QApplication::processEvents();
    sendMouseMove(widget, body_center);
    QTest::keyClick(
        &widget,
        Qt::Key_Tab,
        Qt::NoModifier);
    CHECK(cycle_forward == 1);
    CHECK(cycle_reverse == 0);

    // Keep the native hover armed between focus-cycle gestures instead of
    // draining unrelated global mouse events in the middle of the test.
    sendMouseMove(widget, body_center);
    QTest::keyClick(
        &widget,
        Qt::Key_Tab,
        Qt::ShiftModifier);
    CHECK(cycle_forward == 1);
    CHECK(cycle_reverse == 1);
    QApplication::processEvents();

    // PM-02J R4: a visible box corner must keep stable Vertex/Edge
    // candidates and hover preselection in an oblique engineering view.
    // Boundary samples may differ from the nearest triangle by tiny
    // projection/ray round-trip error, but hidden topology must remain
    // excluded by the existing depth test.
    {
        viewer_qt_occt::QtOcctViewerWidget
            boundary_widget;
        boundary_widget.resize(801, 601);
        boundary_widget.show();
        QApplication::processEvents();
        CHECK(boundary_widget.hasMouseTracking());

        const viewer::CameraState boundary_camera{
            viewer::Point3{60.0, -60.0, 60.0},
            viewer::Point3{0.0, 0.0, 0.0},
            viewer::Vec3{0.0, 0.0, 1.0},
            viewer::CameraProjection::orthographic,
            70.0};
        CHECK(boundary_widget.setCameraState(
            boundary_camera));

        const viewer::PresentationToken
            face_x_token{0x6101U};
        const viewer::PresentationToken
            face_y_token{0x6102U};
        const viewer::PresentationToken
            face_z_token{0x6103U};
        const viewer::PresentationToken
            edge_x_token{0x6201U};
        const viewer::PresentationToken
            edge_y_token{0x6202U};
        const viewer::PresentationToken
            edge_z_token{0x6203U};
        const viewer::PresentationToken
            corner_token{0x6301U};

        viewer::BodyScene boundary_scene;
        boundary_scene.generation = {88U};
        boundary_scene.purpose =
            viewer::BodyScenePurpose::current_body;

        // Three front-visible box faces meeting at (10,-10,10).
        boundary_scene.triangles = {
            // +X
            {{10.0,-10.0,-10.0},{10.0,10.0,-10.0},{10.0,10.0,10.0},
             {1.0,0.0,0.0},{1.0,0.0,0.0},{1.0,0.0,0.0}},
            {{10.0,-10.0,-10.0},{10.0,10.0,10.0},{10.0,-10.0,10.0},
             {1.0,0.0,0.0},{1.0,0.0,0.0},{1.0,0.0,0.0}},
            // -Y
            {{-10.0,-10.0,-10.0},{10.0,-10.0,-10.0},{10.0,-10.0,10.0},
             {0.0,-1.0,0.0},{0.0,-1.0,0.0},{0.0,-1.0,0.0}},
            {{-10.0,-10.0,-10.0},{10.0,-10.0,10.0},{-10.0,-10.0,10.0},
             {0.0,-1.0,0.0},{0.0,-1.0,0.0},{0.0,-1.0,0.0}},
            // +Z
            {{-10.0,-10.0,10.0},{10.0,-10.0,10.0},{10.0,10.0,10.0},
             {0.0,0.0,1.0},{0.0,0.0,1.0},{0.0,0.0,1.0}},
            {{-10.0,-10.0,10.0},{10.0,10.0,10.0},{-10.0,10.0,10.0},
             {0.0,0.0,1.0},{0.0,0.0,1.0},{0.0,0.0,1.0}},
        };
        boundary_scene.faces = {
            {face_x_token, 0U, 2U},
            {face_y_token, 2U, 2U},
            {face_z_token, 4U, 2U},
        };
        boundary_scene.edges = {
            {edge_x_token,
             {{-10.0,-10.0,10.0},{10.0,-10.0,10.0}},
             true,true},
            {edge_y_token,
             {{10.0,-10.0,10.0},{10.0,10.0,10.0}},
             true,true},
            {edge_z_token,
             {{10.0,-10.0,-10.0},{10.0,-10.0,10.0}},
             true,true},
        };
        boundary_scene.vertices = {
            {corner_token,{10.0,-10.0,10.0},true},
        };
        CHECK(boundary_scene.valid());
        CHECK(boundary_widget.setBodyScene(
            boundary_scene));

        const auto corner_point =
            boundary_widget.projectWorldPoint(
                {10.0,-10.0,10.0});
        CHECK(corner_point.has_value());

        int corner_hover_intents = 0;
        viewer::BodyTopologyPickQueryResult
            corner_hover_query;
        boundary_widget
            .setBodyTopologyPreselectionIntentHandler(
                [&boundary_widget,
                 &corner_hover_intents,
                 &corner_hover_query,
                 corner_token](
                    const viewer::BodyTopologyPickQueryResult& query,
                    viewer::ViewportPoint2) {
                    ++corner_hover_intents;
                    corner_hover_query = query;
                    const auto vertex =
                        std::find_if(
                            query.candidates.begin(),
                            query.candidates.end(),
                            [corner_token](const auto& item) {
                                return item.token ==
                                       corner_token;
                            });
                    if (vertex != query.candidates.end()) {
                        CHECK(
                            boundary_widget
                                .setBodyTopologyPreselection(
                                    corner_token));
                    }
                });

        for (const auto offset :
             std::vector<viewer::ViewportPoint2>{
                 {0.0,0.0},
                 {1.0,0.0},
                 {-1.0,0.0},
                 {0.0,1.0},
                 {0.0,-1.0}}) {
            const viewer::ViewportPoint2 probe{
                corner_point->x + offset.x,
                corner_point->y + offset.y};
            const auto query =
                boundary_widget.queryBodyTopology(
                    probe);
            CHECK(query.valid());
            CHECK(query.completed);
            CHECK(
                std::any_of(
                    query.candidates.begin(),
                    query.candidates.end(),
                    [corner_token](const auto& item) {
                        return item.token ==
                               corner_token;
                    }));
            CHECK(
                std::count_if(
                    query.candidates.begin(),
                    query.candidates.end(),
                    [](const auto& item) {
                        return item.kind ==
                            viewer::
                                BodyTopologyPresentationKind::
                                    edge;
                    }) >= 2);
            sendMouseMove(
                boundary_widget,
                probe);
            CHECK(corner_hover_intents >= 1);
            CHECK(corner_hover_query.valid());
            CHECK(
                std::any_of(
                    corner_hover_query.candidates.begin(),
                    corner_hover_query.candidates.end(),
                    [corner_token](const auto& item) {
                        return item.token ==
                               corner_token;
                    }));
        }
    }

    // PM-05F R2-E: real Qt/OCCT cursor hit-testing on a curved circular
    // material boundary. The triangulation is intentionally coarser than
    // the authored Edge polyline, as in a normal cylindrical Body display.
    // The test checks two zooms and an occluded rear/bottom boundary.
    {
        viewer_qt_occt::QtOcctViewerWidget ring_widget;
        ring_widget.resize(801, 601);
        ring_widget.show();
        QApplication::processEvents();

        constexpr double radius = 10.0;
        constexpr double top_z = 20.0;
        constexpr int facets = 64;
        const double full_turn = 2.0 * std::acos(-1.0);

        viewer::BodyScene ring_scene;
        ring_scene.generation = {81U};
        ring_scene.purpose =
            viewer::BodyScenePurpose::current_body;
        const viewer::PresentationToken
            ring_face_token{0x7101U};
        const viewer::PresentationToken
            cylinder_face_token{0x7102U};
        const viewer::PresentationToken
            top_ring_token{0x7201U};
        const viewer::PresentationToken
            bottom_ring_token{0x7202U};

        auto ringPoint = [full_turn](
            int index,
            double z) {
            const double angle =
                full_turn * static_cast<double>(index) /
                static_cast<double>(facets);
            return viewer::Point3{
                radius * std::cos(angle),
                radius * std::sin(angle),
                z};
        };

        for (int index = 0; index < facets; ++index) {
            const auto first = ringPoint(index, top_z);
            const auto next = ringPoint(index + 1, top_z);
            ring_scene.triangles.push_back(
                {
                    {0.0, 0.0, top_z},
                    first,
                    next,
                    {0.0, 0.0, 1.0},
                    {0.0, 0.0, 1.0},
                    {0.0, 0.0, 1.0}});
        }
        ring_scene.faces.push_back(
            {
                ring_face_token,
                0U,
                static_cast<std::size_t>(facets)});
        for (int index = 0; index < facets; ++index) {
            const auto top_first =
                ringPoint(index, top_z);
            const auto top_next =
                ringPoint(index + 1, top_z);
            const auto bottom_first =
                ringPoint(index, 0.0);
            const auto bottom_next =
                ringPoint(index + 1, 0.0);
            const viewer::Vec3 normal{0.0, -1.0, 0.0};
            ring_scene.triangles.push_back(
                {
                    top_first,
                    bottom_first,
                    bottom_next,
                    normal,
                    normal,
                    normal});
            ring_scene.triangles.push_back(
                {
                    top_first,
                    bottom_next,
                    top_next,
                    normal,
                    normal,
                    normal});
        }
        ring_scene.faces.push_back(
            {
                cylinder_face_token,
                static_cast<std::size_t>(facets),
                static_cast<std::size_t>(2 * facets)});

        viewer::BodyEdgePresentation top_ring{
            top_ring_token, {}, true, true};
        viewer::BodyEdgePresentation bottom_ring{
            bottom_ring_token, {}, true, true};
        for (int index = 0; index <= 256; ++index) {
            const double angle =
                full_turn * static_cast<double>(index) /
                256.0;
            const double x = radius * std::cos(angle);
            const double y = radius * std::sin(angle);
            top_ring.points.push_back({x, y, top_z});
            bottom_ring.points.push_back({x, y, 0.0});
        }
        ring_scene.edges.push_back(std::move(top_ring));
        ring_scene.edges.push_back(std::move(bottom_ring));
        CHECK(ring_scene.valid());
        CHECK(ring_widget.setBodyScene(ring_scene));

        int ring_clicks = 0;
        ring_widget.setBodyTopologySelectionIntentHandler(
            [&ring_clicks,
             top_ring_token](
                const viewer::BodyTopologyPickQueryResult& query,
                viewer::SelectionIntentMode) {
                CHECK(query.valid());
                CHECK(query.completed);
                CHECK(std::any_of(
                    query.candidates.begin(),
                    query.candidates.end(),
                    [top_ring_token](const auto& item) {
                        return item.token ==
                               top_ring_token;
                    }));
                ++ring_clicks;
            });

        for (const double scale : {60.0, 35.0}) {
            const viewer::CameraState camera{
                viewer::Point3{45.0, -75.0, 55.0},
                viewer::Point3{0.0, 0.0, 10.0},
                viewer::Vec3{0.0, 0.0, 1.0},
                viewer::CameraProjection::orthographic,
                scale};
            CHECK(ring_widget.setCameraState(camera));
            for (const double angle :
                 {-std::acos(-1.0) / 2.0,
                  -std::acos(-1.0) / 4.0}) {
                const viewer::Point3 target{
                    radius * std::cos(angle),
                    radius * std::sin(angle),
                    top_z};
                const auto screen =
                    ring_widget.projectWorldPoint(target);
                CHECK(screen.has_value());
                const auto query =
                    ring_widget.queryBodyTopology(
                        *screen,
                        viewer::BodyTopologyPickFilter{
                            false, true, false});
                CHECK(query.valid());
                CHECK(query.completed);
                CHECK(query.generation ==
                      ring_scene.generation);
                CHECK(std::any_of(
                    query.candidates.begin(),
                    query.candidates.end(),
                    [top_ring_token](const auto& item) {
                        return item.token ==
                               top_ring_token;
                    }));
                sendMouseMove(ring_widget, *screen);
                QTest::mouseClick(
                    &ring_widget,
                    Qt::LeftButton,
                    Qt::NoModifier,
                    QPoint{
                        static_cast<int>(std::lround(
                            screen->x)),
                        static_cast<int>(std::lround(
                            screen->y))});
                QApplication::processEvents();
                CHECK(ring_clicks > 0);
            }
        }
        CHECK(ring_clicks == 4);

        const auto hidden =
            ring_widget.projectWorldPoint(
                {0.0, radius, 0.0});
        CHECK(hidden.has_value());
        const auto hidden_query =
            ring_widget.queryBodyTopology(
                *hidden,
                viewer::BodyTopologyPickFilter{
                    false, true, false});
        CHECK(hidden_query.valid());
        CHECK(std::none_of(
            hidden_query.candidates.begin(),
            hidden_query.candidates.end(),
            [bottom_ring_token](const auto& item) {
                return item.token ==
                       bottom_ring_token;
            }));
        ring_widget.setBodyTopologySelectionIntentHandler({});
    }

    // View Style lives in the same provider-surface HUD family as the
    // navigation controls. Its click emits an action; the callback/controller
    // remains runtime state authority.
    std::optional<viewer::ViewStyle>
        requested_view_style;
    widget.setViewStyleActionHandler(
        [&widget, &requested_view_style](
            viewer::ViewStyle style) {
            requested_view_style = style;
            CHECK(widget.setViewStyle(style));
        });
    CHECK(widget.setViewStyle(
        viewer::ViewStyle::shaded));

    const QPoint style_trigger{
        widget.width() - 300,
        18};
    QTest::mouseClick(
        &widget,
        Qt::LeftButton,
        Qt::NoModifier,
        style_trigger);
    QApplication::processEvents();

    const QPoint style_edges{
        widget.width() - 300,
        66};
    QTest::mouseClick(
        &widget,
        Qt::LeftButton,
        Qt::NoModifier,
        style_edges);
    QApplication::processEvents();
    CHECK(
        requested_view_style ==
        std::optional<viewer::ViewStyle>{
            viewer::ViewStyle::
                shaded_with_edges});
    CHECK(
        widget.viewStyle() ==
        viewer::ViewStyle::
            shaded_with_edges);

    // Hovering the HUD clears Body preselection instead of letting geometry
    // highlight through the control.
    const int intents_before_hud_hover =
        body_preselection_intents;
    const int current_before_hud_hover =
        body_preselection_current_intents;
    const int clear_before_hud_hover =
        body_preselection_clear_intents;
    sendMouseMove(
        widget,
        {
            static_cast<double>(style_trigger.x()),
            static_cast<double>(style_trigger.y())});
    CHECK(
        body_preselection_intents >
        intents_before_hud_hover);
    CHECK(
        body_preselection_clear_intents >
        clear_before_hud_hover);
    CHECK(
        body_preselection_current_intents ==
        current_before_hud_hover);
    CHECK(
        last_body_preselection_query.valid());
    CHECK(
        !last_body_preselection_query.completed);
    CHECK(
        !last_body_preselection_query.generation.valid());
    CHECK(
        last_body_preselection_query.candidates.empty());
    // ViewportPoint2{0,0} is itself a valid screen coordinate. Clear intent
    // authority is the non-current query payload above, not point invalidity.

    const int cycles_before_cleared_tab =
        cycle_forward + cycle_reverse;
    QTest::keyClick(
        &widget,
        Qt::Key_Tab,
        Qt::NoModifier);
    QApplication::processEvents();
    CHECK(
        cycle_forward + cycle_reverse ==
        cycles_before_cleared_tab);

    CHECK(widget.setPresentationSelection(
        viewer::PresentationSelection{
            {body_face_token,
             body_edge_token,
             body_vertex_token},
            body_vertex_token}));

    viewer::BodyScene diagnostic_body =
        body_scene;
    diagnostic_body.generation = {78U};
    diagnostic_body.purpose =
        viewer::BodyScenePurpose::
            diagnostic_prefix;
    CHECK(widget.setBodyScene(
        diagnostic_body));
    const auto diagnostic_query =
        widget.queryBodyTopology(
            body_center);
    CHECK(diagnostic_query.valid());
    CHECK(diagnostic_query.completed);
    CHECK(
        diagnostic_query.generation ==
        diagnostic_body.generation);
    CHECK(diagnostic_query.candidates.empty());

    // PM-05F R2: Edit of an earlier Fillet/Chamfer presents the exact
    // predecessor Body as a tool_stage scene. Native screen-coordinate
    // queries and actual mouse clicks must work at that stage, whereas a
    // diagnostic_prefix remains non-authorable.
    viewer::BodyScene tool_stage_body = body_scene;
    tool_stage_body.generation = {79U};
    tool_stage_body.purpose =
        viewer::BodyScenePurpose::tool_stage;
    CHECK(widget.setBodyScene(tool_stage_body));
    const auto tool_stage_query =
        widget.queryBodyTopology(
            body_center,
            viewer::BodyTopologyPickFilter{
                false,
                true,
                false});
    CHECK(tool_stage_query.valid());
    CHECK(tool_stage_query.completed);
    CHECK(
        tool_stage_query.generation ==
        tool_stage_body.generation);
    CHECK(tool_stage_query.candidates.size() == 1U);
    CHECK(
        tool_stage_query.candidates.front().token ==
        body_edge_token);

    int tool_stage_click_intents = 0;
    widget.setBodyTopologySelectionIntentHandler(
        [&tool_stage_click_intents,
         &tool_stage_body,
         body_edge_token](
            const viewer::BodyTopologyPickQueryResult& query,
            viewer::SelectionIntentMode mode) {
            CHECK(query.valid());
            CHECK(query.completed);
            CHECK(
                query.generation ==
                tool_stage_body.generation);
            CHECK(
                mode ==
                viewer::SelectionIntentMode::replace);
            CHECK(
                std::any_of(
                    query.candidates.begin(),
                    query.candidates.end(),
                    [body_edge_token](const auto& candidate) {
                        return candidate.token ==
                               body_edge_token;
                    }));
            ++tool_stage_click_intents;
        });
    QTest::mouseClick(
        &widget,
        Qt::LeftButton,
        Qt::NoModifier,
        QPoint{
            static_cast<int>(body_center.x),
            static_cast<int>(body_center.y)});
    QApplication::processEvents();
    CHECK(tool_stage_click_intents == 1);

    // The callback belongs to this tool-stage scenario only. Later tests
    // install unrelated scenes/generations on the same native widget.
    widget.setBodyTopologySelectionIntentHandler({});

    sendMouseMove(widget, body_center);
    CHECK(last_body_preselection_query.completed);
    CHECK(
        last_body_preselection_query.generation ==
        tool_stage_body.generation);

    CHECK(widget.setBodyScene(diagnostic_body));
    CHECK(
        widget.queryBodyTopology(
            body_center).candidates.empty());
    CHECK(widget.setBodyScene(body_scene));
    CHECK(widget.setViewStyle(
        viewer::ViewStyle::shaded));

    const viewer::PresentationToken short_token{
        0x4101U};
    const viewer::PresentationToken long_token{
        0x4102U};
    const viewer::PresentationToken reference_token{
        0x201U};

    viewer::ReferenceScene references;
    references.references.push_back(
        viewer::ReferencePresentation{
            reference_token,
            viewer::ReferencePresentationKind::point,
            viewer::Point3{0.0, 0.0, 0.0},
            {},
            {},
            3.0,
            viewer::PresentationRole::base,
            true});
    CHECK(widget.setReferenceScene(references));

    viewer::SketchScene scene;
    scene.lines = {
        viewer::SketchLinePresentation{
            short_token,
            viewer::Point3{-2.0, 0.0, 0.0},
            viewer::Point3{2.0, 0.0, 0.0}},
        viewer::SketchLinePresentation{
            long_token,
            viewer::Point3{0.0, -40.0, 0.0},
            viewer::Point3{0.0, 40.0, 0.0}},
    };
    scene.origin =
        viewer::SketchOriginPresentation{
            viewer::Point3{0.0, 0.0, 0.0}};
    CHECK(widget.setSketchScene(scene));

    const viewer::SketchGripKey center_grip{
        short_token,
        viewer::SketchGripRole::line_center};
    viewer::SketchGripScene grips;
    grips.grips = {
        viewer::SketchGripPresentation{
            {short_token, viewer::SketchGripRole::line_start},
            viewer::Point3{-2.0, 0.0, 0.0}},
        viewer::SketchGripPresentation{
            center_grip,
            viewer::Point3{0.0, 0.0, 0.0}},
        viewer::SketchGripPresentation{
            {short_token, viewer::SketchGripRole::line_end},
            viewer::Point3{2.0, 0.0, 0.0}},
    };
    CHECK(widget.setSketchGripScene(grips));
    CHECK(widget.setSketchInteractionPresentation(
        viewer::SketchInteractionPresentation{
            std::nullopt,
            center_grip,
            std::nullopt}));

    viewer::SketchGripScene duplicate_grips = grips;
    duplicate_grips.grips.push_back(
        duplicate_grips.grips.front());
    CHECK(!duplicate_grips.valid());
    CHECK(!widget.setSketchGripScene(duplicate_grips));
    CHECK(widget.setSketchGripScene(grips));

    viewer::SketchPreviewScene preview;
    preview.lines.push_back(
        viewer::SketchPreviewLine{
            viewer::Point3{-40.0, -40.0, 0.0},
            viewer::Point3{40.0, 40.0, 0.0}});
    CHECK(widget.setSketchPreviewScene(preview));

    int selection_intents = 0;
    std::optional<viewer::SelectionIntent>
        last_selection_intent;
    widget.setSelectionIntentHandler(
        [&selection_intents,
         &last_selection_intent](
            const viewer::SelectionIntent& intent) {
            ++selection_intents;
            last_selection_intent = intent;
        });

    CHECK(widget.setPresentationSelection(
        viewer::PresentationSelection{
            {reference_token},
            reference_token}));

    const viewer::ViewportPoint2 center{
        static_cast<double>(widget.width()) / 2.0,
        static_cast<double>(widget.height()) / 2.0};

    // R8B native measurement markers are latent until pointer proximity
    // reveals them. Query only considers currently revealed/selected markers.
    const viewer::SketchMeasureMarkerKey measure_center_short{
        short_token,
        viewer::SketchMeasureMarkerRole::line_midpoint};
    const viewer::SketchMeasureMarkerKey measure_center_long{
        long_token,
        viewer::SketchMeasureMarkerRole::line_midpoint};
    viewer::SketchMeasureMarkerScene measure_markers;
    measure_markers.markers = {
        viewer::SketchMeasureMarkerPresentation{
            measure_center_short,
            viewer::Point3{0.0, 0.0, 0.0}},
        viewer::SketchMeasureMarkerPresentation{
            measure_center_long,
            viewer::Point3{0.0, 0.0, 0.0}},
        viewer::SketchMeasureMarkerPresentation{
            {
                short_token,
                viewer::SketchMeasureMarkerRole::line_end},
            viewer::Point3{20.0, 0.0, 0.0}},
    };
    CHECK(widget.setSketchMeasureMarkerScene(
        measure_markers));

    const auto hidden_measure =
        widget.querySketchMeasureMarkers(center);
    CHECK(hidden_measure.valid());
    CHECK(hidden_measure.completed);
    CHECK(hidden_measure.markers.empty());

    // Drive the actual QWidget mouseMoveEvent directly. QTest::mouseMove()
    // depends on the suite-global OS cursor and is nondeterministic on CI.
    sendMouseMove(
        widget,
        viewer::ViewportPoint2{5.0, 5.0});
    sendMouseMove(widget, center);

    const auto revealed_measure =
        widget.querySketchMeasureMarkers(center);
    CHECK(revealed_measure.valid());
    CHECK(revealed_measure.completed);
    CHECK(revealed_measure.markers.size() == 2U);
    CHECK(
        std::find(
            revealed_measure.markers.begin(),
            revealed_measure.markers.end(),
            measure_center_short) !=
        revealed_measure.markers.end());
    CHECK(
        std::find(
            revealed_measure.markers.begin(),
            revealed_measure.markers.end(),
            measure_center_long) !=
        revealed_measure.markers.end());

    sendMouseMove(
        widget,
        viewer::ViewportPoint2{5.0, 5.0});
    const auto hidden_again =
        widget.querySketchMeasureMarkers(center);
    CHECK(hidden_again.completed);
    CHECK(hidden_again.markers.empty());

    measure_markers.selected = {
        measure_center_short};
    CHECK(widget.setSketchMeasureMarkerScene(
        measure_markers));
    sendMouseMove(
        widget,
        viewer::ViewportPoint2{5.0, 5.0});
    const auto pinned_measure =
        widget.querySketchMeasureMarkers(center);
    CHECK(pinned_measure.completed);
    CHECK(pinned_measure.markers.size() == 1U);
    CHECK(
        pinned_measure.markers.front() ==
        measure_center_short);

    viewer::SketchMeasureCueScene measure_cue;
    measure_cue.highlighted_entities = {
        short_token};
    measure_cue.segments = {
        viewer::SketchMeasureCueSegment{
            viewer::Point3{0.0, 0.0, 0.0},
            viewer::Point3{10.0, 10.0, 0.0},
            viewer::SketchMeasureCueSegmentKind::relation},
        viewer::SketchMeasureCueSegment{
            viewer::Point3{10.0, 10.0, 0.0},
            viewer::Point3{15.0, 15.0, 0.0},
            viewer::SketchMeasureCueSegmentKind::
                supporting_line_continuation}};
    measure_cue.cue_point =
        viewer::Point3{10.0, 10.0, 0.0};
    CHECK(widget.setSketchMeasureCueScene(measure_cue));
    CHECK(widget.setSketchMeasureCueScene(
        viewer::SketchMeasureCueScene{}));
    CHECK(widget.setSketchMeasureMarkerScene(
        viewer::SketchMeasureMarkerScene{}));

    const auto grip_hit =
        widget.querySketchGrip(center);
    CHECK(grip_hit.valid());
    CHECK(grip_hit.completed);
    CHECK(grip_hit.grip.has_value());
    CHECK(*grip_hit.grip == center_grip);

    const auto grip_empty =
        widget.querySketchGrip(
            viewer::ViewportPoint2{5.0, 5.0});
    CHECK(grip_empty.valid());
    CHECK(grip_empty.completed);
    CHECK(!grip_empty.grip.has_value());

    CHECK(widget.setSketchInteractionPresentation(
        viewer::SketchInteractionPresentation{
            std::nullopt,
            std::nullopt,
            center_grip}));

    const auto point_hit =
        widget.querySketchPresentation(center);
    CHECK(point_hit.valid());
    CHECK(point_hit.completed);
    CHECK(point_hit.token.has_value());
    CHECK(
        *point_hit.token == short_token ||
        *point_hit.token == long_token);
    CHECK(*point_hit.token != reference_token);
    CHECK(selection_intents == 0);

    const auto point_empty =
        widget.querySketchPresentation(
            viewer::ViewportPoint2{5.0, 5.0});
    CHECK(point_empty.valid());
    CHECK(point_empty.completed);
    CHECK(!point_empty.token.has_value());
    CHECK(selection_intents == 0);

    const auto nan =
        std::numeric_limits<double>::quiet_NaN();
    const auto point_failed =
        widget.querySketchPresentation(
            viewer::ViewportPoint2{nan, 0.0});
    CHECK(point_failed.valid());
    CHECK(!point_failed.completed);

    const auto central_rect =
        viewer::normalizedViewportRect(
            viewer::ViewportPoint2{
                center.x - 25.0,
                center.y - 25.0},
            viewer::ViewportPoint2{
                center.x + 25.0,
                center.y + 25.0});
    CHECK(central_rect.has_value());

    const auto window =
        widget.querySketchPresentations(
            *central_rect,
            viewer::SketchRectangleSelectionRule::
                window);
    CHECK(window.valid());
    CHECK(window.completed);
    CHECK(window.tokens.size() == 1U);
    CHECK(window.tokens[0] == short_token);
    CHECK(contains(window.tokens, short_token));
    CHECK(!contains(window.tokens, long_token));
    CHECK(!contains(window.tokens, reference_token));

    const auto crossing =
        widget.querySketchPresentations(
            *central_rect,
            viewer::SketchRectangleSelectionRule::
                crossing);
    CHECK(crossing.valid());
    CHECK(crossing.completed);
    CHECK(crossing.tokens.size() == 2U);
    CHECK(crossing.tokens[0] == short_token);
    CHECK(crossing.tokens[1] == long_token);
    CHECK(contains(crossing.tokens, short_token));
    CHECK(contains(crossing.tokens, long_token));
    CHECK(!contains(crossing.tokens, reference_token));

    const auto full_rect =
        viewer::normalizedViewportRect(
            viewer::ViewportPoint2{1.0, 1.0},
            viewer::ViewportPoint2{
                static_cast<double>(
                    widget.width() - 2),
                static_cast<double>(
                    widget.height() - 2)});
    CHECK(full_rect.has_value());
    const auto all_window =
        widget.querySketchPresentations(
            *full_rect,
            viewer::SketchRectangleSelectionRule::
                window);
    CHECK(all_window.completed);
    CHECK(all_window.tokens.size() == 2U);
    CHECK(all_window.tokens[0] == short_token);
    CHECK(all_window.tokens[1] == long_token);
    CHECK(contains(all_window.tokens, short_token));
    CHECK(contains(all_window.tokens, long_token));

    const auto revisionless_overlay =
        viewer::SketchSelectionBoxOverlay{
            {
                center.x - 30.0,
                center.y - 20.0},
            {
                center.x + 30.0,
                center.y + 20.0},
            viewer::SketchRectangleSelectionRule::
                window};
    CHECK(
        widget.setSketchSelectionBoxOverlay(
            revisionless_overlay));
    QApplication::processEvents();

    CHECK(
        widget.findChild<QWidget*>(
            QStringLiteral(
                "ss2SketchSelectionBoxOverlay")) ==
        nullptr);

    for (int iteration = 0;
         iteration < 100;
         ++iteration) {
        const auto offset =
            static_cast<double>(iteration % 20);
        CHECK(widget.setSketchSelectionBoxOverlay(
            viewer::SketchSelectionBoxOverlay{
                {
                    revisionless_overlay.anchor.x - offset,
                    revisionless_overlay.anchor.y},
                {
                    revisionless_overlay.current.x + offset,
                    revisionless_overlay.current.y},
                (iteration % 2) == 0
                    ? viewer::SketchRectangleSelectionRule::
                          window
                    : viewer::SketchRectangleSelectionRule::
                          crossing}));
        QApplication::processEvents();
    }

    widget.clearSketchSelectionBoxOverlay();
    QApplication::processEvents();

    const auto after_overlay =
        widget.querySketchPresentations(
            *central_rect,
            viewer::SketchRectangleSelectionRule::
                crossing);
    CHECK(after_overlay.completed);
    CHECK(contains(after_overlay.tokens, short_token));
    CHECK(contains(after_overlay.tokens, long_token));

    widget.orbitByRadians(0.35, -0.22);
    QApplication::processEvents();

    const auto grip_after_orbit =
        widget.querySketchGrip(center);
    CHECK(grip_after_orbit.valid());
    CHECK(grip_after_orbit.completed);
    CHECK(grip_after_orbit.grip.has_value());
    CHECK(*grip_after_orbit.grip == center_grip);

    const auto after_orbit =
        widget.querySketchPresentations(
            *central_rect,
            viewer::SketchRectangleSelectionRule::
                crossing);
    CHECK(after_orbit.valid());
    CHECK(after_orbit.completed);
    CHECK(contains(after_orbit.tokens, short_token));
    CHECK(contains(after_orbit.tokens, long_token));
    CHECK(selection_intents == 0);

    // SR-02: a semantic curve is presented as one OCCT wire object even
    // though its neutral scene retains the exact derived segment chain.
    // Native detection must still map the wire back to the one curve token.
    CHECK(widget.setReferenceScene(
        viewer::ReferenceScene{}));
    const viewer::PresentationToken curve_token{
        0x4201U};
    viewer::SketchScene curve_scene;
    curve_scene.curves.push_back(
        viewer::SketchCurvePresentation{
            curve_token,
            {
                {-20.0, 0.0, 0.0},
                {-10.0, 0.0, 0.0},
                {0.0, 0.0, 0.0},
                {10.0, 0.0, 0.0},
                {20.0, 0.0, 0.0},
            },
            false});
    CHECK(curve_scene.valid());
    CHECK(widget.setSketchScene(curve_scene));
    CHECK(
        widget.runtimeDiagnostics().
            sketch_native_objects_current == 1U);
    const auto curve_hit =
        widget.querySketchPresentation(center);
    CHECK(curve_hit.completed);
    CHECK(curve_hit.token.has_value());
    CHECK(*curve_hit.token == curve_token);

    // SR-02 manual regression: aggregated AIS_Shape curves must receive the
    // same semantic Regular/Construction styling as AIS_Line objects. The
    // concrete-provider diagnostic proves the native wire aspect path is
    // exercised on initial display and on later selection/hover restyles.
    const viewer::PresentationToken
        construction_curve_token{0x4202U};
    viewer::SketchScene curve_style_scene;
    curve_style_scene.curves.push_back(
        viewer::SketchCurvePresentation{
            curve_token,
            {
                {-20.0, -6.0, 0.0},
                {-10.0, -6.0, 0.0},
                {0.0, -6.0, 0.0},
                {10.0, -6.0, 0.0},
                {20.0, -6.0, 0.0},
            },
            false});
    curve_style_scene.curves.push_back(
        viewer::SketchCurvePresentation{
            construction_curve_token,
            {
                {-20.0, 6.0, 0.0},
                {-10.0, 6.0, 0.0},
                {0.0, 6.0, 0.0},
                {10.0, 6.0, 0.0},
                {20.0, 6.0, 0.0},
            },
            true});
    CHECK(curve_style_scene.valid());

    // Isolate this diagnostic from the previous one-curve scene. The
    // authored-scene setter first clears transient Measure cues, whose
    // cleanup intentionally reapplies styles to the currently installed
    // Sketch objects before replacing them.
    CHECK(widget.setSketchScene(
        viewer::SketchScene{}));
    widget.resetRuntimeDiagnostics();
    CHECK(widget.setSketchScene(
        curve_style_scene));
    CHECK(
        widget.runtimeDiagnostics().
            sketch_wire_style_applications == 2U);

    widget.resetRuntimeDiagnostics();
    CHECK(widget.setPresentationSelection(
        viewer::PresentationSelection{
            {curve_token},
            curve_token}));
    CHECK(
        widget.runtimeDiagnostics().
            sketch_wire_style_applications == 2U);

    viewer::SketchInteractionPresentation
        curve_hover;
    curve_hover.hovered_entity =
        construction_curve_token;
    widget.resetRuntimeDiagnostics();
    CHECK(widget.setSketchInteractionPresentation(
        curve_hover));
    CHECK(
        widget.runtimeDiagnostics().
            sketch_wire_style_applications == 2U);

    // Package F: Construction is provider presentation only but must be
    // visibly distinct even when nothing is selected. Replacing the drawer
    // aspect after Display() requires Redisplay() for the Sketch object.
    viewer::SketchScene role_scene;
    role_scene.lines.push_back(
        viewer::SketchLinePresentation{
            viewer::PresentationToken{0x4101U},
            {-12.0, -4.0, 0.0},
            {12.0, -4.0, 0.0},
            false});
    role_scene.lines.push_back(
        viewer::SketchLinePresentation{
            viewer::PresentationToken{0x4102U},
            {-12.0, 4.0, 0.0},
            {12.0, 4.0, 0.0},
            true});
    CHECK(widget.setSketchScene(role_scene));
    CHECK(widget.setPresentationSelection(
        viewer::PresentationSelection{}));
    QApplication::processEvents();

    // Re-apply an empty interaction presentation to exercise the same style
    // path used after hover/selection clears; it must preserve construction
    // semantics and remain a valid native provider update.
    CHECK(widget.setSketchInteractionPresentation(
        viewer::SketchInteractionPresentation{}));
    QApplication::processEvents();

    // Package F: the native provider renders Profile regions as shaded
    // planar faces, preserves holes, and maps native detection back to
    // the neutral presentation token. The provider token remains runtime
    // presentation identity only; Part maps it immediately to ProfileId.
    CHECK(widget.setCameraState(camera));
    CHECK(widget.setReferenceScene(
        viewer::ReferenceScene{}));
    CHECK(widget.setSketchScene(
        viewer::SketchScene{}));

    const viewer::PresentationToken
        profile_token{0x5101U};
    viewer::ProfileRegionPresentation
        profile_region;
    profile_region.outer = {
        {-20.0, -20.0, 0.0},
        {20.0, -20.0, 0.0},
        {20.0, 20.0, 0.0},
        {-20.0, 20.0, 0.0},
    };
    profile_region.holes.push_back({
        {-4.0, -4.0, 0.0},
        {-4.0, 4.0, 0.0},
        {4.0, 4.0, 0.0},
        {4.0, -4.0, 0.0},
    });

    viewer::ProfileScene profile_scene;
    profile_scene.profiles.push_back(
        viewer::ProfilePresentation{
            profile_token,
            profile_region});
    CHECK(widget.setProfileScene(profile_scene));
    CHECK(widget.setPresentationSelection(
        viewer::PresentationSelection{
            {profile_token},
            profile_token}));

    viewer::ProfilePreviewScene
        profile_preview;
    profile_preview.region = profile_region;

    widget.resetRuntimeDiagnostics();
    CHECK(widget.setProfilePreviewScene(
        profile_preview));
    const auto changed_profile_preview_metrics =
        widget.runtimeDiagnostics();
    CHECK(
        changed_profile_preview_metrics.
            update_current_viewer_calls == 1U);
    CHECK(
        changed_profile_preview_metrics.redraw_calls == 0U);

    widget.resetRuntimeDiagnostics();
    CHECK(widget.setProfilePreviewScene(
        viewer::ProfilePreviewScene{}));
    const auto cleared_profile_preview_metrics =
        widget.runtimeDiagnostics();
    CHECK(
        cleared_profile_preview_metrics.
            update_current_viewer_calls == 1U);
    CHECK(
        cleared_profile_preview_metrics.redraw_calls == 0U);

    const QPoint center_point{
        widget.width() / 2,
        widget.height() / 2};
    QTest::mouseClick(
        &widget,
        Qt::LeftButton,
        Qt::NoModifier,
        center_point);
    QApplication::processEvents();
    CHECK(selection_intents == 1);
    CHECK(last_selection_intent.has_value());
    CHECK(
        last_selection_intent->mode ==
        viewer::SelectionIntentMode::clear);

    last_selection_intent.reset();
    QTest::mouseClick(
        &widget,
        Qt::LeftButton,
        Qt::NoModifier,
        QPoint{
            center_point.x() + 60,
            center_point.y()});
    QApplication::processEvents();
    CHECK(selection_intents == 2);
    CHECK(last_selection_intent.has_value());
    CHECK(
        last_selection_intent->mode ==
        viewer::SelectionIntentMode::replace);
    CHECK(
        last_selection_intent->token ==
        profile_token);

    CHECK(widget.setProfileScene(
        viewer::ProfileScene{}));
    CHECK(widget.setProfilePreviewScene(
        viewer::ProfilePreviewScene{}));

    return EXIT_SUCCESS;
}
