/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "android_platform.h"
#include "android_buffer_allocator.h"
#include "android_display.h"
#include "android_framebuffer_window.h"
#include "mir/graphics/platform_ipc_package.h"
#include "mir/compositor/buffer_id.h"

#include <ui/FramebufferNativeWindow.h>

#include <boost/throw_exception.hpp>

#include <stdexcept>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mc=mir::compositor;

std::shared_ptr<mc::GraphicBufferAllocator> mga::AndroidPlatform::create_buffer_allocator(
        const std::shared_ptr<mg::BufferInitializer>& /*buffer_initializer*/)
{
    return std::make_shared<mga::AndroidBufferAllocator>();
}

std::shared_ptr<mg::Display> mga::AndroidPlatform::create_display()
{
 //   ::android::FramebufferNativeWindow *win;
  //  win = (::android::FramebufferNativeWindow*) android_createDisplaySurface();
    //::android::sp< ::android::FramebufferNativeWindow> ref(win);

    auto android_window = std::make_shared< ::android::FramebufferNativeWindow >();
 
//    auto android_window = std::shared_ptr<ANativeWindow>(win,
//                                                         [](ANativeWindow*) {});
//    exit(-1);
//    auto android_window = std::shared_ptr<ANativeWindow>(android_createDisplaySurface(),
//                                                         [](ANativeWindow*) {});
//    auto window = new ::android::FramebufferNativeWindow();
//    ::android::sp< ::android::FramebufferNativeWindow> ref(w);
    //auto android_window = std::make_shared<  ::android::FramebufferNativeWindow>();
    //if (!android_window.get())
    //    BOOST_THROW_EXCEPTION(std::runtime_error("could not open FB window"));
    auto window = std::make_shared<mga::AndroidFramebufferWindow> (android_window);

    return std::make_shared<mga::AndroidDisplay>(window);
}

std::shared_ptr<mg::PlatformIPCPackage> mga::AndroidPlatform::get_ipc_package()
{
    return std::make_shared<mg::PlatformIPCPackage>();
}

std::shared_ptr<mg::Platform> mg::create_platform(std::shared_ptr<DisplayReport> const& /*TODO*/)
{
    return std::make_shared<mga::AndroidPlatform>();
}
