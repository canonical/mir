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

#include "miral/sticky_keys.h"
#include "miral/live_config.h"
#include "mir/server.h"
#include "mir/synchronised.h"
#include "mir/input/event_builder.h"
#include "mir/shell/accessibility_manager.h"
#include "mir/shell/sticky_keys_transformer.h"

namespace mi = mir::input;
namespace msh = mir::shell;

class miral::StickyKeys::Self
{
public:
    explicit Self(bool enabled)
        : state{State{enabled}}
    {
    }

    void set_accessibility_manager(std::shared_ptr<msh::AccessibilityManager> const& manager)
    {
        auto const locked_state = state.lock();
        accessibility_manager = manager;
        manager->sticky_keys_enabled(locked_state->enabled);
        manager->sticky_keys().should_disable_if_two_keys_are_pressed_together(
            locked_state->should_disable_if_two_keys_are_pressed_together);
        manager->sticky_keys().on_modifier_clicked(std::move(locked_state->on_modifier_clicked));
    }

    void enable()
    {
        auto const locked_state = state.lock();
        if (locked_state->enabled)
            return;

        locked_state->enabled = true;
        if (auto const manager = accessibility_manager.lock())
        {
            manager->sticky_keys_enabled(true);
        }
    }

    void disable()
    {
        auto const locked_state = state.lock();
        if (!locked_state->enabled)
            return;

        locked_state->enabled = false;
        if (auto const manager = accessibility_manager.lock())
        {
            manager->sticky_keys_enabled(false);
        }
    }

    void should_disable_if_two_keys_are_pressed_together(bool on)
    {
        if (auto const manager = accessibility_manager.lock())
        {
            manager->sticky_keys().should_disable_if_two_keys_are_pressed_together(on);
        }
        else
        {
            auto const locked_state = state.lock();
            locked_state->should_disable_if_two_keys_are_pressed_together = on;
        }
    }

    void on_modifier_clicked(std::function<void(int32_t)>&& callback)
    {
        if (auto const manager = accessibility_manager.lock())
        {
            manager->sticky_keys().on_modifier_clicked(std::move(callback));
        }
        else
        {
            auto const locked_state = state.lock();
            locked_state->on_modifier_clicked = std::move(callback);
        }
    }

private:
    struct State
    {
        explicit State(bool enabled)
            : enabled{enabled}
        {}

        bool enabled;
        bool should_disable_if_two_keys_are_pressed_together = false;
        std::function<void(int32_t)> on_modifier_clicked{[](int32_t) {}};
    };

    std::weak_ptr<msh::AccessibilityManager> accessibility_manager;
    mir::Synchronised<State> state;
};

auto miral::StickyKeys::enabled() -> StickyKeys
{
    return StickyKeys{std::make_shared<Self>(true)};
}

auto miral::StickyKeys::disabled() -> StickyKeys
{
    return StickyKeys{std::make_shared<Self>(false)};
}

miral::StickyKeys::StickyKeys(live_config::Store& config_store) :
    StickyKeys{std::make_shared<Self>(false)}
{
    config_store.add_bool_attribute(
        {"sticky_keys", "enable"},
        "Whether or not sticky keys are enabled",
        false,
        [self=self](live_config::Key const&, std::optional<bool> val)
        {
            if (val)
            {
                if (*val)
                    self->enable();
                else
                    self->disable();
            }
        });
    config_store.add_bool_attribute(
        {"sticky_keys", "disable_if_two_keys_are_pressed_together"},
        "When set to true, clicking two modifier keys are once will result "
        "in sticky keys being temporarily disabled until all keys are released",
        true,
        [self=self](live_config::Key const&, std::optional<bool> val)
        {
            if (val)
                self->should_disable_if_two_keys_are_pressed_together(*val);
        });
}

miral::StickyKeys::StickyKeys(std::shared_ptr<Self> self)
    : self{std::move(self)}
{}

void miral::StickyKeys::operator()(mir::Server& server)
{
    server.add_init_callback([this, &server]
    {
        self->set_accessibility_manager(server.the_accessibility_manager());
    });
}

miral::StickyKeys& miral::StickyKeys::enable()
{
    self->enable();
    return *this;
}

miral::StickyKeys& miral::StickyKeys::disable()
{
    self->disable();
    return *this;
}

miral::StickyKeys& miral::StickyKeys::should_disable_if_two_keys_are_pressed_together(bool on)
{
    self->should_disable_if_two_keys_are_pressed_together(on);
    return *this;
}

miral::StickyKeys& miral::StickyKeys::on_modifier_clicked(std::function<void(int32_t)>&& callback)
{
    self->on_modifier_clicked(std::move(callback));
    return *this;
}
