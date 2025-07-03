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


#ifndef MIR_SHELL_BASIC_SLOW_KEYS_TRANSFORMER_H
#define MIR_SHELL_BASIC_SLOW_KEYS_TRANSFORMER_H

#include "mir/shell/slow_keys_transformer.h"

#include "mir/input/input_event_transformer.h"
#include "mir/synchronised.h"
#include "mir/time/alarm.h"

namespace mir
{
class MainLoop;
namespace shell
{
class BasicSlowKeysTransformer : public SlowKeysTransformer
{
public:
    BasicSlowKeysTransformer(std::shared_ptr<MainLoop> const& main_loop);

    virtual bool transform_input_event(
        input::InputEventTransformer::EventDispatcher const&, input::EventBuilder*, MirEvent const&) override;

    void on_key_down(std::function<void(unsigned int)>&& okd) override;
    void on_key_rejected(std::function<void(unsigned int)>&& okr) override;
    void on_key_accepted(std::function<void(unsigned int)>&& oka) override;
    void delay(std::chrono::milliseconds) override;

private:
    std::shared_ptr<MainLoop> const main_loop;

    using KeysInFlight = std::unordered_map<unsigned int, std::unique_ptr<mir::time::Alarm>>;

    struct ConfigState
    {
        std::chrono::milliseconds delay{1000};
        std::function<void(unsigned int)> on_key_down{[](auto) {}};
        std::function<void(unsigned int)> on_key_rejected{[](auto) {}};
        std::function<void(unsigned int)> on_key_accepted{[](auto) {}};
    };

    mir::Synchronised<KeysInFlight> keys_in_flight;
    mir::Synchronised<ConfigState> config;
};
}
}

#endif // MIR_SHELL_BASIC_SLOW_KEYS_TRANSFORMER_H
