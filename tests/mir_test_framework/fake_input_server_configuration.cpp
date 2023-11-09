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

#include "mir_test_framework/fake_input_server_configuration.h"

namespace mtf = mir_test_framework;
namespace mi = mir::input;
namespace ms = mir::scene;
namespace msh = mir::shell;

namespace
{
std::vector<std::unique_ptr<mtf::TemporaryEnvironmentValue>> make_test_environment()
{
    std::vector<std::unique_ptr<mtf::TemporaryEnvironmentValue>> environment;

    environment.emplace_back(
        std::make_unique<mtf::TemporaryEnvironmentValue>(
            "MIR_SERVER_PLATFORM_INPUT_LIB",
            "mir:stub-input"));

    return environment;
}
}

mtf::FakeInputServerConfiguration::FakeInputServerConfiguration()
    : FakeInputServerConfiguration({mir::geometry::Rectangle{{0,0},{1600,1600}}})
{
}

mtf::FakeInputServerConfiguration::FakeInputServerConfiguration(std::vector<mir::geometry::Rectangle> const& display_rects)
    : TestingServerConfiguration(display_rects, make_test_environment())
{
}

mtf::FakeInputServerConfiguration::~FakeInputServerConfiguration() = default;


std::shared_ptr<mi::InputManager> mtf::FakeInputServerConfiguration::the_input_manager()
{
    return DefaultServerConfiguration::the_input_manager();
}

std::shared_ptr<msh::InputTargeter> mtf::FakeInputServerConfiguration::the_input_targeter()
{
    return DefaultServerConfiguration::the_input_targeter();
}

std::shared_ptr<mi::InputDispatcher> mtf::FakeInputServerConfiguration::the_input_dispatcher()
{
    return DefaultServerConfiguration::the_input_dispatcher();
}
