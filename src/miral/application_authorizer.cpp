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

#include <miral/application_authorizer.h>

#include <mir/frontend/session_credentials.h>
#include <mir/frontend/session_authorizer.h>
#include <mir/server.h>
#include <string>

namespace mf = mir::frontend;

namespace
{
struct SessionAuthorizerAdapter : mf::SessionAuthorizer
{
    SessionAuthorizerAdapter(std::shared_ptr<miral::ApplicationAuthorizer> const& app_auth) :
        app_auth{app_auth}
    {}

    virtual bool connection_is_allowed(mf::SessionCredentials const& creds) override
    {
        return app_auth->connection_is_allowed(creds);
    }

    std::shared_ptr<miral::ApplicationAuthorizer> const app_auth;
};
}

miral::ApplicationAuthorizer::ApplicationAuthorizer() = default;
miral::ApplicationAuthorizer::~ApplicationAuthorizer() = default;

struct miral::BasicSetApplicationAuthorizer::Self
{
    Self(std::function<std::shared_ptr<ApplicationAuthorizer>()> const& builder) :
        builder{builder} {}

    std::function<std::shared_ptr<ApplicationAuthorizer>()> const builder;
    std::weak_ptr<ApplicationAuthorizer> my_authorizer;
};


miral::BasicSetApplicationAuthorizer::BasicSetApplicationAuthorizer(
    std::function<std::shared_ptr<ApplicationAuthorizer>()> const& builder) :
    self{std::make_shared<Self>(builder)}
{
}

miral::BasicSetApplicationAuthorizer::~BasicSetApplicationAuthorizer() = default;

void miral::BasicSetApplicationAuthorizer::operator()(mir::Server& server)
{
    server.override_the_session_authorizer([this]()
        -> std::shared_ptr<mf::SessionAuthorizer>
        {
            auto wrapped = self->builder();
            self->my_authorizer = wrapped;
            return std::make_shared<SessionAuthorizerAdapter>(wrapped);
        });
}

auto miral::BasicSetApplicationAuthorizer::the_application_authorizer() const -> std::shared_ptr<ApplicationAuthorizer>
{
    return self->my_authorizer.lock();
}

miral::ApplicationCredentials::ApplicationCredentials(mir::frontend::SessionCredentials const& creds) :
    creds{creds} {}

auto miral::ApplicationCredentials::pid() const -> pid_t
{
    return creds.pid();
}

auto miral::ApplicationCredentials::uid() const -> uid_t
{
    return creds.uid();
}

auto miral::ApplicationCredentials::gid() const -> gid_t
{
    return creds.gid();
}

auto miral::ApplicationCredentials::apparmor_label() const -> std::string
{
    return creds.apparmor_label();
}

auto miral::ApplicationCredentials::is_sandboxed() const -> bool
{
    return creds.is_sandboxed();
}

auto miral::ApplicationCredentials::snap_name() const -> std::optional<std::string>
{
    return creds.snap_info().transform([](auto const& info) {
        return info.snap_name;
    });
}

auto miral::ApplicationCredentials::snap_app_name() const -> std::optional<std::string>
{
    return creds.snap_info().transform([](auto const& info) {
        return info.app_name;
    });
}

auto miral::ApplicationCredentials::flatpak_id() const -> std::optional<std::string>
{
    return creds.flatpak_info().transform([](auto const& info) {
        return info.app_id;
    });
}
