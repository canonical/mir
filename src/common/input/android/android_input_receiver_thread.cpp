/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
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

#define MIR_INCLUDE_DEPRECATED_EVENT_HEADER

#include "android_input_receiver_thread.h"
#include "android_input_receiver.h"

#include <thread>

namespace mircva = mir::input::receiver::android;

mircva::InputReceiverThread::InputReceiverThread(std::shared_ptr<mircva::InputReceiver> const& receiver,
                                                std::function<void(MirEvent*)> const& event_handling_callback)
  : receiver(receiver),
    handler(event_handling_callback),
    running(false)
{
}

mircva::InputReceiverThread::~InputReceiverThread()
{
    if (running)
        stop();
    if (thread.joinable())
        join();
}

void mircva::InputReceiverThread::start()
{
    running = true;
    thread = std::thread(std::mem_fn(&mircva::InputReceiverThread::thread_loop), this);
}

void mircva::InputReceiverThread::stop()
{
    running = false;
    receiver->wake();
}

void mircva::InputReceiverThread::join()
{
    thread.join();
}

void mircva::InputReceiverThread::thread_loop()
{
    while (running)
    {
        MirEvent ev;
        while(running && receiver->next_event(ev))
            handler(&ev);
        std::this_thread::yield(); // yield() is needed to ensure reasonable runtime under valgrind
    }
}
