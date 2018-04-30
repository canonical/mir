/*
 * Copyright © 2018 Canonical Ltd.
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
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <gio/gio.h>
#include <gio/gunixfdlist.h>
#include <cstdio>
#include <fcntl.h>
#include <boost/throw_exception.hpp>

#include "mir/anonymous_shm_file.h"
#include "mir_test_framework/executable_path.h"
#include "mir_test_framework/process.h"
#include "mir_test_framework/temporary_environment_value.h"
#include "mir/test/pipe.h"
#include "mir/test/signal.h"
#include "mir/test/doubles/mock_event_handler_register.h"

#include "src/server/console/logind_console_services.h"

namespace mtf = mir_test_framework;
namespace mtd = mir::test::doubles;

namespace
{
struct DBusDaemon
{
    std::string const address;
    std::shared_ptr<mtf::Process> process;
};

DBusDaemon spawn_bus_with_config(std::string const& config_path)
{
    mir::test::Pipe child_stdout;

    auto daemon = mtf::fork_and_run_in_a_different_process(
        [&config_path, out = child_stdout.write_fd()]()
        {
            if (::dup2(out, 1) < 0)
            {
                std::cerr << "Failed to redirect child output" << std::endl;
                return;
            }

            execlp(
                "dbus-daemon",
                "dbus-daemon",
                "--print-address",
                "--config-file",
                config_path.c_str(),
                (char*)nullptr);
        },
        []()
        {
            // The only way we call the exit function is if exec() fails.
            return EXIT_FAILURE;
        });

    char buffer[1024];
    auto bytes_read = ::read(child_stdout.read_fd(), buffer, sizeof(buffer));
    if (bytes_read < 0)
    {
        BOOST_THROW_EXCEPTION((
            std::system_error{
                errno,
                std::system_category(),
                "Failed to read address from dbus-daemon"}));
    }

    if (buffer[bytes_read - 1] != '\n')
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to read all of dbus address?"}));
    }
    buffer[bytes_read - 1] = '\0';
    return DBusDaemon {
        buffer,
        daemon
    };
}

class GLibTimeout
{
public:
    template<typename Period, typename Rep>
    GLibTimeout(std::chrono::duration<Period, Rep> timeout)
        : timed_out{false}
    {
        timer_source = g_timeout_add_seconds(
            std::chrono::duration_cast<std::chrono::seconds>(timeout).count(),
            [](gpointer ctx)
            {
                *static_cast<bool*>(ctx) = true;
                return static_cast<gboolean>(false);
            },
            &timed_out);
    }

    ~GLibTimeout()
    {
        if (!timed_out)
        {
            g_source_remove(timer_source);
        }
    }

    operator bool()
    {
        return timed_out;
    }
private:
    gint timer_source;
    bool timed_out;
};

enum class DeviceState
{
    Active,
    Suspended,
    Gone
};

// Define operator<< so we get more readable GTest failures
std::ostream& operator<<(std::ostream& out, DeviceState const& state)
{
    switch (state)
    {
    case DeviceState::Active:
        out << "Active";
        break;
    case DeviceState::Suspended:
        out << "Suspended";
        break;
    case DeviceState::Gone:
        out << "Gone";
        break;
    }
    return out;
}

}

class LogindConsoleServices : public testing::Test
{
public:
    LogindConsoleServices()
        : system_bus{spawn_bus_with_config(mtf::test_data_path() + "/dbus/system.conf")},
          session_bus{spawn_bus_with_config(mtf::test_data_path() + "/dbus/session.conf")},
          system_bus_env{"DBUS_SYSTEM_BUS_ADDRESS", system_bus.address.c_str()},
          session_bus_env{"DBUS_SESSION_BUS_ADDRESS", session_bus.address.c_str()},
          starter_bus_type_env{"DBUS_STARTER_BUS_TYPE", "session"},
          starter_bus_env{"DBUS_STARTER_BUS_ADDRESS", session_bus.address.c_str()},
          bus_connection{
              g_dbus_connection_new_for_address_sync(
                  system_bus.address.c_str(),
                  static_cast<GDBusConnectionFlags>(
                      G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION |
                      G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT),
                  nullptr,
                  nullptr,
                  nullptr),
              &g_object_unref}
    {
        if (!bus_connection)
        {
            BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to connect to mock System bus"}));
        }
    }

    void TearDown() override
    {
        // Print the dbusmock output if we've failed.
        auto const test_info = ::testing::UnitTest::GetInstance()->current_test_info();
        if (test_info->result()->Failed() && dbusmock)
        {
            char buffer[1024];
            ssize_t bytes_read;

            // Kill the dbusmock process so it closes its stdout
            dbusmock->terminate();

            std::cout << "Messages from DBusMock: " << std::endl;
            while ((bytes_read = ::read(mock_stdout, buffer, sizeof(buffer))) > 0)
            {
                ::write(STDOUT_FILENO, buffer, bytes_read);
            }
            if (bytes_read < 0)
            {
                std::cout << "Failed to read dbusmock output: "
                    << strerror(errno)
                    << "(" << errno << ")" << std::endl;
            }
        }
    }

    std::string add_any_active_session()
    {
        return add_session(
            "42",
            "seat0",
            1000,
            "test_user",
            true,
            "");
    }

    std::string add_session(
        char const* id,
        char const* seat,
        uint32_t uid,
        char const* username,
        bool active,
        char const* take_control_code)
    {
        auto result = std::unique_ptr<GVariant, decltype(&g_variant_unref)>{
            g_dbus_connection_call_sync(
                bus_connection.get(),
                "org.freedesktop.login1",
                "/org/freedesktop/login1",
                "org.freedesktop.DBus.Mock",
                "AddSession",
                g_variant_new(
                    "(ssusb)",
                    id,
                    seat,
                    uid,
                    username,
                    active),
                nullptr,
                G_DBUS_CALL_FLAGS_NO_AUTO_START,
                1000,
                nullptr,
                nullptr),
            &g_variant_unref};

        if (!result)
        {
            BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to add mock session"}));
        }

        // Result is a 1-tuple (session_object_path, )
        auto const session_path_variant = std::unique_ptr<GVariant, decltype(&g_variant_unref)>{
            g_variant_get_child_value(result.get(), 0),
            &g_variant_unref
        };
        auto const session_path =
            g_variant_get_string(session_path_variant.get(), nullptr);

        GError* error{nullptr};
        result.reset(
            g_dbus_connection_call_sync(
                bus_connection.get(),
                "org.freedesktop.login1",
                session_path,
                "org.freedesktop.DBus.Mock",
                "AddMethod",
                g_variant_new(
                    "(sssss)",
                    "org.freedesktop.login1.Session",
                    "TakeControl",
                    "b",
                    "",
                    take_control_code),
                nullptr,
                G_DBUS_CALL_FLAGS_NONE,
                1000,
                nullptr,
                &error));

        if (!result)
        {
            auto error_msg = error ? error->message : "Unknown error";
            BOOST_THROW_EXCEPTION((std::runtime_error{error_msg}));
        }

        /*
         * TODO: python-dbusmock unconditionally sets the seat's ActiveSession to
         * the last created session, regardless of whether the session is marked as
         * "active" or not.
         */
        return session_path;
    }

    void add_take_device_to_session(
        char const* session_path,
        char const* mock_code)
    {
        std::unique_ptr<GVariant, decltype(&g_variant_unref)> result{nullptr, &g_variant_unref};

        GError* error{nullptr};
        result.reset(
            g_dbus_connection_call_sync(
                bus_connection.get(),
                "org.freedesktop.login1",
                session_path,
                "org.freedesktop.DBus.Mock",
                "AddMethod",
                g_variant_new(
                    "(sssss)",
                    "org.freedesktop.login1.Session",
                    "TakeDevice",
                    "uu",
                    "hb",
                    mock_code),
                nullptr,
                G_DBUS_CALL_FLAGS_NONE,
                1000,
                nullptr,
                &error));

        if (!result)
        {
            auto error_msg = error ? error->message : "Unknown error";
            BOOST_THROW_EXCEPTION((std::runtime_error{error_msg}));
        }
    }

    void add_take_device_to_session(
        char const* session_path,
        int fd_to_return)
    {
        std::unique_ptr<GVariant, decltype(&g_variant_unref)> result{nullptr, &g_variant_unref};

        GError* error{nullptr};

        result.reset(
            g_dbus_connection_call_sync(
                bus_connection.get(),
                "org.freedesktop.login1",
                session_path,
                "org.freedesktop.DBus.Mock",
                "AddMethod",
                g_variant_new(
                    "(sssss)",
                    "org.freedesktop.DBus.Mock",
                    "SetTakeDeviceFd",
                    "h",
                    "",
                    "self.take_device_fd = args[0]"),
                nullptr,
                G_DBUS_CALL_FLAGS_NONE,
                1000,
                nullptr,
                &error));

        result.reset(
            g_dbus_connection_call_sync(
                bus_connection.get(),
                "org.freedesktop.login1",
                session_path,
                "org.freedesktop.DBus.Mock",
                "AddMethod",
                g_variant_new(
                    "(sssss)",
                    "org.freedesktop.login1.Session",
                    "TakeDevice",
                    "uu",
                    "hb",
                    "ret = (self.take_device_fd, False)"),
                nullptr,
                G_DBUS_CALL_FLAGS_NONE,
                1000,
                nullptr,
                &error));

        std::unique_ptr<GUnixFDList, decltype(&g_object_unref)> fd_list{
            g_unix_fd_list_new_from_array(&fd_to_return, 1),
            &g_object_unref};

        result.reset(
            g_dbus_connection_call_with_unix_fd_list_sync(
                bus_connection.get(),
                "org.freedesktop.login1",
                session_path,
                "org.freedesktop.DBus.Mock",
                "SetTakeDeviceFd",
                g_variant_new(
                    "(h)",
                    0),
                nullptr,
                G_DBUS_CALL_FLAGS_NONE,
                1000,
                fd_list.get(),
                nullptr,
                nullptr,
                &error));

        if (!result)
        {
            auto error_msg = error ? error->message : "Unknown error";
            BOOST_THROW_EXCEPTION((std::runtime_error{error_msg}));
        }
    }

    void ensure_mock_logind()
    {
        if (dbusmock)
            return;

        mir::test::Pipe stdout_pipe;
        mock_stdout = stdout_pipe.read_fd();

        dbusmock = mtf::fork_and_run_in_a_different_process(
            [stdout_fd = stdout_pipe.write_fd()]()
            {
                ::dup2(stdout_fd, 1);

                execlp(
                    "python3",
                    "python3",
                    "-m", "dbusmock",
                    "--template", "logind",
                    (char*)nullptr);
            },
            []()
            {
                return EXIT_FAILURE;
            });

        bool mock_on_bus{false};
        auto const watch_id = g_bus_watch_name(
            G_BUS_TYPE_SYSTEM,
            "org.freedesktop.login1",
            G_BUS_NAME_WATCHER_FLAGS_NONE,
            [](auto, auto, auto, gpointer ctx)
            {
                *static_cast<bool*>(ctx) = true;
            },
            [](auto, auto, auto)
            {
            },
            &mock_on_bus,
            nullptr);

        GLibTimeout timeout{std::chrono::seconds{30}};

        while (!mock_on_bus)
        {
            g_main_context_iteration(g_main_context_default(), true);
            if (timeout)
            {
                BOOST_THROW_EXCEPTION((std::runtime_error{"Timeout waiting for dbusmock to start"}));
            }
        }

        g_bus_unwatch_name(watch_id);
    }

    enum class SessionState {
        Online,
        Active,
        Closing
    };

    void set_logind_session_state(std::string const& session_path, SessionState state)
    {
        std::unique_ptr<GVariant, decltype(&g_variant_unref)> result{nullptr, &g_variant_unref};

        GError* error{nullptr};
        // Set the Active property appropriately
        result.reset(
            g_dbus_connection_call_sync(
                bus_connection.get(),
                "org.freedesktop.login1",
                session_path.c_str(),
                "org.freedesktop.DBus.Properties",
                "Set",
                g_variant_new(
                    "(ssv)",
                    "org.freedesktop.login1.Session",
                    "Active",
                    g_variant_new(
                        "b",
                        state == SessionState::Active)),
                nullptr,
                G_DBUS_CALL_FLAGS_NONE,
                1000,
                nullptr,
                &error));

        if (!result)
        {
            auto error_msg = error ? error->message : "Unknown error";
            BOOST_THROW_EXCEPTION((std::runtime_error{error_msg}));
        }

        auto const state_string =
            [state]()
            {
                switch(state)
                {
                case SessionState::Active:
                    return "active";
                case SessionState::Online:
                    return "online";
                case SessionState::Closing:
                    return "closing";
                }
                BOOST_THROW_EXCEPTION((std::logic_error{"GCC doesn't accept that the above switch is exhaustive"}));
            }();
        // Then set the State
        result.reset(
            g_dbus_connection_call_sync(
                bus_connection.get(),
                "org.freedesktop.login1",
                session_path.c_str(),
                "org.freedesktop.DBus.Properties",
                "Set",
                g_variant_new(
                    "(ssv)",
                    "org.freedesktop.login1.Session",
                    "State",
                    g_variant_new(
                        "s",
                        state_string)),
                nullptr,
                G_DBUS_CALL_FLAGS_NONE,
                1000,
                nullptr,
                &error));

        if (!result)
        {
            auto error_msg = error ? error->message : "Unknown error";
            BOOST_THROW_EXCEPTION((std::runtime_error{error_msg}));
        }
    }

    enum class SuspendType
    {
        Paused,
        Gone
    };

    void emit_device_suspended(char const* session_path, int major, int minor, SuspendType type)
    {
        std::unique_ptr<GVariant, decltype(&g_variant_unref)> result{nullptr, &g_variant_unref};

        auto const suspend_type_str =
            [type]()
            {
                switch (type)
                {
                    case SuspendType::Paused:
                        return "pause";
                    case SuspendType::Gone:
                        return "gone";
                }
                BOOST_THROW_EXCEPTION((std::logic_error{"GCC doesn't accept that the above switch is exhaustive"}));
            }();

        GError* error{nullptr};
        result.reset(
            g_dbus_connection_call_sync(
                bus_connection.get(),
                "org.freedesktop.login1",
                session_path,
                "org.freedesktop.DBus.Mock",
                "EmitSignal",
                g_variant_new(
                    "(sssv)",
                    "org.freedesktop.login1.Session",
                    "PauseDevice",
                    "uus",
                    g_variant_new(
                        "(uus)",
                        major,
                        minor,
                        suspend_type_str)),
                nullptr,
                G_DBUS_CALL_FLAGS_NONE,
                1000,
                nullptr,
                &error));

        if (!result)
        {
            BOOST_THROW_EXCEPTION((std::runtime_error{error->message}));
        }
    }

    void emit_device_activated(char const* session_path, int major, int minor, int fd)
    {
        std::unique_ptr<GVariant, decltype(&g_variant_unref)> result{nullptr, &g_variant_unref};

        std::unique_ptr<GUnixFDList, decltype(&g_object_unref)> fd_list{
            g_unix_fd_list_new(),
            &g_object_unref};

        GError* error{nullptr};

        int handle;
        if ((handle = g_unix_fd_list_append(fd_list.get(), fd, &error)) == -1)
        {
            BOOST_THROW_EXCEPTION((std::runtime_error{error->message}));
        }

        result.reset(
            g_dbus_connection_call_with_unix_fd_list_sync(
                bus_connection.get(),
                "org.freedesktop.login1",
                session_path,
                "org.freedesktop.DBus.Mock",
                "EmitSignal",
                g_variant_new(
                    "(sssv)",
                    "org.freedesktop.login1.Session",
                    "ResumeDevice",
                    "uuh",
                    g_variant_new(
                        "(uuh)",
                        major,
                        minor,
                        handle)),
                nullptr,
                G_DBUS_CALL_FLAGS_NONE,
                1000,
                fd_list.get(),
                nullptr,
                nullptr,
                &error));

        if (!result)
        {
            BOOST_THROW_EXCEPTION((std::runtime_error{error->message}));
        }
    }
