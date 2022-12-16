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

#include "kiosk_window_manager.h"
#include "miral/append_event_filter.h"
#include "mir_toolkit/event.h"

#include <miral/runner.h>
#include <miral/application_authorizer.h>
#include <miral/display_configuration.h>
#include <miral/external_client.h>
#include <miral/command_line_option.h>
#include <miral/keymap.h>
#include <miral/set_window_management_policy.h>
#include <miral/internal_client.h>
#include <miral/wayland_extensions.h>

#include <unistd.h>
#include <atomic>

namespace
{
struct KioskAuthorizer : miral::ApplicationAuthorizer
{
    KioskAuthorizer(std::shared_ptr<SplashSession> const& splash) : splash{splash}{}

    virtual bool connection_is_allowed(miral::ApplicationCredentials const& creds) override
    {
        // Allow internal applications and (optionally) only ones that start "immediately"
        // (For the sake of an example "immediately" means while the spash is running)
        return getpid() == creds.pid() || !startup_only || splash->session();
    }

    virtual bool configure_display_is_allowed(miral::ApplicationCredentials const& /*creds*/) override
    {
        return false;
    }

    virtual bool set_base_display_configuration_is_allowed(miral::ApplicationCredentials const& /*creds*/) override
    {
        return false;
    }

    virtual bool screencast_is_allowed(miral::ApplicationCredentials const& /*creds*/) override
    {
        return true;
    }

    virtual bool prompt_session_is_allowed(miral::ApplicationCredentials const& /*creds*/) override
    {
        return false;
    }

    bool configure_input_is_allowed(miral::ApplicationCredentials const& /*creds*/) override
    {
        return false;
    }

    bool set_base_input_configuration_is_allowed(miral::ApplicationCredentials const& /*creds*/) override
    {
        return false;
    }

    static std::atomic<bool> startup_only;

    std::shared_ptr<SplashSession> splash;
};

std::atomic<bool> KioskAuthorizer::startup_only{false};
}

int main(int argc, char const* argv[])
{
    using namespace miral;

    SwSplash splash;

    ConfigurationOption startup_only{
        [&](bool startup_only) {KioskAuthorizer::startup_only = startup_only; },
        "kiosk-startup-apps-only",
        "Only allow applications to connect during startup",
        KioskAuthorizer::startup_only};

    ExternalClientLauncher launcher;

    auto run_startup_apps = [&](std::string const& apps)
    {
      for (auto i = begin(apps); i != end(apps); )
      {
          auto const j = find(i, end(apps), ':');
          launcher.launch(std::vector<std::string>{std::string{i, j}});
          if ((i = j) != end(apps)) ++i;
      }
    };

    MirRunner runner{argc, argv};

    DynamicDisplayConfiguration display_config{runner};

    auto const reload_display_config = [&](MirEvent const* event)
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
            case XKB_KEY_r:
            case XKB_KEY_R:display_config.reload();
                return true;

            default:
                return false;
            };
        };


    ConfigurationOption show_splash{
        [&](bool show_splash) {splash.enable(show_splash); },
        "show-splash",
        "Set to false to suppress display of splash on startup",
        true};

    WaylandExtensions wayland_extensions;

    for (auto const& extension : {"zwp_pointer_constraints_v1", "zwp_relative_pointer_manager_v1"})
        wayland_extensions.enable(extension);

    return runner.run_with(
        {
            display_config,
            display_config.layout_option(),
            set_window_management_policy<KioskWindowManagerPolicy>(splash),
            SetApplicationAuthorizer<KioskAuthorizer>{splash},
            Keymap{},
            show_splash,
            startup_only,
            launcher,
            wayland_extensions,
            ConfigurationOption{run_startup_apps, "startup-apps", "Colon separated list of startup apps", ""},
            StartupInternalClient{splash},
            AppendEventFilter{reload_display_config}
        });
}
