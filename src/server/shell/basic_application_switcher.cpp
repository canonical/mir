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

#include "basic_application_switcher.h"

namespace msh = mir::shell;

msh::BasicApplicationSwitcher::BasicApplicationSwitcher(Executor& executor)
    : ApplicationSwitcher(executor)
{
}

void msh::BasicApplicationSwitcher::activate(ApplicationSwitcherState state)
{
    for_each_observer(&ApplicationSwitcherObserver::activate, state);
}

void msh::BasicApplicationSwitcher::deactivate()
{
    for_each_observer(&ApplicationSwitcherObserver::deactivate);
}

void msh::BasicApplicationSwitcher::set_state(ApplicationSwitcherState state)
{
    for_each_observer(&ApplicationSwitcherObserver::set_state, state);
}

void msh::BasicApplicationSwitcher::next()
{
    for_each_observer(&ApplicationSwitcherObserver::next);
}
