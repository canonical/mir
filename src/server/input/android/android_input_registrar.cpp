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

#include "android_input_registrar.h"

#include "android_input_window_handle.h"
#include "android_input_application_handle.h"

#include <InputDispatcher.h>

#include <boost/throw_exception.hpp>

#include <stdexcept>
#include <mutex>

namespace mi = mir::input;
namespace mia = mi::android;
namespace ms = mir::scene;

mia::InputRegistrar::InputRegistrar(droidinput::sp<droidinput::InputDispatcherInterface> const& input_dispatcher)
    : input_dispatcher(input_dispatcher)
{
}

mia::InputRegistrar::~InputRegistrar() noexcept(true)
{
}

// Be careful on the locking in these two functions.
void mia::InputRegistrar::input_channel_opened(std::shared_ptr<input::InputChannel> const& channel,
                                               std::shared_ptr<mi::Surface> const& surface,
                                               mi::InputReceptionMode input_mode)
{
    droidinput::sp<droidinput::InputWindowHandle> window_handle;
    {
        std::unique_lock<std::mutex> lock(handles_mutex);

        // TODO: We don't have much use for InputApplicationHandle so we simply use one per channel.
        // it is only used in droidinput for logging and determining application not responding (ANR),
        // we determine ANR on a per channel basis. When we have time we should factor InputApplicationHandle out
        // of the input stack (merging it's state with WindowHandle). ~racarr
        if (window_handles.find(channel) != window_handles.end())
            BOOST_THROW_EXCEPTION(std::logic_error("A channel was opened twice"));

        auto application_handle = new mia::InputApplicationHandle(surface);
        window_handle = new mia::InputWindowHandle(application_handle, channel, surface);
    
        window_handles[channel] = window_handle;
    }

    bool monitors_input = (input_mode == mi::InputReceptionMode::receives_all_input);
    input_dispatcher->registerInputChannel(window_handle->getInfo()->inputChannel, window_handle, monitors_input);
}

void mia::InputRegistrar::input_channel_closed(std::shared_ptr<input::InputChannel> const& closed_channel)
{
    droidinput::sp<droidinput::InputWindowHandle> window_handle;
    {
        std::unique_lock<std::mutex> lock(handles_mutex);
        auto it = window_handles.find(closed_channel);
        if (it == window_handles.end())
            BOOST_THROW_EXCEPTION(std::logic_error("A channel was closed twice"));
        window_handle = it->second;
        window_handles.erase(it);
    }
    input_dispatcher->unregisterInputChannel(window_handle->getInputChannel());
}

droidinput::sp<droidinput::InputWindowHandle> mia::InputRegistrar::handle_for_channel(
    std::shared_ptr<input::InputChannel const> const& channel)
{
    std::unique_lock<std::mutex> lock(handles_mutex);
    if (window_handles.find(channel) == window_handles.end())
        BOOST_THROW_EXCEPTION(std::logic_error("Requesting handle for an unregistered channel"));
    return window_handles[channel];
}
