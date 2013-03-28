/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/compositor/multi_threaded_compositor.h"
#include "mir/graphics/display.h"
#include "mir/graphics/display_buffer.h"
#include "mir/compositor/compositing_strategy.h"
#include "mir/compositor/renderables.h"

#include <thread>
#include <condition_variable>

namespace mc = mir::compositor;
namespace mg = mir::graphics;

namespace mir
{
namespace compositor
{

class CompositingFunctor
{
public:
    CompositingFunctor(std::shared_ptr<mc::CompositingStrategy> const& strategy,
                       mg::DisplayBuffer& buffer)
        : compositing_strategy{strategy},
          buffer(buffer),
          running{true},
          compositing_scheduled{false}
    {
    }

    void operator()()
    {
        std::unique_lock<std::mutex> lock{run_mutex};

        while (running)
        {
            /* Wait until compositing has been scheduled or we are stopped */
            while (!compositing_scheduled && running)
                run_cv.wait(lock);

            compositing_scheduled = false;

            /*
             * Check if we are running before rendering, since we may have
             * been stopped while waiting for the run_cv above.
             */
            if (running)
            {
                lock.unlock();
                compositing_strategy->render(buffer);
                lock.lock();
            }
        }
    }

    void schedule_compositing()
    {
        std::lock_guard<std::mutex> lock{run_mutex};
        if (!compositing_scheduled)
        {
            compositing_scheduled = true;
            run_cv.notify_one();
        }
    }

    void stop()
    {
        std::lock_guard<std::mutex> lock{run_mutex};
        running = false;
        run_cv.notify_one();
    }

private:
    std::shared_ptr<mc::CompositingStrategy> const compositing_strategy;
    mg::DisplayBuffer& buffer;
    bool running;
    bool compositing_scheduled;
    std::mutex run_mutex;
    std::condition_variable run_cv;
};

}
}

mc::MultiThreadedCompositor::MultiThreadedCompositor(
    std::shared_ptr<mg::Display> const& display,
    std::shared_ptr<mc::Renderables> const& renderables,
    std::shared_ptr<mc::CompositingStrategy> const& strategy)
    : display{display},
      renderables{renderables},
      compositing_strategy{strategy}
{
}

mc::MultiThreadedCompositor::~MultiThreadedCompositor()
{
    stop();
}

void mc::MultiThreadedCompositor::start()
{
    /* Start the compositing threads */
    display->for_each_display_buffer([this](mg::DisplayBuffer& buffer)
    {
        auto thread_functor_raw = new mc::CompositingFunctor{compositing_strategy, buffer};
        auto thread_functor = std::unique_ptr<mc::CompositingFunctor>(thread_functor_raw);

        threads.push_back(std::thread{std::ref(*thread_functor)});
        thread_functors.push_back(std::move(thread_functor));
    });

    /* Recomposite whenever the renderables change */
    renderables->set_change_callback([this]()
    {
        for (auto& f : thread_functors)
            f->schedule_compositing();
    });

    /* First render */
    for (auto& f : thread_functors)
        f->schedule_compositing();
}

void mc::MultiThreadedCompositor::stop()
{
    renderables->set_change_callback([]{});

    for (auto& f : thread_functors)
        f->stop();

    for (auto& t : threads)
        t.join();

    thread_functors.clear();
    threads.clear();
}
