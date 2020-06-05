/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

#include "mir_test_framework/connected_client_with_a_window.h"

namespace mtf = mir_test_framework;

void mtf::ConnectedClientWithAWindow::SetUp()
{
    ConnectedClientHeadlessServer::SetUp();

    auto const spec = mir_create_normal_window_spec(
        connection,
        surface_size.width.as_int(), surface_size.height.as_int());
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    mir_window_spec_set_pixel_format(spec, mir_pixel_format_abgr_8888);
    mir_window_spec_set_name(spec, "ConnectedClientWithASurfaceFixtureSurface");
    mir_window_spec_set_buffer_usage(spec, mir_buffer_usage_software);
#pragma GCC diagnostic pop

    window = mir_create_window_sync(spec);
    mir_window_spec_release(spec);
    ASSERT_TRUE(mir_window_is_valid(window));
}

void mtf::ConnectedClientWithAWindow::TearDown()
{
    mir_window_release_sync(window);
    ConnectedClientHeadlessServer::TearDown();
}
