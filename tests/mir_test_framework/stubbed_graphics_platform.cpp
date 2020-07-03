/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#include "stubbed_graphics_platform.h"
#include "mir_test_framework/stub_graphics_platform_operation.h"

#include "mir/graphics/buffer_ipc_message.h"

#include "mir_test_framework/stub_platform_helpers.h"
#include "mir_test_framework/stub_platform_native_buffer.h"

#include "mir_toolkit/common.h"
#include "mir/test/doubles/stub_buffer_allocator.h"
#include "mir/test/doubles/fake_display.h"
#include "mir/fd.h"
#include "mir/assert_module_entry_point.h"
#include "mir/test/pipe.h"
#include "mir/libname.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <system_error>
#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>

namespace geom = mir::geometry;
namespace mg = mir::graphics;
namespace mo = mir::options;
namespace mtd = mir::test::doubles;
namespace mtf = mir_test_framework;

namespace
{

bool flavor_enabled = true;
struct WrappingDisplay : mg::Display
{
    WrappingDisplay(std::shared_ptr<mg::Display> const& display) : display{display} {}

    void for_each_display_sync_group(std::function<void(mg::DisplaySyncGroup&)> const& f) override
    {
        display->for_each_display_sync_group(f);
    }
    std::unique_ptr<mg::DisplayConfiguration> configuration() const override
    {
        return display->configuration();
    }
    bool apply_if_configuration_preserves_display_buffers(mg::DisplayConfiguration const& /*conf*/) override
    {
        return false;
    }
    void configure(mg::DisplayConfiguration const& conf) override
    {
        display->configure(conf);
    }
    void register_configuration_change_handler(
        mg::EventHandlerRegister& handlers,
        mg::DisplayConfigurationChangeHandler const& conf_change_handler) override
    {
        display->register_configuration_change_handler(handlers, conf_change_handler);
    }

    void register_pause_resume_handlers(
        mg::EventHandlerRegister& handlers,
        mg::DisplayPauseHandler const& pause_handler,
        mg::DisplayResumeHandler const& resume_handler) override
    {
        display->register_pause_resume_handlers(handlers, pause_handler, resume_handler);
    }

    void pause() override
    {
        display->pause();
    }

    void resume( )override
    {
        display->resume();
    }
    std::shared_ptr<mg::Cursor> create_hardware_cursor() override
    {
        return display->create_hardware_cursor();
    }
    std::unique_ptr<mg::VirtualOutput> create_virtual_output(int width, int height) override
    {
        return display->create_virtual_output(width, height);
    }
    mg::NativeDisplay* native_display() override
    {
        return display->native_display();
    }
    mg::Frame last_frame_on(unsigned output_id) const override
    {
        return display->last_frame_on(output_id);
    }
    std::shared_ptr<Display> const display;
};

class StubGraphicBufferAllocator : public mtd::StubBufferAllocator
{
 public:
    std::shared_ptr<mg::Buffer> alloc_software_buffer(geom::Size sz, MirPixelFormat pf) override
    {
        if (sz.width == geom::Width{0} || sz.height == geom::Height{0})
            BOOST_THROW_EXCEPTION(std::runtime_error("invalid size"));
        return mtd::StubBufferAllocator::alloc_software_buffer(sz, pf);
    }
};

}

mtf::StubGraphicPlatform::StubGraphicPlatform(std::vector<geom::Rectangle> const& display_rects)
    : display_rects{display_rects}
{
}

mir::UniqueModulePtr<mg::GraphicBufferAllocator> mtf::StubGraphicPlatform::create_buffer_allocator(
    mg::Display const&)
{
    return mir::make_module_ptr<StubGraphicBufferAllocator>();
}

namespace
{
std::shared_ptr<mg::Display> display_preset;
}

mir::UniqueModulePtr<mg::Display> mtf::StubGraphicPlatform::create_display(
    std::shared_ptr<mg::DisplayConfigurationPolicy> const&,
    std::shared_ptr<mg::GLConfig> const&)
{
    if (display_preset)
    {
        decltype(display_preset) temp;
        swap(temp, display_preset);
        return mir::make_module_ptr<WrappingDisplay>(temp);
    }

    return mir::make_module_ptr<mtd::FakeDisplay>(display_rects);
}

namespace
{
struct GuestPlatformAdapter : mg::Platform
{
    GuestPlatformAdapter(
        std::shared_ptr<mg::PlatformAuthentication> const& context,
        std::shared_ptr<mg::Platform> const& adaptee) :
        context(context),
        adaptee(adaptee)
    {
    }

    mir::UniqueModulePtr<mir::graphics::GraphicBufferAllocator> create_buffer_allocator(
        mg::Display const& output) override
    {
        return adaptee->create_buffer_allocator(output);
    }

