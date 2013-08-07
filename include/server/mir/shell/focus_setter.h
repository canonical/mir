/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_SHELL_FOCUS_SETTER_H_
#define MIR_SHELL_FOCUS_SETTER_H_

#include <memory>

namespace mir
{

namespace shell
{
class Session;

/// Interface used by the Shell to customize behavior based around focus
class FocusSetter
{
public:
    virtual ~FocusSetter() {}

    virtual void surface_created_for(std::shared_ptr<Session> const& session) = 0;
    virtual void session_opened(std::shared_ptr<Session> const& session) = 0;
    virtual void session_closed(std::shared_ptr<Session> const& session) = 0;

    //TODO: this is only used in example code
    virtual void focus_next() = 0;

    virtual std::weak_ptr<Session> focused_session() const = 0;

protected:
    FocusSetter() = default;
    FocusSetter(const FocusSetter&) = delete;
    FocusSetter& operator=(const FocusSetter&) = delete;
};

}
}


#endif // MIR_SHELL_FOCUS_SETTER_H_
