/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_INPUT_ANDROID_INPUT_DISPATCHER_H_
#define MIR_INPUT_ANDROID_INPUT_DISPATCHER_H_

#include "mir/input/input_dispatcher.h"

#include <memory>

namespace android
{
class InputDispatcherInterface;
}
namespace droidinput = android;

namespace mir
{
namespace input
{
namespace android
{
class InputThread;

/*!
 * \brief Android based input dispatcher
 */
class AndroidInputDispatcher : public mir::input::InputDispatcher
{
public:
    AndroidInputDispatcher(std::shared_ptr<droidinput::InputDispatcherInterface> const& dispatcher,
                           std::shared_ptr<InputThread> const& thread);
    ~AndroidInputDispatcher();
    void configuration_changed(nsecs_t when) override;
    void device_reset(int32_t device_id, nsecs_t when) override;
    void dispatch(MirEvent const& event) override;
    void start() override;
    void stop() override;
private:
    std::shared_ptr<droidinput::InputDispatcherInterface> const dispatcher;
    std::shared_ptr<InputThread> const dispatcher_thread;

};

}
}
}

#endif
