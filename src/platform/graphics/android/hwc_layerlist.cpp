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
        printf("ok.\n");
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

    printf("COPY!\n");
    auto i = 0u;
    for( auto& layer : layers)
    {
        hwc_representation->hwLayers[i++] = *layer; 
    }
}


mga::LayerList::LayerList(bool has_target_layer)
    : composition_layers_present(false),
      fb_target_present(has_target_layer),
      hwc_representation(nullptr),
      retire_fence(-1),
      fb_fence(-1)
{
    skip_layer = std::make_shared<mga::ForceGLLayer>();
    fb_target_layer = std::make_shared<mga::FramebufferLayer>();
    reset_composition_layers();
}

void mga::LayerList::with_native_list(std::function<void(hwc_display_contents_1_t&)> const& fn)
{
    update_representation();

    fn(*hwc_representation);

    if (fb_target_present)
    {
        auto fb_position = layers.size() - 1;
        fb_fence = hwc_representation->hwLayers[fb_position].releaseFenceFd;
    }
}

void mga::LayerList::reset_composition_layers()
{
    std::list<std::shared_ptr<mga::HWCLayer>> next_layer_list;

    next_layer_list.push_back(skip_layer);
    if (fb_target_present)
    {
        next_layer_list.push_back(fb_target_layer);
    } 

    std::swap(layers, next_layer_list);
}

void mga::LayerList::set_composition_layers(
    std::list<std::shared_ptr<graphics::Renderable>> const& list)
{
    std::list<std::shared_ptr<mga::HWCLayer>> next_layer_list;
    for(auto& renderable : list)
    {
        next_layer_list.push_back(std::make_shared<mga::CompositionLayer>(*renderable));
    }

    //preserve FB_TARGET for lists that need it
    if (fb_target_present)
    {
        next_layer_list.push_back(fb_target_layer);
    }

    std::swap(layers, next_layer_list);
}

void mga::LayerList::set_fb_target(mg::NativeBuffer const& native_buffer)
{
    *skip_layer = mga::ForceGLLayer(native_buffer);
    if (fb_target_present)
    {
        *fb_target_layer = mga::FramebufferLayer(native_buffer);
    }
}

mga::NativeFence mga::LayerList::fb_target_fence()
{
    return fb_fence;
}

mga::NativeFence mga::LayerList::retirement_fence()
{
    return retire_fence;
}
