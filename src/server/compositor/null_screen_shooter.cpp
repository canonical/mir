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

#include "null_screen_shooter.h"
#include "mir/log.h"
#include "mir/executor.h"

namespace mc = mir::compositor;
namespace mrs = mir::renderer::software;
namespace geom = mir::geometry;
namespace mg = mir::graphics;

mc::NullScreenShooter::NullScreenShooter(Executor& executor)
    : executor{executor}
{
}

void mc::NullScreenShooter::capture(
    geom::Rectangle const&,
    glm::mat2 const&,
    bool,
    std::function<void(std::optional<time::Timestamp>, std::shared_ptr<mg::Buffer>)>&& callback)
{
    log_warning("Failed to capture screen because NullScreenShooter is in use");
    executor.spawn([callback=std::move(callback)]
        {
            callback(std::nullopt, {});
        });
}

mc::CompositorID mc::NullScreenShooter::id() const
{
    return this;
}
