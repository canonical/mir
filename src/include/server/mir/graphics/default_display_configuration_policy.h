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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_GRAPHICS_DEFAULT_DISPLAY_CONFIGURATION_POLICY_H_
#define MIR_GRAPHICS_DEFAULT_DISPLAY_CONFIGURATION_POLICY_H_

#include "mir/graphics/display_configuration_policy.h"

namespace mir
{
namespace graphics
{
/** @name default DisplayConfigurationPolicy options.
 * Some simple default implementations: Mir will default to CloneDisplayConfigurationPolicy,
 * The others are plausible defaults for display servers that don't want to do anything
 * more sophisticated.
 * @{ */
/// All screens placed at (0, 0)
class CloneDisplayConfigurationPolicy : public DisplayConfigurationPolicy
{
public:
    CloneDisplayConfigurationPolicy() = default;
    CloneDisplayConfigurationPolicy(double scale)
        : scale{scale}
    {
    }

    void apply_to(DisplayConfiguration& conf);

private:
    double const scale{1.0};
};

/// Each screen placed to the right of the previous one
class SideBySideDisplayConfigurationPolicy : public DisplayConfigurationPolicy
{
public:
    SideBySideDisplayConfigurationPolicy() = default;
    SideBySideDisplayConfigurationPolicy(double scale)
        : scale{scale}
    {
    }

    void apply_to(graphics::DisplayConfiguration& conf);

private:
    double const scale{1.0};
};

/// Just use the first screen
class SingleDisplayConfigurationPolicy : public DisplayConfigurationPolicy
{
public:
    SingleDisplayConfigurationPolicy() = default;
    SingleDisplayConfigurationPolicy(double scale)
        : scale{scale}
    {
    }

    void apply_to(graphics::DisplayConfiguration& conf);

private:
    double const scale{1.0};
};
/** @} */
}
}

#endif /* MIR_GRAPHICS_DEFAULT_DISPLAY_CONFIGURATION_POLICY_H_ */
