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
 * Authored by:
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "swapping_gl_context.h"
#include "hwc_device.h"
#include "hwc_layerlist.h"
#include "hwc_wrapper.h"
#include "framebuffer_bundle.h"
#include "buffer.h"
#include "hwc_fallback_gl_renderer.h"
#include <limits>
#include <algorithm>

namespace mg = mir::graphics;
namespace mga=mir::graphics::android;
namespace geom = mir::geometry;

namespace
{
bool plane_alpha_is_translucent(mg::Renderable const& renderable)
{
    float static const tolerance
    {
        1.0f/(2.0 * static_cast<float>(std::numeric_limits<decltype(hwc_layer_1_t::planeAlpha)>::max()))
    };
    return (renderable.alpha() < 1.0f - tolerance);
}

bool renderable_list_is_hwc_incompatible(mg::RenderableList const& list)
{
    if (list.empty())
        return true;

    for(auto const& renderable : list)
    {
        //TODO: enable planeAlpha for (hwc version >= 1.2), 90 deg rotation
        static glm::mat4 const identity;
        if (plane_alpha_is_translucent(*renderable) ||
           (renderable->transformation() != identity))
        {
            return true;
        }
    }
    return false;
}
}

mga::HwcDevice::HwcDevice(
    std::shared_ptr<HwcWrapper> const& hwc_wrapper,
    std::shared_ptr<LayerAdapter> const& layer_adapter) :
    hwc_list{layer_adapter, {}},
    hwc_wrapper(hwc_wrapper)
{
}

bool mga::HwcDevice::buffer_is_onscreen(mg::Buffer const& buffer) const
{
    /* check the handles, as the buffer ptrs might change between sets */
    auto const handle = buffer.native_buffer_handle().get();
    auto it = std::find_if(
        onscreen_overlay_buffers.begin(), onscreen_overlay_buffers.end(),
        [&handle](std::shared_ptr<mg::Buffer> const& b)
        {
            return (handle == b->native_buffer_handle().get());
        });
    return it != onscreen_overlay_buffers.end();
}

void mga::HwcDevice::post_gl(SwappingGLContext const& context)
{
    hwc_list.update_list({});

    auto buffer = context.last_rendered_buffer();
    hwc_list.setup_fb(*buffer);

    hwc_wrapper->prepare({{hwc_list.native_list(), nullptr, nullptr}});

    context.swap_buffers();

    buffer = context.last_rendered_buffer();
    hwc_list.setup_fb(*buffer);

    for(auto& layer : hwc_list)
        layer.layer.set_acquirefence_from(*buffer);

    hwc_wrapper->set({{hwc_list.native_list(), nullptr, nullptr}});
    onscreen_overlay_buffers.clear();

    for(auto& layer : hwc_list)
        layer.layer.update_from_releasefence(*buffer);

    mir::Fd retire_fd(hwc_list.retirement_fence());
}

bool mga::HwcDevice::post_overlays(
    SwappingGLContext const& context,
    RenderableList const& renderables,
    RenderableListCompositor const& list_compositor)
{
    if (renderable_list_is_hwc_incompatible(renderables))
        return false;

    hwc_list.update_list(renderables);
    auto& fbtarget = *hwc_list.additional_layers_begin();

    bool needs_commit{false};
    for(auto& layer : hwc_list)
        needs_commit |= layer.needs_commit;
    if (!needs_commit)
        return false;

    hwc_list.setup_fb(*context.last_rendered_buffer());

    hwc_wrapper->prepare({{hwc_list.native_list(), nullptr, nullptr}});

    mg::RenderableList rejected_renderables;
    std::vector<std::shared_ptr<mg::Buffer>> next_onscreen_overlay_buffers;
    auto it = hwc_list.begin();
    for(auto const& renderable : renderables)
    {
        if (it->layer.needs_gl_render())
        {
            rejected_renderables.push_back(renderable);
        }
        else
        {
            auto buffer = renderable->buffer();
            if (!buffer_is_onscreen(*buffer))
                it->layer.set_acquirefence_from(*buffer);

            next_onscreen_overlay_buffers.push_back(buffer);
        }
        it++;
    }

    if (!rejected_renderables.empty())
    {
        list_compositor.render(rejected_renderables, context);

        hwc_list.setup_fb(*context.last_rendered_buffer());
        fbtarget.layer.set_acquirefence_from(*context.last_rendered_buffer());
    }

    hwc_wrapper->set({{hwc_list.native_list(), nullptr, nullptr}});
    onscreen_overlay_buffers = std::move(next_onscreen_overlay_buffers);

    it = hwc_list.begin();
    for(auto const& renderable : renderables)
    {
        it->layer.update_from_releasefence(*renderable->buffer());
        it++;
    }
    if (!rejected_renderables.empty())
        fbtarget.layer.update_from_releasefence(*context.last_rendered_buffer());

    mir::Fd retire_fd(hwc_list.retirement_fence());
    return true;
}

void mga::HwcDevice::content_cleared()
{
    onscreen_overlay_buffers.clear();
}
