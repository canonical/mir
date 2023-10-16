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

#include "stubbed_graphics_platform.h"

#include "mir/graphics/platform.h"

#include "mir_toolkit/common.h"
#include "mir/test/doubles/stub_buffer_allocator.h"
#include "mir/test/doubles/stub_gl_rendering_provider.h"
#include "mir/test/doubles/fake_display.h"
#include "mir/assert_module_entry_point.h"

#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>

#include <stdexcept>

namespace geom = mir::geometry;
namespace mg = mir::graphics;
namespace mo = mir::options;
namespace mtd = mir::test::doubles;
namespace mtf = mir_test_framework;

namespace
{

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

extern "C" void set_next_display_rects(std::unique_ptr<std::vector<geom::Rectangle>>&& display_rects);

namespace
{
std::unique_ptr<mg::Display> display_preset;
}

mir::UniqueModulePtr<mg::Display> mtf::StubGraphicPlatform::create_display(
    std::shared_ptr<mg::DisplayConfigurationPolicy> const&,
    std::shared_ptr<mg::GLConfig> const&)
{
    if (display_preset)
    {
        struct Deleter : mir::ModuleDeleter<mg::Display>
        {
            Deleter()
                : ModuleDeleter<mg::Display>(reinterpret_cast<void*>(&set_next_display_rects))
            {
            }
        };

        return mir::UniqueModulePtr<mg::Display>{display_preset.release(), Deleter{}};
    }

    return mir::make_module_ptr<mtd::FakeDisplay>(display_rects);
}

auto mtf::StubGraphicPlatform::maybe_create_provider(
    mir::graphics::RenderingProvider::Tag const& tag)
    -> std::shared_ptr<mir::graphics::RenderingProvider>
{
    if (dynamic_cast<mg::GLRenderingProvider::Tag const*>(&tag))
    {
        return std::make_shared<mtd::StubGlRenderingProvider>();
    }
    return nullptr;
}

auto mtf::StubGraphicPlatform::target_for()
    -> std::shared_ptr<mg::DisplayTarget>
{
    class NullInterfaceProvider : public mg::DisplayTarget
    {
    protected:
        auto maybe_create_interface(mg::DisplayProvider::Tag const&)
            -> std::shared_ptr<mg::DisplayProvider>
        {
            return nullptr;
        }
    };
    return std::make_shared<NullInterfaceProvider>();
}

namespace
{
std::unique_ptr<std::vector<geom::Rectangle>> chosen_display_rects;
}

#if defined(__clang__)
#pragma clang diagnostic push
// These functions are given "C" linkage to avoid name-mangling, not for C compatibility.
// (We don't want a warning for doing this intentionally.)
#pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
#endif
extern "C" std::shared_ptr<mtf::StubGraphicPlatform> create_stub_platform(std::vector<geom::Rectangle> const& display_rects)
{
    return std::make_shared<mtf::StubGraphicPlatform>(display_rects);
}

extern "C" std::shared_ptr<mg::RenderingPlatform> create_stub_render_platform()
{
    static std::vector<geom::Rectangle> const default_display_rects{geom::Rectangle{{0,0},{1600,1600}}};
    return std::make_shared<mtf::StubGraphicPlatform>(default_display_rects);
}
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

mir::UniqueModulePtr<mg::DisplayPlatform> create_display_platform(
    mg::SupportedDevice const&,
    std::shared_ptr<mo::Option> const&,
    std::shared_ptr<mir::EmergencyCleanupRegistry> const&,
    std::shared_ptr<mir::ConsoleServices> const&,
    std::shared_ptr<mg::DisplayReport> const&)
{
    mir::assert_entry_point_signature<mg::CreateDisplayPlatform>(&create_display_platform);

    if (auto const display_rects = std::move(chosen_display_rects))
    {
        return mir::make_module_ptr<mtf::StubGraphicPlatform>(*display_rects);
    }
    else
    {
        static std::vector<geom::Rectangle> const default_display_rects{geom::Rectangle{{0,0},{1600,1600}}};
        return mir::make_module_ptr<mtf::StubGraphicPlatform>(default_display_rects);
    }
}

mir::UniqueModulePtr<mg::RenderingPlatform> create_rendering_platform(
    mg::SupportedDevice const&,
    std::vector<std::shared_ptr<mg::DisplayTarget>> const&,
    mo::Option const&,
    mir::EmergencyCleanupRegistry&)
{
    mir::assert_entry_point_signature<mg::CreateRenderPlatform>(&create_rendering_platform);

    static std::vector<geom::Rectangle> const default_display_rects{geom::Rectangle{{0,0},{1600,1600}}};
    return mir::make_module_ptr<mtf::StubGraphicPlatform>(default_display_rects);
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

extern "C" void set_next_preset_display(std::unique_ptr<mir::graphics::Display> display)
{
    display_preset = std::move(display);
}
