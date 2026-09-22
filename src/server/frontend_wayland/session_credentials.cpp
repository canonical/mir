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
#include <mir/fd.h>
#include <mir/log.h>

#include <gio/gdesktopappinfo.h>

#include <string>
#include <cstring>
#ifdef MIR_USE_APPARMOR
#include <sys/apparmor.h>
#endif
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

namespace mf = mir::frontend;

mf::SessionCredentials::SessionCredentials(SessionCredentials&&) = default;
mf::SessionCredentials& mf::SessionCredentials::operator=(SessionCredentials&&) = default;

mf::SessionCredentials::SessionCredentials(Fd const& client_sock)
{
    struct ucred cred;
    socklen_t cred_len = sizeof cred;

    if (getsockopt(client_sock, SOL_SOCKET, SO_PEERCRED, &cred, &cred_len) < 0)
    {
        mir::log_debug("Failed to read socket peer credentials, falling back to get(uid,gid)");
        cred.pid = 0;
        cred.uid = getuid();
        cred.gid = getgid();
    }
    the_pid = cred.pid;
    the_uid = cred.uid;
    the_gid = cred.gid;

#ifdef MIR_USE_APPARMOR
    char *label_cstr = nullptr;
    if (aa_getpeercon(client_sock, &label_cstr, nullptr) >= 0)
    {
        the_apparmor_label = label_cstr;
        std::free(label_cstr);
    }
    else
    {
        mir::log_info("Unable to determine AppArmor label for Wayland client");
    }
#endif

    sandbox_info = detect_sandbox_info(the_pid, the_apparmor_label);
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

    sandbox_info = detect_sandbox_info(the_pid, the_apparmor_label);
}

mf::SessionCredentials::SessionCredentials(
    pid_t pid,
    uid_t uid,
    gid_t gid,
    std::string apparmor_label,
    SandboxInfo const& sandbox_info)
    : the_pid{pid}, the_uid{uid}, the_gid{gid}, the_apparmor_label{apparmor_label}, sandbox_info{sandbox_info}
{
}

auto mf::SessionCredentials::detect_sandbox_info(pid_t pid, std::string const& apparmor_label) -> SandboxInfo
{
    SandboxInfo info = resolve_if_snap(apparmor_label);
    if (std::holds_alternative<std::monostate>(info))
    {
        info = resolve_if_flatpak(pid);
    }
    return info;
}

auto mf::SessionCredentials::resolve_if_snap(std::string const& apparmor_label) -> SandboxInfo
{
    // We are reading the security profile here, which comes to us in the form:
    //      snap.name-space.binary-name
    char const* const snap_security_label_prefix = "snap.";
    if (apparmor_label.starts_with(snap_security_label_prefix))
    {
        // Get the contents after snap. and before the security annotation (denoted by a space)
        auto const snap_start_index = std::strlen (snap_security_label_prefix);
        auto snap_end_index = apparmor_label.find_first_of('.', snap_start_index);
        if (snap_end_index != std::string::npos)
        {
            return SnapInfo{
                apparmor_label.substr(snap_start_index, snap_end_index - snap_start_index),
                apparmor_label.substr(snap_end_index+1),
            };
        }
    }
    return std::monostate{};
}

auto mf::SessionCredentials::resolve_if_flatpak(pid_t pid) -> SandboxInfo {
    g_autoptr(GKeyFile) key_file = g_key_file_new();
    g_autofree char * info_filename = g_strdup_printf ("/proc/%d/root/.flatpak-info", pid);

    if (!g_key_file_load_from_file(key_file, info_filename, G_KEY_FILE_NONE, nullptr))
        return std::monostate{};

    g_autofree char* flatpak_id = g_key_file_get_string(key_file, "Application", "name", nullptr);
    if (flatpak_id)
    {
        return FlatpakInfo{flatpak_id};
    }

    return std::monostate{};
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
