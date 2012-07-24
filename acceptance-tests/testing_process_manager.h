/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Thomas Guest <thomas.guest@canonical.com>
 */

#ifndef MIR_TESTING_PROCESS_MANAGER
#define MIR_TESTING_PROCESS_MANAGER

#include "mir/process/process.h"

#include <memory>
#include <list>

namespace mir
{
class DisplayServer;
namespace compositor
{
class BufferAllocationStrategy;
}
namespace graphics
{
class Renderer;
}

class TestingProcessManager
{
public:
    TestingProcessManager();
    ~TestingProcessManager();

    void launch_server_process(
            std::shared_ptr<mir::graphics::Renderer> const& renderer,
            std::shared_ptr<mir::compositor::BufferAllocationStrategy> const& buffer_allocation_strategy,
            std::function<void()>&& functor);
    void launch_client_process(std::function<void()>&& functor);

    mir::DisplayServer* display_server() const;
    void tear_down_clients();
    void tear_down_server();
    void tear_down_all();

private:

    void os_signal_handler(int signal);

    std::unique_ptr<mir::DisplayServer> server;
    std::shared_ptr<mir::process::Process> server_process;
    std::list<std::shared_ptr<mir::process::Process>> clients;

    bool is_test_process;
};
}

#endif // MIR_TESTING_PROCESS_MANAGER
