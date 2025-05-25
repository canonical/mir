/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "kms-utils/drm_mode_resources.h"

#include "mir/geometry/size.h"
#include "mir/test/doubles/mock_drm.h"

#include <unordered_set>
#include <array>
#include <vector>
#include <tuple>

#include <boost/throw_exception.hpp>

#include <fcntl.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mtd = mir::test::doubles;
namespace mgk = mir::graphics::kms;

TEST(DRMModeResources, can_access_plane_properties)
{
    using namespace testing;

    NiceMock<mtd::MockDRM> mock_drm;

    char const* const drm_device = "/dev/dri/card0";
    uint32_t plane_id = 0x10;
    uint32_t crtc_id = 0x20;
    uint32_t fb_id = 0x30;
    auto foo_prop_id = mock_drm.add_property(drm_device, "FOO");
    auto type_prop_id = mock_drm.add_property(drm_device, "type");
    auto crtc_id_prop_id = mock_drm.add_property(drm_device, "CRTC_ID");
    mock_drm.add_plane(drm_device, {0}, plane_id, crtc_id, fb_id,
                       0, 0, 0, 0, 0xff, 0,
                       {foo_prop_id, type_prop_id, crtc_id_prop_id},
                       {0xF00, DRM_PLANE_TYPE_CURSOR, 29});

    auto const drm_fd = open(drm_device, 0, 0);
    mgk::ObjectProperties plane_props{drm_fd, plane_id, DRM_MODE_OBJECT_PLANE};

    EXPECT_THAT(plane_props["type"],
                Eq(static_cast<unsigned>(DRM_PLANE_TYPE_CURSOR)));
    EXPECT_THAT(plane_props.id_for("CRTC_ID"), Eq(crtc_id_prop_id));
}
