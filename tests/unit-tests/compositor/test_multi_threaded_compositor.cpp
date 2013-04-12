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

#include "mir/compositor/multi_threaded_compositor.h"
#include "mir/compositor/compositing_strategy.h"
#include "mir/compositor/renderables.h"
#include "mir/graphics/display.h"
#include "mir_test_doubles/null_display_buffer.h"

#include <unordered_map>
#include <unordered_set>
#include <thread>
#include <mutex>
#include <chrono>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;

namespace
{

class StubDisplay : public mg::Display
{
 public:
    StubDisplay(unsigned int nbuffers) : buffers{nbuffers} {}
    geom::Rectangle view_area() const { return geom::Rectangle(); }
    void for_each_display_buffer(std::function<void(mg::DisplayBuffer&)> const& f)
    {
        for (auto& db : buffers)
            f(db);
    }
    std::shared_ptr<mg::DisplayConfiguration> configuration()
    {
        return std::shared_ptr<mg::DisplayConfiguration>();
    }
    void register_pause_resume_handlers(mir::MainLoop&,
                                        std::function<void()> const&,
                                        std::function<void()> const&)
    {
    }
    void pause() {}
    void resume() {}

private:
    std::vector<mtd::NullDisplayBuffer> buffers;
};

class StubRenderables : public mc::Renderables
{
public:
    StubRenderables() : callback{[]{}} {}

    void for_each_if(mc::FilterForRenderables&, mc::OperatorForRenderables&)
    {
    }

    void set_change_callback(std::function<void()> const& f)
    {
        std::lock_guard<std::mutex> lock{callback_mutex};
        assert(f);
        callback = f;
    }

    void emit_change_event()
    {
        {
            std::lock_guard<std::mutex> lock{callback_mutex};
            callback();
        }
        /* Reduce run-time under valgrind */
        std::this_thread::yield();
    }

private:
    std::function<void()> callback;
    std::mutex callback_mutex;
};

class RecordingCompositingStrategy : public mc::CompositingStrategy
{
public:
    void render(mg::DisplayBuffer& buffer)
    {
        {
            std::lock_guard<std::mutex> lk{m};

            if (records.find(&buffer) == records.end())
                records[&buffer] = Record(0, std::unordered_set<std::thread::id>());

            ++records[&buffer].first;
            records[&buffer].second.insert(std::this_thread::get_id());
        }

        /* Reduce run-time under valgrind */
        std::this_thread::yield();
    }

    bool enough_records_gathered(unsigned int nbuffers)
    {
        static unsigned int const min_record_count{1000};

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
            std::function<bool(unsigned int)> const& check)
    {
        std::lock_guard<std::mutex> lk{m};

        if (records.size() < nbuffers)
            return false;

        for (auto const& e : records)
        {
            Record const& r = e.second;
            if (!check(r.first))
                return false;
        }

        return true;
    }

private:
    std::mutex m;
    typedef std::pair<unsigned int, std::unordered_set<std::thread::id>> Record;
    std::unordered_map<mg::DisplayBuffer*,Record> records;
};

class SurfaceUpdatingCompositingStrategy : public mc::CompositingStrategy
{
public:
    SurfaceUpdatingCompositingStrategy(std::shared_ptr<StubRenderables> const& renderables)
        : renderables{renderables},
          render_count{0}
    {
    }

    void render(mg::DisplayBuffer&)
    {
        renderables->emit_change_event();
        ++render_count;
        /* Reduce run-time under valgrind */
        std::this_thread::yield();
    }

    bool enough_renders_happened()
    {
        unsigned int const enough_renders{1000};
        return render_count > enough_renders;
    }

private:
    std::shared_ptr<StubRenderables> const renderables;
    unsigned int render_count;
};

}

TEST(MultiThreadedCompositor, compositing_happens_in_different_threads)
{
    using namespace testing;

    unsigned int const nbuffers{3};

    auto display = std::make_shared<StubDisplay>(nbuffers);
    auto renderables = std::make_shared<StubRenderables>();
    auto strategy = std::make_shared<RecordingCompositingStrategy>();
    mc::MultiThreadedCompositor compositor{display, renderables, strategy};

    compositor.start();

    while (!strategy->enough_records_gathered(nbuffers))
        renderables->emit_change_event();

    compositor.stop();

    EXPECT_TRUE(strategy->each_buffer_rendered_in_single_thread());
    EXPECT_TRUE(strategy->buffers_rendered_in_different_threads());
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

    unsigned int const nbuffers{3};

    auto display = std::make_shared<StubDisplay>(nbuffers);
    auto renderables = std::make_shared<StubRenderables>();
    auto strategy = std::make_shared<RecordingCompositingStrategy>();
    mc::MultiThreadedCompositor compositor{display, renderables, strategy};

    auto at_least_one = [](unsigned int n){return n >= 1;};
    auto at_least_two = [](unsigned int n){return n >= 2;};
    auto exactly_two = [](unsigned int n){return n == 2;};

    compositor.start();

    /* Wait until buffers have been rendered to once (initial render) */
    while (!strategy->check_record_count_for_each_buffer(nbuffers, at_least_one))
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    renderables->emit_change_event();

    /* Wait until buffers have been rendered to twice (initial render + change) */
    while (!strategy->check_record_count_for_each_buffer(nbuffers, at_least_two))
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    /* Give some more time for other renders (if any) to happen */
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    compositor.stop();

    /* Only two renders should have happened */
    EXPECT_TRUE(strategy->check_record_count_for_each_buffer(nbuffers, exactly_two));
}

TEST(MultiThreadedCompositor, surface_update_from_render_doesnt_deadlock)
{
    using namespace testing;

    unsigned int const nbuffers{3};

    auto display = std::make_shared<StubDisplay>(nbuffers);
    auto renderables = std::make_shared<StubRenderables>();
    auto strategy = std::make_shared<SurfaceUpdatingCompositingStrategy>(renderables);
    mc::MultiThreadedCompositor compositor{display, renderables, strategy};

    compositor.start();

    while (!strategy->enough_renders_happened())
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    compositor.stop();
}
