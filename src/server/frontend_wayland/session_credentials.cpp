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

#include <wayland-server-core.h>

#ifdef MIR_USE_APPARMOR
#include <sys/apparmor.h>
#endif
#include <sys/stat.h>

namespace mf = mir::frontend;

mf::SessionCredentials::SessionCredentials(pid_t pid, uid_t uid, gid_t gid, std::string&& apparmor_label) :
    the_pid{pid},
    the_uid{uid},
    the_gid{gid},
    the_apparmor_label{std::move(apparmor_label)}
{
}

mf::SessionCredentials::SessionCredentials(SessionCredentials&&) = default;

mf::SessionCredentials::SessionCredentials(wl_client* client) :
    SessionCredentials{from_client(client)}
{
}

mf::SessionCredentials::SessionCredentials(pid_t pid) :
    SessionCredentials{from_pid(pid)}
{
}

auto mf::SessionCredentials::from_client(wl_client* client) -> SessionCredentials
{
    pid_t pid;
    uid_t uid;
    gid_t gid;
    std::string apparmor_label;

    wl_client_get_credentials(client, &pid, &uid, &gid);

#ifdef MIR_USE_APPARMOR
    char *label_cstr = nullptr;
    if (aa_getpeercon(wl_client_get_fd(client), &label_cstr, nullptr) >= 0)
    {
        apparmor_label = label_cstr;
        std::free(label_cstr);
    }
    else
    {
        mir::log_info("Unable to determine AppArmor label for Wayland client");
    }
#endif

    return {pid, uid, gid, std::move(apparmor_label)};
}

auto mf::SessionCredentials::from_pid(pid_t client_pid) -> SessionCredentials
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

    std::string apparmor_label;
#ifdef MIR_USE_APPARMOR
    char *label_cstr = nullptr;
    if (aa_gettaskcon(client_pid, &label_cstr, nullptr) >= 0)
    {
        apparmor_label = label_cstr;
        std::free(label_cstr);
    }
    else
    {
        mir::log_info("Unable to determine AppArmor label for pid");
    }
#endif

    return {client_pid, proc_stat.st_uid, proc_stat.st_gid, std::move(apparmor_label)};
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
