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

#ifndef MIR_GRAPHICS_ATOMIC_PRESENTATION_STATE_H_
#define MIR_GRAPHICS_ATOMIC_PRESENTATION_STATE_H_

#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace mir::graphics
{
class FBHandle;
}

namespace mir::graphics::atomic
{
using PlaneId = uint32_t;

/// A client framebuffer scanned out directly on an additional plane
struct ClientPlaneAssignment
{
    PlaneId plane;
    /// The imported framebuffer, which keeps the client buffer it was imported from alive
    std::shared_ptr<FBHandle const> framebuffer;
};

/// Everything a single presentation puts on an output, and owns while it is queued or on screen
class PresentationState
{
public:
    /// A state whose primary framebuffer is complete on its own
    static auto full_scene(std::shared_ptr<FBHandle const> primary) -> PresentationState;

    /// A state whose primary framebuffer omits the content scanned out from \a client_planes
    ///
    /// \a client_planes must not be empty: a partial primary is never presentable on its own.
    /// Violating this is a programming error and aborts.
    static auto mixed(std::shared_ptr<FBHandle const> primary, std::vector<ClientPlaneAssignment> client_planes)
        -> PresentationState;

    auto primary() const -> FBHandle const&;
    auto client_planes() const -> std::span<ClientPlaneAssignment const>;
    auto primary_holds_full_scene() const -> bool;

private:
    PresentationState(std::shared_ptr<FBHandle const> primary, std::vector<ClientPlaneAssignment> client_planes);

    std::shared_ptr<FBHandle const> primary_;
    std::vector<ClientPlaneAssignment> client_planes_;
};
}

#endif // MIR_GRAPHICS_ATOMIC_PRESENTATION_STATE_H_
