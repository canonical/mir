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
 * Authored By: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_SCENE_SESSION_EVENT_HANDLER_REGISTER_H_
#define MIR_SCENE_SESSION_EVENT_HANDLER_REGISTER_H_

#include <memory>

namespace mir
{
namespace scene
{
class SessionEventSink;

class SessionEventHandlerRegister
{
public:
    virtual ~SessionEventHandlerRegister() = default;

    virtual void add(SessionEventSink* handler) = 0;
    virtual void remove(SessionEventSink* handler) = 0;

protected:
    SessionEventHandlerRegister() = default;
    SessionEventHandlerRegister(SessionEventHandlerRegister const&) = delete;
    SessionEventHandlerRegister& operator=(SessionEventHandlerRegister const&) = delete;
};

}
}

#endif /* MIR_SCENE_SESSION_EVENT_HANDLER_REGISTER_H_ */
