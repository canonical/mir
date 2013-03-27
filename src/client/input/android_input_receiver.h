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

#ifndef MIR_INPUT_ANDROID_INPUT_RECEIVER_H_
#define MIR_INPUT_ANDROID_INPUT_RECEIVER_H_

#include "mir_toolkit/input/event.h"

#include <utils/StrongPointer.h>
#include <androidfw/Input.h>

#include <memory>
#include <chrono>

namespace droidinput = android;

namespace android
{
class InputChannel;
class InputConsumer;
class Looper;
}

namespace mir
{
namespace client
{
namespace input
{
namespace android
{

/// Synchronously receives input events in a blocking manner
class InputReceiver
{
public:
    InputReceiver(droidinput::sp<droidinput::InputChannel> const& input_channel);
    InputReceiver(int fd);

    virtual ~InputReceiver();
    int get_fd() const;
    
    /// Synchronously receive an event with millisecond timeout. A negative timeout value is used to indicate
    /// indefinite polling.
    virtual bool next_event(std::chrono::milliseconds const& timeout, MirEvent &ev);
    virtual bool next_event(MirEvent &ev) { return next_event(std::chrono::milliseconds(-1), ev); }
    
    /// May be used from any thread to wake an InputReceiver blocked in next_event
    virtual void wake();

protected:
    InputReceiver(const InputReceiver&) = delete;
    InputReceiver& operator=(const InputReceiver&) = delete;

private:
    droidinput::sp<droidinput::InputChannel> input_channel;
    std::shared_ptr<droidinput::InputConsumer> input_consumer;
    droidinput::PooledInputEventFactory event_factory;
    droidinput::sp<droidinput::Looper> looper;

    bool fd_added;
    
    static const bool consume_batches = true;
    static const bool do_not_consume_batches = false;
    static const int default_frame_time = -1;
    static const bool handle_event = true;
    static const bool do_not_handle_event = false;
};

}
}
}
} // namespace mir

#endif // MIR_INPUT_ANDROID_INPUT_RECEIVER_H_
