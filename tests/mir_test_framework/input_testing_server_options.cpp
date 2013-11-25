/*
 * Copyright © 2013 Canonical Ltd.
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
 */

#include "mir_test_framework/input_testing_server_configuration.h"

#include "mir/input/input_channel.h"
#include "mir/scene/input_registrar.h"
#include "mir/input/surface.h"
#include "mir/shell/surface_creation_parameters.h"
#include "mir/frontend/shell.h"
#include "mir/frontend/session.h"
#include "mir/input/composite_event_filter.h"

#include "mir_test/fake_event_hub.h"
#include "mir_test/fake_event_hub_input_configuration.h"

#include <boost/throw_exception.hpp>

#include <functional>
#include <stdexcept>

namespace mtf = mir_test_framework;

namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace mi = mir::input;
namespace mia = mi::android;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;

mtf::InputTestingServerConfiguration::InputTestingServerConfiguration()
{
}

void mtf::InputTestingServerConfiguration::exec()
{
    input_injection_thread = std::thread(std::mem_fn(&mtf::InputTestingServerConfiguration::inject_input), this);
}

void mtf::InputTestingServerConfiguration::on_exit()
{
    input_injection_thread.join();
}

std::shared_ptr<mi::InputConfiguration> mtf::InputTestingServerConfiguration::the_input_configuration()
{
    if (!input_configuration)
    {
        std::shared_ptr<mi::CursorListener> null_cursor_listener{nullptr};

        input_configuration = std::make_shared<mtd::FakeEventHubInputConfiguration>(
            the_composite_event_filter(),
            the_input_region(),
            null_cursor_listener,
            the_input_report());
        fake_event_hub = input_configuration->the_fake_event_hub();

        fake_event_hub->synthesize_builtin_keyboard_added();
        fake_event_hub->synthesize_builtin_cursor_added();
        fake_event_hub->synthesize_device_scan_complete();
    }
    
    return input_configuration;
}
