/*
 * Copyright © 2015 Canonical Ltd.
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
 *
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "mir/graphics/platform.h"
#include "mir/test/doubles/null_platform.h"
#include "mir/assert_module_entry_point.h"
#include "mir/libname.h"

#include <boost/throw_exception.hpp>
#include <stdlib.h>
#include <unordered_map>
#include <libgen.h>
#include <malloc.h>
#include <mir/shared_library.h>

namespace mg = mir::graphics;
namespace mo = mir::options;
namespace mir
{
namespace options
{
class ProgramOption;
}
}

namespace
{
class ExceptionThrowingPlatform : public mg::Platform
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
        auto create_stub_platform = stub_platform_library.load_function<mg::CreateHostPlatform>("create_host_platform");

        stub_platform = create_stub_platform(nullptr, nullptr, nullptr, nullptr, nullptr);
    }

    mir::UniqueModulePtr<mir::graphics::GraphicBufferAllocator>
    create_buffer_allocator(mg::Display const& output) override
    {
        if (should_throw.at(ExceptionLocation::at_create_buffer_allocator))
            BOOST_THROW_EXCEPTION(std::runtime_error("Exception during create_buffer_allocator"));

        return stub_platform->create_buffer_allocator(output);
    }

    mir::UniqueModulePtr<mg::Display> create_display(
        std::shared_ptr<mg::DisplayConfigurationPolicy> const& ptr,
        std::shared_ptr<mg::GLConfig> const& shared_ptr) override
    {
        if (should_throw.at(ExceptionLocation::at_create_display))
            BOOST_THROW_EXCEPTION(std::runtime_error("Exception during create_display"));

        return stub_platform->create_display(ptr, shared_ptr);
    }

    mir::UniqueModulePtr<mg::PlatformIpcOperations> make_ipc_operations() const override
    {
        if (should_throw.at(ExceptionLocation::at_make_ipc_operations))
            BOOST_THROW_EXCEPTION(std::runtime_error("Exception during make_ipc_operations"));

        return stub_platform->make_ipc_operations();
    }

    mg::NativeRenderingPlatform* native_rendering_platform() override
    {
        if (should_throw.at(ExceptionLocation::at_native_rendering_platform))
            BOOST_THROW_EXCEPTION(std::runtime_error("Exception during egl_native_display"));

        return stub_platform->native_rendering_platform();
    }

    mg::NativeDisplayPlatform* native_display_platform() override
    {
        if (should_throw.at(ExceptionLocation::at_native_display_platform))
            BOOST_THROW_EXCEPTION(std::runtime_error("Exception during egl_native_display"));

        return stub_platform->native_display_platform();
    }

    std::vector<mir::ExtensionDescription> extensions() const override
    {
        return {};
    }

private:
    enum ExceptionLocation : uint32_t
    {
        at_constructor,
        at_create_buffer_allocator,
        at_create_display,
        at_make_ipc_operations,
        at_native_rendering_platform,
        at_native_display_platform,
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
        requested_exceptions[ExceptionLocation::at_native_display_platform] =
            static_cast<bool>(strstr(request, "native_display_platform"));

        return requested_exceptions;
    };

    std::unordered_map<ExceptionLocation, bool, std::hash<uint32_t>> const should_throw;
    mir::UniqueModulePtr<mg::Platform> stub_platform;
};

}

mg::PlatformPriority probe_graphics_platform(
    std::shared_ptr<mir::ConsoleServices> const&,
    mo::ProgramOption const& /*options*/)
{
    mir::assert_entry_point_signature<mg::PlatformProbe>(&probe_graphics_platform);
    return mg::PlatformPriority::unsupported;
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

mir::UniqueModulePtr<mg::Platform> create_host_platform(
    std::shared_ptr<mo::Option> const&,
    std::shared_ptr<mir::EmergencyCleanupRegistry> const&,
    std::shared_ptr<mir::ConsoleServices> const&,
    std::shared_ptr<mg::DisplayReport> const&,
    std::shared_ptr<mir::logging::Logger> const&)
{
    mir::assert_entry_point_signature<mg::CreateHostPlatform>(&create_host_platform);
    return mir::make_module_ptr<ExceptionThrowingPlatform>();
}

