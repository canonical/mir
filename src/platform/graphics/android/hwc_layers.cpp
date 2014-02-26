/*
 * Copyright © 2014 Canonical Ltd.
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

#include <boost/throw_exception.hpp>
#include <stdexcept>
#include <cstring>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace geom=mir::geometry;

mga::HWCLayer& mga::HWCLayer::operator=(HWCLayer && other)
{
    hwc_layer = other.hwc_layer;
    hwc_list = std::move(other.hwc_list);
    visible_rect = std::move(other.visible_rect);
    return *this;
}

mga::HWCLayer::HWCLayer(HWCLayer && other)
    : hwc_layer(std::move(other.hwc_layer)),
      hwc_list(std::move(other.hwc_list)),
      visible_rect(std::move(other.visible_rect))
{
}

mga::HWCLayer::HWCLayer(std::shared_ptr<hwc_display_contents_1_t> list, size_t layer_index)
    : hwc_layer(&list->hwLayers[layer_index]),
      hwc_list(list),
      associated_buffer(nullptr) //todo: take this as a constructor param 
{
    memset(hwc_layer, 0, sizeof(hwc_layer_1_t));
    memset(&visible_rect, 0, sizeof(hwc_rect_t));

    hwc_layer->hints = 0;
    hwc_layer->transform = 0;
    hwc_layer->acquireFenceFd = -1;
    hwc_layer->releaseFenceFd = -1;
    hwc_layer->blending = HWC_BLENDING_NONE;

    hwc_layer->visibleRegionScreen.numRects=1;
    hwc_layer->visibleRegionScreen.rects= &visible_rect;
}

mga::HWCLayer::HWCLayer(
    LayerType type,
    geometry::Rectangle position,
    bool alpha_enabled,
    std::shared_ptr<hwc_display_contents_1_t> list,
    size_t layer_index)
     : HWCLayer(list, layer_index)
{
    set_layer_type(type);
    set_render_parameters(position, alpha_enabled);
}

bool mga::HWCLayer::needs_gl_render() const
{
    return ((hwc_layer->compositionType == HWC_FRAMEBUFFER) || (hwc_layer->flags == HWC_SKIP_LAYER));
}

void mga::HWCLayer::update_fence_and_release_buffer()
{
    if (hwc_layer->compositionType != HWC_FRAMEBUFFER)
    { 
        associated_buffer->update_fence(hwc_layer->releaseFenceFd);
        hwc_layer->releaseFenceFd = -1;
        hwc_layer->acquireFenceFd = -1;
        associated_buffer.reset();
    }
}

void mga::HWCLayer::set_layer_type(LayerType type)
{
    hwc_layer->flags = 0;
    switch(type)
    {
        case mga::LayerType::skip:
            hwc_layer->compositionType = HWC_FRAMEBUFFER;
            hwc_layer->flags = HWC_SKIP_LAYER;
        break;

        case mga::LayerType::gl_rendered:
            hwc_layer->compositionType = HWC_FRAMEBUFFER;
        break;

        case mga::LayerType::framebuffer_target:
            hwc_layer->compositionType = HWC_FRAMEBUFFER_TARGET;
        break;

        case mga::LayerType::overlay: //driver is the only one who can set to overlay
        default:
            BOOST_THROW_EXCEPTION(std::logic_error("invalid layer type"));
    }
}

void mga::HWCLayer::set_render_parameters(geometry::Rectangle position, bool alpha_enabled)
{
    if (alpha_enabled)
        hwc_layer->blending = HWC_BLENDING_COVERAGE;
    else
        hwc_layer->blending = HWC_BLENDING_NONE;

    /* note, if the sourceCrop and DisplayFrame sizes differ, the output will be linearly scaled */
    hwc_layer->displayFrame = 
    {
        position.top_left.x.as_int(),
        position.top_left.y.as_int(),
        position.size.width.as_int(),
        position.size.height.as_int()
    };

    visible_rect = hwc_layer->displayFrame;
}

void mga::HWCLayer::set_buffer(Buffer const& buffer)
{
    associated_buffer.reset();
    associated_buffer = buffer.native_buffer_handle();
    updated = (hwc_layer->handle != associated_buffer->handle());

    hwc_layer->handle = associated_buffer->handle();
    hwc_layer->sourceCrop = 
    {
        0, 0,
        associated_buffer->anwb()->width,
        associated_buffer->anwb()->height
    };
}

void mga::HWCLayer::prepare_for_draw()
{
    //we shouldn't be copying the FD unless the HWC has marked this as a buffer its interested in.
    //we disregard fences that haven't changed, as the hwc will still own the buffer
    if (updated && (((hwc_layer->compositionType == HWC_OVERLAY) ||
        (hwc_layer->compositionType == HWC_FRAMEBUFFER_TARGET))))
    {
        hwc_layer->acquireFenceFd = associated_buffer->copy_fence();
    }
    //the HWC is not interested in this buffer. we can release the buffer.
    else if (hwc_layer->compositionType == HWC_FRAMEBUFFER)
    {
        hwc_layer->acquireFenceFd = -1;
        associated_buffer.reset();
    }

    hwc_layer->releaseFenceFd = -1;
}

bool mga::HWCLayer::needs_hwc_commit() const
{
    return (updated || needs_gl_render());
}
