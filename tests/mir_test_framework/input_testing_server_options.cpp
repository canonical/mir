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
#include "mir/input/surface.h"
#include "mir/scene/surface_creation_parameters.h"
#include "mir/frontend/session.h"
#include "mir/input/composite_event_filter.h"

#include "mir/test/wait_condition.h"

#include <boost/throw_exception.hpp>

#include <functional>
#include <stdexcept>

namespace mtf = mir_test_framework;
namespace mt = mir::test;

namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace mi = mir::input;
namespace ms = mir::shell;
namespace mia = mi::android;
namespace geom = mir::geometry;

mtf::InputTestingServerConfiguration::InputTestingServerConfiguration()
{
}

mtf::InputTestingServerConfiguration::InputTestingServerConfiguration(
    std::vector<geom::Rectangle> const& display_rects) :
    TestingServerConfiguration(display_rects)
{
}

void mtf::InputTestingServerConfiguration::on_start()
{
    auto const start_input_injection = std::make_shared<mt::WaitCondition>();

    input_injection_thread = std::thread{
        [this, start_input_injection]
        {
            // We need to wait for the 'input_injection_thread' variable to be
            // assigned to before starting. Otherwise we may end up calling
            // on_exit() before assignment and try to join an unjoinable thread.
            start_input_injection->wait_for_at_most_seconds(3);
            if (!start_input_injection->woken())
                BOOST_THROW_EXCEPTION(std::runtime_error("Input injection thread start signal timed out"));
            inject_input();
        }};

    start_input_injection->wake_up_everyone();
}

void mtf::InputTestingServerConfiguration::on_exit()
{
    input_injection_thread.join();
}

std::shared_ptr<ms::InputTargeter> mtf::InputTestingServerConfiguration::the_input_targeter()
{
    return DefaultServerConfiguration::the_input_targeter();
}

std::shared_ptr<mi::InputManager> mtf::InputTestingServerConfiguration::the_input_manager()
{
    return DefaultServerConfiguration::the_input_manager();
}

std::shared_ptr<mi::InputDispatcher> mtf::InputTestingServerConfiguration::the_input_dispatcher()
{
    return DefaultServerConfiguration::the_input_dispatcher();
}

std::shared_ptr<mi::InputSender> mtf::InputTestingServerConfiguration::the_input_sender()
{
    return DefaultServerConfiguration::the_input_sender();
}
