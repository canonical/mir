/*
 * Copyright © 2018-2019 Canonical Ltd.
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
WlcsExtensionDescriptor const extensions[] = {
    {"wl_compositor",               4},
    {"wl_shm",                      1},
    {"wl_data_device_manager",      3},
    {"wl_shell",                    1},
    {"wl_seat",                     6},
    {"wl_output",                   3},
    {"wl_subcompositor",            1},
    {"xdg_wm_base",                 1},
    {"zxdg_shell_unstable_v6",      1},
    {"wlr_layer_shell_unstable_v1", 1}
};

WlcsIntegrationDescriptor const descriptor{
    1,
    sizeof(extensions) / sizeof(extensions[0]),
    extensions
};

WlcsIntegrationDescriptor const* get_descriptor(WlcsDisplayServer const* /*server*/)
{
    return &descriptor;
}

WlcsDisplayServer* wlcs_create_server(int argc, char const** argv)
{
    auto server = new miral::TestWlcsDisplayServer(argc, argv);

    server->get_descriptor = &get_descriptor;
    return server;
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
