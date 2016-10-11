/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by:
 * Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_COMPOSITOR_BUFFER_ACQUISITION_H_
#define MIR_COMPOSITOR_BUFFER_ACQUISITION_H_

#include "mir/graphics/buffer_properties.h"
#include <memory>

namespace mir
{
namespace graphics { class Buffer; struct BufferProperties; }

namespace compositor
{

enum class MultiMonitorMode
{
    multi_monitor_sync, // lower latency+framerate, and supports multi-monitor
    single_monitor_fast // higher latency+framerate, no multi-monitor
};

class BufferAcquisition
{
public:
    /**
     * Acquire the next buffer that's ready to display/composite.
     *
     * \param [in] user_id A unique identifier of who is going to use the
     *                     buffer, to ensure that separate users representing
     *                     separate monitors who need the same frame will get
     *                     the same buffer. However consecutive calls for the
     *                     same user will get different buffers. To avoid
     *                     collisions, all callers should determine user_id
     *                     in the same way (e.g. always use "this" pointer).
     */
    virtual std::shared_ptr<graphics::Buffer>
        compositor_acquire(void const* user_id) = 0;
    virtual void compositor_release(std::shared_ptr<graphics::Buffer> const&) = 0;
    virtual std::shared_ptr<graphics::Buffer> snapshot_acquire() = 0;
    virtual void snapshot_release(std::shared_ptr<graphics::Buffer> const&) = 0;
    virtual void set_mode(MultiMonitorMode mode) = 0;
    virtual ~BufferAcquisition() = default;

protected:
    BufferAcquisition() = default;
    BufferAcquisition(BufferAcquisition const&) = delete;
    BufferAcquisition& operator=(BufferAcquisition const&) = delete;
};

}
}

#endif /*MIR_COMPOSITOR_BUFFER_ACQUISITION_H_*/
