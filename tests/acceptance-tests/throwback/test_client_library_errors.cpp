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

#include "src/include/client/mir/client_platform_factory.h"
#include "src/include/client/mir/client_platform.h"
#include "src/include/client/mir/client_buffer_factory.h"

#include "mir/test/validity_matchers.h"

#include "mir_test_framework/headless_in_process_server.h"
#include "mir_test_framework/using_client_platform.h"
#include "mir_test_framework/using_stub_client_platform.h"
#include "mir_test_framework/stub_client_connection_configuration.h"
#include "mir_test_framework/any_surface.h"
#include "mir/test/doubles/stub_client_buffer_factory.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <stdexcept>
#include <boost/throw_exception.hpp>
#include <cstring>

namespace mcl = mir::client;
namespace mtf = mir_test_framework;
namespace mtd = mir::test::doubles;

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

template<Method failure_set>
class ConfigurableFailurePlatform : public mir::client::ClientPlatform
{
    std::shared_ptr<EGLNativeWindowType> create_egl_native_window(mir::client::EGLNativeSurface *)
    {
        if (should_fail<Method::create_egl_native_window, failure_set>())
        {
            BOOST_THROW_EXCEPTION(std::runtime_error{exception_text});
        }
        return std::shared_ptr<EGLNativeWindowType>{};
    }

    void populate(MirPlatformPackage&) const override
    {
    }

    MirPlatformMessage* platform_operation(MirPlatformMessage const*) override
    {
        return nullptr;
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
        return std::make_shared<mtd::StubClientBufferFactory>();
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
    MirPixelFormat get_egl_pixel_format(EGLDisplay, EGLConfig) const override
    {
        return mir_pixel_format_invalid;
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

struct ClientLibraryErrors : mtf::HeadlessInProcessServer
{
    ClientLibraryErrors() { add_to_environment("MIR_SERVER_ENABLE_INPUT","off"); }
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

    auto surface = mtf::make_any_surface(connection);
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

    auto surface = mtf::make_any_surface(connection);

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

    auto surface = mtf::make_any_surface(connection);
    ASSERT_NE(surface, nullptr);
    EXPECT_FALSE(mir_surface_is_valid(surface));
    EXPECT_THAT(mir_surface_get_error_message(surface), testing::HasSubstr(exception_text));

    mir_surface_release_sync(surface);
    mir_connection_release(connection);
}

TEST_F(ClientLibraryErrors, passing_invalid_parent_id_to_surface_create)
{
    using namespace testing;

    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    ASSERT_THAT(connection, IsValid());

    // An ID that parses as valid, but doesn't correspond to any
    auto invalid_id = mir_persistent_id_from_string("05f223a2-39e5-48b9-9416-b0ce837351b6");

    auto spec = mir_connection_create_spec_for_input_method(connection,
                                                            200, 200,
                                                            mir_pixel_format_argb_8888);
    MirRectangle rect{
        100,
        100,
        10,
        10
    };
    mir_surface_spec_attach_to_foreign_parent(spec, invalid_id, &rect, mir_edge_attachment_any);

    auto surface = mir_surface_create_sync(spec);
    EXPECT_THAT(surface, Not(IsValid()));
    EXPECT_THAT(mir_surface_get_error_message(surface), MatchesRegex(".*Lookup.*failed.*"));

    mir_persistent_id_release(invalid_id);
    mir_surface_spec_release(spec);
    mir_surface_release_sync(surface);
    mir_connection_release(connection);
}

using ClientLibraryErrorsDeathTest = ClientLibraryErrors;


TEST_F(ClientLibraryErrorsDeathTest, creating_surface_on_garbage_connection_is_fatal)
{
    mtf::UsingStubClientPlatform stubby;

    auto connection = mir_connect_sync("garbage", __PRETTY_FUNCTION__);

    ASSERT_FALSE(mir_connection_is_valid(connection));
    EXPECT_DEATH(
        mtf::make_any_surface(connection), "");

    mir_connection_release(connection);
}


TEST_F(ClientLibraryErrorsDeathTest, creating_surface_synchronosly_on_malconstructed_connection_is_fatal)
{
    mtf::UsingClientPlatform<ConfigurableFailureConfiguration<Method::the_client_platform_factory>> stubby;

    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    ASSERT_FALSE(mir_connection_is_valid(connection));
    EXPECT_DEATH(mtf::make_any_surface(connection), "");

    mir_connection_release(connection);
}

TEST_F(ClientLibraryErrorsDeathTest, creating_surface_synchronosly_on_invalid_connection_is_fatal)
{
    mtf::UsingClientPlatform<ConfigurableFailureConfiguration<Method::create_client_platform>> stubby;

    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    ASSERT_FALSE(mir_connection_is_valid(connection));
    EXPECT_DEATH(mtf::make_any_surface(connection), "");

    mir_connection_release(connection);
}

TEST_F(ClientLibraryErrorsDeathTest, surface_spec_attaching_invalid_parent_id)
{
    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    auto spec = mir_connection_create_spec_for_input_method(connection, 100, 100, mir_pixel_format_argb_8888);

    MirRectangle rect{
        100,
        100,
        10,
        10
    };
    EXPECT_DEATH(mir_surface_spec_attach_to_foreign_parent(spec, nullptr, &rect, mir_edge_attachment_any), "");

    mir_connection_release(connection);
}

TEST_F(ClientLibraryErrorsDeathTest, surface_spec_attaching_invalid_rectangle)
{
    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    auto spec = mir_connection_create_spec_for_input_method(connection, 100, 100, mir_pixel_format_argb_8888);

    auto id = mir_persistent_id_from_string("fa69b2e9-d507-4005-be61-5068f40a5aec");

    EXPECT_DEATH(mir_surface_spec_attach_to_foreign_parent(spec, id, nullptr, mir_edge_attachment_any), "");

    mir_persistent_id_release(id);
    mir_connection_release(connection);
}
