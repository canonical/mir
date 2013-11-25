/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_INPUT_RECEIVER_PLATFORM_H_
#define MIR_INPUT_RECEIVER_PLATFORM_H_

#include "mir_toolkit/event.h"

#include <memory>
#include <functional>

namespace mir
{
namespace input
{
namespace receiver
{
class InputReceiverThread;
class InputReceiverReport;

// Interface for MirSurface to construct input dispatcher threads.
class InputPlatform
{
public:
    virtual ~InputPlatform() {};

    virtual std::shared_ptr<InputReceiverThread> create_input_thread(int fd, std::function<void(MirEvent *)> const& callback) = 0;

    static std::shared_ptr<InputPlatform> create();
    static std::shared_ptr<InputPlatform> create(std::shared_ptr<InputReceiverReport> const& report);

protected:
    InputPlatform() = default;
    InputPlatform(const InputPlatform&) = delete;
    InputPlatform& operator=(const InputPlatform&) = delete;
};

}
}
} // namespace mir

#endif // MIR_INPUT_RECEIVER_PLATFORM_H_