private:
    DBusDaemon const system_bus;
    DBusDaemon const session_bus;
    mtf::TemporaryEnvironmentValue system_bus_env;
    mtf::TemporaryEnvironmentValue session_bus_env;
    mtf::TemporaryEnvironmentValue starter_bus_type_env;
    mtf::TemporaryEnvironmentValue starter_bus_env;
    std::shared_ptr<mtf::Process> dbusmock;
    std::unique_ptr<GDBusConnection, decltype(&g_object_unref)> bus_connection;
    mir::Fd mock_stdout;
};

using namespace testing;
using namespace std::literals::chrono_literals;

TEST_F(LogindConsoleServices, happy_path_succeeds)
{
    ensure_mock_logind();
    add_any_active_session();

    mir::LogindConsoleServices test{};
}

TEST_F(LogindConsoleServices, construction_fails_if_cannot_claim_control)
{
    ensure_mock_logind();

    add_session(
        "S3",
        "seat0",
        1001,
        "testy",
        true,
        "raise dbus.exceptions.DBusException('Device or resource busy (36)', name='System.Error.EBUSY')");

    EXPECT_THROW(
        mir::LogindConsoleServices test{};,
        std::runtime_error);
}

TEST_F(LogindConsoleServices, selects_active_session)
{
    ensure_mock_logind();

    add_session(
        "S3",
        "seat0",
        1001,
        "testy",
        false,
        "raise dbus.exceptions.DBusException('Device or resource busy (36)', name='System.Error.EBUSY')");

    // DBusMock will set the active session to the last-created one
    add_any_active_session();

    mir::LogindConsoleServices test{};
}

