/*
 * Copyright © 2016 Canonical Ltd.
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
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "active_outputs.h"
#include "display_configuration_listeners.h"

#include <mir/server.h>

#include <mir/observer_registrar.h>

void miral::ActiveOutputsListener::advise_output_begin() {}
void miral::ActiveOutputsListener::advise_output_end() {}
void miral::ActiveOutputsListener::advise_output_create(Output const& /*output*/) {}
void miral::ActiveOutputsListener::advise_output_update(Output const& /*updated*/, Output const& /*original*/) {}
void miral::ActiveOutputsListener::advise_output_delete(Output const& /*output*/) {}

struct miral::ActiveOutputsMonitor::Self : DisplayConfigurationListeners
{
};

miral::ActiveOutputsMonitor::ActiveOutputsMonitor() :
    self{std::make_shared<Self>()}
{
}

miral::ActiveOutputsMonitor::~ActiveOutputsMonitor() = default;
miral::ActiveOutputsMonitor::ActiveOutputsMonitor(ActiveOutputsMonitor const&) = default;
miral::ActiveOutputsMonitor& miral::ActiveOutputsMonitor::operator=(ActiveOutputsMonitor const&) = default;

void miral::ActiveOutputsMonitor::add_listener(ActiveOutputsListener* listener)
{
    self->add_listener(listener);
}

void miral::ActiveOutputsMonitor::delete_listener(ActiveOutputsListener* listener)
{
    self->delete_listener(listener);
}

void miral::ActiveOutputsMonitor::operator()(mir::Server& server)
{
    server.add_pre_init_callback([this, &server]
        { server.the_display_configuration_observer_registrar()->register_interest(self); });
}

void miral::ActiveOutputsMonitor::process_outputs(
    std::function<void(std::vector<Output> const& outputs)> const& functor) const
{
    self->process_active_outputs(functor);
}
