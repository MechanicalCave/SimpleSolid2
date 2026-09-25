#include <simplesolid2/sketch/sketch_model.hpp>

#include <iostream>
#include <limits>
#include <stdexcept>

namespace {

#define CHECK(expr) \
    do { \
        if (!(expr)) { \
            std::cerr << "CHECK failed: " #expr \
                      << " at line " << __LINE__ << '\n'; \
            return 1; \
        } \
    } while (false)

template <class Fn>
bool rejectsInvalidArgument(Fn&& fn) {
    try {
        fn();
    } catch (const std::invalid_argument&) {
        return true;
    }
    return false;
}

} // namespace

int main() {
    using namespace simplesolid2::sketch;

    SketchModel model;
    CHECK(model.entityCount() == 0U);
    CHECK(!EntityId{}.valid());

    const auto nan = std::numeric_limits<double>::quiet_NaN();
    const auto inf = std::numeric_limits<double>::infinity();

    CHECK(rejectsInvalidArgument([&] {
        (void)model.addLine(Point2{nan, 0.0}, Point2{1.0, 0.0});
    }));
    CHECK(rejectsInvalidArgument([&] {
        (void)model.addLine(Point2{0.0, 0.0}, Point2{inf, 1.0});
    }));
    CHECK(model.entityCount() == 0U);

    CHECK(rejectsInvalidArgument([&] {
        (void)model.addLine(Point2{2.0, -4.0}, Point2{2.0, -4.0});
    }));
    CHECK(model.entityCount() == 0U);

    const auto first_id =
        model.addLine(Point2{0.0, 0.0}, Point2{10.0, 0.0});
    const auto second_id =
        model.addLine(Point2{10.0, 0.0}, Point2{10.0, 5.0});

    CHECK(first_id.valid());
    CHECK(second_id.valid());
    CHECK(first_id != second_id);
    CHECK(model.entityCount() == 2U);

    const auto* first = model.findLine(first_id);
    const auto* second = model.findLine(second_id);
    CHECK(first != nullptr);
    CHECK(second != nullptr);
    CHECK(first->id() == first_id);
    CHECK((first->start() == Point2{0.0, 0.0}));
    CHECK((first->end() == Point2{10.0, 0.0}));
    CHECK((second->start() == Point2{10.0, 0.0}));
    CHECK((second->end() == Point2{10.0, 5.0}));

    // Equal endpoint coordinates are merely equal values. They do not create
    // another authored object, shared Point identity or persistent relation.
    CHECK(first->end() == second->start());

    auto copied = model;
    CHECK(copied.entityCount() == 2U);
    CHECK(copied.findLine(first_id) != nullptr);
    CHECK(copied.findLine(second_id) != nullptr);

    CHECK(copied.erase(first_id));
    CHECK(copied.entityCount() == 1U);
    CHECK(copied.findLine(first_id) == nullptr);

    // Value-copy state is independent: editing one state cannot alias another.
    CHECK(model.entityCount() == 2U);
    CHECK(model.findLine(first_id) != nullptr);

    CHECK(model.erase(first_id));
    CHECK(model.entityCount() == 1U);
    CHECK(model.findLine(first_id) == nullptr);

    // Storage compaction/reordering must not alter another entity's identity.
    second = model.findLine(second_id);
    CHECK(second != nullptr);
    CHECK(second->id() == second_id);
    CHECK((second->start() == Point2{10.0, 0.0}));
    CHECK((second->end() == Point2{10.0, 5.0}));

    CHECK(!model.erase(first_id));
    CHECK(!model.erase(EntityId{}));
    CHECK(model.entityCount() == 1U);

    const auto third_id =
        model.addLine(Point2{-2.0, 1.0}, Point2{-1.0, 2.0});
    CHECK(third_id.valid());
    CHECK(third_id != first_id);
    CHECK(third_id != second_id);

    // R1-A deliberately has no epsilon/near-zero rejection policy.
    const auto tiny_id =
        model.addLine(Point2{0.0, 0.0}, Point2{1.0e-300, 0.0});
    CHECK(tiny_id.valid());
    CHECK(model.findLine(tiny_id) != nullptr);

    const auto before_invalid = model.entityCount();
    CHECK(rejectsInvalidArgument([&] {
        (void)model.addLine(Point2{7.0, 7.0}, Point2{7.0, 7.0});
    }));
    CHECK(model.entityCount() == before_invalid);

    return 0;
}
