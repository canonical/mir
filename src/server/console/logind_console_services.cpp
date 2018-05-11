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

#include <cinttypes>
#include <cstring>
#include <boost/throw_exception.hpp>
#include <boost/current_function.hpp>
#include <boost/exception/info.hpp>
#include <fcntl.h>
#include <sys/sysmacros.h>
#include <gio/gunixfdlist.h>

#include "logind_console_services.h"

#include "logind-session.h"

#include "mir/fd.h"
#include "mir/main_loop.h"
#include "mir/glib_main_loop.h"

#define MIR_LOG_COMPONTENT "logind"
#include "mir/log.h"

namespace
{
class GErrorPtr
{
public:
    GErrorPtr()
        : raw_error{nullptr}
    {
    }
    GErrorPtr(GErrorPtr const&) = delete;
    GErrorPtr& operator=(GErrorPtr const&) = delete;

    ~GErrorPtr()
    {
        if (raw_error)
            g_error_free(raw_error);
    }

    GError** operator&()
    {
        return &raw_error;
    }

    GError* operator->()
    {
        return raw_error;
    }

    operator bool()
    {
        return raw_error;
    }

private:
    GError* raw_error;
};

#define MIR_MAKE_EXCEPTION_PTR(x)\
    std::make_exception_ptr(::boost::enable_error_info(x) <<\
    ::boost::throw_function(BOOST_CURRENT_FUNCTION) <<\
    ::boost::throw_file(__FILE__) <<\
    ::boost::throw_line((int)__LINE__))

std::unique_ptr<LogindSession, decltype(&g_object_unref)> simple_proxy_on_system_bus(
    mir::GLibMainLoop& ml,
    GDBusConnection* connection,
    char const* object_path)
{
    using namespace std::literals::string_literals;

    GErrorPtr error;

    // GDBus proxies will use the thread-default main context at creation time.
    LogindSession* proxy;
    ml.run_with_context_as_thread_default(
        [&proxy, connection, object_path, &error]()
        {
            proxy = logind_session_proxy_new_sync(
                connection,
                G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
                "org.freedesktop.login1",
                object_path,
                nullptr,
                &error);
        });
    if (!proxy)
    {
        auto error_msg = error ? error->message : "unknown error";
        BOOST_THROW_EXCEPTION((
            std::runtime_error{
                "Failed to connect to DBus interface at "s +
                object_path + ": " + error_msg}));
    }

    if (error)
    {
        mir::log_error("Proxy is non-null, but has error set?! Error: %s", error->message);
    }

    return {proxy, &g_object_unref};
}

std::unique_ptr<LogindSeat, decltype(&g_object_unref)> simple_seat_proxy_on_system_bus(
    mir::GLibMainLoop& ml,
    GDBusConnection* connection,
    char const* object_path)
{
    using namespace std::literals::string_literals;

    GErrorPtr error;

    LogindSeat* proxy;
    ml.run_with_context_as_thread_default(
        [&proxy, connection, object_path, &error]()
        {
            proxy = logind_seat_proxy_new_sync(
                connection,
                G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
                "org.freedesktop.login1",
                object_path,
                nullptr,
                &error);
        });
    if (!proxy)
    {
        auto error_msg = error ? error->message : "unknown error";
        BOOST_THROW_EXCEPTION((
            std::runtime_error{
                "Failed to connect to DBus interface at "s +
                object_path + ": " + error_msg}));
    }

    if (error)
    {
        mir::log_error("Proxy is non-null, but has error set?! Error: %s", error->message);
    }

    return {proxy, &g_object_unref};
}

std::string object_path_for_current_session(LogindSeat* seat_proxy)
{
    auto const session_property = logind_seat_get_active_session(seat_proxy);

    if (!session_property)
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to find active session"}));
    }

    auto const object_path_variant = std::unique_ptr<GVariant, decltype(&g_variant_unref)>{
        g_variant_get_child_value(session_property, 1),
        &g_variant_unref
    };

    return g_variant_get_string(object_path_variant.get(), nullptr);
}

