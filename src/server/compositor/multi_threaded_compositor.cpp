/*
 * Copyright © 2013-2016 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "multi_threaded_compositor.h"
#include "mir/graphics/display.h"
#include "mir/graphics/display_buffer.h"
#include "mir/compositor/display_buffer_compositor.h"
#include "mir/compositor/display_buffer_compositor_factory.h"
#include "mir/compositor/display_listener.h"
#include "mir/compositor/scene.h"
#include "mir/compositor/compositor_report.h"
#include "mir/scene/legacy_scene_change_notification.h"
#include "mir/scene/surface_observer.h"
#include "mir/scene/surface.h"
#include "mir/terminate_with_current_exception.h"
#include "mir/raii.h"
#include "mir/unwind_helpers.h"
#include "mir/thread_name.h"

#include <thread>
#include <chrono>
#include <condition_variable>
#include <boost/throw_exception.hpp>

using namespace std::literals::chrono_literals;

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace ms = mir::scene;

namespace mir
{
namespace compositor
{

class CompositingFunctor
{
public:
    CompositingFunctor(
        std::shared_ptr<mc::DisplayBufferCompositorFactory> const& db_compositor_factory,
        mg::DisplaySyncGroup& group,
        std::shared_ptr<mc::Scene> const& scene,
        std::shared_ptr<DisplayListener> const& display_listener,
        std::chrono::milliseconds fixed_composite_delay,
        std::shared_ptr<CompositorReport> const& report) :
        compositor_factory{db_compositor_factory},
        group(group),
        scene(scene),
        running{true},
        frames_scheduled{0},
        force_sleep{fixed_composite_delay},
        display_listener{display_listener},
        report{report},
        started_future{started.get_future()}
    {
    }

    void operator()() noexcept  // noexcept is important! (LP: #1237332)
    try
    {
        mir::set_thread_name("Mir/Comp");

        std::vector<std::tuple<mg::DisplayBuffer*, std::unique_ptr<mc::DisplayBufferCompositor>>> compositors;
        group.for_each_display_buffer(
        [this, &compositors](mg::DisplayBuffer& buffer)
        {
            compositors.emplace_back(
                std::make_tuple(&buffer, compositor_factory->create_compositor_for(buffer)));

            auto const& r = buffer.view_area();
            auto const comp_id = std::get<1>(compositors.back()).get();
            report->added_display(r.size.width.as_int(), r.size.height.as_int(),
                                  r.top_left.x.as_int(), r.top_left.y.as_int(),
                                  CompositorReport::SubCompositorId{comp_id});
        });

        //Appease TSan, avoid destructor and this thread accessing the same shared_ptr instance
        auto const disp_listener = display_listener;
        auto display_registration = mir::raii::paired_calls(
            [this, &disp_listener]{group.for_each_display_buffer([&disp_listener](mg::DisplayBuffer& buffer)
                { disp_listener->add_display(buffer.view_area()); });},
            [this, &disp_listener]{group.for_each_display_buffer([&disp_listener](mg::DisplayBuffer& buffer)
                { disp_listener->remove_display(buffer.view_area()); });});

        auto compositor_registration = mir::raii::paired_calls(
            [this,&compositors]
            {
                for (auto& compositor : compositors)
                    scene->register_compositor(std::get<1>(compositor).get());
            },
            [this,&compositors]{
                for (auto& compositor : compositors)
                    scene->unregister_compositor(std::get<1>(compositor).get());
            });

        started.set_value();

        try
        {
            std::unique_lock<std::mutex> lock{run_mutex};
            while (running)
            {
                /* Wait until compositing has been scheduled or we are stopped */
                run_cv.wait(lock, [&]{ return (frames_scheduled > 0) || !running; });

                /*
                 * Check if we are running before compositing, since we may have
                 * been stopped while waiting for the run_cv above.
                 */
                if (running)
                {
                    /*
                     * Each surface could have a number of frames ready in its buffer
                     * queue. And we need to ensure that we render all of them so that
                     * none linger in the queue indefinitely (seen as input lag).
                     * frames_scheduled indicates the number of frames that are scheduled
                     * to ensure all surfaces' queues are fully drained.
                     */
                    frames_scheduled--;
                    not_posted_yet = false;
                    lock.unlock();

                    for (auto& tuple : compositors)
                    {
                        auto& buffer = std::get<0>(tuple);
                        auto& compositor = std::get<1>(tuple);
                        compositor->composite(scene->scene_elements_for(compositor.get(), buffer->view_area()));
                    }
                    group.post();

                    /*
                     * "Predictive bypass" optimization: If the last frame was
                     * bypassed/overlayed or you simply have a fast GPU, it is
                     * beneficial to sleep for most of the next frame. This reduces
                     * the latency between snapshotting the scene and post()
                     * completing by almost a whole frame.
                     */
                    auto delay = force_sleep >= std::chrono::milliseconds::zero() ?
                                 force_sleep : group.recommended_sleep();
                    std::this_thread::sleep_for(delay);

                    lock.lock();

                    /*
                     * Note the compositor may have chosen to ignore any number
                     * of renderables and not consumed buffers from them. So it's
                     * important to re-count number of frames pending, separately
                     * to the initial scene_elements_for()...
                     */
                    int pending = 0;
                    for (auto& compositor : compositors)
                    {
                        auto const comp_id = std::get<1>(compositor).get();
                        int pend = scene->frames_pending(comp_id);
                        if (pend > pending)
                            pending = pend;
                    }

                    if (pending > frames_scheduled)
                        frames_scheduled = pending;
                }
            }
        }
        catch (...)
        {
            mir::terminate_with_current_exception();
        }
    }
    catch(...)
    {
        started.set_exception(std::current_exception());

        //Move the promise so that the promise destructor occurs here rather than in the thread
        //destroying CompositingFunctor, mostly to appease TSan
        auto promise = std::move(started);
    }

    void schedule_compositing(int num_frames)
    {
        std::lock_guard<std::mutex> lock{run_mutex};

        if (num_frames > frames_scheduled)
        {
            frames_scheduled = num_frames;
            run_cv.notify_one();
        }
    }

    void schedule_compositing(int num_frames, geometry::Rectangle const& damage)
    {
        std::lock_guard<std::mutex> lock{run_mutex};
        bool took_damage = not_posted_yet;

        group.for_each_display_buffer([&](mg::DisplayBuffer& buffer)
            { if (damage.overlaps(buffer.view_area())) took_damage = true; });

        if (took_damage && num_frames > frames_scheduled)
        {
            frames_scheduled = num_frames;
            run_cv.notify_one();
        }
    }

    void stop()
    {
        std::lock_guard<std::mutex> lock{run_mutex};
        running = false;
        run_cv.notify_one();
    }

    void wait_until_started()
    {
        if (started_future.wait_for(10s) != std::future_status::ready)
            BOOST_THROW_EXCEPTION(std::runtime_error("Compositor thread failed to start"));

        started_future.get();
    }

