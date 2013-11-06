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

#include "focus_setter.h"

#include <memory>
#include <mutex>

namespace mir
{

namespace shell
{
class Surface;
class InputTargeter;
class SurfaceController;

class DefaultFocusMechanism : public FocusSetter
{
public:
    explicit DefaultFocusMechanism(std::shared_ptr<InputTargeter> const& input_targeter,
                                   std::shared_ptr<SurfaceController> const& surface_controller);
    virtual ~DefaultFocusMechanism() = default;

    void set_focus_to(std::shared_ptr<shell::Session> const& new_focus);

protected:
    DefaultFocusMechanism(const DefaultFocusMechanism&) = delete;
    DefaultFocusMechanism& operator=(const DefaultFocusMechanism&) = delete;

private:
    std::shared_ptr<InputTargeter> const input_targeter;
    std::shared_ptr<SurfaceController> const surface_controller;

    std::mutex surface_focus_lock;
    std::weak_ptr<Surface> currently_focused_surface;
};

}
}


#endif // MIR_SHELL_SINGLE_VISIBILITY_FOCUS_MECHANISM_H_
