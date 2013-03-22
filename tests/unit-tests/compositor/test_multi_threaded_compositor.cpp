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

private:
    std::vector<mtd::NullDisplayBuffer> buffers;
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

        for (auto& e : records)
        {
            Record& r = e.second;
            if (r.first < min_record_count)
                return false;
        }

        return true;
    }

    bool each_buffer_rendered_in_single_thread()
    {
        for (auto& e : records)
        {
            Record& r = e.second;
            if (r.second.size() != 1)
                return false;
        }

        return true;
    }

    bool buffers_rendered_in_different_threads()
    {
        std::unordered_set<std::thread::id> seen;
        seen.insert(std::this_thread::get_id());

        for (auto& e : records)
        {
            Record& r = e.second;
            auto iter = r.second.begin();
            if (seen.find(*iter) != seen.end())
                return false;
            seen.insert(*iter);
        }

        return true;
    }

private:
    std::mutex m;
    typedef std::pair<unsigned int, std::unordered_set<std::thread::id>> Record;
    std::unordered_map<mg::DisplayBuffer*,Record> records;
};

}

TEST(MultiThreadedCompositor, compositing_happens_in_different_threads)
{
    using namespace testing;

    unsigned int const nbuffers{3};

    auto display = std::make_shared<StubDisplay>(nbuffers);
    auto strategy = std::make_shared<RecordingCompositingStrategy>();
    mc::MultiThreadedCompositor compositor{display, strategy};

    compositor.start();

    while (!strategy->enough_records_gathered(nbuffers))
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    compositor.stop();

    EXPECT_TRUE(strategy->each_buffer_rendered_in_single_thread());
    EXPECT_TRUE(strategy->buffers_rendered_in_different_threads());
}
