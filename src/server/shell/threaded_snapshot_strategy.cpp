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
 * Authored By: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "threaded_snapshot_strategy.h"
#include "pixel_buffer.h"
#include "mir/shell/surface_buffer_access.h"

#include <deque>
#include <mutex>
#include <condition_variable>

namespace geom = mir::geometry;
namespace msh = mir::shell;

namespace mir
{
namespace shell
{

struct WorkItem
{
    std::shared_ptr<SurfaceBufferAccess> const surface_buffer_access;
    SnapshotCallback const snapshot_taken;
};

class SnapshottingFunctor
{
public:
    SnapshottingFunctor(std::shared_ptr<PixelBuffer> const& pixels)
        : running{true}, pixels{pixels}
    {
    }

    void operator()()
    {
        std::unique_lock<std::mutex> lock{work_mutex};

        while (running)
        {
            while (running && work.empty())
                work_cv.wait(lock);

            if (running)
            {
                auto wi = work.front();
                work.pop_front();

                lock.unlock();

                take_snapshot(wi);

                lock.lock();
            }
        }
    }

    void take_snapshot(WorkItem const& wi)
    {
        wi.surface_buffer_access->with_most_recent_buffer_do(
            [this](graphics::Buffer& buffer)
            {
                pixels->fill_from(buffer);
            });


        wi.snapshot_taken(
            Snapshot{pixels->size(),
                     pixels->stride(),
                     pixels->as_argb_8888()});
    }

    void schedule_snapshot(WorkItem const& wi)
    {
        std::lock_guard<std::mutex> lg{work_mutex};
        work.push_back(wi);
        work_cv.notify_one();
    }

    void stop()
    {
        std::lock_guard<std::mutex> lg{work_mutex};
        running = false;
        work_cv.notify_one();
    }

private:
    bool running;
    std::shared_ptr<PixelBuffer> const pixels;
    std::mutex work_mutex;
    std::condition_variable work_cv;
    std::deque<WorkItem> work;
};

}
}

msh::ThreadedSnapshotStrategy::ThreadedSnapshotStrategy(
    std::shared_ptr<PixelBuffer> const& pixels)
    : pixels{pixels},
      functor{new SnapshottingFunctor{pixels}},
      thread{std::ref(*functor)}
{
}

msh::ThreadedSnapshotStrategy::~ThreadedSnapshotStrategy() noexcept
{
    functor->stop();
    thread.join();
}

void msh::ThreadedSnapshotStrategy::take_snapshot_of(
    std::shared_ptr<SurfaceBufferAccess> const& surface_buffer_access,
    SnapshotCallback const& snapshot_taken)
{
    functor->schedule_snapshot(WorkItem{surface_buffer_access, snapshot_taken});
}
