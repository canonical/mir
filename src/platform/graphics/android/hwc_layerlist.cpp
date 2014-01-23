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

#include "mir/graphics/android/sync_fence.h"
#include "mir/graphics/android/native_buffer.h"
#include "hwc_layerlist.h"

#include <cstring>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace geom=mir::geometry;

mga::LayerList::LayerList(std::initializer_list<HWCLayer> const& layer_list)
{
    auto struct_size = sizeof(hwc_display_contents_1_t) + sizeof(hwc_layer_1_t)*(layer_list.size());
    hwc_representation = std::shared_ptr<hwc_display_contents_1_t>(
        static_cast<hwc_display_contents_1_t*>( ::operator new(struct_size)));

    auto i = 0u;
    for(auto& layer : layer_list)
    {
        hwc_representation->hwLayers[i++] = layer;
    }
    hwc_representation->numHwLayers = layer_list.size();
    hwc_representation->retireFenceFd = -1;
    hwc_representation->flags = HWC_GEOMETRY_CHANGED;

    //aosp exynos hwc in particular, checks that these fields are non-null in hwc1.1, although
    //these fields are deprecated in hwc1.1 and later.
    hwc_representation->dpy = reinterpret_cast<void*>(0xDECAF);
    hwc_representation->sur = reinterpret_cast<void*>(0xC0FFEE);

}

void mga::LayerList::set_fb_target(std::shared_ptr<NativeBuffer> const& native_buffer)
{
    if (hwc_representation->numHwLayers == 2)
    {
        if (hwc_representation->hwLayers[0].flags == HWC_SKIP_LAYER)
        {
            hwc_representation->hwLayers[0] = mga::ForceGLLayer(*native_buffer);
        }

        if (hwc_representation->hwLayers[1].compositionType == HWC_FRAMEBUFFER_TARGET)
        {
            hwc_representation->hwLayers[1] = mga::FramebufferLayer(*native_buffer);
            hwc_representation->hwLayers[1].acquireFenceFd = native_buffer->copy_fence();
        }
    }
}

mga::NativeFence mga::LayerList::framebuffer_fence()
{
    auto fb_position = hwc_representation->numHwLayers - 1;
    return hwc_representation->hwLayers[fb_position].releaseFenceFd;
}

hwc_display_contents_1_t* mga::LayerList::native_list() const
{
    return hwc_representation.get();
}
