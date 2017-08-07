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

#ifndef MIR_GRAPHICS_COMMON_KMS_UTILS_KMS_CONNECTOR_H_
#define MIR_GRAPHICS_COMMON_KMS_UTILS_KMS_CONNECTOR_H_

#include "drm_mode_resources.h"

#include <string>
#include <vector>
#include <xf86drmMode.h>

namespace mir
{
namespace graphics
{
namespace kms
{
std::string connector_name(DRMModeConnectorUPtr const& connector);

/**
 * Finds the first available CRTC that can drive Connector
 *
 * \note    This only finds the first available connector. It does not
 *          check whether currently-assigned resources could be reassigned,
 *          so it's possible for this to fail even if it would be possible for
 *          the total configuration to be set given global knowledge.
 * \param [in]  drm_fd      File descriptor to DRM node
 * \param [in]  connector   Connector to find a CRTC for.
 * \returns     The first available CRTC which can display an image on connector.
 *              The returned UPtr is guaranteed non-null.
 * \throws      A std::runtime_error if there are no available CRTCs.
 */
DRMModeCrtcUPtr find_crtc_for_connector(
    int drm_fd,
    DRMModeConnectorUPtr const& connector);

std::pair<DRMModeCrtcUPtr, DRMModePlaneUPtr> find_crtc_with_primary_plane(
    int drm_fd,
    DRMModeConnectorUPtr const& connector);
}
}
}

#endif //MIR_GRAPHICS_COMMON_KMS_UTILS_KMS_CONNECTOR_H
