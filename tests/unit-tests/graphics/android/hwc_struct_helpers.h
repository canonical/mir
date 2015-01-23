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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_TEST_HWC_STRUCT_HELPERS_H_
#define MIR_TEST_HWC_STRUCT_HELPERS_H_

#include "mir/geometry/rectangle.h"
#include "mir/graphics/buffer.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mir
{
namespace test
{
void fill_hwc_layer(
    hwc_layer_1_t& layer,
    hwc_rect_t* visible_rect,
    mir::geometry::Rectangle const& position,
    mir::graphics::Buffer const& buffer,
    int type, int flags);
}
}

void PrintTo(const hwc_rect_t& rect, ::std::ostream* os);
void PrintTo(const hwc_layer_1& layer , ::std::ostream* os);

MATCHER_P2(MatchesMember, value, str,
          std::string("layer's " + std::string(str) + " should be: " + testing::PrintToString(value)))
{
    return arg == value;
}

MATCHER_P2(HWCRectMatchesRect, value, str,
          std::string("rectangle " + std::string(str) + " should be: " + testing::PrintToString(value)))
{
    hwc_rect_t rect = arg;
    return ((rect.left == value.left) &&
            (rect.top == value.top) &&
            (rect.right == value.right) &&
            (rect.bottom == value.bottom));
}

MATCHER_P2(MatchesRect, value, str,
          std::string("rectangle " + std::string(str) + " should be: " + testing::PrintToString(value)))
{
    return ((arg.left == value.left) &&
            (arg.top == value.top) &&
            (arg.right == value.right) &&
            (arg.bottom == value.bottom));
}

MATCHER_P2(MatchesRectf, value, str,
          std::string("rectangle " + std::string(str) + " should be: " + testing::PrintToString(value)))
{
    using namespace testing;
    EXPECT_THAT(arg.left, FloatEq(value.left));
    EXPECT_THAT(arg.top, FloatEq(value.top));
    EXPECT_THAT(arg.right, FloatEq(value.right));
    EXPECT_THAT(arg.bottom, FloatEq(value.bottom));
    return !(::testing::Test::HasFailure());
}

MATCHER_P(MatchesCommonFields, value, std::string(testing::PrintToString(value)))
{
    EXPECT_THAT(arg.compositionType, MatchesMember(value.compositionType, "compositionType"));
    EXPECT_THAT(arg.hints, MatchesMember(value.hints, "hints"));
    EXPECT_THAT(arg.flags, MatchesMember(value.flags, "flags"));
    EXPECT_THAT(arg.handle, MatchesMember(value.handle, "handle"));
    EXPECT_THAT(arg.transform, MatchesMember(value.transform, "transform"));
    EXPECT_THAT(arg.blending, MatchesMember(value.blending, "blending"));
    EXPECT_THAT(arg.displayFrame, MatchesRect(value.displayFrame, "displayFrame"));
    EXPECT_THAT(arg.visibleRegionScreen.numRects, MatchesMember(value.visibleRegionScreen.numRects, "visibleRegionScreen.numRects"));
    EXPECT_THAT(arg.planeAlpha, MatchesMember(value.planeAlpha, "planeAlpha"));
    EXPECT_THAT(arg.acquireFenceFd, MatchesMember(value.acquireFenceFd, "acquireFenceFd"));
    EXPECT_THAT(arg.releaseFenceFd, MatchesMember(value.releaseFenceFd, "releaseFenceFd")); 
    return !(::testing::Test::HasFailure());
}

MATCHER_P(MatchesLegacyLayer, value, std::string(testing::PrintToString(value)) )
{
    EXPECT_THAT(arg, MatchesCommonFields(value));
    EXPECT_THAT(arg.sourceCropi, MatchesRect(value.sourceCropi, "sourceCrop (int)"));

    return !(::testing::Test::HasFailure());
}

MATCHER_P(MatchesLayer, value, std::string(testing::PrintToString(value)) )
{
    EXPECT_THAT(arg, MatchesCommonFields(value));
    EXPECT_THAT(arg.sourceCropf, MatchesRectf(value.sourceCropf, "sourceCrop (float)"));

    return !(::testing::Test::HasFailure());
}

MATCHER_P(MatchesList, value, std::string(""))
{
    if (arg == nullptr)
        return (value.empty()); 
    auto const& list = *arg;

    EXPECT_EQ(list.numHwLayers, value.size());
    auto i = 0u;
    for(auto layer : value)
    {
        EXPECT_THAT(list.hwLayers[i++], MatchesLegacyLayer(*layer));
        if (::testing::Test::HasFailure())
            return false;
    }
    return !(::testing::Test::HasFailure());
}

MATCHER_P(MatchesPrimaryList, value, std::string(""))
{
    EXPECT_THAT(arg[0], MatchesList(value));
    return !(::testing::Test::HasFailure());
}

MATCHER_P2(MatchesLists, primary, external, std::string(""))
{
    EXPECT_THAT(arg[0], MatchesList(primary));
    EXPECT_THAT(arg[1], MatchesList(external));
    return !(::testing::Test::HasFailure());
}

MATCHER_P3(MatchesListWithEglFields, value, dpy, sur, std::string(""))
{
    if (arg[0] == nullptr)
        return (value.empty()); 
    EXPECT_EQ(arg[0]->dpy, dpy);
    EXPECT_EQ(arg[0]->sur, sur);
    EXPECT_THAT(arg, MatchesPrimaryList(value));
    return !(::testing::Test::HasFailure());
}

#endif /* MIR_TEST_HWC_STRUCT_HELPERS_H_ */
