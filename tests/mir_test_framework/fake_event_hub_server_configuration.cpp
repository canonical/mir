/*
 * Copyright © 2013-2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 *              Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir_test_framework/fake_event_hub_server_configuration.h"

#include "mir_test/fake_event_hub.h"

namespace mtf = mir_test_framework;
namespace mi = mir::input;
namespace ms = mir::shell;
namespace mia = mir::input::android;

std::shared_ptr<mi::InputManager> mtf::FakeEventHubServerConfiguration::the_input_manager()
{
    return DefaultServerConfiguration::the_input_manager();
}

std::shared_ptr<ms::InputTargeter> mtf::FakeEventHubServerConfiguration::the_input_targeter()
{
    return DefaultServerConfiguration::the_input_targeter();
}

std::shared_ptr<mi::InputDispatcher> mtf::FakeEventHubServerConfiguration::the_input_dispatcher()
{
    return DefaultServerConfiguration::the_input_dispatcher();
}

std::shared_ptr<mi::InputSender> mtf::FakeEventHubServerConfiguration::the_input_sender()
{
    return DefaultServerConfiguration::the_input_sender();
}

std::shared_ptr<droidinput::EventHubInterface> mtf::FakeEventHubServerConfiguration::the_event_hub()
{
    if (!fake_event_hub)
    {
        fake_event_hub = std::make_shared<mia::FakeEventHub>();

        fake_event_hub->synthesize_builtin_keyboard_added();
        fake_event_hub->synthesize_builtin_cursor_added();
        fake_event_hub->synthesize_usb_touchscreen_added();
        fake_event_hub->synthesize_device_scan_complete();
    }

    return fake_event_hub;
}
