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
 */

#ifndef MIR_COMPOSITOR_SCHEDULE_H_
#define MIR_COMPOSITOR_SCHEDULE_H_

#include <memory>

namespace mir
{
namespace graphics { class Buffer; }
namespace compositor
{

class Schedule
{
public:
    virtual void schedule(std::shared_ptr<graphics::Buffer> const& buffer) = 0;
    virtual void cancel(std::shared_ptr<graphics::Buffer> const& buffer) = 0;
    virtual bool anything_scheduled() = 0;
    virtual std::shared_ptr<graphics::Buffer> next_buffer() = 0;

    virtual ~Schedule() = default;
    Schedule() = default;
    Schedule(Schedule const&) = delete;
    Schedule& operator=(Schedule const&) = delete;
};

}
}
#endif /* MIR_COMPOSITOR_SCHEDULE_H_ */
