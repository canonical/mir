/*
 * Copyright © 2013-2015 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 *              Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir_test_framework/fake_input_server_configuration.h"

namespace mtf = mir_test_framework;
namespace mi = mir::input;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mia = mir::input::android;

mtf::FakeInputServerConfiguration::FakeInputServerConfiguration()
{
}

mtf::FakeInputServerConfiguration::FakeInputServerConfiguration(std::vector<mir::geometry::Rectangle> const& display_rects)
    : TestingServerConfiguration(display_rects)
{
}

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

std::shared_ptr<mi::InputSender> mtf::FakeInputServerConfiguration::the_input_sender()
{
    return DefaultServerConfiguration::the_input_sender();
}

std::shared_ptr<mi::LegacyInputDispatchable> mtf::FakeInputServerConfiguration::the_legacy_input_dispatchable()
{
    return DefaultServerConfiguration::the_legacy_input_dispatchable();
}

