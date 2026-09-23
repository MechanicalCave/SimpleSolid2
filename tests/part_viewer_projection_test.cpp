#include "part_viewer_projection.hpp"

#include <simplesolid2/part/part_document.hpp>

#include <cstdlib>
#include <iostream>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "WB-01 Part Viewer projection CHECK failed at line "
            << line << ": " << expression << '\n';
        std::abort();
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

const viewer::ReferencePresentation* findReference(
    const viewer::ReferenceScene& scene,
    core::BuiltinReferenceRole role) {
    const auto token =
        ui::presentationTokenFor(role);

    for (const auto& item : scene.references) {
        if (item.token == token) {
            return &item;
        }
    }

    return nullptr;
}

} // namespace

int main() {
    auto document =
        part::PartDocument::create(
            core::DocumentId::generate());

    for (const auto role :
         core::builtin_reference_roles) {
        const auto token =
            ui::presentationTokenFor(role);
        CHECK(token.valid());

        const auto restored =
            ui::builtinReferenceFor(token);
        CHECK(restored.has_value());
        CHECK(*restored == role);
    }

    CHECK(!ui::builtinReferenceFor(
               viewer::PresentationToken{3U})
               .has_value());
    CHECK(!ui::builtinReferenceFor(
               viewer::PresentationToken{128U})
               .has_value());

    auto selection =
        ui::DocumentSelectionState::documentRootOnly();

    auto scene =
        ui::partReferenceScene(
            document,
            selection);

    CHECK(scene.references.size() == 7U);

    const auto* point =
        findReference(
            scene,
            core::BuiltinReferenceRole::origin_point);
    const auto* x_axis =
        findReference(
            scene,
            core::BuiltinReferenceRole::x_axis);
    const auto* xy_plane =
        findReference(
            scene,
            core::BuiltinReferenceRole::xy_plane);

    CHECK(point != nullptr);
    CHECK(x_axis != nullptr);
    CHECK(xy_plane != nullptr);

    CHECK(point->visible);
    CHECK(x_axis->visible);
    CHECK(!xy_plane->visible);

    CHECK(point->role ==
          viewer::PresentationRole::base);
    CHECK(x_axis->role ==
          viewer::PresentationRole::base);

    const auto x_target =
        ui::DocumentSelectionTarget::builtinReference(
            core::BuiltinReferenceRole::x_axis);
    const auto point_target =
        ui::DocumentSelectionTarget::builtinReference(
            core::BuiltinReferenceRole::origin_point);

    selection.selected = {
        x_target,
        point_target};
    selection.primary = point_target;
    CHECK(selection.valid());

    scene =
        ui::partReferenceScene(
            document,
            selection);

    x_axis =
        findReference(
            scene,
            core::BuiltinReferenceRole::x_axis);
    point =
        findReference(
            scene,
            core::BuiltinReferenceRole::origin_point);

    CHECK(x_axis != nullptr);
    CHECK(point != nullptr);
    CHECK(x_axis->role ==
          viewer::PresentationRole::secondary_selection);
    CHECK(point->role ==
          viewer::PresentationRole::primary_selection);

    {
        part::PartDocumentTransaction tx{document};
        CHECK(tx.setBuiltinReferenceVisible(
            core::BuiltinReferenceRole::origin_point,
            false));
        CHECK(tx.commit().changed);
    }

    scene =
        ui::partReferenceScene(
            document,
            selection);

    point =
        findReference(
            scene,
            core::BuiltinReferenceRole::origin_point);
    CHECK(point != nullptr);
    CHECK(!point->visible);
    CHECK(point->role ==
          viewer::PresentationRole::primary_selection);

    const auto grid =
        ui::defaultPartReferenceGrid();
    CHECK(grid.valid());
    CHECK(grid.visible);
    CHECK(grid.spacing > 0.0);
    CHECK(grid.major_step > 0U);

    ui::DocumentSelectionState invalid;
    invalid.selected = {x_target};
    invalid.primary = point_target;
    CHECK(!invalid.valid());

    return EXIT_SUCCESS;
}
