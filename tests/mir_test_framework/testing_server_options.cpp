/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mir_test_framework/testing_server_configuration.h"
#include "mir_test_framework/temporary_environment_value.h"
#include "mir/server_status_listener.h"

#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>

#include <random>
#include <fstream>

namespace mt = mir::test;
namespace mtf = mir_test_framework;
namespace geom = mir::geometry;

mtf::TestingServerConfiguration::TestingServerConfiguration() :
    using_server_started_sync(false)
{
}

mtf::TestingServerConfiguration::TestingServerConfiguration(std::vector<geom::Rectangle> const& display_rects) :
    StubbedServerConfiguration(display_rects),
    using_server_started_sync(false)
{
}

mtf::TestingServerConfiguration::TestingServerConfiguration(
    std::vector<geometry::Rectangle> const& display_rects,
    std::vector<std::unique_ptr<TemporaryEnvironmentValue>>&& setup_environment)
    : StubbedServerConfiguration(display_rects),
      using_server_started_sync(false),
      environment{std::move(setup_environment)}
{
}

mtf::TestingServerConfiguration::~TestingServerConfiguration() = default;

void mtf::TestingServerConfiguration::exec()
{
}

void mtf::TestingServerConfiguration::on_start()
{
}

void mtf::TestingServerConfiguration::on_exit()
{
}

std::shared_ptr<mir::ServerStatusListener>
mtf::TestingServerConfiguration::the_server_status_listener()
{

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
