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

#include "mir/graphics/gamma_curves.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mg = mir::graphics;

namespace
{
std::vector<uint16_t> const r{1};
std::vector<uint16_t> const g{2};
std::vector<uint16_t> const b{3};
}

class MockGammaCurves : public testing::Test
{
public:
    MockGammaCurves() :
        gamma(r, g, b)
    {
    }

    mg::GammaCurves gamma;
};

TEST_F(MockGammaCurves, test_uint16_gamma_curves_size)
{
    EXPECT_THAT(gamma.red.size(), r.size());
}

TEST_F(MockGammaCurves, test_uint16_gamma_curves_rgb_correct)
{
    ASSERT_THAT(gamma.red.size(), r.size());
    EXPECT_THAT(gamma.red[0], r[0]);
    EXPECT_THAT(gamma.green[0], g[0]);
    EXPECT_THAT(gamma.blue[0], b[0]);
}

TEST(GammaCurvesEmpty, test_gamma_curves_empty)
{
    mg::GammaCurves gamma;

    EXPECT_THAT(gamma.red.size(), 0);
}

TEST(GammaCurvesEmpty, test_invalid_lut_size_gamma_curves_throw)
{
    EXPECT_THROW({ mg::GammaCurves({1}, {2, 3}, {4, 5, 6}); }, std::logic_error);
}
