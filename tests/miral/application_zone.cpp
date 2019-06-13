/*
 * Copyright © 2017 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "test_window_manager_tools.h"
#include "mir/test/doubles/mock_display_configuration.h"
#include "mir/graphics/display_configuration_observer.h"

using namespace miral;
using namespace testing;
namespace mt = mir::test;

namespace
{
Rectangle const display_area_a{{20, 30}, {600, 400}};
Rectangle const display_area_b{{620, 0}, {800, 500}};

struct ApplicationZone : mt::TestWindowManagerTools
{
    void SetUp() override
    {
        basic_window_manager.add_session(session);
    }
};
}

TEST_F(ApplicationZone, application_zone_created_for_output)
{
    miral::Zone zone_a{{{}, {}}};
    auto display_config_a = create_fake_display_configuration({display_area_a});

    EXPECT_CALL(*window_manager_policy, advise_application_zone_create(_))
        .WillOnce(Invoke([&](Zone const& zone){ zone_a = zone; }));

    notify_configuration_applied(display_config_a);

    Mock::VerifyAndClearExpectations(window_manager_policy);
    EXPECT_THAT(zone_a.extents(), Eq(display_area_a));
}

TEST_F(ApplicationZone, multiple_outputs_lead_to_multiple_application_zones)
{
    miral::Zone zone_a{{{}, {}}};
    miral::Zone zone_b{{{}, {}}};
    auto display_config_a_b = create_fake_display_configuration({display_area_a, display_area_b});

    EXPECT_CALL(*window_manager_policy, advise_application_zone_create(_))
        .WillOnce(Invoke([&](Zone const& zone){ zone_a = zone; }))
        .WillOnce(Invoke([&](Zone const& zone){ zone_b = zone; }));

    notify_configuration_applied(display_config_a_b);

    Mock::VerifyAndClearExpectations(window_manager_policy);
    EXPECT_THAT(zone_a.extents(), Eq(display_area_a));
    EXPECT_THAT(zone_b.extents(), Eq(display_area_b));
    EXPECT_FALSE(zone_a.is_same_zone(zone_b));
}

TEST_F(ApplicationZone, updating_output_updates_application_zone)
{
    miral::Zone zone_a_initial{{{}, {}}};
    miral::Zone zone_a_original{{{}, {}}};
    miral::Zone zone_b{{{}, {}}};
    auto display_config_a = create_fake_display_configuration({display_area_a});
    auto display_config_b = create_fake_display_configuration({display_area_b});

    EXPECT_CALL(*window_manager_policy, advise_application_zone_create(_))
        .WillOnce(Invoke([&](Zone const& zone){ zone_a_initial = zone; }));

    notify_configuration_applied(display_config_a);

    // Before continuing with the test, we must insure zone_a_initial has been set
    Mock::VerifyAndClearExpectations(window_manager_policy);

    EXPECT_CALL(*window_manager_policy, advise_application_zone_update(_, _))
        .WillOnce(Invoke([&](Zone const& updated, Zone const& original){
            zone_a_original = original;
            zone_b = updated;
        }));

    notify_configuration_applied(display_config_b);

    Mock::VerifyAndClearExpectations(window_manager_policy);
    EXPECT_TRUE(zone_a_initial.is_same_zone(zone_a_original));
    EXPECT_TRUE(zone_a_original.is_same_zone(zone_b));

    EXPECT_THAT(zone_a_initial, Eq(zone_a_original));

    EXPECT_THAT(zone_a_initial.extents(), Eq(display_area_a));
    EXPECT_THAT(zone_a_original.extents(), Eq(display_area_a));
    EXPECT_THAT(zone_b.extents(), Eq(display_area_b));
}

TEST_F(ApplicationZone, removing_output_deletes_application_zone)
{
    miral::Zone zone_a{{{}, {}}};
    miral::Zone zone_b{{{}, {}}};
    miral::Zone deleted_zone{{{}, {}}};
    auto display_config_a_b = create_fake_display_configuration({display_area_a, display_area_b});
    auto display_config_a = create_fake_display_configuration({display_area_a});

    EXPECT_CALL(*window_manager_policy, advise_application_zone_create(_))
        .WillOnce(Invoke([&](Zone const& zone){ zone_a = zone; }))
        .WillOnce(Invoke([&](Zone const& zone){ zone_b = zone; }));

    notify_configuration_applied(display_config_a_b);

    Mock::VerifyAndClearExpectations(window_manager_policy);

    EXPECT_CALL(*window_manager_policy, advise_application_zone_delete(_))
        .WillOnce(Invoke([&](Zone const& zone){ deleted_zone = zone; }));

    notify_configuration_applied(display_config_a);

    Mock::VerifyAndClearExpectations(window_manager_policy);

    EXPECT_TRUE(deleted_zone.is_same_zone(zone_b));
    EXPECT_THAT(deleted_zone, Eq(zone_b));
}
