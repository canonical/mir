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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir_test_framework/testing_server_configuration.h"

#include "mir/server_status_listener.h"

#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>

namespace mtf = mir_test_framework;


mtf::TestingServerConfiguration::TestingServerConfiguration() :
    using_server_started_sync(false)
{
}

void mtf::TestingServerConfiguration::exec()
{
}

void mtf::TestingServerConfiguration::on_start()
{
}

void mtf::TestingServerConfiguration::on_exit()
{
}

std::string mtf::TestingServerConfiguration::the_socket_file() const
{
    return test_socket_file();
}

std::shared_ptr<mir::ServerStatusListener>
mtf::TestingServerConfiguration::the_server_status_listener()
{
    struct TestingServerStatusListener : public mir::ServerStatusListener
    {
        TestingServerStatusListener(CrossProcessSync const& sync,
                                    std::function<void(void)> const& on_start)
            : server_started_sync{sync},
              on_start{on_start}
        {
        }

        void paused() {}
        void resumed() {}
        void started()
        {
            server_started_sync.try_signal_ready_for();
            on_start();
        }

        CrossProcessSync server_started_sync;
        std::function<void(void)> const on_start;
    };

    return server_status_listener(
        [this]
        {
            using_server_started_sync = true;
            return std::make_shared<TestingServerStatusListener>(
                server_started_sync,
                [this] { on_start(); });
        });
}

void mtf::TestingServerConfiguration::wait_for_server_start()
{
    auto listener = the_server_status_listener();

    if (!using_server_started_sync)
    {
        BOOST_THROW_EXCEPTION(
            std::runtime_error(
                "Not using cross process sync mechanism for server startup detection."
                "Did you override the_server_status_listener() in the test?"));
    }

    server_started_sync.wait_for_signal_ready_for();
}

std::string const& mtf::test_socket_file()
{
    static const std::string socket_file{"./mir_socket_test"};
    return socket_file;
}
