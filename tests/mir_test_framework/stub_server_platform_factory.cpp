
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

#include "mir/shared_library.h"

#include "mir/geometry/rectangle.h"
#include "mir/graphics/display.h"

#include "mir_test_framework/executable_path.h"
#include "mir_test_framework/stub_server_platform_factory.h"
#include "mir_test_framework/fake_input_device.h"

#include <vector>

namespace geom = mir::geometry;
namespace mg = mir::graphics;
namespace mtf = mir_test_framework;

namespace
{
// NOTE: Raw pointer, deliberately leaked to bypass all the fun
//       issues around global destructor ordering.
mir::SharedLibrary* platform_graphics_lib{nullptr};
mir::SharedLibrary* platform_input_lib{nullptr};

void ensure_platform_library()
{
    if (!platform_graphics_lib)
    {
        platform_graphics_lib = new mir::SharedLibrary{mtf::server_platform("graphics-dummy")};
    }
    if (!platform_input_lib)
    {
        platform_input_lib = new mir::SharedLibrary{mtf::server_input_platform("input-stub")};
    }
}
}

std::shared_ptr<mg::DisplayPlatform> mtf::make_stubbed_display_platform(std::vector<geom::Rectangle> const& display_rects)
{
    ensure_platform_library();
    auto factory = platform_graphics_lib->load_function<std::shared_ptr<mg::DisplayPlatform>(*)(std::vector<geom::Rectangle> const&)>("create_stub_platform");

    return factory(display_rects);
}

auto mtf::make_stubbed_rendering_platform() -> std::shared_ptr<mg::RenderingPlatform>
{
    ensure_platform_library();

    auto factory = platform_graphics_lib->load_function<std::shared_ptr<mg::RenderingPlatform>(*)()>("create_stub_render_platform");

    return factory();
}

void mtf::set_next_display_rects(std::unique_ptr<std::vector<geom::Rectangle>>&& display_rects)
{
    ensure_platform_library();

    auto rect_setter = platform_graphics_lib->load_function<void(*)(std::unique_ptr<std::vector<geom::Rectangle>>&&)>("set_next_display_rects");

    rect_setter(std::move(display_rects));
}

void mtf::set_next_preset_display(std::unique_ptr<mir::graphics::Display> display)
{
    ensure_platform_library();

    auto display_setter = platform_graphics_lib->load_function<void(*)(std::unique_ptr<mir::graphics::Display>)>("set_next_preset_display");

    display_setter(std::move(display));
}

mir::UniqueModulePtr<mtf::FakeInputDevice> mtf::add_fake_input_device(mir::input::InputDeviceInfo const& info)
{
    ensure_platform_library();

    auto add_device = platform_input_lib->load_function<
        mir::UniqueModulePtr<mtf::FakeInputDevice>(*)(mir::input::InputDeviceInfo const&)
        >("add_fake_input_device");

    return add_device(info);
}
