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

class FakeDisplayConfigurationObserverRegistrar : public ObserverRegistrar<graphics::DisplayConfigurationObserver>
{
public:
    using Observer = graphics::DisplayConfigurationObserver;

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
    void register_early_observer(
        std::weak_ptr<Observer> const& obs,
        Executor&) override
    {
        register_interest(obs);
    }
    void unregister_interest(Observer const&) override
    {
        observer.reset();
    }
    void resize_output(std::size_t output_index, geometry::Size const& output_size)
    {
        output.outputs.at(output_index).modes[0].size = output_size;
        apply_config();
    }
    void disconnect_output(std::size_t output_index)
    {
        output.outputs.at(output_index).connected = false;
        apply_config();
    }

private:
    void apply_config()
    {
        auto o = observer.lock();
        o->configuration_applied(fake_shared(output));
    }

    StubDisplayConfig output{
        {
            geometry::Rectangle{{0, 0}, {100, 100}},
            geometry::Rectangle{{100, 0}, {100, 100}}
        }};

    std::weak_ptr<Observer> observer;
};

}
}
}

#endif // MIR_TEST_DOUBLES_FAKE_DISPLAY_CONFIGURATION_OBSERVER_REGISTRAR_H_
