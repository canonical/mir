/*
 * Copyright © 2013 Canonical Ltd.
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
 */

#ifndef MIR_INPUT_ANDROID_INPUT_RECEIVER_THREAD_H_
#define MIR_INPUT_ANDROID_INPUT_RECEIVER_THREAD_H_

#include "mir_toolkit/input/event.h"

#include <functional>
#include <memory>
#include <atomic>
#include <thread>

namespace mir_toolkit
{
class MirSurface;
}

namespace mir
{
namespace input
{
namespace android
{
namespace transport
{
class InputReceiver;

class InputReceiverThread
{
public:
    InputReceiverThread(std::shared_ptr<InputReceiver> const& receiver, 
                        std::function<void(mir_toolkit::MirSurface*, MirEvent*, void*)> event_handling_callback,
                        void* context);
    virtual ~InputReceiverThread() {};

    void start();
    void stop();
    void join();
protected:
    InputReceiverThread(const InputReceiverThread&) = delete;
    InputReceiverThread& operator=(const InputReceiverThread&) = delete;
private:
    std::shared_ptr<InputReceiver> const receiver;
    std::function<void(mir_toolkit::MirSurface*, MirEvent*, void*)> handler;
    
    void thread_loop();
    std::atomic<bool> running;
    std::thread thread;
};

}
}
}
} // namespace mir

#endif // MIR_INPUT_ANDROID_INPUT_RECEIVER_THREAD_H_
