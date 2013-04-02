/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
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
#include "mir_toolkit/c_types.h"

#include <mutex>
#include <set>

namespace mg = mir::graphics;
namespace mgeglm = mg::egl::mesa;
namespace mf = mir::frontend;

std::mutex valid_displays_guard;
std::set<MirMesaEGLNativeDisplay*> valid_displays;

namespace
{
class MesaNativeDisplayImpl
{
public:
    MesaNativeDisplayImpl(std::shared_ptr<mg::Platform> const& graphics_platform)
        : graphics_platform(graphics_platform)
    {
    }
    
    void populate_platform_package(MirPlatformPackage* package)
    {
        if (!platform_package)
            platform_package = graphics_platform->get_ipc_package();
        package->data_items = platform_package->ipc_data.size();
        for (int i = 0; i < package->data_items; i++)
        {
            package->data[i] = platform_package->ipc_data[i];
        }
        package->fd_items = platform_package->ipc_fds.size();
        for (int i = 0; i < package->fd_items; i++)
        {
            package->fd[i] = platform_package->ipc_fds[i];
        }
    }
    
    void surface_get_current_buffer(mf::Surface* surface, MirBufferPackage* package)
    {
        auto buffer = surface->client_buffer();
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
    
    void surface_advance_buffer(mf::Surface* surface)
    {
        surface->advance_client_buffer();
    }
    
    void surface_get_parameters(mf::Surface* surface, MirSurfaceParameters* parameters)
    {
        parameters->width = surface->size().width.as_uint32_t();
        parameters->height = surface->size().height.as_uint32_t();
        parameters->pixel_format = static_cast<MirPixelFormat>(surface->pixel_format());
        parameters->buffer_usage = mir_buffer_usage_hardware;
    }
    
    static MesaNativeDisplayImpl* impl(MirMesaEGLNativeDisplay* display)
    {
        return static_cast<MesaNativeDisplayImpl*>(display->context);
    }

private:
    std::shared_ptr<mg::Platform> graphics_platform;
    std::shared_ptr<mg::PlatformIPCPackage> platform_package;
};

static void native_display_get_platform(MirMesaEGLNativeDisplay* display, MirPlatformPackage* package)
{
    auto impl = MesaNativeDisplayImpl::impl(display);
    impl->populate_platform_package(package);
}

static void native_display_surface_get_current_buffer(MirMesaEGLNativeDisplay* display, 
                                                      MirEGLNativeWindowType surface,
                                                      MirBufferPackage* buffer_package)
{
    auto impl = MesaNativeDisplayImpl::impl(display);
    auto mir_surface = static_cast<mf::Surface*>(surface);
    impl->surface_get_current_buffer(mir_surface, buffer_package);
}

static void native_display_surface_get_parameters(MirMesaEGLNativeDisplay* display, 
                                                  MirEGLNativeWindowType surface,
                                                  MirSurfaceParameters* parameters)
{
    auto impl = MesaNativeDisplayImpl::impl(display);
    auto mir_surface = static_cast<mf::Surface*>(surface);
    impl->surface_get_parameters(mir_surface, parameters);
}

static void native_display_surface_advance_buffer(MirMesaEGLNativeDisplay* display, 
                                                  MirEGLNativeWindowType surface)
{
    auto impl = MesaNativeDisplayImpl::impl(display);
    auto mir_surface = static_cast<mf::Surface*>(surface);
    impl->surface_advance_buffer(mir_surface);
}

struct NativeDisplayDeleter
{
    void operator()(MirMesaEGLNativeDisplay* display)
    {
        std::unique_lock<std::mutex> lg(valid_displays_guard);
        valid_displays.erase(display);
        auto impl = static_cast<MesaNativeDisplayImpl*>(display->context);
        delete impl;
        delete display;
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
    auto impl = new MesaNativeDisplayImpl(platform);

    auto native_display = std::shared_ptr<MirMesaEGLNativeDisplay>(new MirMesaEGLNativeDisplay, 
                                                                   NativeDisplayDeleter());
    native_display->context = static_cast<void*>(impl);
    native_display->display_get_platform = native_display_get_platform;
    native_display->surface_get_current_buffer = native_display_surface_get_current_buffer;
    native_display->surface_get_parameters = native_display_surface_get_parameters;
    native_display->surface_advance_buffer = native_display_surface_advance_buffer;
    
    std::unique_lock<std::mutex> lg(valid_displays_guard);
    valid_displays.insert(native_display.get());
    
    return native_display;
}
