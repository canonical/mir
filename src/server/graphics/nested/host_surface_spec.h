/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_NESTED_HOST_SURFACE_SPEC_H_
#define MIR_GRAPHICS_NESTED_HOST_SURFACE_SPEC_H_

#include "mir_toolkit/client_types.h"
#include "mir/geometry/size.h"
#include "mir/geometry/displacement.h"

namespace mir
{
namespace graphics
{
namespace nested
{
class HostChain;
class HostStream;
class HostSurfaceSpec
{
public:
    virtual ~HostSurfaceSpec() = default;
    virtual void add_chain(HostChain&, geometry::Displacement disp, geometry::Size size) = 0;
    virtual void add_stream(HostStream&, geometry::Displacement disp, geometry::Size size) = 0;
    virtual MirWindowSpec* handle() = 0;
protected:
    HostSurfaceSpec() = default;
    HostSurfaceSpec(HostSurfaceSpec const&) = delete;
    HostSurfaceSpec& operator=(HostSurfaceSpec const&) = delete;
};
}
}
}
#endif // MIR_GRAPHICS_NESTED_HOST_SURFACE_SPEC_H_
