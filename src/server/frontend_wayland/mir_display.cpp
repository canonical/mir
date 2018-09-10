/*
 * Copyright © 2018 Canonical Ltd.
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

#include "mir_display.h"

#include <mir/frontend/display_changer.h>

#include <mir/graphics/display_configuration.h>
#include <mir/graphics/display_configuration_observer.h>

#include <algorithm>
#include <mutex>
#include <vector>

namespace mf = mir::frontend;
namespace mg = mir::graphics;

namespace 
{
struct DisplayConfigurationObserverAdapter : mg::DisplayConfigurationObserver
{
    std::weak_ptr<mf::OutputObserver> const wrapped;

    DisplayConfigurationObserverAdapter(std::weak_ptr<mf::OutputObserver> const& adaptee) : 
        wrapped{adaptee}
    {
    }

    void initial_configuration(std::shared_ptr<mg::DisplayConfiguration const> const&) override
    {
    }

    void configuration_applied(std::shared_ptr<mg::DisplayConfiguration const> const& config) override
    {
        if (auto const adaptee = wrapped.lock()) 
            adaptee->handle_configuration_change(*config);
    }

    void base_configuration_updated(std::shared_ptr<mg::DisplayConfiguration const> const&) override
    {
    }

    void session_configuration_applied(
        std::shared_ptr<mf::Session> const&, std::shared_ptr<mg::DisplayConfiguration> const&) override
    {
    }

    void session_configuration_removed(std::shared_ptr<mir::frontend::Session> const&) override
    {
    }

    void configuration_failed(
        std::shared_ptr<mg::DisplayConfiguration const> const&,
        std::exception const&) override
    {
    }

    void catastrophic_configuration_error(
        std::shared_ptr<mg::DisplayConfiguration const> const&,
        std::exception const&) override
    {
    }
};
}

struct mf::MirDisplay::Self
{
    std::shared_ptr<DisplayChanger> const changer;
    std::shared_ptr<ObserverRegistrar<mg::DisplayConfigurationObserver>> const registrar;

    Self(
        std::shared_ptr<DisplayChanger> const& changer,
        std::shared_ptr<ObserverRegistrar<mg::DisplayConfigurationObserver>> const& registrar) :
        changer{changer}, registrar{registrar} {}

    std::mutex mutable mutex;
    std::vector<std::shared_ptr<DisplayConfigurationObserverAdapter>> adapters;
};

mf::MirDisplay::MirDisplay(
    std::shared_ptr<DisplayChanger> const& changer,
    std::shared_ptr<ObserverRegistrar<mg::DisplayConfigurationObserver>> const& registrar) :
    self{std::make_unique<Self>(changer, registrar)}
{
}

//std::shared_ptr<mir::ObserverRegistrar<mg::DisplayConfigurationObserver>>
//mir::DefaultServerConfiguration::the_display_configuration_observer_registrar()

void mf::MirDisplay::register_interest(std::weak_ptr<OutputObserver> const& observer)
{
    auto const adapter = std::make_shared<DisplayConfigurationObserverAdapter>(observer);

    std::lock_guard<decltype(self->mutex)> lock{self->mutex};
    self->adapters.push_back(adapter);
    self->registrar->register_interest(adapter);
}

void mf::MirDisplay::unregister_interest(OutputObserver* observer)
{
    std::lock_guard<decltype(self->mutex)> lock{self->mutex};

    self->adapters.erase(
        std::remove_if(
            begin(self->adapters),
            end(self->adapters),
            [observer, registrar = self->registrar](auto const& adapter)
                {
                    auto const adaptee = adapter->wrapped.lock().get();

                    if (adaptee == nullptr)
                    {
                        return true;
                    }
                    else if (adaptee == observer)
                    {
                        registrar->unregister_interest(*adapter);
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }),
        end(self->adapters));
}

mf::MirDisplay::~MirDisplay()
{
    std::lock_guard<decltype(self->mutex)> lock{self->mutex};
    for (auto const& adapter : self->adapters)
    {
        if (adapter)
            self->registrar->unregister_interest(*adapter);
    }
}

void mf::MirDisplay::for_each_output(std::function<void(mg::DisplayConfigurationOutput const&)> f) const
{
    // Not only is this a train-wreck, it also does not use the *current* configuration.
    // OTOH is replicates the functionality it replaces and "session" display configs
    // are only supported through the Mir client API. (For now.)
    self->changer->base_configuration()->for_each_output(f);
}
