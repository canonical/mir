/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#ifndef MIR_SIGNAL_DISPATCHER_H_
#define MIR_SIGNAL_DISPATCHER_H_

#include <boost/signals2.hpp>

#include <memory>

namespace mir
{

// Singleton class that decouples
// signal handling from reacting to
// signals to ensure that the actual signal
// handler only calls async signal-safe functions
// according to man 7 signal.
class SignalDispatcher
{
public:
    struct Constructor;

    typedef boost::signals2::signal<void(int)> SignalType;

    static std::shared_ptr<SignalDispatcher> instance();

    ~SignalDispatcher();

    SignalType& signal_channel();

    void enable_for(int signal);

private:
    friend struct Constructor;

    SignalDispatcher();
    SignalDispatcher(const SignalDispatcher&) = delete;
    SignalDispatcher& operator=(const SignalDispatcher&) = delete;

    struct Private;
    std::unique_ptr<Private> p;
};
}

#endif // MIR_SIGNAL_DISPATCHER_H_
