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
 *   Thomas Guest  <thomas.guest@canonical.com>
 */

#ifndef MIR_GRAPHICS_PLATFORM_H_
#define MIR_GRAPHICS_PLATFORM_H_

#include <EGL/egl.h>

#include <memory>

namespace mir
{
namespace compositor
{
class GraphicBufferAllocator;
}

/// Graphics subsystem. Mediates interaction between core system and
/// the graphics environment.
namespace graphics
{

class Display;
struct PlatformIPCPackage;
class BufferInitializer;

class DisplayReport;

/// Interface to platform specific support for graphics operations.
class Platform
{
public:
    Platform() = default;
    Platform(const Platform& p) = delete;
    Platform& operator=(const Platform& p) = delete;

    virtual std::shared_ptr<compositor::GraphicBufferAllocator> create_buffer_allocator(
        std::shared_ptr<BufferInitializer> const& buffer_initializer) = 0;
    virtual std::shared_ptr<Display> create_display() = 0;
    virtual std::shared_ptr<PlatformIPCPackage> get_ipc_package() = 0;
    
    virtual EGLNativeDisplayType shell_egl_display() = 0;
};

// Create and return a new graphics platform.
std::shared_ptr<Platform> create_platform(std::shared_ptr<DisplayReport> const& report);

}
}

#endif // MIR_GRAPHICS_PLATFORM_H_
