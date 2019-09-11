/*
 * Copyright © 2013-2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
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

#include "src/server/compositor/multi_threaded_compositor.h"
#include "src/server/report/null_report_factory.h"

#include "mir/compositor/display_listener.h"
#include "mir/compositor/display_buffer_compositor.h"
#include "mir/compositor/scene.h"
#include "mir/compositor/display_buffer_compositor_factory.h"
#include "mir/scene/observer.h"
#include "mir/raii.h"

#include "mir/test/current_thread_name.h"
#include "mir/test/doubles/null_display.h"
#include "mir/test/doubles/null_display_buffer.h"
#include "mir/test/doubles/mock_display_buffer.h"
#include "mir/test/doubles/mock_compositor_report.h"
#include "mir/test/doubles/mock_scene.h"
#include "mir/test/doubles/stub_scene.h"
#include "mir/test/doubles/stub_display.h"
#include "mir/test/doubles/null_display_buffer_compositor_factory.h"

#include <boost/throw_exception.hpp>

#include <unordered_map>
#include <unordered_set>
#include <thread>
#include <mutex>
#include <chrono>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <csignal>

using namespace std::literals::chrono_literals;

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace ms = mir::scene;
namespace mr = mir::report;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;
namespace mt = mir::test;

class StubDisplayWithMockBuffers : public mtd::NullDisplay
{
public:
    StubDisplayWithMockBuffers(unsigned int nbuffers) : buffers{nbuffers} {}

    void for_each_display_sync_group(std::function<void(mg::DisplaySyncGroup&)> const& f) override
    {
        for (auto& db : buffers)
            f(db);
    }

    void for_each_mock_buffer(std::function<void(mtd::MockDisplayBuffer&)> const& f)
    {
        for (auto& db : buffers)
            f(db.buffer);
    }

private:
    struct StubDisplaySyncGroup : mg::DisplaySyncGroup
    {
        void for_each_display_buffer(std::function<void(mg::DisplayBuffer&)> const& f) override
        {
            f(buffer);            
        }
        void post() override {}
        std::chrono::milliseconds recommended_sleep() const override
        {
            return std::chrono::milliseconds::zero();
        }
        testing::NiceMock<mtd::MockDisplayBuffer> buffer; 
    };

    std::vector<StubDisplaySyncGroup> buffers;
};

class StubScene : public mtd::StubScene
{
public:
    StubScene()
        : pending{0}, throw_on_add_observer_{false}
    {
    }

    void add_observer(std::shared_ptr<ms::Observer> const& observer_) override
    {
        std::lock_guard<std::mutex> lock{observer_mutex};

        if (throw_on_add_observer_)
            BOOST_THROW_EXCEPTION(std::runtime_error(""));

        assert(!observer);
        observer = observer_;
    }

    void remove_observer(std::weak_ptr<ms::Observer> const& /* observer */) override
    {
        std::lock_guard<std::mutex> lock{observer_mutex};
        observer.reset();
    }

    void emit_change_event()
    {
        {
            std::lock_guard<std::mutex> lock{observer_mutex};
            
            // Any old event will do.
            if (observer)
                observer->surfaces_reordered();
        }
        /* Reduce run-time under valgrind */
        std::this_thread::yield();
    }

    void throw_on_add_observer(bool flag)
    {
        throw_on_add_observer_ = flag;
    }

    int frames_pending(mc::CompositorID) const override
    {
        return pending;
    }

    void set_pending(int n)
    {
        pending = n;
        observer->scene_changed();
    }

private:
    std::atomic<int> pending;
    std::mutex observer_mutex;
    std::shared_ptr<ms::Observer> observer;
    bool throw_on_add_observer_;
};

class RecordingDisplayBufferCompositor : public mc::DisplayBufferCompositor
{
public:
    RecordingDisplayBufferCompositor(std::function<void()> const& mark_render_buffer)
        : mark_render_buffer{mark_render_buffer}
    {
    }

