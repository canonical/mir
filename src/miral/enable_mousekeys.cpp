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

#include "miral/enable_mousekeys.h"

#include "mir/input/composite_event_filter.h"
#include "mir/input/event_filter.h"
#include "mir/options/configuration.h"
#include "mir/server.h"
#include "mir/shell/accessibility_manager.h"
#include "mir_toolkit/events/enums.h"
#include "mir_toolkit/events/input/keyboard_event.h"
#include <memory>

struct MouseKeysFilter : public mir::input::EventFilter
{
    bool handle(const MirEvent &event) override
    {
        if(mir_event_get_type(&event) != mir_event_type_input)
            return false;

        auto const* input_event = mir_event_get_input_event(&event);
        if(mir_input_event_get_type(input_event) != mir_input_event_type_key)
            return false;


        auto const* key_event = mir_input_event_get_keyboard_event(input_event);
        auto const modifiers = mir_keyboard_event_modifiers(key_event);

        if ((modifiers & mir_input_event_modifier_ctrl) && (modifiers & mir_input_event_modifier_shift) &&
            (modifiers & mir_input_event_modifier_num_lock))
        {
            if (mir_keyboard_event_action(key_event) == mir_keyboard_action_down ||
                mir_keyboard_event_action(key_event) == mir_keyboard_action_repeat)
                return true;

            mousekeys_on = !mousekeys_on;
            accessibility_manager->toggle_mousekeys(mousekeys_on);
            return true;
        }

        return false;
    }

    bool mousekeys_on;
    std::shared_ptr<mir::shell::AccessibilityManager> accessibility_manager;
};

struct miral::EnableMouseKeys::Self
{
    Self() :
        filter{std::make_shared<MouseKeysFilter>()}
    {
    }

    std::shared_ptr<MouseKeysFilter> const filter;
};

miral::EnableMouseKeys::EnableMouseKeys() :
    self{std::make_unique<miral::EnableMouseKeys::Self>()}
{
}

void miral::EnableMouseKeys::operator()(mir::Server& server) const
{
    server.add_init_callback(
        [filter = self->filter, &server]
        {
            filter->mousekeys_on = server.get_options()->get<bool>(mir::options::enable_mouse_keys_opt);
            filter->accessibility_manager = server.the_accessibility_manager();
            server.the_composite_event_filter()->append(filter);
        });
}
