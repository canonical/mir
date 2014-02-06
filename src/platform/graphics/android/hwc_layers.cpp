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
    memcpy(static_cast<void*>(this),
           static_cast<void const*>(&layer),
           sizeof(HWCLayer));
    this->visibleRegionScreen = {1, &this->visible_rect};
    return *this;
}

mga::HWCLayer::HWCLayer(HWCLayer const& layer)
{
    memcpy(static_cast<void*>(this),
           static_cast<void const*>(&layer),
           sizeof(HWCLayer));
    this->visibleRegionScreen = {1, &this->visible_rect};
}

mga::HWCLayer::HWCLayer(
        int type,
        buffer_handle_t buffer_handle,
        geom::Rectangle position,
        geom::Size buffer_size,
        bool skip, bool alpha)
{
    (skip) ? flags = HWC_SKIP_LAYER : flags = 0;
    (alpha) ? blending = HWC_BLENDING_COVERAGE : blending = HWC_BLENDING_NONE;
    compositionType = type;
    hints = 0;
    transform = 0;
    //TODO: acquireFenceFd should be buffer.fence()
    acquireFenceFd = -1;
    releaseFenceFd = -1;

    sourceCrop = 
    {
        0, 0,
        buffer_size.width.as_int(),
        buffer_size.height.as_int()
    };


    /* note, if the sourceCrop and DisplayFrame sizes differ, the output will be linearly scaled */
    displayFrame = 
    {
        position.top_left.x.as_int(),
        position.top_left.y.as_int(),
        position.size.width.as_int(),
        position.size.height.as_int()
    };

    visible_rect = displayFrame;
    visibleRegionScreen.numRects=1;
    visibleRegionScreen.rects= &visible_rect;

    handle = buffer_handle;
    memset(&reserved, 0, sizeof(reserved));
}

bool mga::HWCLayer::needs_gl_render() const
{
    return ((compositionType == HWC_FRAMEBUFFER) || (flags == HWC_SKIP_LAYER));
}

mga::FramebufferLayer::FramebufferLayer()
    : HWCLayer(HWC_FRAMEBUFFER_TARGET,
               nullptr,
               geom::Rectangle{{0,0}, {0,0}},
               geom::Size{0,0},
               false,
               false)
{
}

mga::FramebufferLayer::FramebufferLayer(mg::NativeBuffer const& buffer)
    : HWCLayer(HWC_FRAMEBUFFER_TARGET,
               buffer.handle(),
               geom::Rectangle{{0,0}, {buffer.anwb()->width, buffer.anwb()->height}},
               geom::Size{buffer.anwb()->width, buffer.anwb()->height},
               false,
               false)
{
}

mga::ForceGLLayer::ForceGLLayer()
    : HWCLayer(HWC_FRAMEBUFFER,
               nullptr,
               geom::Rectangle{{0,0}, {0,0}},
               geom::Size{0,0},
               true,
               false)
{
}

mga::ForceGLLayer::ForceGLLayer(mg::NativeBuffer const& buffer)
    : HWCLayer(HWC_FRAMEBUFFER,
               buffer.handle(),
               geom::Rectangle{{0,0}, {buffer.anwb()->width, buffer.anwb()->height}},
               geom::Size{buffer.anwb()->width, buffer.anwb()->height},
               true,
               false)
{
}

mga::CompositionLayer::CompositionLayer(mg::Renderable const& renderable)
    : HWCLayer(HWC_FRAMEBUFFER,
               renderable.buffer()->native_buffer_handle()->handle(),
               renderable.screen_position(),
               renderable.buffer()->size(),
               false,
               renderable.alpha_enabled())
{
}
