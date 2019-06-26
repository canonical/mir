/*
 * Copyright © 2018 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 *              Alan Griffiths <alan@octopull.co.uk>
 */

#include <miral/test_wlcs_display_server.h>

namespace
{
WlcsDisplayServer* wlcs_create_server(int argc, char const** argv)
{
    return new miral::TestWlcsDisplayServer(argc, argv);
}

void wlcs_destroy_server(WlcsDisplayServer* server)
{
    delete static_cast<miral::TestWlcsDisplayServer*>(server);
}
}

extern WlcsServerIntegration const wlcs_server_integration {
    1,
    &wlcs_create_server,
    &wlcs_destroy_server,
};
