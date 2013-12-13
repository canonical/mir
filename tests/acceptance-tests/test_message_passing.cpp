/*
 * Copyright © 2013 Canonical Ltd.
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

#include "mir_toolkit/mir_client_library.h"

#include "mir_test_framework/in_process_server.h"
#include "mir_test_framework/testing_server_configuration.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace
{
// TODO we're only using part of TestingServerConfiguration - so that ought to be factored out
struct TestMessagePassingServerConfiguration : mir_test_framework::TestingServerConfiguration
{
private:
    using mir_test_framework::TestingServerConfiguration::exec;
    using mir_test_framework::TestingServerConfiguration::on_exit;
};

struct MessagePassing : mir_test_framework::InProcessServer
{
    virtual mir::DefaultServerConfiguration& server_config() { return server_config_; }

    TestMessagePassingServerConfiguration server_config_;
};
}

TEST_F(MessagePassing, create_and_realize_surface)
{
    auto const connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    ASSERT_TRUE(mir_connection_is_valid(connection));

//    MirSurfaceParameters params{
//        __PRETTY_FUNCTION__,
//        640, 480, mir_pixel_format_abgr_8888,
//        mir_buffer_usage_hardware,
//        mir_display_output_id_invalid};

    mir_connection_release(connection);
}
