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

#ifndef MIR_TEST_FRAMEWORK_HEADLESS_TEST_H_
#define MIR_TEST_FRAMEWORK_HEADLESS_TEST_H_

#include "mir_test_framework/async_server_runner.h"

#include <gtest/gtest.h>


namespace mir { class SharedLibrary; }
namespace mir { namespace graphics { class Display; }}
namespace mir { namespace geometry { class Rectangle; }}

namespace mir_test_framework
{
/** Basic fixture for tests that don't use graphics or input hardware.
 *  This provides a mechanism for temporarily setting environment variables.
 *  It automatically sets "MIR_SERVER_PLATFORM_GRAPHICS_LIB" to "graphics-dummy.so"
 *  and MIR_SERVER_PLATFORM_INPUT_LIB to "input-stub.so"
 *  as the tests do not hit the graphics hardware.
 */
class HeadlessTest : public ::testing::Test, public AsyncServerRunner
{
public:
    HeadlessTest();
    ~HeadlessTest() noexcept;


    void preset_display(std::shared_ptr<mir::graphics::Display> const& display);

    /// Override initial display layout
    void initial_display_layout(std::vector<mir::geometry::Rectangle> const& display_rects);

private:
    std::unique_ptr<mir::SharedLibrary> server_platform_graphics_lib;
};

std::string const& test_socket_file();
}

#endif /* MIR_TEST_FRAMEWORK_HEADLESS_TEST_H_ */
