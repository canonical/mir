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

#include "mir/client/client_platform.h"
#include "src/client/probing_client_platform_factory.h"
#include "src/server/report/null_report_factory.h"

#include "mir/test/doubles/mock_client_context.h"
#include "mir_test_framework/executable_path.h"
#include "mir_test_framework/stub_platform_helpers.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <dlfcn.h>
#include <unordered_map>

namespace mtf = mir_test_framework;
namespace mtd = mir::test::doubles;

namespace
{

void populate_graphics_module_for(MirModuleProperties& props, mir::SharedLibrary const& server_library)
{
    auto server_description =
        server_library.load_function<MirModuleProperties const*(*)()>("describe_graphics_module");

    ::memcpy(&props, server_description(), sizeof(props));
}

#if defined(MIR_BUILD_PLATFORM_MESA_KMS) || defined(MIR_BUILD_PLATFORM_MESA_X11)
void populate_valid_mesa_platform_package(MirPlatformPackage& pkg)
{
    memset(&pkg, 0, sizeof(MirPlatformPackage));
    pkg.fd_items = 1;
    pkg.fd[0] = 23;
}
#endif

class ModuleContext
{
public:
    ModuleContext(
        std::string const& client_name,
        std::string const& server_name,
        std::function<void(MirPlatformPackage&)> const& populate_platform)
        : client_module_filename{mtf::client_platform(client_name)},
          server_library{std::make_shared<mir::SharedLibrary>(mtf::server_platform(server_name))},
          populate_platform{populate_platform}
    {
    }

    std::string client_module_filename;
    void setup_context(mtd::MockClientContext& context) const
    {
        using namespace testing;
        ON_CALL(context, populate_server_package(_))
            .WillByDefault(Invoke(populate_platform));
        // We need to keep the server platform DSO loaded, so shared-ptr-copy it into
        // the MockClientContext closure.
        ON_CALL(context, populate_graphics_module(_))
            .WillByDefault(Invoke(
                [server_module = server_library](auto& prop)
                {
                    populate_graphics_module_for(prop, *server_module);
                }));
    }

private:
    std::shared_ptr<mir::SharedLibrary> server_library;
    std::function<void(MirPlatformPackage&)> populate_platform;
};

ModuleContext dummy_fixture()
{
    return { "dummy.so", "graphics-dummy.so", &mtf::create_stub_platform_package };
}

std::unordered_map<std::string, ModuleContext>
all_available_fixtures()
{
    using namespace testing;
    std::unordered_map<std::string, ModuleContext> modules;

#if defined(MIR_BUILD_PLATFORM_MESA_KMS)
    modules.emplace(
        std::make_pair<std::string, ModuleContext>(
            "mesa-kms", { "mesa", "graphics-mesa-kms", &populate_valid_mesa_platform_package }));
#endif
#if defined(MIR_BUILD_PLATFORM_MESA_X11)
    modules.emplace(
        std::make_pair<std::string, ModuleContext>(
            "mesa-x11", { "mesa", "server-mesa-x11", &populate_valid_mesa_platform_package }));
#endif
    return modules;
}

std::vector<std::string> all_available_modules()
{
    std::vector<std::string> modules;
    auto const fixtures = all_available_fixtures();
    for (auto const& fixture : fixtures)
    {
        if (std::find(modules.begin(), modules.end(), fixture.second.client_module_filename) == modules.end())
        {
            modules.push_back(fixture.second.client_module_filename);
        }
    }
    return modules;
}

bool loaded(std::string const& path)
{
    void* x = dlopen(path.c_str(), RTLD_LAZY | RTLD_NOLOAD);
    if (x)
        dlclose(x);
    return !!x;
}

}

TEST(ProbingClientPlatformFactory, ThrowsErrorWhenConstructedWithNoPlatforms)
{
    std::vector<std::shared_ptr<mir::SharedLibrary>> empty_modules;
    EXPECT_THROW(mir::client::ProbingClientPlatformFactory(
                     mir::report::null_shared_library_prober_report(),
                     {}, {}, nullptr),
                 std::runtime_error);
}

