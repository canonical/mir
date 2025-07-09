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
#include "miral/live_config.h"

#include "mir/shell/mousekeys_transformer.h"
#include "mir/options/configuration.h"
#include "mir/synchronised.h"

#include <mir/server.h>
#include <mir/shell/accessibility_manager.h>
#include <mir/log.h>

#include <memory>


namespace
{
char const* const enable_mouse_keys_opt = "enable-mouse-keys";
char const* const mouse_keys_acceleration_constant_factor = "mouse-keys-acceleration-constant-factor";
char const* const mouse_keys_acceleration_linear_factor = "mouse-keys-acceleration-linear-factor";
char const* const mouse_keys_acceleration_quadratic_factor = "mouse-keys-acceleration-quadratic-factor";
char const* const mouse_keys_max_speed_x = "mouse-keys-max-speed-x";
char const* const mouse_keys_max_speed_y = "mouse-keys-max-speed-y";

namespace defaults
{
auto constexpr enable = false;
auto constexpr acceleration_constant = 100.0;
auto constexpr acceleration_linear = 100.0;
auto constexpr acceleration_quadratic = 30.0;
auto constexpr max_speed_x = 400.0;
auto constexpr max_speed_y = 400.0;
}

template <typename T>
auto maybe_option(std::shared_ptr<mir::options::Option> const& options, char const* const option_name) -> std::optional<T>
{
    return options->is_set(option_name) ? options->get<T>(option_name) : std::optional<T>{};
}

template <typename T> auto current_value_or_default(std::optional<T> current_value, T default_value) -> T
{
    if (current_value)
        return *current_value;

    return default_value;
}

template <typename T>
auto option_or_current_value_or_default(
    std::optional<T> option_value, std::optional<T> current_value, T const& default_value) -> T
{
    if(option_value)
        return *option_value;

    return current_value_or_default(current_value, default_value);
}
}

struct miral::MouseKeysConfig::Self
{
    explicit Self(bool enabled) :
        state{State{enabled}}
    {
    }

    struct State 
    {
        explicit State(bool enabled) :
            enabled{enabled}
        {
        }

        bool enabled{false};
        std::optional<double> acceleration_constant;
        std::optional<double> acceleration_linear;
        std::optional<double> acceleration_quadratic;
        std::optional<double> max_speed_x;
        std::optional<double> max_speed_y;
        std::optional<mir::input::MouseKeysKeymap> keymap;
    };

    std::weak_ptr<mir::shell::AccessibilityManager> accessibility_manager;
    mir::Synchronised<State> state;
};

miral::MouseKeysConfig::MouseKeysConfig(bool enabled_by_default) :
    self{std::make_shared<miral::MouseKeysConfig::Self>(enabled_by_default)}
{
}

miral::MouseKeysConfig::MouseKeysConfig(std::shared_ptr<Self> self) :
    self{std::move(self)}
{
}

auto miral::MouseKeysConfig::enabled() -> MouseKeysConfig 
{
    return MouseKeysConfig{std::make_shared<Self>(true)};
}

auto miral::MouseKeysConfig::disabled() -> MouseKeysConfig 
{
    return MouseKeysConfig{std::make_shared<Self>(false)};
}

miral::MouseKeysConfig::MouseKeysConfig(live_config::Store& config_store)
{
    config_store.add_bool_attribute(
        {"mouse_keys", "enable"},
        "Whether or not to start the compositor with mousekeys enabled",
        [this](live_config::Key const&, std::optional<bool> value)
        {
            if (!value) return;

            if (*value)
                this->enable();
            else
                this->disable();
             
        });

    auto const ensure_non_negative = [](live_config::Key const& key,
                                        std::optional<float> val,
                                        std::function<void(float)> do_something_with_non_negative_value)
    {
        if (!val)
            return;

        if (*val >= 0)
        {
            do_something_with_non_negative_value(*val);
        }
        else
        {
            mir::log_warning(
                "Config value %s does not support negative values. Ignoring the supplied value (%f)...",
                key.to_string().c_str(),
                *val);
        }
    };

    std::initializer_list<std::tuple<live_config::Key, std::string_view, std::optional<double> Self::State::*>>
        float_attribs = {
            {{"mouse_keys", "acceleration", "constant_factor"},
             "The base speed for mousekey pointer motion",
             &Self::State::acceleration_constant},
            {{"mouse_keys", "acceleration", "linear_factor"},
             "The linear speed increase for mousekey pointer motion",
             &Self::State::acceleration_linear},
            {{"mouse_keys", "acceleration", "quadratic_factor"},
             "The quadratic speed increase for mousekey pointer motion",
             &Self::State::acceleration_quadratic},
            {{"mouse_keys", "max_speed_x"},
             "The quadratic speed increase for mousekey pointer motion",
             &Self::State::max_speed_x},
            {{"mouse_keys", "max_speed_y"},
             "The quadratic speed increase for mousekey pointer motion",
             &Self::State::max_speed_y}};

    for (auto const& [key, desc, setter] : float_attribs)
    {
        config_store.add_float_attribute(
            key,
            desc,
            [ensure_non_negative, this, &setter](live_config::Key const& key, std::optional<float> val)
            {
                ensure_non_negative(
                    key,
                    val,
                    [this, &setter](float v)
                    {
                        auto s = self->state.lock();
                        *s.*setter = v;
                    });
            });
    }

    config_store.on_done(
        [this]
        {
            auto s = self->state.lock();
            this->set_acceleration_factors(
                current_value_or_default(s->acceleration_constant, defaults::acceleration_constant),
                current_value_or_default(s->acceleration_linear, defaults::acceleration_linear),
                current_value_or_default(s->acceleration_quadratic, defaults::acceleration_quadratic));

            this->set_max_speed(
                current_value_or_default(s->max_speed_x, defaults::max_speed_x),
                current_value_or_default(s->max_speed_y, defaults::max_speed_y));
        });
}