std::unique_ptr<GDBusConnection, decltype(&g_object_unref)>
connect_to_system_bus(mir::GLibMainLoop& ml)
{
    using namespace std::literals::string_literals;
    GErrorPtr error;

    std::unique_ptr<gchar, decltype(&g_free)> system_bus_address{
        g_dbus_address_get_for_bus_sync(
            G_BUS_TYPE_SYSTEM,
            nullptr,
            &error),
        &g_free};

    if (!system_bus_address)
    {
        auto error_msg = error ? error->message : "unknown error";
        BOOST_THROW_EXCEPTION((
            std::runtime_error{
                "Failed to find address of DBus system bus: "s + error_msg}));
    }

    GDBusConnection* bus;
    ml.run_with_context_as_thread_default(
        [&bus, &system_bus_address, &error]()
        {
            bus = g_dbus_connection_new_for_address_sync(
                system_bus_address.get(),
                static_cast<GDBusConnectionFlags>(
                    G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT |
                    G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION),
                nullptr,
                nullptr,
                &error);
        });
    if (!bus)
    {
        auto error_msg = error ? error->message : "unknown error";
        BOOST_THROW_EXCEPTION((
          std::runtime_error{
              "Failed to connect to DBus system bus: "s + error_msg}));
    }

    return {bus, &g_object_unref};
}
}

mir::LogindConsoleServices::LogindConsoleServices(std::shared_ptr<mir::GLibMainLoop> const& ml)
    : ml{ml},
      connection{connect_to_system_bus(*ml)},
      seat_proxy{
        simple_seat_proxy_on_system_bus(*ml, connection.get(), "/org/freedesktop/login1/seat/seat0")},
      session_path{object_path_for_current_session(seat_proxy.get())},
      session_proxy{
        simple_proxy_on_system_bus(
            *ml,
            connection.get(),
            session_path.c_str())},
      switch_away{[](){ return true; }},
      switch_to{[](){ return true; }},
      active{strncmp("active", logind_session_get_state(session_proxy.get()), strlen("active")) == 0}
{
    GErrorPtr error;

    if (!logind_session_call_take_control_sync(session_proxy.get(), false, nullptr, &error))
    {
        std::string const error_msg = error ? error->message : "unknown error";

        BOOST_THROW_EXCEPTION((
            std::runtime_error{
                std::string{"Logind TakeControl call failed: "} + error_msg}));
    }

    g_signal_connect(
        G_OBJECT(session_proxy.get()),
        "notify::state",
        G_CALLBACK(&LogindConsoleServices::on_state_change),
        this);
    g_signal_connect(
        G_OBJECT(session_proxy.get()),
        "pause-device",
        G_CALLBACK(&LogindConsoleServices::on_pause_device),
        this);

#ifdef MIR_GDBUS_SIGNALS_SUPPORT_FDS
    g_signal_connect(
        G_OBJECT(session_proxy.get()),
        "resume-device",
        G_CALLBACK(&LogindConsoleServices::on_resume_device),
        this);
#else
    /*
     * GDBus does not handle file descriptors in signals!
     *
     * We need to use a manual filter to slurp up the incoming Signal messages,
     * check if they're for ResumeDevice, and process them directly if so.
     */
    g_dbus_connection_add_filter(
        connection.get(),
        &LogindConsoleServices::resume_device_dbus_filter,
        this,
        nullptr);
#endif
}

void mir::LogindConsoleServices::register_switch_handlers(
    mir::graphics::EventHandlerRegister& /*handlers*/,
    std::function<bool()> const& switch_away,
    std::function<bool()> const& switch_back)
{
    this->switch_away = switch_away;
    this->switch_to = switch_back;
}

void mir::LogindConsoleServices::restore()
{
    logind_session_call_release_control_sync(session_proxy.get(), nullptr, nullptr);
}

