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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir_toolkit/mir_screencast.h"
#include "mir_toolkit/mir_client_library.h"

#include "mir_test_framework/stubbed_server_configuration.h"
#include "mir_test_framework/in_process_server.h"
#include "mir_test_framework/using_stub_client_platform.h"

#include "mir_test_doubles/null_display_changer.h"
#include "mir_test_doubles/stub_display_configuration.h"
#include "mir_test_doubles/stub_buffer.h"
#include "mir_test_doubles/mock_screencast.h"
#include "mir_test/fake_shared.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mtf = mir_test_framework;
namespace mtd = mir::test::doubles;
namespace mg = mir::graphics;
namespace mf = mir::frontend;
namespace mt = mir::test;

namespace
{

class StubChanger : public mtd::NullDisplayChanger
{
public:
    StubChanger()
        : stub_display_config{{{connected, !used}, {connected, used}}}
    {
    }

    std::shared_ptr<mg::DisplayConfiguration> active_configuration() override
    {
        return mt::fake_shared(stub_display_config);
    }

    mtd::StubDisplayConfig stub_display_config;

private:
    static bool const connected;
    static bool const used;
};

bool const StubChanger::connected{true};
bool const StubChanger::used{true};

struct StubServerConfig : mir_test_framework::StubbedServerConfiguration
{
    std::shared_ptr<mf::DisplayChanger> the_frontend_display_changer() override
    {
        return mt::fake_shared(stub_changer);
    }

    std::shared_ptr<mf::Screencast> the_screencast() override
    {
        return mt::fake_shared(mock_screencast);
    }

    StubChanger stub_changer;
    mtd::MockScreencast mock_screencast;
};

class MirScreencastTest : public mir_test_framework::InProcessServer
{
public:
    mir::DefaultServerConfiguration& server_config() override { return server_config_; }

    mtd::MockScreencast& mock_screencast() { return server_config_.mock_screencast; }

    StubServerConfig server_config_;
    mtf::UsingStubClientPlatform using_stub_client_platform;
};

}

TEST_F(MirScreencastTest, creation_with_invalid_connection_fails)
{
    using namespace testing;

    uint32_t const output_id{2};
    MirScreencastParameters params{output_id, 0, 0, mir_pixel_format_invalid};

    auto screencast = mir_connection_create_screencast_sync(nullptr, &params);
    ASSERT_EQ(nullptr, screencast);
}

TEST_F(MirScreencastTest, creation_with_invalid_output_fails)
{
    using namespace testing;

    auto const connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    ASSERT_TRUE(mir_connection_is_valid(connection));

    uint32_t const invalid_output_id{33};
    MirScreencastParameters params{invalid_output_id, 0, 0, mir_pixel_format_invalid};

    EXPECT_CALL(mock_screencast(), create_session(_))
        .Times(0);

    auto screencast =
        mir_connection_create_screencast_sync(connection, &params);
    ASSERT_EQ(nullptr, screencast);

    mir_connection_release(connection);
}

TEST_F(MirScreencastTest, contacts_server_screencast_for_create_and_release)
{
    using namespace testing;

    auto const connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    ASSERT_TRUE(mir_connection_is_valid(connection));

    mf::ScreencastSessionId const screencast_session_id{99};
    uint32_t const output_id{2};
    MirScreencastParameters params{output_id, 0, 0, mir_pixel_format_invalid};

    InSequence seq;

    EXPECT_CALL(mock_screencast(),
                create_session(mg::DisplayConfigurationOutputId{output_id}))
        .WillOnce(Return(screencast_session_id));

    EXPECT_CALL(mock_screencast(), capture(screencast_session_id))
        .WillOnce(Return(std::make_shared<mtd::StubBuffer>()));

    EXPECT_CALL(mock_screencast(), destroy_session(screencast_session_id));

    auto screencast = mir_connection_create_screencast_sync(connection, &params);
    ASSERT_NE(nullptr, screencast);
    mir_screencast_release_sync(screencast);

    mir_connection_release(connection);
}

TEST_F(MirScreencastTest, gets_valid_egl_native_window)
{
    using namespace testing;

    auto const connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    ASSERT_TRUE(mir_connection_is_valid(connection));

    mf::ScreencastSessionId const screencast_session_id{99};
    uint32_t const output_id{2};
    MirScreencastParameters params{output_id, 0, 0, mir_pixel_format_invalid};

    InSequence seq;
    EXPECT_CALL(mock_screencast(), create_session(_))
        .WillOnce(Return(screencast_session_id));
    EXPECT_CALL(mock_screencast(), capture(_))
        .WillOnce(Return(std::make_shared<mtd::StubBuffer>()));
    EXPECT_CALL(mock_screencast(), destroy_session(_));

    auto screencast = mir_connection_create_screencast_sync(connection, &params);
    ASSERT_NE(nullptr, screencast);

    auto egl_native_window = mir_screencast_egl_native_window(screencast);
    EXPECT_NE(MirEGLNativeWindowType(), egl_native_window);

    mir_screencast_release_sync(screencast);

    mir_connection_release(connection);
}

TEST_F(MirScreencastTest, fails_on_client_when_server_request_fails)
{
    using namespace testing;

    auto const connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    ASSERT_TRUE(mir_connection_is_valid(connection));

    uint32_t const output_id{2};
    MirScreencastParameters params{output_id, 0, 0, mir_pixel_format_invalid};

    EXPECT_CALL(mock_screencast(), create_session(_))
        .WillOnce(Throw(std::runtime_error("")));

    auto screencast = mir_connection_create_screencast_sync(connection, &params);
    ASSERT_EQ(nullptr, screencast);

    mir_connection_release(connection);
}
