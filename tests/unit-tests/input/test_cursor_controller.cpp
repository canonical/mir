/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "src/server/input/cursor_controller.h"

#include "mir/input/surface.h"
#include "mir/input/input_targets.h"
#include "mir/scene/observer.h"
#include "mir/scene/surface_observer.h"
#include "mir/graphics/cursor_image.h"
#include "mir/graphics/cursor.h"

#include "mir_test/fake_shared.h"
#include "mir_test_doubles/stub_scene_surface.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <initializer_list>
#include <mutex>
#include <algorithm>

#include <assert.h>

namespace mi = mir::input;
namespace mg = mir::graphics;
namespace ms = mir::scene;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mt::doubles;

namespace
{

std::string const default_cursor_name = "default";

struct NamedCursorImage : public mg::CursorImage
{
    NamedCursorImage(std::string const& name)
        : cursor_name(name)
    {
    }
    
    void const* as_argb_8888() const { return nullptr; }
    geom::Size size() const { return geom::Size{}; }

    std::string const cursor_name;
};

bool cursor_is_named(mg::CursorImage const& i, std::string const& name)
{
    auto image = dynamic_cast<NamedCursorImage const*>(&i);
    assert(image);
    
    return image->cursor_name == name;
}

MATCHER(DefaultCursorImage, "")
{
    return cursor_is_named(arg, default_cursor_name);
}

MATCHER_P(CursorNamed, name, "")
{
    return cursor_is_named(arg, name);
}

struct MockCursor : public mg::Cursor
{
    MOCK_METHOD1(show, void(mg::CursorImage const&));
    MOCK_METHOD0(hide, void());
    
    MOCK_METHOD1(move_to, void(geom::Point));
};

// TODO: Override qualifier
// TODO: Comment about stub scene surface
struct StubInputSurface : public mtd::StubSceneSurface
{
    StubInputSurface(geom::Rectangle const& input_bounds, std::shared_ptr<mg::CursorImage> const& cursor_image)
        : mtd::StubSceneSurface(0),
          bounds(input_bounds),
          cursor_image_(cursor_image)
    {
    }
    
    std::string name() const override
    {
        return std::string();
    }
    
    geom::Rectangle input_bounds() const override
    {
        // We could return the bounds here but the cursor controller
        // should only use the input_area_contains method.
        return geom::Rectangle();
    }
    
    bool input_area_contains(geom::Point const& point) const override
    {
        return bounds.contains(point);
    }
    
    std::shared_ptr<mi::InputChannel> input_channel() const override
    {
        return nullptr;
    }
    
    mi::InputReceptionMode reception_mode() const override
    {
        return mi::InputReceptionMode::normal;
    }

    std::shared_ptr<mg::CursorImage> cursor_image() const override
    {
        return cursor_image_;
    }

    void set_cursor_image(std::shared_ptr<mg::CursorImage> const& image)
    {
        // TODO: May need guard.
        cursor_image_ = image;
        
        {
            std::unique_lock<decltype(observer_guard)> lk(observer_guard);
            for (auto o : observers)
                o->cursor_image_set_to(image);
        }
    }

    void add_observer(std::shared_ptr<ms::SurfaceObserver> const& observer)
    {
        std::unique_lock<decltype(observer_guard)> lk(observer_guard);

        observers.push_back(observer);
    }
    void remove_observer(std::weak_ptr<ms::SurfaceObserver> const& observer)
    {
        auto o = observer.lock();
        assert(o);

        auto it = std::find(observers.begin(), observers.end(), o);
        observers.erase(it);
    }

    geom::Rectangle const bounds;
    std::shared_ptr<mg::CursorImage> cursor_image_;
    
    std::mutex observer_guard;
    std::vector<std::shared_ptr<ms::SurfaceObserver>> observers;
};

// TODO: Overrides
struct StubInputTargets : public mi::InputTargets
{
    StubInputTargets(std::initializer_list<std::shared_ptr<ms::Surface>> const& targets)
        : targets(targets.begin(), targets.end())
    {
    }
    
    void for_each(std::function<void(std::shared_ptr<mi::Surface> const&)> const& callback)
    {
        for (auto const& target : targets)
            callback(target);
    }
    
    void add_observer(std::shared_ptr<ms::Observer> const& observer)
    {
        std::unique_lock<decltype(observer_guard)> lk(observer_guard);

        observers.push_back(observer);
        
        for (auto target : targets)
        {
            for (auto observer : observers)
            {
                observer->surface_exists(target.get());
            }
        }
    }

