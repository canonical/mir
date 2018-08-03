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

#include "mir_test_framework/headless_test.h"
#include "mir_test_framework/stub_server_platform_factory.h"
#include "mir_test_framework/headless_display_buffer_compositor_factory.h"

#include "mir/shared_library.h"
#include "mir/geometry/rectangle.h"
#include "mir_test_framework/executable_path.h"

#include <boost/throw_exception.hpp>

namespace geom = mir::geometry;
namespace mtf = mir_test_framework;

mtf::HeadlessTest::HeadlessTest()
{
    add_to_environment("MIR_SERVER_PLATFORM_GRAPHICS_LIB", mtf::server_platform("graphics-dummy.so").c_str());
    add_to_environment("MIR_SERVER_PLATFORM_INPUT_LIB", mtf::server_platform("input-stub.so").c_str());
    add_to_environment("MIR_SERVER_ENABLE_KEY_REPEAT", "false");
    add_to_environment("MIR_SERVER_CONSOLE_PROVIDER", "none");
    server.override_the_display_buffer_compositor_factory([]
    {
        return std::make_shared<mtf::HeadlessDisplayBufferCompositorFactory>();
    });
}

mtf::HeadlessTest::~HeadlessTest() noexcept = default;

void mtf::HeadlessTest::preset_display(std::shared_ptr<mir::graphics::Display> const& display)
{
    mtf::set_next_preset_display(display);
}

void mtf::HeadlessTest::initial_display_layout(std::vector<geom::Rectangle> const& display_rects)
{
    mtf::set_next_display_rects(std::unique_ptr<std::vector<geom::Rectangle>>(new std::vector<geom::Rectangle>(display_rects)));
}
