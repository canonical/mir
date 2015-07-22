/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_COMPOSITOR_QUEUEING_SCHEDULE_H_
#define MIR_COMPOSITOR_QUEUEING_SCHEDULE_H_
#include <memory>
#include <deque>
#include <mutex>

namespace mir
{
namespace graphics { class Buffer; }
namespace compositor
{
class QueueingSchedule
{
public:
    void schedule(std::shared_ptr<graphics::Buffer> const& buffer);
    bool anything_scheduled();
    std::shared_ptr<graphics::Buffer> next_buffer();

private:
    std::mutex mutable mutex;
    std::deque<std::shared_ptr<graphics::Buffer>> queue;
};
}
}
#endif /* MIR_COMPOSITOR_QUEUEING_SCHEDULE_H_ */
