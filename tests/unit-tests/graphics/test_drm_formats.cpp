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

#include <mir/graphics/drm_formats.h>

#include <drm_fourcc.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;
namespace mg = mir::graphics;

TEST(DRMFormatBytesPerPixel, two_byte_formats_report_two)
{
    for (auto const fourcc : {DRM_FORMAT_RGB565, DRM_FORMAT_ARGB4444, DRM_FORMAT_XRGB1555, DRM_FORMAT_RGBA5551})
    {
        auto const info = mg::DRMFormat{fourcc}.info();
        ASSERT_THAT(info, Ne(std::nullopt)) << "for format " << mg::DRMFormat{fourcc}.name();
        EXPECT_THAT(info->bytes_per_pixel(), Eq(2u)) << "for format " << mg::DRMFormat{fourcc}.name();
    }
}

TEST(DRMFormatBytesPerPixel, three_byte_formats_report_three)
{
    for (auto const fourcc : {DRM_FORMAT_RGB888, DRM_FORMAT_BGR888})
    {
        auto const info = mg::DRMFormat{fourcc}.info();
        ASSERT_THAT(info, Ne(std::nullopt)) << "for format " << mg::DRMFormat{fourcc}.name();
        EXPECT_THAT(info->bytes_per_pixel(), Eq(3u)) << "for format " << mg::DRMFormat{fourcc}.name();
    }
}

TEST(DRMFormatBytesPerPixel, four_byte_formats_report_four)
{
    for (auto const fourcc : {DRM_FORMAT_ARGB8888, DRM_FORMAT_ABGR8888, DRM_FORMAT_ARGB2101010})
    {
        auto const info = mg::DRMFormat{fourcc}.info();
        ASSERT_THAT(info, Ne(std::nullopt)) << "for format " << mg::DRMFormat{fourcc}.name();
        EXPECT_THAT(info->bytes_per_pixel(), Eq(4u)) << "for format " << mg::DRMFormat{fourcc}.name();
    }
}

TEST(DRMFormatBytesPerPixel, padding_is_counted_for_opaque_formats)
{
    // XRGB8888 only carries 24 bits of colour, but occupies 4 bytes of storage.
    auto const info = mg::DRMFormat{DRM_FORMAT_XRGB8888}.info();
    ASSERT_THAT(info, Ne(std::nullopt));
    EXPECT_THAT(info->bytes_per_pixel(), Eq(4u));
}
