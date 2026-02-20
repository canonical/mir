/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_TEST_WLCS_DISPLAY_SERVER_INTERNALS_H
#define MIR_TEST_WLCS_DISPLAY_SERVER_INTERNALS_H

#include <mir/test/signal.h>
#include <mir/fatal.h>
#include <mir/module_deleter.h>
#include <mir_test_framework/fake_input_device.h>
#include <miral/test_wlcs_display_server.h>

#include <wlcs/display_server.h>

#include <chrono>

auto constexpr a_long_time = std::chrono::seconds{5};

namespace
{

template<typename T>
void emit_mir_event(
    miral::TestWlcsDisplayServer* runner,
    mir::UniqueModulePtr<mir_test_framework::FakeInputDevice>& emitter,
    T event)
{
    auto event_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch());

    auto event_sent = runner->expect_event_with_time(event_time);

    emitter->emit_event(event.with_event_time(event_time));

    if (!event_sent->wait_for(a_long_time))
        mir::fatal_error("fake event failed to go through");
}

} // namespace

#endif // MIR_TEST_WLCS_DISPLAY_SERVER_INTERNALS_H