    void composite(mc::SceneElementSequence&&)
    {
        mark_render_buffer();
        /* Reduce run-time under valgrind */
        std::this_thread::yield();
    }

private:
    std::function<void()> const mark_render_buffer;
};


class RecordingDisplayBufferCompositorFactory : public mc::DisplayBufferCompositorFactory
{
public:
    std::unique_ptr<mc::DisplayBufferCompositor> create_compositor_for(mg::DisplayBuffer& display_buffer)
    {
        auto raw = new RecordingDisplayBufferCompositor{
            [&display_buffer,this]()
            {
                mark_render_buffer(display_buffer);
            }};
        return std::unique_ptr<RecordingDisplayBufferCompositor>(raw);
    }

    void mark_render_buffer(mg::DisplayBuffer& display_buffer)
    {
        std::lock_guard<std::mutex> lk{m};

        if (records.find(&display_buffer) == records.end())
            records[&display_buffer] = Record(0, std::unordered_set<std::thread::id>());

        ++records[&display_buffer].first;
        records[&display_buffer].second.insert(std::this_thread::get_id());
    }

    bool enough_records_gathered(unsigned int nbuffers, unsigned int min_record_count = 1000)
    {
        std::lock_guard<std::mutex> lk{m};

        if (records.size() < nbuffers)
            return false;

        for (auto const& e : records)
        {
            Record const& r = e.second;
            if (r.first < min_record_count)
                return false;
        }

        return true;
    }

    bool each_buffer_rendered_in_single_thread()
    {
        for (auto const& e : records)
        {
            Record const& r = e.second;
            if (r.second.size() != 1)
                return false;
        }

        return true;
    }

    bool buffers_rendered_in_different_threads()
    {
        std::unordered_set<std::thread::id> seen;
        seen.insert(std::this_thread::get_id());

        for (auto const& e : records)
        {
            Record const& r = e.second;
            auto iter = r.second.begin();
            if (seen.find(*iter) != seen.end())
                return false;
            seen.insert(*iter);
        }

        return true;
    }

    bool check_record_count_for_each_buffer(
            unsigned int nbuffers,
            unsigned int min,
            unsigned int max = ~0u)
    {
        std::lock_guard<std::mutex> lk{m};

        if (records.size() < nbuffers)
            return (min == 0 && max == 0);

        for (auto const& e : records)
        {
            Record const& r = e.second;
            if (r.first < min || r.first > max)
                return false;
        }

        return true;
    }

private:
    std::mutex m;
    typedef std::pair<unsigned int, std::unordered_set<std::thread::id>> Record;
    std::unordered_map<mg::DisplayBuffer*,Record> records;
};

class SurfaceUpdatingDisplayBufferCompositor : public mc::DisplayBufferCompositor
{
public:
    SurfaceUpdatingDisplayBufferCompositor(std::function<void()> const& fake_surface_update)
        : fake_surface_update{fake_surface_update}
    {
    }

    void composite(mc::SceneElementSequence&&) override
    {
        fake_surface_update();
        /* Reduce run-time under valgrind */
        std::this_thread::yield();
    }

private:
    std::function<void()> const fake_surface_update;
};

class SurfaceUpdatingDisplayBufferCompositorFactory : public mc::DisplayBufferCompositorFactory
{
public:
    SurfaceUpdatingDisplayBufferCompositorFactory(std::shared_ptr<StubScene> const& scene)
        : scene{scene},
          render_count{0}
    {
    }

    std::unique_ptr<mc::DisplayBufferCompositor> create_compositor_for(mg::DisplayBuffer&)
    {
        auto raw = new SurfaceUpdatingDisplayBufferCompositor{[this]{fake_surface_update();}};
        return std::unique_ptr<SurfaceUpdatingDisplayBufferCompositor>(raw);
    }

