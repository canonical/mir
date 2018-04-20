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
#ifndef MIR_LOGIND_CONSOLE_SERVICES_H_
#define MIR_LOGIND_CONSOLE_SERVICES_H_

#include <future>
#include <unordered_map>
#include "mir/console_services.h"

#include "glib.h"
#include "logind-seat.h"
#include "logind-session.h"

namespace mir
{
class LogindConsoleServices : public ConsoleServices
{
public:
    LogindConsoleServices();

    void register_switch_handlers(
        graphics::EventHandlerRegister& handlers,
        std::function<bool()> const& switch_away,
        std::function<bool()> const& switch_back) override;

    void restore() override;

    std::future<std::unique_ptr<mir::Device>> acquire_device(
        int major, int minor,
        mir::Device::OnDeviceActivated const& on_activated,
        mir::Device::OnDeviceSuspended const& on_suspended,
        mir::Device::OnDeviceRemoved const& on_removed) override;

    class Device;
private:
    static void on_state_change(GObject* session_proxy, GParamSpec*, gpointer ctx) noexcept;
    static void on_pause_device(
        LogindSession*,
        unsigned major,
        unsigned minor,
        gchar const* type,
        gpointer ctx) noexcept;
#ifdef MIR_GDBUS_SIGNALS_SUPPORT_FDS
    static void on_resume_device(
        LogindSession*,
        unsigned major,
        unsigned minor,
        GVariant* fd,
        gpointer ctx);
#else
    static GDBusMessage* resume_device_dbus_filter(
        GDBusConnection* connection,
        GDBusMessage* message,
        gboolean incoming,
        gpointer ctx) noexcept;
#endif

    std::unique_ptr<GDBusConnection, decltype(&g_object_unref)> const connection;
    std::unique_ptr<LogindSeat, decltype(&g_object_unref)> const seat_proxy;
    std::string const session_path;
    std::unique_ptr<LogindSession, decltype(&g_object_unref)> const session_proxy;
    std::function<bool()> switch_away;
    std::function<bool()> switch_to;
    bool active;
    std::unordered_map<dev_t, Device const* const> acquired_devices;
};
}


#endif //MIR_LOGIND_CONSOLE_SERVICES_H_
