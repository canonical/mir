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
#include "mir/raii.h"
#include <limits>
#include <algorithm>
#include <chrono>
#include <thread>
#include <mutex>

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

std::once_flag checked_environment;
std::chrono::milliseconds force_bypass_sleep(-1);

void check_environment()
{
    std::call_once(checked_environment, []()
    {
        char const* env = getenv("MIR_DRIVER_FORCE_BYPASS_SLEEP");
        if (env != NULL)
            force_bypass_sleep = std::chrono::milliseconds(atoi(env));
    });
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
    hwc_wrapper(hwc_wrapper)
{
    check_environment();
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

void mga::HwcDevice::commit(std::list<DisplayContents> const& contents)
{
    std::array<hwc_display_contents_1*, HWC_NUM_DISPLAY_TYPES> lists{{ nullptr, nullptr, nullptr }};
    std::vector<std::shared_ptr<mg::Buffer>> next_onscreen_overlay_buffers;

    for (auto& content : contents)
    {
        if (content.name == mga::DisplayName::primary)
            lists[HWC_DISPLAY_PRIMARY] = content.list.native_list();
        else if (content.name == mga::DisplayName::external) 
            lists[HWC_DISPLAY_EXTERNAL] = content.list.native_list();

        content.list.setup_fb(content.context.last_rendered_buffer());
    }

    hwc_wrapper->prepare(lists);

    bool purely_overlays = true;

    for (auto& content : contents)
    {
        if (content.list.needs_swapbuffers())
        {
            auto rejected_renderables = content.list.rejected_renderables();
            auto current_context = mir::raii::paired_calls(
                [&]{ content.context.make_current(); },
                [&]{ content.context.release_current(); });
            if (rejected_renderables.empty())
                content.context.swap_buffers();
            else
                content.compositor.render(std::move(rejected_renderables), content.list_offset, content.context);
            content.list.setup_fb(content.context.last_rendered_buffer());
            content.list.swap_occurred();
            purely_overlays = false;
        }
    
        //setup overlays
        for (auto& layer : content.list)
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

    for (auto& content : contents)
    {
        for (auto& it : content.list)
            it.layer.release_buffer();

        mir::Fd retire_fd(content.list.retirement_fence());
    }

    if (purely_overlays)
    {
        /*
         * "Predictive bypass": If we're just displaying pure overlays on this
         * frame then it's very likely the same will be true on the next frame.
         * In that case we don't need to spare any time for GL rendering.
         * Instead just sleep for a significant portion of the frame. This will
         * ensure the scene snapshot that goes into the next frame is
         * younger when it hits the screen, and so has visibly lower latency.
         *   This has extra benefit for the overlays of software cursors and
         * touchpoints as the client surface underneath is then given enough
         * time to update itself and better match (ie. "stick to") the cursor
         * above it.
         */

        // Test results (how long can we sleep for without missing a frame?):
        //   arale:   10ms  (TODO: Find out why arale is so slow)
        //   mako:    15ms
        //   krillin: 11ms (to be fair, the display is 67Hz)
        using namespace std;
        auto delay = force_bypass_sleep >= 0ms ? force_bypass_sleep : 10ms;
        std::this_thread::sleep_for(delay);
    }
}

void mga::HwcDevice::content_cleared()
{
    onscreen_overlay_buffers.clear();
}
