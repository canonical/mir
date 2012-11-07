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

#include "mir/input/input_manager.h"
#include "mir/input/event_filter_dispatcher_policy.h"
#include "mir/input/dummy_input_reader_policy.h"

#include <memory>
#include <vector>

#include <InputReader.h>
#include <InputDispatcher.h>

namespace mi = mir::input;

mi::InputManager::InputManager()
{
    filter_chain = std::make_shared<mi::EventFilterChain>();
}

void mi::InputManager::add_filter(std::shared_ptr<mi::EventFilter> const& filter)
{
    filter_chain->add_filter(filter);
}

void mi::InputManager::stop()
{
    dispatcher->dispatchOnce();
    reader_thread->requestExitAndWait();
// This produces wild results in the integration test :/
//    dispatcher_thread->requestExitAndWait();
}

void mi::InputManager::start(std::shared_ptr<android::EventHubInterface> const& event_hub)
{
    android::sp<android::InputDispatcherPolicyInterface> dispatcher_policy = new mi::EventFilterDispatcherPolicy(filter_chain);
    android::sp<android::InputReaderPolicyInterface> reader_policy = new mi::DummyInputReaderPolicy();
    dispatcher = new android::InputDispatcher(dispatcher_policy);
    reader = new android::InputReader(android::sp<android::EventHubInterface>(event_hub.get()), reader_policy, dispatcher); // oO :/
    reader_thread = new android::InputReaderThread(reader);
    dispatcher_thread = new android::InputDispatcherThread(dispatcher);
    
    dispatcher->setInputDispatchMode(true, false);
    dispatcher->setInputFilterEnabled(true);
    dispatcher_thread->run("InputDispatcher", android::PRIORITY_URGENT_DISPLAY);
    reader_thread->run("InputReader", android::PRIORITY_URGENT_DISPLAY);
}

