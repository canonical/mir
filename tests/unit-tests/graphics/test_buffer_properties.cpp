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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/graphics/buffer_properties.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mg = mir::graphics;
namespace geom = mir::geometry;

TEST(buffer_properties, default_create)
{
    geom::Size size;
    geom::PixelFormat pixel_format{geom::PixelFormat::invalid};
    mg::BufferUsage usage{mg::BufferUsage::undefined};

    mg::BufferProperties prop;

    EXPECT_EQ(size, prop.size);
    EXPECT_EQ(pixel_format, prop.format);
    EXPECT_EQ(usage, prop.usage);
}

TEST(buffer_properties, custom_create)
{
    geom::Size size{66, 166};
    geom::PixelFormat pixel_format{geom::PixelFormat::abgr_8888};
    mg::BufferUsage usage{mg::BufferUsage::hardware};

    mg::BufferProperties prop{size, pixel_format, usage};

    EXPECT_EQ(size, prop.size);
    EXPECT_EQ(pixel_format, prop.format);
    EXPECT_EQ(usage, prop.usage);
}

TEST(buffer_properties, equal_properties_test_equal)
{
    geom::Size size{66, 166};
    geom::PixelFormat pixel_format{geom::PixelFormat::abgr_8888};
    mg::BufferUsage usage{mg::BufferUsage::hardware};

    mg::BufferProperties prop0{size, pixel_format, usage};
    mg::BufferProperties prop1{size, pixel_format, usage};

    EXPECT_EQ(prop0, prop0);
    EXPECT_EQ(prop1, prop1);
    EXPECT_EQ(prop0, prop1);
    EXPECT_EQ(prop1, prop0);
}

TEST(buffer_properties, unequal_properties_test_unequal)
{
    geom::Size size[2] =
    {
        {geom::Width{66}, geom::Height{166}},
        {geom::Width{67}, geom::Height{166}}
    };

    geom::PixelFormat pixel_format[2] =
    {
        geom::PixelFormat::abgr_8888,
        geom::PixelFormat::bgr_888
    };

    mg::BufferUsage usage[2] =
    {
        mg::BufferUsage::hardware,
        mg::BufferUsage::software
    };

    mg::BufferProperties prop000{size[0], pixel_format[0], usage[0]};

    /* This approach doesn't really scale, but it's good enough for now */
    for (int s = 0; s < 2; s++)
    {
        for (int p = 0; p < 2; p++)
        {
            for (int u = 0; u < 2; u++)
            {
                mg::BufferProperties prop{size[s], pixel_format[p], usage[u]};
                if (s != 0 || p != 0 || u != 0)
                {
                    EXPECT_NE(prop000, prop);
                    EXPECT_NE(prop, prop000);
                }
                else
                {
                    EXPECT_EQ(prop000, prop);
                    EXPECT_EQ(prop, prop000);
                }
            }
        }
    }
}
