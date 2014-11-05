/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/debug/surface.h"

#include "src/client/client_platform_factory.h"
#include "src/client/client_platform.h"
#include "src/client/client_buffer_factory.h"

#include "mir_test/validity_matchers.h"

#include "mir_test_framework/in_process_server.h"
#include "mir_test_framework/stubbed_server_configuration.h"
#include "mir_test_framework/using_stub_client_platform.h"
#include "mir_test_framework/stub_client_connection_configuration.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <stdexcept>
#include <boost/throw_exception.hpp>
#include <cstring>

namespace mcl = mir::client;
namespace mtf = mir_test_framework;

namespace
{
enum Method : uint64_t
{
    none = 0,
    the_client_platform_factory = 1<<0,
    create_client_platform      = 1<<1,
    create_egl_native_window    = 1<<2,
    create_buffer_factory       = 1<<3
};

std::string const exception_text{"Ducks!"};

template<Method name, Method failure_set>
bool should_fail()
{
    return (name & failure_set);
}

class StubClientBufferFactory : public mir::client::ClientBufferFactory
{
    std::shared_ptr<mir::client::ClientBuffer> create_buffer(const std::shared_ptr<MirBufferPackage>&,
                                                             mir::geometry::Size,
                                                             MirPixelFormat)
    {
        return std::shared_ptr<mir::client::ClientBuffer>{};
    }
};

template<Method failure_set>
class ConfigurableFailurePlatform : public mir::client::ClientPlatform
{
    std::shared_ptr<EGLNativeWindowType> create_egl_native_window(mir::client::ClientSurface *)
    {
        if (should_fail<Method::create_egl_native_window, failure_set>())
        {
            BOOST_THROW_EXCEPTION(std::runtime_error{exception_text});
        }
        return std::shared_ptr<EGLNativeWindowType>{};
    }
    MirPlatformType platform_type() const
    {
        BOOST_THROW_EXCEPTION(std::runtime_error{exception_text});
        return MirPlatformType::mir_platform_type_gbm;
    }
    std::shared_ptr<mir::client::ClientBufferFactory> create_buffer_factory()
    {
        if (should_fail<Method::create_buffer_factory, failure_set>())
        {
            BOOST_THROW_EXCEPTION(std::runtime_error{exception_text});
        }
        return std::make_shared<StubClientBufferFactory>();
    }
    std::shared_ptr<EGLNativeDisplayType> create_egl_native_display()
    {
        return std::shared_ptr<EGLNativeDisplayType>{};
    }
    MirNativeBuffer *convert_native_buffer(mir::graphics::NativeBuffer*) const
    {
        BOOST_THROW_EXCEPTION(std::runtime_error{exception_text});
        return nullptr;
    }
};

template<Method failure_set>
class ConfigurableFailureFactory: public mir::client::ClientPlatformFactory
{
    std::shared_ptr<mir::client::ClientPlatform>
    create_client_platform(mir::client::ClientContext* /*context*/) override
    {
        if (should_fail<Method::create_client_platform, failure_set>())
        {
            BOOST_THROW_EXCEPTION(std::runtime_error{exception_text});
        }
        return std::make_shared<ConfigurableFailurePlatform<failure_set>>();
    }
};

template<Method failure_set>
class ConfigurableFailureConfiguration : public mtf::StubConnectionConfiguration
{
    using mtf::StubConnectionConfiguration::StubConnectionConfiguration;

    std::shared_ptr<mir::client::ClientPlatformFactory> the_client_platform_factory() override
    {
        if (should_fail<Method::the_client_platform_factory, failure_set>())
        {
            BOOST_THROW_EXCEPTION(std::runtime_error{exception_text});
        }
        return std::make_shared<ConfigurableFailureFactory<failure_set>>();
    }
};

class ClientLibraryErrors : public mtf::InProcessServer
{
private:
    mtf::StubbedServerConfiguration config;

    mir::DefaultServerConfiguration &server_config() override
    {
        return config;
    }
};
}

TEST_F(ClientLibraryErrors, exception_in_client_configuration_constructor_generates_error)
{
    mtf::UsingClientPlatform<ConfigurableFailureConfiguration<Method::the_client_platform_factory>> stubby;

    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    EXPECT_FALSE(mir_connection_is_valid(connection));
    EXPECT_THAT(mir_connection_get_error_message(connection), testing::HasSubstr(exception_text));
    mir_connection_release(connection);
}

TEST_F(ClientLibraryErrors, exception_in_platform_construction_generates_error)
{
    mtf::UsingClientPlatform<ConfigurableFailureConfiguration<Method::create_client_platform>> stubby;

    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    EXPECT_FALSE(mir_connection_is_valid(connection));
    EXPECT_THAT(mir_connection_get_error_message(connection), testing::HasSubstr(exception_text));
    mir_connection_release(connection);
}

