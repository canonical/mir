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

#ifndef MIR_GRAPHICS_MESA_DRM_MODE_RESOURCES_H_
#define MIR_GRAPHICS_MESA_DRM_MODE_RESOURCES_H_

#include <xf86drm.h>
#include <xf86drmMode.h>

#include <memory>
#include <functional>

namespace mir
{
namespace graphics
{
namespace mesa
{

typedef std::unique_ptr<drmModeCrtc,std::function<void(drmModeCrtc*)>> DRMModeCrtcUPtr;
typedef std::unique_ptr<drmModeEncoder,std::function<void(drmModeEncoder*)>> DRMModeEncoderUPtr;
typedef std::unique_ptr<drmModeConnector,std::function<void(drmModeConnector*)>> DRMModeConnectorUPtr;
typedef std::unique_ptr<drmModeRes,std::function<void(drmModeRes*)>> DRMModeResUPtr;

class DRMModeResources
{
public:
    explicit DRMModeResources(int drm_fd);

    void for_each_connector(std::function<void(DRMModeConnectorUPtr)> const& f) const;
    void for_each_encoder(std::function<void(DRMModeEncoderUPtr)> const& f) const;
    void for_each_crtc(std::function<void(DRMModeCrtcUPtr)> const& f) const;

    size_t num_connectors();
    size_t num_encoders();
    size_t num_crtcs();

    DRMModeConnectorUPtr connector(uint32_t id) const;
    DRMModeEncoderUPtr encoder(uint32_t id) const;
    DRMModeCrtcUPtr crtc(uint32_t id) const;

private:
    int const drm_fd;
    DRMModeResUPtr const resources;
};

}
}
}

#endif /* MIR_GRAPHICS_MESA_DRM_MODE_RESOURCES_H_ */
