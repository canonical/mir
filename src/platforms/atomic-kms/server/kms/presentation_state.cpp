/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "presentation_state.h"

#include <mir/fatal.h>
#include <mir/graphics/kms_framebuffer.h>

#include <utility>

namespace mga = mir::graphics::atomic;

mga::PresentationState::PresentationState(
    std::shared_ptr<FBHandle const> primary,
    std::vector<ClientPlaneAssignment> client_planes) :
    primary_{std::move(primary)},
    client_planes_{std::move(client_planes)}
{
}

auto mga::PresentationState::full_scene(std::shared_ptr<FBHandle const> primary) -> PresentationState
{
    return PresentationState{std::move(primary), {}};
}

auto mga::PresentationState::mixed(
    std::shared_ptr<FBHandle const> primary,
    std::vector<ClientPlaneAssignment> client_planes) -> PresentationState
{
    if (client_planes.empty())
    {
        MIR_FATAL_ERROR("A partial primary cannot be presented without client planes");
    }
    return PresentationState{std::move(primary), std::move(client_planes)};
}

auto mga::PresentationState::primary() const -> FBHandle const&
{
    return *primary_;
}

auto mga::PresentationState::client_planes() const -> std::span<ClientPlaneAssignment const>
{
    return client_planes_;
}

auto mga::PresentationState::primary_holds_full_scene() const -> bool
{
    return client_planes_.empty();
}
