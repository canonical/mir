/*
 * Copyright © 2016 Canonical Ltd.
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
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "miral/application_authorizer.h"

#include <mir/frontend/session_credentials.h>
#include <mir/frontend/session_authorizer.h>
#include <mir/server.h>

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

    virtual bool configure_display_is_allowed(mf::SessionCredentials const& creds) override
    {
        return app_auth->configure_display_is_allowed(creds);
    }

    virtual bool set_base_display_configuration_is_allowed(mf::SessionCredentials const& creds) override
    {
        return app_auth->set_base_display_configuration_is_allowed(creds);
    }

    virtual bool screencast_is_allowed(mf::SessionCredentials const& creds) override
    {
        return app_auth->screencast_is_allowed(creds);
    }

    virtual bool prompt_session_is_allowed(mf::SessionCredentials const& creds) override
    {
        return app_auth->prompt_session_is_allowed(creds);
    }

    bool configure_input_is_allowed(mf::SessionCredentials const& creds) override
    {
        return app_auth->configure_input_is_allowed(creds);
    }

    bool set_base_input_configuration_is_allowed(mf::SessionCredentials const& creds) override
    {
        return app_auth->set_base_input_configuration_is_allowed(creds);
    }

    std::shared_ptr<miral::ApplicationAuthorizer> const app_auth;
};
}

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
    return creds.pid();
}

auto miral::ApplicationCredentials::gid() const -> gid_t
{
    return creds.pid();
}
