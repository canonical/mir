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

#include "internal_native_display.h"
#include "mir/display_server.h"
#include "mir/graphics/platform_ipc_package.h"
#include "mir/graphics/platform.h"
#include "mir/frontend/surface.h"
#include "mir/compositor/buffer.h"
#include "mir/compositor/buffer_ipc_package.h"

#include "mir_toolkit/mesa/native_display.h"

#include <mutex>
#include <set>

namespace mg = mir::graphics;
namespace mgg = mir::graphics::gbm;
namespace mf = mir::frontend;

mgg::InternalNativeDisplay::InternalNativeDisplay(std::shared_ptr<mg::PlatformIPCPackage> const& platform_package)
    : platform_package(platform_package)
{
    context = this;
    this->display_get_platform = &InternalNativeDisplay::native_display_get_platform;
    this->surface_get_current_buffer = &InternalNativeDisplay::native_display_surface_get_current_buffer;
    this->surface_get_parameters = &InternalNativeDisplay::native_display_surface_get_parameters;
    this->surface_advance_buffer = &InternalNativeDisplay::native_display_surface_advance_buffer;
}

void mgg::InternalNativeDisplay::native_display_get_platform(MirMesaEGLNativeDisplay* display, MirPlatformPackage* package)
{
    auto native_disp = static_cast<InternalNativeDisplay*>(display);
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

void mgg::InternalNativeDisplay::native_display_surface_get_current_buffer(MirMesaEGLNativeDisplay*, 
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

void mgg::InternalNativeDisplay::native_display_surface_get_parameters(MirMesaEGLNativeDisplay*, 
                                                  MirEGLNativeWindowType surface,
                                                  MirSurfaceParameters* parameters)
{
    auto mir_surface = static_cast<mf::Surface*>(surface);

    parameters->width = mir_surface->size().width.as_uint32_t();
    parameters->height = mir_surface->size().height.as_uint32_t();
    parameters->pixel_format = static_cast<MirPixelFormat>(mir_surface->pixel_format());
    parameters->buffer_usage = mir_buffer_usage_hardware;
}

void mgg::InternalNativeDisplay::native_display_surface_advance_buffer(MirMesaEGLNativeDisplay*, 
                                                  MirEGLNativeWindowType surface)
{
    auto mir_surface = static_cast<mf::Surface*>(surface);
    mir_surface->advance_client_buffer();
}
