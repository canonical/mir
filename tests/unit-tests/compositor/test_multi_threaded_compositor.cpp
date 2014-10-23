/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
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

#include "mir/compositor/display_buffer_compositor.h"
#include "mir/compositor/scene.h"
#include "mir/compositor/display_buffer_compositor_factory.h"
#include "mir/scene/observer.h"

#include "mir_test/current_thread_name.h"
#include "mir_test_doubles/null_display.h"
#include "mir_test_doubles/null_display_buffer.h"
#include "mir_test_doubles/mock_display_buffer.h"
#include "mir_test_doubles/mock_compositor_report.h"
#include "mir_test_doubles/mock_scene.h"
#include "mir_test_doubles/stub_scene.h"

#include <boost/throw_exception.hpp>

#include <unordered_map>
#include <unordered_set>
#include <thread>
#include <mutex>
#include <chrono>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace ms = mir::scene;
namespace mr = mir::report;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;
namespace mt = mir::test;

namespace
{

class StubDisplay : public mtd::NullDisplay
{
 public:
    StubDisplay(unsigned int nbuffers) : buffers{nbuffers} {}

    void for_each_display_buffer(std::function<void(mg::DisplayBuffer&)> const& f) override
    {
        for (auto& db : buffers)
            f(db);
    }

private:
    std::vector<mtd::NullDisplayBuffer> buffers;
};

class StubDisplayWithMockBuffers : public mtd::NullDisplay
{
 public:
    StubDisplayWithMockBuffers(unsigned int nbuffers) : buffers{nbuffers} {}

    void for_each_display_buffer(std::function<void(mg::DisplayBuffer&)> const& f)
    {
        for (auto& db : buffers)
            f(db);
    }

    void for_each_mock_buffer(std::function<void(mtd::MockDisplayBuffer&)> const& f)
    {
        for (auto& db : buffers)
            f(db);
    }

private:
    std::vector<testing::NiceMock<mtd::MockDisplayBuffer>> buffers;
};

class StubScene : public mtd::StubScene
{
public:
    void add_observer(std::shared_ptr<ms::Observer> const& observer_)
    {
        std::lock_guard<std::mutex> lock{observer_mutex};

        if (throw_on_add_observer_)
            BOOST_THROW_EXCEPTION(std::runtime_error(""));

        assert(!observer);
        observer = observer_;
    }

    void remove_observer(std::weak_ptr<ms::Observer> const& /* observer */)
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

private:
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
    unsigned int render_count;
};

class NullDisplayBufferCompositorFactory : public mc::DisplayBufferCompositorFactory
{
public:
    std::unique_ptr<mc::DisplayBufferCompositor> create_compositor_for(mg::DisplayBuffer&)
    {
        struct NullDisplayBufferCompositor : mc::DisplayBufferCompositor
        {
            void composite(mc::SceneElementSequence&&) override
            {
            }
        };

        auto raw = new NullDisplayBufferCompositor{};
        return std::unique_ptr<NullDisplayBufferCompositor>(raw);
    }
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

auto const null_report = mr::null_compositor_report();
unsigned int const composites_per_update{1};
}

TEST(MultiThreadedCompositor, compositing_happens_in_different_threads)
{
    using namespace testing;

    unsigned int const nbuffers{3};

    auto display = std::make_shared<StubDisplay>(nbuffers);
    auto scene = std::make_shared<StubScene>();
    auto db_compositor_factory = std::make_shared<RecordingDisplayBufferCompositorFactory>();
    mc::MultiThreadedCompositor compositor{display, scene, db_compositor_factory, null_report, true};

    compositor.start();

    while (!db_compositor_factory->enough_records_gathered(nbuffers))
        scene->emit_change_event();

    compositor.stop();

    EXPECT_TRUE(db_compositor_factory->each_buffer_rendered_in_single_thread());
    EXPECT_TRUE(db_compositor_factory->buffers_rendered_in_different_threads());
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
                                           mock_report,
                                           true};

    EXPECT_CALL(*mock_report, started())
        .Times(1);

    display->for_each_mock_buffer([](mtd::MockDisplayBuffer& mock_buf)
    {
        EXPECT_CALL(mock_buf, make_current()).Times(1);
        EXPECT_CALL(mock_buf, view_area())
            .WillOnce(Return(geom::Rectangle()));
    });

    EXPECT_CALL(*mock_report, added_display(_,_,_,_,_))
        .Times(1);
    EXPECT_CALL(*mock_report, scheduled())
        .Times(2);

