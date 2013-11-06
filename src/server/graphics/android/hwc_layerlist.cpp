/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
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

mga::HWCLayer::HWCLayer(HWCLayerType type, mg::NativeBuffer const& buffer, bool must_use_gl)
{
    int skip_flag = 0;
    if (must_use_gl)
    {
        skip_flag = HWC_SKIP_LAYER;
    }

    compositionType = type;
    hints = 0;
    flags = skip_flag;
    transform = 0;
    blending = HWC_BLENDING_NONE;
    //TODO: acquireFenceFd should be buffer.fence()
    acquireFenceFd = -1;
    releaseFenceFd = -1;

    visible_rect.top = 0;
    visible_rect.left = 0;
    visible_rect.bottom = buffer.anwb()->height;
    visible_rect.right = buffer.anwb()->width; 
    sourceCrop = visible_rect;
    displayFrame = visible_rect;
    visibleRegionScreen.numRects=1;
    visibleRegionScreen.rects= &visible_rect;
}

mga::FramebufferLayer::FramebufferLayer(mg::NativeBuffer const& buffer)
    : HWCLayer(mga::HWCLayerType::framebuffer, buffer, false)
{
}

mga::CompositionLayer::CompositionLayer(mg::NativeBuffer const& buffer, bool must_use_gl)
    : HWCLayer(mga::HWCLayerType::gles, buffer, must_use_gl)
{
}

#if 0
mga::LayerList::LayerList()
    : layer_list{std::make_shared<HWCFBLayer>()},
      hwc_representation{std::make_shared<hwc_display_contents_1_t>()}
{
    memset(hwc_representation.get(), 0, sizeof(hwc_display_contents_1_t));
    update_list();
}

void mga::LayerList::set_fb_target(std::shared_ptr<mg::Buffer> const& buffer)
{
    auto native_buffer = buffer->native_buffer_handle();
    native_buffer->wait_for_content();

    geom::Point pt{0, 0};
    geom::Rectangle rect{pt, buffer->size()};
    HWCRect display_rect(rect);

    auto fb_layer = std::make_shared<HWCFBLayer>(native_buffer->handle(), display_rect);
    layer_list[fb_position] = fb_layer;

    update_list();
} 

void mga::LayerList::update_list()
{
    if (layer_list.size() != hwc_representation->numHwLayers)
    {
        auto struct_size = sizeof(hwc_display_contents_1_t) + sizeof(hwc_layer_1_t)*(layer_list.size());
        hwc_representation = std::shared_ptr<hwc_display_contents_1_t>(
            static_cast<hwc_display_contents_1_t*>( ::operator new(struct_size)));

        hwc_representation->numHwLayers = layer_list.size();
    }

    auto i = 0u;
    for( auto& layer : layer_list)
    {
        hwc_representation->hwLayers[i++] = *layer;
    }
    hwc_representation->retireFenceFd = -1;
}

hwc_display_contents_1_t* mga::LayerList::native_list() const
{
    return hwc_representation.get();
}
#endif
