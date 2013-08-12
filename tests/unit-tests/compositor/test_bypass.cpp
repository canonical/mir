/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "src/server/compositor/bypass.h"
#include "mir_test_doubles/mock_display_buffer.h"
#include "mir_test_doubles/stub_buffer_stream.h"
#include <glm/gtc/matrix_transform.hpp>

#include <gtest/gtest.h>
#include <memory>

using namespace testing;
using namespace mir::geometry;
using namespace mir::compositor;
using namespace mir::test::doubles;

class TestWindow : public CompositingCriteria
{
public:
    TestWindow(int x, int y, int width, int height)
        : rect{{x, y}, {width, height}}
    {
        const glm::mat4 ident;
        glm::vec3 size(width, height, 0.0f);
        glm::vec3 pos(x + width / 2, y + height / 2, 0.0f);
        trans = glm::scale( glm::translate(ident, pos), size);
    }

    float alpha() const override
    {
        return 1.0f;
    }

    glm::mat4 const& transformation() const override
    {
        return trans;
    }

    bool should_be_rendered_in(const Rectangle &r) const override
    {
        return rect.overlaps(r);
    }

private:
    Rectangle rect;
    glm::mat4 trans;
};

struct BypassFilterTest : public Test
{
    BypassFilterTest()
    {
        monitor_rect.top_left = {0, 0};
        monitor_rect.size = {1920, 1200};

        EXPECT_CALL(display_buffer, view_area())
            .WillRepeatedly(Return(monitor_rect));
    }

    Rectangle monitor_rect;
    MockDisplayBuffer display_buffer;
};

TEST_F(BypassFilterTest, nothing_matches_nothing)
{
    BypassFilter filter(display_buffer);
    BypassMatch match;

    EXPECT_FALSE(filter.fullscreen_on_top());
    EXPECT_FALSE(match.topmost_fullscreen());
}

TEST_F(BypassFilterTest, small_window_not_bypassed)
{
    BypassFilter filter(display_buffer);

    TestWindow win(12, 34, 56, 78);

    EXPECT_FALSE(filter(win));
    EXPECT_FALSE(filter.fullscreen_on_top());
}

TEST_F(BypassFilterTest, single_fullscreen_window_bypassed)
{
    BypassFilter filter(display_buffer);

    TestWindow win(0, 0, 1920, 1200);

    EXPECT_TRUE(filter(win));
    EXPECT_TRUE(filter.fullscreen_on_top());
}

TEST_F(BypassFilterTest, offset_fullscreen_window_not_bypassed)
{
    BypassFilter filter(display_buffer);

    TestWindow win(10, 50, 1920, 1200);

    EXPECT_FALSE(filter(win));
    EXPECT_FALSE(filter.fullscreen_on_top());
}

TEST_F(BypassFilterTest, obscured_fullscreen_window_not_bypassed)
{
    BypassFilter filter(display_buffer);

    TestWindow fs(0, 0, 1920, 1200);
    TestWindow small(20, 30, 40, 50);

    EXPECT_TRUE(filter(fs));
    EXPECT_FALSE(filter(small));
    EXPECT_FALSE(filter.fullscreen_on_top());
}

TEST_F(BypassFilterTest, unobscured_fullscreen_window_bypassed)
{
    BypassFilter filter(display_buffer);

    TestWindow fs(0, 0, 1920, 1200);
    TestWindow small(20, 30, 40, 50);

    EXPECT_FALSE(filter(small));
    EXPECT_TRUE(filter(fs));
    EXPECT_TRUE(filter.fullscreen_on_top());
}

TEST(BypassMatchTest, defaults_to_null)
{
    BypassMatch match;

    EXPECT_EQ(nullptr, match.topmost_fullscreen());
}

TEST(BypassMatchTest, returns_one)
{
    BypassMatch match;
    TestWindow win(1, 2, 3, 4);
    StubBufferStream bufs;

    EXPECT_EQ(nullptr, match.topmost_fullscreen());
    match(win, bufs);
    EXPECT_EQ(&bufs, match.topmost_fullscreen());
}

TEST(BypassMatchTest, returns_latest)
{
    BypassMatch match;
    TestWindow a(1, 2, 3, 4), b(5, 6, 7, 8), c(9, 10, 11, 12);
    StubBufferStream abuf, bbuf, cbuf;

    EXPECT_EQ(nullptr, match.topmost_fullscreen());
    match(a, abuf);
    EXPECT_EQ(&abuf, match.topmost_fullscreen());
    match(b, bbuf);
    EXPECT_EQ(&bbuf, match.topmost_fullscreen());
    match(c, cbuf);
    EXPECT_EQ(&cbuf, match.topmost_fullscreen());
    EXPECT_EQ(&cbuf, match.topmost_fullscreen());
    EXPECT_EQ(&cbuf, match.topmost_fullscreen());
}

