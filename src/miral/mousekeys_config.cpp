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


char const* const enable_mouse_keys_opt = "enable-mouse-keys";
char const* const mouse_keys_acceleration_constant_factor = "mouse-keys-acceleration-constant-factor";
char const* const mouse_keys_acceleration_linear_factor = "mouse-keys-acceleration-linear-factor";
char const* const mouse_keys_acceleration_quadratic_factor = "mouse-keys-acceleration-quadratic-factor";
char const* const mouse_keys_max_speed_x = "mouse-keys-max-speed-x";
char const* const mouse_keys_max_speed_y = "mouse-keys-max-speed-y";

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
    if(self->accessibility_manager.expired())
        mir::log_error("AccessibilityManager not initialized. Will not toggle mousekeys.");

    self->accessibility_manager.lock()->set_mousekeys_enabled(enabled);
}

void miral::MouseKeysConfig::set_keymap(mir::input::MouseKeysKeymap const& new_keymap) const
{
    if (self->accessibility_manager.expired())
        mir::log_error("AccessibilityManager not initialized. Will not update keymap.");

    self->accessibility_manager.lock()->set_mousekeys_keymap(new_keymap);
}

void miral::MouseKeysConfig::set_acceleration_factors(double constant, double linear, double quadratic) const
{
    if (self->accessibility_manager.expired())
        mir::log_error("AccessibilityManager not initialized. Will not set acceleration factors.");

    self->accessibility_manager.lock()->set_acceleration_factors(constant, linear, quadratic);
}


void miral::MouseKeysConfig::set_max_speed(double x_axis, double y_axis) const
{
    if (self->accessibility_manager.expired())
        mir::log_error("AccessibilityManager not initialized. Will not set max speed.");

    self->accessibility_manager.lock()->set_max_speed(x_axis, y_axis);
}

void miral::MouseKeysConfig::operator()(mir::Server& server) const
{
    server.add_configuration_option(
        enable_mouse_keys_opt, "Enable mousekeys (controlling the mouse with the numpad)", mir::OptionType::boolean);
    server.add_configuration_option(
        mouse_keys_acceleration_constant_factor, "The base speed for mousekey pointer motion", 100.0);
    server.add_configuration_option(
        mouse_keys_acceleration_linear_factor, "The linear speed increase for mousekey pointer motion", 100.0);
    server.add_configuration_option(
        mouse_keys_acceleration_quadratic_factor, "The quadratic speed increase for mousekey pointer motion", 30.0);
    server.add_configuration_option(
        mouse_keys_max_speed_x,
        "The maximum speed in pixels/second the mousekeys pointer can "
        "reach on the x axis. Pass zero to disable the speed limit",
        400.0);
    server.add_configuration_option(
        mouse_keys_max_speed_y,
        "The maximum speed in pixels/second the mousekeys pointer can "
        "reach on the y axis. Pass zero to disable the speed limit",
        400.0);

    server.add_init_callback(
        [this, self = this->self, &server]
        {
            self->accessibility_manager = server.the_accessibility_manager();
            auto options = server.get_options();

            auto enable = self->enabled_by_default;

            // Give the config option higher precedence
            if (options->is_set(enable_mouse_keys_opt))
                enable = options->get<bool>(enable_mouse_keys_opt);

            set_acceleration_factors(
                options->get<double>(mouse_keys_acceleration_constant_factor),
                options->get<double>(mouse_keys_acceleration_linear_factor),
                options->get<double>(mouse_keys_acceleration_quadratic_factor));
            set_max_speed(options->get<double>(mouse_keys_max_speed_x), options->get<double>(mouse_keys_max_speed_y));
            set_mousekeys_enabled(enable);
        });
}
