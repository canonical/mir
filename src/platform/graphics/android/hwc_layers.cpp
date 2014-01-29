/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/graphics/renderable.h"
#include "mir/graphics/buffer.h"
#include "mir/graphics/android/sync_fence.h"
#include "mir/graphics/android/native_buffer.h"
#include "hwc_layerlist.h"

#include <cstring>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace geom=mir::geometry;

mga::HWCLayer& mga::HWCLayer::operator=(HWCLayer const& layer)
{
    memcpy(hwc_layer, layer.hwc_layer, sizeof(hwc_layer_1));
    visible_rect = layer.visible_rect;
    hwc_layer->visibleRegionScreen = {1, &visible_rect};
    return *this;
}

mga::HWCLayer::HWCLayer(hwc_layer_1 *native_layer)
    : hwc_layer(native_layer)
{
}

mga::HWCLayer::HWCLayer(
        hwc_layer_1 *native_layer,
        int type,
        buffer_handle_t buffer_handle,
        geom::Rectangle position,
        geom::Size buffer_size,
        bool skip, bool alpha)
    : hwc_layer(native_layer)
{
    (skip) ? hwc_layer->flags = HWC_SKIP_LAYER : hwc_layer->flags = 0;
    (alpha) ? hwc_layer->blending = HWC_BLENDING_COVERAGE : hwc_layer->blending = HWC_BLENDING_NONE;
    hwc_layer->compositionType = type;
    hwc_layer->hints = 0;
    hwc_layer->transform = 0;
    //TODO: acquireFenceFd should be buffer.fence()
    hwc_layer->acquireFenceFd = -1;
    hwc_layer->releaseFenceFd = -1;

    hwc_layer->sourceCrop = 
    {
        0, 0,
        static_cast<int>(buffer_size.width.as_uint32_t()),
        static_cast<int>(buffer_size.height.as_uint32_t())
    };


    /* note, if the sourceCrop and DisplayFrame sizes differ, the output will be linearly scaled */
    hwc_layer->displayFrame = 
    {
        static_cast<int>(position.top_left.x.as_uint32_t()),
        static_cast<int>(position.top_left.y.as_uint32_t()),
        static_cast<int>(position.size.width.as_uint32_t()),
        static_cast<int>(position.size.height.as_uint32_t())
    };

    visible_rect = hwc_layer->displayFrame;
    hwc_layer->visibleRegionScreen.numRects=1;
    hwc_layer->visibleRegionScreen.rects= &visible_rect;

    hwc_layer->handle = buffer_handle;
    memset(&hwc_layer->reserved, 0, sizeof(hwc_layer->reserved));
}

bool mga::HWCLayer::needs_gl_render() const
{
    return ((hwc_layer->compositionType == HWC_FRAMEBUFFER) || (hwc_layer->flags == HWC_SKIP_LAYER));
}

mga::FramebufferLayer::FramebufferLayer(hwc_layer_1 *native_layer)
    : HWCLayer(native_layer,
               HWC_FRAMEBUFFER_TARGET,
               nullptr,
               geom::Rectangle{{0,0}, {0,0}},
               geom::Size{0,0},
               false,
               false)
{
}

mga::FramebufferLayer::FramebufferLayer(hwc_layer_1 *native_layer, mg::NativeBuffer const& buffer)
    : HWCLayer(native_layer,
               HWC_FRAMEBUFFER_TARGET,
               buffer.handle(),
               geom::Rectangle{{0,0}, {buffer.anwb()->width, buffer.anwb()->height}},
               geom::Size{buffer.anwb()->width, buffer.anwb()->height},
               false,
               false)
{
}

mga::ForceGLLayer::ForceGLLayer(hwc_layer_1 *native_layer)
    : HWCLayer(native_layer,
               HWC_FRAMEBUFFER,
               nullptr,
               geom::Rectangle{{0,0}, {0,0}},
               geom::Size{0,0},
               true,
               false)
{
}

mga::ForceGLLayer::ForceGLLayer(hwc_layer_1 *native_layer, mg::NativeBuffer const& buffer)
    : HWCLayer(native_layer,
               HWC_FRAMEBUFFER,
               buffer.handle(),
               geom::Rectangle{{0,0}, {buffer.anwb()->width, buffer.anwb()->height}},
               geom::Size{buffer.anwb()->width, buffer.anwb()->height},
               true,
               false)
{
}

mga::CompositionLayer::CompositionLayer(hwc_layer_1 *native_layer, mg::Renderable const& renderable)
    : HWCLayer(native_layer,
               HWC_FRAMEBUFFER,
               renderable.buffer()->native_buffer_handle()->handle(),
               renderable.screen_position(),
               renderable.buffer()->size(),
               false,
               renderable.alpha_enabled())
{
}