TEST_F(LogindConsoleServices, take_device_happy_path_resolves_to_fd)
{
    ensure_mock_logind();

    auto session_path = add_any_active_session();

    char const device_content[] =
        "Hello, my name is Chris";

    {
        mir::AnonymousShmFile fake_device_node(sizeof(device_content));
        ::memcpy(fake_device_node.base_ptr(), device_content, sizeof(device_content));

        add_take_device_to_session(
            session_path.c_str(),
            fake_device_node.fd());
    }

    mir::LogindConsoleServices services;

    mir::Fd resolved_fd;
    auto device = services.acquire_device(
        22, 33,
        [&resolved_fd](mir::Fd&& fd)
        {
            resolved_fd = std::move(fd);
        },
        [](){},
        [](){});

    GLibTimeout timeout{10s};
    while ((device.wait_for(0ms) == std::future_status::timeout) && !timeout)
    {
        g_main_context_iteration(g_main_context_default(), true);
    }
    ASSERT_THAT(device.wait_for(0ms), Eq(std::future_status::ready));
    EXPECT_THAT(resolved_fd, Not(Eq(mir::Fd::invalid)));
    device.get();

    auto const current_flags = fcntl(resolved_fd, F_GETFL);
    fcntl(resolved_fd, F_SETFL, current_flags | O_NONBLOCK);

    char buffer[sizeof(device_content)];
    auto read_bytes = read(resolved_fd, buffer, sizeof(buffer));
    EXPECT_THAT(read_bytes, Eq(sizeof(device_content)));
    EXPECT_THAT(buffer, StrEq(device_content));
}

