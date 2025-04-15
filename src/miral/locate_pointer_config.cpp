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

#include "miral/locate_pointer_config.h"

#include "mir/options/option.h"
#include "mir/server.h"
#include "mir/shell/accessibility_manager.h"

struct miral::LocatePointerConfig::Self
{
    Self(bool enable_by_default)
        : enable_by_default(enable_by_default)
    {
    }

    bool enable_by_default;
    std::chrono::milliseconds delay;
    std::function<void(float, float)> on_locate_pointer;

    std::weak_ptr<mir::shell::AccessibilityManager> accessibility_manager;
};

miral::LocatePointerConfig::LocatePointerConfig(bool enabled_by_default) :
    self(std::make_shared<Self>(enabled_by_default))
{
}

void miral::LocatePointerConfig::operator()(mir::Server& server) const
{
    constexpr auto* enable_locate_pointer_opt = "enable-locate-pointer";
    constexpr auto* locate_pointer_delay_opt = "locate-pointer-delay";

    server.add_configuration_option(enable_locate_pointer_opt, "Enable locate pointer", self->enable_by_default);
    server.add_configuration_option(locate_pointer_delay_opt, "Locate pointer delay in milliseconds", 1000);

    server.add_init_callback(
        [this, &server]
        {
            self->accessibility_manager = server.the_accessibility_manager();

            auto const options = server.get_options();
            enabled(options->get<bool>(enable_locate_pointer_opt));

            delay(std::chrono::milliseconds{options->get<int>(locate_pointer_delay_opt)});
            if(self->on_locate_pointer)
                on_locate_pointer(std::move(self->on_locate_pointer));
        });
}

void miral::LocatePointerConfig::delay(std::chrono::milliseconds delay) const
{
    // Cache in case the user calls this before the server is started.
    self->delay = delay;

    // Forward to the accessibility manager if it's available.
    if (auto const accessibility_manager = self->accessibility_manager.lock())
    {
        accessibility_manager->locate_pointer_delay(delay);
    }
}

void miral::LocatePointerConfig::on_locate_pointer(std::function<void(float x, float y)>&& on_locate_pointer) const
{
    self->on_locate_pointer = std::move(on_locate_pointer);
    if (auto const accessibility_manager = self->accessibility_manager.lock())
    {
        accessibility_manager->on_locate_pointer(std::move(self->on_locate_pointer));
    }
}

void miral::LocatePointerConfig::enabled(bool enabled) const
{
    self->enable_by_default = enabled;
    if (auto const accessibility_manager = self->accessibility_manager.lock())
    {
        accessibility_manager->locate_pointer_enabled(self->enable_by_default);
    }
}
