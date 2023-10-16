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

#include "test_rendering_platform.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <boost/throw_exception.hpp>

#include "mir/graphics/platform.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/options/program_option.h"
#include "mir/emergency_cleanup_registry.h"
#include "mir/udev/wrapper.h"
#include "mir/test/doubles/null_console_services.h"

namespace mg = mir::graphics;
namespace mtd = mir::test::doubles;

RenderingPlatformTest::RenderingPlatformTest()
{
    GetParam()->setup();
}

RenderingPlatformTest::~RenderingPlatformTest()
{
    GetParam()->teardown();
}

TEST_P(RenderingPlatformTest, has_render_platform_entrypoints)
{
    platform_module = GetParam()->platform_module();

    try
    {
        platform_module->load_function<mg::DescribeModule>(
            "describe_graphics_module",
            MIR_SERVER_GRAPHICS_PLATFORM_VERSION);
    }
    catch(std::runtime_error const& err)
    {
        FAIL()
            << "Failed to find describe_graphics_module (version " << MIR_SERVER_GRAPHICS_PLATFORM_VERSION << "): "
            << err.what();
    }

    try
    {
        platform_module->load_function<mg::CreateRenderPlatform>(
            "create_rendering_platform",
            MIR_SERVER_GRAPHICS_PLATFORM_VERSION);
    }
    catch(std::runtime_error const& err)
    {
        FAIL()
            << "Failed to find create_rendering_platform (version " << MIR_SERVER_GRAPHICS_PLATFORM_VERSION << "): "
            << err.what();
    }

    try
    {
        platform_module->load_function<mg::RenderProbe>(
            "probe_rendering_platform",
            MIR_SERVER_GRAPHICS_PLATFORM_VERSION);
    }
    catch(std::runtime_error const& err)
    {
        FAIL()
            << "Failed to find probe_rendering_platform (version " << MIR_SERVER_GRAPHICS_PLATFORM_VERSION << "): "
            << err.what();
    }
}

namespace
{
class NullEmergencyCleanup : public mir::EmergencyCleanupRegistry
{
public:
    void add(mir::EmergencyCleanupHandler const&) override
    {
    }

    void add(mir::ModuleEmergencyCleanupHandler) override
    {
    }
};
}

/*
 * DISABLED, as this requires more thought
 */
TEST_P(RenderingPlatformTest, DISABLED_supports_gl_rendering)
{
    using namespace testing;
    
    platform_module = GetParam()->platform_module();

    auto const platform_loader = platform_module->load_function<mg::CreateRenderPlatform>(
        "create_rendering_platform",
        MIR_SERVER_GRAPHICS_PLATFORM_VERSION);
    auto const platform_probe = platform_module->load_function<mg::PlatformProbe>(
        "probe_rendering_platform",
        MIR_SERVER_GRAPHICS_PLATFORM_VERSION);

    mir::options::ProgramOption empty_options{};
    NullEmergencyCleanup emergency_cleanup{};
    auto const console_services = std::make_shared<mir::test::doubles::NullConsoleServices>();

    auto supported_devices = platform_probe(
        console_services,
        std::make_shared<mir::udev::Context>(),
        empty_options);
    
    ASSERT_THAT(supported_devices, Not(IsEmpty()));
    
    for (auto const& device : supported_devices)
    {
        std::shared_ptr<mg::RenderingPlatform> const platform = platform_loader(
            device,
            {},            // Hopefully the platform can handle not pre-linking with a DisplayPlatform
            empty_options,
            emergency_cleanup);

        auto const provider = platform->acquire_provider<mg::GLRenderingProvider>(nullptr);
        EXPECT_THAT(provider, testing::NotNull());
    }
}
