/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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

// FIXME: Not an example

#include <miral/cursor_theme.h>
#include <miral/display_configuration_option.h>
#include <miral/input_configuration.h>
#include <miral/keymap.h>
#include <miral/live_config.h>
#include <miral/live_config_ini_file.h>
#include <miral/minimal_window_manager.h>
#include <miral/config_file.h>
#include <miral/runner.h>
#include <miral/set_window_management_policy.h>
#include <miral/wayland_extensions.h>
#include <miral/x11_support.h>
#include <miral/cursor_scale.h>
#include <miral/output_filter.h>
#include <miral/bounce_keys.h>
#include <miral/slow_keys.h>
#include <miral/hover_click.h>

#include "mir/abnormal_exit.h"
#include "mir/main_loop.h"
#include "mir/options/option.h"
#include "mir/report_exception.h"
#include "mir/server.h"

#include <boost/exception/diagnostic_information.hpp>

#include <chrono>
#include <cstdlib>

#include "miral/sticky_keys.h"

int main(int argc, char const* argv[])
{
    using namespace miral;
    MirRunner runner{argc, argv};

    live_config::IniFile config_store;

    CursorScale cursor_scale{config_store};
    OutputFilter output_filter{config_store};
    InputConfiguration input_configuration{config_store};
    BounceKeys bounce_keys{config_store};
    SlowKeys slow_keys{config_store};
    StickyKeys sticky_keys{config_store};
    Keymap keymap{config_store};
    HoverClick hover_click{config_store};

    ConfigFile config_file{
        runner,
        "mir_demo_server.live-config",
        ConfigFile::Mode::reload_on_change,
        [&config_store](auto&... args){ config_store.load_file(args...); }};

    WaylandExtensions wayland_extensions;
    for (auto const& ext : wayland_extensions.all_supported())
        wayland_extensions.enable(ext);

    return runner.run_with({
        // example options for display layout, logging and timeout
        display_configuration_options,
        X11Support{},
        wayland_extensions,
        set_window_management_policy<MinimalWindowManager>(),
        CursorTheme{"default:DMZ-White"},
        output_filter,
        input_configuration,
        cursor_scale,
        bounce_keys,
        slow_keys,
        sticky_keys,
        hover_click,
        keymap,
    });
}
