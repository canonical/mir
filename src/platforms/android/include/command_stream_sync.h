/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_COMMAND_STREAM_SYNC_H_
#define MIR_GRAPHICS_COMMAND_STREAM_SYNC_H_

#include <chrono>

namespace mir
{
namespace graphics
{
class CommandStreamSync
{
public:
    //insert a sync object into the GL command stream of the current context.
    // \warning the calling thread should have a current egl context and display
    virtual void raise() = 0;
    // remove fence without waiting.
    virtual void reset() = 0;
    //wait for fence.
    // \ param [in] ns  The amount of time to wait for the fence to become signalled
    // \ returns        true if the fence was signalled, false if timeout
    virtual bool wait_for(std::chrono::nanoseconds ns) = 0;

    virtual ~CommandStreamSync() = default;
    CommandStreamSync() = default; 
    CommandStreamSync(CommandStreamSync const&) = delete;
    CommandStreamSync& operator=(CommandStreamSync const&) = delete;
};


}
}

#endif /* MIR_GRAPHICS_COMMAND_STREAM_SYNC_H_ */
