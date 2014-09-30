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

mga::HwcLayerEntry::HwcLayerEntry(HWCLayer && layer, bool needs_commit) :
    layer(std::move(layer)),
    needs_commit{needs_commit}
{
}

void mga::LayerList::update_list(RenderableList const& renderlist, size_t additional_layers)
{
    size_t needed_size = renderlist.size() + additional_layers; 

    if ((!hwc_representation) || hwc_representation->numHwLayers != needed_size)
    {
        hwc_representation = generate_hwc_list(needed_size);
    }

    if (layers.size() == needed_size)
    {
        auto it = layers.begin();
        for(auto renderable : renderlist)
        {
            it->needs_commit = it->layer.setup_layer(
                mga::LayerType::gl_rendered,
                renderable->screen_position(),
                renderable->shaped(), // TODO: support alpha() in future too
                *renderable->buffer());
            it++;
        }
    }
    else
    {
        std::list<HwcLayerEntry> new_layers;
        auto i = 0u;
        for(auto const& renderable : renderlist)
        {
            new_layers.emplace_back(
                mga::HWCLayer(
                    mga::LayerType::gl_rendered,
                    renderable->screen_position(),
                    renderable->shaped(), // TODO: support alpha() in future
                    *renderable->buffer(),
                    hwc_representation, i++), true);
        }

        for(; i < needed_size; i++)
        {
            new_layers.emplace_back(mga::HWCLayer(hwc_representation, i), false);
        }
        layers = std::move(new_layers);
    }

    if (additional_layers == 0)
    {
        first_additional_layer = layers.end();
    }
    else
    {
        first_additional_layer = layers.begin();
        std::advance(first_additional_layer, renderlist.size());
    }
}

std::list<mga::HwcLayerEntry>::iterator mga::LayerList::begin()
{
    return layers.begin(); 
}

std::list<mga::HwcLayerEntry>::iterator mga::LayerList::additional_layers_begin()
{
    return first_additional_layer;
}

std::list<mga::HwcLayerEntry>::iterator mga::LayerList::end()
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

mga::LayerList::LayerList(
    RenderableList const& renderlist,
    size_t additional_layers)
{
    update_list(renderlist, additional_layers);
}

