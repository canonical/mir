/*
 * Copyright © 2013 Canonical Ltd.
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
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_GRAPHICS_MESA_REAL_KMS_DISPLAY_CONFIGURATION_H_
#define MIR_GRAPHICS_MESA_REAL_KMS_DISPLAY_CONFIGURATION_H_

#include "kms_display_configuration.h"
#include "kms-utils/drm_mode_resources.h"

#include <xf86drmMode.h>

namespace mir
{
namespace graphics
{
namespace mesa
{
class KMSOutput;
class KMSOutputContainer;

class RealKMSDisplayConfiguration : public KMSDisplayConfiguration
{
friend bool compatible(RealKMSDisplayConfiguration const& conf1, RealKMSDisplayConfiguration const& conf2);

public:
    RealKMSDisplayConfiguration(std::shared_ptr<KMSOutputContainer> const& displays);
    RealKMSDisplayConfiguration(RealKMSDisplayConfiguration const& conf);
    RealKMSDisplayConfiguration& operator=(RealKMSDisplayConfiguration const& conf);

    void for_each_card(std::function<void(DisplayConfigurationCard const&)> f) const override;
    void for_each_output(std::function<void(DisplayConfigurationOutput const&)> f) const override;
    void for_each_output(std::function<void(UserDisplayConfigurationOutput&)> f) override;
    std::unique_ptr<DisplayConfiguration> clone() const override;

    std::shared_ptr<KMSOutput> get_output_for(DisplayConfigurationOutputId id) const override;
    size_t get_kms_mode_index(DisplayConfigurationOutputId id, size_t conf_mode_index) const override;
    void update() override;

private:

    std::shared_ptr<KMSOutputContainer> displays;
    DisplayConfigurationCard card;
    typedef std::pair<DisplayConfigurationOutput, std::shared_ptr<KMSOutput>> Output;
    Output const& output(DisplayConfigurationOutputId id) const;
    std::vector<Output> outputs;
};

bool compatible(RealKMSDisplayConfiguration const& conf1, RealKMSDisplayConfiguration const& conf2);

}
}
}

#endif /* MIR_GRAPHICS_MESA_REAL_KMS_DISPLAY_CONFIGURATION_H_ */
