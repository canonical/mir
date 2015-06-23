/*
 * Copyright © 2015 Canonical Ltd.
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

#include "src/server/graphics/nested/cursor.h"
#include "mir/graphics/cursor_image.h"

#include "mir/test/fake_shared.h"
#include "mir/test/doubles/stub_host_connection.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mg = mir::graphics;
namespace mgn = mg::nested;
namespace geom = mir::geometry;

namespace mt = mir::test;
namespace mtd = mt::doubles;

namespace
{
struct MockHostConnection : public mtd::StubHostConnection
{
    MOCK_METHOD1(set_cursor_image, void(mg::CursorImage const&));
    MOCK_METHOD0(hide_cursor, void());
};

struct StubCursorImage : public mg::CursorImage
{
    void const* as_argb_8888() const { return this; }
    geom::Size size() const { return geom::Size{}; }
    geom::Displacement hotspot() const { return geom::Displacement{0, 0}; }
};

MATCHER_P(CursorImageEquals, image, "")
{
    return arg.as_argb_8888() == image;
}

struct NestedCursor : public testing::Test
{
    StubCursorImage a_cursor_image, another_cursor_image;
    std::shared_ptr<MockHostConnection> connection = std::make_shared<MockHostConnection>();
};
}

TEST_F(NestedCursor, sets_default_cursor_image)
{
    EXPECT_CALL(*connection, set_cursor_image(CursorImageEquals(a_cursor_image.as_argb_8888()))).Times(1);

    mgn::Cursor cursor(connection, mt::fake_shared(a_cursor_image));
}

TEST_F(NestedCursor, can_set_other_images)
{
    EXPECT_CALL(*connection, set_cursor_image(CursorImageEquals(a_cursor_image.as_argb_8888()))).Times(1);
    EXPECT_CALL(*connection, set_cursor_image(CursorImageEquals(another_cursor_image.as_argb_8888()))).Times(1);

    mgn::Cursor cursor(connection, mt::fake_shared(a_cursor_image));
    cursor.show(another_cursor_image);
}

TEST_F(NestedCursor, hides_cursor)
{
    using namespace ::testing;
    EXPECT_CALL(*connection, set_cursor_image(CursorImageEquals(a_cursor_image.as_argb_8888()))).Times(1);
    EXPECT_CALL(*connection, hide_cursor()).Times(1);
    EXPECT_CALL(*connection, set_cursor_image(CursorImageEquals(another_cursor_image.as_argb_8888()))).Times(1);

    mgn::Cursor cursor(connection, mt::fake_shared(a_cursor_image));
    cursor.hide();
    cursor.show(another_cursor_image);
}
