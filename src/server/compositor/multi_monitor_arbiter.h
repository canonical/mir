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
 *
 */

#ifndef MIR_COMPOSITOR_MULTI_MONITOR_ARBITER_H_
#define MIR_COMPOSITOR_MULTI_MONITOR_ARBITER_H_

#include "mir/compositor/compositor_id.h"
#include "mir/compositor/buffer_stream.h"
#include "mir/geometry/forward.h"
#include "mir/synchronised.h"
#include <memory>
#include <vector>
#include <optional>

namespace mir
{
namespace graphics { class Buffer; }
namespace compositor
{

/**
 * Coordinate surface buffer consumption across different outputs
 *
 * When multiple compositors have a view on the same surface, they will
 * request buffers from that surface at their own pace, gated by either
 * the display VSync (for compositors driving physical outputs), by
 * client consumption (for compositors for screencopy), or some other
 * mechanism.
 *
 * These timings will (almost always) not align, so if each compositor's
 * request advanced the surface buffer we would request frames at some
 * multiple of the frequencies involved.
 *
 * As an additional constraint, it must always be possible for a compositor
 * to acquire a buffer.
 *
 * To avoid this, the MultiMonitorArbiter stores up to two buffers -
 * a current buffer and a next buffer - and tracks which compositor has
 * seen the current buffer. The next buffer is only advanced when a
 * compositor requests a buffer *and* that compositor has seen the current buffer.
 *
 * This results in the buffer queue being advanced at only the rate of the fastest
 * compositor.
 *
 * Unfortunately, because this doesn't have any feedback into any other
 * component, we can get into a state where there *is* a new buffer available
 * for a surface, but the *Compositor* isn't going to request a buffer until
 * something else changes. This is most easy to trigger around mode switches,
 * where we destroy all Compositors and then create new ones which then,
 * definitionally, haven't seen the current buffer.
 *
 * This system will need to be significantly overhauled in future. A variety
 * of Wayland extensions we wish to support - notably wp_presentation and
 * wp_fifo - expect that a surface's buffer queue is synchronised to a single
 * output.
 */
class MultiMonitorArbiter : public std::enable_shared_from_this<MultiMonitorArbiter>
{
public:
    MultiMonitorArbiter();
    ~MultiMonitorArbiter();

    auto compositor_acquire(compositor::CompositorID id) -> std::shared_ptr<BufferStream::Submission>;
    bool buffer_ready_for(compositor::CompositorID id);

    void submit_buffer(
        std::shared_ptr<graphics::Buffer> buffer,
        geometry::Size output_size,
        geometry::RectangleD source_sample);

    struct Submission;
private:
    struct State
    {
        std::vector<std::optional<compositor::CompositorID>> current_buffer_users;
        std::shared_ptr<Submission> current_submission;
        std::shared_ptr<Submission> next_submission;
    };
    Synchronised<State> state;

    static void add_current_buffer_user(State& state, compositor::CompositorID id);
    static bool is_user_of_current_buffer(State& state, compositor::CompositorID id);
    static void clear_current_users(State& state);
};

}
}
#endif /* MIR_COMPOSITOR_MULTI_MONITOR_ARBITER_H_ */
