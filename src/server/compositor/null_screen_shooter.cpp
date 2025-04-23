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

mc::NullScreenShooter::NullScreenShooter(Executor& executor)
    : executor{executor}
{
}

void mc::NullScreenShooter::capture(
    std::shared_ptr<mrs::WriteMappableBuffer> const&,
    geom::Rectangle const&,
    std::function<void(std::optional<time::Timestamp>)>&& callback)
{
    log_warning("Failed to capture screen because NullScreenShooter is in use");
    executor.spawn([callback=std::move(callback)]
        {
            callback(std::nullopt);
        });
}

void mc::NullScreenShooter::capture_with_cursor(
    std::shared_ptr<mrs::WriteMappableBuffer> const&,
    geom::Rectangle const&,
    bool,
    std::function<void(std::optional<time::Timestamp>)>&& callback)
{
    log_warning("Failed to capture screen because NullScreenShooter is in use");
    executor.spawn([callback=std::move(callback)]
        {
            callback(std::nullopt);
        });
}

void mc::NullScreenShooter::capture_with_filter(
    std::shared_ptr<mrs::WriteMappableBuffer> const&,
    geom::Rectangle const&,
    std::function<bool(std::shared_ptr<SceneElement const> const&)> const&,
    bool,
    std::function<void(std::optional<time::Timestamp>)>&& callback)
{
    log_warning("Failed to capture screen because NullScreenShooter is in use");
    executor.spawn([callback=std::move(callback)]
        {
            callback(std::nullopt);
        });
}
