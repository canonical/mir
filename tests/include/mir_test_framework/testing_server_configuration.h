/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */


#ifndef MIR_TEST_TESTING_SERVER_CONFIGURATION_H_
#define MIR_TEST_TESTING_SERVER_CONFIGURATION_H_

#include "mir_test_framework/stubbed_server_configuration.h"
#include "mir/test/cross_process_sync.h"

namespace mir_test_framework
{
using namespace mir;

class TestingServerConfiguration : public StubbedServerConfiguration
{
public:
    TestingServerConfiguration();
    explicit TestingServerConfiguration(std::vector<geometry::Rectangle> const& display_rects);

    // Code to run in server process
    virtual void exec();

    // Code to run in server process after server starts
    virtual void on_start();

    // Code to run in server process after server exits
    virtual void on_exit();

    std::shared_ptr<mir::ServerStatusListener> the_server_status_listener() override;

    virtual std::string the_socket_file() const override;
    using DefaultServerConfiguration::the_options;

    virtual void wait_for_server_start();

private:
    mir::test::CrossProcessSync server_started_sync;
    bool using_server_started_sync;
};

std::string const& test_socket_file();
}

#endif /* MIR_TEST_TESTING_SERVER_CONFIGURATION_H_ */
