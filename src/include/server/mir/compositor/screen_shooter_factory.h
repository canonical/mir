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

#ifndef SCREEN_SHOOTER_FACTORY_H
#define SCREEN_SHOOTER_FACTORY_H

#include "mir/compositor/screen_shooter.h"
#include <memory>

namespace mir
{
class Executor;

namespace compositor
{
class ScreenShooterFactory
{
public:
    ScreenShooterFactory() = default;
    virtual ~ScreenShooterFactory() = default;

    /// Creates a new screen shooter
    /// \param executor responsible for queuing the screenshot request
    /// \return a new screen shooter
    virtual auto create(Executor& executor) -> std::unique_ptr<ScreenShooter> = 0;
};
}
}

#endif //SCREEN_SHOOTER_FACTORY_H
