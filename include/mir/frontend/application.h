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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#ifndef MIR_FRONTEND_APPLICATION_H_
#define MIR_FRONTEND_APPLICATION_H_

#include "mir/input/event_handler.h"

#include <boost/signals2.hpp>

#include <cassert>
#include <memory>

namespace mir
{
namespace frontend
{

namespace mi = mir::input;

class Communicator;

class Application : public mi::EventHandler
{
 public:

    enum class State
    {
        disconnected,
        connected
    };

    typedef boost::signals2::signal<
        void(State old_state, State new_state)
        > StateTransitionSignal;

    Application(std::shared_ptr<Communicator> communicator)
            : communicator(communicator)
    {
        assert(communicator);
    }

    virtual ~Application() {}

    // From mi::EventHandler
    void on_event(mi::Event* e)
    {
        assert(e);
    }

    StateTransitionSignal& state_transition_signal()
    {
        return state_transition;
    }

    void connect()
    {
    }

    void disconnect()
    {
    }

 protected:
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

 private:
    // ToDo: Hide away implementation details.
    std::shared_ptr<Communicator> communicator;
    StateTransitionSignal state_transition;
};

}
}

#endif // MIR_FRONTEND_APPLICATION_H_
