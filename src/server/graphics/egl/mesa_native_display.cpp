/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "mir/display_server.h"
#include "mir/graphics/egl/mesa_native_display.h"
#include "mir/graphics/platform_ipc_package.h"
#include "mir/graphics/platform.h"
#include "mir/frontend/surface.h"
#include "mir/compositor/buffer.h"
#include "mir/compositor/buffer_ipc_package.h"

#include "mir_toolkit/mesa/native_display.h"

#include <mutex>
#include <set>

namespace mg = mir::graphics;
namespace mgeglm = mg::egl::mesa;
namespace mf = mir::frontend;

std::mutex valid_displays_guard;
std::set<MirMesaEGLNativeDisplay*> valid_displays;

namespace
{
struct ShellMesaEGLNativeDisplay : MirMesaEGLNativeDisplay
{
public:
    ShellMesaEGLNativeDisplay(std::shared_ptr<mg::Platform> const& graphics_platform)
        : graphics_platform(graphics_platform)
    {
        context = this;
        this->display_get_platform = &ShellMesaEGLNativeDisplay::native_display_get_platform;
        this->surface_get_current_buffer = &ShellMesaEGLNativeDisplay::native_display_surface_get_current_buffer;
        this->surface_get_parameters = &ShellMesaEGLNativeDisplay::native_display_surface_get_parameters;
        this->surface_advance_buffer = &ShellMesaEGLNativeDisplay::native_display_surface_advance_buffer;
    }

    static void native_display_get_platform(MirMesaEGLNativeDisplay* display, MirPlatformPackage* package)
    {
        auto native_disp = static_cast<ShellMesaEGLNativeDisplay*>(display);
        if (!native_disp->platform_package)
            native_disp->platform_package = native_disp->graphics_platform->get_ipc_package();
        package->data_items = native_disp->platform_package->ipc_data.size();
        for (int i = 0; i < package->data_items; i++)
        {
            package->data[i] = native_disp->platform_package->ipc_data[i];
        }
        package->fd_items = native_disp->platform_package->ipc_fds.size();
        for (int i = 0; i < package->fd_items; i++)
        {
            package->fd[i] = native_disp->platform_package->ipc_fds[i];
        }

    }
    
    static void native_display_surface_get_current_buffer(MirMesaEGLNativeDisplay* /* display */, 
                                                          MirEGLNativeWindowType surface,
                                                          MirBufferPackage* package)
    {
        auto mir_surface = static_cast<mf::Surface*>(surface);

        auto buffer = mir_surface->client_buffer();
        auto buffer_package = buffer->get_ipc_package();
        package->data_items = buffer_package->ipc_data.size();
        for (int i = 0; i < package->data_items; i++)
        {
            package->data[i] = buffer_package->ipc_data[i];
        }
        package->fd_items = buffer_package->ipc_fds.size();
        for (int i = 0; i < package->fd_items; i++)
        {
            package->fd[i] = buffer_package->ipc_fds[i];
        }
        package->stride = buffer_package->stride;
    }

    static void native_display_surface_get_parameters(MirMesaEGLNativeDisplay* /* display  */, 
                                                      MirEGLNativeWindowType surface,
                                                      MirSurfaceParameters* parameters)
    {
        auto mir_surface = static_cast<mf::Surface*>(surface);

        parameters->width = mir_surface->size().width.as_uint32_t();
        parameters->height = mir_surface->size().height.as_uint32_t();
        parameters->pixel_format = static_cast<MirPixelFormat>(mir_surface->pixel_format());
        parameters->buffer_usage = mir_buffer_usage_hardware;
    }

    static void native_display_surface_advance_buffer(MirMesaEGLNativeDisplay* /* display */, 
                                                      MirEGLNativeWindowType surface)
    {
        auto mir_surface = static_cast<mf::Surface*>(surface);
        mir_surface->advance_client_buffer();
    }

private:
    std::shared_ptr<mg::Platform> graphics_platform;
    std::shared_ptr<mg::PlatformIPCPackage> platform_package;
};

struct NativeDisplayDeleter
{
    void operator()(MirMesaEGLNativeDisplay* display)
    {
        std::unique_lock<std::mutex> lg(valid_displays_guard);
        valid_displays.erase(display);
        auto disp = static_cast<ShellMesaEGLNativeDisplay*>(display->context);
        delete disp;
    }
};

}

extern "C"
{
int mir_egl_mesa_display_is_valid(MirMesaEGLNativeDisplay* display)
{
    std::unique_lock<std::mutex> lg(valid_displays_guard);
    return valid_displays.find(display) != valid_displays.end();
}
}

std::shared_ptr<MirMesaEGLNativeDisplay> mgeglm::create_native_display(std::shared_ptr<mg::Platform> const& platform)
{
    auto native_display = std::shared_ptr<ShellMesaEGLNativeDisplay>(new ShellMesaEGLNativeDisplay(platform), 
                                                                     NativeDisplayDeleter());
    std::unique_lock<std::mutex> lg(valid_displays_guard);
    valid_displays.insert(native_display.get());
    
    return native_display;
}
