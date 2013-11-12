/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored By: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_SHELL_SESSION_EVENT_HANDLER_REGISTER_H_
#define MIR_SHELL_SESSION_EVENT_HANDLER_REGISTER_H_

#include <functional>
#include <memory>

namespace mir
{
namespace shell
{

class Session;

class SessionEventHandlerRegister
{
public:
    virtual ~SessionEventHandlerRegister() = default;

    virtual void register_focus_change_handler(
        std::function<void(std::shared_ptr<Session> const& session)> const& handler) = 0;
    virtual void register_no_focus_handler(
        std::function<void()> const& handler) = 0;
    virtual void register_session_stopping_handler(
        std::function<void(std::shared_ptr<Session> const& session)> const& handler) = 0;

protected:
    SessionEventHandlerRegister() = default;
    SessionEventHandlerRegister(SessionEventHandlerRegister const&) = delete;
    SessionEventHandlerRegister& operator=(SessionEventHandlerRegister const&) = delete;
};

}
}

#endif /* MIR_SHELL_SESSION_EVENT_HANDLER_REGISTER_H_ */
