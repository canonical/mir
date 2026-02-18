/*
 * Copyright © Canonical Ltd.
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
 */

#include <mir/graphics/cursor_image.h>
#include <mir/input/cursor_observer_multiplexer.h>

#include <mir/test/doubles/explicit_executor.h>
#include <mir/test/doubles/mock_cursor_observer.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace geom = mir::geometry;
namespace mg = mir::graphics;
namespace mi = mir::input;
namespace mt = mir::test;
namespace mtd = mt::doubles;

namespace
{

struct TestCursorImage : public mg::CursorImage
{
    void const* as_argb_8888() const override { return nullptr; }
    geom::Size size() const override { return geom::Size{16, 16}; }
    geom::Displacement hotspot() const override { return geom::Displacement{0, 0}; }
};

struct TestCursorObserverMultiplexer : testing::Test
{

    TestCursorObserverMultiplexer()
        : cursor_observer_multiplexer{std::make_shared<mi::CursorObserverMultiplexer>(executor)},
          cursor_observer{std::make_shared<testing::NiceMock<mtd::MockCursorObserver>>()},
          cursor_image{std::make_shared<TestCursorImage>()}
    {
    }

    void SetUp() override
    {
    }

    void TearDown() override
    {
        executor.execute();
    }

    mtd::ExplicitExecutor executor;
    std::shared_ptr<mi::CursorObserverMultiplexer> const cursor_observer_multiplexer;
    std::shared_ptr<mtd::MockCursorObserver> const cursor_observer;
    std::shared_ptr<mg::CursorImage> const cursor_image;
};
}

TEST_F(TestCursorObserverMultiplexer, sends_initial_state)
{
    using namespace ::testing;

    static const float x = 100.f;
    static const float y = 100.f;

    cursor_observer_multiplexer->pointer_usable();
    cursor_observer_multiplexer->cursor_moved_to(x, y);
    cursor_observer_multiplexer->image_set_to(cursor_image);

    EXPECT_CALL(*cursor_observer, pointer_usable()).Times(1);
    EXPECT_CALL(*cursor_observer, cursor_moved_to(x, y)).Times(1);
    EXPECT_CALL(*cursor_observer, image_set_to(cursor_image)).Times(1);
    cursor_observer_multiplexer->register_interest(cursor_observer);
    executor.execute();
}
