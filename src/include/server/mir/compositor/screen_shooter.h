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

#ifndef MIR_COMPOSITOR_SCREEN_SHOOTER_H_
#define MIR_COMPOSITOR_SCREEN_SHOOTER_H_

#include "mir/geometry/rectangle.h"
#include "mir/time/types.h"

#include <functional>
#include <memory>
#include <optional>

namespace mir
{
namespace renderer
{
namespace software
{
class WriteMappableBuffer;
}
}
namespace compositor
{
class SceneElement;

class ScreenShooter
{
public:
    ScreenShooter() = default;
    virtual ~ScreenShooter() = default;

    /// The callback may be called on a different thread. It will be given the timestamp of the capture if it succeeds
    /// or nullopt if there was an error.
    virtual void capture(
        std::shared_ptr<renderer::software::WriteMappableBuffer> const& buffer,
        mir::geometry::Rectangle const& area,
        std::function<void(std::optional<time::Timestamp>)>&& callback) = 0;

    virtual void capture_with_cursor(
        std::shared_ptr<renderer::software::WriteMappableBuffer> const& buffer,
        mir::geometry::Rectangle const& area,
        bool with_cursor,
        std::function<void(std::optional<time::Timestamp>)>&& callback) = 0;

    virtual void capture_with_filter(
        std::shared_ptr<renderer::software::WriteMappableBuffer> const& buffer,
        geometry::Rectangle const& area,
        std::function<bool(std::shared_ptr<SceneElement const> const&)> const& filter,
        bool with_cursor,
        std::function<void(std::optional<time::Timestamp>)>&& callback) = 0;

private:
    ScreenShooter(ScreenShooter const&) = delete;
    ScreenShooter& operator=(ScreenShooter const&) = delete;
};
}
}

#endif // MIR_COMPOSITOR_SCREEN_SHOOTER_H_
