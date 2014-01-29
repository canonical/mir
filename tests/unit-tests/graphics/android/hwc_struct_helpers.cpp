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