    void fake_surface_update()
    {
        scene->emit_change_event();
        ++render_count;
    }

    bool enough_renders_happened()
    {
        unsigned int const enough_renders{1000};
        return render_count > enough_renders;
    }

private:
    std::shared_ptr<StubScene> const scene;
    std::atomic<unsigned int> render_count;
};

class ThreadNameDisplayBufferCompositorFactory : public mc::DisplayBufferCompositorFactory
{
public:
    std::unique_ptr<mc::DisplayBufferCompositor> create_compositor_for(mg::DisplayBuffer&)
    {
        auto raw = new RecordingDisplayBufferCompositor{
            [this]()
            {
                std::lock_guard<std::mutex> lock{thread_names_mutex};
                thread_names.emplace_back(mt::current_thread_name());
            }};
        return std::unique_ptr<RecordingDisplayBufferCompositor>(raw);
    }

    size_t num_thread_names_gathered()
    {
        std::lock_guard<std::mutex> lock{thread_names_mutex};
        return thread_names.size();
    }

    std::mutex thread_names_mutex;
    std::vector<std::string> thread_names;
};

namespace
{
struct StubDisplayListener : mc::DisplayListener
{
    virtual void add_display(geom::Rectangle const& /*area*/) override {}
    virtual void remove_display(geom::Rectangle const& /*area*/) override {}
};

struct MockDisplayListener : mc::DisplayListener
{
    MOCK_METHOD1(add_display, void(geom::Rectangle const& /*area*/));
    MOCK_METHOD1(remove_display, void(geom::Rectangle const& /*area*/));
};

auto const null_report = mr::null_compositor_report();
unsigned int const composites_per_update{1};
auto const null_display_listener = std::make_shared<StubDisplayListener>();
std::chrono::milliseconds const default_delay{-1};

}

TEST(MultiThreadedCompositor, compositing_happens_in_different_threads)
{
    using namespace testing;

    unsigned int const nbuffers{3};

    auto display = std::make_shared<mtd::StubDisplay>(nbuffers);
    auto scene = std::make_shared<StubScene>();
    auto db_compositor_factory = std::make_shared<RecordingDisplayBufferCompositorFactory>();
    mc::MultiThreadedCompositor compositor{display, scene, db_compositor_factory, null_display_listener, null_report, default_delay, true};

    compositor.start();

    while (!db_compositor_factory->enough_records_gathered(nbuffers))
        scene->emit_change_event();

    compositor.stop();

    EXPECT_TRUE(db_compositor_factory->each_buffer_rendered_in_single_thread());
    EXPECT_TRUE(db_compositor_factory->buffers_rendered_in_different_threads());
}

TEST(MultiThreadedCompositor, does_not_deadlock_itself)
{   // Regression test for LP: #1471909
    auto scene = std::make_shared<StubScene>();

    class ReentrantDisplayListener : public mc::DisplayListener
    {
    public:
        ReentrantDisplayListener(std::shared_ptr<StubScene> const& scene)
            : scene{scene}
        {
        }
        void add_display(geom::Rectangle const&) override
        {
            scene->emit_change_event();
        }
        void remove_display(geom::Rectangle const&) override
        {
        }
    private:
        std::shared_ptr<StubScene> const scene;
    };

    mc::MultiThreadedCompositor compositor{
        std::make_shared<mtd::StubDisplay>(3),
        scene,
        std::make_shared<mtd::NullDisplayBufferCompositorFactory>(),
        std::make_shared<ReentrantDisplayListener>(scene),
        null_report,
        default_delay,
        true
    };

    for (int i = 0; i < 1000; ++i)
    {
        compositor.start();
        compositor.stop();
        std::this_thread::yield();
    }
}

