/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
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

#include "android_fb_factory.h"
#include "android_framebuffer_window.h"
#include "android_display.h"

#include <ui/FramebufferNativeWindow.h>
#include <boost/throw_exception.hpp>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;

std::shared_ptr<mg::Display> mga::AndroidFBFactory::create_hwc1_1_gpu_display() const
{
    //TODO: this will construct the hwc 1.1 framebuffer later
    return create_gpu_display();
}

/* note: gralloc seems to choke when this is opened/closed more than once per process. must investigate drivers further */
std::shared_ptr<mg::Display> mga::AndroidFBFactory::create_gpu_display() const
{
    auto android_window = std::make_shared< ::android::FramebufferNativeWindow>();
    if (!android_window.get())
        BOOST_THROW_EXCEPTION(std::runtime_error("could not open FB window"));
    auto window = std::make_shared<mga::AndroidFramebufferWindow> (android_window);

    return std::make_shared<mga::AndroidDisplay>(window);
}
