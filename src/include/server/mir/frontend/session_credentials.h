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

#ifndef MIR_FRONTEND_SESSION_CREDENTIALS_ID_H_
#define MIR_FRONTEND_SESSION_CREDENTIALS_ID_H_

#include <optional>
#include <string>
#include <variant>
#include <sys/types.h>

namespace mir
{
class Fd;
namespace frontend
{
class SessionCredentials
{
public:
    struct SnapInfo
    {
        std::string snap_name;
        std::string app_name;
    };
    struct FlatpakInfo
    {
        std::string app_id;
    };
    using SandboxInfo = std::variant<std::monostate,SnapInfo,FlatpakInfo>;

    explicit SessionCredentials(Fd const& client_sock);
    explicit SessionCredentials(pid_t client_pid);
    SessionCredentials(pid_t pid, uid_t uid, gid_t gid,
                       std::string apparmor_label,
                       SandboxInfo const& sandbox_info = std::monostate{});
    SessionCredentials(SessionCredentials&& creds);
    SessionCredentials& operator=(SessionCredentials&& creds);

    pid_t pid() const;
    uid_t uid() const;
    gid_t gid() const;

    auto apparmor_label() const -> std::string;
    auto is_sandboxed() const -> bool;
    auto snap_info() const -> std::optional<SnapInfo>;
    auto flatpak_info() const -> std::optional<FlatpakInfo>;

    static auto detect_sandbox_info(pid_t pid, std::string const& apparmor_label) -> SandboxInfo;

private:
    SessionCredentials() = delete;

    static auto resolve_if_snap(std::string const& apparmor_label) -> SandboxInfo;
    static auto resolve_if_flatpak(pid_t pid) -> SandboxInfo;

    pid_t the_pid;
    uid_t the_uid;
    gid_t the_gid;
    std::string the_apparmor_label;

    SandboxInfo sandbox_info;
};
}
}

#endif
