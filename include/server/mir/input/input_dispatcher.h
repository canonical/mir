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

#ifndef MIR_INPUT_INPUT_DISPATCHER_H
#define MIR_INPUT_INPUT_DISPATCHER_H

#include "mir_toolkit/event.h"

namespace mir
{
namespace input
{

/*!
 * \brief The InputDispatchers role is to decide what should happen with user input events.
 *
 * It will receive MirEvents with either MirMotionEvent or MirKeyEvent inside. The InputDispatcher
 * implementation shall either handle the input without informing any clients or pick a client
 * surface and send the event to it.
 */
class InputDispatcher
{
public:
    /*!
     * \brief Called when the device configuration changed.
     */
    virtual void configuration_changed(nsecs_t when) = 0;
    /*!
     * \brief Called when the device \a device_id was added removed or was reset
     */
    virtual void device_reset(int32_t device_id, nsecs_t when) = 0;
    virtual void dispatch(MirEvent const& event) = 0;
    virtual void start() = 0;
    virtual void stop() = 0;

    virtual ~InputDispatcher() = default;
};

}
}

#endif

