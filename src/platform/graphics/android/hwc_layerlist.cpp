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
void mga::BasicLayerList::resize_layer_list(size_t num_layers)
{
    auto struct_size = sizeof(hwc_display_contents_1_t) + sizeof(hwc_layer_1_t)*(num_layers);
    hwc_representation = std::shared_ptr<hwc_display_contents_1_t>(
        static_cast<hwc_display_contents_1_t*>( ::operator new(struct_size)));
    hwc_representation->numHwLayers = num_layers;
    hwc_representation->retireFenceFd = -1;
    hwc_representation->flags = HWC_GEOMETRY_CHANGED;

    //aosp exynos hwc in particular, checks that these fields are non-null in hwc1.1, although
    //these fields are deprecated in hwc1.1 and later.
    hwc_representation->dpy = reinterpret_cast<void*>(0xDECAF);
    hwc_representation->sur = reinterpret_cast<void*>(0xC0FFEE);
}

namespace
{
void copy_layer_list(
     std::shared_ptr<hwc_display_contents_1_t> const& hwc_representation,
     std::list<std::shared_ptr<mg::Renderable>> const& list, geom::Size display_size)
{
    auto i = 0u;
    for( auto& layer : list)
    {
        hwc_representation->hwLayers[i++] = mga::CompositionLayer(*layer, display_size); 
    } 
}
}

mga::BasicLayerList::BasicLayerList(size_t size)
{
    resize_layer_list(size);
}

hwc_display_contents_1_t* mga::BasicLayerList::native_list() const
{
    return hwc_representation.get();
}


mga::FBTargetLayerList::FBTargetLayerList()
    : BasicLayerList(2)
{
    hwc_representation->hwLayers[0] = mga::ForceGLLayer{};
    hwc_representation->hwLayers[1] = fb_target;
    fb_position = 1;
}

void mga::FBTargetLayerList::update_composition_layers(
    std::list<std::shared_ptr<graphics::Renderable>> const& list)
{
    auto needed_size = list.size() + 1;
    if (hwc_representation->numHwLayers != needed_size)
    {
        fb_position = list.size();
        resize_layer_list(needed_size);
        hwc_representation->hwLayers[fb_position] = fb_target; 
    }

    copy_layer_list(hwc_representation, list, geom::Size{});
}

void mga::FBTargetLayerList::set_fb_target(std::shared_ptr<NativeBuffer> const& native_buffer)
{
    fb_target = mga::FramebufferLayer(*native_buffer);
    hwc_representation->hwLayers[fb_position] = fb_target;
    hwc_representation->hwLayers[fb_position].acquireFenceFd = native_buffer->copy_fence();
}

mga::NativeFence mga::FBTargetLayerList::framebuffer_fence()
{
    return hwc_representation->hwLayers[fb_position].releaseFenceFd;
}

mga::HWC10LayerList::HWC10LayerList()
    : BasicLayerList(1)
{
    hwc_representation->hwLayers[0] = mga::ForceGLLayer{};
}

void mga::HWC10LayerList::update_composition_layers(
    std::list<std::shared_ptr<graphics::Renderable>> const& list)
{
    if (hwc_representation->numHwLayers != list.size())
    {
        resize_layer_list(list.size());
    }

    copy_layer_list(hwc_representation, list, geom::Size{});
}
