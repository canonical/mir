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
#include "hwc_layers.h"

#include <cstring>
#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace geom=mir::geometry;

/* hwc layer list uses hwLayers[0] at the end of the struct */
void mga::LayerList::update_representation()
{
    if ((!hwc_representation) || hwc_representation->numHwLayers != layers.size())
    {
        auto struct_size = sizeof(hwc_display_contents_1_t) + sizeof(hwc_layer_1_t)*(layers.size());
        hwc_representation = std::shared_ptr<hwc_display_contents_1_t>(
            static_cast<hwc_display_contents_1_t*>( ::operator new(struct_size)));

        hwc_representation->numHwLayers = layers.size();
        hwc_representation->retireFenceFd = -1;
        hwc_representation->flags = HWC_GEOMETRY_CHANGED;

        //aosp exynos hwc in particular, checks that these fields are non-null in hwc1.1, although
        //these fields are deprecated in hwc1.1 and later.
        hwc_representation->dpy = reinterpret_cast<void*>(0xDECAF);
        hwc_representation->sur = reinterpret_cast<void*>(0xC0FFEE);
    }

    auto i = 0u;
    for( auto& layer : layers)
    {
        hwc_representation->hwLayers[i++] = layer; 
    }
}

mga::LayerList::LayerList(std::initializer_list<mga::HWCLayer> const& list)
    : layers(list)
{
    if (layers.empty() ||
       ((layers.size() == 1) && (layers.front().compositionType == HWC_FRAMEBUFFER_TARGET)))
    {
        layers.push_front(mga::ForceGLLayer{});
    }

    /* sanity check for HWC_FRAMEBUFFER_TARGET usage */
    auto fb_target_count = 0;
    for (auto& layer : layers)
    {
        if (layer.compositionType == HWC_FRAMEBUFFER_TARGET)
        {
            fb_target_count++;
        }
    }

    //not allowed >1 FB target, and it must be at the end if its present
    if ((fb_target_count > 1) ||
       ((fb_target_count == 1) && (layers.back().compositionType != HWC_FRAMEBUFFER_TARGET)))
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("malformed HWCLayerList"));
    }
 
    update_representation();
}


hwc_display_contents_1_t* mga::LayerList::native_list() const
{
    return hwc_representation.get();
}

void mga::LayerList::update_composition_layers(
    std::list<std::shared_ptr<graphics::Renderable>> && list)
{
    std::list<HWCLayer> next_layer_list;
    if (list.empty())
    {
        next_layer_list.push_front(mga::ForceGLLayer{});
    }
    else
    {
        for(auto& renderable : list)
        {
            next_layer_list.push_back(mga::CompositionLayer(*renderable));
        }
    }

    //preserve FB_TARGET for lists that need it
    auto fb_pos = layers.back();
    if (fb_pos.compositionType == HWC_FRAMEBUFFER_TARGET)
    {
        next_layer_list.push_back(fb_pos);
    }

    std::swap(layers, next_layer_list);
    update_representation();
}

void mga::LayerList::set_fb_target(std::shared_ptr<NativeBuffer> const& native_buffer)
{
    if (layers.empty())
        return;

    auto fb_pos = layers.back();
    if (fb_pos.compositionType == HWC_FRAMEBUFFER_TARGET)
    {
        auto fb_position = layers.size() - 1;
        mga::FramebufferLayer fblay(*native_buffer);
        hwc_representation->hwLayers[fb_position] = fblay;
        hwc_representation->hwLayers[fb_position].acquireFenceFd = native_buffer->copy_fence();
        layers.pop_back();
        layers.emplace_back(fblay);
    }
}

mga::NativeFence mga::LayerList::framebuffer_fence()
{
    auto fb_pos = layers.back();
    if (fb_pos.compositionType == HWC_FRAMEBUFFER_TARGET)
    {
        auto fb_position = layers.size() - 1;
        return hwc_representation->hwLayers[fb_position].releaseFenceFd;
    }
    else
    {
        return -1;
    }
}
