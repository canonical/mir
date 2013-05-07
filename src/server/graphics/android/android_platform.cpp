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
#include "android_graphic_buffer_allocator.h"
#include "android_hwc_factory.h"
#include "android_display_allocator.h"
#include "android_display_factory.h"
#include "default_framebuffer_factory.h"
#include "internal_client.h"
#include "mir/graphics/platform_ipc_package.h"
#include "mir/graphics/buffer_initializer.h"
#include "mir/compositor/buffer_id.h"

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mc=mir::compositor;
namespace mf=mir::frontend;

mga::AndroidPlatform::AndroidPlatform(std::shared_ptr<mg::DisplayReport> const& display_report)
    : display_report(display_report)
{
}

std::shared_ptr<mc::GraphicBufferAllocator> mga::AndroidPlatform::create_buffer_allocator(
        std::shared_ptr<mg::BufferInitializer> const& buffer_initializer)
{
    return std::make_shared<mga::AndroidGraphicBufferAllocator>(buffer_initializer);
}

std::shared_ptr<mg::Display> mga::AndroidPlatform::create_display()
{
    auto hwc_factory = std::make_shared<mga::AndroidHWCFactory>();
    auto display_allocator = std::make_shared<mga::AndroidDisplayAllocator>();

    auto buffer_initializer = std::make_shared<mg::NullBufferInitializer>();
    auto buffer_allocator = std::make_shared<mga::AndroidGraphicBufferAllocator>(buffer_initializer);
    auto fb_factory = std::make_shared<mga::DefaultFramebufferFactory>(buffer_allocator);
    auto display_factory = std::make_shared<mga::AndroidDisplayFactory>(display_allocator, hwc_factory, fb_factory, display_report);
    return display_factory->create_display();
}

std::shared_ptr<mg::PlatformIPCPackage> mga::AndroidPlatform::get_ipc_package()
{
    return std::make_shared<mg::PlatformIPCPackage>();
}

std::shared_ptr<mg::InternalClient> mga::AndroidPlatform::create_internal_client(
    std::shared_ptr<mf::Surface> const& surface)
{
    return std::make_shared<mga::InternalClient>(surface);
}

std::shared_ptr<mg::Platform> mg::create_platform(std::shared_ptr<DisplayReport> const& display_report)
{
    return std::make_shared<mga::AndroidPlatform>(display_report);
}
