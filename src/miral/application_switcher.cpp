/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "miral/application_switcher.h"

#include "mir/server.h"

namespace msh = mir::shell;

struct miral::ApplicationSwitcher::Self
{
    std::weak_ptr<msh::ApplicationSwitcher> app_switcher;
    bool is_activated = false;
    msh::ApplicationSwitcherState state = msh::ApplicationSwitcherState::all_forward;
};

miral::ApplicationSwitcher::ApplicationSwitcher()
    : self(std::make_shared<Self>())
{
}

void miral::ApplicationSwitcher::operator()(mir::Server& server) const
{
    server.add_init_callback([&]
    {
        self->app_switcher = server.the_application_switcher();
        if (self->is_activated)
            self->app_switcher.lock()->activate(self->state);
    });
}


void miral::ApplicationSwitcher::activate(msh::ApplicationSwitcherState state) const
{
    if (auto const locked = self->app_switcher.lock())
        locked->activate(state);
    else
    {
        self->is_activated = true;
        self->state = state;
    }
}

void miral::ApplicationSwitcher::deactivate() const
{
    if (auto const locked = self->app_switcher.lock())
        locked->deactivate();
    else
        self->is_activated = false;
}

void miral::ApplicationSwitcher::set_state(msh::ApplicationSwitcherState state) const
{
    if (auto const locked = self->app_switcher.lock())
        locked->set_state(state);
    else
        self->state = state;
}

void miral::ApplicationSwitcher::next() const
{
    if (auto const locked = self->app_switcher.lock())
        locked->next();
}


