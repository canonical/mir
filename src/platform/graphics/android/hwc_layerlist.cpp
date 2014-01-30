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

namespace
{

std::shared_ptr<hwc_display_contents_1_t> generate_hwc_list(size_t needed_size)
{
    /* hwc layer list uses hwLayers[0] at the end of the struct */
    auto struct_size = sizeof(hwc_display_contents_1_t) + sizeof(hwc_layer_1_t)*(needed_size);
    auto new_hwc_representation = std::shared_ptr<hwc_display_contents_1_t>(
        static_cast<hwc_display_contents_1_t*>( ::operator new(struct_size)));

    new_hwc_representation->numHwLayers = needed_size;
    new_hwc_representation->retireFenceFd = -1;
    new_hwc_representation->flags = HWC_GEOMETRY_CHANGED;

    //aosp exynos hwc in particular, checks that these fields are non-null in hwc1.1, although
    //these fields are deprecated in hwc1.1 and later.
    static int fake_egl_values = 0;
    new_hwc_representation->dpy = &fake_egl_values;
    new_hwc_representation->sur = &fake_egl_values;

    return new_hwc_representation;
}

}

void mga::LayerListBase::update_representation(size_t needed_size)
{
    std::shared_ptr<hwc_display_contents_1_t> new_hwc_representation;
    std::list<std::shared_ptr<HWCLayer>> new_layers;

    if (hwc_representation->numHwLayers != needed_size)
    {
        new_hwc_representation = generate_hwc_list(needed_size);
    }
    else
    {
        new_hwc_representation = hwc_representation;
    }

    for (auto i = 0u; i < needed_size; i++)
    {
        new_layers.push_back(
                std::make_shared<mga::HWCLayer>(&new_hwc_representation->hwLayers[i]));
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

mga::LayerListBase::LayerListBase(size_t initial_list_size)
    : hwc_representation{generate_hwc_list(initial_list_size)}
{
    update_representation(initial_list_size);
}

mga::LayerList::LayerList()
    : LayerListBase{1}
{
    *layers.back() = mga::ForceGLLayer{};
}

mga::FBTargetLayerList::FBTargetLayerList()
    : LayerListBase{2}
{
    *layers.front() = mga::ForceGLLayer{};
    *layers.back()  = mga::FramebufferLayer{};
}

void mga::FBTargetLayerList::reset_composition_layers()
{
    hwc_layer_1_t tmp_layer;

    mga::HWCLayer fb_target(&tmp_layer);
    fb_target = *layers.back();

//    mga::ForceGLLayer skip_layer(fb_target);

    update_representation(2);

//    *layers.front() = skip_layer;
    *layers.back() = fb_target;

    skip_layers_present = true;
}

void mga::FBTargetLayerList::set_composition_layers(std::list<std::shared_ptr<graphics::Renderable>> const& list)
{
    auto needed_size = list.size() + 1;
    hwc_layer_1_t tmp_layer;
    mga::HWCLayer fb_target(&tmp_layer);
    fb_target = *layers.back();

    update_representation(needed_size);

    auto layers_it = layers.begin();
    for( auto& renderable : list)
    {
        **layers_it = mga::CompositionLayer(*renderable);
        layers_it++;
    }

    *layers.back() = fb_target;

    skip_layers_present = false;
}


void mga::FBTargetLayerList::set_fb_target(mg::NativeBuffer const& native_buffer)
{
    if (skip_layers_present)
    {
        *layers.front() = mga::ForceGLLayer(native_buffer);
    }

    *layers.back() = mga::FramebufferLayer(native_buffer);
}

mga::NativeFence mga::FBTargetLayerList::fb_target_fence()
{
   return layers.back()->release_fence();
}
