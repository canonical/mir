/*
 * Copyright © 2013 Canonical Ltd.
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
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_NULL_CONSOLE_SERVICES_H_
#define MIR_NULL_CONSOLE_SERVICES_H_

#include "linux_virtual_terminal.h"
#include "mir/console_services.h"
#include "mir/fd.h"
#include <boost/throw_exception.hpp>
#include <future>

namespace mir
{
class NullConsoleDevice : public Device
{
public:
    NullConsoleDevice(VTFileOperations& fops, std::unique_ptr<mir::Device::Observer> observer, std::string const& dev);
    void on_activated();
    void on_suspended();

private:
    VTFileOperations& fops;
    std::unique_ptr<mir::Device::Observer> observer;
    std::string dev;
};

class NullConsoleServices : public ConsoleServices
{
public:
    NullConsoleServices(std::shared_ptr<VTFileOperations> const& fops);
    void register_switch_handlers(graphics::EventHandlerRegister&,
                                  std::function<bool()> const&,
                                  std::function<bool()> const&) override;
    void restore() override;
    std::unique_ptr<VTSwitcher> create_vt_switcher() override;
    std::future<std::unique_ptr<Device>> acquire_device(int, int, std::unique_ptr<Device::Observer>) override;

private:
    const std::shared_ptr<VTFileOperations> fops;
};
}  // namespace mir

#endif /* MIR_NULL_CONSOLE_SERVICES_H_ */
