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

    MirSurfaceParameters request_params =
    {
        __PRETTY_FUNCTION__,
        640, 480,
        mir_pixel_format_abgr_8888,
        mir_buffer_usage_hardware,
        mir_display_output_id_invalid
    };

    surface = mir_connection_create_surface_sync(connection, &request_params);
    ASSERT_TRUE(mir_surface_is_valid(surface));
}

void mtf::ConnectedClientWithASurface::TearDown()
{
    mir_surface_release_sync(surface);
    ConnectedClientHeadlessServer::TearDown();
}
