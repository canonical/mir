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

#include <random>
#include <fstream>

namespace mt = mir::test;
namespace mtf = mir_test_framework;
namespace geom = mir::geometry;

namespace
{

bool socket_exists(std::string const& socket_name)
{
    std::string socket_path{socket_name};
    socket_path.insert(std::begin(socket_path), ' ');

    std::ifstream socket_names_file("/proc/net/unix");
    std::string line;
    while (std::getline(socket_names_file, line))
    {
       if (line.find(socket_path) != std::string::npos)
           return true;
    }
    return false;
}

std::string create_random_socket_name()
{
    std::random_device random_device;
    std::mt19937 generator(random_device());
    int max_concurrent_test_instances = 99999;
    std::uniform_int_distribution<> dist(1, max_concurrent_test_instances);
    std::string const suffix{"-mir_socket_test"};

    /* check for name collisions against other test instances
     * running concurrently
     */
    for (int i = 0; i < max_concurrent_test_instances; i++)
    {
        std::string name{std::to_string(dist(generator))};
        name.append(suffix);
        if (!socket_exists(name))
            return name;
    }
    throw std::runtime_error("Too many test socket instances exist!");
}
}

mtf::TestingServerConfiguration::TestingServerConfiguration() :
    using_server_started_sync(false)
{
}

mtf::TestingServerConfiguration::TestingServerConfiguration(std::vector<geom::Rectangle> const& display_rects) :
    StubbedServerConfiguration(display_rects),
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
        TestingServerStatusListener(mt::CrossProcessSync const& sync,
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

        mt::CrossProcessSync server_started_sync;
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
    static const std::string socket_file{create_random_socket_name()};
    return socket_file;
}