TEST(MultiThreadedCompositor, reports_in_the_right_places)
{
    using namespace testing;

    auto display = std::make_shared<StubDisplayWithMockBuffers>(1);
    auto scene = std::make_shared<StubScene>();
    auto db_compositor_factory =
        std::make_shared<RecordingDisplayBufferCompositorFactory>();
    auto mock_report = std::make_shared<mtd::MockCompositorReport>();
    mc::MultiThreadedCompositor compositor{display, scene,
                                           db_compositor_factory,
                                           null_display_listener,
                                           mock_report,
                                           default_delay,
                                           true};

    EXPECT_CALL(*mock_report, started())
        .Times(1);

    display->for_each_mock_buffer([](mtd::MockDisplayBuffer& mock_buf)
    {
        EXPECT_CALL(mock_buf, view_area()).Times(AtLeast(1))
            .WillRepeatedly(Return(geom::Rectangle()));
    });

    EXPECT_CALL(*mock_report, added_display(_,_,_,_,_))
        .Times(1);
    EXPECT_CALL(*mock_report, scheduled())
        .Times(2);

    EXPECT_CALL(*mock_report, stopped())
        .Times(AtLeast(1));

    compositor.start();
    scene->emit_change_event();
    while (!db_compositor_factory->check_record_count_for_each_buffer(1, composites_per_update))
        std::this_thread::yield();
    compositor.stop();
}

/*
 * It's difficult to test that a render won't happen, without some further
 * introspective capabilities that would complicate the code. This test will
 * catch a problem if it occurs, but can't ensure that a problem, even if
 * present, will occur in a determinstic manner.
 *
 * Nonetheless, the test is useful since it's very likely to fail if a problem
 * is present (and don't forget that it's usually run multiple times per day).
 */
TEST(MultiThreadedCompositor, composites_only_on_demand)
{
    using namespace testing;

    unsigned int const nbuffers = 3;

    auto display = std::make_shared<mtd::StubDisplay>(nbuffers);
    auto scene = std::make_shared<StubScene>();
    auto db_compositor_factory = std::make_shared<RecordingDisplayBufferCompositorFactory>();
    mc::MultiThreadedCompositor compositor{display, scene, db_compositor_factory, null_display_listener, null_report, default_delay, true};

    // Verify we're actually starting at zero frames
    EXPECT_TRUE(db_compositor_factory->check_record_count_for_each_buffer(nbuffers, 0, 0));

    compositor.start();

    // Initial render: initial_composites frames should be composited at least
    while (!db_compositor_factory->check_record_count_for_each_buffer(nbuffers, composites_per_update))
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Now we have initial_composites redraws, pause for a while
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // ... and make sure the number is still only 3
    EXPECT_TRUE(db_compositor_factory->check_record_count_for_each_buffer(nbuffers, composites_per_update, composites_per_update));

    // Trigger more surface changes
    scene->emit_change_event();

    // Display buffers should be forced to render another 3, so that's 6
    while (!db_compositor_factory->check_record_count_for_each_buffer(nbuffers, 2*composites_per_update))
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Now pause without any further surface changes
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Verify we never triggered more than 2*initial_composites compositions
    EXPECT_TRUE(db_compositor_factory->check_record_count_for_each_buffer(nbuffers, 2*composites_per_update, 2*composites_per_update));

    compositor.stop();  // Pause the compositor so we don't race

    // Now trigger many surfaces changes close together
    for (int i = 0; i < 10; i++)
        scene->emit_change_event();

    compositor.start();

    // Display buffers should be forced to render another 3, so that's 9
    while (!db_compositor_factory->check_record_count_for_each_buffer(nbuffers, 3*composites_per_update))
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Now pause without any further surface changes
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Verify we never triggered more than 9 compositions
    EXPECT_TRUE(db_compositor_factory->check_record_count_for_each_buffer(nbuffers, 3*composites_per_update, 3*composites_per_update));

    compositor.stop();
}

