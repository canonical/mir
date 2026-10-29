/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

#include "presentation_state.h"

#include "mock_kms_framebuffer.h"

#include <mir/test/death.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <csignal>
#include <memory>

namespace mga = mir::graphics::atomic;
namespace mt = mir::test;

using namespace testing;

namespace
{
constexpr mga::PlaneId overlay_plane{31};
}

TEST(PresentationState, only_a_state_without_client_planes_holds_the_full_scene)
{
    auto const primary = std::make_shared<NiceMock<mt::MockKMSFramebuffer>>();
    auto const overlay = std::make_shared<NiceMock<mt::MockKMSFramebuffer>>();

    auto const full = mga::PresentationState::full_scene(primary);
    EXPECT_TRUE(full.primary_holds_full_scene());

    auto const mixed = mga::PresentationState::mixed(
        primary,
        {mga::ClientPlaneAssignment{overlay_plane, overlay}});
    EXPECT_FALSE(mixed.primary_holds_full_scene());
}

TEST(PresentationState, mixed_state_without_client_planes_is_fatal)
{
    auto const primary = std::make_shared<NiceMock<mt::MockKMSFramebuffer>>();

    MIR_EXPECT_EXIT(
        {
            mga::PresentationState::mixed(primary, {});
        },
        KilledBySignal(SIGABRT),
        "A partial primary cannot be presented without client planes.*"
        "presentation_state.cpp:[0-9]+ in .*PresentationState::mixed.*");
}
