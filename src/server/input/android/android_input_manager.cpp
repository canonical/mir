/*
 * Copyright © 2012 Canonical Ltd.
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
 *              Daniel d'Andradra <daniel.dandrada@canonical.com>
 */

#define MIR_INCLUDE_DEPRECATED_EVENT_HEADER

#include "mir/input/input_dispatcher.h"
#include "android_input_manager.h"
#include "android_input_constants.h"
#include "android_input_thread.h"
#include "android_input_channel.h"

#include <EventHub.h>

namespace mi = mir::input;
namespace mia = mi::android;

mia::InputManager::InputManager(std::shared_ptr<droidinput::EventHubInterface> const& event_hub,
                                std::shared_ptr<InputThread> const& reader_thread)
    : event_hub(event_hub), reader_thread(reader_thread)
{
}

mia::InputManager::~InputManager()
{
    // It is safe to call stop(), even if we haven't been started at all,
    // or we have been previously started and stopped manually.
    stop();
}

void mia::InputManager::stop()
{
    reader_thread->request_stop();
    event_hub->wake();
    reader_thread->join();
}

void mia::InputManager::start()
{
    event_hub->flush();
    reader_thread->start();
}
