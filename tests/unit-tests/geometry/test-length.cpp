/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "mir/geometry/length.h"

#include <gtest/gtest.h>

using namespace mir::geometry;

TEST(Length, zero)
{
    Length zero;

    EXPECT_EQ(0, zero.as(Length::micrometres));
    EXPECT_EQ(0, zero.as(Length::millimetres));
    EXPECT_EQ(0, zero.as(Length::centimetres));
    EXPECT_EQ(0, zero.as(Length::inches));

    EXPECT_EQ(zero, 0_mm);
    EXPECT_EQ(zero, 0.0_mm);
    EXPECT_EQ(zero, 0_cm);
    EXPECT_EQ(zero, 0.0_cm);
    EXPECT_EQ(zero, 0_in);
    EXPECT_EQ(zero, 0.0_in);
}

TEST(Length, millimetres)
{
    Length one_mm(1, Length::millimetres);
    EXPECT_EQ(Length(1000, Length::micrometres), one_mm);
    EXPECT_EQ(1.0f, one_mm.as(Length::millimetres));
    EXPECT_FLOAT_EQ(0.1f, one_mm.as(Length::centimetres));
    EXPECT_FLOAT_EQ(0.039370079f, one_mm.as(Length::inches));

    for (int n = 0; n < 10; ++n)
    {
        EXPECT_EQ(Length(1000*n, Length::micrometres),
                  Length(n, Length::millimetres));
    }
}

TEST(Length, centimetres)
{
    Length one_cm(1, Length::centimetres);
    EXPECT_EQ(Length(10000, Length::micrometres), one_cm);
    EXPECT_EQ(Length(10, Length::millimetres), one_cm);
    EXPECT_EQ(1.0f, one_cm.as(Length::centimetres));
    EXPECT_EQ(10.0f, one_cm.as(Length::millimetres));
    EXPECT_FLOAT_EQ(0.39370079f, one_cm.as(Length::inches));

    EXPECT_EQ(one_cm, 1_cm);
    EXPECT_EQ(one_cm, 1.0_cm);
    EXPECT_EQ(one_cm, 10_mm);
    EXPECT_EQ(one_cm, 10.0_mm);

    for (int n = 0; n < 10; ++n)
    {
        EXPECT_EQ(Length(10000*n, Length::micrometres),
                  Length(n, Length::centimetres));
    }
}

TEST(Length, inches)
{
    Length one_in(1, Length::inches);
    EXPECT_EQ(Length(25400, Length::micrometres), one_in);
    EXPECT_FLOAT_EQ(2.54f, one_in.as(Length::centimetres));

    EXPECT_EQ(one_in, 1.0_in);
    EXPECT_EQ(one_in, 2.54_cm);
    EXPECT_EQ(one_in, 25.4_mm);

    for (int n = 0; n < 10; ++n)
    {
        EXPECT_EQ(Length(25400*n, Length::micrometres),
                  Length(n, Length::inches));
    }
}

TEST(Length, DPI)
{
    EXPECT_EQ(24, (0.25_in).as_pixels(96.0f));

    EXPECT_FLOAT_EQ(3543.3070866f, (30_cm).as_pixels(300.0f));

    EXPECT_EQ(123456, Length(123456, Length::inches).as_pixels(1.0f));
}

TEST(Length, assign)
{
    auto const a = 123.45_in;
    Length b;

    EXPECT_NE(a, b);
    EXPECT_EQ(0, b.as(Length::inches));

    b = a;
    EXPECT_EQ(a, b);
    EXPECT_EQ(3135630, b.as(Length::micrometres));
}

TEST(Length, copy)
{
    auto const a = 123.45_in;
    Length b(a);

    EXPECT_EQ(a, b);
    EXPECT_EQ(3135630, b.as(Length::micrometres));
}
