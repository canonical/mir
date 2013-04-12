/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
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

#include "android_platform.h"
#include "android_buffer_allocator.h"
#include "android_hwc_factory.h"
#include "android_display_allocator.h"
#include "android_display_factory.h"
#include "mir/graphics/platform_ipc_package.h"
#include "mir/compositor/buffer_id.h"

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mc=mir::compositor;

std::shared_ptr<mc::GraphicBufferAllocator> mga::AndroidPlatform::create_buffer_allocator(
        std::shared_ptr<mg::BufferInitializer> const& buffer_initializer)
{
    return std::make_shared<mga::AndroidBufferAllocator>(buffer_initializer);
}

std::shared_ptr<mg::Display> mga::AndroidPlatform::create_display()
{
    auto hwc_factory = std::make_shared<mga::AndroidHWCFactory>();
    auto display_allocator = std::make_shared<mga::AndroidDisplayAllocator>();
    auto display_factory = std::make_shared<mga::AndroidDisplayFactory>(display_allocator, hwc_factory);
    return display_factory->create_display();
}

std::shared_ptr<mg::PlatformIPCPackage> mga::AndroidPlatform::get_ipc_package()
{
    return std::make_shared<mg::PlatformIPCPackage>();
}

EGLNativeDisplayType mga::AndroidPlatform::shell_egl_display()
{
    // TODO: Implement
    return static_cast<EGLNativeDisplayType>(0);
}

std::shared_ptr<mg::Platform> mg::create_platform(std::shared_ptr<DisplayReport> const& /*TODO*/)
{
    return std::make_shared<mga::AndroidPlatform>();
}
