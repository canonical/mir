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
 */

#include "mir_display.h"

#include <mir/frontend/display_changer.h>

#include <mir/graphics/display_configuration.h>
#include <mir/graphics/display_configuration_observer.h>

#include <algorithm>
#include <mutex>
#include <vector>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace mg = mir::graphics;

struct mf::MirDisplay::Self
{
    std::shared_ptr<DisplayChanger> const changer;
    std::shared_ptr<ObserverRegistrar<mg::DisplayConfigurationObserver>> const registrar;

    Self(
        std::shared_ptr<DisplayChanger> const& changer,
        std::shared_ptr<ObserverRegistrar<mg::DisplayConfigurationObserver>> const& registrar) :
        changer{changer}, registrar{registrar} {}
};

mf::MirDisplay::MirDisplay(
    std::shared_ptr<DisplayChanger> const& changer,
    std::shared_ptr<ObserverRegistrar<mg::DisplayConfigurationObserver>> const& registrar) :
    self{std::make_unique<Self>(changer, registrar)}
{
}

mf::MirDisplay::~MirDisplay()
{
}

void mf::MirDisplay::register_interest(
    std::weak_ptr<graphics::DisplayConfigurationObserver> const& observer,
    Executor& executor)
{
    self->registrar->register_interest(observer, executor);
}

void mf::MirDisplay::unregister_interest(graphics::DisplayConfigurationObserver const& observer)
{
    self->registrar->unregister_interest(observer);
}

void mf::MirDisplay::for_each_output(std::function<void(mg::DisplayConfigurationOutput const&)> f) const
{
    // Not only is this a train-wreck, it also does not use the *current* configuration.
    // OTOH is replicates the functionality it replaces and "session" display configs
    // are only supported through the Mir client API. (For now.)
    self->changer->base_configuration()->for_each_output(f);
}
