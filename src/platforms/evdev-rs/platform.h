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

#include "mir/input/platform.h"

namespace mir
{
namespace input
{
namespace evdev_rs
{
class Platform : public input::Platform
{
public:
    Platform() = default;
    std::shared_ptr<mir::dispatch::Dispatchable> dispatchable() override;
    void start() override;
    void stop() override;
    void pause_for_config() override;
    void continue_after_config() override;
};
}
}
}
