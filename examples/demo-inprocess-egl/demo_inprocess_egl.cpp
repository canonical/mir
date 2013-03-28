/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "inprocess_egl_client.h"

#include "mir/run_mir.h"
#include "mir/default_server_configuration.h"

namespace me = mir::examples;

namespace
{

struct InprocessClientStarter
{
    void operator()(mir::DisplayServer& server)
    {
        auto client = new me::InprocessEGLClient(server);
        client->start();
    }
};

}


int main(int argc, char const* argv[])
{
    mir::DefaultServerConfiguration config(argc, argv);
    
    mir::run_mir(config, InprocessClientStarter());
}
