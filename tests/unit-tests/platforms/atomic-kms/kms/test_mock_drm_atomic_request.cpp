/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3,
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

#include <mir/test/doubles/mock_drm.h>

#include <gtest/gtest.h>

#include <memory>

namespace mtd = mir::test::doubles;

using namespace testing;

TEST(MockDRMAtomicRequest, add_property_returns_one_based_cursor_including_duplicate_additions)
{
    NiceMock<mtd::MockDRM> mock_drm;
    std::unique_ptr<drmModeAtomicReq, decltype(&drmModeAtomicFree)> request{
        drmModeAtomicAlloc(), &drmModeAtomicFree};
    ASSERT_NE(request, nullptr);

    constexpr mtd::AtomicPropertyKey framebuffer_property{40, 311};
    constexpr uint64_t first_framebuffer{77};
    constexpr uint64_t next_framebuffer{88};
    constexpr uint64_t final_framebuffer{99};

    EXPECT_EQ(1, drmModeAtomicAddProperty(
        request.get(), framebuffer_property.object_id, framebuffer_property.property_id, first_framebuffer));
    EXPECT_EQ(2, drmModeAtomicAddProperty(
        request.get(), framebuffer_property.object_id, framebuffer_property.property_id, next_framebuffer));
    EXPECT_EQ(3, drmModeAtomicAddProperty(
        request.get(), framebuffer_property.object_id, framebuffer_property.property_id, final_framebuffer));
    EXPECT_THAT(
        request.get(), mtd::AtomicRequestWith(ElementsAre(Pair(framebuffer_property, final_framebuffer))));
}