TEST_F(LogindConsoleServices, take_device_calls_suspended_callback_when_initially_suspended)
{
    ensure_mock_logind();

    auto session_path = add_any_active_session();
    add_take_device_to_session(
        session_path.c_str(),
        "ret = (os.open('/dev/zero', os.O_RDONLY), True)");

    mir::LogindConsoleServices services;

    bool suspend_called{false};
    auto device = services.acquire_device(
        22, 33,
        [](mir::Fd&&)
        {
            FAIL() << "Unexpectedly called Active callback";
        },
        [&suspend_called](){ suspend_called = true; },
        [](){});

    GLibTimeout timeout{10s};
    while ((device.wait_for(0ms) == std::future_status::timeout) && !timeout)
    {
        g_main_context_iteration(g_main_context_default(), true);
    }
    ASSERT_THAT(device.wait_for(0ms), Eq(std::future_status::ready));
    EXPECT_TRUE(suspend_called);
    device.get();
}

TEST_F(LogindConsoleServices, take_device_resolves_to_exception_on_error)
{
    ensure_mock_logind();

    auto session_path = add_any_active_session();
    add_take_device_to_session(
        session_path.c_str(),
        "raise dbus.exceptions.DBusException('No such file or directory (2)', name='System.Error.ENOENT')");

    mir::LogindConsoleServices services;

    auto device = services.acquire_device(
        22, 33,
        [](auto){},
        [](){},
        [](){});

    GLibTimeout timeout{10s};
    while ((device.wait_for(0ms) == std::future_status::timeout) && !timeout)
    {
        g_main_context_iteration(g_main_context_default(), true);
    }

    EXPECT_THROW(
        device.get(),
        std::runtime_error
    );
}

