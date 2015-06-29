/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_TEST_FRAMEWORK_STUB_SERVER_PLATFORM_FACTORY_
#define MIR_TEST_FRAMEWORK_STUB_SERVER_PLATFORM_FACTORY_

#include "mir/geometry/rectangle.h"

#include "mir/graphics/platform.h"
#include "mir/module_deleter.h"
#include <vector>
#include <memory>
#include <string>

namespace geom = mir::geometry;

namespace mir
{
namespace graphics
{
class Platform;
}
namespace input
{
class InputDeviceInfo;
}
}

namespace mg = mir::graphics;

namespace mir_test_framework
{
class FakeInputDevice;

std::shared_ptr<mg::Platform> make_stubbed_server_graphics_platform(std::vector<geom::Rectangle> const& display_rects);

void set_next_display_rects(std::unique_ptr<std::vector<geom::Rectangle>>&& display_rects);

void set_next_preset_display(std::shared_ptr<mir::graphics::Display> const& display);

mir::UniqueModulePtr<FakeInputDevice> add_fake_input_device(mir::input::InputDeviceInfo const& info);
}
#endif /* MIR_TEST_FRAMEWORK_STUB_SERVER_PLATFORM_FACTORY_ */
