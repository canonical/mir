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

#include "mir/input/input_event_transformer.h"
#include "mir/time/types.h"
#include "mir/time/alarm.h"


namespace mir
{
class MainLoop;
namespace shell
{
class SlowKeysTransformer : public mir::input::InputEventTransformer::Transformer
{
public:
    virtual void enable() = 0;
    virtual void disable() = 0;

    virtual void on_enable(std::function<void()>&&) = 0;
    virtual void on_disable(std::function<void()>&&) = 0;
    virtual void on_key_down(std::function<void(unsigned int)>&&) = 0;
    virtual void on_key_rejected(std::function<void(unsigned int)>&&) = 0;
    // TODO: should this be called with repeats as well?
    virtual void on_key_accepted(std::function<void(unsigned int)>&&) = 0;

    virtual void delay(std::chrono::milliseconds) = 0;
};
class BasicSlowKeysTransformer : public SlowKeysTransformer
{
public:
    BasicSlowKeysTransformer(std::shared_ptr<MainLoop> const& main_loop);

    virtual bool transform_input_event(
        input::InputEventTransformer::EventDispatcher const&,
        input::EventBuilder*,
        MirEvent const&,
        MirInputDeviceId virtual_device_id) override;

    void enable() override {}
    void disable() override {}
    void on_enable(std::function<void()>&&) override {}
    void on_disable(std::function<void()>&&) override {}
    void on_key_down(std::function<void(unsigned int)>&& okd) override { on_key_down_ = std::move(okd); }
    void on_key_rejected(std::function<void(unsigned int)>&& okr) override { on_key_rejected_ = std::move(okr); }
    void on_key_accepted(std::function<void(unsigned int)>&& oka) override { on_key_accepted_ = std::move(oka); }
    void delay(std::chrono::milliseconds) override {}
private:
    std::shared_ptr<MainLoop> const main_loop;
    std::unordered_map<unsigned int, std::pair<std::unique_ptr<mir::time::Alarm>, bool>> keys_in_flight;

    std::chrono::milliseconds delay_{1000};
    std::function<void(unsigned int)> on_key_down_{[](auto){}}, on_key_rejected_{[](auto){}}, on_key_accepted_{[](auto){}};
};
}
}
