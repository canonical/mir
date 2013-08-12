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

struct BypassTest : public Test
{
    BypassTest()
    {
        monitor_rect.top_left = {0, 0};
        monitor_rect.size = {1920, 1200};

        EXPECT_CALL(display_buffer, view_area())
            .WillRepeatedly(Return(monitor_rect));
    }

    Rectangle monitor_rect;
    MockDisplayBuffer display_buffer;
};

TEST_F(BypassTest, nothing_matches_nothing)
{
    BypassFilter filter(display_buffer);
    BypassMatch match;

    EXPECT_FALSE(filter.fullscreen_on_top());
    EXPECT_FALSE(match.topmost_fullscreen());
}

TEST_F(BypassTest, small_window_not_bypassed)
{
    BypassFilter filter(display_buffer);

    TestWindow win(12, 34, 56, 78);

    EXPECT_FALSE(filter(win));
    EXPECT_FALSE(filter.fullscreen_on_top());
}

TEST_F(BypassTest, single_fullscreen_window_bypassed)
{
    BypassFilter filter(display_buffer);

    TestWindow win(0, 0, 1920, 1200);

    EXPECT_TRUE(filter(win));
    EXPECT_TRUE(filter.fullscreen_on_top());
}

TEST_F(BypassTest, offset_fullscreen_window_not_bypassed)
{
    BypassFilter filter(display_buffer);

    TestWindow win(10, 50, 1920, 1200);

    EXPECT_FALSE(filter(win));
    EXPECT_FALSE(filter.fullscreen_on_top());
}

TEST_F(BypassTest, obscured_fullscreen_window_not_bypassed)
{
    BypassFilter filter(display_buffer);

    TestWindow fs(0, 0, 1920, 1200);
    TestWindow small(20, 30, 40, 50);

    EXPECT_TRUE(filter(fs));
    EXPECT_FALSE(filter(small));
    EXPECT_FALSE(filter.fullscreen_on_top());
}

TEST_F(BypassTest, unobscured_fullscreen_window_bypassed)
{
    BypassFilter filter(display_buffer);

    TestWindow fs(0, 0, 1920, 1200);
    TestWindow small(20, 30, 40, 50);

    EXPECT_FALSE(filter(small));
    EXPECT_TRUE(filter(fs));
    EXPECT_TRUE(filter.fullscreen_on_top());
}
