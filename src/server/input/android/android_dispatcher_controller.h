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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_INPUT_ANDROID_DISPATCHER_CONTROLLER_H_
#define MIR_INPUT_ANDROID_DISPATCHER_CONTROLLER_H_

#include "mir/shell/input_target_listener.h"

#include <utils/StrongPointer.h>

#include <map>
#include <mutex>

namespace android
{
class InputDispatcherInterface;
class InputWindowHandle;
class InputApplicationHandle;
}

namespace droidinput = android;

namespace mir
{
namespace input
{
namespace android
{
class InputConfiguration;

class DispatcherController : public shell::InputTargetListener
{
public:
    explicit DispatcherController(std::shared_ptr<InputConfiguration> const& input_configuration);
    virtual ~DispatcherController() noexcept(true) {}
    
    void input_application_opened(std::shared_ptr<input::SessionTarget> const& application);
    void input_application_closed(std::shared_ptr<input::SessionTarget> const& application);

    void input_surface_opened(std::shared_ptr<input::SessionTarget> const& application,
       std::shared_ptr<input::SurfaceTarget> const& opened_surface);
    void input_surface_closed(std::shared_ptr<input::SurfaceTarget> const& closed_surface);

    void focus_changed(std::shared_ptr<input::SurfaceTarget> const& focus_surface);
    void focus_cleared();

protected:
    DispatcherController(const DispatcherController&) = delete;
    DispatcherController& operator=(const DispatcherController&) = delete;

private:
    droidinput::sp<droidinput::InputDispatcherInterface> input_dispatcher;

    std::map<std::shared_ptr<input::SessionTarget>, droidinput::sp<droidinput::InputApplicationHandle>> application_handles;
    std::map<std::shared_ptr<input::SurfaceTarget>, droidinput::sp<droidinput::InputWindowHandle>> window_handles;

    std::mutex handles_mutex;
};

}
}
} // namespace mir

#endif // MIR_INPUT_ANDROID_DISPATCHER_CONTROLLER_H_
