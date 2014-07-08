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

#ifndef MIR_INPUT_INPUT_SEND_OBSERVER_H_
#define MIR_INPUT_INPUT_SEND_OBSERVER_H_

#include <mir_toolkit/event.h>

#include <memory>

namespace mir
{
namespace input
{
class Surface;

class InputSendObserver
{
public:
    InputSendObserver() = default;
    virtual ~InputSendObserver() = default;

    enum FailureReason
    {
        surface_disappeared,
        no_response_received,
        socket_error
    };
    /*!
     * \brief An attempt to send an input event to a destination failed.
     *
     * Reasons for failure could be the surface disappearing from the scene, before the response
     * was received. Or the client not responding in time.
     */
    virtual void send_failed(MirEvent const& event, input::Surface* surface, FailureReason reason) = 0;

    enum InputResponse
    {
        consumed,
        not_consumed
    };
    /*!
     * \brief Client responded to an input event.
     */
    virtual void send_suceeded(MirEvent const& event, input::Surface* surface, InputResponse response) = 0;

    /*!
     * \brief Called when client is temporary blocked because input events are still in
     * the queue.
     */
    virtual void client_blocked(MirEvent const& event, input::Surface* client) = 0;

protected:
    InputSendObserver& operator=(InputSendObserver const&) = delete;
    InputSendObserver(InputSendObserver const&) = delete;
};

}
}

#endif
