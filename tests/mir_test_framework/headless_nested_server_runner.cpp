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
 * Authored By: Robert Carr <robert.carr@canonical.com>
 */

#include "mir_test_framework/headless_nested_server_runner.h"
#include "mir_test_framework/executable_path.h"
#include "mir_test_framework/headless_display_buffer_compositor_factory.h"
#include "mir_test_framework/passthrough_tracker.h"

namespace mtf = mir_test_framework;

mtf::HeadlessNestedServerRunner::HeadlessNestedServerRunner(std::string const& connect_string) :
    passthrough_tracker(std::make_shared<mtf::PassthroughTracker>())
{
    add_to_environment("MIR_SERVER_PLATFORM_GRAPHICS_LIB", mtf::server_platform("graphics-dummy.so").c_str());
    add_to_environment("MIR_SERVER_HOST_SOCKET", connect_string.c_str());
    server.override_the_display_buffer_compositor_factory([this]
    {
        return std::make_shared<mtf::HeadlessDisplayBufferCompositorFactory>(passthrough_tracker);
    });
}
