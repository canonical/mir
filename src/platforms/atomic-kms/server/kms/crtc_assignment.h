/*
 * Copyright © Canonical Ltd.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_GRAPHICS_ATOMIC_KMS_CRTC_ASSIGNMENT_H_
#define MIR_GRAPHICS_ATOMIC_KMS_CRTC_ASSIGNMENT_H_

#include <mir/graphics/kms/drm_mode_resources.h>
#include <mir/graphics/display_configuration.h>
#include <mir/fd.h>

#include <map>

namespace mir
{
namespace graphics
{
namespace atomic
{
class RealKMSDisplayConfiguration;

struct CrtcAssignment
{
    kms::DRMModeCrtcUPtr crtc;
    kms::DRMModePlaneUPtr plane;
    CrtcAssignment(kms::DRMModeCrtcUPtr crtc, kms::DRMModePlaneUPtr plane);
};

auto solve_crtc_assignments(mir::Fd const& drm_fd, RealKMSDisplayConfiguration const& conf)
    -> std::map<mir::graphics::DisplayConfigurationOutputId, CrtcAssignment>;

}
}
}

#endif /* MIR_GRAPHICS_ATOMIC_KMS_CRTC_ASSIGNMENT_H_ */
