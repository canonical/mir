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
    ClientPlatformTraits(std::string const& library,
                         std::function<void(MirPlatformPackage&)> populator,
                         MirPlatformType type)
        : platform_library_name{library},
          populate_package_for{populator},
          platform_type{type}
    {
    }

    std::string const platform_library_name;
    std::function<void(MirPlatformPackage&)> const populate_package_for;
    MirPlatformType const platform_type;
};

struct ClientPlatformTest : public ::testing::TestWithParam<ClientPlatformTraits const*>
{
    ClientPlatformTest()
        : platform_library{mtf::client_platform(GetParam()->platform_library_name)},
          create_client_platform{platform_library.load_function<mcl::CreateClientPlatform>("create_client_platform")},
          probe{platform_library.load_function<mcl::ClientPlatformProbe>("is_appropriate_module")}
    {
        using namespace testing;
        ON_CALL(context, populate_server_package(_))
            .WillByDefault(Invoke(GetParam()->populate_package_for));
    }

    mtd::MockClientContext context;
#ifdef MIR_BUILD_PLATFORM_ANDROID
    testing::NiceMock<mtd::HardwareAccessMock> hw_access_mock;
#endif
    mir::SharedLibrary platform_library;
    mcl::CreateClientPlatform const create_client_platform;
    mcl::ClientPlatformProbe const probe;
};

#ifdef MIR_BUILD_PLATFORM_ANDROID
ClientPlatformTraits const android_platform{"android",
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

#ifdef MIR_BUILD_PLATFORM_MESA_KMS
ClientPlatformTraits const mesa_platform{"mesa",
                                         [](MirPlatformPackage& pkg)
                                         {
                                             ::memset(&pkg, 0, sizeof(pkg));
                                             pkg.fd_items = 1;
                                         },
                                         mir_platform_type_gbm
                                        };

INSTANTIATE_TEST_CASE_P(Mesa,
                        ClientPlatformTest,
                        ::testing::Values(&mesa_platform));

#endif

ClientPlatformTraits const dummy_platform{"dummy.so",
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
    EXPECT_NE(*native_window, (EGLNativeWindowType) NULL);
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

    EXPECT_FALSE(probe(&context));
}
