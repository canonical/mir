/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "android_dispatcher_controller.h"

#include "android_input_window_handle.h"
#include "android_input_application_handle.h"

#include "mir/input/android/android_input_configuration.h"

#include <InputDispatcher.h>

namespace mi = mir::input;
namespace mia = mi::android;

mia::DispatcherController::DispatcherController(std::shared_ptr<mia::InputConfiguration> const& config) :
    input_dispatcher(config->the_dispatcher()),
    focused_window_handle(0),
    focused_application_handle(0)
{
}

void mia::DispatcherController::set_input_focus_to(std::shared_ptr<mi::SessionTarget> const& session,
    std::shared_ptr<mi::SurfaceTarget> const& surface)
{
    if (focused_window_handle.get())
    {
        input_dispatcher->unregisterInputChannel(focused_window_handle->getInfo()->inputChannel);
        focused_window_handle.clear();
        focused_application_handle.clear();
    }

    droidinput::Vector<droidinput::sp<droidinput::InputWindowHandle>> windows;
    if (surface)
    {
        focused_application_handle = new mia::InputApplicationHandle(session);
        focused_window_handle = new mia::InputWindowHandle(focused_application_handle, surface);
        input_dispatcher->setFocusedApplication(focused_application_handle);

        input_dispatcher->registerInputChannel(focused_window_handle->getInfo()->inputChannel, focused_window_handle, false);
        windows.push_back(focused_window_handle);
    }
    input_dispatcher->setInputWindows(windows);
}
