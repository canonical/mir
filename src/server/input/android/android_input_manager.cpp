/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 *              Daniel d'Andradra <daniel.dandrada@canonical.com>
 */

#include "mir/graphics/viewable_area.h"

#include "android_input_manager.h"
#include "android_input_constants.h"
#include "android_input_configuration.h"
#include "android_input_thread.h"
#include "android_input_channel.h"
#include "default_android_input_configuration.h"

#include <EventHub.h>
#include <InputDispatcher.h>

#include <memory>
#include <vector>

namespace mg = mir::graphics;
namespace mi = mir::input;
namespace mia = mi::android;

mia::InputManager::InputManager(std::shared_ptr<mia::InputConfiguration> const& config)
  : event_hub(config->the_event_hub()),
    dispatcher(config->the_dispatcher()),
    reader_thread(config->the_reader_thread()),
    dispatcher_thread(config->the_dispatcher_thread())
{
}

mia::InputManager::~InputManager()
{
}

void mia::InputManager::stop()
{
    dispatcher_thread->request_stop();
    dispatcher->setInputDispatchMode(mia::DispatchDisabled, mia::DispatchFrozen);
    dispatcher_thread->join();

    reader_thread->request_stop();
    event_hub->wake();
    reader_thread->join();
}

void mia::InputManager::start()
{
    dispatcher->setInputDispatchMode(mia::DispatchEnabled, mia::DispatchUnfrozen);
    dispatcher->setInputFilterEnabled(true);

    reader_thread->start();
    dispatcher_thread->start();
}

std::shared_ptr<mi::InputChannel> mia::InputManager::make_input_channel()
{
    return std::make_shared<mia::AndroidInputChannel>();
}

std::shared_ptr<mi::InputManager> mi::create_input_manager(
    const std::initializer_list<std::shared_ptr<mi::EventFilter> const>& event_filters,
    std::shared_ptr<mg::ViewableArea> const& view_area)
{
    static const std::shared_ptr<mi::CursorListener> null_cursor_listener{};
    auto config = std::make_shared<mia::DefaultInputConfiguration>(event_filters, view_area, null_cursor_listener);

    return std::make_shared<mia::InputManager>(config);
}
