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

#include <gtest/gtest.h>

void PrintTo(const hwc_rect_t& rect, ::std::ostream* os)
{
    *os << "( left: "  << rect.left
        << ", top: "   << rect.top
        << ", right "  << rect.right
        << ", bottom: "<< rect.bottom << ")";
}

void PrintTo(const hwc_layer_1& layer , ::std::ostream* os)
{
  *os << "compositionType: " << layer.compositionType << std::endl
    << "\thints: " << layer.hints << std::endl
    << "\tflags: " << layer.flags << std::endl
    << "\thandle: " << layer.handle << std::endl
    << "\ttransform: " << layer.transform << std::endl
    << "\tblending: " << layer.blending << std::endl
    << "\tsourceCrop:  ";
    PrintTo(layer.sourceCrop, os);
    *os << std::endl << "\tdisplayFrame:";
    PrintTo(layer.displayFrame, os);
    *os << std::endl;
    *os << "\tvisibleRegionScreen.numRects: " << layer.visibleRegionScreen.numRects << std::endl
    << "\tacquireFenceFd: " << layer.acquireFenceFd << std::endl
    << "\treleaseFenceFd: " << layer.releaseFenceFd << std::endl;
}

MATCHER_P2(MatchesMember, value, str,
          std::string("layer's " + std::string(str) + " should be: " + testing::PrintToString(value)))
{
    return arg == value;
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
    auto layer = arg; //makes for nicer printing
    EXPECT_THAT(layer.compositionType, MatchesMember(value.compositionType, "compositionType"));
    EXPECT_THAT(layer.hints, MatchesMember(value.hints, "hints"));
    EXPECT_THAT(layer.flags, MatchesMember(value.flags, "flags"));
    EXPECT_THAT(layer.handle, MatchesMember(value.handle, "backgroundColor.handle"));
    EXPECT_THAT(layer.transform, MatchesMember(value.transform, "backgroundColor.transform"));
    EXPECT_THAT(layer.blending, MatchesMember(value.blending, "backgroundColor.blending"));
    EXPECT_THAT(layer.sourceCrop, MatchesRect(value.sourceCrop, "sourceCrop"));
    EXPECT_THAT(layer.displayFrame, MatchesRect(value.displayFrame, "displayFrame"));
    EXPECT_THAT(layer.visibleRegionScreen.numRects, MatchesMember(value.visibleRegionScreen.numRects, "visibleRegionScreen.numRects"));
    EXPECT_THAT(layer.acquireFenceFd, MatchesMember(value.acquireFenceFd, "acquireFenceFd"));
    EXPECT_THAT(layer.releaseFenceFd, MatchesMember(value.releaseFenceFd, "releaseFenceFd"));

    return !(::testing::Test::HasFailure());
}

#if 0
TEST_F(HWCLayerListTest, matcher)
{
    hwc_layer_1_t good_layer;
    memset(&good_layer, 0, sizeof(hwc_layer_1_t));
    good_layer.compositionType = 2;

    hwc_layer_1_t layer;
    memset(&layer, 0, sizeof(hwc_layer_1_t));
    layer.compositionType = 3;
    EXPECT_THAT(layer, MatchesLayer( good_layer ));
}
#endif

