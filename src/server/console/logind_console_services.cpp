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
#include <boost/throw_exception.hpp>
#include <boost/current_function.hpp>
#include <boost/exception/info.hpp>

#include "logind_console_services.h"

#include "logind-session.h"

#include "mir/fd.h"
#include "mir/main_loop.h"

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

LogindSession* simple_proxy_on_system_bus(
    char const* object_path)
{
    using namespace std::literals::string_literals;

    GErrorPtr error;

    auto const proxy = logind_session_proxy_new_for_bus_sync(
        G_BUS_TYPE_SYSTEM,
        G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
        "org.freedesktop.login1",
        object_path,
        nullptr,
        &error);

    if (!proxy)
    {
        auto error_msg = error ? error->message : "unknown error";
        BOOST_THROW_EXCEPTION((
            std::runtime_error{
                "Failed to connect to DBus interface at "s +
                object_path + ": " + error_msg
            }));
    }

    if (error)
    {
        mir::log_error("Proxy is non-null, but has error set?! Error: %s", error->message);
    }

    return proxy;
}

LogindSeat* simple_seat_proxy_on_system_bus(
    char const* object_path)
{
    using namespace std::literals::string_literals;

    GErrorPtr error;

    auto const proxy = logind_seat_proxy_new_for_bus_sync(
        G_BUS_TYPE_SYSTEM,
        G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
        "org.freedesktop.login1",
        object_path,
        nullptr,
        &error);

    if (!proxy)
    {
        auto error_msg = error ? error->message : "unknown error";
        BOOST_THROW_EXCEPTION((
                                  std::runtime_error{
                                      "Failed to connect to DBus interface at "s +
                                      object_path + ": " + error_msg
                                  }));
    }

    if (error)
    {
        mir::log_error("Proxy is non-null, but has error set?! Error: %s", error->message);
    }

    return proxy;
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
}

mir::LogindConsoleServices::LogindConsoleServices()
    : seat_proxy{simple_seat_proxy_on_system_bus("/org/freedesktop/login1/seat/seat0"),
        &g_object_unref},
      session_proxy{simple_proxy_on_system_bus(
        object_path_for_current_session(seat_proxy.get()).c_str()),
        &g_object_unref},
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

namespace
{
class LogindDevice : public mir::Device
{
public:
    LogindDevice(
        Device::OnDeviceActivated on_activated,
        Device::OnDeviceSuspended on_suspended,
        Device::OnDeviceRemoved on_removed)
        : on_activated{std::move(on_activated)},
          on_suspended{std::move(on_suspended)},
          on_removed{std::move(on_removed)}
    {
    }

    void emit_activated(mir::Fd&& device_fd)
    {
        on_activated(std::move(device_fd));
    }

    void emit_suspended()
    {
        on_suspended();
    }

    void emit_removed()
    {
        on_removed();
    }
private:
    Device::OnDeviceActivated const on_activated;
    Device::OnDeviceSuspended const on_suspended;
    Device::OnDeviceRemoved const on_removed;
};

struct TakeDeviceContext
{
    std::unique_ptr<LogindDevice> device;
    std::promise<std::unique_ptr<mir::Device>> promise;
};

#define MIR_MAKE_EXCEPTION_PTR(x)\
    std::make_exception_ptr(::boost::enable_error_info(x) <<\
    ::boost::throw_function(BOOST_CURRENT_FUNCTION) <<\
    ::boost::throw_file(__FILE__) <<\
    ::boost::throw_line((int)__LINE__))

void complete_take_device_call(
    GObject* proxy,
    GAsyncResult* result,
    gpointer ctx)
{
    std::unique_ptr<TakeDeviceContext> const context{
        static_cast<TakeDeviceContext*>(ctx)};

    GErrorPtr error;
    GVariant* fd_holder;
    gboolean inactive;

    if (!logind_session_call_take_device_finish(
        LOGIND_SESSION(proxy),
        &fd_holder,
        &inactive,
        result,
        &error))
    {
        std::string error_msg = error ? error->message : "unknown error";
        context->promise.set_exception(
            MIR_MAKE_EXCEPTION_PTR((
                std::runtime_error{
                    std::string{"TakeDevice call failed: "} + error_msg})));
        return;
    }

    auto fd = mir::Fd{g_variant_get_handle(fd_holder)};

    g_variant_unref(fd_holder);

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
    context->device = std::make_unique<LogindDevice>(
        on_activated,
        on_suspended,
        on_removed);
    auto future = context->promise.get_future();

    logind_session_call_take_device(
        session_proxy.get(),
        major, minor,
        nullptr,
        &complete_take_device_call,
        context.release());

    return future;
}

void mir::LogindConsoleServices::on_state_change(
    GObject* session_proxy,
    GParamSpec*,
    gpointer ctx)
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
