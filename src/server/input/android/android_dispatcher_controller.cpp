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

#include "android_dispatcher_controller.h"

#include "android_input_window_handle.h"
#include "android_input_application_handle.h"

#include "mir/input/android/android_input_configuration.h"

#include <InputDispatcher.h>

#include <boost/throw_exception.hpp>

#include <stdexcept>
#include <mutex>

namespace mi = mir::input;
namespace mia = mi::android;

mia::DispatcherController::DispatcherController(std::shared_ptr<mia::InputConfiguration> const& config) :
    input_dispatcher(config->the_dispatcher())
{
}

void mia::DispatcherController::input_application_opened(std::shared_ptr<mi::SessionTarget> const& session)
{
    std::unique_lock<std::mutex> lock(handles_mutex);
    if (application_handles.find(session) != application_handles.end())
        BOOST_THROW_EXCEPTION(std::logic_error("An application was opened twice"));
    application_handles[session] = new mia::InputApplicationHandle(session);
}

void mia::DispatcherController::input_application_closed(std::shared_ptr<mi::SessionTarget> const& session)
{
    std::unique_lock<std::mutex> lock(handles_mutex);
    if (application_handles.find(session) == application_handles.end())
        BOOST_THROW_EXCEPTION(std::logic_error("An application was closed twice"));
    application_handles.erase(session);
}

void mia::DispatcherController::input_surface_opened(std::shared_ptr<mi::SessionTarget> const& session,
                                                     std::shared_ptr<input::SurfaceTarget> const& opened_surface)
{
    std::unique_lock<std::mutex> lock(handles_mutex);
    auto application_handle = application_handles.find(session);
    if (application_handle == application_handles.end())
        BOOST_THROW_EXCEPTION(std::logic_error("A surface was opened for an unopened application"));
    if (window_handles.find(opened_surface) != window_handles.end())
        BOOST_THROW_EXCEPTION(std::logic_error("A surface was opened twice"));

    droidinput::sp<droidinput::InputWindowHandle> window_handle = new mia::InputWindowHandle(application_handle->second, opened_surface);
    input_dispatcher->registerInputChannel(window_handle->getInfo()->inputChannel, window_handle, false);
    
    window_handles[opened_surface] = window_handle;
}

void mia::DispatcherController::input_surface_closed(std::shared_ptr<input::SurfaceTarget> const& closed_surface)
{
    std::unique_lock<std::mutex> lock(handles_mutex);
    auto it = window_handles.find(closed_surface);
    if (it == window_handles.end())
        BOOST_THROW_EXCEPTION(std::logic_error("A surface was closed twice"));

    input_dispatcher->unregisterInputChannel(it->second->getInfo()->inputChannel);
    window_handles.erase(it);
}

void mia::DispatcherController::focus_cleared()
{
    droidinput::Vector<droidinput::sp<droidinput::InputWindowHandle>> empty_windows;
    droidinput::sp<droidinput::InputApplicationHandle> null_application = nullptr;
    
    input_dispatcher->setFocusedApplication(null_application);
    input_dispatcher->setInputWindows(empty_windows);
}

void mia::DispatcherController::focus_changed(std::shared_ptr<mi::SurfaceTarget> const& surface)
{
    std::unique_lock<std::mutex> lock(handles_mutex);

    auto window_handle = window_handles[surface];

    if (!window_handle.get())
        BOOST_THROW_EXCEPTION(std::logic_error("Focus changed to an unopened surface"));
    auto application_handle = window_handle->inputApplicationHandle;
    
    input_dispatcher->setFocusedApplication(application_handle);

    droidinput::Vector<droidinput::sp<droidinput::InputWindowHandle>> windows;
    windows.push_back(window_handle);

    input_dispatcher->setInputWindows(windows);
}
