/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir_toolkit/mir_client_library.h"

#include "mir/test/signal_actions.h"
#include "mir/test/spin_wait.h"
#include "mir/test/event_matchers.h"
#include "mir_test_framework/process.h"
#include "mir_test_framework/executable_path.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>
#include <chrono>
#include <thread>
#include <random>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <libevdev/libevdev-uinput.h>

namespace mt = mir::test;
namespace mtf = mir_test_framework;

using namespace std::literals::chrono_literals;

namespace
{

template <typename T> using UPtrWithDeleter = std::unique_ptr<T,void(*)(T*)>;
using MirConnectionUPtr = UPtrWithDeleter<MirConnection>;
using MirSurfaceUPtr = UPtrWithDeleter<MirWindow>;
using EvDevUPtr = UPtrWithDeleter<libevdev>;
using EvDevUInputUPtr = UPtrWithDeleter<libevdev_uinput>;

struct FakeInputDevice
{
    FakeInputDevice()
    {
        auto const dev_raw = libevdev_new();
        if (!dev_raw)
            throw std::runtime_error("Failed to create libevdev object");
        dev.reset(dev_raw);

        libevdev_set_name(dev.get(), "test device");
        libevdev_enable_event_type(dev.get(), EV_KEY);
        libevdev_enable_event_code(dev.get(), EV_KEY, KEY_A, nullptr);

        libevdev_uinput* uidev_raw{nullptr};
        auto err = libevdev_uinput_create_from_device(
            dev.get(), LIBEVDEV_UINPUT_OPEN_MANAGED, &uidev_raw);
        if (err != 0)
            throw std::runtime_error("Failed to create libevdev_uinput object");
        uidev.reset(uidev_raw);
    }

    void emit_event(int type, int code, int value)
    {
        auto const err = libevdev_uinput_write_event(uidev.get(), type, code, value);
        if (err != 0)
            throw std::runtime_error("Failed to write event to uidev");
    }

    void syn()
    {
        auto const err = libevdev_uinput_write_event(uidev.get(), EV_SYN, SYN_REPORT, 0);
        if (err != 0)
            throw std::runtime_error("Failed to write sync event to uidev");
    }

    EvDevUPtr dev{nullptr, libevdev_free};
    EvDevUInputUPtr uidev{nullptr, libevdev_uinput_destroy};
};

struct MockInputHandler
{
    MOCK_METHOD1(handle_input, void(MirEvent const*));
};

// Template string:
// X is replaced with random alphabetic character
// # is replaced with pid
std::string available_filename(std::string const& tmpl)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<char> random_char{'a','z'};

    std::string s;
    struct stat sb;

    do
    {
        s.clear();
        for (auto c : tmpl)
        {
            if (c == 'X')
                s += random_char(gen);
            else if (c == '#')
                s += std::to_string(getpid());
            else
                s += c;
        }
    }
    while (stat(s.c_str(), &sb) != -1);

    return s;
}

struct InputEvents : testing::Test
{
    std::string const host_socket{available_filename("/tmp/host_mir_socket_#_XXXXXX")};
    std::string const nested_socket{available_filename("/tmp/nested_mir_socket_#_XXXXXX")};
    std::string const server_path{mtf::executable_path() + "/mir_demo_server_minimal"};

    InputEvents()
    {
        host_server = mtf::fork_and_run_in_a_different_process(
            [this]
            {
                execlp(
                    server_path.c_str(),
                    server_path.c_str(),
                    "-f", host_socket.c_str(),
                    nullptr);
            },
            [] { return 0; });

        wait_for_socket(host_socket);

        nested_server = mtf::fork_and_run_in_a_different_process(
            [this]
            {
                execlp(
                    server_path.c_str(),
                    server_path.c_str(),
                    "-f", nested_socket.c_str(),
                    "--host-socket", host_socket.c_str(),
                    nullptr);
            },
            [] { return 0; });

        wait_for_socket(nested_socket);
    }

    static void handle_input(MirWindow*, MirEvent const* ev, void* context)
    {
        auto const handler = static_cast<MockInputHandler*>(context);
        auto const type = mir_event_get_type(ev);
        if (type == mir_event_type_input)
            handler->handle_input(ev);
    }