TEST_F(LogindConsoleServices, device_activated_callback_called_on_activate)
{
    ensure_mock_logind();

    auto session_path = add_any_active_session();
    add_take_device_to_session(
        session_path.c_str(),
        "ret = (os.open('/dev/zero', os.O_RDONLY), True)");

    mir::LogindConsoleServices services;

    DeviceState state;

    mir::Fd received_fd;
    auto device = services.acquire_device(
        22, 33,
        [&state, &received_fd](mir::Fd&& device_fd)
        {
            received_fd = std::move(device_fd);
            state = DeviceState::Active;
        },
        [&state]()
        {
            state = DeviceState::Suspended;
        },
        [](){});

    {
        GLibTimeout timeout{10s};
        while ((device.wait_for(0ms) == std::future_status::timeout) && !timeout)
        {
            g_main_context_iteration(g_main_context_default(), true);
        }
    }

    ASSERT_THAT(device.wait_for(0ms), Eq(std::future_status::ready));
    ASSERT_THAT(state, Eq(DeviceState::Suspended));
    auto handle = device.get();

    char const device_content[] =
        "I am the very model of a modern major general";
    mir::AnonymousShmFile fake_device_node(sizeof(device_content));
    ::memcpy(fake_device_node.base_ptr(), device_content, sizeof(device_content));

    emit_device_activated(session_path.c_str(), 22, 33, fake_device_node.fd());

    {
        GLibTimeout timeout{10s};
        while (state != DeviceState::Active && !timeout)
        {
            g_main_context_iteration(g_main_context_default(), true);
        }
    }
    ASSERT_THAT(state, Eq(DeviceState::Active));

    auto const current_flags = fcntl(received_fd, F_GETFL);
    fcntl(received_fd, F_SETFL, current_flags | O_NONBLOCK);

    char buffer[sizeof(device_content)];
    auto read_bytes = read(received_fd, buffer, sizeof(buffer));
    EXPECT_THAT(read_bytes, Eq(sizeof(device_content)));
    EXPECT_THAT(buffer, StrEq(device_content));
}

