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

#include "mir_toolkit/mir_surface.h"
#include "mir_toolkit/mir_presentation_chain.h"
#include "host_chain.h"
#include "host_stream.h"
#include "surface_spec.h"

namespace mgn = mir::graphics::nested;

mgn::SurfaceSpec::SurfaceSpec() :
    spec(mir_connection_create_spec_for_changes(nullptr))
{
}

mgn::SurfaceSpec::~SurfaceSpec()
{
    mir_surface_spec_release(spec);
}

void mgn::SurfaceSpec::add_chain(HostChain& chain, geometry::Displacement disp, geometry::Size size)
{
    mir_surface_spec_add_presentation_chain(spec, size.width.as_int(), size.height.as_int(),
        disp.dx.as_int(), disp.dy.as_int(), chain.handle());
}

void mgn::SurfaceSpec::add_stream(HostStream& stream, geometry::Displacement disp)
{
    mir_surface_spec_add_buffer_stream(spec, disp.dx.as_int(), disp.dy.as_int(), stream.handle());
}
