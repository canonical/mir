/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 */

#include <mir/frontend/session_credentials.h>
#include <mir/log.h>

#include <gio/gdesktopappinfo.h>
#include <string>
#include <wayland-server-core.h>

#include <cstring>
#ifdef MIR_USE_APPARMOR
#include <sys/apparmor.h>
#endif
#include <sys/stat.h>

namespace mf = mir::frontend;

mf::SessionCredentials::SessionCredentials(SessionCredentials&&) = default;

mf::SessionCredentials::SessionCredentials(wl_client* client)
{
    wl_client_get_credentials(client, &the_pid, &the_uid, &the_gid);

#ifdef MIR_USE_APPARMOR
    char *label_cstr = nullptr;
    if (aa_getpeercon(wl_client_get_fd(client), &label_cstr, nullptr) >= 0)
    {
        the_apparmor_label = label_cstr;
        std::free(label_cstr);
    }
    else
    {
        mir::log_info("Unable to determine AppArmor label for Wayland client");
    }
#endif

    detect_sandbox_info();
}

mf::SessionCredentials::SessionCredentials(pid_t client_pid)
{
    auto const proc = "/proc/" + std::to_string(client_pid);

    struct stat proc_stat{};
    if (stat(proc.c_str(), &proc_stat) == -1)
    {
        log_debug("Failed to get uid & gid for PID %d using stat(%s, ...), falling back to get(uid,gid)",
                  client_pid, proc.c_str());

        proc_stat.st_uid = getuid();
        proc_stat.st_gid = getgid();
    }
    the_pid = client_pid;
    the_uid = proc_stat.st_uid;
    the_gid = proc_stat.st_gid;

#ifdef MIR_USE_APPARMOR
    char *label_cstr = nullptr;
    if (aa_gettaskcon(client_pid, &label_cstr, nullptr) >= 0)
    {
        the_apparmor_label = label_cstr;
        std::free(label_cstr);
    }
    else
    {
        mir::log_info("Unable to determine AppArmor label for pid");
    }
#endif

    detect_sandbox_info();
}

void mf::SessionCredentials::detect_sandbox_info()
{
    if (resolve_if_snap())
        return;
    resolve_if_flatpak();
}

bool mf::SessionCredentials::resolve_if_snap()
{
    // We are reading the security profile here, which comes to us in the form:
    //      snap.name-space.binary-name
    char const* const snap_security_label_prefix = "snap.";
    if (the_apparmor_label.starts_with(snap_security_label_prefix))
    {
        // Get the contents after snap. and before the security annotation (denoted by a space)
        auto const snap_start_index = std::strlen (snap_security_label_prefix);
        auto snap_end_index = the_apparmor_label.find_first_of('.', snap_start_index);
        if (snap_end_index != std::string::npos)
        {
            sandbox_info = SnapInfo{
                the_apparmor_label.substr(snap_start_index, snap_end_index - snap_start_index),
                the_apparmor_label.substr(snap_end_index+1),
            };
            return true;
        }
    }
    return false;
}

bool mf::SessionCredentials::resolve_if_flatpak() {
    g_autoptr(GKeyFile) key_file = g_key_file_new();
    g_autofree char * info_filename = g_strdup_printf ("/proc/%d/root/.flatpak-info", the_pid);

    if (!g_key_file_load_from_file(key_file, info_filename, G_KEY_FILE_NONE, nullptr))
        return false;

    char* flatpak_id = g_key_file_get_string(key_file, "Application", "name", nullptr);
    if (flatpak_id)
    {
        sandbox_info = FlatpakInfo{flatpak_id};
        return true;
    }

    return false;
}

pid_t mf::SessionCredentials::pid() const
{
    return the_pid;
}

uid_t mf::SessionCredentials::uid() const
{
    return the_uid;
}

gid_t mf::SessionCredentials::gid() const
{
    return the_gid;
}

auto mf::SessionCredentials::apparmor_label() const -> std::string
{
    return the_apparmor_label;
}

auto mf::SessionCredentials::is_sandboxed() const -> bool
{
    return !std::holds_alternative<std::monostate>(sandbox_info);
}

auto mf::SessionCredentials::snap_info() const -> std::optional<SnapInfo>
{
    if (auto snap_info = std::get_if<SnapInfo>(&sandbox_info))
    {
        return *snap_info;
    }
    return std::nullopt;
}

auto mf::SessionCredentials::flatpak_info() const -> std::optional<FlatpakInfo>
{
    if (auto flatpak_info = std::get_if<FlatpakInfo>(&sandbox_info))
    {
        return *flatpak_info;
    }
    return std::nullopt;
}