TEST_F(LogindConsoleServices, device_suspended_callback_called_on_suspend)
{
    ensure_mock_logind();

    auto session_path = add_any_active_session();
    add_take_device_to_session(
        session_path.c_str(),
        "ret = (os.open('/dev/zero', os.O_RDONLY), False)");

    mir::LogindConsoleServices services;

    DeviceState state;

    auto device = services.acquire_device(
        22, 33,
        [&state](mir::Fd&&)
        {
            // We don't actually care about the fd.
            state = DeviceState::Active;
        },
        [&state]()
        {
            state = DeviceState::Suspended;
        },
        [](){});

    {
        GLibTimeout timeout{10s};
        while ((device.wait_for(0ms) == std::future_status::timeout) && !timeout)
        {
            g_main_context_iteration(g_main_context_default(), true);
        }
    }

    ASSERT_THAT(device.wait_for(0ms), Eq(std::future_status::ready));
    ASSERT_THAT(state, Eq(DeviceState::Active));
    auto handle = device.get();

    emit_device_suspended(session_path.c_str(), 22, 33, SuspendType::Paused);

    {
        GLibTimeout timeout{10s};
        while (state != DeviceState::Suspended && !timeout)
        {
            g_main_context_iteration(g_main_context_default(), true);
        }
    }
    EXPECT_THAT(state, Eq(DeviceState::Suspended));
}

