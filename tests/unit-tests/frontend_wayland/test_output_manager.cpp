/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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

#include "src/server/frontend_wayland/output_manager.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mf = mir::frontend;
namespace mw = mir::wayland;

using namespace testing;

namespace
{
  
struct TransformTestParams
{
    int32_t transform;
    MirOrientation orientation;
    MirMirrorMode mirror_mode;
};
}

class OutputManagerTest : public Test
{
};

class OutputManagerTransformTest : public OutputManagerTest, public WithParamInterface<TransformTestParams>
{
};

TEST_P(OutputManagerTransformTest, output_transform_to_orientation_and_mirror_mode)
{
    const auto [transform, orientation, mirror_mode] = GetParam();
    auto const result = mf::OutputManager::from_output_transform(transform);
    EXPECT_THAT(std::get<0>(result), Eq(orientation));
    EXPECT_THAT(std::get<1>(result), Eq(mirror_mode));
}

TEST_P(OutputManagerTransformTest, orientation_and_mirror_mode_to_output_transform)
{
    const auto [transform, orientation, mirror_mode] = GetParam();
    auto const result = mf::OutputManager::to_output_transform(orientation, mirror_mode);
    EXPECT_THAT(result, Eq(transform));
}

INSTANTIATE_TEST_SUITE_P(OutputManagerTransformTest, OutputManagerTransformTest, ::testing::Values(
    TransformTestParams(mw::Output::Transform::normal, mir_orientation_normal, mir_mirror_mode_none),
    TransformTestParams(mw::Output::Transform::_90, mir_orientation_left, mir_mirror_mode_none),
    TransformTestParams(mw::Output::Transform::_180, mir_orientation_inverted, mir_mirror_mode_none),
    TransformTestParams(mw::Output::Transform::_270, mir_orientation_right, mir_mirror_mode_none),
    TransformTestParams(mw::Output::Transform::flipped, mir_orientation_normal, mir_mirror_mode_horizontal),
    TransformTestParams(mw::Output::Transform::flipped_90, mir_orientation_left, mir_mirror_mode_horizontal),
    TransformTestParams(mw::Output::Transform::flipped_180, mir_orientation_inverted, mir_mirror_mode_horizontal),
    TransformTestParams(mw::Output::Transform::flipped_270, mir_orientation_right, mir_mirror_mode_horizontal)
));

TEST_F(OutputManagerTest, output_transform_throws_when_invalid)
{
    EXPECT_THROW(mf::OutputManager::from_output_transform(123), std::out_of_range);
}

