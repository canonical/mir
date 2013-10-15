/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
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

#include "mir/graphics/android/sync_fence.h"
#include "mir/graphics/android/native_buffer.h"
#include "hwc_layerlist.h"
#include "buffer.h"

#include <cstring>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace geom=mir::geometry;

mga::HWCRect::HWCRect()
    : self{0,0,0,0}
{
}

mga::HWCRect::HWCRect(geom::Rectangle& rect)
{
    self.top = rect.top_left.y.as_uint32_t();
    self.left = rect.top_left.x.as_uint32_t();
    self.bottom= rect.size.height.as_uint32_t();
    self.right = rect.size.width.as_uint32_t();
}

mga::HWCDefaultLayer::HWCDefaultLayer(std::initializer_list<mga::HWCRect> list)
{
    /* default values.*/
    self.compositionType = HWC_FRAMEBUFFER_TARGET;
    self.hints = 0;
    self.flags = 0;
    self.transform = 0;
    self.blending = HWC_BLENDING_NONE;
    self.acquireFenceFd = -1;
    self.releaseFenceFd = -1;

    HWCRect emptyrect;
    self.sourceCrop = emptyrect;
    self.displayFrame = emptyrect;
    self.visibleRegionScreen.numRects=list.size();
    self.visibleRegionScreen.rects=nullptr;
    if (list.size() != 0)
    {
        auto rect_array = new hwc_rect_t[list.size()];
        auto i = 0u;
        for( auto& rect : list )
        {
            rect_array[i++] = rect; 
        }
        self.visibleRegionScreen.rects = rect_array;
    }
}

mga::HWCDefaultLayer::~HWCDefaultLayer()
{
    if (self.visibleRegionScreen.rects)
    {
        delete self.visibleRegionScreen.rects;
    }
}

mga::HWCFBLayer::HWCFBLayer(
        buffer_handle_t native_handle,
        HWCRect display_frame_rect)
    : HWCDefaultLayer{display_frame_rect}
{
    self.compositionType = HWC_FRAMEBUFFER_TARGET;

    self.handle = native_handle;
    self.sourceCrop = display_frame_rect;
    self.displayFrame = display_frame_rect;
}

mga::HWCFBLayer::HWCFBLayer()
    : HWCFBLayer{nullptr, mga::HWCRect{}}
{
}

mga::LayerList::LayerList()
    : layer_list{std::make_shared<HWCFBLayer>()},
      hwc_representation{std::make_shared<hwc_display_contents_1_t>()}
{
    memset(hwc_representation.get(), 0, sizeof(hwc_display_contents_1_t));
    update_list();
}

void mga::LayerList::set_fb_target(std::shared_ptr<mg::Buffer> const& buffer)
{
    auto native_buffer = buffer->native_buffer_handle();
    native_buffer->wait_for_content();

    geom::Point pt{0, 0};
    geom::Rectangle rect{pt, buffer->size()};
    HWCRect display_rect(rect);

    auto fb_layer = std::make_shared<HWCFBLayer>(native_buffer->handle(), display_rect);
    layer_list[fb_position] = fb_layer;

    update_list();
} 

void mga::LayerList::update_list()
{
    if (layer_list.size() != hwc_representation->numHwLayers)
    {
        auto struct_size = sizeof(hwc_display_contents_1_t) + sizeof(hwc_layer_1_t)*(layer_list.size());
        hwc_representation = std::shared_ptr<hwc_display_contents_1_t>(
            static_cast<hwc_display_contents_1_t*>( ::operator new(struct_size)));

        hwc_representation->numHwLayers = layer_list.size();
    }

    auto i = 0u;
    for( auto& layer : layer_list)
    {
        hwc_representation->hwLayers[i++] = *layer;
    }
    hwc_representation->retireFenceFd = -1;
}

hwc_display_contents_1_t* mga::LayerList::native_list() const
{
    return hwc_representation.get();
}
