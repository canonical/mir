/*
 * Copyright © 2014 Canonical Ltd.
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
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir/shared_library.h"

#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/mir_screencast.h"

#include "mir/client/client_platform_factory.h"
#include "mir/client/client_platform.h"
#include "mir/client/client_buffer_factory.h"

#include "mir/test/validity_matchers.h"
#include "mir/test/death.h"

#include "mir_test_framework/headless_in_process_server.h"
#include "mir_test_framework/stub_client_platform_options.h"
#include "mir_test_framework/any_surface.h"
#include "mir_test_framework/temporary_environment_value.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <stdexcept>
#include <boost/throw_exception.hpp>
#include <cstring>

namespace mcl = mir::client;
namespace mtf = mir_test_framework;

using ClientLibraryErrors = mtf::HeadlessInProcessServer;

TEST_F(ClientLibraryErrors, exception_in_platform_construction_generates_error)
{
    std::string const exception_text{"Ducks!"};
    mtf::add_client_platform_error(
        mtf::FailurePoint::create_client_platform,
        std::make_exception_ptr(std::runtime_error{exception_text}));

    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    EXPECT_FALSE(mir_connection_is_valid(connection));
    EXPECT_THAT(mir_connection_get_error_message(connection), testing::HasSubstr(exception_text));
    mir_connection_release(connection);
}

TEST_F(ClientLibraryErrors, connecting_to_garbage_socket_returns_appropriate_error)
{
    using namespace testing;

    auto connection = mir_connect_sync("garbage", __PRETTY_FUNCTION__);
    ASSERT_THAT(connection, NotNull());

    char const* error = mir_connection_get_error_message(connection);

    if (std::strcmp("connect: No such file or directory", error) &&
        std::strcmp("Can't find MIR server", error) &&
        !std::strstr(error, "Failed to connect to server socket"))
    {
        FAIL() << error;
    }
    mir_connection_release(connection);
}

TEST_F(ClientLibraryErrors, create_surface_returns_error_object_on_failure)
{
    std::string const exception_text{"Avocado!"};
    mtf::add_client_platform_error(
        mtf::FailurePoint::create_buffer_factory,
        std::make_exception_ptr(std::runtime_error{exception_text}));


    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    ASSERT_THAT(connection, IsValid());

    auto spec = mir_create_normal_window_spec(connection, 800, 600);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    mir_window_spec_set_pixel_format(spec, mir_pixel_format_xbgr_8888);
#pragma GCC diagnostic pop
    auto window = mir_create_window_sync(spec);
    mir_window_spec_release(spec);

    ASSERT_NE(window, nullptr);
    EXPECT_FALSE(mir_window_is_valid(window));
    EXPECT_THAT(mir_window_get_error_message(window), testing::HasSubstr(exception_text));

    mir_window_release_sync(window);
    mir_connection_release(connection);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
TEST_F(ClientLibraryErrors, create_buffer_stream_returns_error_object_on_failure)
{
    std::string const exception_text{"Extravaganza!"};
    mtf::add_client_platform_error(
        mtf::FailurePoint::create_buffer_factory,
        std::make_exception_ptr(std::runtime_error{exception_text}));

    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    ASSERT_THAT(connection, IsValid());

    auto stream = mir_connection_create_buffer_stream_sync(connection,
        640, 480, mir_pixel_format_abgr_8888, mir_buffer_usage_software);
    ASSERT_NE(stream, nullptr);
    EXPECT_FALSE(mir_buffer_stream_is_valid(stream));
    EXPECT_THAT(mir_buffer_stream_get_error_message(stream), testing::HasSubstr(exception_text));

    mir_buffer_stream_release_sync(stream);
    mir_connection_release(connection);
}
#pragma GCC diagnostic pop

TEST_F(ClientLibraryErrors, create_surface_doesnt_double_close_buffer_file_descriptors_on_error)
{
    using namespace testing;

    std::string const exception_text{"Master!"};
    mtf::add_client_platform_error(
        mtf::FailurePoint::create_buffer_factory,
        std::make_exception_ptr(std::runtime_error{exception_text}));

    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    ASSERT_THAT(connection, IsValid());

    auto spec = mir_create_normal_window_spec(connection, 800, 600);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    mir_window_spec_set_pixel_format(spec, mir_pixel_format_xbgr_8888);
#pragma GCC diagnostic pop
    auto window = mir_create_window_sync(spec);
    mir_window_spec_release(spec);

    mir_window_release_sync(window);
    mir_connection_release(connection);
}

namespace
{
void recording_surface_callback(MirWindow*, void* ctx)
{
    auto called = static_cast<bool*>(ctx);
    *called = true;
}
}

TEST_F(ClientLibraryErrors, surface_release_on_error_object_still_calls_callback)
{
    std::string const exception_text{"Beep! Boop!"};
    mtf::add_client_platform_error(
        mtf::FailurePoint::create_buffer_factory,
        std::make_exception_ptr(std::runtime_error{exception_text}));

    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    ASSERT_THAT(connection, IsValid());

    auto spec = mir_create_normal_window_spec(connection, 800, 600);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    mir_window_spec_set_pixel_format(spec, mir_pixel_format_xbgr_8888);
#pragma GCC diagnostic pop
    auto window = mir_create_window_sync(spec);
    mir_window_spec_release(spec);

    ASSERT_NE(window, nullptr);
    EXPECT_FALSE(mir_window_is_valid(window));
    EXPECT_THAT(mir_window_get_error_message(window), testing::HasSubstr(exception_text));

    bool callback_called{false};
    mir_window_release(window, &recording_surface_callback, &callback_called);
    EXPECT_TRUE(callback_called);
    mir_connection_release(connection);
}


TEST_F(ClientLibraryErrors, create_surface_returns_error_object_on_failure_in_reply_processing)
{
    std::string const exception_text{"...---...!"};
    mtf::add_client_platform_error(
        mtf::FailurePoint::create_egl_native_window,
        std::make_exception_ptr(std::runtime_error{exception_text}));

    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    ASSERT_THAT(connection, IsValid());

    auto spec = mir_create_normal_window_spec(connection, 800, 600);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    mir_window_spec_set_pixel_format(spec, mir_pixel_format_xbgr_8888);
#pragma GCC diagnostic pop
    auto window = mir_create_window_sync(spec);
    mir_window_spec_release(spec);

    ASSERT_NE(window, nullptr);
    EXPECT_FALSE(mir_window_is_valid(window));
    EXPECT_THAT(mir_window_get_error_message(window), testing::HasSubstr(exception_text));

    mir_window_release_sync(window);
    mir_connection_release(connection);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
TEST_F(ClientLibraryErrors, passing_invalid_parent_id_to_surface_create)
{
    using namespace testing;

    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    ASSERT_THAT(connection, IsValid());

    // An ID that parses as valid, but doesn't correspond to any
    auto invalid_id = mir_persistent_id_from_string("05f223a2-39e5-48b9-9416-b0ce837351b6");

    auto spec = mir_create_input_method_window_spec(connection, 200, 200);
    mir_window_spec_set_pixel_format(spec, mir_pixel_format_argb_8888);
    MirRectangle rect{
        100,
        100,
        10,
        10
    };
    mir_window_spec_attach_to_foreign_parent(spec, invalid_id, &rect, mir_edge_attachment_any);

    auto window = mir_create_window_sync(spec);
    EXPECT_THAT(window, Not(IsValid()));
    EXPECT_THAT(mir_window_get_error_message(window), MatchesRegex(".*Lookup.*failed.*"));

    mir_persistent_id_release(invalid_id);
    mir_window_spec_release(spec);
    mir_window_release_sync(window);
    mir_connection_release(connection);
}

using ClientLibraryErrorsDeathTest = ClientLibraryErrors;


TEST_F(ClientLibraryErrorsDeathTest, creating_surface_on_garbage_connection_is_fatal)
{
    auto connection = mir_connect_sync("garbage", __PRETTY_FUNCTION__);

    ASSERT_FALSE(mir_connection_is_valid(connection));
    MIR_EXPECT_DEATH(mtf::make_any_surface(connection), "");

    mir_connection_release(connection);
}


TEST_F(ClientLibraryErrorsDeathTest, creating_surface_synchronosly_on_invalid_connection_is_fatal)
{
    mtf::add_client_platform_error(
        mtf::FailurePoint::create_client_platform,
        std::make_exception_ptr(std::runtime_error{""}));

    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    ASSERT_FALSE(mir_connection_is_valid(connection));
    MIR_EXPECT_DEATH(mtf::make_any_surface(connection), "");

    mir_connection_release(connection);
}

TEST_F(ClientLibraryErrorsDeathTest, surface_spec_attaching_invalid_parent_id)
{
    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    auto spec = mir_create_input_method_window_spec(connection, 100, 100);
    mir_window_spec_set_pixel_format(spec, mir_pixel_format_argb_8888);

    MirRectangle rect{
        100,
        100,
        10,
        10
    };
    MIR_EXPECT_DEATH(
    {
        mir_window_spec_attach_to_foreign_parent(spec, nullptr, &rect,
                                                  mir_edge_attachment_any);
    }, "");

    mir_connection_release(connection);
}

TEST_F(ClientLibraryErrorsDeathTest, surface_spec_attaching_invalid_rectangle)
{
    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    auto spec = mir_create_input_method_window_spec(connection, 100, 100);
    mir_window_spec_set_pixel_format(spec, mir_pixel_format_argb_8888);

    auto id = mir_persistent_id_from_string("fa69b2e9-d507-4005-be61-5068f40a5aec");

    MIR_EXPECT_DEATH(
    {
        mir_window_spec_attach_to_foreign_parent(spec, id, nullptr,
                                                  mir_edge_attachment_any);
    }, "");

    mir_persistent_id_release(id);
    mir_connection_release(connection);
}

#pragma GCC diagnostic pop
TEST_F(ClientLibraryErrorsDeathTest, creating_screencast_with_invalid_connection)
{
    MIR_EXPECT_DEATH(mir_create_screencast_spec(nullptr), "");
}
