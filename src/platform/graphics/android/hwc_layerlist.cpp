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

void mga::LayerList::update_representation(
    std::list<std::shared_ptr<mg::Renderable>> const& new_render_list)
{
    size_t needed_size = 0u;
    if (new_render_list.empty())
    {
        needed_size = 1; //need a skip layer
    } else
    {
        needed_size = new_render_list.size();
    }

    //tack on a fb target at the end if we need it
    if (fb_target_present)
    {
        needed_size++;
    }

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
        if (!composition_layers_present)
        {
            //if we are in skip mode, preserve the skip layer
            skip_layer = std::make_shared<mga::HWCLayer>(&hwc_representation->hwLayers[i++]);
            *skip_layer = *layers.front();
        }
        else
        {
            //if we are not in skip mode, make a new layer
            skip_layer = std::make_shared<mga::ForceGLLayer>(&hwc_representation->hwLayers[i++]);
        }
        new_layers.push_back(skip_layer);
    }
    else
    {
        for( auto& renderable : new_render_list)
        {
            new_layers.push_back(
                std::make_shared<mga::CompositionLayer>(&hwc_representation->hwLayers[i++], *renderable));
        }
    }


    if (fb_target_present)
    {
        std::shared_ptr<mga::HWCLayer> fb_target;
        if (layers.empty())
        {
            fb_target = std::make_shared<mga::FramebufferLayer>(&hwc_representation->hwLayers[i++]);
        }
        else
        {
            fb_target = std::make_shared<mga::HWCLayer>(&hwc_representation->hwLayers[i++]);
            //preserve the old target
            *fb_target = *layers.back();
        }

        new_layers.push_back(fb_target);
    }

    std::swap(new_layers, layers);
    std::swap(new_hwc_representation, hwc_representation);
}

mga::LayerList::LayerList(bool has_target_layer)
    : composition_layers_present(false),
      fb_target_present(has_target_layer)
{
#if 0
    skip_layer = std::make_shared<mga::ForceGLLayer>();
    fb_target_layer = std::make_shared<mga::FramebufferLayer>();
#endif
    reset_composition_layers();
}

void mga::LayerList::with_native_list(std::function<void(hwc_display_contents_1_t&)> const& fn)
{
    fn(*hwc_representation);

    if (fb_target_present)
    {
        auto fb_position = layers.size() - 1;
        fb_fence = hwc_representation->hwLayers[fb_position].releaseFenceFd;
    }
}

void mga::LayerList::reset_composition_layers()
{
    update_representation({});
}

void mga::LayerList::set_composition_layers(
    std::list<std::shared_ptr<graphics::Renderable>> const& list)
{
    update_representation(list);
}

void mga::LayerList::set_fb_target(mg::NativeBuffer const& native_buffer)
{
    (void) native_buffer;
#if 0
    *skip_layer = mga::ForceGLLayer(native_buffer);
    if (fb_target_present)
    {
        *fb_target_layer = mga::FramebufferLayer(native_buffer);
    }
#endif
}

mga::NativeFence mga::LayerList::fb_target_fence()
{
    return fb_fence;
}

mga::NativeFence mga::LayerList::retirement_fence()
{
    return retire_fence;
}
