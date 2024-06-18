/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_COMPOSITOR_STREAM_H_
#define MIR_COMPOSITOR_STREAM_H_

#include "mir/compositor/buffer_stream.h"
#include "mir/geometry/size.h"
#include "mir/geometry/rectangle.h"
#include "mir/synchronised.h"

#include <atomic>
#include <memory>
#include <atomic>

namespace mir
{
namespace compositor
{
class MultiMonitorArbiter;

class Stream : public BufferStream
{
public:
    Stream();
    ~Stream();

    void submit_buffer(
        std::shared_ptr<graphics::Buffer> const& buffer,
        geometry::Size dest_size,
        geometry::RectangleD src_bounds) override;
    void set_frame_posted_callback(
        std::function<void(geometry::Size const&)> const& callback) override;
    auto next_submission_for_compositor(void const* user_id) -> std::shared_ptr<Submission> override;
    bool has_submitted_buffer() const override;
private:
    std::shared_ptr<MultiMonitorArbiter> const arbiter;

    std::atomic<bool> first_frame_posted;

    Synchronised<std::function<void(geometry::Size const&)>> frame_callback;
};
}
}

#endif /* MIR_COMPOSITOR_STREAM_H_ */