    void wait_for_socket(std::string const& path)
    {
        bool const success = mt::spin_wait_for_condition_or_timeout(
            [&]
            {
                struct stat sb;
                if (stat(path.c_str(), &sb) == -1)
                    return false;

                return S_ISSOCK(sb.st_mode);
            },
            std::chrono::seconds{5});

        if (!success)
            throw std::runtime_error("Timeout waiting for socket to appear");
    }

    void wait_for_surface_to_become_focused_and_exposed(MirWindow* window)
    {
        bool const success = mt::spin_wait_for_condition_or_timeout(
            [&]
            {
                return mir_window_get_visibility(window) == mir_window_visibility_exposed &&
                       mir_window_get_focus_state(window) == mir_window_focus_state_focused;
            },
            std::chrono::seconds{5});

        if (!success)
            throw std::runtime_error("Timeout waiting for window to become focused and exposed");
    }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    MirSurfaceUPtr create_surface_with_input_handler(
        MirConnection* connection,
        MockInputHandler* handler)
    {
        MirPixelFormat pixel_format;
        unsigned int valid_formats;
        mir_connection_get_available_surface_formats(connection, &pixel_format, 1, &valid_formats);
        auto spec = mir_create_normal_window_spec(connection, 640, 480);
        mir_window_spec_set_pixel_format(spec, pixel_format);
        mir_window_spec_set_event_handler(spec, handle_input, handler);
        auto const window = mir_create_window_sync(spec);
        mir_window_spec_release(spec);
        if (!mir_window_is_valid(window))
            throw std::runtime_error("Failed to create MirWindow");

        mir_buffer_stream_swap_buffers_sync(
            mir_window_get_buffer_stream(window));

        wait_for_surface_to_become_focused_and_exposed(window);

        return MirSurfaceUPtr(window, mir_window_release_sync);
    }
#pragma GCC diagnostic pop
    static int const key_down = 1;
    static int const key_up = 0;

    std::shared_ptr<mtf::Process> host_server;
    std::shared_ptr<mtf::Process> nested_server;
    FakeInputDevice fake_keyboard;
};

}

TEST_F(InputEvents, reach_host_client)
{
    using namespace testing;

    MockInputHandler handler;

    MirConnectionUPtr host_connection{
        mir_connect_sync(host_socket.c_str(), "test"),
        mir_connection_release};
    auto window = create_surface_with_input_handler(host_connection.get(), &handler);

    mt::Signal all_events_received;
    InSequence seq;
    EXPECT_CALL(handler,
                handle_input(AllOf(mt::KeyDownEvent(), mt::KeyOfSymbol(XKB_KEY_a))));
    EXPECT_CALL(handler,
                handle_input(AllOf(mt::KeyUpEvent(), mt::KeyOfSymbol(XKB_KEY_a))))
        .WillOnce(mt::WakeUp(&all_events_received));

    fake_keyboard.emit_event(EV_KEY, KEY_A, key_down);
    fake_keyboard.emit_event(EV_KEY, KEY_A, key_up);
    fake_keyboard.syn();

    all_events_received.wait_for(std::chrono::seconds{5});
}

TEST_F(InputEvents, DISABLED_reach_nested_client)
{
    using namespace testing;

    MockInputHandler handler;

    MirConnectionUPtr nested_connection{
        mir_connect_sync(nested_socket.c_str(), "test"),
        mir_connection_release};
    auto window = create_surface_with_input_handler(nested_connection.get(), &handler);

    // Give some time to the nested server to swap its framebuffer, so the nested
    // server window becomes visible and focused and can accept input events.
    // TODO: Find a more reliable way to wait for the nested server to gain focus
    std::this_thread::sleep_for(100ms);

    mt::Signal all_events_received;
    InSequence seq;
    EXPECT_CALL(handler,
                handle_input(AllOf(mt::KeyDownEvent(), mt::KeyOfSymbol(XKB_KEY_a))));
    EXPECT_CALL(handler,
                handle_input(AllOf(mt::KeyUpEvent(), mt::KeyOfSymbol(XKB_KEY_a))))
        .WillOnce(mt::WakeUp(&all_events_received));

    fake_keyboard.emit_event(EV_KEY, KEY_A, key_down);
    fake_keyboard.emit_event(EV_KEY, KEY_A, key_up);
    fake_keyboard.syn();

    all_events_received.wait_for(std::chrono::seconds{5});
}
