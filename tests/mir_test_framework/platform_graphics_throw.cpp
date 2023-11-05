/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mir/graphics/platform.h"
#include "mir/test/doubles/null_platform.h"
#include "mir/test/doubles/null_emergency_cleanup.h"
#include "mir/assert_module_entry_point.h"
#include "mir/libname.h"
#include "mir/options/program_option.h"
#include "mir/udev/wrapper.h"

#include <boost/throw_exception.hpp>
#include <stdlib.h>
#include <unordered_map>
#include <libgen.h>
#include <malloc.h>
#include <mir/shared_library.h>

namespace mg = mir::graphics;
namespace mo = mir::options;
namespace mtd = mir::test::doubles;

namespace
{
class ExceptionThrowingPlatform : public mg::DisplayPlatform, public mg::RenderingPlatform
{
public:
    ExceptionThrowingPlatform()
        : should_throw{parse_exception_request(getenv("MIR_TEST_FRAMEWORK_THROWING_PLATFORM_EXCEPTIONS"))}
    {
        if (should_throw.at(ExceptionLocation::at_constructor))
            BOOST_THROW_EXCEPTION(std::runtime_error("Exception during construction"));

        std::unique_ptr<char, void(*)(void*)> library_path{strdup(mir::libname()), &free};
        auto platform_path = dirname(library_path.get());

        mir::SharedLibrary stub_platform_library{std::string(platform_path) + "/graphics-dummy.so"};
        auto create_stub_display_platform = stub_platform_library.load_function<mg::CreateDisplayPlatform>("create_display_platform");
        auto create_stub_render_platform = stub_platform_library.load_function<mg::CreateRenderPlatform>("create_rendering_platform");

        mtd::NullEmergencyCleanup null_cleanup;
        mg::SupportedDevice device = {
            nullptr,
            mg::probe::unsupported,
            nullptr
        };
        stub_render_platform = create_stub_render_platform(device, {}, mo::ProgramOption{}, null_cleanup);
        stub_display_platform = create_stub_display_platform(device, nullptr, nullptr, nullptr, nullptr);
    }

protected:
    auto maybe_create_provider(
        mg::RenderingProvider::Tag const&) -> std::shared_ptr<mg::RenderingProvider> override
    {
        return nullptr;
    }

    auto maybe_create_provider(mg::DisplayProvider::Tag const&)
        -> std::shared_ptr<mg::DisplayProvider> override
    {
        return nullptr;
    }

public:

    mir::UniqueModulePtr<mir::graphics::GraphicBufferAllocator>
    create_buffer_allocator(mg::Display const& output) override
    {
        if (should_throw.at(ExceptionLocation::at_create_buffer_allocator))
            BOOST_THROW_EXCEPTION(std::runtime_error("Exception during create_buffer_allocator"));

        return stub_render_platform->create_buffer_allocator(output);
    }

    mir::UniqueModulePtr<mg::Display> create_display(
        std::shared_ptr<mg::DisplayConfigurationPolicy> const& ptr,
        std::shared_ptr<mg::GLConfig> const& shared_ptr) override
    {
        if (should_throw.at(ExceptionLocation::at_create_display))
            BOOST_THROW_EXCEPTION(std::runtime_error("Exception during create_display"));

        return stub_display_platform->create_display(ptr, shared_ptr);
    }

private:
    enum ExceptionLocation : uint32_t
    {
        at_constructor,
        at_create_buffer_allocator,
        at_create_display,
        at_make_ipc_operations,
        at_native_rendering_platform,
    };

    static std::unordered_map<ExceptionLocation, bool, std::hash<uint32_t>> parse_exception_request(char const* request)
    {
        std::unordered_map<ExceptionLocation, bool, std::hash<uint32_t>> requested_exceptions;
        requested_exceptions[ExceptionLocation::at_constructor] =
            static_cast<bool>(strstr(request, "constructor"));
        requested_exceptions[ExceptionLocation::at_create_buffer_allocator] =
            static_cast<bool>(strstr(request, "create_buffer_allocator"));
        requested_exceptions[ExceptionLocation::at_create_display] =
            static_cast<bool>(strstr(request, "create_display"));
        requested_exceptions[ExceptionLocation::at_make_ipc_operations] =
            static_cast<bool>(strstr(request, "make_ipc_operations"));
        requested_exceptions[ExceptionLocation::at_native_rendering_platform] =
            static_cast<bool>(strstr(request, "native_rendering_platform"));

        return requested_exceptions;
    };

    std::unordered_map<ExceptionLocation, bool, std::hash<uint32_t>> const should_throw;
    mir::UniqueModulePtr<mg::RenderingPlatform> stub_render_platform;
    std::shared_ptr<mg::DisplayPlatform> stub_display_platform;
};

}

auto probe_display_platform(
    std::shared_ptr<mir::ConsoleServices> const&,
    std::shared_ptr<mir::udev::Context> const&,
    mir::options::ProgramOption const&) -> std::vector<mir::graphics::SupportedDevice>
{
    mir::assert_entry_point_signature<mg::PlatformProbe>(&probe_display_platform);
    std::vector<mg::SupportedDevice> result;
    result.emplace_back(
        mg::SupportedDevice {
            nullptr,
            mg::probe::unsupported,
            nullptr
         });
    return result;
}

auto probe_rendering_platform(
    std::span<std::shared_ptr<mg::DisplayPlatform>> const&,
    mir::ConsoleServices&,
    std::shared_ptr<mir::udev::Context> const&,
    mir::options::ProgramOption const&) -> std::vector<mir::graphics::SupportedDevice>
{
    mir::assert_entry_point_signature<mg::RenderProbe>(&probe_rendering_platform);
    std::vector<mg::SupportedDevice> result;
    result.emplace_back(
        mg::SupportedDevice {
            nullptr,
            mg::probe::unsupported,
            nullptr
         });
    return result;
}

mir::ModuleProperties const description {
    "throw-on-creation",
    MIR_VERSION_MAJOR,
    MIR_VERSION_MINOR,
    MIR_VERSION_MICRO,
    mir::libname()
};

mir::ModuleProperties const* describe_graphics_module()
{
    mir::assert_entry_point_signature<mg::DescribeModule>(&describe_graphics_module);
    return &description;
}

void add_graphics_platform_options(boost::program_options::options_description&)
{
    mir::assert_entry_point_signature<mg::AddPlatformOptions>(&add_graphics_platform_options);
}

mir::UniqueModulePtr<mg::RenderingPlatform> create_rendering_platform(
    mg::SupportedDevice const&,
    std::vector<std::shared_ptr<mg::DisplayPlatform>> const&,
    mo::Option const&,
    mir::EmergencyCleanupRegistry&)
{
    mir::assert_entry_point_signature<mg::CreateRenderPlatform>(&create_rendering_platform);
    return mir::make_module_ptr<ExceptionThrowingPlatform>();
}

mir::UniqueModulePtr<mg::DisplayPlatform> create_display_platform(
    mg::SupportedDevice const&,
    std::shared_ptr<mo::Option> const&,
    std::shared_ptr<mir::EmergencyCleanupRegistry> const&,
    std::shared_ptr<mir::ConsoleServices> const&,
    std::shared_ptr<mg::DisplayReport> const&)
{
    mir::assert_entry_point_signature<mg::CreateDisplayPlatform>(&create_display_platform);
    return mir::make_module_ptr<ExceptionThrowingPlatform>();
}

