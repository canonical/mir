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

#ifndef MIRAL_APPLICATION_AUTHORIZER_H
#define MIRAL_APPLICATION_AUTHORIZER_H

#include <expected>
#include <sys/types.h>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <system_error>

namespace mir { class Server; }
namespace mir
{
namespace frontend { class SessionCredentials; }
}

namespace miral
{
class ApplicationCredentials
{
public:
    ApplicationCredentials(mir::frontend::SessionCredentials const& creds);

    pid_t pid() const;
    uid_t uid() const;
    gid_t gid() const;

    /**
     * The AppArmor label of the connecting process
     *
     * If Mir was built without AppArmor support this will return ENOTSUP.
     * Otherwise, returns the label of the connecting process or the error.
     */
    auto apparmor_label() const -> std::expected<std::string, std::error_code>;

    /**
     * Is the client running within an application sandbox?
     *
     * Processes running within application sandbox systems
     * such as Snap or Flatpak can be meaningfully identified
     * and their privileges limited.
     *
     * Connections from processes running within an application
     * sandbox should have more limited access to sensitive
     * Wayland protocols — applications are expected to use
     * Desktop Portals to acquire mediated access to any extra
     * functionality they require.
     *
     * \note This check has false-positives — it is possible for
     *        unprivileged unconfined processes to impersonate
     *        running in an application sandbox.
     *
     *        Because of this, this check should only be used
     *        to restrict permissions; it cannot be safely used
     *        to provide extra permissions only to sandboxed
     *        processes.
     */
    auto from_application_sandbox() const -> bool;

    /**
     * The name of the Snap this client is from
     *
     * \note The security guarantees of this are subtle.
     *        For any confined snap this will return the true name.
     *        However, any unconfined process — either outside of a snap,
     *        or from an unconfined snap — can impersonate any snap on
     *        the system.
     */
    auto snap_name() const -> std::optional<std::string>;
    /**
     * The name of the Flatpak this client is from
     */
    auto flatpak_id() const -> std::optional<std::string>;

private:
    ApplicationCredentials() = delete;

    mir::frontend::SessionCredentials const& creds;
};

class ApplicationAuthorizer
{
public:
    ApplicationAuthorizer();
    virtual ~ApplicationAuthorizer();
    ApplicationAuthorizer(ApplicationAuthorizer const&) = delete;
    ApplicationAuthorizer& operator=(ApplicationAuthorizer const&) = delete;

    virtual bool connection_is_allowed(ApplicationCredentials const& creds) = 0;
};

class BasicSetApplicationAuthorizer
{
public:
    explicit BasicSetApplicationAuthorizer(std::function<std::shared_ptr<ApplicationAuthorizer>()> const& builder);
    ~BasicSetApplicationAuthorizer();

    void operator()(mir::Server& server);
    auto the_application_authorizer() const -> std::shared_ptr<ApplicationAuthorizer>;

private:
    struct Self;
    std::shared_ptr<Self> self;
};

template<typename Policy>
    requires std::is_base_of_v<ApplicationAuthorizer, Policy>
class SetApplicationAuthorizer : public BasicSetApplicationAuthorizer
{
public:
    template<typename... Args>
        requires std::is_constructible_v<Policy, Args const&...>
    explicit SetApplicationAuthorizer(Args const&... args) :
        BasicSetApplicationAuthorizer{[&args...]() { return std::make_shared<Policy>(args...); }}
    {}

    auto the_custom_application_authorizer() const -> std::shared_ptr<Policy>
    { return std::static_pointer_cast<Policy>(the_application_authorizer()); }
};
}

#endif //MIRAL_APPLICATION_AUTHORIZER_H
