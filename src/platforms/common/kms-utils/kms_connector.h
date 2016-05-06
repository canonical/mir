/*
 * Copyright © 2016 Canonical Ltd.
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
std::string connector_name(mesa::DRMModeConnectorUPtr const& connector);

mesa::DRMModeCrtcUPtr find_crtc_for_connector(
    int drm_fd,
    mesa::DRMModeConnectorUPtr const& connector);

}
}
}

#endif //MIR_GRAPHICS_COMMON_KMS_UTILS_KMS_CONNECTOR_H