private:
    std::shared_ptr<mc::DisplayBufferCompositorFactory> const compositor_factory;
    mg::DisplaySyncGroup& group;
    std::shared_ptr<mc::Scene> const scene;
    bool running;
    int frames_scheduled;
    std::chrono::milliseconds force_sleep{-1};
    std::mutex run_mutex;
    std::condition_variable run_cv;
    std::shared_ptr<DisplayListener> const display_listener;
    std::shared_ptr<CompositorReport> const report;
    std::promise<void> started;
    std::future<void> started_future;
    bool not_posted_yet = true;
};

}
}

mc::MultiThreadedCompositor::MultiThreadedCompositor(
    std::shared_ptr<mg::Display> const& display,
    std::shared_ptr<mc::Scene> const& scene,
    std::shared_ptr<DisplayBufferCompositorFactory> const& db_compositor_factory,
    std::shared_ptr<DisplayListener> const& display_listener,
    std::shared_ptr<CompositorReport> const& compositor_report,
    std::chrono::milliseconds fixed_composite_delay,
    bool compose_on_start)
    : display{display},
      scene{scene},
      display_buffer_compositor_factory{db_compositor_factory},
      display_listener{display_listener},
      report{compositor_report},
      state{CompositorState::stopped},
      fixed_composite_delay{fixed_composite_delay},
      compose_on_start{compose_on_start},
      thread_pool{1}
{
    observer = std::make_shared<ms::LegacySceneChangeNotification>(
    [this]()
    {
        schedule_compositing(1);
    },
    [this](int num, geometry::Rectangle const& damage)
    {
        schedule_compositing(num, damage);
    });
}

mc::MultiThreadedCompositor::~MultiThreadedCompositor()
{
    stop();
}

void mc::MultiThreadedCompositor::schedule_compositing(int num)
{
    report->scheduled();
    for (auto& f : thread_functors)
        f->schedule_compositing(num);
}

void mc::MultiThreadedCompositor::schedule_compositing(int num, geometry::Rectangle const& damage) const
{
    report->scheduled();
    for (auto& f : thread_functors)
        f->schedule_compositing(num, damage);
}

void mc::MultiThreadedCompositor::start()
{
    auto stopped = CompositorState::stopped;

    if (!state.compare_exchange_strong(stopped, CompositorState::starting))
        return;

    report->started();

    /* To cleanup state if any code below throws */
    auto cleanup_if_unwinding = on_unwind([this]
        {
            // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=62258
            // After using rethrow_exception() (and catching the exception),
            // all subsequent calls to uncaught_exception() return `true'.
            if (state == CompositorState::started) return;

            destroy_compositing_threads(); state = CompositorState::stopped;
        });

    create_compositing_threads();

    /* Add the observer after we have created the compositing threads */
    scene->add_observer(observer);

    /* Optional first render */
    if (compose_on_start)
        schedule_compositing(1);

    state = CompositorState::started;
}

void mc::MultiThreadedCompositor::stop()
{
    auto started = CompositorState::started;

    if (!state.compare_exchange_strong(started, CompositorState::stopping))
        return;

    /* To cleanup state if any code below throws */
    auto cleanup_if_unwinding = on_unwind([this]
        {
            // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=62258
            // After using rethrow_exception() (and catching the exception),
            // all subsequent calls to uncaught_exception() return `true'.
            if (state == CompositorState::stopped) return;

            state = CompositorState::started;
        });

    /* Remove the observer before destroying the compositing threads */
    scene->remove_observer(observer);

    destroy_compositing_threads();

    // If the compositor is restarted we've likely got clients blocked
    // so we will need to schedule compositing immediately
    compose_on_start = true;

    report->stopped();

    state = CompositorState::stopped;
}

void mc::MultiThreadedCompositor::create_compositing_threads()
{
    /* Start the display buffer compositing threads */
    display->for_each_display_sync_group([this](mg::DisplaySyncGroup& group)
    {
        auto thread_functor = std::make_unique<mc::CompositingFunctor>(
            display_buffer_compositor_factory, group, scene, display_listener,
            fixed_composite_delay, report);

        futures.push_back(thread_pool.run(std::ref(*thread_functor), &group));
        thread_functors.push_back(std::move(thread_functor));
    });

    thread_pool.shrink();

    for (auto& functor : thread_functors)
        functor->wait_until_started();
}

void mc::MultiThreadedCompositor::destroy_compositing_threads()
{
    for (auto& f : thread_functors)
        f->stop();

    for (auto& f : futures)
        f.wait();

    thread_functors.clear();
    futures.clear();
}
