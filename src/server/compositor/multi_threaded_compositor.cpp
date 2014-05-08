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
#include "mir/scene/legacy_scene_change_notification.h"
#include "mir/scene/surface_observer.h"
#include "mir/scene/surface.h"
#include "mir/run_mir.h"

#include <thread>
#include <condition_variable>
#include <cstdint>
#include <chrono>

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace ms = mir::scene;

namespace
{

class ApplyIfUnwinding
{
public:
    ApplyIfUnwinding(std::function<void()> const& apply)
        : apply{apply}
    {
    }

    ~ApplyIfUnwinding()
    {
        if (std::uncaught_exception())
            apply();
    }

private:
    ApplyIfUnwinding(ApplyIfUnwinding const&) = delete;
    ApplyIfUnwinding& operator=(ApplyIfUnwinding const&) = delete;

    std::function<void()> const apply;
};

}

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
    CompositingFunctor() : running{true}, frames_scheduled{false} {}
    virtual ~CompositingFunctor() = default;

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

protected:
    void run_compositing_loop(std::function<bool()> const& composite)
    {
        std::unique_lock<std::mutex> lock{run_mutex};

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

                auto more_frames_pending = composite();

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

private:
    bool running;
    bool frames_scheduled;
    std::mutex run_mutex;
    std::condition_variable run_cv;
};

class DisplayBufferCompositingFunctor : public CompositingFunctor
{
public:
    DisplayBufferCompositingFunctor(
        std::shared_ptr<mc::DisplayBufferCompositorFactory> const& db_compositor_factory,
        mg::DisplayBuffer& buffer,
        std::shared_ptr<mc::CompositorReport> const& report)
        : display_buffer_compositor_factory{db_compositor_factory},
          buffer(buffer),
          report{report}
    {
    }

    void operator()() noexcept // noexcept is important! (LP: #1237332)
    try
    {
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

        run_compositing_loop([&] { return display_buffer_compositor->composite();});
    }
    catch (...)
    {
        mir::terminate_with_current_exception();
    }

private:
    std::shared_ptr<mc::DisplayBufferCompositorFactory> const display_buffer_compositor_factory;
    mg::DisplayBuffer& buffer;
    std::shared_ptr<CompositorReport> const report;
};

class BufferConsumingFunctor : public CompositingFunctor
{
public:
    BufferConsumingFunctor(std::shared_ptr<mc::Scene> const& scene)
        : scene{scene}
    {
    }

    void operator()() noexcept // noexcept is important! (LP: #1237332)
    try
    {
        run_compositing_loop(
            [this]
            {
                std::vector<std::shared_ptr<mg::Buffer>> saved_resources;

                auto const& renderables = scene->renderable_list_for(this);

                for (auto const& r : renderables)
                {
                    if (r->buffers_ready_for_compositor() > 0)
                        saved_resources.push_back(r->buffer());
                }

                wait_until_next_fake_vsync();

                return false;
            });
    }
    catch (...)
    {
        mir::terminate_with_current_exception();
    }

private:
    void wait_until_next_fake_vsync()
    {
        using namespace std::chrono;
        typedef duration<int64_t, std::ratio<1, 60>> vsync_periods;

        /* Truncate to vsync periods */
        auto const previous_vsync =
            duration_cast<vsync_periods>(steady_clock::now().time_since_epoch());
        /* Convert back to a timepoint */
        auto const previous_vsync_tp =
            time_point<steady_clock, vsync_periods>{previous_vsync};
        /* Next vsync time point */
        auto const next_vsync = previous_vsync_tp + vsync_periods(1);

        std::this_thread::sleep_until(next_vsync);
    }

    std::shared_ptr<mc::Scene> const scene;
};

}
}

mc::MultiThreadedCompositor::MultiThreadedCompositor(
    std::shared_ptr<mg::Display> const& display,
    std::shared_ptr<mc::Scene> const& scene,
    std::shared_ptr<DisplayBufferCompositorFactory> const& db_compositor_factory,
    std::shared_ptr<CompositorReport> const& compositor_report,
    bool compose_on_start)
    : display{display},
      scene{scene},
      display_buffer_compositor_factory{db_compositor_factory},
      report{compositor_report},
      state{CompositorState::stopped},
      compose_on_start{compose_on_start}
{
    observer = std::make_shared<ms::LegacySceneChangeNotification>([this]()
        {
            /* Recomposite whenever the scene changes */
            schedule_compositing();
        });
}

mc::MultiThreadedCompositor::~MultiThreadedCompositor()
{
    stop();
}

void mc::MultiThreadedCompositor::schedule_compositing()
{
    std::unique_lock<std::mutex> lk(state_guard);

    report->scheduled();
    for (auto& f : thread_functors)
        f->schedule_compositing();
}

void mc::MultiThreadedCompositor::start()
{
    std::unique_lock<std::mutex> lk(state_guard);
    if (state != CompositorState::stopped)
    {
        return;
    }

    state = CompositorState::starting;
    report->started();

    /* To cleanup state if any code below throws */
    ApplyIfUnwinding cleanup_if_unwinding{
        [this, &lk]{
            destroy_compositing_threads(lk);
        }};

    lk.unlock();
    scene->add_observer(observer);
    lk.lock();

    create_compositing_threads();

    /* Optional first render */
    if (compose_on_start)
    {
        lk.unlock();
        schedule_compositing();
    }
}

void mc::MultiThreadedCompositor::stop()
{
    std::unique_lock<std::mutex> lk(state_guard);
    if (state != CompositorState::started)
    {
        return;
    }

    state = CompositorState::stopping;

    /* To cleanup state if any code below throws */
    ApplyIfUnwinding cleanup_if_unwinding{
        [this, &lk]{
            if(!lk.owns_lock()) lk.lock();
            state = CompositorState::started;
        }};

    lk.unlock();
    scene->remove_observer(observer);
    lk.lock();

    destroy_compositing_threads(lk);

    // If the compositor is restarted we've likely got clients blocked
    // so we will need to schedule compositing immediately
    compose_on_start = true;
}

void mc::MultiThreadedCompositor::create_compositing_threads()
{
    /* Start the display buffer compositing threads */
    display->for_each_display_buffer([this](mg::DisplayBuffer& buffer)
    {
        auto thread_functor_raw =
            new mc::DisplayBufferCompositingFunctor{
                display_buffer_compositor_factory, buffer, report};
        auto thread_functor =
            std::unique_ptr<mc::DisplayBufferCompositingFunctor>(thread_functor_raw);

        threads.push_back(std::thread{std::ref(*thread_functor)});
        thread_functors.push_back(std::move(thread_functor));
    });

    /* Start the buffer consuming thread */
    auto thread_functor_raw = new mc::BufferConsumingFunctor{scene};
    auto thread_functor = std::unique_ptr<mc::BufferConsumingFunctor>(thread_functor_raw);

    threads.push_back(std::thread{std::ref(*thread_functor)});
    thread_functors.push_back(std::move(thread_functor));

    state = CompositorState::started;
}

void mc::MultiThreadedCompositor::destroy_compositing_threads(std::unique_lock<std::mutex>& lock)
{
    /* Could be called during unwinding,
     * ensure the lock is held before changing state
     */
    if(!lock.owns_lock())
        lock.lock();

    for (auto& f : thread_functors)
        f->stop();

    for (auto& t : threads)
        t.join();

    thread_functors.clear();
    threads.clear();

    report->stopped();

    state = CompositorState::stopped;
}
