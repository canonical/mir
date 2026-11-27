/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3,
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

#include "crtc_assignment.h"
#include "real_kms_display_configuration.h"
#include "real_kms_output_container.h"

#include <mir/test/doubles/mock_drm.h>

#include <mir/fd.h>

#include <drm_mode.h>
#include <fcntl.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <stdexcept>
#include <vector>

namespace mga = mir::graphics::atomic;
namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;

using namespace testing;

namespace
{
constexpr char const* drm_device{"/dev/dri/card0"};
constexpr uint16_t mode_hdisplay{1920};
constexpr uint16_t mode_vdisplay{1080};
constexpr uint16_t mode_htotal{2000};
constexpr uint16_t mode_vtotal{1100};
constexpr uint32_t mode_clock_khz{148500};
constexpr uint32_t crtc0_id{10};
constexpr uint32_t crtc1_id{11};
constexpr uint32_t unassigned_crtc_id{0};
constexpr uint32_t crtc0_mask{1u << 0};
constexpr uint32_t crtc1_mask{1u << 1};
constexpr uint32_t encoder0_id{20};
constexpr uint32_t encoder1_id{21};
constexpr uint32_t connector0_id{30};
constexpr uint32_t connector1_id{31};
constexpr uint32_t plane0_id{40};
constexpr uint32_t plane1_id{41};
constexpr uint32_t connector_crtc_property_id{101};
constexpr uint32_t plane_type_property_id{301};
constexpr uint32_t insufficient_crtcs_connector_crtc_property_id{501};
constexpr uint32_t insufficient_crtcs_plane_type_property_id{502};
constexpr uint32_t insufficient_primary_planes_connector_crtc_property_id{601};
constexpr uint32_t insufficient_primary_planes_plane_type_property_id{602};
constexpr std::size_t active_mode_index{0};
mg::DisplayConfigurationOutputId const output0_id{1};
mg::DisplayConfigurationOutputId const output1_id{2};
geom::Size const unknown_physical_size{0, 0};

class CrtcAssignmentTest : public Test
{
public:
    CrtcAssignmentTest() : drm_fd{mock_drm.open(drm_device, O_RDWR)} {}

    void SetUp() override
    {
        EXPECT_GE(static_cast<int>(drm_fd), 0);

        reset_resources();

        // Two CRTCs: crtc0 at index 0, crtc1 at index 1.
        mock_drm.add_crtc(drm_device, crtc0_id, modes.back());
        mock_drm.add_crtc(drm_device, crtc1_id, modes.back());

        // Encoder 0 can only drive crtc0; encoder 1 can drive either.
        mock_drm.add_encoder(drm_device, encoder0_id, unassigned_crtc_id, crtc0_mask);
        mock_drm.add_encoder(drm_device, encoder1_id, unassigned_crtc_id, crtc0_mask | crtc1_mask);

        connector0_encoders = {encoder0_id};
        connector1_encoders = {encoder1_id};
        add_connector(connector0_id, DRM_MODE_CONNECTOR_VGA, encoder0_id, connector0_encoders);
        add_connector(connector1_id, DRM_MODE_CONNECTOR_DisplayPort, encoder1_id, connector1_encoders);

        // Distinct primary planes, each tied to one CRTC.
        mock_drm.add_plane(drm_device, plane0_id, crtc0_mask);
        mock_drm.add_plane(drm_device, plane1_id, crtc1_mask);

        mock_drm.prepare(drm_device);

        add_connector_crtc_property(connector0_id, connector_crtc_property_id);
        add_connector_crtc_property(connector1_id, connector_crtc_property_id);
        add_plane_type_property(plane0_id, plane_type_property_id, DRM_PLANE_TYPE_PRIMARY);
        add_plane_type_property(plane1_id, plane_type_property_id, DRM_PLANE_TYPE_PRIMARY);
    }

protected:
    static drmModeModeInfo make_mode()
    {
        drmModeModeInfo mode{};
        mode.hdisplay = mode_hdisplay;
        mode.vdisplay = mode_vdisplay;
        mode.htotal = mode_htotal;
        mode.vtotal = mode_vtotal;
        mode.clock = mode_clock_khz;
        return mode;
    }

    void reset_resources()
    {
        mock_drm.reset(drm_device);
        modes.clear();
        modes.push_back(make_mode());
    }

    void add_connector(uint32_t connector_id, uint32_t type, uint32_t encoder_id, std::vector<uint32_t>& encoder_ids)
    {
        mock_drm.add_connector(
            drm_device, connector_id, type, DRM_MODE_CONNECTED, encoder_id, modes, encoder_ids, unknown_physical_size);
    }

    void add_connector_crtc_property(uint32_t connector_id, uint32_t property_id)
    {
        mock_drm.add_property(drm_device, connector_id, DRM_MODE_OBJECT_CONNECTOR, property_id, "CRTC_ID");
    }

    void add_plane_type_property(uint32_t plane_id, uint32_t property_id, uint32_t type)
    {
        mock_drm.add_property(drm_device, plane_id, DRM_MODE_OBJECT_PLANE, property_id, "type", type);
    }

    auto active_configuration() -> std::unique_ptr<mga::RealKMSDisplayConfiguration>
    {
        auto output_container = std::make_shared<mga::RealKMSOutputContainer>(drm_fd);
        output_container->update_from_hardware_state();
        auto conf = std::make_unique<mga::RealKMSDisplayConfiguration>(output_container);
        conf->for_each_output(
            [](mg::UserDisplayConfigurationOutput& output)
            {
                output.power_mode = mir_power_mode_on;
                output.used = true;
                output.current_mode_index = active_mode_index;
            });
        return conf;
    }

