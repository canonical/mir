/*
 * Copyright © 2018 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_STUB_CONSOLE_SERVICES_H_
#define MIR_TEST_DOUBLES_STUB_CONSOLE_SERVICES_H_

#include "mir/console_services.h"

#include <string>
#include <unordered_map>

namespace mir
{
namespace test
{
namespace doubles
{
class StubConsoleServices : public mir::ConsoleServices
{
public:
    void
    register_switch_handlers(
        mir::graphics::EventHandlerRegister&,
        std::function<bool()> const&,
        std::function<bool()> const&) override;

    void restore() override;

    std::unique_ptr<VTSwitcher> create_vt_switcher() override;

    std::future<std::unique_ptr<mir::Device>> acquire_device(
        int major, int minor,
        std::unique_ptr<mir::Device::Observer> observer) override;

private:
    char const* devnode_for_devnum(int major, int minor);

    int device_count{0};
    std::unordered_map<dev_t, std::string> devnodes;
};
}
}
}

#endif //MIR_TEST_DOUBLES_STUB_CONSOLE_SERVICES_H_
