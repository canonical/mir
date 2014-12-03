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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir_test_framework/headless_test.h"

#include "mir/shared_library.h"
#include "mir/geometry/rectangle.h"

#include <boost/throw_exception.hpp>

namespace geom = mir::geometry;
namespace mtf = mir_test_framework;

namespace
{
const char* const mir_server_platform_graphics_lib = "MIR_SERVER_PLATFORM_GRAPHICS_LIB";

std::chrono::seconds const timeout{10};
}

mtf::HeadlessTest::HeadlessTest()
{
    add_to_environment(mir_server_platform_graphics_lib, "libmirplatformstub.so");
}

mtf::HeadlessTest::~HeadlessTest() noexcept = default;


void mtf::HeadlessTest::preset_display(std::shared_ptr<mir::graphics::Display> const& display)
{
    if (!server_platform_graphics_lib)
        server_platform_graphics_lib.reset(new mir::SharedLibrary{getenv(mir_server_platform_graphics_lib)});

    typedef void (*PresetDisplay)(std::shared_ptr<mir::graphics::Display> const&);

    auto const preset_display =
        server_platform_graphics_lib->load_function<PresetDisplay>("preset_display");

    preset_display(display);
}

void mtf::HeadlessTest::initial_display_layout(std::vector<geom::Rectangle> const& display_rects)
{
    if (!server_platform_graphics_lib)
        server_platform_graphics_lib.reset(new mir::SharedLibrary{getenv(mir_server_platform_graphics_lib)});

    typedef void (*SetDisplayRects)(std::unique_ptr<std::vector<geom::Rectangle>>&&);

    auto const set_display_rects =
        server_platform_graphics_lib->load_function<SetDisplayRects>("set_display_rects");

    set_display_rects(std::unique_ptr<std::vector<geom::Rectangle>>(new std::vector<geom::Rectangle>(display_rects)));
}
