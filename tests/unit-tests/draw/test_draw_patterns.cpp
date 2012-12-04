/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir_client/mir_client_library.h"
#include "mir/draw/patterns.h"

#include <gtest/gtest.h>

namespace md=mir::draw;

class DrawPatternsTest : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        test_region = std::make_shared<MirGraphicsRegion>();
        int bytes_pp = 4;
        test_region->width  = 50;
        test_region->height = 100;
        test_region->stride = bytes_pp*100;
        test_region->pixel_format = mir_pixel_format_rgba_8888;
        test_region->vaddr = (char*) malloc(sizeof(char) * bytes_pp * test_region->height * test_region->stride);
    }
    virtual void TearDown()
    {
        free(test_region->vaddr);
    }

    std::shared_ptr<MirGraphicsRegion> test_region;
};

TEST_F(DrawPatternsTest, solid_color_unaccelerated)
{
    md::DrawPatternSolid pattern(0x43214321);
    pattern.draw(test_region);
    EXPECT_TRUE(pattern.check(test_region));  
}

TEST_F(DrawPatternsTest, solid_color_unaccelerated_error)
{
    md::DrawPatternSolid pattern(0x43214321);
    pattern.draw(test_region);

    test_region->vaddr[test_region->width]++;
    EXPECT_FALSE(pattern.check(test_region));
 
    test_region->vaddr[test_region->width]--;
    EXPECT_TRUE(pattern.check(test_region));  
}

TEST_F(DrawPatternsTest, checkered_pattern)
{
    uint32_t colors [2][2] = {{0x12345678, 0x23456789},
                                    {0x34567890, 0x45678901}}; 
    md::DrawPatternCheckered<2,2> pattern(colors);
    pattern.draw(test_region);
    EXPECT_TRUE(pattern.check(test_region));
}