class mir::LogindConsoleServices::Device : public mir::Device
{
public:
    Device(
        Device::OnDeviceActivated on_activated,
        Device::OnDeviceSuspended on_suspended,
        Device::OnDeviceRemoved on_removed,
        std::function<void(Device const*)> on_destroy)
        : on_activated{std::move(on_activated)},
          on_suspended{std::move(on_suspended)},
          on_removed{std::move(on_removed)},
          on_destroy{std::move(on_destroy)}
    {
    }

    ~Device() noexcept
    {
        on_destroy(this);
    }

    void emit_activated(mir::Fd&& device_fd) const
    {
        on_activated(std::move(device_fd));
    }

    void emit_suspended() const
    {
        on_suspended();
    }

    void emit_removed() const
    {
        on_removed();
    }
private:
    Device::OnDeviceActivated const on_activated;
    Device::OnDeviceSuspended const on_suspended;
    Device::OnDeviceRemoved const on_removed;
    std::function<void(Device const*)> const on_destroy;
};

namespace
{
struct TakeDeviceContext
{
    std::unique_ptr<mir::LogindConsoleServices::Device> device;
    std::promise<std::unique_ptr<mir::Device>> promise;
};

void complete_take_device_call(
    GObject* proxy,
    GAsyncResult* result,
    gpointer ctx) noexcept
{
    std::unique_ptr<TakeDeviceContext> const context{
        static_cast<TakeDeviceContext*>(ctx)};

    GErrorPtr error;
    GUnixFDList* fd_list;

    std::unique_ptr<GVariant, decltype(&g_variant_unref)> dbus_result{
        g_dbus_proxy_call_with_unix_fd_list_finish(
            G_DBUS_PROXY(proxy),
            &fd_list,
            result,
            &error),
        &g_variant_unref};
    if (!dbus_result)
    {
        std::string error_msg = error ? error->message : "unknown error";
        context->promise.set_exception(
            MIR_MAKE_EXCEPTION_PTR((
                std::runtime_error{
                    std::string{"TakeDevice call failed: "} + error_msg})));
        return;
    }

    int num_fds;
    std::unique_ptr<gint[], std::function<void(gint*)>> fds{
        g_unix_fd_list_steal_fds(fd_list, &num_fds),
        [&num_fds](gint* fd_list)
        {
            // First close any open fds…
            for (int i = 0; i < num_fds; ++i)
            {
                if (fd_list[i] != -1)
                {
                    ::close(fd_list[i]);
                }
            }
            // …then free the list itself.
            g_free(fd_list);
        }};
    g_object_unref(fd_list);

    gboolean inactive;
    int fd_index;

    g_variant_get(dbus_result.get(), "(hb)", &fd_index, &inactive);

    mir::Fd fd{fds[fd_index]};
    // We don't want our FD to be closed by the fds destructor
    fds[fd_index] = -1;

    if (fd == mir::Fd::invalid)
    {
        /*
         * I don't believe we can get here - the proxy checks that the DBus return value
         * has type (fd, boolean), and gdbus will error if it can't read the fd out of the
         * auxiliary channel.
         *
         * Better safe than sorry, though
         */
        context->promise.set_exception(
            MIR_MAKE_EXCEPTION_PTR((
                std::runtime_error{"TakeDevice call returned invalid FD"})));
        return;
    }

    if (!inactive)
    {
        context->device->emit_activated(std::move(fd));
    }
    else
    {
        context->device->emit_suspended();
    }
    context->promise.set_value(std::move(context->device));
}
}