TEST_F(LogindConsoleServices, device_removed_callback_called_on_remove)
{
    ensure_mock_logind();

    auto session_path = add_any_active_session();
    add_take_device_to_session(
        session_path.c_str(),
        "ret = (os.open('/dev/zero', os.O_RDONLY), False)");

    mir::LogindConsoleServices services;

    DeviceState state;

    auto device = services.acquire_device(
        22, 33,
        [&state](mir::Fd&&)
        {
            // We don't actually care about the fd.
            state = DeviceState::Active;
        },
        [&state]()
        {
            state = DeviceState::Suspended;
        },
        [&state]()
        {
            state = DeviceState::Gone;
        });

    {
        GLibTimeout timeout{10s};
        while ((device.wait_for(0ms) == std::future_status::timeout) && !timeout)
        {
            g_main_context_iteration(g_main_context_default(), true);
        }
    }

    ASSERT_THAT(device.wait_for(0ms), Eq(std::future_status::ready));
    ASSERT_THAT(state, Eq(DeviceState::Active));
    auto handle = device.get();

    emit_device_suspended(session_path.c_str(), 22, 33, SuspendType::Gone);

    {
        GLibTimeout timeout{10s};
        while (state != DeviceState::Gone && !timeout)
        {
            g_main_context_iteration(g_main_context_default(), true);
        }
    }
    EXPECT_THAT(state, Eq(DeviceState::Gone));
}

TEST_F(LogindConsoleServices, calls_pause_handler_on_pause)
{
    ensure_mock_logind();

    auto session_path = add_any_active_session();

    bool paused{false};
    testing::NiceMock<mtd::MockEventHandlerRegister> registrar;
    mir::LogindConsoleServices services;

    services.register_switch_handlers(
        registrar,
        [&paused]()
        {
            paused = true;
            return true;
        },
        [&paused]()
        {
            paused = false;
            return true;
        });

    set_logind_session_state(session_path, SessionState::Online);

    GLibTimeout timeout{std::chrono::seconds{10}};

    while (!paused && !timeout)
    {
        g_main_context_iteration(g_main_context_default(), true);
    }

    if (timeout)
    {
        FAIL() << "Timeout waiting for pause signal";
    }
}

TEST_F(LogindConsoleServices, calls_resume_handler_on_resume)
{
    ensure_mock_logind();

    auto session_path = add_any_active_session();

    bool paused{false};
    testing::NiceMock<mtd::MockEventHandlerRegister> registrar;
    mir::LogindConsoleServices services;

    services.register_switch_handlers(
        registrar,
        [&paused]()
        {
            paused = true;
            return true;
        },
        [&paused]()
        {
            paused = false;
            return true;
        });

    set_logind_session_state(session_path, SessionState::Online);

    GLibTimeout change_to_online_timeout{std::chrono::seconds{10}};

    while (!paused && !change_to_online_timeout)
    {
        g_main_context_iteration(g_main_context_default(), true);
    }

    if (change_to_online_timeout)
    {
        FAIL() << "Test precondition failure: Failed to switch-from the current session";
    }

    set_logind_session_state(session_path, SessionState::Active);

    GLibTimeout change_to_active_timeout{std::chrono::seconds{10}};

    while (paused && !change_to_active_timeout)
    {
        g_main_context_iteration(g_main_context_default(), true);
    }

    if (change_to_active_timeout)
    {
        FAIL() << "Timeout while waiting for resume signal";
    }
}

