/*
 * Copyright © Canonical Ltd.
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
 */

#include "desktop_file_manager.h"

#include "mir/main_loop.h"
#include "mir/log.h"
#include "mir/scene/surface.h"
#include "mir/scene/session.h"
#include <fstream>
#include <filesystem>

#ifdef MIR_USE_APPARMOR
#include <sys/apparmor.h>
#endif

namespace mf = mir::frontend;

namespace
{
std::pair<std::string, std::string> const app_id_to_desktop_file_quirks[] = {
    {"gnome-terminal-server", "org.gnome.Terminal"} // https://gitlab.gnome.org/GNOME/gnome-terminal/-/issues/8033
};

inline void rtrim(std::string &s) {
    s.erase(std::find_if(
        s.rbegin(),
        s.rend(),
        [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), s.end());
}
}

mf::DesktopFile::DesktopFile(const char* id, const char* wm_class, const char* exec)
    : id{id ? id : ""},
      wm_class{wm_class ? wm_class : ""},
      exec{exec ? exec : ""}
{}

mf::DesktopFileManager::DesktopFileManager(std::shared_ptr<DesktopFileCache> cache)
    : cache{cache}
{
}

std::string mf::DesktopFileManager::resolve_app_id(const scene::Surface* surface)
{
    // In this method, we're attempting to resolve a Surface back to it's GAppInfo so that
    // we can report a best-effort app_id to the user.
    // Unfortunately, the connection between a window and a desktop is ill-defined.
    // Hence, we jump through a series of hoops in the hope that we'll find what we're looking for.
    // For more info on the checks happening here, see:
    // https://gitlab.gnome.org/GNOME/gnome-shell/-/blob/main/src/shell-window-tracker.c?ref_type=heads#L387
    auto app_id = surface->application_id();
    rtrim(app_id); // Sometimes, the app id has a space at the end
    mir::log_info("Attempting to resolve app id from app_id=%s", app_id.c_str());

    // First, let's see if this is just a WM_CLASS
    auto app = cache->lookup_by_wm_class(app_id);
    if (app)
    {
        mir::log_info("Successfully resolved app id from wm_class, id=%s", app->id.c_str());
        return app->id;
    }

    for (auto const& q: app_id_to_desktop_file_quirks)
    {
        if (q.first == app_id)
        {
            mir::log_info("Successfully resolved app id from quirk, id=%s", q.second.c_str());
            return q.second;
        }
    }

    // Second, let's see if we can map it straight to a desktop file
    auto found = lookup_basename(app_id);
    if (found)
    {
        mir::log_info("Successfully resolved app id from basename, id=%s", found->id.c_str());
        return found->id;
    }

    // Third, lowercase it and try again
    auto lowercase_desktop_file = app_id;
    for(char &ch : lowercase_desktop_file)
        ch = std::tolower(ch);

    found = lookup_basename(lowercase_desktop_file);
    if (found)
    {
        mir::log_info("Successfully resolved app id from lowercase basename, id=%s", found->id.c_str());
        return found->id;
    }

    auto session = surface->session().lock();
    auto pid = session->process_id();
    auto socket_fd = session->socket_fd();

    // Fourth, check if the window belongs to snap
    found = resolve_if_snap(pid, socket_fd);
    if (found)
    {
        mir::log_info("Successfully resolved app id from snap, id=%s", found->id.c_str());
        return found->id;
    }

    // Fifth, check if the window belongs to flatpak
    found = resolve_if_flatpak(pid);
    if (found)
    {
        mir::log_info("Successfully resolved app id from flatpak, id=%s", found->id.c_str());
        return found->id;
    }

    // Sixth, get the exec command from our pid and see if we can match it to a GAppInfo's Exec
    found = resolve_if_executable_matches(pid);
    if (found)
    {
        mir::log_info("Successfully resolved app id from executable, id=%s", found->id.c_str());
        return found->id;
    }

    // NOTE: Here is the list of things that we aren't doing, as we don't have a good way of doing it:
    // 1. Resolving from the list of internally running apps using the PID
    // 2. Resolving via a startup notification
    // 3. Resolving from a GApplicationID, which GTK sends over DBUS
    mir::log_warning("Failed to resolve app id, returning %s", app_id.c_str());
    return app_id;
}

std::shared_ptr<mf::DesktopFile> mf::DesktopFileManager::lookup_basename(std::string& name)
{
    // Check if it's in the list of names
    auto app = cache->lookup_by_app_id(name);
    if (app)
        return app;

    // Otherwise, check if any of the special prefixes can find the app.
    std::vector<const char*> vendor_prefixes{
        "gnome-",
        "fedora-",
        "mozilla-",
        "debian-" };
    for (auto prefix : vendor_prefixes)
    {
        auto prefixed = prefix + name;
        app = cache->lookup_by_app_id(prefixed);
        if (app)
            return app;
    }

    return nullptr;
}

std::string mf::DesktopFileManager::parse_snap_security_profile_to_desktop_id(std::string const& contents)
{
    // We are reading the security profile here, which comes to us in the form:
    //      snap.name-space.binary-name (current).
    char const* const snap_security_label_prefix = "snap.";
    if (!contents.starts_with(snap_security_label_prefix))
        return "";

    // Get the contents after snap. and before the security annotation (denoted by a space)
    auto const contents_start_index = strlen (snap_security_label_prefix);
    auto contents_end_index = contents.find_first_of(' ', contents_start_index);
    if (contents_end_index == std::string::npos)
        contents_end_index = contents.size();
    std::string sandboxed_app_id = contents.substr(contents_start_index, contents_end_index - contents_start_index);
    for (auto& c : sandboxed_app_id)
        if (c == '.')
            c = '_';

    return sandboxed_app_id;
}

std::shared_ptr<mf::DesktopFile> mf::DesktopFileManager::resolve_if_snap(int pid, mir::Fd const& socket_fd)
{
#ifdef MIR_USE_APPARMOR
    // First, try to resolve the AppArmor profile
    char* label_cstr;
    char* mode_cstr;

    if (aa_getpeercon(socket_fd, &label_cstr, &mode_cstr) >= 0)
    {
        mir::log_info("Attempting to resolve desktop file via AppArmor for pid: %d", pid);
        std::string const label{label_cstr};
        free(label_cstr);
        // mode_cstr should NOT be freed, as it's from the same buffer as label_cstr

        auto sandboxed_app_id = parse_snap_security_profile_to_desktop_id(label);
        if (!sandboxed_app_id.empty())
        {
            if (auto file = cache->lookup_by_app_id(sandboxed_app_id))
            {
                mir::log_info("Successfully resolved desktop file via AppArmor for pid: %d", pid);
                return file;
            }
        }
    }
    else
    {
        mir::log_info("Unable to connect to AppArmor while resolving snap desktop file");
    }
#else
    (void)socket_fd;
    mir::log_warning("Unable to use AppArmor to resolve snap desktop file");
#endif

    // If that fails, try to read /proc/<PID>/attr/current
    mir::log_info("Attempting to resolve desktop file via proc directory for pid: %d", pid);
    std::string attr_file = "/proc/" + std::to_string(pid) + "/attr/current";
    if (!std::filesystem::exists(attr_file))
    {
        mir::log_warning("Failed to resolve desktop file via proc directory for pid %d: %s does not exist", pid, attr_file.c_str());
        return nullptr;
    }

    std::string contents;
    std::getline(std::ifstream(attr_file), contents, '\0');

    auto sandboxed_app_id = parse_snap_security_profile_to_desktop_id(contents);
    if (sandboxed_app_id.empty())
    {
        mir::log_info("Failed to resolve desktop file from sandboxed_app_id  for pid %d", pid);
        return nullptr;
    }

    // Now we will have something like firefox_firefox, for example.
    // This should match something in the app ID map.
    auto file = cache->lookup_by_app_id(sandboxed_app_id);
    if (file)
    {
        mir::log_info("Successfully resolved desktop file via proc directory for pid: %d", pid);
        return file;
    }

    return nullptr;
}

std::shared_ptr<mf::DesktopFile> mf::DesktopFileManager::resolve_if_flatpak(int pid)
{
    g_autoptr(GKeyFile) key_file = g_key_file_new();
    g_autofree char * info_filename = g_strdup_printf ("/proc/%d/root/.flatpak-info", pid);

    if (!g_key_file_load_from_file(key_file, info_filename, G_KEY_FILE_NONE, NULL))
        return nullptr;

    char* sandboxed_app_id = g_key_file_get_string(key_file, "Application", "name", NULL);
    if (!sandboxed_app_id)
        return nullptr;

    return cache->lookup_by_app_id(std::string(sandboxed_app_id));
}

std::shared_ptr<mf::DesktopFile> mf::DesktopFileManager::resolve_if_executable_matches(int pid)
{
    std::string proc_file = "/proc/" + std::to_string(pid) + "/cmdline";
    if (!std::filesystem::exists(proc_file))
        return nullptr;

    // First, read the executable name.
    std::string cmdline_call;
    std::getline(std::ifstream(proc_file), cmdline_call, '\0');

    auto file = cache->lookup_by_exec_string(cmdline_call);
    if (file)
        return file;

    // Then, try removing the path to just the exe name
    cmdline_call = std::filesystem::path(cmdline_call).filename();
    return cache->lookup_by_exec_string(cmdline_call);
}