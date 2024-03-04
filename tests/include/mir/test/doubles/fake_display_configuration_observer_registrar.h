/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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

#ifndef MIR_TEST_DOUBLES_FAKE_DISPLAY_CONFIGURATION_OBSERVER_REGISTRAR_H_
#define MIR_TEST_DOUBLES_FAKE_DISPLAY_CONFIGURATION_OBSERVER_REGISTRAR_H_

#include "mir/executor.h"
#include "mir/graphics/display_configuration_observer.h"

#include "mir/test/doubles/stub_display_configuration.h"
#include "mir/test/doubles/stub_observer_registrar.h"
#include "mir/test/fake_shared.h"

namespace mir
{
namespace test
{
namespace doubles
{
struct FakeDisplayConfigurationObserverRegistrar : ObserverRegistrar<graphics::DisplayConfigurationObserver>
{
    using Observer = graphics::DisplayConfigurationObserver;
    StubDisplayConfig output{{geometry::Rectangle{{0, 0}, {100, 100}}}};
    void register_interest(std::weak_ptr<Observer> const& obs) override
    {
        register_interest(obs, immediate_executor);
    }
    void register_interest(
        std::weak_ptr<Observer> const& obs,
        Executor&) override
    {
        observer = obs;
        auto o = observer.lock();
        o->initial_configuration(fake_shared(output));
    }
    void unregister_interest(Observer const&) override
    {
        observer.reset();
    }
    void update_output(geometry::Size const& output_size)
    {
        output.outputs[0].modes[0].size = output_size;
        auto o = observer.lock();
        o->configuration_applied(fake_shared(output));
    }
    std::weak_ptr<Observer> observer;
};

}
}
}

#endif // MIR_TEST_DOUBLES_FAKE_DISPLAY_CONFIGURATION_OBSERVER_REGISTRAR_H_
