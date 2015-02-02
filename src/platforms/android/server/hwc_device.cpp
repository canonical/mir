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
unsigned int const primary_only{1};
unsigned int const both_displays{2};

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
    committed{false},
    needed_list_count{primary_only}
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

    if ((name == mga::DisplayName::external) && (needed_list_count == primary_only))
    {
        lists[1] = nullptr;
        return;
    }

    if ((name == mga::DisplayName::external) && (needed_list_count == both_displays))
    {
        lists[1] = list.native_list();
        displays.emplace_back(ListResources{name, list, context, compositor});
    }
    else if (name == mga::DisplayName::primary)
    {
        lists[0] = list.native_list();
        displays.emplace_back(ListResources{name, list, context, compositor});
    }
   
    context.release_current();

    if (name == mga::DisplayName::primary)
    {
        list_cv.wait(lk, [this]{ return displays.size() == needed_list_count; });
        commit();

        committed = true;
        displays.clear();
        commit_cv.notify_all();
    }
    else
    {
        list_cv.notify_all();
        commit_cv.wait(lk, [this] { return committed; });
        committed = false;
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
            display.context.make_current();
            if (rejected_renderables.empty())
                display.context.swap_buffers();
            else
                display.compositor.render(std::move(rejected_renderables), display.context);
            display.context.release_current();
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

void mga::HwcDevice::start_posting_external_display()
{
    std::unique_lock<decltype(mutex)> lk(mutex);
    needed_list_count = both_displays;
    committed = false;
}

void mga::HwcDevice::stop_posting_external_display()
{
    std::unique_lock<decltype(mutex)> lk(mutex);
    lists[1] = nullptr;
    needed_list_count = primary_only;
    auto it = std::find_if(displays.begin(), displays.end(),
        [&](mga::HwcDevice::ListResources r){ return r.name == mga::DisplayName::external;});
    if (it != displays.end())
        displays.erase(it);
    list_cv.notify_all();
}
