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

#ifndef MIR_CLIENT_MESA_MESA_NATIVE_DISPLAY_CONTAINER_H_
#define MIR_CLIENT_MESA_MESA_NATIVE_DISPLAY_CONTAINER_H_

#include "../egl_native_display_container.h"

#include "mir_toolkit/client_types.h"
#include "mir_toolkit/mesa/native_display.h"

#include <unordered_set>
#include <mutex>

namespace mir
{
namespace client
{
namespace mesa
{

class MesaNativeDisplayContainer : public EGLNativeDisplayContainer
{
public:
    MesaNativeDisplayContainer();
    virtual ~MesaNativeDisplayContainer();

    MirEGLNativeDisplayType create(MirConnection* connection);
    void release(MirEGLNativeDisplayType display);

    bool validate(MirEGLNativeDisplayType display) const;

protected:
    MesaNativeDisplayContainer(MesaNativeDisplayContainer const&) = delete;
    MesaNativeDisplayContainer& operator=(MesaNativeDisplayContainer const&) = delete;

private:
    std::mutex mutable guard;
    std::unordered_set<MirEGLNativeDisplayType> valid_displays;
};

extern "C" int mir_client_mesa_egl_native_display_is_valid(MirMesaEGLNativeDisplay* display);

}
}
} // namespace mir

#endif // MIR_CLIENT_MESA_MESA_NATIVE_DISPLAY_CONTAINER_H_
