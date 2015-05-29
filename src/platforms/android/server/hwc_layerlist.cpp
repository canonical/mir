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

size_t mga::LayerList::additional_layers_for(mga::LayerList::Mode mode)
{
    switch (mode)
    {
        case Mode::skip_and_target:
            return 2;
        case Mode::skip_only:
        case Mode::target_only:
            return 1;
        default:
        case Mode::no_extra_layers:
            return 0;
    }
}

void mga::LayerList::update_list_mode(mg::RenderableList const& renderlist)
{
    if (renderlist.empty() && layer_adapter->needs_fb_target())
        mode = Mode::skip_and_target;
    else if (!renderlist.empty() && layer_adapter->needs_fb_target())
        mode = Mode::target_only;
    else if (renderlist.empty() && !layer_adapter->needs_fb_target())
        mode = Mode::skip_only;
    else    
        mode = Mode::no_extra_layers;
}

void mga::LayerList::update_list(RenderableList const& renderlist, geometry::Displacement offset)
{
    renderable_list = renderlist;
    update_list_mode(renderlist);
    size_t additional_layers = additional_layers_for(mode);
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
            auto position = renderable->screen_position();
            position.top_left = position.top_left - offset;
            it->needs_commit = it->layer.setup_layer(
                mga::LayerType::gl_rendered,
                position,
                renderable->shaped(), // TODO: support alpha() in future too
                renderable->buffer());
            it++;
        }
    }
    else
    {
        std::list<HwcLayerEntry> new_layers;
        auto i = 0u;
        for(auto const& renderable : renderlist)
        {
            auto position = renderable->screen_position();
            position.top_left = position.top_left - offset;
            new_layers.emplace_back(
                mga::HWCLayer(
                    layer_adapter, hwc_representation, i++,
                    mga::LayerType::gl_rendered,
                    position,
                    renderable->shaped(), // TODO: support alpha() in future
                    renderable->buffer()), true);
        }

        for(; i < needed_size; i++)
        {
            new_layers.emplace_back(mga::HWCLayer(layer_adapter, hwc_representation, i), false);
        }
        layers = std::move(new_layers);
    }
}

std::list<mga::HwcLayerEntry>::iterator mga::LayerList::begin()
{
    return layers.begin(); 
}

std::list<mga::HwcLayerEntry>::iterator mga::LayerList::end()
{
    return layers.end(); 
}

hwc_display_contents_1_t* mga::LayerList::native_list()
{
    return hwc_representation.get();
}

mga::NativeFence mga::LayerList::retirement_fence()
{
    renderable_list.clear();
    return hwc_representation->retireFenceFd;
}

mga::LayerList::LayerList(
    std::shared_ptr<LayerAdapter> const& layer_adapter,
    RenderableList const& renderlist,
    geometry::Displacement list_offset) :
    layer_adapter{layer_adapter}
{
    update_list(renderlist, list_offset);
}

bool mga::LayerList::needs_swapbuffers()
{
    bool any_rendered = false;
    for (auto const& layer : layers)
        any_rendered |= layer.layer.needs_gl_render();
    return any_rendered;
}

mg::RenderableList mga::LayerList::rejected_renderables()
{
    mg::RenderableList rejected_renderables;
    auto it = layers.begin();
    for (auto const& renderable : renderable_list)
    {
        if (it->layer.needs_gl_render())
            rejected_renderables.push_back(renderable);
        it++;
    }
    return rejected_renderables;
}

void mga::LayerList::setup_fb(std::shared_ptr<mg::Buffer> const& fb)
{
    geom::Rectangle const disp_frame{{0,0}, {fb->size()}};
    auto it = layers.begin();
    std::advance(it, renderable_list.size());

    if (mode == Mode::skip_only)
    {
        it->layer.setup_layer(mga::LayerType::skip, disp_frame, false, fb);
    }
    else if (mode == Mode::target_only)
    {
        it->layer.setup_layer(mga::LayerType::framebuffer_target, disp_frame, false, fb);
    }
    else if (mode == Mode::skip_and_target)
    {
        it++->layer.setup_layer(mga::LayerType::skip, disp_frame, false, fb);
        it->layer.setup_layer(mga::LayerType::framebuffer_target, disp_frame, false, fb);
    }
}

void mga::LayerList::swap_occurred()
{
    if ((mode == Mode::target_only) || (mode == Mode::skip_and_target))
        layers.back().layer.set_acquirefence();
}
