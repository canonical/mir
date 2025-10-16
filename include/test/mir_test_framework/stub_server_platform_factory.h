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

#ifndef MIR_TEST_FRAMEWORK_STUB_SERVER_PLATFORM_FACTORY_
#define MIR_TEST_FRAMEWORK_STUB_SERVER_PLATFORM_FACTORY_

#include <mir/geometry/rectangle.h>

#include <mir/graphics/platform.h>
#include <mir/module_deleter.h>
#include <vector>
#include <memory>
#include <string>

namespace mir
{
namespace input
{
struct InputDeviceInfo;
}
}

namespace mir_test_framework
{
class FakeInputDevice;

auto make_stubbed_display_platform(std::vector<mir::geometry::Rectangle> const& display_rects)
    -> std::shared_ptr<mir::graphics::DisplayPlatform>;

auto make_stubbed_rendering_platform()
    -> std::shared_ptr<mir::graphics::RenderingPlatform>;

void set_next_display_rects(std::unique_ptr<std::vector<mir::geometry::Rectangle>>&& display_rects);

void set_next_preset_display(std::unique_ptr<mir::graphics::Display> display);

mir::UniqueModulePtr<FakeInputDevice> add_fake_input_device(mir::input::InputDeviceInfo const& info);
}
#endif /* MIR_TEST_FRAMEWORK_STUB_SERVER_PLATFORM_FACTORY_ */
