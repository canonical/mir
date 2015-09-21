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

#ifndef MIR_EXAMPLE_TEST_CLIENT_H_
#define MIR_EXAMPLE_TEST_CLIENT_H_

#include <memory>
#include <future>

#include "mir/main_loop.h"

namespace mir
{
class Server;

namespace examples
{
struct ClientContext
{
    std::unique_ptr<mir::time::Alarm> client_kill_action;
    std::unique_ptr<mir::time::Alarm> server_stop_action;
    std::atomic<bool> test_failed;
};

void add_test_client_option_to(mir::Server& server, ClientContext& context);
}
}

#endif /* MIR_EXAMPLE_TEST_CLIENT_H_ */
