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

#include "null_screen_shooter_factory.h"
#include "null_screen_shooter.h"

namespace mc = mir::compositor;

mc::NullScreenShooterFactory::NullScreenShooterFactory(Executor& executor)
    : executor(executor)
{
}

auto mc::NullScreenShooterFactory::create() -> std::unique_ptr<ScreenShooter>
{
    return std::make_unique<NullScreenShooter>(executor);
}
