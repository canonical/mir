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

#include "mir/time/steady_clock.h"
#include "mir/glib_main_loop.h"
#include "mir/anonymous_shm_file.h"
#include "mir_test_framework/executable_path.h"
#include "mir_test_framework/process.h"
#include "mir_test_framework/temporary_environment_value.h"
#include "mir/test/pipe.h"
#include "mir/test/signal.h"
#include "mir/test/doubles/mock_event_handler_register.h"
#include "mir/test/auto_unblock_thread.h"
#include "mir/test/doubles/simple_device_observer.h"
#include "mir/test/doubles/null_device_observer.h"

#include "src/server/console/logind_console_services.h"

namespace mtf = mir_test_framework;
namespace mtd = mir::test::doubles;
namespace mt = mir::test;

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
              &g_object_unref},
          ml{std::make_shared<mir::GLibMainLoop>(std::make_shared<mir::time::SteadyClock>())},
          ml_thread{
              [this]() { ml->stop(); ml_thread_stopped = true; },
              [this]() { ml->run(); }}
    {
        if (!bus_connection)
        {
            BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to connect to mock System bus"}));
        }
    }

    void TearDown() override
    {
        // Tests cases *must* stop the mainloop thread before destroying
        // block scope objects that are referenced by enqueued logic.
        EXPECT_TRUE(ml_thread_stopped);

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
                [](auto){}(::write(STDOUT_FILENO, buffer, bytes_read));
            }
            if (bytes_read < 0)
            {
                std::cout << "Failed to read dbusmock output: "
                    << strerror(errno)
                    << "(" << errno << ")" << std::endl;
            }
            std::cout << "Errors from DBusMock: " << std::endl;
            while ((bytes_read = ::read(mock_stderr, buffer, sizeof(buffer))) > 0)
            {
                [](auto){}(::write(STDOUT_FILENO, buffer, bytes_read));
            }
            if (bytes_read < 0)
            {
                std::cout << "Failed to read dbusmock error output: "
                          << strerror(errno)
                          << "(" << errno << ")" << std::endl;
            }
        }
    }

    // Tests cases *must* stop the mainloop thread before destroying
    // block scope objects that are referenced by enqueued logic.
    void stop_mainloop()
    {
        ml_thread.stop();
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

    void add_seat(char const* name)
    {
        auto result = std::unique_ptr<GVariant, decltype(&g_variant_unref)>{
            g_dbus_connection_call_sync(
                bus_connection.get(),
                "org.freedesktop.login1",
                "/org/freedesktop/login1",
                "org.freedesktop.DBus.Mock",
                "AddSeat",
                g_variant_new(
                    "(s)",
                    name),
                nullptr,
                G_DBUS_CALL_FLAGS_NO_AUTO_START,
                1000,
                nullptr,
                nullptr),
            &g_variant_unref};

        if (!result)
        {
            BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to add Seat"}));
        }
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

    void add_release_device_to_session(
        char const* session_path,
        char const* mock_code = "")
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
                    "ReleaseDevice",
                    "uu",
                    "",
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

    void ensure_mock_logind()
    {
        if (dbusmock)
            return;

        mir::test::Pipe stdout_pipe;
        mir::test::Pipe stderr_pipe;
        mock_stdout = stdout_pipe.read_fd();
        mock_stderr = stderr_pipe.read_fd();

        dbusmock = mtf::fork_and_run_in_a_different_process(
            [
                stdout_fd = stdout_pipe.write_fd(),
                stderr_fd = stderr_pipe.write_fd()
            ]()
            {
                ::dup2(stdout_fd, 1);
                ::dup2(stderr_fd, 2);

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

    void add_switch_to_to_seat(
        char const* seat_path,
        char const* mock_code)
    {
        auto result = std::unique_ptr<GVariant, decltype(&g_variant_unref)>{
            g_dbus_connection_call_sync(
                bus_connection.get(),
                "org.freedesktop.login1",
                seat_path,
                "org.freedesktop.DBus.Mock",
                "AddMethod",
                g_variant_new(
                    "(sssss)",
                    "org.freedesktop.login1.Seat",
                    "SwitchTo",
                    "u",
                    "",
                    mock_code),
                nullptr,
                G_DBUS_CALL_FLAGS_NO_AUTO_START,
                1000,
                nullptr,
                nullptr),
            &g_variant_unref};

        if (!result)
        {
            BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to add SwitchTo"}));
        }
    }

    void add_pause_device_complete_to_session(
        char const* session_path,
        char const* mock_code = "")
    {
        auto result = std::unique_ptr<GVariant, decltype(&g_variant_unref)>{
            g_dbus_connection_call_sync(
                bus_connection.get(),
                "org.freedesktop.login1",
                session_path,
                "org.freedesktop.DBus.Mock",
                "AddMethod",
                g_variant_new(
                    "(sssss)",
                    "org.freedesktop.login1.Session",
                    "PauseDeviceComplete",
                    "uu",
                    "",
                    mock_code),
                nullptr,
                G_DBUS_CALL_FLAGS_NO_AUTO_START,
                1000,
                nullptr,
                nullptr),
            &g_variant_unref};

        if (!result)
        {
            BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to add PauseDeviceComplete"}));
        }
    }

    struct CallInfo
    {
        using GVariantPtr = std::unique_ptr<GVariant, decltype(&g_variant_unref)>;

        CallInfo(
            uint64_t timestamp,
            std::string method_name,
            GVariantIter* argument_array)
            : timestamp{timestamp},
              method_name{std::move(method_name)}
        {
            arguments.reserve(g_variant_iter_n_children(argument_array));

            GVariant* argument;
            while (g_variant_iter_next(argument_array, "v", &argument))
            {
                // We own a reference to argument, so we don't need to increase the refcount.
                arguments.emplace_back(GVariantPtr{argument, &g_variant_unref});
            }
        }

        uint64_t timestamp;
        std::string method_name;
        std::vector<GVariantPtr> arguments;
    };

    std::vector<CallInfo> get_calls_for_object(char const* object_path)
    {
        auto result = std::unique_ptr<GVariant, decltype(&g_variant_unref)>{
            g_dbus_connection_call_sync(
                bus_connection.get(),
                "org.freedesktop.login1",
                object_path,
                "org.freedesktop.DBus.Mock",
                "GetCalls",
                g_variant_new("()"),
                nullptr,
                G_DBUS_CALL_FLAGS_NO_AUTO_START,
                1000,
                nullptr,
                nullptr),
            &g_variant_unref};

        if (!result)
        {
            BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to query list of mock calls made"}));
        }

        auto variant_iter = g_variant_iter_new(g_variant_get_child_value(result.get(), 0));
        std::vector<CallInfo> calls;
        calls.reserve(g_variant_iter_n_children(variant_iter));

        uint64_t timestamp;
        char const* method_name;
        GVariantIter* arguments;
        while (g_variant_iter_next(variant_iter, "(tsav)", &timestamp, &method_name, &arguments))
        {
            calls.emplace_back(
                timestamp,
                method_name,
                arguments);
            g_variant_iter_free(arguments);
        }

        return calls;
    }

    std::future<CallInfo> expect_call(
        std::string object_path,
        std::string method_name)
    {
        auto promised_call = std::make_shared<std::promise<CallInfo>>();
        auto future_call = promised_call->get_future();

        ml->run_with_context_as_thread_default(
            [
                this,
                path = std::move(object_path),
                name = std::move(method_name),
                promised_call
            ]()
            {
                struct HandlerCtx
                {
                    std::remove_const<decltype(promised_call)>::type promise;
                    guint subscription_id;
                };

                auto context = new HandlerCtx;
                context->promise = promised_call;

                context->subscription_id = g_dbus_connection_signal_subscribe(
                    bus_connection.get(),
                    nullptr,
                    "org.freedesktop.DBus.Mock",
                    "MethodCalled",
                    path.c_str(),
                    name.c_str(),
                    G_DBUS_SIGNAL_FLAGS_NONE,
                    [](GDBusConnection* connection,
                       gchar const* /*sender_name*/,
                       gchar const* /*object_path*/,
                       gchar const* /*interface_name*/,
                       gchar const* /*signal_name*/,
                       GVariant* parameters,
                       gpointer ctx) noexcept
                    {
                        auto context = static_cast<HandlerCtx*>(ctx);

                        GVariant* argument_array;
                        char const* method_name;

                        g_variant_get(parameters, "(&s@av)", &method_name, &argument_array);
                        auto iter = g_variant_iter_new(argument_array);
                        g_variant_unref(argument_array);

                        context->promise->set_value(
                            CallInfo{
                                static_cast<uint64_t>(
                                    std::chrono::steady_clock::now().time_since_epoch().count()),
                                method_name,
                                iter
                            });

                        g_variant_iter_free(iter);

                        g_dbus_connection_signal_unsubscribe(
                            connection,
                            context->subscription_id);
                    },
                    context,
                    [](gpointer ctx)
                    {
                        delete static_cast<HandlerCtx*>(ctx);
                    });
            }).get(); // Make sure our signal handler is actually registered before continuing.
        return future_call;
    }

    template<typename T, typename R>
    void wait_for_dbus_sync_point(std::chrono::duration<T, R> wait_for)
    {
        // Add a method we can call…
        auto result = std::unique_ptr<GVariant, decltype(&g_variant_unref)>{
            g_dbus_connection_call_sync(
                bus_connection.get(),
                "org.freedesktop.login1",
                "/org/freedesktop/login1",
                "org.freedesktop.DBus.Mock",
                "AddMethod",
                g_variant_new(
                    "(sssss)",
                    "org.freedesktop.DBus.Mock",
                    "Syncpoint",
                    "",
                    "",
                    ""),
                nullptr,
                G_DBUS_CALL_FLAGS_NO_AUTO_START,
                1000,
                nullptr,
                nullptr),
            &g_variant_unref};

        // …subscribe to a notification for when it's called…
        auto syncpoint_called = expect_call("/org/freedesktop/login1", "Syncpoint");

        // …call it…
        result.reset(
            g_dbus_connection_call_sync(
                bus_connection.get(),
                "org.freedesktop.login1",
                "/org/freedesktop/login1",
                "org.freedesktop.DBus.Mock",
                "Syncpoint",
                nullptr,
                nullptr,
                G_DBUS_CALL_FLAGS_NO_AUTO_START,
                1000,
                nullptr,
                nullptr));

        /*
         * …now, because signals are handled in-order, we know that any
         * previous DBus call or signal will have been processed by the time
         * the Syncpoint-called signal is received
         */
        if (syncpoint_called.wait_for(wait_for) != std::future_status::ready)
        {
            BOOST_THROW_EXCEPTION((std::runtime_error{"Timeout waiting for syncpoint"}));
        }
    }

    std::shared_ptr<mir::GLibMainLoop> the_main_loop()
    {
        return ml;
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
    mir::Fd mock_stderr;

    std::shared_ptr<mir::GLibMainLoop> const ml;
    mt::AutoUnblockThread ml_thread;
    bool ml_thread_stopped = false;
};

using namespace testing;
using namespace std::literals::chrono_literals;

TEST_F(LogindConsoleServices, happy_path_succeeds)
{
    ensure_mock_logind();
    add_any_active_session();

    mir::LogindConsoleServices test{the_main_loop()};
    stop_mainloop();
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
        mir::LogindConsoleServices test{the_main_loop()};,
        std::runtime_error);
    stop_mainloop();
}

TEST_F(LogindConsoleServices, construction_fails_if_no_logind)
{
    EXPECT_THROW(
        mir::LogindConsoleServices test{the_main_loop()},
        std::runtime_error);
    stop_mainloop();
}

TEST_F(LogindConsoleServices, construction_fails_if_no_active_session)
{
    ensure_mock_logind();

    add_seat("seat0");

    EXPECT_THROW(
        mir::LogindConsoleServices test{the_main_loop()};,
        std::runtime_error);
    stop_mainloop();
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

    mir::LogindConsoleServices test{the_main_loop()};
    stop_mainloop();
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

    mir::LogindConsoleServices services{the_main_loop()};

    mir::Fd resolved_fd;
    auto device = services.acquire_device(
        22, 33,
        std::make_unique<mtd::SimpleDeviceObserver>(
            [&resolved_fd](mir::Fd&& fd)
            {
                resolved_fd = std::move(fd);
            },
            [](){},
            [](){}));

    ASSERT_THAT(device.wait_for(30s), Eq(std::future_status::ready));

    EXPECT_THAT(resolved_fd, Not(Eq(mir::Fd::invalid)));
    device.get();

    auto const current_flags = fcntl(resolved_fd, F_GETFL);
    fcntl(resolved_fd, F_SETFL, current_flags | O_NONBLOCK);

    char buffer[sizeof(device_content)];
    auto read_bytes = read(resolved_fd, buffer, sizeof(buffer));
    EXPECT_THAT(read_bytes, Eq(sizeof(device_content)));
    EXPECT_THAT(buffer, StrEq(device_content));
    stop_mainloop();
}

TEST_F(LogindConsoleServices, take_device_calls_suspended_callback_when_initially_suspended)
{
    ensure_mock_logind();

    auto session_path = add_any_active_session();
    add_take_device_to_session(
        session_path.c_str(),
        "ret = (os.open('/dev/zero', os.O_RDONLY), True)");

    mir::LogindConsoleServices services{the_main_loop()};

    bool suspend_called{false};
    auto device = services.acquire_device(
        22, 33,
        std::make_unique<mtd::SimpleDeviceObserver>(
            [](mir::Fd&&)
            {
                FAIL() << "Unexpectedly called Active callback";
            },
            [&suspend_called](){ suspend_called = true; },
            [](){}));

    ASSERT_THAT(device.wait_for(30s), Eq(std::future_status::ready));
    EXPECT_TRUE(suspend_called);
    device.get();
    stop_mainloop();
}

TEST_F(LogindConsoleServices, take_device_resolves_to_exception_on_error)
{
    ensure_mock_logind();

    auto session_path = add_any_active_session();
    add_take_device_to_session(
        session_path.c_str(),
        "raise dbus.exceptions.DBusException('No such file or directory (2)', name='System.Error.ENOENT')");

    mir::LogindConsoleServices services{the_main_loop()};

    auto device = services.acquire_device(
        22, 33,
        std::make_unique<mtd::SimpleDeviceObserver>(
            [](auto){},
            [](){},
            [](){}));

    ASSERT_THAT(device.wait_for(30s), Eq(std::future_status::ready));

    EXPECT_THROW(
        device.get(),
        std::runtime_error
    );
    stop_mainloop();
}

TEST_F(LogindConsoleServices, device_activated_callback_called_on_activate)
{
    ensure_mock_logind();

    auto session_path = add_any_active_session();
    add_take_device_to_session(
        session_path.c_str(),
        "ret = (os.open('/dev/zero', os.O_RDONLY), True)");

    mir::LogindConsoleServices services{the_main_loop()};

    DeviceState state;

    auto active = std::make_shared<mt::Signal>();
    mir::Fd received_fd;
    auto device = services.acquire_device(
        22, 33,
        std::make_unique<mtd::SimpleDeviceObserver>(
            [active, &received_fd](mir::Fd&& device_fd)
            {
                received_fd = std::move(device_fd);
                active->raise();
            },
            [&state]()
            {
                state = DeviceState::Suspended;
            },
            [](){}));

    ASSERT_THAT(device.wait_for(30s), Eq(std::future_status::ready));
    ASSERT_THAT(state, Eq(DeviceState::Suspended));
    auto handle = device.get();

    char const device_content[] =
        "I am the very model of a modern major general";
    mir::AnonymousShmFile fake_device_node(sizeof(device_content));
    ::memcpy(fake_device_node.base_ptr(), device_content, sizeof(device_content));

    emit_device_activated(session_path.c_str(), 22, 33, fake_device_node.fd());

    ASSERT_TRUE(active->wait_for(30s));

    auto const current_flags = fcntl(received_fd, F_GETFL);
    fcntl(received_fd, F_SETFL, current_flags | O_NONBLOCK);

    char buffer[sizeof(device_content)];
    auto read_bytes = read(received_fd, buffer, sizeof(buffer));
    EXPECT_THAT(read_bytes, Eq(sizeof(device_content)));
    EXPECT_THAT(buffer, StrEq(device_content));
    stop_mainloop();
}

TEST_F(LogindConsoleServices, device_suspended_callback_called_on_suspend)
{
    ensure_mock_logind();

    auto session_path = add_any_active_session();
    add_take_device_to_session(
        session_path.c_str(),
        "ret = (os.open('/dev/zero', os.O_RDONLY), False)");

    mir::LogindConsoleServices services{the_main_loop()};

    DeviceState state;
    auto suspended = std::make_shared<mt::Signal>();

    auto device = services.acquire_device(
        22, 33,
        std::make_unique<mtd::SimpleDeviceObserver>(
            [&state](mir::Fd&&)
            {
                // We don't actually care about the fd.
                state = DeviceState::Active;
            },
            [suspended]()
            {
                suspended->raise();
            },
            [](){}));

    ASSERT_THAT(device.wait_for(30s), Eq(std::future_status::ready));
    ASSERT_THAT(state, Eq(DeviceState::Active));
    auto handle = device.get();

    emit_device_suspended(session_path.c_str(), 22, 33, SuspendType::Paused);

    EXPECT_TRUE(suspended->wait_for(30s));
    stop_mainloop();
}

TEST_F(LogindConsoleServices, acks_device_suspend_signal)
{
    ensure_mock_logind();

    auto session_path = add_any_active_session();
    add_take_device_to_session(
        session_path.c_str(),
        "ret = (os.open('/dev/zero', os.O_RDONLY), False)");
    add_pause_device_complete_to_session(session_path.c_str());

    mir::LogindConsoleServices services{the_main_loop()};

    auto device = services.acquire_device(
        22, 33,
        std::make_unique<mtd::NullDeviceObserver>());

    ASSERT_THAT(device.wait_for(30s), Eq(std::future_status::ready));

    auto pause_ack_call = expect_call(
        session_path,
        "PauseDeviceComplete");

    emit_device_suspended(session_path.c_str(), 22, 33, SuspendType::Paused);

    ASSERT_THAT(pause_ack_call.wait_for(30s), Eq(std::future_status::ready));

    auto call_details = pause_ack_call.get();

    ASSERT_THAT(call_details.arguments, SizeIs(2));

    EXPECT_THAT(g_variant_get_uint32(call_details.arguments[0].get()), Eq(22));
    EXPECT_THAT(g_variant_get_uint32(call_details.arguments[1].get()), Eq(33));

    stop_mainloop();
}

TEST_F(LogindConsoleServices, handles_ack_device_suspend_failure)
{
    ensure_mock_logind();

    auto session_path = add_any_active_session();
    add_take_device_to_session(
        session_path.c_str(),
        "ret = (os.open('/dev/zero', os.O_RDONLY), False)");
    add_pause_device_complete_to_session(
        session_path.c_str(),
        "raise dbus.exceptions.DBusException('Device or resource busy (36)', name='System.Error.EBUSY')");

    mir::LogindConsoleServices services{the_main_loop()};

    auto device = services.acquire_device(
        22, 33,
        std::make_unique<mtd::NullDeviceObserver>());

    ASSERT_THAT(device.wait_for(30s), Eq(std::future_status::ready));

    auto pause_ack_call = expect_call(
        session_path,
        "PauseDeviceComplete");

    emit_device_suspended(session_path.c_str(), 22, 33, SuspendType::Paused);

    ASSERT_THAT(pause_ack_call.wait_for(30s), Eq(std::future_status::ready));

    // We only want to assert that this doesn't blow up; insert a sync-point
    // to ensure the ack call has been processed.
    wait_for_dbus_sync_point(30s);

    stop_mainloop();
}

TEST_F(LogindConsoleServices, device_removed_callback_called_on_remove)
{
    ensure_mock_logind();

    auto session_path = add_any_active_session();
    add_take_device_to_session(
        session_path.c_str(),
        "ret = (os.open('/dev/zero', os.O_RDONLY), False)");

    mir::LogindConsoleServices services{the_main_loop()};

    DeviceState state;

    auto gone_received = std::make_shared<mt::Signal>();
    auto device = services.acquire_device(
        22, 33,
        std::make_unique<mtd::SimpleDeviceObserver>(
            [&state](mir::Fd&&)
            {
                // We don't actually care about the fd.
                state = DeviceState::Active;
            },
            []() {},
            [gone_received]()
            {
                gone_received->raise();
            }));

    ASSERT_THAT(device.wait_for(30s), Eq(std::future_status::ready));
    ASSERT_THAT(state, Eq(DeviceState::Active));
    auto handle = device.get();

    emit_device_suspended(session_path.c_str(), 22, 33, SuspendType::Gone);

    EXPECT_TRUE(gone_received->wait_for(30s));
    stop_mainloop();
}

TEST_F(LogindConsoleServices, calls_pause_handler_on_pause)
{
    ensure_mock_logind();

    auto session_path = add_any_active_session();

    testing::NiceMock<mtd::MockEventHandlerRegister> registrar;
    mir::LogindConsoleServices services{the_main_loop()};

    auto paused = std::make_shared<mt::Signal>();
    services.register_switch_handlers(
        registrar,
        [paused]()
        {
            paused->raise();
            return true;
        },
        []()
        {
            return true;
        });

    set_logind_session_state(session_path, SessionState::Online);

    EXPECT_TRUE(paused->wait_for(30s));
    stop_mainloop();
}

TEST_F(LogindConsoleServices, calls_resume_handler_on_resume)
{
    ensure_mock_logind();

    auto session_path = add_any_active_session();

    testing::NiceMock<mtd::MockEventHandlerRegister> registrar;
    mir::LogindConsoleServices services{the_main_loop()};

    auto paused = std::make_shared<mt::Signal>();
    auto resumed = std::make_shared<mt::Signal>();
    services.register_switch_handlers(
        registrar,
        [paused]()
        {
            paused->raise();
            return true;
        },
        [resumed]()
        {
            resumed->raise();
            return true;
        });

    set_logind_session_state(session_path, SessionState::Online);

    ASSERT_TRUE(paused->wait_for(30s)) << "Test precondition failure: Failed to switch-from the current session";

    set_logind_session_state(session_path, SessionState::Active);

    EXPECT_TRUE(resumed->wait_for(30s));
    stop_mainloop();
}

TEST_F(LogindConsoleServices, calls_pause_handler_on_closing_state)
{
    ensure_mock_logind();

    auto session_path = add_any_active_session();

    testing::NiceMock<mtd::MockEventHandlerRegister> registrar;
    mir::LogindConsoleServices services{the_main_loop()};

    auto paused = std::make_shared<mt::Signal>();
    services.register_switch_handlers(
        registrar,
        [paused]()
        {
            paused->raise();
            return true;
        },
        []()
        {
            return true;
        });

    set_logind_session_state(session_path, SessionState::Closing);

    EXPECT_TRUE(paused->wait_for(30s));
    stop_mainloop();
}

TEST_F(LogindConsoleServices, spurious_online_state_transitions_are_ignored)
{
    ensure_mock_logind();

    auto session_path = add_any_active_session();

    testing::NiceMock<mtd::MockEventHandlerRegister> registrar;
    mir::LogindConsoleServices services{the_main_loop()};

    auto paused = std::make_shared<mt::Signal>();
    services.register_switch_handlers(
        registrar,
        [paused]()
        {
            paused->raise();
            return true;
        },
        []()
        {
            return true;
        });

    set_logind_session_state(session_path, SessionState::Online);

    ASSERT_TRUE(paused->wait_for(30s)) << "Test precondition failure: Failed to switch-from the current session";

    paused->reset();

    set_logind_session_state(session_path, SessionState::Online);

    /*
     * We're waiting for something to *not* happen, so use a smaller timeout;
    * this can only false-pass, not false-fail.
    */
    EXPECT_FALSE(paused->wait_for(5s)) << "Unexpectedly received pause notification";
    stop_mainloop();
}

TEST_F(LogindConsoleServices, online_to_closing_state_transition_is_ignored)
{
    ensure_mock_logind();

    auto session_path = add_any_active_session();

    mtd::MockEventHandlerRegister registrar;
    mir::LogindConsoleServices services{the_main_loop()};

    auto paused = std::make_shared<mt::Signal>();
    services.register_switch_handlers(
        registrar,
        [paused]()
        {
            paused->raise();
            return true;
        },
        []()
        {
            return true;
        });

    set_logind_session_state(session_path, SessionState::Online);

    ASSERT_TRUE(paused->wait_for(30s)) << "Test precondition failure: Failed to switch-from the current session";

    paused->reset();

    set_logind_session_state(session_path, SessionState::Closing);

    /*
     * We're waiting for something to *not* happen, so use a smaller timeout;
     * this can only false-pass, not false-fail.
     */
    EXPECT_FALSE(paused->wait_for(5s)) << "Unexpectedly received pause notification";
    stop_mainloop();
}

TEST_F(LogindConsoleServices, construction_does_not_require_running_mainloop)
{
    ensure_mock_logind();
    add_any_active_session();

    auto not_running_main_loop =
        std::make_shared<mir::GLibMainLoop>(std::make_shared<mir::time::SteadyClock>());

    mir::LogindConsoleServices services{not_running_main_loop};
    stop_mainloop();
}

TEST_F(LogindConsoleServices, runs_callbacks_on_provided_main_loop)
{
    ensure_mock_logind();
    auto const session_path = add_any_active_session();

    auto main_loop =
        std::make_shared<mir::GLibMainLoop>(std::make_shared<mir::time::SteadyClock>());

    // Construct services before the main loop starts…
    mir::LogindConsoleServices services{main_loop};

    std::thread::id main_loop_id;
    mt::AutoUnblockThread main_loop_thread{
        [main_loop]() { main_loop->stop(); },
        [main_loop, &main_loop_id]()
        {
            main_loop_id = std::this_thread::get_id();
            main_loop->run();
        }};

    auto started = std::make_shared<mt::Signal>();
    main_loop->spawn([started]() { started->raise(); });
    ASSERT_TRUE(started->wait_for(30s));

    mtd::MockEventHandlerRegister registrar;
    auto paused = std::make_shared<mt::Signal>();
    auto resumed = std::make_shared<mt::Signal>();
    services.register_switch_handlers(
        registrar,
        [main_loop_id, paused]()
        {
            EXPECT_THAT(std::this_thread::get_id(), Eq(main_loop_id));
            paused->raise();
            return true;
        },
        [main_loop_id, resumed]()
        {
            EXPECT_THAT(std::this_thread::get_id(), Eq(main_loop_id));
            resumed->raise();
            return true;
        });
    set_logind_session_state(session_path, SessionState::Online);
    EXPECT_TRUE(paused->wait_for(30s));
    set_logind_session_state(session_path, SessionState::Active);
    EXPECT_TRUE(resumed->wait_for(30s));
    stop_mainloop();
}

/*
 * During Mir initialisation, platform probing might require access to devices
 * before the main loop is running.
 *
 * Ensure that's possible.
 */
TEST_F(LogindConsoleServices, can_acquire_device_without_running_main_loop)
{
    ensure_mock_logind();
    auto const session_path = add_any_active_session();

    add_take_device_to_session(
        session_path.c_str(),
        "ret = (os.open('/dev/zero', os.O_RDONLY), False)");

    auto not_running_main_loop =
        std::make_shared<mir::GLibMainLoop>(std::make_shared<mir::time::SteadyClock>());

    mir::LogindConsoleServices services{not_running_main_loop};


    bool device_acquired{false};
    services.acquire_device(
        42, 22,
        std::make_unique<mtd::SimpleDeviceObserver>(
            [&device_acquired](auto) { device_acquired = true; },
            [](){},
            [](){})).get();

    EXPECT_TRUE(device_acquired);
    stop_mainloop();
}

TEST_F(LogindConsoleServices, creates_vt_switcher_when_vt_switching_possible)
{
    ensure_mock_logind();
    add_any_active_session();

    mir::LogindConsoleServices console{the_main_loop()};

    EXPECT_THAT(console.create_vt_switcher(), NotNull());

    stop_mainloop();
}

//TODO: Add test for create_vt_switcher() failing when Logind *can't* switch VTs

TEST_F(LogindConsoleServices, vt_switcher_calls_error_handler_on_error)
{
    ensure_mock_logind();
    add_seat("seat0");
    add_session(
        "s1",
        "seat0",
        1000,
        "ubuntu",
        true,
        "");

    add_switch_to_to_seat(
        "/org/freedesktop/login1/seat/seat0",
        "raise dbus.exceptions.DBusException('No such file or directory (2)', name='System.Error.ENOENT')");

    mir::LogindConsoleServices console{the_main_loop()};
    auto switcher = console.create_vt_switcher();

    auto error_received = std::make_shared<mt::Signal>();

    switcher->switch_to(
        7,
        [error_received](std::exception const& err)
        {
            EXPECT_THAT(dynamic_cast<std::runtime_error const*>(&err), NotNull());
            EXPECT_THAT(err.what(), ContainsRegex(".*No such file or directory.*"));
            error_received->raise();
        });
    EXPECT_TRUE(error_received->wait_for(30s));

    stop_mainloop();
}

TEST_F(LogindConsoleServices, vt_switcher_calls_switch_to_with_correct_argument)
{
    ensure_mock_logind();
    add_seat("seat0");
    add_session(
        "s1",
        "seat0",
        1000,
        "ubuntu",
        true,
        "");

    add_switch_to_to_seat(
        "/org/freedesktop/login1/seat/seat0",
        "");

    mir::LogindConsoleServices console{the_main_loop()};
    auto switcher = console.create_vt_switcher();

    auto switch_to_future = expect_call(
        "/org/freedesktop/login1/seat/seat0",
        "SwitchTo");

    switcher->switch_to(
        3,
        [](auto){});

    ASSERT_THAT(switch_to_future.wait_for(30s), Eq(std::future_status::ready));
    auto switch_to_call = switch_to_future.get();

    ASSERT_THAT(switch_to_call.arguments.size(), Eq(1));

    unsigned vt_arg;
    g_variant_get(
        switch_to_call.arguments[0].get(),
        "u",
        &vt_arg);

    EXPECT_THAT(vt_arg, Eq(3));

    stop_mainloop();
}

TEST_F(LogindConsoleServices, destroying_device_handle_releases_logind_device)
{
    ensure_mock_logind();
    auto const session_path = add_any_active_session();

    add_take_device_to_session(
        session_path.c_str(),
        "ret = (os.open('/dev/zero', os.O_RDONLY), False)");
    add_release_device_to_session(
        session_path.c_str());

    mir::LogindConsoleServices services{the_main_loop()};

    int const device_major{42};
    int const device_minor{88};

    bool device_acquired{false};
    auto device = services.acquire_device(
        device_major, device_minor,
        std::make_unique<mtd::SimpleDeviceObserver>(
            [&device_acquired](auto) { device_acquired = true; },
            [](){},
            [](){})).get();

    ASSERT_TRUE(device_acquired);

    auto future_call = expect_call(session_path.c_str(), "ReleaseDevice");

    device.reset();

    ASSERT_THAT(future_call.wait_for(30s), Eq(std::future_status::ready));

    auto call_info = future_call.get();
    ASSERT_THAT(call_info.arguments.size(), Eq(2));

    int released_major, released_minor;

    g_variant_get(
        call_info.arguments[0].get(),
        "u",
        &released_major);
    g_variant_get(
        call_info.arguments[1].get(),
        "u",
        &released_minor);

    EXPECT_THAT(released_major, Eq(device_major));
    EXPECT_THAT(released_minor, Eq(device_minor));

    stop_mainloop();
}
