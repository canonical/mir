/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_INPUT_INPUT_EVENT_HANDLER_REGISTER_H_
#define MIR_INPUT_INPUT_EVENT_HANDLER_REGISTER_H_

#include <functional>
#include <initializer_list>

namespace mir
{
namespace input
{

/**
 * InputEventHandler Register shall be used register handlers for different
 * input event or input device sources. All handlers are guaranteed to be executed in the same
 * thread.
 */
class InputEventHandlerRegister
{
public:
    InputEventHandlerRegister() = default;

    /**
     * Registers a read handler for the given set of fds.
     *
     * This call does not transfer ownership of the file descriptors
     * in \a fds. The caller is still responsible to close when necessary.
     *
     * \param owner a distinct value to identify the caller
     */
    virtual void register_fd_handler(
        std::initializer_list<int> fds,
        void const* owner,
        std::function<void(int)> const&& handler) = 0;

    /**
     * Unregisters the file descriptors and read handlers associated with \a
     * owner.
     *
     * \param owner a distinct value to identify the caller
     */
    virtual void unregister_fd_handler(void const* owner) = 0;

    /**
     * Wakes the thread to execute the given \a handler once.
     *
     * This method should be used when no file descriptor is available to
     * indicate available input events or device changes.
     */
    virtual void register_handler(std::function<void()> const&& handler) = 0;

protected:
    virtual ~InputEventHandlerRegister() = default;
    InputEventHandlerRegister(InputEventHandlerRegister const&) = delete;
    InputEventHandlerRegister& operator=(InputEventHandlerRegister const&) = delete;

};
}
}

#endif

