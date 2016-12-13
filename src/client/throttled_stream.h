/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#ifndef MIR_CLIENT_THROTTLED_STREAM_H_
#define MIR_CLIENT_THROTTLED_STREAM_H_

#include "mir/mir_buffer_stream.h"
#include "./mir_surface.h"
#include "./frame_clock.h"
#include <unordered_set>

namespace mir { namespace client {

class ThrottledStream : public MirBufferStream
{
public:
    ThrottledStream();
    virtual ~ThrottledStream();
    virtual int swap_interval() const override;
    virtual MirWaitHandle* set_swap_interval(int interval) override;
    virtual void adopted_by(MirSurface*) override;
    virtual void unadopted_by(MirSurface*) override;
protected:
    void wait_for_vsync();
    void swap_buffers_sync();
private:
    int interval;
    std::unordered_set<MirSurface*> users;
    std::shared_ptr<FrameClock> frame_clock;
    mir::time::PosixTimestamp last_vsync;
};

} } // mir::client

#endif // MIR_CLIENT_THROTTLED_STREAM_H_
