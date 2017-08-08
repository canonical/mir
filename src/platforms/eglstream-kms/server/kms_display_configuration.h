/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_PLATFORMS_EGLSTREAM_KMS_KMS_DISPLAY_CONFIGURATION_H_
#define MIR_PLATFORMS_EGLSTREAM_KMS_KMS_DISPLAY_CONFIGURATION_H_

#include "egl_output.h"

#include "mir/graphics/display_configuration.h"


#include <xf86drmMode.h>

namespace mir
{
namespace graphics
{
namespace eglstream
{

class KMSDisplayConfiguration : public DisplayConfiguration
{
public:
    KMSDisplayConfiguration(int drm_fd, EGLDisplay dpy);
    KMSDisplayConfiguration(KMSDisplayConfiguration const& conf);

    void for_each_card(std::function<void(DisplayConfigurationCard const&)> f) const override;
    void for_each_output(std::function<void(DisplayConfigurationOutput const&)> f) const override;
    void for_each_output(std::function<void(UserDisplayConfigurationOutput&)> f) override;
    std::unique_ptr<DisplayConfiguration> clone() const override;

    uint32_t get_kms_connector_id(DisplayConfigurationOutputId id) const;
    size_t get_kms_mode_index(DisplayConfigurationOutputId id, size_t conf_mode_index) const;
    void update();

    void for_each_output(std::function<void(kms::EGLOutput const&)> const& f) const;

private:
    void update_output(
        graphics::kms::DRMModeResources const& resources,
        graphics::kms::DRMModeConnectorUPtr const& connector,
        kms::EGLOutput& output);
    std::vector<std::shared_ptr<kms::EGLOutput>>::iterator find_output_with_id(DisplayConfigurationOutputId id);
    std::vector<std::shared_ptr<kms::EGLOutput>>::const_iterator find_output_with_id(DisplayConfigurationOutputId id) const;

    int drm_fd;
    DisplayConfigurationCard const card;
    std::vector<std::shared_ptr<kms::EGLOutput>> outputs;
};

}
}
}

#endif // MIR_PLATFORMS_EGLSTREAM_KMS_KMS_DISPLAY_CONFIGURATION_H_
