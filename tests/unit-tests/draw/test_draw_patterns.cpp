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

#include "mir_toolkit/mir_client_library.h"
#include "testdraw/patterns.h"

#include <gtest/gtest.h>
#include <stdexcept>

namespace mtd=mir::test::draw;

class DrawPatternsTest : public ::testing::Test
{
protected:
    DrawPatternsTest()
     : stride_color(0x77)
    {}

    virtual void SetUp()
    {

        test_region.pixel_format = mir_pixel_format_abgr_8888;
        bytes_pp = 4;
        test_region.width  = 100;
        /* misaligned stride to tease out stride problems */
        test_region.stride = (test_region.width * bytes_pp) + 2;
        test_region.height = 50;
        auto region_size =
            sizeof(char) * bytes_pp * test_region.height * test_region.stride;
        region = std::shared_ptr<char>(static_cast<char*>(::operator new(region_size)));
        test_region.vaddr = region.get();

        uint32_t colors[2][2] = {{0x12345678, 0x23456789},
                                 {0x34567890, 0x45678901}};
        memcpy(pattern_colors, colors, sizeof(uint32_t)*4);

        write_stride_region();
    }
    virtual void TearDown()
    {
        EXPECT_TRUE(check_stride_region_unaltered());
    }

    MirGraphicsRegion test_region;
    uint32_t pattern_colors [2][2];
    int bytes_pp;
    std::shared_ptr<char> region;
private:
    /* check that no pattern writes or checks the stride region */
    void write_stride_region()
    {
        for(auto i = 0; i < test_region.height; i++)
        {
            for(auto j = test_region.width * bytes_pp; j < test_region.stride; j++)
            {
                test_region.vaddr[ (i * test_region.stride) + j ] = stride_color;
            }
        }
    }

    bool check_stride_region_unaltered()
    {
        for(auto i=0; i < test_region.height; i++)
        {
            for(auto j=test_region.width * bytes_pp; j < test_region.stride; j++)
            {
                if(test_region.vaddr[ ((i * test_region.stride) + j) ] != stride_color)
                {
                    return false;
                }
            }
        }
        return true;
    }

    const char stride_color;
};

TEST_F(DrawPatternsTest, solid_color_unaccelerated)
{
    mtd::DrawPatternSolid pattern(0x43214321);

    pattern.draw(test_region);
    EXPECT_TRUE(pattern.check(test_region));
}

TEST_F(DrawPatternsTest, solid_color_unaccelerated_error)
{
    mtd::DrawPatternSolid pattern(0x43214321);

    pattern.draw(test_region);
    test_region.vaddr[0]++;
    EXPECT_FALSE(pattern.check(test_region));
}

TEST_F(DrawPatternsTest, solid_bad_pixel_formats)
{
    test_region.pixel_format = mir_pixel_format_xbgr_8888;

    mtd::DrawPatternSolid pattern(0x43214321);

    EXPECT_THROW({
        pattern.draw(test_region);
    }, std::runtime_error);

    EXPECT_THROW({
        pattern.check(test_region);
    }, std::runtime_error);

}

TEST_F(DrawPatternsTest, checkered_pattern)
{
    mtd::DrawPatternCheckered<2,2> pattern(pattern_colors);
    pattern.draw(test_region);
    EXPECT_TRUE(pattern.check(test_region));
}

TEST_F(DrawPatternsTest, checkered_pattern_error)
{
    mtd::DrawPatternCheckered<2,2> pattern(pattern_colors);

    pattern.draw(test_region);
    test_region.vaddr[0]++;
    EXPECT_FALSE(pattern.check(test_region));
}

TEST_F(DrawPatternsTest, checkered_bad_pixel_formats)
{
    test_region.pixel_format = mir_pixel_format_xbgr_8888;

    mtd::DrawPatternCheckered<2,2> pattern(pattern_colors);

    EXPECT_THROW({
        pattern.draw(test_region);
    }, std::runtime_error);

    EXPECT_THROW({
        pattern.check(test_region);
    }, std::runtime_error);

}
