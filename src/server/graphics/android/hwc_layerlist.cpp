/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "hwc_layerlist.h"
#include "native_buffer_handle.h"
#include "mir/compositor/buffer.h"

namespace mga=mir::graphics::android;
namespace mc=mir::compositor;
namespace geom=mir::geometry;

//construction is a bit funny because hwc_layer_1 has unions
mga::HWCRect::HWCRect()
{
}

mga::HWCRect::HWCRect(geom::Rectangle& rect)
{
    top = rect.top_left.y.as_uint32_t();
    left = rect.top_left.x.as_uint32_t();

    bottom= rect.size.height.as_uint32_t();
    right = rect.size.width.as_uint32_t();
}

mga::HWCLayerBase::HWCLayerBase()
{
    /* default values.*/
    compositionType = HWC_FRAMEBUFFER;
    hints = 0;
    flags = 0;
    transform = 0;
    blending = HWC_BLENDING_NONE;
    acquireFenceFd = -1;
    releaseFenceFd = -1;

    HWCRect emptyrect;
    visible_screen_rect = emptyrect;
    sourceCrop = emptyrect;
    displayFrame = emptyrect;
    visible_screen_rect = emptyrect;
    visibleRegionScreen.numRects=1u;
    visibleRegionScreen.rects = &visible_screen_rect; 
}

mga::HWCFBLayer::HWCFBLayer(
        std::shared_ptr<compositor::NativeBufferHandle> const& native_buf,
        HWCRect& display_frame_rect)
    : HWCLayerBase()
{
    compositionType = HWC_FRAMEBUFFER_TARGET;
    handle = native_buf->handle;

    visible_screen_rect = display_frame_rect;
    sourceCrop = display_frame_rect;
    displayFrame = display_frame_rect;
    visible_screen_rect =  display_frame_rect; 
}

mga::HWCDummyLayer::HWCDummyLayer()
    : HWCLayerBase()
{
    compositionType = HWC_FRAMEBUFFER;
    hints = HWC_SKIP_LAYER;
}

mga::HWCLayerList::HWCLayerList()
{
    auto fb_layer = std::make_shared<HWCDummyLayer>();
    layer_list.push_back(fb_layer);
}

const mga::LayerList& mga::HWCLayerList::native_list() const
{
    return layer_list;
}

void mga::HWCLayerList::set_fb_target(std::shared_ptr<compositor::Buffer> const& buffer)
{
    auto handle = buffer->native_buffer_handle();

    geom::Point pt{geom::X{0}, geom::Y{0}};
    geom::Rectangle rect{pt, buffer->size()};
    HWCRect display_rect(rect);

    auto fb_layer = std::make_shared<HWCFBLayer>(handle, display_rect);
    if (layer_list.size() == 1)
    {
        layer_list.push_back(fb_layer);
    }
    else
    {
        layer_list[1] = fb_layer;
    }
} 
