/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 *              Thomas Guest <thomas.guest@canonical.com>
 */

#include "mir/frontend/application.h"

#include <cassert>

namespace mf = mir::frontend;

mf::Application::Application(std::shared_ptr<Communicator> communicator)
    : communicator(communicator)
{
    assert(communicator);
}

mf::Application::~Application()
{
}

void mf::Application::on_event(mi::Event* e)
{
    assert(e);
}

mf::Application::StateTransitionSignal& mf::Application::state_transition_signal()
{
    return state_transition;
}

void mf::Application::connect()
{
}

void mf::Application::disconnect()
{
}
