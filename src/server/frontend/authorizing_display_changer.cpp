/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "authorizing_display_changer.h"
#include "mir/client_visible_error.h"
#include "mir_toolkit/client_types.h"
#include <boost/throw_exception.hpp>

namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace ms = mir::scene;

namespace
{
class UnauthorizedConfigurationRequest : public mir::ClientVisibleError
{
public:
    UnauthorizedConfigurationRequest()
        : mir::ClientVisibleError("Not authorized to set base display configuration")
    {
    }
    MirErrorDomain domain() const noexcept override
    {
        return mir_error_domain_display_configuration;
    }
    uint32_t code() const noexcept override
    {
        return mir_display_configuration_error_unauthorized;
    }
};
}

mf::AuthorizingDisplayChanger::AuthorizingDisplayChanger(
    std::shared_ptr<frontend::DisplayChanger> const& changer,
    bool configuration_is_authorized,
    bool base_configuration_modification_is_authorized)
    : changer(changer),
      configure_display_is_allowed{configuration_is_authorized},
      set_base_configuration_is_allowed{base_configuration_modification_is_authorized}
{
}

std::shared_ptr<mg::DisplayConfiguration> mf::AuthorizingDisplayChanger::base_configuration()
{
    return changer->base_configuration();
}

void mf::AuthorizingDisplayChanger::configure(
    std::shared_ptr<ms::Session> const& session,
    std::shared_ptr<mg::DisplayConfiguration> const& config)
{
    if (configure_display_is_allowed)
        changer->configure(session, config);
    else
        BOOST_THROW_EXCEPTION(std::runtime_error("not authorized to apply display configurations"));
}

void mf::AuthorizingDisplayChanger::remove_session_configuration(
    std::shared_ptr<ms::Session> const& session)
{
    if (configure_display_is_allowed)
        changer->remove_session_configuration(session);
    else
        BOOST_THROW_EXCEPTION(std::runtime_error("not authorized to remove display configurations"));
}

void mf::AuthorizingDisplayChanger::set_base_configuration(
    std::shared_ptr<graphics::DisplayConfiguration> const& config)
{
    if (set_base_configuration_is_allowed)
        changer->set_base_configuration(config);
    else
        BOOST_THROW_EXCEPTION(std::runtime_error("not authorized to set base display configurations"));
}

void mf::AuthorizingDisplayChanger::preview_base_configuration(
    std::weak_ptr<ms::Session> const& session,
    std::shared_ptr<graphics::DisplayConfiguration> const& config,
    std::chrono::seconds timeout)
{
    if (set_base_configuration_is_allowed)
    {
        changer->preview_base_configuration(session, config, timeout);
    }
    else
    {
        BOOST_THROW_EXCEPTION(UnauthorizedConfigurationRequest{});
    }
}

void mf::AuthorizingDisplayChanger::confirm_base_configuration(
    std::shared_ptr<ms::Session> const& session,
    std::shared_ptr<graphics::DisplayConfiguration> const& config)
{
    if (set_base_configuration_is_allowed)
    {
        changer->confirm_base_configuration(session, config);
    }
    else
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("not authorized to set base display configurations"));
    }
}

void
mf::AuthorizingDisplayChanger::cancel_base_configuration_preview(std::shared_ptr<ms::Session> const& session)
{
    // There's no particular reason to require authorisation here - it only takes effect if the client
    // has already been authorised to change configuration.
    changer->cancel_base_configuration_preview(session);
}
