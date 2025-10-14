/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <mir/fd.h>
#include <mir/time/posix_clock.h>

namespace mir::graphics::drm
{

class Syncobj
{
public:
    Syncobj(Fd drm_fd, uint32_t handle);

    ~Syncobj();

    auto wait(uint64_t point, time::PosixClock<CLOCK_MONOTONIC>::time_point until) const -> bool;

    void signal(uint64_t point);

    auto to_eventfd(uint64_t point) const -> Fd;
private:
    Fd const drm_fd;
    uint32_t const handle;
};
}
