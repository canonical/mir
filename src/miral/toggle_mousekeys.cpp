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

#include "miral/toggle_mousekeys.h"
#include "mir/options/configuration.h"

#include <mir/server.h>
#include <mir/shell/accessibility_manager.h>
#include <mir/log.h>

#include <memory>

struct miral::ToggleMouseKeys::Self
{
    Self(bool enabled_by_default) :
        enabled_by_default{enabled_by_default}
    {
    }

    std::weak_ptr<mir::shell::AccessibilityManager> accessibility_manager;
    bool const enabled_by_default;
};

miral::ToggleMouseKeys::ToggleMouseKeys(bool enabled_by_default) :
    self{std::make_shared<miral::ToggleMouseKeys::Self>(enabled_by_default)}
{
}

void miral::ToggleMouseKeys::toggle_mousekeys(bool enabled) const
{
    if (auto const am = self->accessibility_manager.lock())
        am->toggle_mousekeys(enabled);
    else
        mir::log_error("AccessibilityManager not initialized. Will not toggle mousekeys.");
}

void miral::ToggleMouseKeys::operator()(mir::Server& server) const
{
    server.add_init_callback(
        [this, self = this->self, &server]
        {
            self->accessibility_manager = server.the_accessibility_manager();
            auto options = server.get_options();

            auto enable = self->enabled_by_default;
            if (options->is_set(mir::options::enable_mouse_keys_opt))
                enable = options->get<bool>(mir::options::enable_mouse_keys_opt);

            toggle_mousekeys(enable);
        });
}
