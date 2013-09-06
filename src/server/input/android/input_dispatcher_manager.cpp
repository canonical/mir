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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "input_dispatcher_manager.h"
#include "android_input_constants.h"
#include "android_input_thread.h"
#include "android_input_channel.h"

#include <InputDispatcher.h>

namespace mi = mir::input;
namespace mia = mi::android;

mia::InputDispatcherManager::InputDispatcherManager(
    droidinput::sp<droidinput::InputDispatcherInterface> const& dispatcher,
    std::shared_ptr<InputThread> const& dispatcher_thread) :
    dispatcher(dispatcher),
    dispatcher_thread(dispatcher_thread)
{
}

mia::InputDispatcherManager::~InputDispatcherManager()
{
}

void mia::InputDispatcherManager::stop()
{
    dispatcher_thread->request_stop();
    dispatcher->setInputDispatchMode(mia::DispatchDisabled, mia::DispatchFrozen);
    dispatcher_thread->join();
}

void mia::InputDispatcherManager::start()
{
    dispatcher->setInputDispatchMode(mia::DispatchEnabled, mia::DispatchUnfrozen);
    dispatcher->setInputFilterEnabled(true);
    dispatcher_thread->start();
}

std::shared_ptr<mi::InputChannel> mia::InputDispatcherManager::make_input_channel()
{
    return std::make_shared<mia::AndroidInputChannel>();
}
