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
#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace geom=mir::geometry;

mga::HWCLayer& mga::HWCLayer::operator=(HWCLayer const& layer)
{
    memcpy(this, &layer, sizeof(HWCLayer));
    this->visibleRegionScreen = {1, &this->visible_rect};
    return *this;
}

mga::HWCLayer::HWCLayer(HWCLayer const& layer)
{
    memcpy(this, &layer, sizeof(HWCLayer));
    this->visibleRegionScreen = {1, &this->visible_rect};
}

mga::HWCLayer::HWCLayer(
        int type,
        buffer_handle_t buffer_handle,
        geom::Rectangle position,
        geom::Size buffer_size,
        geom::Size screen_size,
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
        static_cast<int>(buffer_size.width.as_uint32_t()),
        static_cast<int>(buffer_size.height.as_uint32_t())
    };

    /* note, if the sourceCrop and DisplayFrame sizes differ, the output will be linearly scaled */
    displayFrame = 
    {
        static_cast<int>(position.top_left.x.as_uint32_t()),
        static_cast<int>(position.top_left.y.as_uint32_t()),
        static_cast<int>(position.size.width.as_uint32_t()),
        static_cast<int>(position.size.height.as_uint32_t())
    };

    visible_rect.top = 0;
    visible_rect.left = 0;
    visible_rect.bottom = static_cast<int>(screen_size.height.as_uint32_t());
    visible_rect.right =  static_cast<int>(screen_size.width.as_uint32_t());
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
               geom::Size{0,0},
               true,
               false)
{
}

mga::CompositionLayer::CompositionLayer(mg::Renderable const& renderable, geom::Size display_size)
    : HWCLayer(HWC_FRAMEBUFFER,
               renderable.buffer()->native_buffer_handle()->handle(),
               renderable.screen_position(),
               renderable.buffer()->size(),
               display_size,
               false,
               renderable.alpha_enabled())
{
}

mga::LayerList::LayerList(std::initializer_list<HWCLayer> const& layer_list)
{
    auto struct_size = sizeof(hwc_display_contents_1_t) + sizeof(hwc_layer_1_t)*(layer_list.size());
    hwc_representation = std::shared_ptr<hwc_display_contents_1_t>(
        static_cast<hwc_display_contents_1_t*>( ::operator new(struct_size)));

    auto i = 0u;
    for(auto& layer : layer_list)
    {
        hwc_representation->hwLayers[i++] = layer;
    }
    hwc_representation->numHwLayers = layer_list.size();
    hwc_representation->retireFenceFd = -1;
    hwc_representation->flags = HWC_GEOMETRY_CHANGED;

    //aosp exynos hwc in particular, checks that these fields are non-null in hwc1.1, although
    //these fields are deprecated in hwc1.1 and later.
    hwc_representation->dpy = reinterpret_cast<void*>(0xDECAF);
    hwc_representation->sur = reinterpret_cast<void*>(0xC0FFEE);

}

void mga::LayerList::set_fb_target(std::shared_ptr<NativeBuffer> const& native_buffer)
{
    if (hwc_representation->numHwLayers <= 0)
        BOOST_THROW_EXCEPTION(std::runtime_error("no HWC layers. cannot set fb target"));

    auto fb_position = hwc_representation->numHwLayers - 1;
    if (hwc_representation->hwLayers[fb_position].compositionType == HWC_FRAMEBUFFER_TARGET)
    {
        hwc_representation->hwLayers[fb_position] = mga::FramebufferLayer(*native_buffer);
        hwc_representation->hwLayers[fb_position].acquireFenceFd = native_buffer->copy_fence();
    }
}

mga::NativeFence mga::LayerList::framebuffer_fence()
{
    if (hwc_representation->numHwLayers <= 0)
        BOOST_THROW_EXCEPTION(std::runtime_error("no HWC layers. cannot get fb fence"));

    auto fb_position = hwc_representation->numHwLayers - 1;
    return hwc_representation->hwLayers[fb_position].releaseFenceFd;
}

hwc_display_contents_1_t* mga::LayerList::native_list() const
{
    return hwc_representation.get();
}
