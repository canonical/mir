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
 */

#include "multi_threaded_compositor.h"
#include "mir/compositor/scene_element.h"
#include "mir/graphics/display.h"
#include "mir/graphics/display_sink.h"
#include "mir/compositor/display_buffer_compositor.h"
#include "mir/compositor/display_buffer_compositor_factory.h"
#include "mir/compositor/display_listener.h"
#include "mir/compositor/scene.h"
#include "mir/compositor/compositor_report.h"
#include "mir/scene/scene_change_notification.h"
#include "mir/terminate_with_current_exception.h"
#include "mir/raii.h"
#include "mir/unwind_helpers.h"
#include "mir/thread_name.h"
#include "mir/executor.h"
#include "mir/signal.h"

#include <atomic>
#include <thread>
#include <chrono>
#include <future>
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
        force_sleep{fixed_composite_delay},
        display_listener{display_listener},
        report{report},
        started_future{started.get_future()},
        stopped_future{stopped.get_future()}
    {
    }

    void operator()() noexcept  // noexcept is important! (LP: #1237332)
    try
    {
        mir::set_thread_name("Mir/Comp");
        auto const signal_when_stopped = mir::raii::paired_calls(
            [](){},
            [this]()
            {
                stopped.set_value();
            });

        std::vector<std::tuple<mg::DisplaySink*, std::unique_ptr<mc::DisplayBufferCompositor>>> compositors;
        group.for_each_display_sink(
        [this, &compositors](mg::DisplaySink& sink)
        {
            compositors.emplace_back(
                std::make_tuple(&sink, compositor_factory->create_compositor_for(sink)));

            auto const& r = sink.view_area();
            auto const comp_id = std::get<1>(compositors.back()).get();
            report->added_display(r.size.width.as_int(), r.size.height.as_int(),
                                  r.top_left.x.as_int(), r.top_left.y.as_int(),
                                  CompositorReport::SubCompositorId{comp_id});
        });

        //Appease TSan, avoid destructor and this thread accessing the same shared_ptr instance
        auto const disp_listener = display_listener;
        auto display_registration = mir::raii::paired_calls(
            [this, &disp_listener]{group.for_each_display_sink([&disp_listener](mg::DisplaySink& sink)
                { disp_listener->add_display(sink.view_area()); });},
            [this, &disp_listener]{group.for_each_display_sink([&disp_listener](mg::DisplaySink& sink)
                { disp_listener->remove_display(sink.view_area()); });});

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
            /* Ensure we get a fresh frame by consuming any existing stale frame
             *
             * This ensures each Compositor is treated as displaying the
             * “current frame”, so the first time through the composition
             * loop below we will pull the most recently submitted frame from
             * each surface (either the “current” frame, or the ”next” frame).
             */
            for (auto const& [_, compositor] : compositors)
            {
                auto elements = scene->scene_elements_for(compositor.get());
                for (auto const& element : elements)
                {
                    // We need to actually access the buffer in order for it to be marked in use.
                    element->renderable()->buffer();
                }
            }

            while (running)
            {
                /* Wait until compositing has been scheduled or we are stopped */
                wakeup.wait();

                /*
                 * Check if we are running before compositing, since we may have
                 * been stopped while waiting for the run_cv above.
                 */
                if (running)
                {
                    bool needs_post = false;
                    for (auto& tuple : compositors)
                    {
                        auto& compositor = std::get<1>(tuple);
                        if (compositor->composite(scene->scene_elements_for(compositor.get())))
                            needs_post = true;
                    }

                    // We can skip the post if none of the compositors ended up compositing
                    if (needs_post)
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
    }

    void schedule_compositing()
    {
        wakeup.raise();
    }

    void schedule_compositing(geometry::Rectangle const& damage)
    {
        bool took_damage = false;
        group.for_each_display_sink([&](mg::DisplaySink& sink)
            { if (damage.overlaps(sink.view_area())) took_damage = true; });

        if (took_damage)
        {
            wakeup.raise();
        }
    }

    void stop()
    {
        running = false;
        wakeup.raise();
    }

    void wait_until_started()
    {
        if (started_future.wait_for(10s) != std::future_status::ready)
            BOOST_THROW_EXCEPTION(std::runtime_error("Compositor thread failed to start"));

        started_future.get();
    }

    void wait_until_stopped()
    {
        stop();
        if (stopped_future.wait_for(10h) != std::future_status::ready)
            BOOST_THROW_EXCEPTION(std::runtime_error("Compositor thread failed to stop"));

        stopped_future.get();
    }

private:
    std::shared_ptr<mc::DisplayBufferCompositorFactory> const compositor_factory;
    mg::DisplaySyncGroup& group;
    std::shared_ptr<mc::Scene> const scene;
    mir::Signal wakeup;
    std::atomic<bool> running;
    std::chrono::milliseconds force_sleep{-1};
    std::shared_ptr<DisplayListener> const display_listener;
    std::shared_ptr<CompositorReport> const report;
    std::promise<void> started;
    std::future<void> started_future;
    std::promise<void> stopped;
    std::future<void> stopped_future;
};

}
}

mc::MultiThreadedCompositor::MultiThreadedCompositor(
    std::shared_ptr<mg::Display> const& display,
    std::shared_ptr<DisplayBufferCompositorFactory> const& db_compositor_factory,
    std::shared_ptr<mc::Scene> const& scene,
    std::shared_ptr<DisplayListener> const& display_listener,
    std::shared_ptr<CompositorReport> const& compositor_report,
    std::chrono::milliseconds fixed_composite_delay,
    bool compose_on_start)
    : display{display},
      display_buffer_compositor_factory{db_compositor_factory},
      scene{scene},
      display_listener{display_listener},
      report{compositor_report},
      state{CompositorState::stopped},
      fixed_composite_delay{fixed_composite_delay},
      compose_on_start{compose_on_start}
{
    observer = std::make_shared<ms::SceneChangeNotification>(
    [this]()
    {
        schedule_compositing();
    },
    [this](geometry::Rectangle const& damage)
    {
        schedule_compositing(damage);
    });
}

mc::MultiThreadedCompositor::~MultiThreadedCompositor()
{
    stop();
}

void mc::MultiThreadedCompositor::schedule_compositing()
{
    report->scheduled();
    for (auto& f : thread_functors)
        f->schedule_compositing();
}

void mc::MultiThreadedCompositor::schedule_compositing(geometry::Rectangle const& damage) const
{
    report->scheduled();
    for (auto& f : thread_functors)
        f->schedule_compositing(damage);
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
            destroy_compositing_threads();
            state = CompositorState::stopped;
        });

    create_compositing_threads();

    /* Add the observer after we have created the compositing threads */
    scene->add_observer(observer);

    /* Optional first render */
    if (compose_on_start)
        schedule_compositing();

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

        mir::thread_pool_executor.spawn(std::ref(*thread_functor));
        thread_functors.push_back(std::move(thread_functor));
    });

    std::exception_ptr x;
    for (auto& functor : thread_functors)
    try
    {
        functor->wait_until_started();
    }
    catch (...)
    {
        x = std::current_exception();
    }

    if (x)
    {
        rethrow_exception(x);
    }
}

void mc::MultiThreadedCompositor::destroy_compositing_threads()
{
    for (auto& f : thread_functors)
        f->stop();

    for (auto& f : thread_functors)
        f->wait_until_stopped();

    thread_functors.clear();
}
