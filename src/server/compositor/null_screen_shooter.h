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

#ifndef MIR_COMPOSITOR_NULL_SCREEN_SHOOTER_H_
#define MIR_COMPOSITOR_NULL_SCREEN_SHOOTER_H_

#include "mir/compositor/screen_shooter.h"

namespace mir
{
class Executor;
namespace compositor
{

/// An implementation of ScreenShooter that always calls the callback with an error
class NullScreenShooter: public ScreenShooter
{
public:
    NullScreenShooter(Executor& executor);

    void capture(
        std::shared_ptr<renderer::software::WriteMappableBuffer> const& buffer,
        geometry::Rectangle const& area,
        bool overlay_cursor,
        std::function<void(std::optional<time::Timestamp>)>&& callback) override;

    CompositorID id() const override;

private:
    Executor& executor;
};
}
}

#endif // MIR_COMPOSITOR_NULL_SCREEN_SHOOTER_H_
