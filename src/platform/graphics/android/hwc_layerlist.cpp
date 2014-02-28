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

mga::LayerList::LayerList(
    std::list<std::shared_ptr<mg::Renderable>> const& renderlist,
    size_t additional_layers)
{
    update_list_and_check_if_changed(renderlist, additional_layers);
}

bool mga::LayerList::update_list_and_check_if_changed(
    std::list<std::shared_ptr<mg::Renderable>> const& renderlist,
    size_t additional_layers)
{
#if 0
    size_t current_list_size = 

    bool any_buffer_updated = false;
    if (hwc_representation->numHwLayers != needed_size)
    {
        hwc_representation = generate_hwc_list(needed_size);
    }

    if (layers.size() == needed_size)
    {
        auto layers_it = layers.begin();
        for(auto renderable : renderlist)
        {
            layers_it->set_render_parameters(
                renderable->screen_position(), renderable->alpha_enabled());
            layers_it->set_buffer(*renderable->buffer());
            any_buffer_updated |= layers_it->needs_hwc_commit(); 
            layers_it++;
        }
    }
    else
    {
        any_buffer_updated = true;
        std::list<HWCLayer> new_layers;
        auto i = 0u;
        for(auto const& renderable : renderlist)
        {
            new_layers.emplace_back(
                mga::HWCLayer(
                    mga::LayerType::gl_rendered,
                    renderable->screen_position(),
                    renderable->alpha_enabled(),
                    hwc_representation, i++));
            new_layers.back().set_buffer(*renderable->buffer());
        }

        for(; i < needed_size; i++)
        {
            new_layers.emplace_back(mga::HWCLayer(hwc_representation, i));
        }
        layers = std::move(new_layers);
    }
    return any_buffer_updated;
#endif
    (void)renderlist; (void) additional_layers;
    return true;
}

std::list<mga::HWCLayer>::iterator mga::LayerList::renderable_layers_begin()
{
    return layers.begin(); 
}

std::list<mga::HWCLayer>::iterator mga::LayerList::additional_layers_begin()
{
    return layers.begin(); 
}

std::list<mga::HWCLayer>::iterator mga::LayerList::end()
{
    return layers.end(); 
}

std::weak_ptr<hwc_display_contents_1_t> mga::LayerList::native_list()
{
    return hwc_representation;
}

mga::NativeFence mga::LayerList::retirement_fence()
{
    return hwc_representation->retireFenceFd;
}

#if 0
mga::LayerList::LayerList()
    : LayerListBase{{}, 1}
{
    layers.back().set_layer_type(mga::LayerType::skip);
}
#endif
