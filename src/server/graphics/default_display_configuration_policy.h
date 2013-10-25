/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_GRAPHICS_DEFAULT_DISPLAY_CONFIGURATION_POLICY_H_
#define MIR_GRAPHICS_DEFAULT_DISPLAY_CONFIGURATION_POLICY_H_

#include "mir/graphics/display_configuration_policy.h"

namespace mir
{
namespace graphics
{

class DefaultDisplayConfigurationPolicy : public DisplayConfigurationPolicy
{
public:
    void apply_to(DisplayConfiguration& conf);
};

}
}

#endif /* MIR_GRAPHICS_DEFAULT_DISPLAY_CONFIGURATION_POLICY_H_ */
