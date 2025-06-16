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

#include "mir/graphics/transformation.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mg = mir::graphics;
using namespace testing;

class TransformationTest : public Test, public WithParamInterface<MirOrientation>
{
};

TEST_P(TransformationTest, InverseTransformIsInverseOfTransformation)
{
    auto const orientation = GetParam();
    EXPECT_THAT(
        mg::transformation(orientation) * mg::inverse_transformation(orientation),
        Eq(glm::mat2(1.f)));
}

INSTANTIATE_TEST_SUITE_P(TransformationTest, TransformationTest, ::testing::Values(
    mir_orientation_normal,
    mir_orientation_left,
    mir_orientation_inverted,
    mir_orientation_right
));
