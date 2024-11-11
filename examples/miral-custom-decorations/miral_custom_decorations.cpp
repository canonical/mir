/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
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


#include "mir/server.h"
#include "mir/main_loop.h" // Needed for the conversion of `the_main_loop` to an executor

#include "floating_window_manager.h"
#include "miral/decoration/decoration_manager_adapter.h"
#include "miral/runner.h"
#include "miral/set_window_management_policy.h"
#include "user_decoration_manager_example.h"
#include <memory>
#include <miral/display_configuration_option.h>
#include <miral/external_client.h>
#include <miral/window_management_options.h>
#include <miral/append_event_filter.h>
#include <miral/internal_client.h>
#include <miral/command_line_option.h>
#include <miral/cursor_theme.h>
#include <miral/decorations.h>
#include <miral/keymap.h>
#include <miral/toolkit_event.h>
#include <miral/x11_support.h>
#include <miral/wayland_extensions.h>

#include <xkbcommon/xkbcommon-keysyms.h>

#include <cstring>

namespace
{
struct ConfigureDecorations
{
    miral::Decorations const decorations{[]
        {
            if (auto const strategy = getenv("MIRAL_SHELL_DECORATIONS"))
            {
                if (strcmp(strategy, "always-ssd") == 0) return miral::Decorations::always_ssd();
                if (strcmp(strategy, "prefer-ssd") == 0) return miral::Decorations::prefer_ssd();
                if (strcmp(strategy, "always-csd") == 0) return miral::Decorations::always_csd();
            }
            return miral::Decorations::prefer_csd();
        }()};

    void operator()(mir::Server& s) const
    {
        decorations(s);
    }
};

struct CustomDecorations
{
    CustomDecorations() = default;

    void operator()(mir::Server& s) const
    {
        s.add_pre_init_callback(
            [&s]
            {
                s.set_the_decoration_manager_init(
                    [&s]
                    {
                        auto custom_manager = std::make_shared<UserDecorationManagerExample>(
                            *s.the_display_configuration_observer_registrar(),
                            s.the_buffer_allocator(),
                            s.the_main_loop(),
                            s.the_cursor_images());

                        return miral::decoration::DecorationManagerBuilder::build(
                                   [custom_manager](auto shell)
                                   {
                                       custom_manager->init(shell);
                                   },
                                   [custom_manager](auto... args)
                                   {
                                       custom_manager->decorate(args...);
                                   },
                                   [custom_manager](auto... args)
                                   {
                                       custom_manager->undecorate(args...);
                                   },
                                   [custom_manager]()
                                   {
                                       custom_manager->undecorate_all();
                                   })
                            .done();
                    });
            });
    }
};
}

int main(int argc, char const* argv[])
{

    using namespace miral;
    using namespace miral::toolkit;

    std::function<void()> shutdown_hook{[]{}};

    InternalClientLauncher launcher;

    MirRunner runner{argc, argv};

    runner.add_stop_callback([&] { shutdown_hook(); });

    ExternalClientLauncher external_client_launcher;

    std::string terminal_cmd{"miral-terminal"};

    auto const quit_on_ctrl_alt_bksp = [&](MirEvent const* event)
        {
            if (mir_event_get_type(event) != mir_event_type_input)
                return false;

            MirInputEvent const* input_event = mir_event_get_input_event(event);
            if (mir_input_event_get_type(input_event) != mir_input_event_type_key)
                return false;

            MirKeyboardEvent const* kev = mir_input_event_get_keyboard_event(input_event);
            if (mir_keyboard_event_action(kev) != mir_keyboard_action_down)
                return false;

            MirInputEventModifiers mods = mir_keyboard_event_modifiers(kev);
            if (!(mods & mir_input_event_modifier_alt) || !(mods & mir_input_event_modifier_ctrl))
                return false;

            switch (mir_keyboard_event_keysym(kev))
            {
            case XKB_KEY_BackSpace:
                runner.stop();
                return true;

            case XKB_KEY_t:
            case XKB_KEY_T:
                external_client_launcher.launch({terminal_cmd});
                return false;

            case XKB_KEY_x:
            case XKB_KEY_X:
                external_client_launcher.launch_using_x11({"xterm"});
                return false;

            default:
                return false;
            };
        };

    Keymap config_keymap;

    return runner.run_with({
        CustomDecorations{},
        miral::display_configuration_options,
        miral::set_window_management_policy<miral::MinimalWindowManager>(),
        CursorTheme{"default:DMZ-White"},
        WaylandExtensions{},
        X11Support{},
        ConfigureDecorations{},
        display_configuration_options,
        external_client_launcher,
        launcher,
        config_keymap,
        AppendEventFilter{quit_on_ctrl_alt_bksp},
        ConfigurationOption{
            [&](std::string const& cmd)
            {
                terminal_cmd = cmd;
            },
            "shell-terminal-emulator",
            "terminal emulator to use",
            terminal_cmd},
        /* [&external_client_launcher](mir::Server& s) */
        /* { */
        /*     s.add_init_callback( */
        /*         [&external_client_launcher] */
        /*         { */
        /*             external_client_launcher.launch("xeyes"); */
        /*         }); */
        /* }, */
    });
}
