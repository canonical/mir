/*
 * Copyright © 2013-2014 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "multi_threaded_compositor.h"
#include "mir/graphics/display.h"
#include "mir/graphics/display_buffer.h"
#include "mir/compositor/display_buffer_compositor.h"
#include "mir/compositor/display_buffer_compositor_factory.h"
#include "mir/compositor/scene.h"
#include "mir/compositor/compositor_report.h"

#include <thread>
#include <condition_variable>

namespace mc = mir::compositor;
namespace mg = mir::graphics;

namespace mir
{
namespace compositor
{

class CurrentRenderingTarget
{
public:
    CurrentRenderingTarget(mg::DisplayBuffer& buffer)
        : buffer(buffer)
    {
        buffer.make_current();
    }

    ~CurrentRenderingTarget()
    {
        buffer.release_current();
    }

private:
    mg::DisplayBuffer& buffer;
};

class CompositingFunctor
{
public:
    CompositingFunctor(std::shared_ptr<mc::DisplayBufferCompositorFactory> const& db_compositor_factory,
                       mg::DisplayBuffer& buffer,
                       std::shared_ptr<CompositorReport> const& report)
        : display_buffer_compositor_factory{db_compositor_factory},
          buffer(buffer),
          running{true},
          frames_scheduled{0},
          report{report}
    {
    }

    void operator()() noexcept  // noexcept is important! (LP: #1237332)
    {
        std::unique_lock<std::mutex> lock{run_mutex};

        /*
         * Make the buffer the current rendering target, and release
         * it when the thread is finished.
         */
        CurrentRenderingTarget target{buffer};

        auto display_buffer_compositor = display_buffer_compositor_factory->create_compositor_for(buffer);

        CompositorReport::SubCompositorId report_id =
            display_buffer_compositor.get();

        const auto& r = buffer.view_area();
        report->added_display(r.size.width.as_int(), r.size.height.as_int(),
                              r.top_left.x.as_int(), r.top_left.y.as_int(),
                              report_id);

        while (running)
        {
            /* Wait until compositing has been scheduled or we are stopped */
            run_cv.wait(lock, [&]{ return frames_scheduled || !running; });

            /*
             * Check if we are running before compositing, since we may have
             * been stopped while waiting for the run_cv above.
             */
            if (running)
            {
                frames_scheduled = false;
                lock.unlock();
                auto more_frames_pending = display_buffer_compositor->composite();

                /*
                 * Each surface could have a number of frames ready in its buffer
                 * queue. And we need to ensure that we render all of them so that
                 * none linger in the queue indefinitely (seen as input lag).
                 * more_frames_pending indicates that there are we need to schedule
                 * more frames to ensure all surfaces' queues are fully drained.
                 */
                lock.lock();
                frames_scheduled |= more_frames_pending;
            }
        }
    }

    void schedule_compositing()
    {
        std::lock_guard<std::mutex> lock{run_mutex};

        frames_scheduled = true;
        run_cv.notify_one();
    }

    void stop()
    {
        std::lock_guard<std::mutex> lock{run_mutex};
        running = false;
        run_cv.notify_one();
    }

private:
    std::shared_ptr<mc::DisplayBufferCompositorFactory> const display_buffer_compositor_factory;
    mg::DisplayBuffer& buffer;
    bool running;
    bool frames_scheduled;
    std::mutex run_mutex;
    std::condition_variable run_cv;
    std::shared_ptr<CompositorReport> const report;
};

}
}

mc::MultiThreadedCompositor::MultiThreadedCompositor(
    std::shared_ptr<mg::Display> const& display,
    std::shared_ptr<mc::Scene> const& scene,
    std::shared_ptr<DisplayBufferCompositorFactory> const& db_compositor_factory,
    std::shared_ptr<CompositorReport> const& compositor_report)
    : display{display},
      scene{scene},
      display_buffer_compositor_factory{db_compositor_factory},
      report{compositor_report},
      started{false}
{
}

mc::MultiThreadedCompositor::~MultiThreadedCompositor()
{
    stop();
}

void mc::MultiThreadedCompositor::start()
{
    std::unique_lock<std::mutex> lk(started_guard);
    if (started)
    {
        return;
    }

    report->started();

    /* Start the compositing threads */
    display->for_each_display_buffer([this](mg::DisplayBuffer& buffer)
    {
        auto thread_functor_raw = new mc::CompositingFunctor{display_buffer_compositor_factory, buffer, report};
        auto thread_functor = std::unique_ptr<mc::CompositingFunctor>(thread_functor_raw);

        threads.push_back(std::thread{std::ref(*thread_functor)});
        thread_functors.push_back(std::move(thread_functor));
    });

    /* Recomposite whenever the scene changes */
    scene->set_change_callback([this]()
    {
        report->scheduled();
        for (auto& f : thread_functors)
            f->schedule_compositing();
    });

    /* First render */
    for (auto& f : thread_functors)
        f->schedule_compositing();

    started = true;
}

void mc::MultiThreadedCompositor::stop()
{
    std::unique_lock<std::mutex> lk(started_guard);
    if (!started)
    {
        return;
    }

    scene->set_change_callback([]{});

    for (auto& f : thread_functors)
        f->stop();

    for (auto& t : threads)
        t.join();

    thread_functors.clear();
    threads.clear();

    report->stopped();

    started = false;
}
