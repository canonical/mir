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
    Stream(geometry::Size sz, MirPixelFormat format);
    ~Stream();

    void submit_buffer(
        std::shared_ptr<graphics::Buffer> const& buffer,
        geometry::Size dest_size,
        geometry::generic::Rectangle<double> src_bounds) override;
    MirPixelFormat pixel_format() const override;
    void set_frame_posted_callback(
        std::function<void(geometry::Size const&)> const& callback) override;
    std::shared_ptr<graphics::Buffer>
        lock_compositor_buffer(void const* user_id) override;
    geometry::Size stream_size() override;
    bool has_submitted_buffer() const override;
    void set_scale(float scale) override;

private:
    std::shared_ptr<MultiMonitorArbiter> const arbiter;
    struct State
    {
        geometry::Size latest_buffer_size;
        float scale_{1.0f};
        MirPixelFormat pf;
    };
    Synchronised<State> latest_state;

    std::atomic<bool> first_frame_posted;


    Synchronised<std::function<void(geometry::Size const&)>> frame_callback;
};
}
}

#endif /* MIR_COMPOSITOR_STREAM_H_ */