std::future<std::unique_ptr<mir::Device>> mir::LogindConsoleServices::acquire_device(
    int major, int minor,
    Device::OnDeviceActivated const& on_activated,
    Device::OnDeviceSuspended const& on_suspended,
    Device::OnDeviceRemoved const& on_removed)
{
    auto context = std::make_unique<TakeDeviceContext>();
    context->device = std::make_unique<Device>(
        on_activated,
        on_suspended,
        on_removed,
        [this, devnum = makedev(major, minor)](Device const* destroying)
        {
            auto const it = acquired_devices.find(devnum);
            // Device could have been removed from the map by a PauseDevice("gone") signal
            if (it != acquired_devices.end())
            {
                // It could have been removed and then re-added by a subsequent acquire_device…
                if (it->second == destroying)
                {
                    acquired_devices.erase(it);
                }
            }
        });
    acquired_devices.insert(std::make_pair(makedev(major, minor), context->device.get()));

    auto future = context->promise.get_future();

    if (ml->running())
    {
        /*
         * The main loop is running; we can run this call asynchronously and get
         * notified of completion.
         *
         * It uses the thread-default main context, so must be run from the context of
         * the main loop.
         */
        ml->run_with_context_as_thread_default(
            [
                major,
                minor,
                proxy = G_DBUS_PROXY(session_proxy.get()),
                userdata = context.release()
            ]()
            {
                g_dbus_proxy_call_with_unix_fd_list(
                    proxy,
                    "TakeDevice",
                    g_variant_new(
                        "(uu)",
                        major,
                        minor),
                    G_DBUS_CALL_FLAGS_NO_AUTO_START,
                    G_MAXINT,
                    nullptr,
                    nullptr,
                    &complete_take_device_call,
                    userdata);
            });
    }
    else
    {
        /*
         * The main loop is not running; we must make a synchronous call, as
         * the asynchronous call requires the main loop to dispatch the result.
         */
        using namespace std::chrono;
        using namespace std::chrono_literals;

        GUnixFDList* resultant_fds;
        GErrorPtr error;

        std::unique_ptr<GVariant, decltype(&g_variant_unref)> dbus_result{
            g_dbus_proxy_call_with_unix_fd_list_sync(
                G_DBUS_PROXY(session_proxy.get()),
                "TakeDevice",
                g_variant_new(
                    "(uu)",
                    major,
                    minor),
                G_DBUS_CALL_FLAGS_NO_AUTO_START,
                duration_cast<milliseconds>(10s).count(),
                nullptr,
                &resultant_fds,
                nullptr,
                &error),
            &g_variant_unref};

        if (!dbus_result)
        {
            std::string error_msg = error ? error->message : "unknown error";
            context->promise.set_exception(
                MIR_MAKE_EXCEPTION_PTR((
                    std::runtime_error{
                        std::string{"TakeDevice call failed: "} + error_msg})));
            return future;
        }

        int num_fds;
        std::unique_ptr<gint[], std::function<void(gint*)>> fds{
            g_unix_fd_list_steal_fds(resultant_fds, &num_fds),
            [&num_fds](gint* fd_list)
            {
                // First close any open fds…
                for (int i = 0; i < num_fds; ++i)
                {
                    if (fd_list[i] != -1)
                    {
                        ::close(fd_list[i]);
                    }
                }
                // …then free the list itself.
                g_free(fd_list);
            }};
        g_object_unref(resultant_fds);

        gboolean inactive;
        int fd_index;

        g_variant_get(dbus_result.get(), "(hb)", &fd_index, &inactive);

        mir::Fd fd{fds[fd_index]};
        // We don't want our FD to be closed by the fds destructor
        fds[fd_index] = -1;

        if (fd == mir::Fd::invalid)
        {
            /*
             * I don't believe we can get here - the proxy checks that the DBus return value
             * has type (fd, boolean), and gdbus will error if it can't read the fd out of the
             * auxiliary channel.
             *
             * Better safe than sorry, though
             */
            context->promise.set_exception(
                MIR_MAKE_EXCEPTION_PTR((
                    std::runtime_error{"TakeDevice call returned invalid FD"})));
            return future;
        }

        if (!inactive)
        {
            context->device->emit_activated(std::move(fd));
        }
        else
        {
            context->device->emit_suspended();
        }
        context->promise.set_value(std::move(context->device));
    }
    return future;
}

