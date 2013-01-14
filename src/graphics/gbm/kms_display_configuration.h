/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_GRAPHICS_GBM_KMS_DISPLAY_CONFIGURATION_H_
#define MIR_GRAPHICS_GBM_KMS_DISPLAY_CONFIGURATION_H_

#include "mir/graphics/display_configuration.h"

#include <memory>

#include <xf86drmMode.h>

namespace mir
{
namespace graphics
{
namespace gbm
{

class KMSDisplayConfiguration : public DisplayConfiguration
{
public:
    KMSDisplayConfiguration(int drm_fd);

    void for_each_card(std::function<void(DisplayConfigurationCard&)> f);
    void for_each_output(std::function<void(DisplayConfigurationOutput&)> f);

private:
    void add_output(drmModeRes const& resources, drmModeConnector const& connector);

    int const drm_fd;
    std::vector<DisplayConfigurationOutput> outputs;
};

}
}
}

#endif /* MIR_GRAPHICS_GBM_KMS_DISPLAY_CONFIGURATION_H_ */
