/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_INPUT_ANDROID_INPUT_THREAD_H_
#define MIR_INPUT_ANDROID_INPUT_THREAD_H_

namespace mir
{
namespace input
{
namespace android
{
class InputThread
{
public:
    virtual ~InputThread() {}

    virtual void start() = 0;
    virtual void request_stop() = 0;
    virtual void join() = 0;

protected:
    InputThread() {};
    InputThread(const InputThread&) = delete;
    InputThread& operator=(const InputThread&) = delete;
};

}
}
} // namespace mir

#endif // MIR_INPUT_ANDROID_INPUT_THREAD_H_