void mir::LogindConsoleServices::on_state_change(
    GObject* session_proxy,
    GParamSpec*,
    gpointer ctx) noexcept
{
    // No threadsafety concerns, as this is only ever called from the glib mainloop.
    auto me = static_cast<LogindConsoleServices*>(ctx);

    auto const state = logind_session_get_state(LOGIND_SESSION(session_proxy));
    if (strncmp(state, "active", strlen("active")) == 0)
    {
        // “active” means running and foregrounded.
        if (!me->active)
        {
            me->switch_to();
        }
        me->active = true;
    }
    else
    {
        // Everything else is not foreground.
        if (me->active)
        {
            me->switch_away();
        }
        me->active = false;
    }
}

void mir::LogindConsoleServices::on_pause_device(
    LogindSession*,
    unsigned major, unsigned minor,
    gchar const* suspend_type,
    gpointer ctx) noexcept
{
    // No threadsafety concerns, as this is only ever called from the glib mainloop.
    auto me = static_cast<LogindConsoleServices*>(ctx);

    auto const it = me->acquired_devices.find(makedev(major, minor));
    if (it != me->acquired_devices.end())
    {
        using namespace std::literals::string_literals;
        if ("pause"s == suspend_type)
        {
            it->second->emit_suspended();
        }
        else if ("gone"s == suspend_type)
        {
            it->second->emit_removed();
            // The device is gone; logind promises not to send further events for it
            me->acquired_devices.erase(it);
        }
        else
        {
            mir::log_warning("Received unhandled PauseDevice type: %s", suspend_type);
        }
    }
}

#ifdef MIR_GDBUS_SIGNALS_SUPPORT_FDS
void mir::LogindConsoleServices::on_resume_device(
    LogindSession*,
    unsigned major, unsigned minor,
    GVariant* fd,
    gpointer ctx) noexcept
{
    // No threadsafety concerns, as this is only ever called from the glib mainloop.
    auto me = static_cast<LogindConsoleServices*>(ctx);

    auto const it = me->acquired_devices.find(makedev(major, minor));
    if (it != me->acquired_devices.end())
    {
        it->second->emit_activated(mir::Fd{g_variant_get_handle(fd)});
    }
}
#else
GDBusMessage* mir::LogindConsoleServices::resume_device_dbus_filter(
    GDBusConnection* /*connection*/,
    GDBusMessage* message,
    gboolean incoming,
    gpointer ctx) noexcept
{
    // If this isn't an incoming message it's definitely not a ResumeDevice signal…
    if (!incoming)
        return message;

    // …if this isn't a signal, then it's not a ResumeDevice signal…
    if (g_dbus_message_get_message_type(message) != G_DBUS_MESSAGE_TYPE_SIGNAL)
        return message;

    // …if it's a signal, but it's not ResumeDevice, we don't need to process it.
    if (strncmp(g_dbus_message_get_member(message), "ResumeDevice", strlen("ResumeDevice")) != 0)
        return message;

    // We've definitely got a ResumeDevice signal! Now to extract the parameters, and
    // hook up the fd.

    auto body = g_dbus_message_get_body(message);

    unsigned major, minor;
    int handle_index;

    g_variant_get(
        body,
        "(uuh)",
        &major,
        &minor,
        &handle_index);

    int num_fds;
    std::unique_ptr<gint[], std::function<void(gint*)>> fd_list{
        g_unix_fd_list_steal_fds(g_dbus_message_get_unix_fd_list(message), &num_fds),
        [&num_fds](gint* fd_list)
        {
            // First close any open fds…
            for (int i = 0; i < num_fds; ++i)
            {
                if (fd_list[i] != -1)
                {
                    ::close(fd_list[i]);
                }
            }
            // …then free the list itself.
            g_free(fd_list);
        }};

    // No threadsafety concerns, as this is only ever called from the glib mainloop.
    auto me = static_cast<LogindConsoleServices*>(ctx);

    auto const it = me->acquired_devices.find(makedev(major, minor));
    if (it != me->acquired_devices.end())
    {
        it->second->emit_activated(mir::Fd{fd_list[handle_index]});
        // Don't close the file descriptor we've just handed out.
        fd_list[handle_index] = -1;
    }

    // We don't need to further process this message.
    g_object_unref(message);
    return nullptr;
}
#endif
