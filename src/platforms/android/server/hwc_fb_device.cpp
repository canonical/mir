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

#include "hwc_fb_device.h"
#include "framebuffer_bundle.h"
#include "hwc_wrapper.h"
#include "hwc_fallback_gl_renderer.h"
#include "mir/graphics/buffer.h"
#include "mir/graphics/android/native_buffer.h"
#include "mir/graphics/android/android_format_conversion-inl.h"
#include "swapping_gl_context.h"
#include "hwc_layerlist.h"

#include <boost/throw_exception.hpp>
#include <sstream>
#include <stdexcept>
#include <algorithm>

namespace mg = mir::graphics;
namespace mga = mir::graphics::android;
namespace geom = mir::geometry;

mga::HwcFbDevice::HwcFbDevice(
    std::shared_ptr<HwcWrapper> const& hwc_wrapper,
    std::shared_ptr<framebuffer_device_t> const& fb_device) :
    hwc_wrapper(hwc_wrapper), 
    fb_device(fb_device),
    vsync_subscription{
        [hwc_wrapper, this]{
            using namespace std::placeholders;
            hwc_wrapper->subscribe_to_events(this,
                std::bind(&mga::HwcFbDevice::notify_vsync, this, _1, _2),
                [](mga::DisplayName, bool){},
                []{});
        },
        [hwc_wrapper, this]{
            hwc_wrapper->unsubscribe_from_events(this);
        }
    }
{
}

void mga::HwcFbDevice::commit(std::list<DisplayContents> const& contents)
{
    auto primary_contents = std::find_if(contents.begin(), contents.end(),
        [](mga::DisplayContents const& c) {
            return (c.name == mga::DisplayName::primary);
    });
    if (primary_contents == contents.end()) return;
    auto& layer_list = primary_contents->list;
    auto& context = primary_contents->context;

    layer_list.setup_fb(context.last_rendered_buffer());

    if (auto display_list = layer_list.native_list())
    {
        hwc_wrapper->prepare({{display_list, nullptr, nullptr}});
        display_list->dpy = eglGetCurrentDisplay();
        display_list->sur = eglGetCurrentSurface(EGL_DRAW);

        //set() may affect EGL state by calling eglSwapBuffers.
        //HWC 1.0 is the only version of HWC that can do this.
        hwc_wrapper->set({{display_list, nullptr, nullptr}});
    }
    else
    {
        std::stringstream ss;
        ss << "error accessing list during hwc prepare()";
        BOOST_THROW_EXCEPTION(std::runtime_error(ss.str()));
    }

    auto& buffer = *context.last_rendered_buffer();
    auto native_buffer = buffer.native_buffer_handle();
    native_buffer->ensure_available_for(mga::BufferAccess::read);
    if (fb_device->post(fb_device.get(), native_buffer->handle()) != 0)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("error posting with fb device"));
    }

    std::unique_lock<std::mutex> lk(vsync_wait_mutex);
    vsync_occurred = false;
    vsync_trigger.wait(lk, [this]{return vsync_occurred;});
}

void mga::HwcFbDevice::notify_vsync(mga::DisplayName, std::chrono::nanoseconds)
{
    std::unique_lock<std::mutex> lk(vsync_wait_mutex);
    vsync_occurred = true;
    vsync_trigger.notify_all();
}

bool mga::HwcFbDevice::compatible_renderlist(RenderableList const&)
{
    return false;
}

void mga::HwcFbDevice::content_cleared()
{
}

std::chrono::milliseconds mga::HwcFbDevice::recommended_sleep() const
{
    return std::chrono::milliseconds::zero();
}
