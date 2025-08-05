/*
* Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIRAL_GDBUS_H_
#define MIRAL_GDBUS_H_

#include <gio/gio.h>

#include <memory>
#include <thread>

/// Helper classes for using GDbus
namespace miral::gdbus
{
class Connection : std::shared_ptr<GDBusConnection>
{
public:
    explicit Connection(GDBusConnection* connection) : std::shared_ptr<GDBusConnection>{connection, &g_object_unref} {}

    operator GDBusConnection*() const { return get(); }

private:
    friend void g_object_unref(GDBusConnection*) = delete;
};

class Variant : std::shared_ptr<GVariant>
{
public:
    explicit Variant(GVariant* variant) : std::shared_ptr<GVariant>{variant, &g_variant_unref} {}

    operator GVariant*() const { return get(); }
    using std::shared_ptr<GVariant>::operator bool;
private:
    friend void g_variant_unref(GVariant*) = delete;
};

class MainLoop
{
public:

    static auto the_main_loop() -> std::shared_ptr<MainLoop>;

    ~MainLoop();

private:
    MainLoop();
    GMainLoop* const loop;
    std::jthread const t;
};
}

#endif //MIRAL_GDBUS_H_
