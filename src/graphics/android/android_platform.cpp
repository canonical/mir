/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by:
 *   Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include <mir/graphics/android/android_platform.h>
#include <mir/graphics/android/android_buffer_allocator.h>
#include <mir/graphics/android/android_display.h>
#include <mir/graphics/android/android_framebuffer_window.h>

#include <ui/FramebufferNativeWindow.h>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mc=mir::compositor;

std::shared_ptr<mc::GraphicBufferAllocator> mga::AndroidPlatform::create_buffer_allocator()
{
    return std::make_shared<mga::AndroidBufferAllocator>();
}

std::shared_ptr<mg::Display> mga::AndroidPlatform::create_display()
{
    auto android_window = std::shared_ptr<ANativeWindow>((ANativeWindow*) new ::android::FramebufferNativeWindow);
    auto window = std::make_shared<mga::AndroidFramebufferWindow> (android_window);

    return std::make_shared<mga::AndroidDisplay>(window);
}

std::shared_ptr<mg::Platform> mg::create_platform()
{
    return std::make_shared<mga::AndroidPlatform>();
}

std::shared_ptr<mc::GraphicBufferAllocator> mg::create_buffer_allocator(
        const std::shared_ptr<Platform>& platform)
{
    return platform->create_buffer_allocator();
}

std::shared_ptr<mg::Display> mg::create_display(
        const std::shared_ptr<Platform>& platform)
{
    return platform->create_display();
}
