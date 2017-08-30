/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_TEST_FRAMEWORK_STUBBED_SERVER_CONFIGURATION_H_
#define MIR_TEST_FRAMEWORK_STUBBED_SERVER_CONFIGURATION_H_

#include "mir/default_server_configuration.h"
#include "mir/geometry/rectangle.h"

#include <vector>

namespace mir_test_framework
{
using namespace mir;

/**
 * stubs out the graphics and input subsystems to avoid tests
 * failing where the hardware is not accessible
 */
class StubbedServerConfiguration : public DefaultServerConfiguration
{
public:
    StubbedServerConfiguration();
    explicit StubbedServerConfiguration(std::vector<geometry::Rectangle> const& display_rects);
    ~StubbedServerConfiguration();

    std::shared_ptr<graphics::Platform> the_graphics_platform() override;
    std::shared_ptr<renderer::RendererFactory> the_renderer_factory() override;
    std::shared_ptr<input::InputManager> the_input_manager() override;
    std::shared_ptr<shell::InputTargeter> the_input_targeter() override;
    shell::WindowManagerBuilder the_window_manager_builder() override;

    std::shared_ptr<graphics::Cursor> the_cursor() override;
    std::shared_ptr<logging::Logger> the_logger() override;

private:
    std::shared_ptr<graphics::Platform> graphics_platform;
    std::vector<geometry::Rectangle> const display_rects;
};
}

#endif /* MIR_TEST_FRAMEWORK_STUBBED_SERVER_CONFIGURATION_H_ */
