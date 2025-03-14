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

#include "miral/mousekeys_config.h"
#include "mir/options/configuration.h"

#include <mir/server.h>
#include <mir/shell/accessibility_manager.h>
#include <mir/log.h>

#include <memory>

struct miral::MouseKeysConfig::Self
{
    Self(bool enabled_by_default) :
        enabled_by_default{enabled_by_default}
    {
    }

    std::weak_ptr<mir::shell::AccessibilityManager> accessibility_manager;
    bool const enabled_by_default;
};

miral::MouseKeysConfig::MouseKeysConfig(bool enabled_by_default) :
    self{std::make_shared<miral::MouseKeysConfig::Self>(enabled_by_default)}
{
}

void miral::MouseKeysConfig::set_mousekeys_enabled(bool enabled) const
{
    if (auto const am = self->accessibility_manager.lock())
        am->set_mousekeys_enabled(enabled);
    else
        mir::log_error("AccessibilityManager not initialized. Will not toggle mousekeys.");
}

void miral::MouseKeysConfig::set_keymap(mir::input::MouseKeysKeymap const& new_keymap) const
{
    if (auto const am = self->accessibility_manager.lock())
        am->set_mousekeys_keymap(new_keymap);
    else
        mir::log_error("AccessibilityManager not initialized. Will not update keymap.");
}


void miral::MouseKeysConfig::operator()(mir::Server& server) const
{
    server.add_init_callback(
        [this, self = this->self, &server]
        {
            self->accessibility_manager = server.the_accessibility_manager();
            auto options = server.get_options();

            auto enable = self->enabled_by_default;
            if (options->is_set(mir::options::enable_mouse_keys_opt))
                enable = options->get<bool>(mir::options::enable_mouse_keys_opt);

            set_mousekeys_enabled(enable);
        });
}