    NiceMock<mtd::MockDRM> mock_drm;
    mir::Fd const drm_fd;
    std::vector<drmModeModeInfo> modes;
    std::vector<uint32_t> connector0_encoders;
    std::vector<uint32_t> connector1_encoders;
};

class CrtcAssignmentInsufficientCrtcsTest : public CrtcAssignmentTest
{
public:
    void SetUp() override
    {
        EXPECT_GE(static_cast<int>(drm_fd), 0);

        reset_resources();

        // Only one CRTC, but two active connectors competing for it.
        mock_drm.add_crtc(drm_device, crtc0_id, modes.back());

        mock_drm.add_encoder(drm_device, encoder0_id, unassigned_crtc_id, crtc0_mask);
        mock_drm.add_encoder(drm_device, encoder1_id, unassigned_crtc_id, crtc0_mask);

        connector0_encoders = {encoder0_id};
        connector1_encoders = {encoder1_id};
        add_connector(connector0_id, DRM_MODE_CONNECTOR_VGA, encoder0_id, connector0_encoders);
        add_connector(connector1_id, DRM_MODE_CONNECTOR_DisplayPort, encoder1_id, connector1_encoders);

        mock_drm.add_plane(drm_device, plane0_id, crtc0_mask);

        mock_drm.prepare(drm_device);

        add_connector_crtc_property(connector0_id, insufficient_crtcs_connector_crtc_property_id);
        add_connector_crtc_property(connector1_id, insufficient_crtcs_connector_crtc_property_id);
        add_plane_type_property(plane0_id, insufficient_crtcs_plane_type_property_id, DRM_PLANE_TYPE_PRIMARY);
    }
};

class CrtcAssignmentInsufficientPrimaryPlanesTest : public CrtcAssignmentTest
{
public:
    void SetUp() override
    {
        EXPECT_GE(static_cast<int>(drm_fd), 0);

        reset_resources();

        // Two CRTCs, but only crtc0 has a primary plane.
        mock_drm.add_crtc(drm_device, crtc0_id, modes.back());
        mock_drm.add_crtc(drm_device, crtc1_id, modes.back());

        // Each connector can drive either CRTC.
        mock_drm.add_encoder(drm_device, encoder0_id, unassigned_crtc_id, crtc0_mask | crtc1_mask);
        mock_drm.add_encoder(drm_device, encoder1_id, unassigned_crtc_id, crtc0_mask | crtc1_mask);

        connector0_encoders = {encoder0_id};
        connector1_encoders = {encoder1_id};
        add_connector(connector0_id, DRM_MODE_CONNECTOR_VGA, encoder0_id, connector0_encoders);
        add_connector(connector1_id, DRM_MODE_CONNECTOR_DisplayPort, encoder1_id, connector1_encoders);

        // Only one primary plane available for two outputs.
        mock_drm.add_plane(drm_device, plane0_id, crtc0_mask);

        mock_drm.prepare(drm_device);

        add_connector_crtc_property(connector0_id, insufficient_primary_planes_connector_crtc_property_id);
        add_connector_crtc_property(connector1_id, insufficient_primary_planes_connector_crtc_property_id);
        add_plane_type_property(
            plane0_id, insufficient_primary_planes_plane_type_property_id, DRM_PLANE_TYPE_PRIMARY);
    }
};

}

TEST_F(CrtcAssignmentTest, assigns_last_viable_crtc_to_more_flexible_output)
{
    auto conf = active_configuration();
    auto assignments = mga::solve_crtc_assignments(drm_fd, *conf);

    ASSERT_THAT(assignments, SizeIs(2));
    auto const& assignment0 = assignments.at(output0_id);
    EXPECT_THAT(assignment0.crtc->crtc_id, Eq(crtc0_id));
    EXPECT_THAT(assignment0.plane->plane_id, Eq(plane0_id));

    auto const& assignment1 = assignments.at(output1_id);
    EXPECT_THAT(assignment1.crtc->crtc_id, Eq(crtc1_id));
    EXPECT_THAT(assignment1.plane->plane_id, Eq(plane1_id));
}

TEST_F(CrtcAssignmentTest, skips_inactive_outputs)
{
    auto conf = active_configuration();
    conf->for_each_output(
        [](mg::UserDisplayConfigurationOutput& output)
        {
            if (output.id != output0_id)
            {
                output.used = false;
            }
        });

    auto assignments = mga::solve_crtc_assignments(drm_fd, *conf);

    ASSERT_THAT(assignments, SizeIs(1));
    EXPECT_THAT(assignments.at(output0_id).crtc->crtc_id, Eq(crtc0_id));
}

TEST_F(CrtcAssignmentInsufficientCrtcsTest, throws_when_insufficient_crtcs)
{
    auto conf = active_configuration();
    EXPECT_THROW(mga::solve_crtc_assignments(drm_fd, *conf), std::runtime_error);
}

TEST_F(
    CrtcAssignmentInsufficientPrimaryPlanesTest,
    throws_when_not_all_active_outputs_can_be_assigned_a_primary_plane)
{
    auto conf = active_configuration();
    EXPECT_THROW(mga::solve_crtc_assignments(drm_fd, *conf), std::runtime_error);
}
