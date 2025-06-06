/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "mir/test/doubles/mock_display.h"
#include "mir/test/doubles/mock_cursor.h"
#include "mir/test/doubles/stub_cursor_image.h"
#include "mir/test/doubles/stub_renderable.h"
#include "src/server/graphics/multiplexing_hw_cursor.h"

using namespace testing;
namespace mg = mir::graphics;
namespace mtd = mir::test::doubles;
namespace geom = mir::geometry;

class MultiplexingCursorTest : public Test
{
};

TEST_F(MultiplexingCursorTest, can_show_cursors)
{
    auto const display = std::make_unique<mtd::MockDisplay>();
    auto const cursor = std::make_shared<mtd::MockCursor>();
    auto const cursor_image = std::make_shared<mtd::StubCursorImage>();
    EXPECT_CALL(*display, create_hardware_cursor)
        .WillOnce(Return(cursor));
    EXPECT_CALL(*cursor, show(Eq(cursor_image)));

    std::vector<mg::Display*> displays = { display.get() };
    mg::MultiplexingCursor multiplexing_cursor(displays);
    multiplexing_cursor.show(cursor_image);
}

TEST_F(MultiplexingCursorTest, can_hide_cursors)
{
    auto const display = std::make_unique<mtd::MockDisplay>();
    auto const cursor = std::make_shared<mtd::MockCursor>();
    EXPECT_CALL(*display, create_hardware_cursor)
        .WillOnce(Return(cursor));
    EXPECT_CALL(*cursor, hide());

    std::vector<mg::Display*> displays = { display.get() };
    mg::MultiplexingCursor multiplexing_cursor(displays);
    multiplexing_cursor.hide();
}

TEST_F(MultiplexingCursorTest, can_move_cursors)
{
    auto const display = std::make_unique<mtd::MockDisplay>();
    auto const cursor = std::make_shared<mtd::MockCursor>();
    geom::Point position(100, 100);
    EXPECT_CALL(*display, create_hardware_cursor)
        .WillOnce(Return(cursor));
    EXPECT_CALL(*cursor, move_to(Eq(position)));

    std::vector<mg::Display*> displays = { display.get() };
    mg::MultiplexingCursor multiplexing_cursor(displays);
    multiplexing_cursor.move_to(position);
}

TEST_F(MultiplexingCursorTest, can_scale_cursors)
{
    auto const display = std::make_unique<mtd::MockDisplay>();
    auto const cursor = std::make_shared<mtd::MockCursor>();
    float constexpr scale = 2.f;
    EXPECT_CALL(*display, create_hardware_cursor)
        .WillOnce(Return(cursor));
    EXPECT_CALL(*cursor, scale(Eq(scale)));

    std::vector<mg::Display*> displays = { display.get() };
    mg::MultiplexingCursor multiplexing_cursor(displays);
    multiplexing_cursor.scale(scale);
}

TEST_F(MultiplexingCursorTest, renderable_is_not_null_with_cursors)
{
    auto const display = std::make_unique<mtd::MockDisplay>();
    auto const cursor = std::make_shared<mtd::MockCursor>();
    EXPECT_CALL(*display, create_hardware_cursor)
        .WillOnce(Return(cursor));
    EXPECT_CALL(*cursor, renderable())
        .WillOnce(Return(std::make_shared<mtd::StubRenderable>()));

    std::vector<mg::Display*> displays = { display.get() };
    mg::MultiplexingCursor multiplexing_cursor(displays);
    EXPECT_THAT(multiplexing_cursor.renderable(), NotNull());
}

TEST_F(MultiplexingCursorTest, needs_compositing_is_first_cursor_value_with_cursors)
{
    auto const display = std::make_unique<mtd::MockDisplay>();
    auto const cursor = std::make_shared<mtd::MockCursor>();
    EXPECT_CALL(*display, create_hardware_cursor)
        .WillOnce(Return(cursor));
    EXPECT_CALL(*cursor, needs_compositing())
        .WillOnce(Return(true));

    std::vector<mg::Display*> displays = { display.get() };
    mg::MultiplexingCursor multiplexing_cursor(displays);
    EXPECT_THAT(multiplexing_cursor.needs_compositing(), Eq(true));
}
