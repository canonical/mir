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

#ifndef MIR_COMPOSITOR_NULL_SCREEN_SHOOTER_FACTORY_H
#define MIR_COMPOSITOR_NULL_SCREEN_SHOOTER_FACTORY_H

#include "mir/compositor/screen_shooter_factory.h"

namespace mir
{
class Executor;

namespace compositor
{
class NullScreenShooterFactory : public ScreenShooterFactory
{
public:
    auto create(Executor& executor) -> std::unique_ptr<ScreenShooter> override;
};
}
}

#endif //MIR_COMPOSITOR_NULL_SCREEN_SHOOTER_FACTORY_H
