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

#ifndef MIR_SHELL_SINGLE_VISIBILITY_FOCUS_MECHANISM_H_
#define MIR_SHELL_SINGLE_VISIBILITY_FOCUS_MECHANISM_H_

#include <memory>
#include "mir/shell/focus_setter.h"

namespace mir
{

namespace shell
{
class Surface;
class InputTargeter;
class SurfaceController;
class SessionListener;
class FocusSequence;

class DefaultFocusMechanism : public FocusSetter
{
public:
    explicit DefaultFocusMechanism(std::shared_ptr<FocusSequence> const& sequence,
                                   std::shared_ptr<InputTargeter> const& input_targeter,
                                   std::shared_ptr<SurfaceController> const& surface_controller,
                                   std::shared_ptr<SessionListener> const& session_listener);
    virtual ~DefaultFocusMechanism() = default;

    void surface_created_for(std::shared_ptr<Session> const& session);
    void session_opened(std::shared_ptr<Session> const& session);
    void session_closed(std::shared_ptr<Session> const& session);
    std::weak_ptr<Session> focused_session() const;

    //TODO: this is only used in example code
    void focus_next();

protected:
    void focus_next_locked();

    DefaultFocusMechanism(const DefaultFocusMechanism&) = delete;
    DefaultFocusMechanism& operator=(const DefaultFocusMechanism&) = delete;

private:
    void set_focus(std::shared_ptr<Session> const& session);

    std::shared_ptr<FocusSequence> const sequence;
    std::shared_ptr<SessionListener> const session_listener;
    std::shared_ptr<InputTargeter> const input_targeter;
    std::shared_ptr<SurfaceController> const surface_controller;

    std::mutex mutable mutex;
    std::shared_ptr<Session> focus_session;
    // TODO: Protected wiht mutex?
    std::weak_ptr<Surface> currently_focused_surface;
};

}
}


#endif // MIR_SHELL_SINGLE_VISIBILITY_FOCUS_MECHANISM_H_
