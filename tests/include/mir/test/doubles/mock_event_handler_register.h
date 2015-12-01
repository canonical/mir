/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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

#ifndef MIR_TEST_DOUBLES_MOCK_EVENT_HANDLER_REGISTER_H_
#define MIR_TEST_DOUBLES_MOCK_EVENT_HANDLER_REGISTER_H_

#include "mir/graphics/event_handler_register.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

class MockEventHandlerRegister : public graphics::EventHandlerRegister
{
public:
    MOCK_METHOD2(register_signal_handler,
                 void(std::initializer_list<int>,
                 std::function<void(int)> const&));
    MOCK_METHOD2(register_signal_handler_module_ptr,
                 void(std::initializer_list<int>,
                 std::function<void(int)> const&));
    MOCK_METHOD3(register_fd_handler,
                 void(std::initializer_list<int>,
                 void const*, std::function<void(int)> const&));
    MOCK_METHOD3(register_fd_handler_module_ptr,
                 void(std::initializer_list<int>,
                 void const*, std::function<void(int)> const&));
    MOCK_METHOD1(unregister_fd_handler,
                 void(void const*));

    void register_signal_handler(
        std::initializer_list<int> sigs,
        mir::UniqueModulePtr<std::function<void(int)>> handler)
    {
        register_signal_handler_module_ptr(sigs, *handler);
    }

    void register_fd_handler(
         std::initializer_list<int> fds,
         void const* owner,
         mir::UniqueModulePtr<std::function<void(int)>> handler)
    {
        register_fd_handler_module_ptr(fds, owner, *handler);
    }
};

}
}
}

#endif
