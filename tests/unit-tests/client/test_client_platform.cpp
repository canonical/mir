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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/client_platform.h"
#include "mir/egl_native_surface.h"

#include "mir/test/doubles/mock_client_context.h"
#include "mir/test/doubles/mock_egl_native_surface.h"
#include "mir_test_framework/executable_path.h"
#include "mir_test_framework/stub_platform_helpers.h"

#ifdef MIR_BUILD_PLATFORM_ANDROID
#include "mir/test/doubles/mock_android_hw.h"
#endif

#include "mir/client_platform_factory.h"

#include "mir/shared_library.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mcl=mir::client;
namespace mtd = mir::test::doubles;
namespace mtf = mir_test_framework;

namespace
{
struct ClientPlatformTraits
{
    ClientPlatformTraits(
        std::string const& server_platform,
        std::string const& client_platform,
        std::function<void(MirPlatformPackage&)> populator,
        MirPlatformType type)
        : server_platform_library_name{server_platform},
          platform_library_name{client_platform},
          populate_package_for{populator},
          platform_type{type}
    {
    }

    std::string const server_platform_library_name;
    std::string const platform_library_name;
    std::function<void(MirPlatformPackage&)> const populate_package_for;
    MirPlatformType const platform_type;
};

struct ClientPlatformTest : public ::testing::TestWithParam<ClientPlatformTraits const*>
{
    ClientPlatformTest()
        : server_platform_library{mtf::server_platform(GetParam()->server_platform_library_name)},
          platform_library{mtf::client_platform(GetParam()->platform_library_name)},
          create_client_platform{platform_library.load_function<mcl::CreateClientPlatform>("create_client_platform")},
          probe{platform_library.load_function<mcl::ClientPlatformProbe>("is_appropriate_module")},
          server_description{
              server_platform_library.load_function<MirPlatformPackage const*(*)()>("describe_graphics_module")}
    {
        using namespace testing;
        ON_CALL(context, populate_server_package(_))
            .WillByDefault(Invoke(GetParam()->populate_package_for));
        ON_CALL(context, populate_graphics_module(_))
            .WillByDefault(Invoke(
                [this](auto& props)
                {
                    ::memcpy(&props, server_description(), sizeof(props));
                }));
    }

    testing::NiceMock<mtd::MockClientContext> context;
#ifdef MIR_BUILD_PLATFORM_ANDROID
    testing::NiceMock<mtd::HardwareAccessMock> hw_access_mock;
#endif
    mir::SharedLibrary const server_platform_library;
    mir::SharedLibrary const platform_library;
    mcl::CreateClientPlatform const create_client_platform;
    mcl::ClientPlatformProbe const probe;
    MirPlatformPackage const* (* const server_description)();
};

#ifdef MIR_BUILD_PLATFORM_ANDROID
ClientPlatformTraits const android_platform{"graphics-android",
                                            "android",
                                            [](MirPlatformPackage& pkg)
                                            {
                                                ::memset(&pkg, 0, sizeof(pkg));
                                            },
                                            mir_platform_type_android
                                           };

INSTANTIATE_TEST_CASE_P(Android,
                        ClientPlatformTest,
                        ::testing::Values(&android_platform));

#endif

#if defined(MIR_BUILD_PLATFORM_MESA_KMS)
ClientPlatformTraits const mesa_kms_platform{
    "graphics-mesa-kms",
    "mesa",
    [](MirPlatformPackage& pkg)
    {
        ::memset(&pkg, 0, sizeof(pkg));
        pkg.fd_items = 1;
    },
    mir_platform_type_gbm
};

INSTANTIATE_TEST_CASE_P(MesaKMS,
                        ClientPlatformTest,
                        ::testing::Values(&mesa_kms_platform));

#endif

#if defined(MIR_BUILD_PLATFORM_MESA_X11)

ClientPlatformTraits const mesa_x11_platform{
    "server-mesa-x11",
    "mesa",
    [](MirPlatformPackage& pkg)
    {
        ::memset(&pkg, 0, sizeof(pkg));
        pkg.fd_items = 1;
    },
    mir_platform_type_gbm
};

INSTANTIATE_TEST_CASE_P(MesaX11,
                        ClientPlatformTest,
                        ::testing::Values(&mesa_x11_platform));

#endif

ClientPlatformTraits const dummy_platform{
    "graphics-dummy.so",
    "dummy.so",
    [](MirPlatformPackage& pkg)
    {
        mtf::create_stub_platform_package(pkg);
    },
    mir_platform_type_gbm
};

INSTANTIATE_TEST_CASE_P(Dummy,
                        ClientPlatformTest,
                        ::testing::Values(&dummy_platform));
}

TEST_P(ClientPlatformTest, platform_name)
{
    auto platform = create_client_platform(&context);

    EXPECT_EQ(GetParam()->platform_type, platform->platform_type());
}

TEST_P(ClientPlatformTest, platform_creates)
{
    auto platform = create_client_platform(&context);
    auto buffer_factory = platform->create_buffer_factory();
    EXPECT_NE(buffer_factory.get(), (mcl::ClientBufferFactory*) NULL);
}

TEST_P(ClientPlatformTest, platform_creates_native_window)
{
    auto platform = create_client_platform(&context);
    auto mock_client_surface = std::make_shared<mtd::MockEGLNativeSurface>();
    auto native_window = platform->create_egl_native_window(mock_client_surface.get());
    EXPECT_THAT(native_window.get(), testing::Ne(nullptr));
}

TEST_P(ClientPlatformTest, platform_creates_egl_native_display)
{
    auto platform = create_client_platform(&context);
    auto native_display = platform->create_egl_native_display();
    EXPECT_NE(nullptr, native_display.get());
}

TEST_P(ClientPlatformTest, platform_probe_returns_success_when_matching)
{
    EXPECT_TRUE(probe(&context));
}

TEST_P(ClientPlatformTest, platform_probe_returns_false_when_not_matching)
{
    using namespace testing;
    ON_CALL(context, populate_server_package(_))
        .WillByDefault(Invoke([](MirPlatformPackage& pkg)
                              {
                                  //Mock up something that hopefully looks nothing like
                                  //what the platform is expecting...
                                  ::memset(&pkg, 0, sizeof(pkg));
                                  pkg.data_items = mir_platform_package_max + 1;
                                  pkg.fd_items = -23;
                              }));
    MirModuleProperties const dummy_properties = {
        "mir:not-actually-a-platform",
        0,
        0,
        0,
        "No, I'm also not a filename"
    };
    ON_CALL(context, populate_graphics_module(_))
        .WillByDefault(Invoke([&dummy_properties](MirModuleProperties& props)
                              {
                                  ::memcpy(&props, &dummy_properties, sizeof(props));
                              }));


    EXPECT_FALSE(probe(&context));
}