    void remove_observer(std::weak_ptr<ms::Observer> const& observer)
    {
        std::unique_lock<decltype(observer_guard)> lk(observer_guard);
        
        auto o = observer.lock();
        assert(o);

        auto it = std::find(observers.begin(), observers.end(), o);
        observers.erase(it);
    }
    
    void add_surface(std::shared_ptr<StubInputSurface> const& surface)
    {
        targets.push_back(surface);
        for (auto observer : observers)
        {
            observer->surface_added(surface.get());
        }
    }
    
    // TODO: Probably needs mutex
    // TODO: Only needs one observer...
    // TODO: Should be mi::Surface
    std::vector<std::shared_ptr<ms::Surface>> targets;

    std::mutex observer_guard;

    std::vector<std::shared_ptr<ms::Observer>> observers;
};

}

TEST(CursorController, moves_cursor)
{
    using namespace ::testing;

    StubInputTargets targets({});
    MockCursor cursor;
    
    mi::CursorController controller(mt::fake_shared(targets),
        mt::fake_shared(cursor), std::make_shared<NamedCursorImage>(default_cursor_name));

    // TODO: Ensure warnings dissapear

    InSequence seq;
    EXPECT_CALL(cursor, move_to(geom::Point{geom::X{1.0f}, geom::Y{1.0f}}));
    EXPECT_CALL(cursor, move_to(geom::Point{geom::X{0.0f}, geom::Y{0.0f}}));
    
    controller.cursor_moved_to(1.0f, 1.0f);
    controller.cursor_moved_to(0.0f, 0.0f);
}

TEST(CursorController, updates_cursor_image_when_entering_surface)
{
    using namespace ::testing;

    std::string const cursor_name = "test_cursor";

    StubInputSurface surface{geom::Rectangle{geom::Point{geom::X{1}, geom::Y{1}},
                                  geom::Size{geom::Width{1}, geom::Height{1}}},
                             std::make_shared<NamedCursorImage>(cursor_name)};
    StubInputTargets targets({mt::fake_shared(surface)});
    MockCursor cursor;
    
    mi::CursorController controller(mt::fake_shared(targets),
        mt::fake_shared(cursor), std::make_shared<NamedCursorImage>(default_cursor_name));

    EXPECT_CALL(cursor, move_to(_)).Times(AnyNumber());
    EXPECT_CALL(cursor, show(CursorNamed(cursor_name))).Times(1);

    controller.cursor_moved_to(1.0f, 1.0f);
}

TEST(CursorController, surface_with_no_cursor_image_hides_cursor)
{
    using namespace ::testing;

    StubInputSurface surface{geom::Rectangle{geom::Point{geom::X{1}, geom::Y{1}},
                                  geom::Size{geom::Width{1}, geom::Height{1}}},
                             nullptr};
    StubInputTargets targets({mt::fake_shared(surface)});
    MockCursor cursor;
    
    mi::CursorController controller(mt::fake_shared(targets),
        mt::fake_shared(cursor), std::make_shared<NamedCursorImage>(default_cursor_name));

    EXPECT_CALL(cursor, move_to(_)).Times(AnyNumber());
    EXPECT_CALL(cursor, hide()).Times(1);

    controller.cursor_moved_to(1.0f, 1.0f);
}

TEST(CursorController, takes_cursor_image_from_topmost_surface)
{
    using namespace ::testing;

    std::string const surface_1_cursor_name = "test_cursor_1";
    std::string const surface_2_cursor_name = "test_cursor_2";
    
    geom::Rectangle const bounds{geom::Point{geom::X{1}, geom::Y{1}},
            geom::Size{geom::Width{1}, geom::Height{1}}};

    StubInputSurface surface_1{bounds, std::make_shared<NamedCursorImage>(surface_1_cursor_name)};
    StubInputSurface surface_2{bounds, std::make_shared<NamedCursorImage>(surface_2_cursor_name)};
    StubInputTargets targets({mt::fake_shared(surface_1), mt::fake_shared(surface_2)});
    MockCursor cursor;
    
    mi::CursorController controller(mt::fake_shared(targets),
        mt::fake_shared(cursor), std::make_shared<NamedCursorImage>(default_cursor_name));

    EXPECT_CALL(cursor, move_to(_)).Times(AnyNumber());
    EXPECT_CALL(cursor, show(CursorNamed(surface_2_cursor_name))).Times(1);

    controller.cursor_moved_to(1.0f, 1.0f);
}

