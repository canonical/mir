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
    std::list<HWCLayer> new_layers;

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
        new_layers.emplace_back(mga::HWCLayer(new_hwc_representation, i));
    }

    std::swap(new_layers, layers);
    hwc_representation = new_hwc_representation;
}

std::weak_ptr<hwc_display_contents_1_t> mga::LayerListBase::native_list()
{
    return hwc_representation;
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
    layers.back().set_layer_type(mga::LayerType::skip);
}

mga::FBTargetLayerList::FBTargetLayerList()
    : LayerListBase{2}
{
    layers.front().set_layer_type(mga::LayerType::skip);
    layers.back().set_layer_type(mga::LayerType::framebuffer_target);
}

void mga::FBTargetLayerList::reset_composition_layers()
{
    update_representation(2);

    layers.front().set_layer_type(mga::LayerType::skip);
    layers.back().set_layer_type(mga::LayerType::framebuffer_target);

    skip_layers_present = true;
}

void mga::FBTargetLayerList::set_composition_layers(std::list<std::shared_ptr<graphics::Renderable>> const& list)
{
    auto const needed_size = list.size() + 1;
    update_representation(needed_size);

    auto layers_it = layers.begin();
    for(auto const& renderable : list)
    {
        layers_it->set_layer_type(mga::LayerType::gl_rendered);
        layers_it->set_render_parameters(renderable->screen_position(), renderable->alpha_enabled());
        layers_it->set_buffer(*renderable->buffer(1));  // TODO: implement frameno
        layers_it++;
    }

    layers_it->set_layer_type(mga::LayerType::framebuffer_target);
    skip_layers_present = false;
}


void mga::FBTargetLayerList::set_fb_target(mg::Buffer const& buffer)
{
    geom::Rectangle const disp_frame{{0,0}, {buffer.size()}};
    if (skip_layers_present)
    {
        layers.front().set_render_parameters(disp_frame, false);
        layers.front().set_buffer(buffer);
    }

    layers.back().set_render_parameters(disp_frame, false);
    layers.back().set_buffer(buffer);
}

mga::NativeFence mga::FBTargetLayerList::fb_target_fence()
{
   return layers.back().release_fence();
}
