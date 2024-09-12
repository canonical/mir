/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_GRAPHICS_GBM_KMS_DISPLAY_CONFIGURATION_H_
#define MIR_GRAPHICS_GBM_KMS_DISPLAY_CONFIGURATION_H_

#include "mir/graphics/display_configuration.h"
#include <memory>

namespace mir
{
namespace graphics
{
namespace atomic
{
class KMSOutput;

class DRMModeResources;

class KMSDisplayConfiguration : public DisplayConfiguration
{
public:
    virtual std::shared_ptr<KMSOutput> get_output_for(DisplayConfigurationOutputId id) const = 0;
    virtual size_t get_kms_mode_index(
        DisplayConfigurationOutputId id,
        size_t conf_mode_index) const = 0;
    virtual void update() = 0;
};

}
}
}

#endif /* MIR_GRAPHICS_GBM_KMS_DISPLAY_CONFIGURATION_H_ */
