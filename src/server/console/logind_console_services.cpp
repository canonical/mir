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
    // GDBus proxies will use the thread-default main context at creation time.
    return ml.run_with_context_as_thread_default(
        [connection, object_path]()
        {
            GErrorPtr error;

            auto proxy = std::unique_ptr<LogindSession, decltype(&g_object_unref)>{
                logind_session_proxy_new_sync(
                    connection,
                    G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
                    "org.freedesktop.login1",
                    object_path,
                    nullptr,
                    &error),
                &g_object_unref};

            if (!proxy)
            {
                using namespace std::literals::string_literals;

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
            return proxy;

        }).get();
}

std::unique_ptr<LogindSeat, decltype(&g_object_unref)> simple_seat_proxy_on_system_bus(
    mir::GLibMainLoop& ml,
    GDBusConnection* connection,
    char const* object_path)
{
    using namespace std::literals::string_literals;

    return ml.run_with_context_as_thread_default(
        [connection, object_path]()
        {
            GErrorPtr error;

            auto proxy = std::unique_ptr<LogindSeat, decltype(&g_object_unref)>{
                logind_seat_proxy_new_sync(
                    connection,
                    G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
                    "org.freedesktop.login1",
                    object_path,
                    nullptr,
                    &error),
                &g_object_unref};

            if (!proxy)
            {
                using namespace std::literals::string_literals;
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

            return proxy;
        }).get();
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

    auto const object_path = g_variant_get_string(object_path_variant.get(), nullptr);

    /* We require the display manager to have already set up an active session for us.
     *
     * Because logind couldn't be bothered using the *perfectly functional* DBus optional
     * type, we detect this by the ActiveSession property having an object-path of "/"
     */
    if (!object_path || (strcmp(object_path, "/") == 0))
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{"Seat has no active session"}));
    }

    return {object_path};
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

    return ml.run_with_context_as_thread_default(
        [&system_bus_address]()
        {
            GErrorPtr error;

            auto bus = std::unique_ptr<GDBusConnection, decltype(&g_object_unref)>{
                g_dbus_connection_new_for_address_sync(
                    system_bus_address.get(),
                    static_cast<GDBusConnectionFlags>(
                        G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT |
                        G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION),
                    nullptr,
                    nullptr,
                    &error),
                &g_object_unref};

            if (!bus)
            {
                auto error_msg = error ? error->message : "unknown error";
                BOOST_THROW_EXCEPTION((
                    std::runtime_error{
                        "Failed to connect to DBus system bus: "s + error_msg}));
            }

            return bus;
        }).get();
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
        "notify::active",
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
        std::unique_ptr<Observer> observer,
        std::function<void(Device const*)> on_destroy)
        : observer{std::move(observer)},
          on_destroy{std::move(on_destroy)}
    {
    }

    ~Device() noexcept
    {
        on_destroy(this);
    }

    void emit_activated(mir::Fd&& device_fd) const
    {
        observer->activated(std::move(device_fd));
    }

    void emit_suspended() const
    {
        observer->suspended();
    }

    void emit_removed() const
    {
        observer->removed();
    }
private:
    std::unique_ptr<Observer> const observer;
    std::function<void(Device const*)> const on_destroy;
};

namespace
{
struct TakeDeviceContext
{
    std::unique_ptr<mir::LogindConsoleServices::Device> device;
    std::promise<std::unique_ptr<mir::Device>> promise;
};

void handle_take_device_dbus_result(
    std::unique_ptr<TakeDeviceContext> context,
    std::unique_ptr<GVariant, decltype(&g_variant_unref)> result,
    GUnixFDList* fd_list)
{
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

    g_variant_get(result.get(), "(hb)", &fd_index, &inactive);

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

void complete_take_device_call(
    GObject* proxy,
    GAsyncResult* result,
    gpointer ctx) noexcept
{
    std::unique_ptr<TakeDeviceContext> context{
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

    handle_take_device_dbus_result(
        std::move(context),
        std::move(dbus_result),
        fd_list);
}

void complete_release_device_call(
    GObject* proxy,
    GAsyncResult* result,
    gpointer) noexcept
{
    GErrorPtr error;

    if (!logind_session_call_release_device_finish(
        LOGIND_SESSION(proxy),
        result,
        &error))
    {
        auto const error_message = error ? error->message : "unknown error";

        mir::log_warning(
            "ReleaseDevice call failed: %s",
            error_message);
    }
}
}

std::future<std::unique_ptr<mir::Device>> mir::LogindConsoleServices::acquire_device(
    int major, int minor,
    std::unique_ptr<Device::Observer> observer)
{
    auto context = std::make_unique<TakeDeviceContext>();
    context->device = std::make_unique<Device>(
        std::move(observer),
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
                    ml->run_with_context_as_thread_default(
                        [
                            session = std::shared_ptr<GObject>{
                                G_OBJECT(g_object_ref(session_proxy.get())),
                                &g_object_unref},
                            devnum
                        ]()
                        {
                            logind_session_call_release_device(
                                LOGIND_SESSION(session.get()),
                                major(devnum), minor(devnum),
                                nullptr,
                                &complete_release_device_call,
                                nullptr);
                        });
                }
            }
        });

    if (!acquired_devices.insert(std::make_pair(makedev(major, minor), context->device.get())).second)
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{"Attempted to acquire a device multiple times"}));
    }

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
            }); // We don't need to wait for completion here, so throw away the std::future.
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

        handle_take_device_dbus_result(
            std::move(context),
            std::move(dbus_result),
            resultant_fds);
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

    auto const active = logind_session_get_active(LOGIND_SESSION(session_proxy));
    if (active)
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

