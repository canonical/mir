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

#ifndef MIR_SHELL_APPLICATION_SWITCHER_H
#define MIR_SHELL_APPLICATION_SWITCHER_H

#include "mir/observer_multiplexer.h"
#include "mir/observer_registrar.h"

namespace mir
{
namespace shell
{
enum class ApplicationSwitcherState
{
    all_forward,
    all_backward,
    within_forward,
    within_backward,
};

class ApplicationSwitcherObserver
{
public:
    virtual ~ApplicationSwitcherObserver() = default;
    virtual void activate(ApplicationSwitcherState state) = 0;
    virtual void deactivate() = 0;
    virtual void set_state(ApplicationSwitcherState state) = 0;
    virtual void next() = 0;
};

class ApplicationSwitcher : public ObserverMultiplexer<ApplicationSwitcherObserver>
{
public:
    explicit ApplicationSwitcher(Executor& executor)
        : ObserverMultiplexer(executor) {}
};
}
}

#endif //MIR_SHELL_APPLICATION_SWITCHER_H