TEST(MultiThreadedCompositor, schedules_enough_frames)
{
    using namespace testing;

    unsigned int const nbuffers = 3;

    auto display = std::make_shared<mtd::StubDisplay>(nbuffers);
    auto scene = std::make_shared<StubScene>();
    auto factory = std::make_shared<RecordingDisplayBufferCompositorFactory>();
    mc::MultiThreadedCompositor compositor{display, scene, factory,
                                           null_display_listener, null_report, default_delay, true};

    EXPECT_TRUE(factory->check_record_count_for_each_buffer(nbuffers, 0, 0));

    compositor.start();

    int const max_retries = 100;
    int retry = 0;
    while (retry < max_retries &&
           !factory->check_record_count_for_each_buffer(nbuffers, 1))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        ++retry;
    }

    ASSERT_LT(retry, max_retries);

    // Now we have the initial frame wait a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    // ... and make sure the number is still only 1
    ASSERT_TRUE(factory->check_record_count_for_each_buffer(nbuffers, 1, 1));

    // Trigger scene changes. Only need one frame to cover them all...
    scene->emit_change_event();
    scene->emit_change_event();
    scene->emit_change_event();

    // Display buffers should be forced to render another 1, so that's 2
    retry = 0;
    while (retry < max_retries &&
           !factory->check_record_count_for_each_buffer(nbuffers, 2))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        ++retry;
    }
    ASSERT_LT(retry, max_retries);

    // Scene unchanged. Expect no new frames:
    retry = 0;
    while (retry < max_retries &&
           !factory->check_record_count_for_each_buffer(nbuffers, 2))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        ++retry;
    }
    ASSERT_LT(retry, max_retries);

    // Some surface queues up multiple frames
    scene->set_pending(3);

    // Make sure they all get composited separately (2 + 3 more)
    retry = 0;
    while (retry < max_retries &&
           !factory->check_record_count_for_each_buffer(nbuffers, 5))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        ++retry;
    }
    ASSERT_LT(retry, max_retries);

    compositor.stop();
}

TEST(MultiThreadedCompositor, recommended_sleep_throttles_compositor_loop)
{
    using namespace testing;
    using namespace std::chrono;

    unsigned int const nbuffers = 3;
    milliseconds const recommendation(10);

    auto display = std::make_shared<mtd::StubDisplay>(nbuffers);
    auto scene = std::make_shared<StubScene>();
    auto factory = std::make_shared<RecordingDisplayBufferCompositorFactory>();
    mc::MultiThreadedCompositor compositor{display, scene, factory,
                                           null_display_listener, null_report,
                                           recommendation, false};

    EXPECT_TRUE(factory->check_record_count_for_each_buffer(nbuffers, 0, 0));

    compositor.start();

    int const max_retries = 100;
    int const nframes = 10;
    auto start = system_clock::now();

    for (int frame = 1; frame <= nframes; ++frame)
    {
        scene->emit_change_event();

        int retry = 0;
        while (retry < max_retries &&
               !factory->check_record_count_for_each_buffer(nbuffers, frame))
        {
            std::this_thread::sleep_for(milliseconds(1));
            ++retry;
        }
        ASSERT_LT(retry, max_retries);
    }

    /*
     * Detecting the throttling from outside the compositor thread is actually
     * trickier than you think. Because the display buffer counter won't be
     * delayed by the sleep; only the subsequent frame will be delayed. So
     * that's why we measure overall duration here...
     */
    auto duration = system_clock::now() - start;
    // Minus 2 because the first won't be throttled, and the last not detected.
    int minimum = recommendation.count() * (nframes - 2);
    ASSERT_THAT(duration_cast<milliseconds>(duration).count(),
                Ge(minimum));

    compositor.stop();
}

