/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/geometry/displacement.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace geom = mir::geometry;

TEST(geometry, displacement)
{
    using namespace geom;
    Displacement disp;
    Displacement const dx2dy4{DeltaX{2}, DeltaY{4}};

    EXPECT_EQ(DeltaX{0}, disp.dx);
    EXPECT_EQ(DeltaY{0}, disp.dy);

    EXPECT_EQ(DeltaX{2}, dx2dy4.dx);
    EXPECT_EQ(DeltaY{4}, dx2dy4.dy);

    EXPECT_EQ(disp, disp);
    EXPECT_NE(disp, dx2dy4);
    EXPECT_EQ(dx2dy4, dx2dy4);
    EXPECT_NE(dx2dy4, disp);
}

TEST(geometry, displacement_arithmetic)
{
    using namespace geom;
    Displacement const dx2dy4{DeltaX{2}, DeltaY{4}};
    Displacement const dx3dy9{DeltaX{3}, DeltaY{9}};
    Displacement const expected_add{DeltaX{5}, DeltaY{13}};
    Displacement const expected_sub{DeltaX{1}, DeltaY{5}};

    Displacement const add = dx3dy9 + dx2dy4;
    Displacement const sub = dx3dy9 - dx2dy4;

    EXPECT_EQ(expected_add, add);
    EXPECT_EQ(expected_sub, sub);
}

TEST(geometry, displacement_point_arithmetic)
{
    using namespace geom;
    Point const x2y4{X{2}, Y{4}};
    Point const x3y9{X{3}, Y{9}};
    Displacement const dx2dy4{DeltaX{2}, DeltaY{4}};
    Displacement const dx7dy11{DeltaX{7}, DeltaY{11}};

    Point const expected_pd_add{X{5}, Y{13}};
    Point const expected_pd_sub{X{1}, Y{5}};
    Point const expected_pd_sub2{X{-4}, Y{-2}};
    Displacement const expected_pp_sub{DeltaX{1}, DeltaY{5}};
    Displacement const expected_pp_sub2{DeltaX{-1}, DeltaY{-5}};

    Point const pd_add = x3y9 + dx2dy4;
    Point const pd_sub = x3y9 - dx2dy4;
    Point const pd_sub2 = x3y9 - dx7dy11;
    Displacement const pp_sub = x3y9 - x2y4;
    Displacement const pp_sub2 = x2y4 - x3y9;

    EXPECT_EQ(expected_pd_add, pd_add);
    EXPECT_EQ(expected_pd_sub, pd_sub);
    EXPECT_EQ(expected_pd_sub2, pd_sub2);
    EXPECT_EQ(expected_pp_sub, pp_sub);
    EXPECT_EQ(expected_pp_sub2, pp_sub2);
}
