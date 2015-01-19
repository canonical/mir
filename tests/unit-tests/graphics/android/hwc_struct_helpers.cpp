/*
 * Copyright © 2014 Canonical Ltd.
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

#include <hardware/hwcomposer.h>
#include "hwc_struct_helpers.h"
#include "mir/graphics/android/native_buffer.h"

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
    PrintTo(layer.sourceCropi, os);
    *os << std::endl << "\tdisplayFrame:";
    PrintTo(layer.displayFrame, os);
    *os << std::endl;
    *os << "\tvisibleRegionScreen.numRects: " << layer.visibleRegionScreen.numRects << std::endl
    << "\tplaneAlpha: " << layer.planeAlpha << std::endl
    << "\tacquireFenceFd: " << layer.acquireFenceFd << std::endl
    << "\treleaseFenceFd: " << layer.releaseFenceFd << std::endl;
}

void mir::test::fill_hwc_layer(
    hwc_layer_1_t& layer,
    hwc_rect_t* visible_rect,
    mir::geometry::Rectangle const& position,
    mir::graphics::Buffer const& buffer,
    int type, int flags)
{
    *visible_rect = {0, 0, buffer.size().width.as_int(), buffer.size().height.as_int()};
    layer.compositionType = type;
    layer.hints = 0;
    layer.flags = flags;
    layer.handle = buffer.native_buffer_handle()->handle();
    layer.transform = 0;
    layer.blending = HWC_BLENDING_NONE;
    layer.sourceCrop = *visible_rect;
    layer.displayFrame = {
        position.top_left.x.as_int(),
        position.top_left.y.as_int(),
        position.bottom_right().x.as_int(),
        position.bottom_right().y.as_int()
    };
    layer.visibleRegionScreen = {1, visible_rect};
    layer.acquireFenceFd = -1;
    layer.releaseFenceFd = -1;
    layer.planeAlpha = std::numeric_limits<decltype(hwc_layer_1_t::planeAlpha)>::max();
}
