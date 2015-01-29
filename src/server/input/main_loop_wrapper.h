/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_INPUT_MAIN_LOOP_WRAPPER_H
#define MIR_INPUT_MAIN_LOOP_WRAPPER_H

#include "mir/input/input_event_handler_register.h"
#include <memory>

namespace mir
{
class MainLoop;
namespace input
{

class MainLoopWrapper : public InputEventHandlerRegister
{
public:
    MainLoopWrapper(std::shared_ptr<MainLoop> const& loop);
    void register_fd_handler(std::initializer_list<int> fds,
        void const* owner, std::function<void(int)> const&& handler) override;
    void unregister_fd_handler(void const* owner) override;
    void register_handler(std::function<void()> const&& handler) override;

private:
    std::shared_ptr<MainLoop> const loop;
};

}
}

#endif
