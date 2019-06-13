/*
 * Copyright © 2019 Canonical Ltd.
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
 * Authored by: William Wold <william.wold@canonical.com>
 */

#include <miral/zone.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace testing;
using namespace mir::geometry;

struct Zone : public testing::Test
{
};

TEST_F(Zone, zone_returns_correct_extents)
{
    mir::geometry::Rectangle extents{{50, 60}, {70, 80}};
    miral::Zone zone{extents};

    EXPECT_THAT(zone.extents(), Eq(extents));
}

TEST_F(Zone, can_modify_zone_extents)
{
    mir::geometry::Rectangle a{{50, 60}, {70, 80}};
    mir::geometry::Rectangle b{{10, 20}, {30, 40}};

    miral::Zone zone{a};
    zone.extents(b);

    EXPECT_THAT(zone.extents(), Not(Eq(a))) << "Extents not modified";
    EXPECT_THAT(zone.extents(), Eq(b));
}

TEST_F(Zone, zone_is_same_as_self)
{
    miral::Zone zone{{{50, 60}, {70, 80}}};

    EXPECT_TRUE(zone.is_same_zone(zone));
}

TEST_F(Zone, zone_extents_copied)
{
    mir::geometry::Rectangle extents{{50, 60}, {70, 80}};

    miral::Zone zone_a{extents};
    auto zone_b = zone_a;
    miral::Zone zone_c{zone_a};

    EXPECT_THAT(zone_b.extents(), Eq(extents));
    EXPECT_THAT(zone_c.extents(), Eq(extents));
}

TEST_F(Zone, zone_same_after_copy)
{
    miral::Zone zone_a{{{50, 60}, {70, 80}}};
    auto zone_b = zone_a;
    miral::Zone zone_c{zone_a};

    EXPECT_TRUE(zone_a.is_same_zone(zone_b));
    EXPECT_TRUE(zone_c.is_same_zone(zone_a));
    EXPECT_TRUE(zone_b.is_same_zone(zone_c));
}

TEST_F(Zone, zone_same_after_modify)
{
    miral::Zone zone_a{{{50, 60}, {70, 80}}};
    miral::Zone zone_b{zone_a};
    zone_b.extents({{10, 20}, {30, 40}});

    EXPECT_TRUE(zone_a.is_same_zone(zone_b));
}

TEST_F(Zone, zone_equal_to_copy)
{
    miral::Zone zone_a{{{50, 60}, {70, 80}}};
    miral::Zone zone_b{zone_a};

    EXPECT_THAT(zone_a, Eq(zone_b));
}

TEST_F(Zone, zone_not_equal_to_modified_same_zone)
{
    miral::Zone zone_a{{{50, 60}, {70, 80}}};
    miral::Zone zone_b{zone_a};
    zone_b.extents({{10, 20}, {30, 40}});

    EXPECT_THAT(zone_a, Not(Eq(zone_b)));
}

TEST_F(Zone, zone_not_equal_to_different_zone_with_same_extents)
{
    mir::geometry::Rectangle extents{{50, 60}, {70, 80}};
    miral::Zone zone_a{extents};
    miral::Zone zone_b{extents};

    EXPECT_THAT(zone_a, Not(Eq(zone_b)));
}