    mir::UniqueModulePtr<mg::Display> create_display(
        std::shared_ptr<mg::DisplayConfigurationPolicy> const& initial_conf_policy,
        std::shared_ptr<mg::GLConfig> const& gl_config) override
    {
        return adaptee->create_display(initial_conf_policy, gl_config);
    }

    mg::NativeDisplayPlatform* native_display_platform() override
    {
        return adaptee->native_display_platform();
    }

    std::vector<mir::ExtensionDescription> extensions() const override
    {
        std::vector<mir::ExtensionDescription> ext
        {
            { "mir_extension_gbm_buffer", { 1, 2 } },
            { "mir_extension_fenced_buffers", { 1 } },
            { "mir_extension_hardware_buffer_stream", { 1 } },
            { "mir_extension_graphics_module", { 1 } },
            { "mir_extension_animal_names", {1} }
        };
        if (flavor_enabled)
            ext.push_back({ std::string{"mir_extension_favorite_flavor"}, {1, 9} });

        return ext;
    }

    std::shared_ptr<mg::PlatformAuthentication> const context;
    std::shared_ptr<mg::Platform> const adaptee;
};

std::weak_ptr<mg::Platform> the_graphics_platform{};
std::unique_ptr<std::vector<geom::Rectangle>> chosen_display_rects;
}

#if defined(__clang__)
#pragma clang diagnostic push
// These functions are given "C" linkage to avoid name-mangling, not for C compatibility.
// (We don't want a warning for doing this intentionally.)
#pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
#endif
extern "C" std::shared_ptr<mg::Platform> create_stub_platform(std::vector<geom::Rectangle> const& display_rects)
{
    return std::make_shared<mtf::StubGraphicPlatform>(display_rects);
}
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

mir::UniqueModulePtr<mg::Platform> create_host_platform(
    std::shared_ptr<mo::Option> const&,
    std::shared_ptr<mir::EmergencyCleanupRegistry> const&,
    std::shared_ptr<mir::ConsoleServices> const&,
    std::shared_ptr<mg::DisplayReport> const&,
    std::shared_ptr<mir::logging::Logger> const&)
{
    mir::assert_entry_point_signature<mg::CreateHostPlatform>(&create_host_platform);
    std::shared_ptr<mg::Platform> result{};

    if (auto const display_rects = std::move(chosen_display_rects))
    {
        result = create_stub_platform(*display_rects);
    }
    else
    {
        static std::vector<geom::Rectangle> const default_display_rects{geom::Rectangle{{0,0},{1600,1600}}};
        result = create_stub_platform(default_display_rects);
    }
    the_graphics_platform = result;
    return mir::make_module_ptr<GuestPlatformAdapter>(nullptr, result);
}

mir::UniqueModulePtr<mg::DisplayPlatform> create_display_platform(
    std::shared_ptr<mo::Option> const&,
    std::shared_ptr<mir::EmergencyCleanupRegistry> const&,
    std::shared_ptr<mir::ConsoleServices> const&,
    std::shared_ptr<mg::DisplayReport> const&,
    std::shared_ptr<mir::logging::Logger> const&)
{
    mir::assert_entry_point_signature<mg::CreateDisplayPlatform>(&create_display_platform);
    std::shared_ptr<mg::Platform> result{};

    if (auto const display_rects = std::move(chosen_display_rects))
    {
        result = create_stub_platform(*display_rects);
    }
    else
    {
        static std::vector<geom::Rectangle> const default_display_rects{geom::Rectangle{{0,0},{1600,1600}}};
        result = create_stub_platform(default_display_rects);
    }
    the_graphics_platform = result;
    return mir::make_module_ptr<GuestPlatformAdapter>(nullptr, result);
}

mir::UniqueModulePtr<mir::graphics::RenderingPlatform> create_rendering_platform(
    std::shared_ptr<mir::options::Option> const&, 
    std::shared_ptr<mir::graphics::PlatformAuthentication> const&) 
{
    mir::assert_entry_point_signature<mg::CreateRenderingPlatform>(&create_rendering_platform);

    static std::vector<geom::Rectangle> const default_display_rects{geom::Rectangle{{0,0},{1600,1600}}};
    auto result = create_stub_platform(default_display_rects);
    return mir::make_module_ptr<GuestPlatformAdapter>(nullptr, result);
}

void add_graphics_platform_options(
    boost::program_options::options_description& /*config*/)
{
    mir::assert_entry_point_signature<mg::AddPlatformOptions>(&add_graphics_platform_options);
}

extern "C" void set_next_display_rects(
    std::unique_ptr<std::vector<geom::Rectangle>>&& display_rects)
{
    chosen_display_rects = std::move(display_rects);
}

extern "C" void set_next_preset_display(std::shared_ptr<mir::graphics::Display> const& display)
{
    display_preset = display;
}

extern "C" void disable_flavors()
{
    flavor_enabled = false;
}
