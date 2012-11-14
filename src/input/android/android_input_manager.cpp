/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
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

#include "android_input_manager.h"
#include "android_input_constants.h"
#include "event_filter_dispatcher_policy.h"
#include "dummy_input_reader_policy.h"

#include <EventHub.h>
#include <InputDispatcher.h>
#include <InputReader.h>

#include <memory>
#include <vector>

namespace mi = mir::input;
namespace mia = mi::android;

mia::InputManager::InputManager(const droidinput::sp<droidinput::EventHubInterface>& event_hub,
                                std::initializer_list<std::shared_ptr<mi::EventFilter> const> filters) :
  event_hub(event_hub),
  filter_chain(std::shared_ptr<mi::EventFilterChain>(new mi::EventFilterChain(filters)))
{
    droidinput::sp<droidinput::InputDispatcherPolicyInterface> dispatcher_policy = new mia::EventFilterDispatcherPolicy(filter_chain);
    droidinput::sp<droidinput::InputReaderPolicyInterface> reader_policy = new mia::DummyInputReaderPolicy();
    dispatcher = new droidinput::InputDispatcher(dispatcher_policy);
    reader = new droidinput::InputReader(event_hub, reader_policy, dispatcher);
    reader_thread = new droidinput::InputReaderThread(reader);
    dispatcher_thread = new droidinput::InputDispatcherThread(dispatcher);

    dispatcher->setInputDispatchMode(mia::DispatchEnabled, mia::DispatchUnfrozen);
    dispatcher->setInputFilterEnabled(true);
}

mia::InputManager::~InputManager()
{
}

void mia::InputManager::stop()
{
    reader_thread->requestExit();
    dispatcher_thread->requestExit();
}

void mia::InputManager::start()
{
    dispatcher_thread->run("InputDispatcher", droidinput::PRIORITY_URGENT_DISPLAY);
    reader_thread->run("InputReader", droidinput::PRIORITY_URGENT_DISPLAY);
}
