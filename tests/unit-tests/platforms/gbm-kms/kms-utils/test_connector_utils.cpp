/*
 * Copyright © 2016 Canonical Ltd.
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
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "kms-utils/kms_connector.h"

#include "mir/geometry/size.h"
#include "mir/test/doubles/mock_drm.h"

#include <array>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <fcntl.h>

namespace mtd = mir::test::doubles;
namespace mgk = mir::graphics::kms;

//This cries out for property-based testing via a QuickCheck implementation,
//but there aren't any in the Ubuntu archive!
TEST(KMSConnectorHelper, finds_compatible_crtc_for_connectors)
{
    using namespace testing;
    NiceMock<mtd::MockDRM> drm;
    char const* const drm_device = "/dev/dri/card0";

    std::array<uint32_t, 3> const crtc_ids = {{21, 25, 30}};
    std::array<uint32_t, 3> const encoder_ids = {{3, 5, 7}};
    std::array<uint32_t, 3> const connector_id = {{9, 10, 11}};

    drm.reset(drm_device);
    // Add a bunch of CRTCs
    for (auto id : crtc_ids)
    {
        drm.add_crtc(drm_device, id, drmModeModeInfo());
    }
    // Add an encoder that can only drive any CRTC...
    drm.add_encoder(drm_device, encoder_ids[0], 0, 1 << 0 | 1 << 1 | 1 << 2);
    // ...and one that can only drive the third...
    drm.add_encoder(drm_device, encoder_ids[1], 0, 1 << 2);
    // ...and one that can only drive the second two...
    drm.add_encoder(drm_device, encoder_ids[2], 0, 1 << 1 | 1 << 2);

    std::vector<drmModeModeInfo> modes{
        mtd::FakeDRMResources::create_mode(
            1200,
            1600,
            138500,
            1400,
            1800,
            mtd::FakeDRMResources::ModePreference::PreferredMode)};
    // Finally, add a connector that can only be driven by any encoder...
    std::vector<uint32_t> any_encoder{encoder_ids.begin(), encoder_ids.end()};
    drm.add_connector(
        drm_device,
        connector_id[0],
        DRM_MODE_CONNECTOR_VGA,
        DRM_MODE_CONNECTED,
        0,
        modes,
        any_encoder,
        mir::geometry::Size{300, 200});

    // ...then one that can only be driven by the third...
    std::vector<uint32_t> third_encoder{encoder_ids[2]};
    drm.add_connector(
        drm_device,
        connector_id[1],
        DRM_MODE_CONNECTOR_VGA,
        DRM_MODE_CONNECTED,
        0,
        modes,
        third_encoder,
        mir::geometry::Size{300, 200});

    // ...and finally one that can only be driven by the second...
    std::vector<uint32_t> second_encoder{encoder_ids[1]};
    drm.add_connector(
        drm_device,
        connector_id[2],
        DRM_MODE_CONNECTOR_VGA,
        DRM_MODE_CONNECTED,
        0,
        modes,
        second_encoder,
        mir::geometry::Size{300, 200});

    drm.prepare(drm_device);

    auto const drm_fd = open(drm_device, 0, 0);

    auto crtc = mgk::find_crtc_for_connector(
        drm_fd,
        mgk::get_connector(drm_fd, connector_id[0]));
    EXPECT_THAT(crtc->crtc_id, AnyOf(crtc_ids[0], crtc_ids[1], crtc_ids[2]));

    crtc = mgk::find_crtc_for_connector(
        drm_fd,
        mgk::get_connector(drm_fd, connector_id[1]));
    EXPECT_THAT(crtc->crtc_id, AnyOf(crtc_ids[1], crtc_ids[2]));

    crtc = mgk::find_crtc_for_connector(
        drm_fd,
        mgk::get_connector(drm_fd, connector_id[2]));
    EXPECT_THAT(crtc->crtc_id, Eq(crtc_ids[2]));
}

TEST(KMSConnectorHelper, ignores_currently_used_encoders)
{
    using namespace testing;
    NiceMock<mtd::MockDRM> drm;
    char const* const drm_device = "/dev/dri/card0";
    int const drm_fd = open(drm_device, 0, 0);

    std::array<uint32_t, 2> const crtc_ids = {{21, 25}};
    std::array<uint32_t, 2> const encoder_ids = {{3, 5}};
    std::array<uint32_t, 2> const connector_id = {{9, 10}};

    drm.reset(drm_device);
    auto boring_mode =
        mtd::FakeDRMResources::create_mode(
            1200,
            1600,
            138500,
            1400,
            1800,
            mtd::FakeDRMResources::ModePreference::PreferredMode);
    // Add an active CRTC...
    drm.add_crtc(drm_device, crtc_ids[0], boring_mode);
    // ...and an inactive one.
    drm.add_crtc(drm_device, crtc_ids[1], drmModeModeInfo());

    // Add an encoder hooked up to the active CRTC
    drm.add_encoder(drm_device, encoder_ids[0], crtc_ids[0], 1 << 0 | 1 << 1);
    // ...and one not connected to anything
    drm.add_encoder(drm_device, encoder_ids[1], 0, 1 << 0 | 1 << 1);

    std::vector<drmModeModeInfo> modes{boring_mode};
    std::vector<uint32_t> any_encoder{encoder_ids.begin(), encoder_ids.end()};

    // Finally, a connector hooked up to a CRTC-encoder
    drm.add_connector(
        drm_device,
        connector_id[0],
        DRM_MODE_CONNECTOR_VGA,
        DRM_MODE_CONNECTED,
        encoder_ids[0],
        modes,
        any_encoder,
        mir::geometry::Size{300, 200});

    // ... and one not hooked up to anything
    drm.add_connector(
        drm_device,
        connector_id[1],
        DRM_MODE_CONNECTOR_VGA,
        DRM_MODE_CONNECTED,
        0,
        modes,
        any_encoder,
        mir::geometry::Size{300, 200});

    drm.prepare(drm_device);

    auto crtc = mgk::find_crtc_for_connector(
        drm_fd,
        mgk::get_connector(drm_fd, connector_id[1]));
    EXPECT_THAT(crtc->crtc_id, Eq(crtc_ids[1]));
}

TEST(KMSConnectorHelper, returns_current_crtc_if_it_exists)
{
    using namespace testing;
    NiceMock<mtd::MockDRM> drm;
    char const* const drm_device = "/dev/dri/card0";
    int const drm_fd = open(drm_device, 0, 0);


    std::array<uint32_t, 2> const crtc_ids = {{21, 25}};
    std::array<uint32_t, 2> const encoder_ids = {{3, 5}};
    std::array<uint32_t, 2> const connector_id = {{9, 10}};

    drm.reset(drm_device);
    auto boring_mode =
        mtd::FakeDRMResources::create_mode(
            1200,
            1600,
            138500,
            1400,
            1800,
            mtd::FakeDRMResources::ModePreference::PreferredMode);
    // Add an active CRTC...
    drm.add_crtc(drm_device, crtc_ids[0], boring_mode);
    // ...and an inactive one.
    drm.add_crtc(drm_device, crtc_ids[1], drmModeModeInfo());

    // Add an encoder hooked up to the active CRTC
    drm.add_encoder(drm_device, encoder_ids[0], crtc_ids[0], 1 << 0 | 1 << 1);
    // ...and one not connected to anything
    drm.add_encoder(drm_device, encoder_ids[1], 0, 1 << 0 | 1 << 1);

    std::vector<drmModeModeInfo> modes{boring_mode};
    std::vector<uint32_t> any_encoder{encoder_ids.begin(), encoder_ids.end()};

    // Finally, a connector hooked up to a CRTC-encoder
    drm.add_connector(
        drm_device,
        connector_id[0],
        DRM_MODE_CONNECTOR_VGA,
        DRM_MODE_CONNECTED,
        encoder_ids[0],
        modes,
        any_encoder,
        mir::geometry::Size{300, 200});

    // ... and one not hooked up to anything
    drm.add_connector(
        drm_device,
        connector_id[1],
        DRM_MODE_CONNECTOR_VGA,
        DRM_MODE_CONNECTED,
        0,
        modes,
        any_encoder,
        mir::geometry::Size{300, 200});

    drm.prepare(drm_device);

    auto crtc = mgk::find_crtc_for_connector(
        drm_fd,
        mgk::get_connector(drm_fd, connector_id[0]));
    EXPECT_THAT(crtc->crtc_id, Eq(crtc_ids[0]));
}
