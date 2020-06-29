/*
 * Copyright © 2019-2020 Canonical Ltd.
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
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "open_desktop_entry.h"

#include <mir/log.h>

#include <gio/gio.h>
#include <memory>

namespace
{
class Connection : std::shared_ptr<GDBusConnection>
{
public:
    explicit Connection(GDBusConnection* connection) : std::shared_ptr<GDBusConnection>{connection, &g_object_unref} {}

    operator GDBusConnection*() const { return get(); }

private:
    friend void g_object_unref(GDBusConnection*) = delete;
};

// Desktop File ID (https://standards.freedesktop.org/desktop-entry-spec/desktop-entry-spec-latest.html#desktop-file-id)
//
// Each desktop entry representing an application is identified by its desktop file ID, which is based
// on its filename.
//
// To determine the ID of a desktop file, make its full path relative to the $XDG_DATA_DIRS component
// in which the desktop file is installed, remove the "applications/" prefix, and turn '/' into '-'.
//
// For example /usr/share/applications/foo/bar.desktop has the desktop file ID foo-bar.desktop.
//
// If multiple files have the same desktop file ID, the first one in the $XDG_DATA_DIRS precedence
// order is used.
//
// For example, if $XDG_DATA_DIRS contains the default paths /usr/local/share:/usr/share, then
// /usr/local/share/applications/org.foo.bar.desktop and /usr/share/applications/org.foo.bar.desktop
// both have the same desktop file ID org.foo.bar.desktop, but only the first one will be used.
//
// If both foo-bar.desktop and foo/bar.desktop exist, it is undefined which is selected.
//
// If the desktop file is not installed in an applications subdirectory of one of the $XDG_DATA_DIRS
// components, it does not have an ID.
auto extract_id(std::string const& filename) -> std::string
{
    static std::string const applications{"/applications/"};

    auto const pos = filename.rfind(applications);
    if (pos == std::string::npos)
        return filename;

    auto result = filename.substr(pos+applications.length());

    for (auto& r : result)
    {
        if (r == '/') r = '-';
    }

    return result;
}
}

void miral::open_desktop_entry(std::string const& desktop_file, std::vector<std::string> const& env)
{
    Connection const connection{g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, nullptr)};

    static char const* const dest = "io.snapcraft.Launcher";
    static char const* const object_path = "/io/snapcraft/Launcher";
    static char const* const interface_name = "io.snapcraft.Launcher";
    static char const* const method_name = "OpenDesktopEntryEnv";
    auto const id = extract_id(desktop_file);

    GError* error = nullptr;

    GVariantBuilder* const builder = g_variant_builder_new(G_VARIANT_TYPE ("as"));
    for (auto const& e : env)
        g_variant_builder_add (builder, "s", e.c_str());

    if (auto const result = g_dbus_connection_call_sync(
            connection,
            dest,
            object_path,
            interface_name,
            method_name,
            g_variant_new("(sas)", id.c_str(), builder),
            nullptr,
            G_DBUS_CALL_FLAGS_NONE,
            G_MAXINT,
            nullptr,
            &error))
    {
        g_variant_unref(result);
    }

    if (error)
    {
        mir::log_info("Dbus error=%s, dest=%s, object_path=%s, interface_name=%s, method_name=%s, id=%s",
                      error->message, dest, object_path, interface_name, method_name, id.c_str());
        g_error_free(error);
    }

    g_variant_builder_unref(builder);
}
