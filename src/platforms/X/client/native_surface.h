/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#ifndef MIR_CLIENT_X_NATIVE_SURFACES_H_
#define MIR_CLIENT_X_NATIVE_SURFACES_H_

#include "../../mesa/include/mir_toolkit/mesa/native_display.h"
#include "mir/egl_native_surface.h"

namespace mir
{
namespace client
{
namespace X
{

class NativeSurface : public MirMesaEGLNativeSurface
{
public:
    explicit NativeSurface(EGLNativeSurface&);

    int advance_buffer(MirBufferPackage* buffer_package);
    int get_parameters(MirSurfaceParameters* surface_parameters);
    int set_swapinterval(int interval);

private:
    bool starting;
    EGLNativeSurface& surface;
};

}
}
}

#endif /* MIR_CLIENT_X_NATIVE_SURFACES_H_ */
