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
namespace mg = mir::graphics;

namespace
{
Rectangle const display_area_a{{20, 30}, {600, 400}};
Rectangle const display_area_b{{620, 0}, {800, 500}};
Rectangle const display_area_c{{0, 600}, {430, 200}};
Rectangle const display_area_d{{430, 600}, {800, 200}};

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

TEST_F(ApplicationZone, multiple_outputs_in_the_same_logical_group_lead_to_one_application_zone)
{
    miral::Zone zone_a{{{}, {}}};
    auto display_config = create_fake_display_configuration({
        std::make_pair(mg::DisplayConfigurationLogicalGroupId{1}, display_area_a),
        std::make_pair(mg::DisplayConfigurationLogicalGroupId{1}, display_area_b)});

    EXPECT_CALL(*window_manager_policy, advise_application_zone_create(_))
        .WillOnce(Invoke([&](Zone const& zone){ zone_a = zone; }));

    notify_configuration_applied(display_config);

    Mock::VerifyAndClearExpectations(window_manager_policy);

    auto const extents{Rectangles{{display_area_a, display_area_b}}.bounding_rectangle()};
    EXPECT_THAT(zone_a.extents(), Eq(extents));
}

TEST_F(ApplicationZone, multiple_logical_output_groups_lead_to_multiple_application_zones)
{
    miral::Zone zone_a_b{{{}, {}}};
    miral::Zone zone_c_d{{{}, {}}};
    auto display_config = create_fake_display_configuration({
        std::make_pair(mg::DisplayConfigurationLogicalGroupId{1}, display_area_a),
        std::make_pair(mg::DisplayConfigurationLogicalGroupId{1}, display_area_b),
        std::make_pair(mg::DisplayConfigurationLogicalGroupId{2}, display_area_c),
        std::make_pair(mg::DisplayConfigurationLogicalGroupId{2}, display_area_d)});

    EXPECT_CALL(*window_manager_policy, advise_application_zone_create(_))
        .WillOnce(Invoke([&](Zone const& zone){ zone_a_b = zone; }))
        .WillOnce(Invoke([&](Zone const& zone){ zone_c_d = zone; }));

    notify_configuration_applied(display_config);

    Mock::VerifyAndClearExpectations(window_manager_policy);

    EXPECT_FALSE(zone_a_b.is_same_zone(zone_c_d));

    auto const extents_a_b{Rectangles{{display_area_a, display_area_b}}.bounding_rectangle()};
    EXPECT_THAT(zone_a_b.extents(), Eq(extents_a_b));

    auto const extents_c_d{Rectangles{{display_area_c, display_area_d}}.bounding_rectangle()};
    EXPECT_THAT(zone_c_d.extents(), Eq(extents_c_d));
}

TEST_F(ApplicationZone, adding_output_to_logical_group_resizes_application_zone)
{
    miral::Zone initial_zone{{{}, {}}};
    miral::Zone updated_zone{{{}, {}}};
    auto display_config_a_b = create_fake_display_configuration({
        std::make_pair(mg::DisplayConfigurationLogicalGroupId{1}, display_area_a),
        std::make_pair(mg::DisplayConfigurationLogicalGroupId{1}, display_area_b)});
    auto display_config_a = create_fake_display_configuration({
        std::make_pair(mg::DisplayConfigurationLogicalGroupId{1}, display_area_a)});

    EXPECT_CALL(*window_manager_policy, advise_application_zone_create(_))
        .WillOnce(Invoke([&](Zone const& zone){ initial_zone = zone; }));

    notify_configuration_applied(display_config_a);

    Mock::VerifyAndClearExpectations(window_manager_policy);

    EXPECT_CALL(*window_manager_policy, advise_application_zone_update(_, _))
        .WillOnce(Invoke([&](Zone const& updated, Zone const& original){
            EXPECT_THAT(original, Eq(initial_zone));
            updated_zone = updated;
        }));

    notify_configuration_applied(display_config_a_b);

    Mock::VerifyAndClearExpectations(window_manager_policy);

    auto const extents{Rectangles{{display_area_a, display_area_b}}.bounding_rectangle()};
    EXPECT_TRUE(updated_zone.is_same_zone(initial_zone));
    EXPECT_THAT(updated_zone.extents(), Eq(extents));
}

TEST_F(ApplicationZone, removing_output_in_logical_group_resizes_application_zone)
{
    miral::Zone initial_zone{{{}, {}}};
    miral::Zone updated_zone{{{}, {}}};
    auto display_config_a_b = create_fake_display_configuration({
        std::make_pair(mg::DisplayConfigurationLogicalGroupId{1}, display_area_a),
        std::make_pair(mg::DisplayConfigurationLogicalGroupId{1}, display_area_b)});
    auto display_config_a = create_fake_display_configuration({
        std::make_pair(mg::DisplayConfigurationLogicalGroupId{1}, display_area_a)});

    EXPECT_CALL(*window_manager_policy, advise_application_zone_create(_))
        .WillOnce(Invoke([&](Zone const& zone){ initial_zone = zone; }));

    notify_configuration_applied(display_config_a_b);

    Mock::VerifyAndClearExpectations(window_manager_policy);

    EXPECT_CALL(*window_manager_policy, advise_application_zone_update(_, _))
        .WillOnce(Invoke([&](Zone const& updated, Zone const& original){
            EXPECT_THAT(original, Eq(initial_zone));
            updated_zone = updated;
        }));

    notify_configuration_applied(display_config_a);

    Mock::VerifyAndClearExpectations(window_manager_policy);

    EXPECT_TRUE(updated_zone.is_same_zone(initial_zone));
    EXPECT_THAT(updated_zone.extents(), Eq(display_area_a));
}

TEST_F(ApplicationZone, updating_output_in_logical_group_updates_application_zone)
{
    miral::Zone zone_a_b{{{}, {}}};
    miral::Zone zone_a_c{{{}, {}}};
    auto display_config_a_b = create_fake_display_configuration({
        std::make_pair(mg::DisplayConfigurationLogicalGroupId{1}, display_area_a),
        std::make_pair(mg::DisplayConfigurationLogicalGroupId{1}, display_area_b)});
    auto display_config_a_c = create_fake_display_configuration({
        std::make_pair(mg::DisplayConfigurationLogicalGroupId{1}, display_area_a),
        std::make_pair(mg::DisplayConfigurationLogicalGroupId{1}, display_area_c)});

    EXPECT_CALL(*window_manager_policy, advise_application_zone_create(_))
        .WillOnce(Invoke([&](Zone const& zone){ zone_a_b = zone; }));

    notify_configuration_applied(display_config_a_b);

    // Before continuing with the test, we must insure zone_a_initial has been set
    Mock::VerifyAndClearExpectations(window_manager_policy);

    EXPECT_CALL(*window_manager_policy, advise_application_zone_update(_, _))
        .WillOnce(Invoke([&](Zone const& updated, Zone const& original){
            EXPECT_THAT(original, Eq(zone_a_b));
            zone_a_c = updated;
        }));

    notify_configuration_applied(display_config_a_c);

    Mock::VerifyAndClearExpectations(window_manager_policy);

    auto const extents_a_c{Rectangles{{display_area_a, display_area_c}}.bounding_rectangle()};
    EXPECT_TRUE(zone_a_b.is_same_zone(zone_a_c));
    EXPECT_THAT(zone_a_c.extents(), Ne(zone_a_b.extents()));
    EXPECT_THAT(zone_a_c.extents(), Eq(extents_a_c));
}

TEST_F(ApplicationZone, removing_all_outputs_in_logical_group_deletes_application_zone)
{
    miral::Zone zone_a{{{}, {}}};
    miral::Zone zone_b_c{{{}, {}}};
    miral::Zone deleted_zone{{{}, {}}};
    auto display_config_a_b_c = create_fake_display_configuration({
        std::make_pair(mg::DisplayConfigurationLogicalGroupId{0}, display_area_a),
        std::make_pair(mg::DisplayConfigurationLogicalGroupId{1}, display_area_b),
        std::make_pair(mg::DisplayConfigurationLogicalGroupId{1}, display_area_c)});
    auto display_config_a = create_fake_display_configuration({
        std::make_pair(mg::DisplayConfigurationLogicalGroupId{0}, display_area_a)});

    EXPECT_CALL(*window_manager_policy, advise_application_zone_create(_))
        .WillOnce(Invoke([&](Zone const& zone){ zone_a = zone; }))
        .WillOnce(Invoke([&](Zone const& zone){ zone_b_c = zone; }));

    notify_configuration_applied(display_config_a_b_c);

    Mock::VerifyAndClearExpectations(window_manager_policy);

    EXPECT_CALL(*window_manager_policy, advise_application_zone_delete(_))
        .WillOnce(Invoke([&](Zone const& zone){ deleted_zone = zone; }));

    notify_configuration_applied(display_config_a);

    Mock::VerifyAndClearExpectations(window_manager_policy);

    EXPECT_TRUE(deleted_zone.is_same_zone(zone_b_c));
}
