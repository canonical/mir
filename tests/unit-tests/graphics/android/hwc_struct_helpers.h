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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

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

MATCHER_P(MatchesLayer, value, std::string(testing::PrintToString(value)) )
{
    EXPECT_THAT(arg.compositionType, MatchesMember(value.compositionType, "compositionType"));
    EXPECT_THAT(arg.hints, MatchesMember(value.hints, "hints"));
    EXPECT_THAT(arg.flags, MatchesMember(value.flags, "flags"));
    EXPECT_THAT(arg.handle, MatchesMember(value.handle, "backgroundColor.handle"));
    EXPECT_THAT(arg.transform, MatchesMember(value.transform, "backgroundColor.transform"));
    EXPECT_THAT(arg.blending, MatchesMember(value.blending, "backgroundColor.blending"));
    EXPECT_THAT(arg.sourceCrop, MatchesRect(value.sourceCrop, "sourceCrop"));
    EXPECT_THAT(arg.displayFrame, MatchesRect(value.displayFrame, "displayFrame"));
    EXPECT_THAT(arg.visibleRegionScreen.numRects, MatchesMember(value.visibleRegionScreen.numRects, "visibleRegionScreen.numRects"));
    EXPECT_THAT(arg.acquireFenceFd, MatchesMember(value.acquireFenceFd, "acquireFenceFd"));
    EXPECT_THAT(arg.releaseFenceFd, MatchesMember(value.releaseFenceFd, "releaseFenceFd"));

    return !(::testing::Test::HasFailure());
}

#endif /* MIR_TEST_HWC_STRUCT_HELPERS_H_ */
