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

#ifndef MIR_INPUT_RECEIVER_RECEIVER_THREAD_H_
#define MIR_INPUT_RECEIVER_RECEIVER_THREAD_H_

namespace mir
{
namespace input
{
namespace receiver
{

class InputReceiverThread
{
public:
    virtual ~InputReceiverThread() {};

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void join() = 0;

protected:
    InputReceiverThread() = default;
    InputReceiverThread(const InputReceiverThread&) = delete;
    InputReceiverThread& operator=(const InputReceiverThread&) = delete;
};

}
}
} // namespace mir

#endif // MIR_INPUT_RECEIVER_RECEIVER_THREAD_H_
