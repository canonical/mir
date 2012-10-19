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
 *   Thomas Guest  <thomas.guest@canonical.com>
 */

#ifndef MIR_GRAPHICS_PLATFORM_H_
#define MIR_GRAPHICS_PLATFORM_H_

#include <memory>

// Interface to platform specific support for graphics operations.

namespace mir
{
namespace compositor
{
class GraphicBufferAllocator;
}
namespace logging
{
class Logger;
}

namespace graphics
{

class Display;
class PlatformIPCPackage;
class BufferInitializer;

class DisplayListener
{
  public:

    virtual void report_successful_setup_of_native_resources() = 0;
    virtual void report_successful_egl_make_current_on_construction() = 0;
    virtual void report_successful_egl_buffer_swap_on_construction() = 0;
    virtual void report_successful_drm_mode_set_crtc_on_construction() = 0;
    virtual void report_successful_display_construction() = 0;

  protected:
    DisplayListener() = default;
    ~DisplayListener() = default;
    DisplayListener(const DisplayListener&) = delete;
    DisplayListener& operator=(const DisplayListener&) = delete;
};

// TODO not the best place, but convenient for refactoring
class NullDisplayListener : public DisplayListener
{
  public:

    virtual void report_successful_setup_of_native_resources() {}
    virtual void report_successful_egl_make_current_on_construction() {}
    virtual void report_successful_egl_buffer_swap_on_construction() {}
    virtual void report_successful_drm_mode_set_crtc_on_construction() {}
    virtual void report_successful_display_construction() {}
};

class Platform
{
public:
    Platform() = default;
    Platform(const Platform& p) = delete;
    Platform& operator=(const Platform& p) = delete;

    virtual std::shared_ptr<compositor::GraphicBufferAllocator> create_buffer_allocator(
            const std::shared_ptr<BufferInitializer>& buffer_initializer) = 0;
    virtual std::shared_ptr<Display> create_display() = 0;
    virtual std::shared_ptr<PlatformIPCPackage> get_ipc_package() = 0;
};

// Create and return a new graphics platform.
std::shared_ptr<Platform> create_platform();

}
}

#endif // MIR_GRAPHICS_PLATFORM_H_
