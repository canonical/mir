/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_GRAPHICS_MESA_REAL_KMS_DISPLAY_CONFIGURATION_H_
#define MIR_GRAPHICS_MESA_REAL_KMS_DISPLAY_CONFIGURATION_H_

#include "kms_display_configuration.h"

#include <xf86drmMode.h>

namespace mir
{
namespace graphics
{
namespace mesa
{

class RealKMSDisplayConfiguration : public KMSDisplayConfiguration
{
friend bool compatible(RealKMSDisplayConfiguration const& conf1, RealKMSDisplayConfiguration const& conf2);

public:
    RealKMSDisplayConfiguration(int drm_fd);
    RealKMSDisplayConfiguration(RealKMSDisplayConfiguration const& conf);
    RealKMSDisplayConfiguration& operator=(RealKMSDisplayConfiguration const& conf);

    void for_each_card(std::function<void(DisplayConfigurationCard const&)> f) const override;
    void for_each_output(std::function<void(DisplayConfigurationOutput const&)> f) const override;
    void for_each_output(std::function<void(UserDisplayConfigurationOutput&)> f) override;

    uint32_t get_kms_connector_id(DisplayConfigurationOutputId id) const override;
    size_t get_kms_mode_index(DisplayConfigurationOutputId id, size_t conf_mode_index) const override;
    void update() override;

private:
    void add_or_update_output(DRMModeResources const& resources, drmModeConnector const& connector);
    std::vector<DisplayConfigurationOutput>::iterator find_output_with_id(DisplayConfigurationOutputId id);
    std::vector<DisplayConfigurationOutput>::const_iterator find_output_with_id(DisplayConfigurationOutputId id) const;

    int drm_fd;
    DisplayConfigurationCard card;
    std::vector<DisplayConfigurationOutput> outputs;
};

bool compatible(RealKMSDisplayConfiguration const& conf1, RealKMSDisplayConfiguration const& conf2);

}
}
}

#endif /* MIR_GRAPHICS_MESA_REAL_KMS_DISPLAY_CONFIGURATION_H_ */
