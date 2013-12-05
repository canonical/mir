/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "mesa_native_display_container.h"

#include "mir_toolkit/mir_client_library.h"

#include <cstring>
#include <unordered_set>
#include <mutex>

namespace mcl = mir::client;
namespace mclg = mcl::gbm;

namespace
{
extern "C"
{

static int egl_display_get_platform(MirMesaEGLNativeDisplay* display,
                                    MirPlatformPackage* package)
{
    auto connection = static_cast<MirConnection*>(display->context);
    mir_connection_get_platform(connection, package);
    return MIR_MESA_TRUE;
}

int mir_client_mesa_egl_native_display_is_valid(MirMesaEGLNativeDisplay* display)
{
    return mcl::EGLNativeDisplayContainer::instance().validate(display);
}

}
}

mcl::EGLNativeDisplayContainer& mcl::EGLNativeDisplayContainer::instance()
{
    static mclg::MesaNativeDisplayContainer default_display_container;
    return default_display_container;
}

mclg::MesaNativeDisplayContainer::MesaNativeDisplayContainer()
{
}

mclg::MesaNativeDisplayContainer::~MesaNativeDisplayContainer()
{
    std::lock_guard<std::mutex> lg(guard);

    for (auto display : valid_displays)
    {
        delete static_cast<MirMesaEGLNativeDisplay*>(display);
    }
}

bool mclg::MesaNativeDisplayContainer::validate(MirEGLNativeDisplayType display) const
{
    std::lock_guard<std::mutex> lg(guard);
    return (valid_displays.find(display) != valid_displays.end());
}

MirEGLNativeDisplayType
mclg::MesaNativeDisplayContainer::create(MirConnection* connection)
{
    MirMesaEGLNativeDisplay* display = new MirMesaEGLNativeDisplay();
    display->display_get_platform = egl_display_get_platform;
    display->context = connection;

    std::lock_guard<std::mutex> lg(guard);
    auto egl_display = static_cast<MirEGLNativeDisplayType>(display);
    valid_displays.insert(egl_display);

    return egl_display;
}

void mclg::MesaNativeDisplayContainer::release(MirEGLNativeDisplayType display)
{
    std::lock_guard<std::mutex> lg(guard);

    auto it = valid_displays.find(display);
    if (it == valid_displays.end())
        return;

    delete static_cast<MirMesaEGLNativeDisplay*>(*it);
    valid_displays.erase(it);
}