TEST_F(ClientLibraryErrors, connecting_to_garbage_socket_returns_appropriate_error)
{
    using namespace testing;
    mtf::UsingStubClientPlatform stubby;

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
    mtf::UsingClientPlatform<ConfigurableFailureConfiguration<Method::create_buffer_factory>> stubby;

    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    ASSERT_THAT(connection, IsValid());

    MirSurfaceParameters const request_params =
    {
        __PRETTY_FUNCTION__,
        640, 480,
        mir_pixel_format_abgr_8888,
        mir_buffer_usage_hardware,
        mir_display_output_id_invalid
    };

    auto surface = mir_connection_create_surface_sync(connection, &request_params);
    ASSERT_NE(surface, nullptr);
    EXPECT_FALSE(mir_surface_is_valid(surface));
    EXPECT_THAT(mir_surface_get_error_message(surface), testing::HasSubstr(exception_text));

    mir_surface_release_sync(surface);
    mir_connection_release(connection);
}

namespace
{
void recording_surface_callback(MirSurface*, void* ctx)
{
    auto called = static_cast<bool*>(ctx);
    *called = true;
}
}

TEST_F(ClientLibraryErrors, surface_release_on_error_object_still_calls_callback)
{
    mtf::UsingClientPlatform<ConfigurableFailureConfiguration<Method::create_buffer_factory>> stubby;

    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    ASSERT_THAT(connection, IsValid());

    MirSurfaceParameters const request_params =
    {
        __PRETTY_FUNCTION__,
        640, 480,
        mir_pixel_format_abgr_8888,
        mir_buffer_usage_hardware,
        mir_display_output_id_invalid
    };

    auto surface = mir_connection_create_surface_sync(connection, &request_params);
    ASSERT_NE(surface, nullptr);
    EXPECT_FALSE(mir_surface_is_valid(surface));
    EXPECT_THAT(mir_surface_get_error_message(surface), testing::HasSubstr(exception_text));

    bool callback_called{false};
    mir_surface_release(surface, &recording_surface_callback, &callback_called);
    EXPECT_TRUE(callback_called);
    mir_connection_release(connection);
}


TEST_F(ClientLibraryErrors, create_surface_returns_error_object_on_failure_in_reply_processing)
{
    mtf::UsingClientPlatform<ConfigurableFailureConfiguration<Method::create_egl_native_window>> stubby;

    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    ASSERT_THAT(connection, IsValid());

    MirSurfaceParameters const request_params =
    {
        __PRETTY_FUNCTION__,
        640, 480,
        mir_pixel_format_abgr_8888,
        mir_buffer_usage_hardware,
        mir_display_output_id_invalid
    };

    auto surface = mir_connection_create_surface_sync(connection, &request_params);
    ASSERT_NE(surface, nullptr);
    EXPECT_FALSE(mir_surface_is_valid(surface));
    EXPECT_THAT(mir_surface_get_error_message(surface), testing::HasSubstr(exception_text));

    mir_surface_release_sync(surface);
    mir_connection_release(connection);
}

using ClientLibraryErrorsDeathTest = ClientLibraryErrors;


TEST_F(ClientLibraryErrorsDeathTest, createing_surface_on_garbage_connection_is_fatal)
{
    mtf::UsingStubClientPlatform stubby;

    auto connection = mir_connect_sync("garbage", __PRETTY_FUNCTION__);

    MirSurfaceParameters const request_params =
    {
        __PRETTY_FUNCTION__,
        640, 480,
        mir_pixel_format_abgr_8888,
        mir_buffer_usage_hardware,
        mir_display_output_id_invalid
    };

    ASSERT_FALSE(mir_connection_is_valid(connection));
    EXPECT_DEATH(mir_connection_create_surface_sync(connection, &request_params), "");
}


TEST_F(ClientLibraryErrorsDeathTest, creating_surface_synchronosly_on_malconstructed_connection_is_fatal)
{
    MirSurfaceParameters const request_params =
    {
        __PRETTY_FUNCTION__,
        640, 480,
        mir_pixel_format_abgr_8888,
        mir_buffer_usage_hardware,
        mir_display_output_id_invalid
    };

    mtf::UsingClientPlatform<ConfigurableFailureConfiguration<Method::the_client_platform_factory>> stubby;

    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    ASSERT_FALSE(mir_connection_is_valid(connection));
    EXPECT_DEATH(mir_connection_create_surface_sync(connection, &request_params), "");
}

TEST_F(ClientLibraryErrorsDeathTest, creating_surface_synchronosly_on_invalid_connection_is_fatal)
{
    MirSurfaceParameters const request_params =
    {
        __PRETTY_FUNCTION__,
        640, 480,
        mir_pixel_format_abgr_8888,
        mir_buffer_usage_hardware,
        mir_display_output_id_invalid
    };

    mtf::UsingClientPlatform<ConfigurableFailureConfiguration<Method::create_client_platform>> stubby;

    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    ASSERT_FALSE(mir_connection_is_valid(connection));
    EXPECT_DEATH(mir_connection_create_surface_sync(connection, &request_params), "");
}
