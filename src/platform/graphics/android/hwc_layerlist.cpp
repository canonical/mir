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

void mga::LayerListBase::update_representation(
    size_t needed_size,
    std::list<std::shared_ptr<mg::Renderable>> const& new_render_list)
{
    std::shared_ptr<hwc_display_contents_1_t> new_hwc_representation;
    if ((!hwc_representation) || hwc_representation->numHwLayers != needed_size)
    {
        /* hwc layer list uses hwLayers[0] at the end of the struct */
        auto struct_size = sizeof(hwc_display_contents_1_t) + sizeof(hwc_layer_1_t)*(needed_size);
        new_hwc_representation = std::shared_ptr<hwc_display_contents_1_t>(
            static_cast<hwc_display_contents_1_t*>( ::operator new(struct_size)));

        new_hwc_representation->numHwLayers = needed_size;
        new_hwc_representation->retireFenceFd = -1;
        new_hwc_representation->flags = HWC_GEOMETRY_CHANGED;

        //aosp exynos hwc in particular, checks that these fields are non-null in hwc1.1, although
        //these fields are deprecated in hwc1.1 and later.
        new_hwc_representation->dpy = &fake_egl_values;
        new_hwc_representation->sur = &fake_egl_values;
    }
    else
    {
        new_hwc_representation = hwc_representation;
    }

    std::list<std::shared_ptr<HWCLayer>> new_layers;

    auto i = 0u;
    if (new_render_list.empty())
    {
        std::shared_ptr<mga::HWCLayer> skip_layer;
        if ((composition_layers_present) || (layers.empty()))
        {
            //if we are not in skip mode, make a new layer
            skip_layer = std::make_shared<mga::ForceGLLayer>(&new_hwc_representation->hwLayers[i++]);
        }
        else
        {
            //if we are in skip mode, preserve the skip layer
            skip_layer = std::make_shared<mga::HWCLayer>(&new_hwc_representation->hwLayers[i++]);
            *skip_layer = *layers.front();
        }
        new_layers.push_back(skip_layer);

        composition_layers_present = false;
    }
    else
    {
        for( auto& renderable : new_render_list)
        {
            new_layers.push_back(
                std::make_shared<mga::CompositionLayer>(&new_hwc_representation->hwLayers[i++], *renderable));
        }

        composition_layers_present = true;
    }

    for (; i < needed_size; i++)
    {
        new_layers.push_back(
                std::make_shared<mga::HWCLayer>(&new_hwc_representation->hwLayers[i++]));
    }

    std::swap(new_layers, layers);
    std::swap(new_hwc_representation, hwc_representation);
}

void mga::LayerListBase::with_native_list(std::function<void(hwc_display_contents_1_t&)> const& fn)
{
    fn(*hwc_representation);
}

mga::NativeFence mga::LayerListBase::retirement_fence()
{
    return hwc_representation->retireFenceFd;
}

mga::LayerList::LayerList()
{
    update_representation(1, {});
}

mga::FBTargetLayerList::FBTargetLayerList()
{
    reset_composition_layers();
}

void mga::FBTargetLayerList::reset_composition_layers()
{
    update_representation(2, {});
    hwc_layer_1_t tmp_layer;
    *layers.back() = mga::FramebufferLayer(&tmp_layer);
}

void mga::FBTargetLayerList::set_composition_layers(std::list<std::shared_ptr<graphics::Renderable>> const& list)
{
    auto needed_size = list.size() + 1;
    hwc_layer_1_t tmp_layer;
    mga::HWCLayer fb_target(&tmp_layer);
    fb_target = *layers.back();

    update_representation(needed_size, list);
    *layers.back() = fb_target;
}


void mga::FBTargetLayerList::set_fb_target(mg::NativeBuffer const& native_buffer)
{
    hwc_layer_1_t tmp_layer;
    if (!composition_layers_present)
    {
        *layers.front() = mga::ForceGLLayer(&tmp_layer, native_buffer);
    }

    *layers.back() = mga::FramebufferLayer(&tmp_layer, native_buffer);
}

mga::NativeFence mga::FBTargetLayerList::fb_target_fence()
{
   return layers.back()->release_fence();
}