TEST_F(LogindConsoleServices, calls_pause_handler_on_closing_state)
{
    ensure_mock_logind();

    auto session_path = add_any_active_session();

    bool paused{false};
    testing::NiceMock<mtd::MockEventHandlerRegister> registrar;
    mir::LogindConsoleServices services;

    services.register_switch_handlers(
        registrar,
        [&paused]()
        {
            paused = true;
            return true;
        },
        [&paused]()
        {
            paused = false;
            return true;
        });

    set_logind_session_state(session_path, SessionState::Closing);

    GLibTimeout timeout{std::chrono::seconds{10}};

    while (!paused && !timeout)
    {
        g_main_context_iteration(g_main_context_default(), true);
    }

    if (timeout)
    {
        FAIL() << "Timeout waiting for pause signal";
    }
}

TEST_F(LogindConsoleServices, spurious_online_state_transitions_are_ignored)
{
    ensure_mock_logind();

    auto session_path = add_any_active_session();

    bool paused{false};
    testing::NiceMock<mtd::MockEventHandlerRegister> registrar;
    mir::LogindConsoleServices services;

    services.register_switch_handlers(
        registrar,
        [&paused]()
        {
            paused = true;
            return true;
        },
        [&paused]()
        {
            paused = false;
            return true;
        });

    set_logind_session_state(session_path, SessionState::Online);

    GLibTimeout switch_to_online_timeout{std::chrono::seconds{10}};

    while (!paused && !switch_to_online_timeout)
    {
        g_main_context_iteration(g_main_context_default(), true);
    }

    if (switch_to_online_timeout)
    {
        FAIL() << "Test precondition failure: Failed to switch-from the current session";
    }

    paused = false;
    set_logind_session_state(session_path, SessionState::Online);

    /*
     * We're waiting for something to *not* happen, so use a smaller timeout;
    * this can only false-pass, not false-fail.
    */
    GLibTimeout timeout{std::chrono::seconds{5}};

    while (!paused && !timeout)
    {
        g_main_context_iteration(g_main_context_default(), true);
    }

    EXPECT_FALSE(paused) << "Unexpectedly received pause notification";
}

TEST_F(LogindConsoleServices, online_to_closing_state_transition_is_ignored)
{
    ensure_mock_logind();

    auto session_path = add_any_active_session();

    bool paused{false};
    mtd::MockEventHandlerRegister registrar;
    mir::LogindConsoleServices services;

    services.register_switch_handlers(
        registrar,
        [&paused]()
        {
            paused = true;
            return true;
        },
        [&paused]()
        {
            paused = false;
            return true;
        });

    set_logind_session_state(session_path, SessionState::Online);

    GLibTimeout switch_to_online_timeout{std::chrono::seconds{10}};

    while (!paused && !switch_to_online_timeout)
    {
        g_main_context_iteration(g_main_context_default(), true);
    }

    if (switch_to_online_timeout)
    {
        FAIL() << "Test precondition failure: Failed to switch-from the current session";
    }

    paused = false;
    set_logind_session_state(session_path, SessionState::Closing);

    /*
     * We're waiting for something to *not* happen, so use a smaller timeout;
     * this can only false-pass, not false-fail.
     */
    GLibTimeout timeout{std::chrono::seconds{5}};

    while (!paused && !timeout)
    {
        g_main_context_iteration(g_main_context_default(), true);
    }

    EXPECT_FALSE(paused) << "Unexpectedly received pause notification";
}