TEST(MultiThreadedCompositor, when_no_initial_composite_is_needed_there_is_none)
{
    using namespace testing;

    unsigned int const nbuffers = 3;

    auto display = std::make_shared<mtd::StubDisplay>(nbuffers);
    auto scene = std::make_shared<StubScene>();
    auto db_compositor_factory = std::make_shared<RecordingDisplayBufferCompositorFactory>();
    mc::MultiThreadedCompositor compositor{display, scene, db_compositor_factory, null_display_listener, null_report, default_delay, false};

    // Verify we're actually starting at zero frames
    ASSERT_TRUE(db_compositor_factory->check_record_count_for_each_buffer(nbuffers, 0, 0));

    compositor.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Verify we're still at zero frames
    EXPECT_TRUE(db_compositor_factory->check_record_count_for_each_buffer(nbuffers, 0, 0));

    compositor.stop();
}

TEST(MultiThreadedCompositor, when_no_initial_composite_is_needed_we_still_composite_on_restart)
{
    using namespace testing;

    unsigned int const nbuffers = 3;

    auto display = std::make_shared<mtd::StubDisplay>(nbuffers);
    auto scene = std::make_shared<StubScene>();
    auto db_compositor_factory = std::make_shared<RecordingDisplayBufferCompositorFactory>();
    mc::MultiThreadedCompositor compositor{display, scene, db_compositor_factory, null_display_listener, null_report, default_delay, false};

    compositor.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Verify we're actually starting at zero frames
    ASSERT_TRUE(db_compositor_factory->check_record_count_for_each_buffer(nbuffers, 0, 0));

    compositor.stop();
    compositor.start();

    for (int countdown = 100;
        countdown != 0 &&
        !db_compositor_factory->check_record_count_for_each_buffer(nbuffers, composites_per_update, composites_per_update);
        --countdown)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Verify we composited the expected frame
    EXPECT_TRUE(db_compositor_factory->check_record_count_for_each_buffer(nbuffers, composites_per_update, composites_per_update));

    compositor.stop();
}

TEST(MultiThreadedCompositor, surface_update_from_render_doesnt_deadlock)
{
    using namespace testing;

    unsigned int const nbuffers{3};

    auto display = std::make_shared<mtd::StubDisplay>(nbuffers);
    auto scene = std::make_shared<StubScene>();
    auto db_compositor_factory = std::make_shared<SurfaceUpdatingDisplayBufferCompositorFactory>(scene);
    mc::MultiThreadedCompositor compositor{display, scene, db_compositor_factory, null_display_listener, null_report, default_delay, true};

    compositor.start();

    while (!db_compositor_factory->enough_renders_happened())
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    compositor.stop();
}

TEST(MultiThreadedCompositor, double_start_or_stop_ignored)
{
    using namespace testing;

    unsigned int const nbuffers{3};
    auto display = std::make_shared<StubDisplayWithMockBuffers>(nbuffers);
    auto mock_scene = std::make_shared<NiceMock<mtd::MockScene>>();
    auto db_compositor_factory = std::make_shared<mtd::NullDisplayBufferCompositorFactory>();
    auto mock_report = std::make_shared<testing::NiceMock<mtd::MockCompositorReport>>();

    EXPECT_CALL(*mock_report, started())
        .Times(1);
    EXPECT_CALL(*mock_report, stopped())
        .Times(1);
    EXPECT_CALL(*mock_scene, add_observer(_))
        .Times(1);
    EXPECT_CALL(*mock_scene, remove_observer(_))
        .Times(1);
    EXPECT_CALL(*mock_scene, scene_elements_for(_))
        .Times(AtLeast(0))
        .WillRepeatedly(Return(mc::SceneElementSequence{}));

    mc::MultiThreadedCompositor compositor{display, mock_scene, db_compositor_factory, null_display_listener, mock_report, default_delay, true};

    compositor.start();
    compositor.start();
    compositor.stop();
    compositor.stop();
}

