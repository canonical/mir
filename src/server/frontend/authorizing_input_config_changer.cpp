/*
 * Copyright © 2017 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "authorizing_input_config_changer.h"
#include "mir/input/mir_input_config.h"
#include "mir/client_visible_error.h"

#include "mir_toolkit/client_types.h"
#include <boost/throw_exception.hpp>

namespace mf = mir::frontend;

namespace
{
class UnauthorizedConfigurationRequest : public mir::ClientVisibleError
{
public:
    UnauthorizedConfigurationRequest(std::string const& readable_error, MirInputConfigurationError config_error)
        : mir::ClientVisibleError(readable_error), config_error(config_error)
    {
    }
    MirErrorDomain domain() const noexcept override
    {
        return mir_error_domain_input_configuration;
    }
    uint32_t code() const noexcept override
    {
        return config_error;
    }
    MirInputConfigurationError config_error;
};
}

mf::AuthorizingInputConfigChanger::AuthorizingInputConfigChanger(
    std::shared_ptr<InputConfigurationChanger> const& changer,
    bool configure_input_is_allowed,
    bool set_base_configuration_is_allowed)
    : changer{changer},
      configure_input_is_allowed{configure_input_is_allowed},
      set_base_configuration_is_allowed{set_base_configuration_is_allowed}
{
}

MirInputConfig mf::AuthorizingInputConfigChanger::base_configuration()
{
    return changer->base_configuration();
}

void mf::AuthorizingInputConfigChanger::configure(std::shared_ptr<scene::Session> const& session, MirInputConfig && config)
{
    if (configure_input_is_allowed)
        changer->configure(session, std::move(config));
    else
        BOOST_THROW_EXCEPTION(UnauthorizedConfigurationRequest("Not authorized to configure input devices",
                                                               mir_input_configuration_error_unauthorized));
}

void mf::AuthorizingInputConfigChanger::set_base_configuration(MirInputConfig && config)
{
    if (set_base_configuration_is_allowed)
        changer->set_base_configuration(std::move(config));
    else
        BOOST_THROW_EXCEPTION(
            UnauthorizedConfigurationRequest("Not authorized to configure input devices",
                                             mir_input_configuration_error_base_configuration_unauthorized));
}
