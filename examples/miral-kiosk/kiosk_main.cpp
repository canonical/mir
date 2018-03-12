/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * under the terms of the GNU General Public License version 2 or 3 as as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "kiosk_window_manager.h"

#include <miral/runner.h>
#include <miral/application_authorizer.h>
#include <miral/command_line_option.h>
#include <miral/keymap.h>
#include <miral/set_window_management_policy.h>
#include <miral/internal_client.h>

#include <unistd.h>
#include <atomic>

namespace
{
struct KioskAuthorizer : miral::ApplicationAuthorizer
{
    KioskAuthorizer(SwSplash const& splash) : splash{splash}{}

    virtual bool connection_is_allowed(miral::ApplicationCredentials const& creds) override
    {
        // Allow internal applications and (optionally) only ones that start "immediately"
        // (For the sake of an example "immediately" means while the spash is running)
        return getpid() == creds.pid() || !startup_only || splash.session().lock();
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

    SwSplash splash;
};

std::atomic<bool> KioskAuthorizer::startup_only{false};
}

int main(int argc, char const* argv[])
{
    using namespace miral;

    SwSplash splash;

    CommandLineOption startup_only{
        [&](bool startup_only) {KioskAuthorizer::startup_only = startup_only; },
        "kiosk-startup-apps-only",
        "Only allow applications to connect during startup",
        KioskAuthorizer::startup_only};

    return MirRunner{argc, argv}.run_with(
        {
            CommandLineOption{[&](std::string const& ) { },
                              "desktop_file_hint", "Ignored for Unity8 compatability", "miral-shell.desktop"},
            set_window_management_policy<KioskWindowManagerPolicy>(splash),
            SetApplicationAuthorizer<KioskAuthorizer>{splash},
            Keymap{},
            startup_only,
            StartupInternalClient{"Intro", splash}
        });
}