void miral::MouseKeysConfig::enabled(bool enabled) const
{
    if (auto const accessibility_manager = self->accessibility_manager.lock())
        accessibility_manager->mousekeys_enabled(enabled);
    else
        self->state.lock()->enabled = enabled;
    
}

void miral::MouseKeysConfig::enable() const
{
    if (auto const accessibility_manager = self->accessibility_manager.lock())
        accessibility_manager->mousekeys_enabled(true);
    else
        self->state.lock()->enabled = true;
}

void miral::MouseKeysConfig::disable() const
{
    if (auto const accessibility_manager = self->accessibility_manager.lock())
        accessibility_manager->mousekeys_enabled(false);
    else
        self->state.lock()->enabled = false;
}

void miral::MouseKeysConfig::set_keymap(mir::input::MouseKeysKeymap const& new_keymap) const
{
    if (auto const accessibility_manager = self->accessibility_manager.lock())
        accessibility_manager->mousekeys().keymap(new_keymap);
    else
        self->state.lock()->keymap = new_keymap;
}

void miral::MouseKeysConfig::set_acceleration_factors(double constant, double linear, double quadratic) const
{

    if (auto const accessibility_manager = self->accessibility_manager.lock())
    {
        accessibility_manager->mousekeys().acceleration_factors(constant, linear, quadratic);
    }
    else
    {
        auto s = self->state.lock();
        s->acceleration_constant = constant;
        s->acceleration_linear = linear;
        s->acceleration_quadratic = quadratic;
    }
}

void miral::MouseKeysConfig::set_max_speed(double x_axis, double y_axis) const
{
    if (auto const accessibility_manager = self->accessibility_manager.lock())
    {
        accessibility_manager->mousekeys().max_speed(x_axis, y_axis);
    }
    else
    {
        auto s = self->state.lock();
        s->max_speed_x = x_axis;
        s->max_speed_y = y_axis;
    }
}

void miral::MouseKeysConfig::operator()(mir::Server& server) const
{
    {
        auto s = self->state.lock();
        server.add_configuration_option(
            enable_mouse_keys_opt,
            "Enable mousekeys (controlling the mouse with the numpad)",
            mir::OptionType::boolean);
        server.add_configuration_option(
            mouse_keys_acceleration_constant_factor,
            "The base speed for mousekey pointer motion",
            current_value_or_default(s->acceleration_constant, defaults::acceleration_constant));
        server.add_configuration_option(
            mouse_keys_acceleration_linear_factor,
            "The linear speed increase for mousekey pointer motion",
            current_value_or_default(s->acceleration_linear, defaults::acceleration_linear));
        server.add_configuration_option(
            mouse_keys_acceleration_quadratic_factor,
            "The quadratic speed increase for mousekey pointer motion",
            current_value_or_default(s->acceleration_quadratic, defaults::acceleration_quadratic));
        server.add_configuration_option(
            mouse_keys_max_speed_x,
            "The maximum speed in pixels/second the mousekeys pointer can "
            "reach on the x axis. Pass zero to disable the speed limit",
            current_value_or_default(s->max_speed_x, defaults::max_speed_x));
        server.add_configuration_option(
            mouse_keys_max_speed_y,
            "The maximum speed in pixels/second the mousekeys pointer can "
            "reach on the y axis. Pass zero to disable the speed limit",
            current_value_or_default(s->max_speed_x, defaults::max_speed_x));
    }

    server.add_init_callback(
        [this, self = this->self, &server]
        {
            self->accessibility_manager = server.the_accessibility_manager();
            auto options = server.get_options();

            auto const state = self->state.lock();
            auto enable = option_or_current_value_or_default(
                maybe_option<bool>(options, enable_mouse_keys_opt), {state->enabled}, defaults::enable);

            if(enable)
                this->enable();

            // If a config (CLI or environment variable) option is set, then it
            // overrides both the state set options (until server init at
            // least) and defaults. Otherwise, state set options override
            // defaults. Defaults are used only if neither are provided.
            auto& mk = server.the_accessibility_manager()->mousekeys();
            {
                float acceleration_constant = option_or_current_value_or_default(
                    maybe_option<double>(options, mouse_keys_acceleration_constant_factor),
                    state->acceleration_constant,
                    defaults::acceleration_constant);
                float acceleration_linear = option_or_current_value_or_default(
                    maybe_option<double>(options, mouse_keys_acceleration_linear_factor),
                    state->acceleration_linear,
                    defaults::acceleration_linear);
                float acceleration_quadratic = option_or_current_value_or_default(
                    maybe_option<double>(options, mouse_keys_acceleration_quadratic_factor),
                    state->acceleration_quadratic,
                    defaults::acceleration_quadratic);
                mk.acceleration_factors(acceleration_constant, acceleration_linear, acceleration_quadratic);
            }

            {
                float max_speed_x = option_or_current_value_or_default(
                    maybe_option<double>(options, mouse_keys_max_speed_x), state->max_speed_x, defaults::max_speed_x);
                float max_speed_y = option_or_current_value_or_default(
                    maybe_option<double>(options, mouse_keys_max_speed_y), state->max_speed_y, defaults::max_speed_y);
                mk.max_speed(max_speed_x, max_speed_y);
            }

            if (state->keymap)
                mk.keymap(*state->keymap);
        });
}
