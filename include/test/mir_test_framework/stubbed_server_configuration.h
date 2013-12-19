/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_TEST_FRAMEWORK_STUBBED_SERVER_CONFIGURATION_H_
#define MIR_TEST_FRAMEWORK_STUBBED_SERVER_CONFIGURATION_H_

#include "mir/default_server_configuration.h"

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

    std::shared_ptr<graphics::Platform> the_graphics_platform();
    std::shared_ptr<compositor::RendererFactory> the_renderer_factory();
    // We override the_input_manager in the default server configuration
    // to avoid starting and stopping the full android input stack for tests
    // which do not leverage input.
    std::shared_ptr<input::InputConfiguration> the_input_configuration();

private:
    std::shared_ptr<graphics::Platform> graphics_platform;
};
}

#endif /* MIR_TEST_FRAMEWORK_STUBBED_SERVER_CONFIGURATION_H_ */
