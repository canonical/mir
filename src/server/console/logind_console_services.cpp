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

    auto const object_path_variant = g_variant_get_child_value(session_property, 1);

    return g_variant_get_string(object_path_variant, nullptr);
}
}

mir::LogindConsoleServices::LogindConsoleServices()
    : seat_proxy{simple_seat_proxy_on_system_bus("/org/freedesktop/login1/seat/seat0"),
        &g_object_unref},
      session_proxy{simple_proxy_on_system_bus(
        object_path_for_current_session(seat_proxy.get()).c_str()),
        &g_object_unref}
{
    GErrorPtr error;

    if (!logind_session_call_take_control_sync(session_proxy.get(), false, nullptr, &error))
    {
        std::string const error_msg = error ? error->message : "unknown error";

        BOOST_THROW_EXCEPTION((
            std::runtime_error{
                std::string{"Logind TakeControl call failed: "} + error_msg}));
    }
}

void mir::LogindConsoleServices::register_switch_handlers(
    mir::graphics::EventHandlerRegister& /*handlers*/,
    std::function<bool()> const& /*switch_away*/,
    std::function<bool()> const& /*switch_back*/)
{

}

void mir::LogindConsoleServices::restore()
{
    logind_session_call_release_control_sync(session_proxy.get(), nullptr, nullptr);
}

namespace
{
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
    std::unique_ptr<boost::promise<mir::Fd>> const promise{
        static_cast<boost::promise<mir::Fd>*>(ctx)};

    GErrorPtr error;
    GVariant* fd_holder;
    gboolean inactive;      // TODO: What do we need to do about this?

    if (!logind_session_call_take_device_finish(
        LOGIND_SESSION(proxy),
        &fd_holder,
        &inactive,
        result,
        &error))
    {
        std::string error_msg = error ? error->message : "unknown error";
        promise->set_exception(
            MIR_MAKE_EXCEPTION_PTR((
                std::runtime_error{
                    std::string{"TakeDevice call failed: "} + error_msg})));
        return;
    }

    auto fd = mir::Fd{g_variant_get_handle(fd_holder)};

    if (fd == mir::Fd::invalid)
    {
        /*
         * I don't believe we can get here - the proxy checks that the DBus return value
         * has type (fd, boolean), and gdbus will error if it can't read the fd out of the
         * auxiliary channel.
         *
         * Better safe than sorry, though
         */
        promise->set_exception(
            MIR_MAKE_EXCEPTION_PTR((
                std::runtime_error{"TakeDevice call returned invalid FD"})));
        return;
    }

    promise->set_value(std::move(fd));
}
}

boost::future<mir::Fd> mir::LogindConsoleServices::acquire_device(int major, int minor)
{
    auto pending_device = std::make_unique<boost::promise<Fd>>();
    auto device_future = pending_device->get_future();

    logind_session_call_take_device(
        session_proxy.get(),
        major, minor,
        nullptr,
        &complete_take_device_call,
        pending_device.release());

    return device_future;
}
