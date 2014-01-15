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


#ifndef MIR_EXAMPLES_SELECT_PIXEL_FORMAT_H
#define MIR_EXAMPLES_SELECT_PIXEL_FORMAT_H

#include "mir/graphics/display_configuration_policy.h"

#include <memory>

namespace mir
{
namespace examples
{

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

}
}

#endif
