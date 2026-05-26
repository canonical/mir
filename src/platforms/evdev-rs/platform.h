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

#include <mir/console_services.h>
#include <mir/input/platform.h>
#include <memory>
#include <string>

namespace mir
{
class ConsoleServices;

namespace input
{
class InputDeviceRegistry;
class InputReport;

namespace evdev_rs
{
class Platform : public input::Platform
{
public:
    Platform(std::shared_ptr<ConsoleServices> const& console,
        std::shared_ptr<InputDeviceRegistry> const& input_device_registry,
        std::shared_ptr<input::InputReport> const& report);
    std::shared_ptr<mir::dispatch::Dispatchable> dispatchable() override;
    void start() override;
    void stop() override;
    void pause_for_config() override;
    void continue_after_config() override;
    std::shared_ptr<InputDevice> create_input_device(int device_id) const;

    /// Create a Device::Observer that forwards lifecycle events to the Rust platform.
    std::unique_ptr<mir::Device::Observer> create_device_observer(
        std::string const& devnode, uint64_t devnum);

private:
    class Self;
    std::shared_ptr<Self> self;
};
}
}
}
