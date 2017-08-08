/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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
 */

 #ifndef MIR_FRONTEND_SCREENCAST_BUFFER_TRACKER_H_
 #define MIR_FRONTEND_SCREENCAST_BUFFER_TRACKER_H_

#include "mir/frontend/screencast.h"

#include <functional>
#include <unordered_set>
#include <unordered_map>

namespace mir
{
namespace graphics
{
class Buffer;
}
namespace frontend
{

/// Responsible for tracking what buffers the client library knows about for a given screencast id.

class ScreencastBufferTracker
{
public:
    ScreencastBufferTracker() = default;
    /* Track a buffer associated with a screencast session
     * \param id     id that the the buffer is associated with
     * \param buffer buffer to be tracked
     * \returns      true if the buffer is already tracked
     *               false if the buffer is not tracked
     */
    bool track_buffer(ScreencastSessionId id, graphics::Buffer* buffer);

    /* removes all tracked buffers associated with id */
    void remove_session(ScreencastSessionId id);

    /* Iterates over all tracked session ids */
    void for_each_session(std::function<void(ScreencastSessionId)> f) const;


private:
    ScreencastBufferTracker(ScreencastBufferTracker const&) = delete;
    ScreencastBufferTracker& operator=(ScreencastBufferTracker const&) = delete;

    std::unordered_map<ScreencastSessionId, std::unordered_set<graphics::Buffer *>> tracker;
};

}
}

 #endif // MIR_FRONTEND_SCREENCAST_BUFFER_TRACKER_H_
