/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_EXAMPLES_TRANSLUCENT_SERVER_CONFIGURATION_H_
#define MIR_EXAMPLES_TRANSLUCENT_SERVER_CONFIGURATION_H_

#include "basic_server_configuration.h"

namespace mir
{
namespace examples
{

/**
 * \brief TranslucentServerConfiguration extends BasicServerConfiguration with a different pixel format selection
 */
class TranslucentServerConfiguration : public BasicServerConfiguration
{
public:
    TranslucentServerConfiguration(int argc, char const** argv);

    std::shared_ptr<graphics::DisplayConfigurationPolicy> the_display_configuration_policy();
};

}
}

#endif /* MIR_EXAMPLES_TRANSLUCENT_SERVER_CONFIGURATION_H_ */
