/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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

#include "mir/thread_safe_list.h"
#include "mir/input/surface.h"
#include "mir/input/scene.h"
#include "mir/scene/observer.h"
#include "mir/scene/surface_observer.h"
#include "mir/graphics/cursor_image.h"
#include "mir/graphics/cursor.h"

#include "mir_toolkit/common.h"
#include "mir_toolkit/cursors.h"

#include "mir/test/fake_shared.h"
#include "mir/test/doubles/stub_scene_surface.h"
#include "mir/test/doubles/stub_input_scene.h"

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

struct NamedCursorImage : public mg::CursorImage
{
    NamedCursorImage(std::string const& name)
        : cursor_name(name)
    {
    }

    void const* as_argb_8888() const override { return nullptr; }
    geom::Size size() const override { return geom::Size{16, 16}; }
    geom::Displacement hotspot() const override { return geom::Displacement{0, 0}; }

    std::string const cursor_name;
};

struct ZeroSizedCursorImage : public mg::CursorImage
{
    void const* as_argb_8888() const override { return nullptr; }
    geom::Size size() const override { return geom::Size{}; }
    geom::Displacement hotspot() const override { return geom::Displacement{0, 0}; }
};

bool cursor_is_named(mg::CursorImage const& i, std::string const& name)
{
    auto image = dynamic_cast<NamedCursorImage const*>(&i);
    assert(image);

    return image->cursor_name == name;
}

MATCHER(DefaultCursorImage, "")
{
    return cursor_is_named(arg, mir_default_cursor_name);
}

MATCHER_P(CursorNamed, name, "")
{
    return cursor_is_named(arg, name);
}

struct MockCursor : public mg::Cursor
{
    MOCK_METHOD0(show, void());
    MOCK_METHOD1(show, void(mg::CursorImage const&));
    MOCK_METHOD0(hide, void());

    MOCK_METHOD1(move_to, void(geom::Point));
};

// TODO: This should only inherit from mi::Surface but to use the Scene observer we need an
// ms::Surface base class.
struct StubInputSurface : public mtd::StubSceneSurface
{
    StubInputSurface(geom::Rectangle const& input_bounds, std::shared_ptr<mg::CursorImage> const& cursor_image)
        : mtd::StubSceneSurface{},
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
        // We could return bounds here but lets make sure the cursor controller
        // is only using input_area_contains.
        return geom::Rectangle();
    }

    bool input_area_contains(geom::Point const& point) const override
    {
        return bounds.contains(point);
    }

    mi::InputReceptionMode reception_mode() const override
    {
        return mi::InputReceptionMode::normal;
    }

    std::shared_ptr<mg::CursorImage> cursor_image() const override
    {
        return cursor_image_;
    }

    void set_cursor_image(std::shared_ptr<mg::CursorImage> const& image) override
    {
        cursor_image_ = image;

        {
            std::unique_lock<decltype(observer_guard)> lk(observer_guard);
            for (auto o : observers)
                o->cursor_image_set_to(this, *image);
        }
    }

    void add_observer(std::shared_ptr<ms::SurfaceObserver> const& observer) override
    {
        std::unique_lock<decltype(observer_guard)> lk(observer_guard);

        observers.push_back(observer);
    }

    void remove_observer(std::weak_ptr<ms::SurfaceObserver> const& observer) override
    {
        auto o = observer.lock();
        assert(o);

        auto it = std::find(observers.begin(), observers.end(), o);
        observers.erase(it);
    }

    void post_frame()
    {
        for (auto observer : observers)
        {
            observer->frame_posted(this, 1, geom::Size{0, 0});
        }
    }

    void set_cursor_image_without_notifications(std::shared_ptr<mg::CursorImage> const& image)
    {
        cursor_image_ = image;
    }

    std::experimental::optional<mir::geometry::Rectangle> clip_area() const override
    {
        return std::experimental::optional<mir::geometry::Rectangle>();
    }

    void set_clip_area(std::experimental::optional<mir::geometry::Rectangle> const& /*area*/) override
    {
    }

    geom::Rectangle const bounds;
    std::shared_ptr<mg::CursorImage> cursor_image_;

    std::mutex observer_guard;
    std::vector<std::shared_ptr<ms::SurfaceObserver>> observers;
};

struct StubScene : public mtd::StubInputScene
{
    StubScene(std::initializer_list<std::shared_ptr<ms::Surface>> const& targets)
        : targets(targets.begin(), targets.end())
    {
    }

    void for_each(std::function<void(std::shared_ptr<mi::Surface> const&)> const& callback) override
    {
        for (auto const& target : targets)
            callback(target);
    }

