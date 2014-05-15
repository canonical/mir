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

#include "mir/scene/surface.h"
#include "mir/compositor/scene.h"

#include <InputDispatcher.h>

#include <boost/throw_exception.hpp>

#include <stdexcept>
#include <mutex>

namespace mi = mir::input;
namespace ms = mir::scene;
namespace mia = mi::android;
namespace mc = mir::compositor;

mia::InputRegistrar::InputRegistrar(std::shared_ptr<mc::Scene> const& scene) :
    scene(scene),
    observer(std::make_shared<SceneObserver>(
            [this](ms::Surface *surface) { add_window_handle_for_surface(surface); },
            [this](ms::Surface *surface) { remove_window_handle_for_surface(surface); }
            ))
{
    scene->add_observer(observer);
}

void mia::InputRegistrar::set_dispatcher(std::shared_ptr<droidinput::InputDispatcherInterface> const& dispatcher)
{
    input_dispatcher = dispatcher;
}

mia::InputRegistrar::~InputRegistrar() noexcept(true)
{
    scene->remove_observer(observer);
}

void mia::InputRegistrar::add_window_handle_for_surface(ms::Surface* surface)
{
    droidinput::sp<droidinput::InputWindowHandle> window_handle;
    {
        std::unique_lock<std::mutex> lock(handles_mutex);
        std::shared_ptr<InputChannel> channel = surface->input_channel();

        // TODO: We don't have much use for InputApplicationHandle so we simply use one per channel.
        // it is only used in droidinput for logging and determining application not responding (ANR),
        // we determine ANR on a per channel basis. When we have time we should factor InputApplicationHandle out
        // of the input stack (merging it's state with WindowHandle). ~racarr
        if (window_handles.find(surface->input_channel()) != window_handles.end())
            BOOST_THROW_EXCEPTION(std::logic_error("A channel was opened twice"));

        auto application_handle = new mia::InputApplicationHandle(surface);
        window_handle = new mia::InputWindowHandle(application_handle, channel, surface);

        window_handles[channel] = window_handle;
    }

    bool monitors_input = (surface->reception_mode() == mi::InputReceptionMode::receives_all_input);
    auto dispatcher = input_dispatcher.lock();
    dispatcher->registerInputChannel(window_handle->getInfo()->inputChannel, window_handle, monitors_input);
}

void mia::InputRegistrar::remove_window_handle_for_surface(ms::Surface* surface)
{
    droidinput::sp<droidinput::InputWindowHandle> window_handle;
    {
        std::unique_lock<std::mutex> lock(handles_mutex);
        auto it = window_handles.find(surface->input_channel());
        if (it == window_handles.end())
            BOOST_THROW_EXCEPTION(std::logic_error("A channel was closed twice"));
        window_handle = it->second;
        window_handles.erase(it);
    }
    auto dispatcher = input_dispatcher.lock();
    dispatcher->unregisterInputChannel(window_handle->getInputChannel());
}

droidinput::sp<droidinput::InputWindowHandle> mia::InputRegistrar::handle_for_channel(
    std::shared_ptr<input::InputChannel const> const& channel)
{
    std::unique_lock<std::mutex> lock(handles_mutex);
    if (window_handles.find(channel) == window_handles.end())
        BOOST_THROW_EXCEPTION(std::logic_error("Requesting handle for an unregistered channel"));
    return window_handles[channel];
}

mia::InputRegistrar::SceneObserver::SceneObserver(SurfaceCallback const& add, SurfaceCallback const& remove)
    : add(add), remove(remove)
{}

void mia::InputRegistrar::SceneObserver::surface_added(ms::Surface* surface)
{
    add(surface);
}

void mia::InputRegistrar::SceneObserver::surface_exists(ms::Surface* surface)
{
    add(surface);
}

void mia::InputRegistrar::SceneObserver::surface_removed(ms::Surface* surface)
{
    remove(surface);
}
