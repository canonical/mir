/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_OPTIONS_CONFIGURATION_H_
#define MIR_OPTIONS_CONFIGURATION_H_

#include "mir/options/option.h"

#include <memory>

namespace mir
{
class SharedLibrary;

namespace options
{
extern char const* const arw_server_socket_opt;
extern char const* const enable_input_opt;
extern char const* const shared_library_prober_report_opt;
extern char const* const shell_report_opt;
extern char const* const compositor_report_opt;
extern char const* const display_report_opt;
extern char const* const scene_report_opt;
extern char const* const input_report_opt;
extern char const* const seat_report_opt;
extern char const* const touchspots_opt;
extern char const* const cursor_opt;
extern char const* const fatal_except_opt;
extern char const* const debug_opt;
extern char const* const composite_delay_opt;
extern char const* const enable_key_repeat_opt;
extern char const* const enable_mouse_keys_opt;
extern char const* const mouse_keys_acceleration_constant_factor;
extern char const* const mouse_keys_acceleration_linear_factor;
extern char const* const mouse_keys_acceleration_quadratic_factor;
extern char const* const x11_display_opt;
extern char const* const x11_scale_opt;
extern char const* const wayland_extensions_opt;
extern char const* const add_wayland_extensions_opt;
extern char const* const drop_wayland_extensions_opt;
extern char const* const idle_timeout_opt;
extern char const* const idle_timeout_when_locked_opt;

extern char const* const enable_key_repeat_opt;

extern char const* const off_opt_value;
extern char const* const log_opt_value;
extern char const* const lttng_opt_value;

extern char const* const platform_display_libs;
extern char const* const platform_rendering_libs;
extern char const* const platform_input_lib;
extern char const* const platform_path;

extern char const* const console_provider;
extern char const* const logind_console;
extern char const* const vt_console;
extern char const* const null_console;
extern char const* const auto_console;

extern char const* const vt_option_name;

class Configuration
{
public:
    /**
     * The options not associated with a specific loaded module
     */
    virtual std::shared_ptr<options::Option> global_options() const = 0;
    /**
     * All options, including those added by the specified module
     */
    virtual auto options_for(SharedLibrary const& module) const -> std::shared_ptr<options::Option> = 0;

protected:
    Configuration() = default;
    virtual ~Configuration() = default;

    Configuration(Configuration const&) = delete;
    Configuration& operator=(Configuration const&) = delete;
};
}
}

#endif /* MIR_OPTIONS_CONFIGURATION_H_ */