    void add_observer(std::shared_ptr<ms::Observer> const& observer) override
    {
        observers.add(observer);

        for (auto target : targets)
        {
            observers.for_each([&target](std::shared_ptr<ms::Observer> const& observer)
            {
                observer->surface_exists(target);
            });
        }
    }

    void remove_observer(std::weak_ptr<ms::Observer> const& observer) override
    {
        auto o = observer.lock();
        assert(o);

        observers.remove(o);
    }

    void add_surface(std::shared_ptr<StubInputSurface> const& surface)
    {
        targets.push_back(surface);
        observers.for_each([&surface](std::shared_ptr<ms::Observer> const& observer)
        {
            observer->surface_added(surface);
        });
    }

    // TODO: Should be mi::Surface. See comment on StubInputSurface.
    std::vector<std::shared_ptr<ms::Surface>> targets;

    mir::ThreadSafeList<std::shared_ptr<ms::Observer>> observers;
};

struct TestCursorController : public testing::Test
{
    TestCursorController()
        : default_cursor_image(std::make_shared<NamedCursorImage>(mir_default_cursor_name))
    {
    }
    geom::Rectangle const rect_0_0_1_1{{0, 0}, {1, 1}};
    geom::Rectangle const rect_1_1_1_1{{1, 1}, {1, 1}};
    std::string const cursor_name_1 = "test-cursor-1";
    std::string const cursor_name_2 = "test-cursor-2";

    MockCursor cursor;
    std::shared_ptr<mg::CursorImage> const default_cursor_image;
};

}

TEST_F(TestCursorController, moves_cursor)
{
    using namespace ::testing;

    StubScene targets({});

    mi::CursorController controller(mt::fake_shared(targets),
        mt::fake_shared(cursor), default_cursor_image);

    InSequence seq;
    EXPECT_CALL(cursor, move_to(geom::Point{geom::X{1.0f}, geom::Y{1.0f}}));
    EXPECT_CALL(cursor, move_to(geom::Point{geom::X{0.0f}, geom::Y{0.0f}}));

    controller.cursor_moved_to(1.0f, 1.0f);
    controller.cursor_moved_to(0.0f, 0.0f);
}

TEST_F(TestCursorController, updates_cursor_image_when_entering_surface)
{
    using namespace ::testing;

    StubInputSurface surface{rect_1_1_1_1,
        std::make_shared<NamedCursorImage>(cursor_name_1)};
    StubScene targets({mt::fake_shared(surface)});

    mi::CursorController controller(mt::fake_shared(targets),
        mt::fake_shared(cursor), default_cursor_image);

    EXPECT_CALL(cursor, move_to(_)).Times(AnyNumber());
    EXPECT_CALL(cursor, show(CursorNamed(cursor_name_1))).Times(1);

    controller.cursor_moved_to(1.0f, 1.0f);
}

TEST_F(TestCursorController, surface_with_no_cursor_image_hides_cursor)
{
    using namespace ::testing;

    StubInputSurface surface{rect_1_1_1_1,
        nullptr};
    StubScene targets({mt::fake_shared(surface)});

    mi::CursorController controller(mt::fake_shared(targets),
        mt::fake_shared(cursor), default_cursor_image);

    EXPECT_CALL(cursor, move_to(_)).Times(AnyNumber());
    EXPECT_CALL(cursor, hide()).Times(1);

    controller.cursor_moved_to(1.0f, 1.0f);
}

TEST_F(TestCursorController, takes_cursor_image_from_topmost_surface)
{
    using namespace ::testing;

    StubInputSurface surface_1{rect_1_1_1_1, std::make_shared<NamedCursorImage>(cursor_name_1)};
    StubInputSurface surface_2{rect_1_1_1_1, std::make_shared<NamedCursorImage>(cursor_name_2)};
    StubScene targets({mt::fake_shared(surface_1), mt::fake_shared(surface_2)});

    mi::CursorController controller(mt::fake_shared(targets),
        mt::fake_shared(cursor), default_cursor_image);

    EXPECT_CALL(cursor, move_to(_)).Times(AnyNumber());
    EXPECT_CALL(cursor, show(CursorNamed(cursor_name_2))).Times(1);

    controller.cursor_moved_to(1.0f, 1.0f);
}

