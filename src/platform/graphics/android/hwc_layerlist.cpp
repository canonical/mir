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
/* hwc layer list uses hwLayers[0] at the end of the struct */
std::shared_ptr<hwc_display_contents_1_t> alloc_layer_list(size_t num_layers)
{
    auto struct_size = sizeof(hwc_display_contents_1_t) + sizeof(hwc_layer_1_t)*(num_layers);
    auto hwc_representation = std::shared_ptr<hwc_display_contents_1_t>(
        static_cast<hwc_display_contents_1_t*>( ::operator new(struct_size)));
    hwc_representation->numHwLayers = num_layers;
    hwc_representation->retireFenceFd = -1;
    hwc_representation->flags = HWC_GEOMETRY_CHANGED;

    //aosp exynos hwc in particular, checks that these fields are non-null in hwc1.1, although
    //these fields are deprecated in hwc1.1 and later.
    hwc_representation->dpy = reinterpret_cast<void*>(0xDECAF);
    hwc_representation->sur = reinterpret_cast<void*>(0xC0FFEE);

    return hwc_representation;
}
}

mga::LayerList::LayerList()
    : hwc_representation(alloc_layer_list(1)),
      fb_target_in_use(false)
{
    hwc_representation->hwLayers[0] = mga::ForceGLLayer{};
}

#if 0
void mga::LayerList::replace_composition_layers(
    std::list<std::shared_ptr<graphics::Renderable>> const& list)
{
    if ( fb_target_in_use )
    {
        mga::HWCLayer lay = 
    }
    hwc_representation = alloc_layer_list(layer_list.size());
    for(auto &layer : list)
    {

    }
}

void mga::LayerList::set_fb_target(std::shared_ptr<NativeBuffer> const& native_buffer)
{
    if (!fb_target_in_use)
        BOOST_THROW_EXCEPTION(std::runtime_error("no HWC layers. cannot set fb target"));

    if (hwc_representation->hwLayers[fb_position].compositionType == HWC_FRAMEBUFFER_TARGET)
    {
        hwc_representation->hwLayers[fb_position] = mga::FramebufferLayer(*native_buffer);
        hwc_representation->hwLayers[fb_position].acquireFenceFd = native_buffer->copy_fence();
    }
}

mga::NativeFence mga::LayerList::framebuffer_fence()
{
    if (!fb_target_in_use)
        BOOST_THROW_EXCEPTION(std::runtime_error("no HWC layers. cannot get fb fence"));
    return hwc_representation->hwLayers[fb_position].releaseFenceFd;
}

hwc_display_contents_1_t* mga::LayerList::native_list() const
{
    return hwc_representation.get();
}
#endif
