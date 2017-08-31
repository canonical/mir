/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_EXAMPLE_DISPLAY_CONFIGURATION_POLICY_H_
#define MIR_EXAMPLE_DISPLAY_CONFIGURATION_POLICY_H_

#include "mir/graphics/display_configuration_policy.h"

#include <memory>

namespace mir
{
class Server;

namespace examples
{
extern char const* const display_config_opt;
extern char const* const display_config_descr;
extern char const* const clone_opt_val;
extern char const* const sidebyside_opt_val;
extern char const* const single_opt_val;
extern char const* const display_alpha_opt;
extern char const* const display_alpha_descr;
extern char const* const display_alpha_off;
extern char const* const display_alpha_on;

/**
 * \brief Example of a DisplayConfigurationPolicy that tries to find
 * an opaque or transparent pixel format, or falls back to the default
 * if not found.
 */
class PixelFormatSelector : public graphics::DisplayConfigurationPolicy
{
public:
    PixelFormatSelector(std::shared_ptr<graphics::DisplayConfigurationPolicy> const& base_policy,
                        bool with_alpha);
    virtual void apply_to(graphics::DisplayConfiguration& conf);
private:
    std::shared_ptr<graphics::DisplayConfigurationPolicy> const base_policy;
    bool const with_alpha;
};

void add_display_configuration_options_to(Server& server);
}
}

#endif /* MIR_EXAMPLE_DISPLAY_CONFIGURATION_POLICY_H_ */
