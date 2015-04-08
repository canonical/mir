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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_INPUT_DEFAULT_INPUT_DISPATCHER_H_
#define MIR_INPUT_DEFAULT_INPUT_DISPATCHER_H_

#include "mir/input/input_dispatcher.h"

#include <memory>
#include <mutex>

namespace mir
{
namespace input
{
class Surface;
class Scene;

// TODO: Add shell dispatcher interface for focus...or this is the focus targeter or something
class DefaultInputDispatcher : public mir::input::InputDispatcher
{
public:
    DefaultInputDispatcher(std::shared_ptr<input::Scene> const& scene);

// mir::input::InputDispatcher
    void configuration_changed(std::chrono::nanoseconds when) override;
    void device_reset(int32_t device_id, std::chrono::nanoseconds when) override;
    bool dispatch(MirEvent const& event) override;
    void start() override;
    void stop() override;

// FocusTargeter
    void set_focus(std::shared_ptr<input::Surface> const& target);
    
private:
    bool dispatch_key(MirKeyEvent *kev);
    bool dispatch_pointer(MirPointerEvent *pev);
    bool dispatch_touch(MirTouchEvent *tev);
    
    std::shared_ptr<input::Scene> const scene;

    std::mutex focus_mutex;
    std::weak_ptr<input::Surface> focus_surface;
};

}
}

#endif // MIR_INPUT_DEFAULT_INPUT_DISPATCHER_H_