TEST(MultiThreadedCompositor, cleans_up_after_throw_in_start)
{
    unsigned int const nbuffers{3};

    auto display = std::make_shared<StubDisplayWithMockBuffers>(nbuffers);
    auto scene = std::make_shared<StubScene>();
    auto db_compositor_factory = std::make_shared<RecordingDisplayBufferCompositorFactory>();
    mc::MultiThreadedCompositor compositor{display, scene, db_compositor_factory, null_display_listener, null_report, default_delay, true};

    scene->throw_on_add_observer(true);

    EXPECT_THROW(compositor.start(), std::runtime_error);

    scene->throw_on_add_observer(false);

    /* No point in running the rest of the test if it throws again */
    ASSERT_NO_THROW(compositor.start());

    /* The minimum number of records here should be nbuffers *2, since we are checking for
     * presence of at least one additional rogue compositor thread per display buffer
     * However to avoid timing considerations like one good thread compositing the display buffer
     * twice before the rogue thread gets a chance to, an arbitrary number of records are gathered
     */
    unsigned int min_number_of_records = 100;

    /* Timeout here in case the exception from setting the scene callback put the compositor
     * in a bad state that did not allow it to composite (hence no records gathered)
     */
    auto time_out = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!db_compositor_factory->enough_records_gathered(nbuffers, min_number_of_records) &&
           std::chrono::steady_clock::now() <= time_out)
    {
        scene->emit_change_event();
        std::this_thread::yield();
    }

    /* Check expectation in case a timeout happened */
    EXPECT_TRUE(db_compositor_factory->enough_records_gathered(nbuffers, min_number_of_records));

    compositor.stop();

    /* Only one thread should be rendering each display buffer
     * If the compositor failed to cleanup correctly more than one thread could be
     * compositing the same display buffer
     */
    EXPECT_TRUE(db_compositor_factory->each_buffer_rendered_in_single_thread());
}

#ifndef MIR_DONT_USE_PTHREAD_GETNAME_NP
TEST(MultiThreadedCompositor, names_compositor_threads)
#else
TEST(MultiThreadedCompositor, DISABLED_names_compositor_threads)
#endif
{
    using namespace testing;

    unsigned int const nbuffers{3};

    auto display = std::make_shared<StubDisplayWithMockBuffers>(nbuffers);
    auto scene = std::make_shared<StubScene>();
    auto db_compositor_factory = std::make_shared<ThreadNameDisplayBufferCompositorFactory>();
    mc::MultiThreadedCompositor compositor{display, scene, db_compositor_factory, null_display_listener, null_report, default_delay, true};

    compositor.start();

    unsigned int const min_number_of_thread_names = 10;

    while (db_compositor_factory->num_thread_names_gathered() < min_number_of_thread_names)
        scene->emit_change_event();

    compositor.stop();

    auto const& thread_names = db_compositor_factory->thread_names;

    for (size_t i = 0; i < thread_names.size(); ++i)
        EXPECT_THAT(thread_names[i], Eq("Mir/Comp")) << "i=" << i;
}

TEST(MultiThreadedCompositor, registers_and_unregisters_with_scene)
{
    using namespace testing;
    unsigned int const nbuffers{3};
    auto display = std::make_shared<StubDisplayWithMockBuffers>(nbuffers);
    auto mock_scene = std::make_shared<NiceMock<mtd::MockScene>>();
    auto db_compositor_factory = std::make_shared<mtd::NullDisplayBufferCompositorFactory>();
    auto mock_report = std::make_shared<testing::NiceMock<mtd::MockCompositorReport>>();

    EXPECT_CALL(*mock_scene, register_compositor(_))
        .Times(nbuffers);
    mc::MultiThreadedCompositor compositor{
        display, mock_scene, db_compositor_factory, null_display_listener, mock_report, default_delay, true};

    compositor.start();

    Mock::VerifyAndClearExpectations(mock_scene.get());

    EXPECT_CALL(*mock_scene, unregister_compositor(_))
        .Times(nbuffers);

    compositor.stop();
}

