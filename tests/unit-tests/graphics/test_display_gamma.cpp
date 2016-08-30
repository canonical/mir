/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Brandon Schaefer <brandon.schaefer@gmail.com>
 */

#include "mir/graphics/display_gamma.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mg = mir::graphics;

namespace
{
uint16_t const r[]{1};
uint16_t const g[]{2};
uint16_t const b[]{3};
uint32_t const size{1};
}

class MockDisplayGammaUint16 : public testing::Test
{
public:
    MockDisplayGammaUint16() :
        gamma(r, g, b, size)
    {
    }

    mg::DisplayGamma gamma;
};

TEST_F(MockDisplayGammaUint16, test_uint16_display_gamma_size)
{
    EXPECT_THAT(gamma.size(), size);
}

TEST_F(MockDisplayGammaUint16, test_uint16_display_gamma_rgb_correct)
{
    ASSERT_THAT(gamma.size(), gamma.size());
    EXPECT_THAT(gamma.red()[0],   r[0]);
    EXPECT_THAT(gamma.green()[0], g[0]);
    EXPECT_THAT(gamma.blue()[0],  b[0]);
}

TEST_F(MockDisplayGammaUint16, test_display_gamma_copy_ctor)
{
    mg::DisplayGamma gamma_second(gamma);

    ASSERT_THAT(gamma_second.size(),     gamma.size());
    EXPECT_THAT(gamma_second.red()[0],   gamma.red()[0]);
    EXPECT_THAT(gamma_second.green()[0], gamma.green()[0]);
    EXPECT_THAT(gamma_second.blue()[0],  gamma.blue()[0]);
}

TEST_F(MockDisplayGammaUint16, test_display_gamma_assign_copy_ctor)
{
    mg::DisplayGamma gamma_second = gamma;

    ASSERT_THAT(gamma_second.size(),     gamma.size());
    EXPECT_THAT(gamma_second.red()[0],   gamma.red()[0]);
    EXPECT_THAT(gamma_second.green()[0], gamma.green()[0]);
    EXPECT_THAT(gamma_second.blue()[0],  gamma.blue()[0]);
}

TEST_F(MockDisplayGammaUint16, test_display_gamma_move_ctor)
{
    mg::DisplayGamma gamma_second(mg::DisplayGamma(r, g, b, size));

    ASSERT_THAT(gamma_second.size(),     size);
    EXPECT_THAT(gamma_second.red()[0],   r[0]);
    EXPECT_THAT(gamma_second.green()[0], g[0]);
    EXPECT_THAT(gamma_second.blue()[0],  b[0]);
}

TEST_F(MockDisplayGammaUint16, test_display_gamma_assign_move_ctor)
{
    mg::DisplayGamma gamma_second = mg::DisplayGamma(r, g, b, size);

    ASSERT_THAT(gamma_second.size(),     size);
    EXPECT_THAT(gamma_second.red()[0],   r[0]);
    EXPECT_THAT(gamma_second.green()[0], g[0]);
    EXPECT_THAT(gamma_second.blue()[0],  b[0]);
}

TEST(DisplayGammaString, test_display_gamma_even_string_converts_correct_uint16)
{
    std::string r_str{1, 2};
    std::string g_str{2, 3};
    std::string b_str{static_cast<char>(254),
                      static_cast<char>(255)};

    mg::DisplayGamma gamma(r_str, g_str, b_str);

    ASSERT_THAT(gamma.size(), 1);

    EXPECT_THAT(gamma.red()[0],   r_str[1] << CHAR_BIT | r_str[0]);
    EXPECT_THAT(gamma.green()[0], g_str[1] << CHAR_BIT | g_str[0]);
    EXPECT_THAT(gamma.blue()[0],  b_str[1] << CHAR_BIT | b_str[0]);
}

TEST(DisplayGammaEmpty, test_display_gamma_empty)
{
    mg::DisplayGamma gamma;

    EXPECT_THAT(gamma.size(), 0);
}