TEST_F(TestCursorController, restores_cursor_when_leaving_surface)
{
    using namespace ::testing;

    StubInputSurface surface{rect_1_1_1_1,
        std::make_shared<NamedCursorImage>(cursor_name_1)};
    StubScene targets({mt::fake_shared(surface)});

    mi::CursorController controller(mt::fake_shared(targets),
        mt::fake_shared(cursor), default_cursor_image);

    EXPECT_CALL(cursor, move_to(_)).Times(AnyNumber());

    {
        InSequence seq;
        EXPECT_CALL(cursor, show(CursorNamed(cursor_name_1))).Times(1);
        EXPECT_CALL(cursor, show(DefaultCursorImage())).Times(1);
    }

    controller.cursor_moved_to(1.0f, 1.0f);
    controller.cursor_moved_to(2.0f, 2.0f);
}

TEST_F(TestCursorController, change_in_cursor_request_triggers_image_update_without_cursor_motion)
{
    using namespace ::testing;

    StubInputSurface surface{rect_1_1_1_1,
        std::make_shared<NamedCursorImage>(cursor_name_1)};
    StubScene targets({mt::fake_shared(surface)});

    mi::CursorController controller(mt::fake_shared(targets),
        mt::fake_shared(cursor), default_cursor_image);

    EXPECT_CALL(cursor, move_to(_)).Times(AnyNumber());
    {
        InSequence seq;
        EXPECT_CALL(cursor, show(CursorNamed(cursor_name_1))).Times(1);
        EXPECT_CALL(cursor, show(CursorNamed(cursor_name_2))).Times(1);
    }

    controller.cursor_moved_to(1.0f, 1.0f);
    surface.set_cursor_image(std::make_shared<NamedCursorImage>(cursor_name_2));
}

TEST_F(TestCursorController, change_in_scene_triggers_image_update)
{
    using namespace ::testing;

    // Here we also demonstrate that the cursor begins at 0,0.
    StubInputSurface surface{rect_0_0_1_1,
        std::make_shared<NamedCursorImage>(cursor_name_1)};
    StubScene targets({});

    mi::CursorController controller(mt::fake_shared(targets),
        mt::fake_shared(cursor), default_cursor_image);

    EXPECT_CALL(cursor, move_to(_)).Times(AnyNumber());
    EXPECT_CALL(cursor, show(CursorNamed(cursor_name_1))).Times(1);

    targets.add_surface(mt::fake_shared(surface));
}

TEST_F(TestCursorController, cursor_image_not_reset_needlessly)
{
    using namespace ::testing;

    auto image = std::make_shared<NamedCursorImage>(cursor_name_1);

    // Here we also demonstrate that the cursor begins at 0,0.
    StubInputSurface surface1{rect_0_0_1_1, image};
    StubInputSurface surface2{rect_0_0_1_1, image};
    StubScene targets({});

    mi::CursorController controller(mt::fake_shared(targets),
        mt::fake_shared(cursor), default_cursor_image);

    EXPECT_CALL(cursor, move_to(_)).Times(AnyNumber());
    EXPECT_CALL(cursor, show(CursorNamed(cursor_name_1))).Times(1);

    targets.add_surface(mt::fake_shared(surface1));
    targets.add_surface(mt::fake_shared(surface2));
}

TEST_F(TestCursorController, updates_cursor_image_when_surface_posts_first_frame)
{
    using namespace ::testing;

    StubInputSurface surface{rect_0_0_1_1,
        std::make_shared<NamedCursorImage>(cursor_name_1)};
    StubScene targets({mt::fake_shared(surface)});

    // Cursor is set when we first create the controller
    // because of the existing surface
    EXPECT_CALL(cursor, show(CursorNamed(cursor_name_1))).Times(1);

    mi::CursorController controller(mt::fake_shared(targets),
        mt::fake_shared(cursor), default_cursor_image);

    Mock::VerifyAndClearExpectations(&cursor);

    surface.set_cursor_image_without_notifications(
        std::make_shared<NamedCursorImage>(cursor_name_2));

    // First post should lead to cursor update
    EXPECT_CALL(cursor, show(CursorNamed(cursor_name_2))).Times(1);
    surface.post_frame();
    Mock::VerifyAndClearExpectations(&cursor);

    // Second post should have no effect
    EXPECT_CALL(cursor, show(_)).Times(0);
    surface.post_frame();
}

TEST_F(TestCursorController, zero_sized_image_hides_cursor)
{
    using namespace ::testing;

    auto image = std::make_shared<ZeroSizedCursorImage>();

    // Here we also demonstrate that the cursor begins at 0,0.
    StubInputSurface surface{rect_0_0_1_1, image};
    StubScene targets({});

    mi::CursorController controller(mt::fake_shared(targets),
        mt::fake_shared(cursor), default_cursor_image);

    EXPECT_CALL(cursor, move_to(_)).Times(AnyNumber());
    EXPECT_CALL(cursor, hide()).Times(1);

    targets.add_surface(mt::fake_shared(surface));
}
