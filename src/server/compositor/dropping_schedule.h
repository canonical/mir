/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_COMPOSITOR_DROPPING_SCHEDULE_H_
#define MIR_COMPOSITOR_DROPPING_SCHEDULE_H_
#include "schedule.h"
#include <memory>
#include <mutex>

namespace mir
{
namespace graphics { class Buffer; }
namespace compositor
{
class DroppingSchedule : public Schedule
{
public:
    DroppingSchedule();
    void schedule(std::shared_ptr<graphics::Buffer> const& buffer) override;
    unsigned int num_scheduled() override;
    std::shared_ptr<graphics::Buffer> next_buffer() override;

private:
    std::mutex mutable mutex;
    std::shared_ptr<graphics::Buffer> the_only_buffer;
};
}
}
#endif /* MIR_COMPOSITOR_DROPPING_SCHEDULE_H_ */