TEST(CursorController, restores_cursor_when_leaving_surface)
{
    using namespace ::testing;

    std::string const cursor_name = "test_cursor";

    StubInputSurface surface{geom::Rectangle{geom::Point{geom::X{1}, geom::Y{1}},
                                  geom::Size{geom::Width{1}, geom::Height{1}}},
                             std::make_shared<NamedCursorImage>(cursor_name)};
    StubInputTargets targets({mt::fake_shared(surface)});
    MockCursor cursor;
    
    mi::CursorController controller(mt::fake_shared(targets),
        mt::fake_shared(cursor), std::make_shared<NamedCursorImage>(default_cursor_name));

    EXPECT_CALL(cursor, move_to(_)).Times(AnyNumber());

    {
        InSequence seq;
        EXPECT_CALL(cursor, show(CursorNamed(cursor_name))).Times(1);
        EXPECT_CALL(cursor, show(DefaultCursorImage())).Times(1);
    }

    controller.cursor_moved_to(1.0f, 1.0f);
    controller.cursor_moved_to(2.0f, 2.0f);
}

TEST(CursorController, change_in_cursor_request_triggers_image_update_without_cursor_motion)
{
    using namespace ::testing;

    std::string const first_cursor_name = "test_cursor1";
    std::string const second_cursor_name = "test_cursor2";

    StubInputSurface surface{geom::Rectangle{geom::Point{geom::X{1}, geom::Y{1}},
                                  geom::Size{geom::Width{1}, geom::Height{1}}},
                             std::make_shared<NamedCursorImage>(first_cursor_name)};
    StubInputTargets targets({mt::fake_shared(surface)});
    MockCursor cursor;
    
    mi::CursorController controller(mt::fake_shared(targets),
        mt::fake_shared(cursor), std::make_shared<NamedCursorImage>(default_cursor_name));

    EXPECT_CALL(cursor, move_to(_)).Times(AnyNumber());
    {
        InSequence seq;
        EXPECT_CALL(cursor, show(CursorNamed(first_cursor_name))).Times(1);
        EXPECT_CALL(cursor, show(CursorNamed(second_cursor_name))).Times(1);
    }

    controller.cursor_moved_to(1.0f, 1.0f);
    surface.set_cursor_image(std::make_shared<NamedCursorImage>(second_cursor_name));
}

TEST(CursorController, change_in_scene_triggers_image_update)
{
    using namespace ::testing;

    std::string const cursor_name = "test_cursor";

    // Here we also demonstrate that the cursor begins at 0,0.
    StubInputSurface surface{geom::Rectangle{geom::Point{geom::X{0}, geom::Y{0}},
                                  geom::Size{geom::Width{1}, geom::Height{1}}},
                             std::make_shared<NamedCursorImage>(cursor_name)};
    StubInputTargets targets({});
    MockCursor cursor;
    
    mi::CursorController controller(mt::fake_shared(targets),
        mt::fake_shared(cursor), std::make_shared<NamedCursorImage>(default_cursor_name));

    EXPECT_CALL(cursor, move_to(_)).Times(AnyNumber());
    EXPECT_CALL(cursor, show(CursorNamed(cursor_name))).Times(1);

    targets.add_surface(mt::fake_shared(surface));
}

TEST(CursorController, cursor_image_not_reset_needlessly)
{
    using namespace ::testing;

    std::string const cursor_name = "test_cursor";
    auto image = std::make_shared<NamedCursorImage>(cursor_name);

    // Here we also demonstrate that the cursor begins at 0,0.
    StubInputSurface surface1{geom::Rectangle{geom::Point{geom::X{0}, geom::Y{0}},
                                  geom::Size{geom::Width{1}, geom::Height{1}}},
                              image};
    StubInputSurface surface2{geom::Rectangle{geom::Point{geom::X{0}, geom::Y{0}},
                                  geom::Size{geom::Width{1}, geom::Height{1}}},
                              image};
    StubInputTargets targets({});
    MockCursor cursor;
    
    mi::CursorController controller(mt::fake_shared(targets),
        mt::fake_shared(cursor), std::make_shared<NamedCursorImage>(default_cursor_name));

    EXPECT_CALL(cursor, move_to(_)).Times(AnyNumber());
    EXPECT_CALL(cursor, show(CursorNamed(cursor_name))).Times(1);

    targets.add_surface(mt::fake_shared(surface1));
    targets.add_surface(mt::fake_shared(surface2));
}