    display->for_each_mock_buffer([](mtd::MockDisplayBuffer& mock_buf)
    {
        EXPECT_CALL(mock_buf, release_current()).Times(1);
    });

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

    auto display = std::make_shared<StubDisplay>(nbuffers);
    auto scene = std::make_shared<StubScene>();
    auto db_compositor_factory = std::make_shared<RecordingDisplayBufferCompositorFactory>();
    mc::MultiThreadedCompositor compositor{display, scene, db_compositor_factory, null_report, true};

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

TEST(MultiThreadedCompositor, when_no_initial_composite_is_needed_there_is_none)
{
    using namespace testing;

    unsigned int const nbuffers = 3;

    auto display = std::make_shared<StubDisplay>(nbuffers);
    auto scene = std::make_shared<StubScene>();
    auto db_compositor_factory = std::make_shared<RecordingDisplayBufferCompositorFactory>();
    mc::MultiThreadedCompositor compositor{display, scene, db_compositor_factory, null_report, false};

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

    auto display = std::make_shared<StubDisplay>(nbuffers);
    auto scene = std::make_shared<StubScene>();
    auto db_compositor_factory = std::make_shared<RecordingDisplayBufferCompositorFactory>();
    mc::MultiThreadedCompositor compositor{display, scene, db_compositor_factory, null_report, false};

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

    auto display = std::make_shared<StubDisplay>(nbuffers);
    auto scene = std::make_shared<StubScene>();
    auto db_compositor_factory = std::make_shared<SurfaceUpdatingDisplayBufferCompositorFactory>(scene);
    mc::MultiThreadedCompositor compositor{display, scene, db_compositor_factory, null_report, true};

    compositor.start();

    while (!db_compositor_factory->enough_renders_happened())
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    compositor.stop();
}

TEST(MultiThreadedCompositor, makes_and_releases_display_buffer_current_target)
{
    using namespace testing;

    unsigned int const nbuffers{3};

    auto display = std::make_shared<StubDisplayWithMockBuffers>(nbuffers);
    auto scene = std::make_shared<StubScene>();
    auto db_compositor_factory = std::make_shared<NullDisplayBufferCompositorFactory>();
    mc::MultiThreadedCompositor compositor{display, scene, db_compositor_factory, null_report, true};

    display->for_each_mock_buffer([](mtd::MockDisplayBuffer& mock_buf)
    {
        EXPECT_CALL(mock_buf, view_area())
            .WillOnce(Return(geom::Rectangle()));
        EXPECT_CALL(mock_buf, make_current()).Times(1);
        EXPECT_CALL(mock_buf, release_current()).Times(1);
    });

    compositor.start();
    compositor.stop();
}

TEST(MultiThreadedCompositor, double_start_or_stop_ignored)
{
    using namespace testing;

    unsigned int const nbuffers{3};
    auto display = std::make_shared<StubDisplayWithMockBuffers>(nbuffers);
    auto mock_scene = std::make_shared<mtd::MockScene>();
    auto db_compositor_factory = std::make_shared<NullDisplayBufferCompositorFactory>();
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

    mc::MultiThreadedCompositor compositor{display, mock_scene, db_compositor_factory, mock_report, true};

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
    mc::MultiThreadedCompositor compositor{display, scene, db_compositor_factory, null_report, true};

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
    auto time_out = std::chrono::steady_clock::now() + std::chrono::seconds(1);
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

TEST(MultiThreadedCompositor, names_compositor_threads)
{
    using namespace testing;

    unsigned int const nbuffers{3};

    auto display = std::make_shared<StubDisplayWithMockBuffers>(nbuffers);
    auto scene = std::make_shared<StubScene>();
    auto db_compositor_factory = std::make_shared<ThreadNameDisplayBufferCompositorFactory>();
    mc::MultiThreadedCompositor compositor{display, scene, db_compositor_factory, null_report, true};

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
    auto db_compositor_factory = std::make_shared<NullDisplayBufferCompositorFactory>();
    auto mock_report = std::make_shared<testing::NiceMock<mtd::MockCompositorReport>>();

    EXPECT_CALL(*mock_scene, register_compositor(_))
        .Times(nbuffers);
    EXPECT_CALL(*mock_scene, unregister_compositor(_))
        .Times(nbuffers);
    mc::MultiThreadedCompositor compositor{display, mock_scene, db_compositor_factory, mock_report, true};

    compositor.start();
    compositor.stop();
}
