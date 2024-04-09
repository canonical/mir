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
#include "buffer_acquisition.h"
#include <memory>
#include <mutex>
#include <vector>
#include <optional>

namespace mir
{
namespace graphics { class Buffer; }
namespace compositor
{
class Schedule;

class MultiMonitorArbiter : public BufferAcquisition 
{
public:
    MultiMonitorArbiter();
    ~MultiMonitorArbiter();

    std::shared_ptr<graphics::Buffer> compositor_acquire(compositor::CompositorID id) override;
    bool buffer_ready_for(compositor::CompositorID id);

    void submit_buffer(std::shared_ptr<graphics::Buffer> buffer);

private:
    void add_current_buffer_user(compositor::CompositorID id);
    bool is_user_of_current_buffer(compositor::CompositorID id);
    void clear_current_users();

    std::mutex mutable mutex;
    std::shared_ptr<graphics::Buffer> current_buffer;
    std::shared_ptr<graphics::Buffer> next_buffer;
    std::vector<std::optional<compositor::CompositorID>> current_buffer_users;
};

}
}
#endif /* MIR_COMPOSITOR_MULTI_MONITOR_ARBITER_H_ */