TEST(MultiThreadedCompositor, notifies_about_display_additions_and_removals)
{
    using namespace testing;
    unsigned int const nbuffers{3};
    auto display = std::make_shared<StubDisplayWithMockBuffers>(nbuffers);
    auto stub_scene = std::make_shared<NiceMock<StubScene>>();
    auto mock_display_listener = std::make_shared<MockDisplayListener>();
    auto db_compositor_factory = std::make_shared<mtd::NullDisplayBufferCompositorFactory>();
    auto mock_report = std::make_shared<testing::NiceMock<mtd::MockCompositorReport>>();

    mc::MultiThreadedCompositor compositor{
        display, stub_scene, db_compositor_factory, mock_display_listener, mock_report, default_delay, true};

    EXPECT_CALL(*mock_display_listener, add_display(_)).Times(nbuffers);

    compositor.start();
    Mock::VerifyAndClearExpectations(mock_display_listener.get());

    EXPECT_CALL(*mock_display_listener, remove_display(_)).Times(nbuffers);
    compositor.stop();
}

TEST(MultiThreadedCompositor, when_compositor_thread_fails_start_reports_error)
{
    using namespace testing;
    unsigned int const nbuffers{3};
    auto display = std::make_shared<StubDisplayWithMockBuffers>(nbuffers);
    auto stub_scene = std::make_shared<NiceMock<StubScene>>();
    auto mock_display_listener = std::make_shared<MockDisplayListener>();
    auto db_compositor_factory = std::make_shared<mtd::NullDisplayBufferCompositorFactory>();
    auto mock_report = std::make_shared<testing::NiceMock<mtd::MockCompositorReport>>();

    mc::MultiThreadedCompositor compositor{
        display, stub_scene, db_compositor_factory, mock_display_listener, mock_report, default_delay, true};

    EXPECT_CALL(*mock_display_listener, add_display(_))
        .WillRepeatedly(Throw(std::runtime_error("Failed to add display")));

    EXPECT_THROW(compositor.start(), std::runtime_error);
}

//LP: 1481418
TEST(MultiThreadedCompositor, can_schedule_from_display_observer_when_adding_display)
{
    using namespace testing;
    unsigned int const nbuffers{3};
    auto display = std::make_shared<StubDisplayWithMockBuffers>(nbuffers);
    auto stub_scene = std::make_shared<NiceMock<StubScene>>();
    auto mock_display_listener = std::make_shared<NiceMock<MockDisplayListener>>();
    auto db_compositor_factory = std::make_shared<mtd::NullDisplayBufferCompositorFactory>();
    auto mock_report = std::make_shared<testing::NiceMock<mtd::MockCompositorReport>>();

    ON_CALL(*mock_display_listener, add_display(_))
        .WillByDefault(InvokeWithoutArgs([&]{ stub_scene->emit_change_event(); }));

    mc::MultiThreadedCompositor compositor{
        display, stub_scene, db_compositor_factory, mock_display_listener, mock_report, default_delay, true};
    compositor.start();
}

TEST(MultiThreadedCompositor, can_schedule_from_display_observer_when_removing_display)
{
    using namespace testing;
    unsigned int const nbuffers{3};
    auto display = std::make_shared<StubDisplayWithMockBuffers>(nbuffers);
    auto stub_scene = std::make_shared<NiceMock<StubScene>>();
    auto mock_display_listener = std::make_shared<NiceMock<MockDisplayListener>>();
    auto db_compositor_factory = std::make_shared<mtd::NullDisplayBufferCompositorFactory>();
    auto mock_report = std::make_shared<testing::NiceMock<mtd::MockCompositorReport>>();

    ON_CALL(*mock_display_listener, remove_display(_))
        .WillByDefault(InvokeWithoutArgs([&]{ stub_scene->emit_change_event(); }));

    mc::MultiThreadedCompositor compositor{
        display, stub_scene, db_compositor_factory, mock_display_listener, mock_report, default_delay, true};
    compositor.start();
}