TEST(ProbingClientPlatformFactory, ThrowsErrorWhenNoPlatformPluginProbesSuccessfully)
{
    using namespace testing;

    mir::client::ProbingClientPlatformFactory factory(
        mir::report::null_shared_library_prober_report(),
        all_available_modules(),
        {}, nullptr);

    NiceMock<mtd::MockClientContext> context;
    ON_CALL(context, populate_server_package(_))
            .WillByDefault(Invoke([](MirPlatformPackage& pkg)
                           {
                               ::memset(&pkg, 0, sizeof(MirPlatformPackage));
                               // Mock up a platform package that looks nothing like
                               // a Mesa package
                               pkg.fd_items = 0xdeadbeef;
                               pkg.data_items = -23;
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

    EXPECT_THROW(factory.create_client_platform(&context),
                 std::runtime_error);
}

TEST(ProbingClientPlatformFactory, DoesNotLeakTheUsedDriverModuleOnShutdown)
{   // Regression test for LP: #1527449
    using namespace testing;
    auto const fixtures = all_available_fixtures();
    ASSERT_FALSE(fixtures.empty());
    std::string const preferred_module = fixtures.begin()->second.client_module_filename;

    mir::client::ProbingClientPlatformFactory factory(
        mir::report::null_shared_library_prober_report(),
        {preferred_module},
        {}, nullptr);

    std::shared_ptr<mir::client::ClientPlatform> platform;
    NiceMock<mtd::MockClientContext> context;
    fixtures.begin()->second.setup_context(context);

    ASSERT_FALSE(loaded(preferred_module));
    platform = factory.create_client_platform(&context);
    ASSERT_TRUE(loaded(preferred_module));
    platform.reset();
    EXPECT_FALSE(loaded(preferred_module));
}

TEST(ProbingClientPlatformFactory, DoesNotLeakUnusedDriverModulesOnStartup)
{   // Regression test for LP: #1527449 and LP: #1526658
    using namespace testing;
    auto const modules = all_available_modules();
    auto const fixtures = all_available_fixtures();
    ASSERT_FALSE(modules.empty());

    // Note: This test is only really effective with nmodules>1, which many of
    //       our builds will have. But nmodules==1 is harmless.

    mir::client::ProbingClientPlatformFactory factory(
        mir::report::null_shared_library_prober_report(),
        modules,
        {}, nullptr);

    std::shared_ptr<mir::client::ClientPlatform> platform;
    NiceMock<mtd::MockClientContext> context;
    fixtures.begin()->second.setup_context(context);

    int nloaded = 0;
    for (auto const& m : modules)
        if (loaded(m)) ++nloaded;
    ASSERT_EQ(0, nloaded);

    platform = factory.create_client_platform(&context);

    nloaded = 0;
    for (auto const& m : modules)
        if (loaded(m)) ++nloaded;
    EXPECT_EQ(1, nloaded);

    platform.reset();

    nloaded = 0;
    for (auto const& m : modules)
        if (loaded(m)) ++nloaded;
    ASSERT_EQ(0, nloaded);
}

#if defined(MIR_BUILD_PLATFORM_MESA_KMS)
TEST(ProbingClientPlatformFactory, CreatesMesaPlatformOnMesaKMS)
#else
TEST(ProbingClientPlatformFactory, DISABLED_CreatesMesaPlatformOnMesaKMS)
#endif
{
    using namespace testing;

    mir::client::ProbingClientPlatformFactory factory(
        mir::report::null_shared_library_prober_report(),
        all_available_modules(),
        {}, nullptr);

    NiceMock<mtd::MockClientContext> context;
    all_available_fixtures().at("mesa-kms").setup_context(context);

    auto platform = factory.create_client_platform(&context);
    EXPECT_EQ(mir_platform_type_gbm, platform->platform_type());
}

#if defined(MIR_BUILD_PLATFORM_MESA_X11)
TEST(ProbingClientPlatformFactory, CreatesMesaPlatformOnMesaX11)
#else
TEST(ProbingClientPlatformFactory, DISABLED_CreatesMesaPlatformOnMesaX11)
#endif
{
    using namespace testing;

    mir::client::ProbingClientPlatformFactory factory(
        mir::report::null_shared_library_prober_report(),
        all_available_modules(),
        {}, nullptr);

    NiceMock<mtd::MockClientContext> context;
    all_available_fixtures().at("mesa-x11").setup_context(context);

    auto platform = factory.create_client_platform(&context);
    EXPECT_EQ(mir_platform_type_gbm, platform->platform_type());
}

TEST(ProbingClientPlatformFactory, IgnoresNonClientPlatformModules)
{
    using namespace testing;

    auto modules = all_available_modules();
    // NOTE: For minimum fuss, load something that has minimal side-effects...
    modules.push_back("libc.so.6");
    modules.push_back(dummy_fixture().client_module_filename);

    mir::client::ProbingClientPlatformFactory factory(
        mir::report::null_shared_library_prober_report(),
        modules,
        {}, nullptr);

    NiceMock<mtd::MockClientContext> context;
    dummy_fixture().setup_context(context);

    auto platform = factory.create_client_platform(&context);
}
