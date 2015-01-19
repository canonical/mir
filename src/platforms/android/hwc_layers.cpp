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

#include <limits>
#include <boost/throw_exception.hpp>
#include <stdexcept>
#include <cstring>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace geom=mir::geometry;

namespace
{
decltype(hwc_layer_1_t::planeAlpha) static const plane_alpha_max{
    std::numeric_limits<decltype(hwc_layer_1_t::planeAlpha)>::max()
};
}

void mga::FloatSourceCrop::fill_source_crop(
    hwc_layer_1_t& hwc_layer, geometry::Rectangle const& crop_rect) const
{
    hwc_layer.sourceCropf = 
    {
        crop_rect.top_left.x.as_float(),
        crop_rect.top_left.y.as_float(),
        crop_rect.size.width.as_float(),
        crop_rect.size.height.as_float()
    };
}

bool mga::FloatSourceCrop::needs_fb_target() const
{
    return true;
}

void mga::IntegerSourceCrop::fill_source_crop(
    hwc_layer_1_t& hwc_layer, geometry::Rectangle const& crop_rect) const
{
    hwc_layer.sourceCropi = 
    {
        crop_rect.top_left.x.as_int(),
        crop_rect.top_left.y.as_int(),
        crop_rect.size.width.as_int(),
        crop_rect.size.height.as_int()
    };
}

bool mga::IntegerSourceCrop::needs_fb_target() const
{
    return true;
}

void mga::Hwc10Adapter::fill_source_crop(
    hwc_layer_1_t& hwc_layer, geometry::Rectangle const& crop_rect) const
{
    hwc_layer.sourceCropi = 
    {
        crop_rect.top_left.x.as_int(),
        crop_rect.top_left.y.as_int(),
        crop_rect.size.width.as_int(),
        crop_rect.size.height.as_int()
    };
}

bool mga::Hwc10Adapter::needs_fb_target() const
{
    return false;
}

mga::HWCLayer& mga::HWCLayer::operator=(HWCLayer && other)
{
    layer_adapter = std::move(other.layer_adapter);
    hwc_layer = other.hwc_layer;
    hwc_list = std::move(other.hwc_list);
    visible_rect = std::move(other.visible_rect);
    return *this;
}

mga::HWCLayer::HWCLayer(HWCLayer && other)
    : layer_adapter{std::move(other.layer_adapter)},
      hwc_layer(std::move(other.hwc_layer)),
      hwc_list(std::move(other.hwc_list)),
      visible_rect(std::move(other.visible_rect))
{
}

mga::HWCLayer::HWCLayer(
    std::shared_ptr<LayerAdapter> const& layer_adapter,
    std::shared_ptr<hwc_display_contents_1_t> const& list,
    size_t layer_index) :
    layer_adapter(layer_adapter),
    hwc_layer(&list->hwLayers[layer_index]),
    hwc_list(list)
{
    memset(hwc_layer, 0, sizeof(hwc_layer_1_t));
    memset(&visible_rect, 0, sizeof(hwc_rect_t));

    hwc_layer->hints = 0;
    hwc_layer->transform = 0;
    hwc_layer->acquireFenceFd = -1;
    hwc_layer->releaseFenceFd = -1;
    hwc_layer->blending = HWC_BLENDING_NONE;
    hwc_layer->planeAlpha = plane_alpha_max;

    hwc_layer->visibleRegionScreen.numRects=1;
    hwc_layer->visibleRegionScreen.rects= &visible_rect;
}

mga::HWCLayer::HWCLayer(
    std::shared_ptr<LayerAdapter> const& layer_adapter,
    std::shared_ptr<hwc_display_contents_1_t> const& list,
    size_t layer_index,
    LayerType type,
    geometry::Rectangle const& position,
    bool alpha_enabled,
    Buffer const& buffer) :
    HWCLayer(layer_adapter, list, layer_index)
{
    setup_layer(type, position, alpha_enabled, buffer);
}

bool mga::HWCLayer::needs_gl_render() const
{
    return (hwc_layer->compositionType == HWC_FRAMEBUFFER);
}

void mga::HWCLayer::update_from_releasefence(mg::Buffer const& buffer)
{
    if (hwc_layer->compositionType != HWC_FRAMEBUFFER)
    { 
        auto const& native_buffer = buffer.native_buffer_handle();
        native_buffer->update_usage(hwc_layer->releaseFenceFd, mga::BufferAccess::read);
        hwc_layer->releaseFenceFd = -1;
        hwc_layer->acquireFenceFd = -1;
    }
}

bool mga::HWCLayer::setup_layer(
    LayerType type,
    geometry::Rectangle const& position,
    bool alpha_enabled,
    Buffer const& buffer)
{
    bool needs_commit = needs_gl_render();

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

    if (alpha_enabled)
        hwc_layer->blending = HWC_BLENDING_PREMULT;
    else
        hwc_layer->blending = HWC_BLENDING_NONE;

    hwc_layer->planeAlpha = plane_alpha_max;

    /* note, if the sourceCrop and DisplayFrame sizes differ, the output will be linearly scaled */
    hwc_layer->displayFrame = 
    {
        position.top_left.x.as_int(),
        position.top_left.y.as_int(),
        position.bottom_right().x.as_int(),
        position.bottom_right().y.as_int()
    };

    geom::Rectangle crop_rect{{0,0}, buffer.size()};
    layer_adapter->fill_source_crop(*hwc_layer, crop_rect);

    visible_rect = hwc_layer->displayFrame;

    auto const& native_buffer = buffer.native_buffer_handle();
    needs_commit |= (hwc_layer->handle != native_buffer->handle());
    hwc_layer->handle = native_buffer->handle();

    return needs_commit;
}

void mga::HWCLayer::set_acquirefence_from(mg::Buffer const& buffer)
{
    hwc_layer->releaseFenceFd = -1;
    hwc_layer->acquireFenceFd = -1;
    //we shouldn't be copying the FD unless the HWC has marked this as a buffer its interested in.
    //we disregard fences that haven't changed, as the hwc will still own the buffer
    if (!needs_gl_render())
    {
        auto const& native_buffer = buffer.native_buffer_handle();
        hwc_layer->acquireFenceFd = native_buffer->copy_fence();
    }
}
