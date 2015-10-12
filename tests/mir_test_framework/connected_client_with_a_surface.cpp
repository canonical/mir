/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir_test_framework/connected_client_with_a_surface.h"

namespace mtf = mir_test_framework;

void mtf::ConnectedClientWithASurface::SetUp()
{
    ConnectedClientHeadlessServer::SetUp();

    auto const spec = mir_connection_create_spec_for_normal_surface(
        connection, surface_size.width.as_int(), surface_size.height.as_int(), mir_pixel_format_abgr_8888);
    mir_surface_spec_set_name(spec, "ConnectedClientWithASurfaceFixtureSurface");
    mir_surface_spec_set_buffer_usage(spec, mir_buffer_usage_hardware);

    surface = mir_surface_create_sync(spec);
    mir_surface_spec_release(spec);
    ASSERT_TRUE(mir_surface_is_valid(surface));
}

void mtf::ConnectedClientWithASurface::TearDown()
{
    mir_surface_release_sync(surface);
    ConnectedClientHeadlessServer::TearDown();
}
