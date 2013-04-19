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

#include "hwc_layerlist.h"
#include "android_buffer.h"

#include <cstring>

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
    self.compositionType = HWC_FRAMEBUFFER;
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
        std::shared_ptr<ANativeWindowBuffer> const& native_buf,
        HWCRect& display_frame_rect)
    : HWCDefaultLayer{display_frame_rect}
{
    self.compositionType = HWC_FRAMEBUFFER_TARGET;
    self.handle = native_buf->handle;

    self.sourceCrop = display_frame_rect;
    self.displayFrame = display_frame_rect;
}

mga::HWCLayerList::HWCLayerList()
{
}

const mga::LayerList& mga::HWCLayerList::native_list() const
{
    return layer_list;
}

void mga::HWCLayerList::set_fb_target(std::shared_ptr<mga::AndroidBuffer> const& buffer)
{
    auto handle = buffer->native_buffer_handle();

    geom::Point pt{geom::X{0}, geom::Y{0}};
    geom::Rectangle rect{pt, buffer->size()};
    HWCRect display_rect(rect);

    auto fb_layer = std::make_shared<HWCFBLayer>(handle, display_rect);
    if (layer_list.empty())
    {
        layer_list.push_back(fb_layer);
    }
    else
    {
        layer_list[0] = fb_layer;
    }
} 
