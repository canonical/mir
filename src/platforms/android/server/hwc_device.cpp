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
}

bool mga::HwcDevice::compatible_renderlist(RenderableList const& list)
{
    if (list.empty())
        return false;

    for (auto const& renderable : list)
    {
        //TODO: enable planeAlpha for (hwc version >= 1.2), 90 deg rotation
        static glm::mat4 const identity;
        if (plane_alpha_is_translucent(*renderable) ||
           (renderable->transformation() != identity))
        {
            return false;
        }
    }
    return true;
}

mga::HwcDevice::HwcDevice(std::shared_ptr<HwcWrapper> const& hwc_wrapper) :
    hwc_wrapper(hwc_wrapper),
    posted{false},
    needed_list_count{1} //primary always connected
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

void mga::HwcDevice::commit(
    mga::DisplayName name,
    mga::LayerList& list,
    SwappingGLContext const& context,
    RenderableListCompositor const& compositor)
{
    std::unique_lock<decltype(mutex)> lk(mutex);

    if (name == mga::DisplayName::external)
        lists[1] = list.native_list();
    else if (name == mga::DisplayName::primary)
        lists[0] = list.native_list();

    displays.emplace_back(ListResources{list, context, compositor});
    if (displays.size() < needed_list_count)
    {
        posted = false;
        cv.wait(lk, [this]{ return posted; }); 
    }
    else
    {
        commit();
        displays.clear();
        posted = true;
        cv.notify_all();
    }
}

void mga::HwcDevice::commit()
{
    for(auto& display : displays)
    {
        display.list.setup_fb(display.context.last_rendered_buffer());
    }

    hwc_wrapper->prepare(lists);

    std::vector<std::shared_ptr<mg::Buffer>> next_onscreen_overlay_buffers;
    for(auto& display : displays)
    {
        if (display.list.needs_swapbuffers())
        {
            auto rejected_renderables = display.list.rejected_renderables();
            if (rejected_renderables.empty())
                display.context.swap_buffers();
            else
                display.compositor.render(std::move(rejected_renderables), display.context);
            display.list.setup_fb(display.context.last_rendered_buffer());
            display.list.swap_occurred();
        }

        //setup overlays
        for (auto& layer : display.list)
        {
            auto buffer = layer.layer.buffer();
            if (layer.layer.is_overlay() && buffer)
            {
                if (!buffer_is_onscreen(*buffer))
                    layer.layer.set_acquirefence();
                next_onscreen_overlay_buffers.push_back(buffer);
            }
        }
    }

    hwc_wrapper->set(lists);
    onscreen_overlay_buffers = std::move(next_onscreen_overlay_buffers);

    for(auto& display : displays)
    {
        for (auto& it : display.list)
            it.layer.release_buffer();

        mir::Fd retire_fd(display.list.retirement_fence());
    }
}

void mga::HwcDevice::content_cleared()
{
    onscreen_overlay_buffers.clear();
}

void mga::HwcDevice::display_added(mga::DisplayName name)
{
    std::unique_lock<decltype(mutex)> lk(mutex);
    if (name == mga::DisplayName::external)
        needed_list_count = 2;
}

void mga::HwcDevice::display_removed(mga::DisplayName name)
{
    std::unique_lock<decltype(mutex)> lk(mutex);
    if (name == mga::DisplayName::external)
        needed_list_count = 1;
}
