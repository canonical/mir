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

#include "mir/graphics/egl/mesa_native_display.h"
#include "mir/graphics/platform_ipc_package.h"

#include "mir/display_server.h"
#include "mir/graphics/platform.h"

#include "mir_toolkit/mesa/native_display.h"
#include "mir_toolkit/c_types.h"

namespace mg = mir::graphics;
namespace mgeglm = mg::egl::mesa;

namespace
{
class MesaNativeDisplayImpl
{
public:
    MesaNativeDisplayImpl(mir::DisplayServer *server)
        : graphics_platform(server->graphics_platform())
    {
    }
    
    void populate_platform_package(mir_toolkit::MirPlatformPackage *package)
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
private:
    std::shared_ptr<mg::Platform> graphics_platform;
    std::shared_ptr<mg::PlatformIPCPackage> platform_package;
};

static void native_display_get_platform(mir_toolkit::MirMesaEGLNativeDisplay* display, mir_toolkit::MirPlatformPackage* package)
{
    auto impl = (MesaNativeDisplayImpl*)display->context; // TODO: Fix cast ~racarr
    impl->populate_platform_package(package);
}

struct NativeDisplayDeleter
{
    void operator()(mir_toolkit::MirMesaEGLNativeDisplay *display)
    {
        auto impl = (MesaNativeDisplayImpl*)display->context; // TODO: Fix cast ~racarr
        delete impl;
        delete display;
    }
};

}

std::shared_ptr<mir_toolkit::MirMesaEGLNativeDisplay> mgeglm::create_native_display(mir::DisplayServer* server)
{
    auto impl = new MesaNativeDisplayImpl(server);

    auto native_display = std::shared_ptr<mir_toolkit::MirMesaEGLNativeDisplay>(new mir_toolkit::MirMesaEGLNativeDisplay, 
                                                                                NativeDisplayDeleter());
    native_display->context = (mir_toolkit::MirConnection *)impl; // TODO: Fix cast ~racarr
    native_display->display_get_platform = native_display_get_platform;
    
    return native_display;
}
