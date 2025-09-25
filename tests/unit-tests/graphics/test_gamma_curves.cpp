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

#include "mir/graphics/gamma_curves.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <algorithm>

namespace mg = mir::graphics;

namespace
{
MATCHER(IsMonotonicallyIncreasing, "")
{
    return std::is_sorted(std::begin(arg), std::end(arg));
}
}
class GammaCurves : public testing::Test
{
public:
    mg::GammaCurve const r{1};
    mg::GammaCurve const g{2};
    mg::GammaCurve const b{3};
    mg::GammaCurves gamma{r, g, b};
};

TEST_F(GammaCurves, have_correct_size)
{
    EXPECT_THAT(gamma.red.size(), r.size());
    EXPECT_THAT(gamma.green.size(), g.size());
    EXPECT_THAT(gamma.blue.size(), b.size());
}

TEST_F(GammaCurves, have_expected_content)
{
    using namespace testing;
    EXPECT_THAT(gamma.red, ContainerEq(r));
    EXPECT_THAT(gamma.green, ContainerEq(g));
    EXPECT_THAT(gamma.blue, ContainerEq(b));
}

TEST_F(GammaCurves, are_empty_by_default)
{
    mg::GammaCurves gamma;

    EXPECT_THAT(gamma.red.size(), 0);
    EXPECT_THAT(gamma.green.size(), 0);
    EXPECT_THAT(gamma.blue.size(), 0);
}

TEST_F(GammaCurves, throw_with_differing_lut_sizes)
{
    EXPECT_THROW({ mg::GammaCurves({1}, {2, 3}, {4, 5, 6}); }, std::logic_error);
}

TEST(LinearGammaLUTs, throw_when_size_is_too_small)
{
    EXPECT_THROW({ mg::LinearGammaLUTs(1); }, std::logic_error);
}

TEST(LinearGammaLUTs, have_expected_size)
{
    int const expected_size = 10;
    mg::LinearGammaLUTs luts(expected_size);

    EXPECT_THAT(luts.red.size(), expected_size);
    EXPECT_THAT(luts.green.size(), expected_size);
    EXPECT_THAT(luts.blue.size(), expected_size);
}

TEST(LinearGammaLUTs, is_monotonically_increasing)
{
    mg::LinearGammaLUTs luts(256);

    EXPECT_THAT(luts.red, IsMonotonicallyIncreasing());
    EXPECT_THAT(luts.green, IsMonotonicallyIncreasing());
    EXPECT_THAT(luts.blue, IsMonotonicallyIncreasing());
}