namespace {
void complete_pause_device_complete(
    GObject* session_proxy,
    GAsyncResult* result,
    gpointer)
{
    GErrorPtr err;
    if (!logind_session_call_pause_device_complete_finish(
        LOGIND_SESSION(session_proxy),
        result,
        &err))
    {
        mir::log_warning("PauseDeviceComplete failed: %s", err->message);
    }
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
            mir::log_debug("Received logind pause event for device %i:%i", major, minor);
            it->second->emit_suspended();
            logind_session_call_pause_device_complete(
                me->session_proxy.get(),
                major, minor,
                nullptr,
                &complete_pause_device_complete,
                nullptr);
        }
        else if ("force"s == suspend_type)
        {
            mir::log_debug("Received logind force-pause event for device %i:%i", major, minor);
            it->second->emit_suspended();
        }
        else if ("gone"s == suspend_type)
        {
            it->second->emit_removed();
            // The device is gone; logind promises not to send further events for it
            me->acquired_devices.erase(it);
            // …unfortunately, logind is a FILTHY LIAR.
            logind_session_call_release_device(
                me->session_proxy.get(),
                major, minor,
                nullptr,
                &complete_release_device_call,
                nullptr);
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

namespace
{
class LogindVTSwitcher : public mir::VTSwitcher
{
public:
    LogindVTSwitcher(
        std::unique_ptr<LogindSeat, decltype(&g_object_unref)> seat_proxy,
        std::shared_ptr<mir::GLibMainLoop> ml)
        : seat_proxy{std::move(seat_proxy)},
          ml{std::move(ml)}
    {
    }

    void switch_to(
        int vt_number,
        std::function<void(std::exception const&)> error_handler) override
    {
        ml->run_with_context_as_thread_default(
            [this, vt_number, error_handler = std::move(error_handler)]()
            {
                logind_seat_call_switch_to(
                    seat_proxy.get(),
                    vt_number,
                    nullptr,
                    &LogindVTSwitcher::complete_switch_to,
                    new std::function<void(std::exception const&)>{error_handler});
            }); // No need to wait for this to run; drop the std::future on the floor
    }

private:
    static void complete_switch_to(
        GObject* seat_proxy,
        GAsyncResult* result,
        gpointer userdata)
    {
        auto const error_handler = std::unique_ptr<std::function<void(std::exception const&)>>{
            static_cast<std::function<void(std::exception const&)>*>(userdata)};
        GErrorPtr error;

        if (!logind_seat_call_switch_to_finish(
           LOGIND_SEAT(seat_proxy),
           result,
           &error))
        {
            auto const err = boost::enable_error_info(
                std::runtime_error{
                    std::string{"Logind request to switch vt failed: "} +
                    error->message})
                        << boost::throw_file(__FILE__)
                        << boost::throw_function(__PRETTY_FUNCTION__)
                        << boost::throw_line(__LINE__);

            (*error_handler)(err);
        }
    }

    std::unique_ptr<LogindSeat, decltype(&g_object_unref)> const seat_proxy;
    std::shared_ptr<mir::GLibMainLoop> const ml;
};
}

std::unique_ptr<mir::VTSwitcher> mir::LogindConsoleServices::create_vt_switcher()
{
    // TODO: Query logind whether VTs exist at all, and throw if they don't.
    return std::make_unique<LogindVTSwitcher>(
        std::unique_ptr<LogindSeat, decltype(&g_object_unref)>{
            LOGIND_SEAT(g_object_ref(seat_proxy.get())),
            &g_object_unref},
        ml);
}
