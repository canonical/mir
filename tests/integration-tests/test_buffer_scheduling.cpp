/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "src/client/client_buffer_depository.h"
#include "src/server/compositor/buffer_queue.h"
#include "src/server/compositor/buffer_stream_surfaces.h"
#include "mir/test/doubles/mock_client_buffer_factory.h"
#include "mir/test/doubles/stub_buffer_allocator.h"
#include "mir/test/doubles/stub_frame_dropping_policy_factory.h"
#include "mir/test/doubles/mock_frame_dropping_policy_factory.h"
#include "mir/test/fake_shared.h"
#include <gtest/gtest.h>

namespace mt  = mir::test;
namespace mtd = mir::test::doubles;
namespace mcl = mir::client;
namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace geom = mir::geometry;
using namespace testing;

namespace
{
enum class Access
{
    blocked,
    unblocked
};

struct BufferEntry
{
    mg::BufferID id;
    unsigned int age;
    Access blockage;
    bool operator==(BufferEntry const& b) const
    {
        return ((blockage == b.blockage) && (id == b.id) && (age == b.age));
    }
};

struct ProducerSystem
{
    virtual bool can_produce() = 0;
    virtual void produce() = 0;
    virtual std::vector<BufferEntry> production_log() = 0;
    virtual ~ProducerSystem() = default;
    ProducerSystem() = default;
    ProducerSystem(ProducerSystem const&) = delete;
    ProducerSystem& operator=(ProducerSystem const&) = delete;
};

struct ConsumerSystem
{
    virtual void consume() = 0;
    virtual std::vector<BufferEntry> consumption_log() = 0;
    ConsumerSystem() = default;
    virtual ~ConsumerSystem() = default;
    ConsumerSystem(ConsumerSystem const&) = delete;
    ConsumerSystem& operator=(ConsumerSystem const&) = delete;
};

//buffer queue testing
struct BufferQueueProducer : ProducerSystem
{
    BufferQueueProducer(mc::BufferStream& stream) :
        stream(stream)
    {
        stream.swap_buffers(buffer,
            std::bind(&BufferQueueProducer::buffer_ready, this, std::placeholders::_1));
    }

    bool can_produce()
    {
        std::unique_lock<decltype(mutex)> lk(mutex);
        return buffer;
    }

    void produce()
    {
        mg::Buffer* b = nullptr;
        if (can_produce())
        {
            {
                std::unique_lock<decltype(mutex)> lk(mutex);
                b = buffer;
                buffer = nullptr;
                age++;
                entries.emplace_back(BufferEntry{b->id(), age, Access::unblocked});
                b->write(reinterpret_cast<unsigned char const*>(&age), sizeof(age));
            }
            stream.swap_buffers(b,
                std::bind(&BufferQueueProducer::buffer_ready, this, std::placeholders::_1));
        }
        else
        {
            entries.emplace_back(BufferEntry{mg::BufferID{2}, 0u, Access::blocked});
        }
    }

    std::vector<BufferEntry> production_log()
    {
        std::unique_lock<decltype(mutex)> lk(mutex);
        return entries;
    }

    geom::Size last_size()
    {
        if (buffer)
            return buffer->size();
        return geom::Size{};
    }
private:
    mc::BufferStream& stream;
    void buffer_ready(mg::Buffer* b)
    {
        std::unique_lock<decltype(mutex)> lk(mutex);
        buffer = b;
    }
    std::mutex mutex;
    unsigned int age {0};
    std::vector<BufferEntry> entries;
    mg::Buffer* buffer {nullptr};
};

struct BufferQueueConsumer : ConsumerSystem
{
    BufferQueueConsumer(mc::BufferStream& stream) :
        stream(stream)
    {
    }

    void consume()
    {
        auto b = stream.lock_compositor_buffer(this);
        last_size_ = b->size();
        b->read([this, b](unsigned char const* p) {
            entries.emplace_back(BufferEntry{b->id(), *reinterpret_cast<unsigned int const*>(p), Access::unblocked});
        });
    }

    std::vector<BufferEntry> consumption_log()
    {
        return entries;
    }

    geom::Size last_size()
    {
        return last_size_;
    }
    
    mc::BufferStream& stream;
    std::vector<BufferEntry> entries;
    geom::Size last_size_;
};

//schedule helpers
using tick = std::chrono::duration<int, std::ratio<27182818, 31415926>>;
constexpr tick operator ""_t(unsigned long long t)
{
    return tick(t);
}
struct ScheduleEntry
{
    tick timestamp;
    std::vector<ProducerSystem*> producers;
    std::vector<ConsumerSystem*> consumers;
};

void run_system(std::vector<ScheduleEntry>& schedule)
{
    std::sort(schedule.begin(), schedule.end(),
        [](ScheduleEntry& a, ScheduleEntry& b)
        {
            return a.timestamp < b.timestamp;
        });

    for(auto const& entry : schedule)
    {
        for(auto const& p : entry.producers)
            p->produce();
        for(auto const& c : entry.consumers)
            c->consume();
    }
}

void repeat_system_until(
    std::vector<ScheduleEntry>& schedule,
    std::function<bool()> const& predicate)
{
    std::sort(schedule.begin(), schedule.end(),
        [](ScheduleEntry& a, ScheduleEntry& b)
        {
            return a.timestamp < b.timestamp;
        });

    auto entry_it = schedule.begin();
    if (entry_it == schedule.end()) return;
    while(predicate())
    {
        for(auto const& p : entry_it->producers)
            p->produce();
        for(auto const& c : entry_it->consumers)
            c->consume();
        entry_it++;
        if (entry_it == schedule.end()) entry_it = schedule.begin();
    }
}


//test infrastructure
struct BufferScheduling : public Test, ::testing::WithParamInterface<int>
{
    mtd::MockClientBufferFactory client_buffer_factory;
    mtd::StubBufferAllocator server_buffer_factory;
    mtd::StubFrameDroppingPolicyFactory stub_policy;
    mg::BufferProperties properties{geom::Size{3,3}, mir_pixel_format_abgr_8888, mg::BufferUsage::hardware};
    int nbuffers = GetParam();
    mcl::ClientBufferDepository depository{mt::fake_shared(client_buffer_factory), nbuffers};
    mc::BufferQueue queue{nbuffers, mt::fake_shared(server_buffer_factory), properties, stub_policy};
    mc::BufferStreamSurfaces stream{mt::fake_shared(queue)};
    BufferQueueProducer producer{stream};
    BufferQueueConsumer consumer{stream};
    BufferQueueConsumer second_consumer{stream};
};
struct WithAnyNumberOfBuffers : BufferScheduling {};
struct WithTwoOrMoreBuffers   : BufferScheduling {};
struct WithThreeOrMoreBuffers : BufferScheduling {};
struct WithOneBuffer : BufferScheduling {};
struct WithTwoBuffers : BufferScheduling {};
struct WithThreeBuffers : BufferScheduling {};
}

/* Regression test for LP#1270964 */
TEST_P(WithAnyNumberOfBuffers, all_buffers_consumed_in_interleaving_pattern)
{
    std::vector<ScheduleEntry> schedule = {
        {1_t,   {&producer}, {}},
        {60_t,           {}, {&consumer}},
        {61_t,  {&producer}, {}},
        {120_t,          {}, {&consumer}},
        {121_t, {&producer}, {}},
        {180_t,          {}, {&consumer}},
    };
    run_system(schedule);

    auto production_log = producer.production_log();
    auto consumption_log = consumer.consumption_log();
    EXPECT_THAT(production_log, Not(IsEmpty()));
    EXPECT_THAT(consumption_log, Not(IsEmpty()));
    EXPECT_THAT(consumption_log, ContainerEq(production_log));
}

TEST_P(WithTwoOrMoreBuffers, framedropping_producers_dont_block)
{
    queue.allow_framedropping(true);
    std::vector<ScheduleEntry> schedule = {
        {0_t,  {&producer}, {}},
        {61_t, {&producer}, {}},
        {62_t, {&producer}, {}},
        {63_t, {&producer}, {}},
        {64_t, {&producer}, {}},
        {90_t,          {}, {&consumer}},
        {91_t, {&producer}, {}},
        {92_t, {&producer}, {}},
        {93_t, {&producer}, {}},
        {94_t, {&producer}, {}},
        {120_t,         {}, {&consumer}},
    };
    run_system(schedule);
    auto production_log = producer.production_log();
    auto consumption_log = consumer.consumption_log();
    ASSERT_THAT(production_log, SizeIs(9));
    EXPECT_THAT(consumption_log, SizeIs(2));
}

TEST_P(WithThreeOrMoreBuffers, synchronous_overproducing_producers_has_all_buffers_consumed)
{
    std::vector<ScheduleEntry> schedule = {
        {1_t,   {&producer}, {}},
        {60_t,           {}, {&consumer}},
        {61_t,  {&producer}, {}},
        {62_t,  {&producer}, {}},
        {120_t,          {}, {&consumer}},
        {180_t,          {}, {&consumer}},
    };
    run_system(schedule);

    auto production_log = producer.production_log();
    auto consumption_log = consumer.consumption_log();
    EXPECT_THAT(production_log, Not(IsEmpty()));
    EXPECT_THAT(consumption_log, Not(IsEmpty()));
    EXPECT_THAT(consumption_log, ContainerEq(production_log));
}


MATCHER_P(EachBufferIdIs, value, "")
{
    auto id_matcher = [](BufferEntry const& a, BufferEntry const& b){ return a.id == b.id; };
    return std::search_n(arg.begin(), arg.end(), arg.size(), value, id_matcher) != std::end(arg);
}

MATCHER(HasIncreasingAge, "")
{
    return std::is_sorted(arg.begin(), arg.end(),
        [](BufferEntry const& a, BufferEntry const& b) {
            return a.age < b.age;
        });
}

TEST_P(WithOneBuffer, client_and_server_get_concurrent_access)
{
    std::vector<ScheduleEntry> schedule = {
        {1_t, {&producer}, {&consumer}},
        {2_t, {&producer}, {&consumer}},
        {3_t, {&producer}, {}},
        {4_t,          {}, {&consumer}},
    };
    run_system(schedule);

    auto production_log = producer.production_log();
    auto consumption_log = consumer.consumption_log();
    EXPECT_THAT(production_log, Not(IsEmpty()));
    EXPECT_THAT(consumption_log, Not(IsEmpty()));
    EXPECT_THAT(consumption_log, ContainerEq(production_log));

    EXPECT_THAT(consumption_log, EachBufferIdIs(consumption_log[0]));
    EXPECT_THAT(production_log, EachBufferIdIs(consumption_log[0]));
    EXPECT_THAT(consumption_log, HasIncreasingAge());
} 


/* Regression test for LP: #1210042 */
TEST_P(WithThreeOrMoreBuffers, consumers_dont_recycle_startup_buffer )
{
    std::vector<ScheduleEntry> schedule = {
        {1_t, {&producer}, {}},
        {2_t, {&producer}, {}},
        {3_t,          {}, {&consumer}},
    };
    run_system(schedule);

    auto production_log = producer.production_log();
    auto consumption_log = consumer.consumption_log();
    ASSERT_THAT(production_log, SizeIs(2)); 
    ASSERT_THAT(consumption_log, SizeIs(1));
    EXPECT_THAT(consumption_log[0], Eq(production_log[0])); 
}

TEST_P(WithTwoOrMoreBuffers, consumer_cycles_through_all_available_buffers)
{
    auto acquirable_buffers = nbuffers - 1;
    auto tick = 0_t;
    std::vector<ScheduleEntry> schedule;
    for(auto i = 0; i < acquirable_buffers; i++)
        schedule.emplace_back(ScheduleEntry{tick++, {&producer}, {}});
    run_system(schedule);

    auto production_log = producer.production_log();
    EXPECT_THAT(production_log, SizeIs(acquirable_buffers));
}

TEST_P(WithAnyNumberOfBuffers, compositor_can_always_get_a_buffer)
{
    std::vector<ScheduleEntry> schedule = {
        {1_t, {},          {&consumer}},
        {2_t, {},          {&consumer}},
        {3_t, {},          {&consumer}},
        {5_t, {},          {&consumer}},
        {6_t, {},          {&consumer}},
    };
    run_system(schedule);

    auto consumption_log = consumer.consumption_log();
    ASSERT_THAT(consumption_log, SizeIs(5));
}

TEST_P(WithTwoOrMoreBuffers, compositor_doesnt_starve_from_slow_client)
{
    std::vector<ScheduleEntry> schedule = {
        {1_t,   {},          {&consumer}},
        {60_t,  {},          {&consumer}},
        {120_t, {},          {&consumer}},
        {150_t, {&producer}, {}},
        {180_t, {},          {&consumer}},
        {240_t, {},          {&consumer}},
        {270_t, {&producer}, {}},
        {300_t, {},          {&consumer}},
        {360_t, {},          {&consumer}},
    };
    run_system(schedule);

    auto consumption_log = consumer.consumption_log();
    ASSERT_THAT(consumption_log, SizeIs(7));
    EXPECT_THAT(std::count(consumption_log.begin(), consumption_log.end(), consumption_log[0]), Eq(3));
    EXPECT_THAT(std::count(consumption_log.begin(), consumption_log.end(), consumption_log[3]), Eq(2));
    EXPECT_THAT(std::count(consumption_log.begin(), consumption_log.end(), consumption_log[5]), Eq(2));
}

TEST_P(WithTwoOrMoreBuffers, multiple_consumers_are_in_sync)
{
    std::vector<ScheduleEntry> schedule = {
        {0_t,     {&producer}, {}},
        {1_t,     {},          {&consumer}},
        {60_t,    {},          {&consumer, &second_consumer}},
        {119_t,   {},          {&consumer}},
        {120_t,   {},          {&second_consumer}},
        {130_t,   {&producer}, {}},
        {178_t,   {},          {&consumer}},
        {180_t,   {},          {&second_consumer}},
        {237_t,   {},          {&consumer}},
        {240_t,   {},          {&second_consumer}},
    };
    run_system(schedule);

    auto production_log = producer.production_log();
    auto consumption_log_1 = consumer.consumption_log();
    auto consumption_log_2 = second_consumer.consumption_log();
    ASSERT_THAT(consumption_log_1, SizeIs(5));
    ASSERT_THAT(consumption_log_2, SizeIs(4));
    ASSERT_THAT(production_log, SizeIs(2));

    std::for_each(consumption_log_1.begin(), consumption_log_1.begin() + 3,
        [&](BufferEntry const& entry){ EXPECT_THAT(entry, Eq(production_log[0])); });
    std::for_each(consumption_log_1.begin() + 3, consumption_log_1.end(),
        [&](BufferEntry const& entry){ EXPECT_THAT(entry, Eq(production_log[1])); });
    std::for_each(consumption_log_2.begin(), consumption_log_2.begin() + 2,
        [&](BufferEntry const& entry){ EXPECT_THAT(entry, Eq(production_log[0])); });
    std::for_each(consumption_log_2.begin() + 2, consumption_log_2.end(),
        [&](BufferEntry const& entry){ EXPECT_THAT(entry, Eq(production_log[1])); });
}

TEST_P(WithThreeOrMoreBuffers, multiple_fast_compositors_are_in_sync)
{ 
    std::vector<ScheduleEntry> schedule = {
        {0_t,     {&producer}, {}},
        {1_t,     {&producer}, {}},
        {60_t,    {},          {&consumer, &second_consumer}},
        {61_t,    {},          {&consumer, &second_consumer}},
    };
    run_system(schedule);

    auto production_log = producer.production_log();
    auto consumption_log_1 = consumer.consumption_log();
    auto consumption_log_2 = second_consumer.consumption_log();
    EXPECT_THAT(consumption_log_1, Eq(production_log));
    EXPECT_THAT(consumption_log_2, Eq(production_log));
}

TEST_P(WithTwoOrMoreBuffers, framedropping_clients_get_all_buffers)
{
    queue.allow_framedropping(true);
    std::vector<ScheduleEntry> schedule = {
        {2_t,  {&producer}, {}},
        {4_t,  {&producer}, {}},
        {6_t,  {&producer}, {}},
        {8_t,  {&producer}, {}},
    };
    run_system(schedule);
    auto production_log = producer.production_log();

    auto last = std::unique(production_log.begin(), production_log.end(),
        [](BufferEntry const& a, BufferEntry const& b) { return a.id == b.id; });
    production_log.erase(last, production_log.end());

    EXPECT_THAT(production_log.size(), Ge(nbuffers)); //Ge is to accommodate overallocation
}

TEST_P(WithTwoOrMoreBuffers, nonframedropping_client_throttles_to_compositor_rate)
{
    unsigned int reps = 50;
    auto const expected_blocks = reps - nbuffers;
    std::vector<ScheduleEntry> schedule = {
        {1_t,  {&producer, &producer}, {&consumer}},
    };
    repeat_system_until(schedule, [&reps]{ return --reps != 0; });

    auto log = producer.production_log();
    auto block_count = std::count_if(log.begin(), log.end(),
        [](BufferEntry const& e) { return e.blockage == Access::blocked; });
    EXPECT_THAT(block_count, Ge(expected_blocks));
}

TEST_P(WithAnyNumberOfBuffers, resize_affects_client_acquires_immediately)
{
    unsigned int const sizes_to_test{4};
    geom::Size new_size = properties.size;
    for(auto i = 0u; i < sizes_to_test; i++)
    {
        new_size = new_size * 2;
        queue.resize(new_size);

        std::vector<ScheduleEntry> schedule = {{1_t,  {&producer}, {&consumer}}};
        run_system(schedule);
        EXPECT_THAT(producer.last_size(), Eq(new_size));
    }
}

TEST_P(WithAnyNumberOfBuffers, compositor_acquires_resized_frames)
{
    unsigned int const sizes_to_test{4};
    int const attempt_limit{100};
    geom::Size new_size = properties.size;
    for(auto i = 0u; i < sizes_to_test; i++)
    {
        new_size = new_size * 2;
        queue.resize(new_size);

        std::vector<ScheduleEntry> schedule = {
            {1_t,  {&producer}, {}},
            {2_t,  {}, {&consumer}},
            {3_t,  {&producer}, {}},
        };
        run_system(schedule);

        int attempt_count = 0;
        schedule = {{2_t,  {}, {&consumer}}};
        repeat_system_until(schedule, [&] {
            return (consumer.last_size() != new_size) && (attempt_count++ < attempt_limit); });

        ASSERT_THAT(attempt_count, Lt(attempt_limit)) << "consumer never got the new size";
    }
}

// Regression test for LP: #1396006
TEST_P(WithTwoOrMoreBuffers, framedropping_policy_never_drops_newest_frame)
{
    mtd::MockFrameDroppingPolicyFactory policy_factory;
    mc::BufferQueue queue{nbuffers, mt::fake_shared(server_buffer_factory), properties, policy_factory};
    mc::BufferStreamSurfaces stream{mt::fake_shared(queue)};
    BufferQueueProducer producer(stream);

    for(auto i = 0; i < nbuffers; i++)
        producer.produce();
    policy_factory.trigger_policies();
    producer.produce();

    auto production_log = producer.production_log();
    ASSERT_THAT(production_log, SizeIs(nbuffers + 1));
    EXPECT_THAT(production_log[nbuffers], Not(Eq(production_log[nbuffers - 1]))); 
}

TEST_P(WithTwoOrMoreBuffers, uncomposited_client_swaps_when_policy_triggered)
{
    mtd::MockFrameDroppingPolicyFactory policy_factory;
    mc::BufferQueue queue{nbuffers, mt::fake_shared(server_buffer_factory), properties, policy_factory};
    mc::BufferStreamSurfaces stream{mt::fake_shared(queue)};
    BufferQueueProducer producer(stream);

    for(auto i = 0; i < nbuffers; i++)
        producer.produce();
    policy_factory.trigger_policies();
    producer.produce();

    auto production_log = producer.production_log();
    ASSERT_THAT(production_log, SizeIs(nbuffers + 1));
    EXPECT_THAT(production_log[nbuffers - 1].blockage, Eq(Access::blocked));
    EXPECT_THAT(production_log[nbuffers].blockage, Eq(Access::unblocked));
}

TEST_P(WithOneBuffer, with_single_buffer_compositor_acquires_resized_frames_eventually)
{
    unsigned int attempts = 100;
    geom::Size const new_size{123,456};
    queue.resize(new_size);

    std::vector<ScheduleEntry> schedule = {
        {1_t,  {&producer}, {&consumer}},
    };

    repeat_system_until(schedule, [&]{
        if (consumer.last_size() == new_size) return false;
        if (--attempts == 0) return false;
        return true;
    });
    EXPECT_THAT(attempts, Ne(0));
}

// Regression test for LP: #1319765
TEST_P(WithTwoBuffers, client_is_not_blocked_prematurely)
{
    producer.produce();

    auto a = queue.compositor_acquire(this);

    producer.produce();

    auto b = queue.compositor_acquire(this);

    ASSERT_NE(a.get(), b.get());

    queue.compositor_release(a);

    producer.produce();

    queue.compositor_release(b);

    /*
     * Update to the original test case; This additional compositor acquire
     * represents the fixing of LP: #1395581 in the compositor logic.
     */
    if (queue.buffers_ready_for_compositor(this))
        queue.compositor_release(queue.compositor_acquire(this));

    // With the fix, a buffer will be available instantaneously:
    EXPECT_TRUE(producer.can_produce());
}

// Extended regression test for LP: #1319765
TEST_P(WithTwoBuffers, composite_on_demand_never_deadlocks)
{
    for (int i = 0; i < 100; ++i)
    {
        producer.produce();

        auto a = queue.compositor_acquire(this);

        producer.produce();

        auto b = queue.compositor_acquire(this);
    
        ASSERT_NE(a.get(), b.get());
    
        queue.compositor_release(a);

        producer.produce();
    
        queue.compositor_release(b);

        /*
         * Update to the original test case; This additional compositor acquire
         * represents the fixing of LP: #1395581 in the compositor logic.
         */
        if (queue.buffers_ready_for_compositor(this))
            queue.compositor_release(queue.compositor_acquire(this));

        EXPECT_TRUE(producer.can_produce());

        consumer.consume();
        consumer.consume();
    }
}

// Regression test for LP: #1395581
TEST_P(WithTwoOrMoreBuffers, buffers_ready_is_not_underestimated)
{
    // Produce frame 1
    producer.produce();
    // Acquire frame 1
    auto a = queue.compositor_acquire(this);

    // Produce frame 2
    producer.produce();
    // Acquire frame 2
    auto b = queue.compositor_acquire(this);

    // Release frame 1
    queue.compositor_release(a);
    // Produce frame 3
    producer.produce();
    // Release frame 2
    queue.compositor_release(b);

    // Verify frame 3 is ready for the first compositor
    EXPECT_THAT(queue.buffers_ready_for_compositor(this), Ge(1));
    auto c = queue.compositor_acquire(this);

    // Verify frame 3 is ready for a second compositor
    int const other_compositor_id = 0;
    ASSERT_THAT(queue.buffers_ready_for_compositor(&other_compositor_id), Ge(1));

    queue.compositor_release(c);
}

TEST_P(WithTwoOrMoreBuffers, buffers_ready_eventually_reaches_zero)
{
    const int nmonitors = 3;
    int monitor[nmonitors];

    for (int m = 0; m < nmonitors; ++m)
        EXPECT_EQ(0, queue.buffers_ready_for_compositor(&monitor[m]));

    producer.produce();

    for (int m = 0; m < nmonitors; ++m)
    {
        ASSERT_NE(0, queue.buffers_ready_for_compositor(&monitor[m]));

        // Double consume to account for the +1 that
        // buffers_ready_for_compositor adds to do dynamic performance
        // detection.
        queue.compositor_release(queue.compositor_acquire(&monitor[m]));
        queue.compositor_release(queue.compositor_acquire(&monitor[m]));

        ASSERT_EQ(0, queue.buffers_ready_for_compositor(&monitor[m]));
    }
}

TEST_P(WithAnyNumberOfBuffers, compositor_inflates_ready_count_for_slow_clients)
{
    for (int frame = 0; frame < 10; frame++)
    {
        ASSERT_EQ(0, queue.buffers_ready_for_compositor(&consumer));
        producer.produce();

        // Detecting a slow client requires scheduling at least one extra
        // frame...
        int nready = queue.buffers_ready_for_compositor(&consumer);
        ASSERT_EQ(2, nready);
        for (int i = 0; i < nready; ++i)
            consumer.consume();
    }
}

TEST_P(WithAnyNumberOfBuffers, first_user_is_recorded)
{
    consumer.consume();
    EXPECT_TRUE(queue.is_a_current_buffer_user(this));
}

TEST_P(WithThreeBuffers, gives_compositor_a_valid_buffer_after_dropping_old_buffers_without_clients)
{
    queue.drop_old_buffers();
    consumer.consume();
    EXPECT_THAT(consumer.consumption_log(), SizeIs(1));
}

TEST_P(WithThreeBuffers, gives_new_compositor_the_newest_buffer_after_dropping_old_buffers)
{
    producer.produce();
    consumer.consume();
    producer.produce();
    queue.drop_old_buffers();

    void const* const new_compositor_id{&nbuffers};
    auto comp2 = queue.compositor_acquire(new_compositor_id);

    auto production_log = producer.production_log();
    auto consumption_log = consumer.consumption_log();
    ASSERT_THAT(production_log, SizeIs(2));
    ASSERT_THAT(consumption_log, SizeIs(1));

    EXPECT_THAT(production_log[0], Eq(consumption_log[0]));
    EXPECT_THAT(production_log[1].id, Eq(comp2->id()));
}

int const max_buffers_to_test{5};
INSTANTIATE_TEST_CASE_P(
    BufferScheduling,
    WithAnyNumberOfBuffers,
    Range(1, max_buffers_to_test));
INSTANTIATE_TEST_CASE_P(
    BufferScheduling,
    WithTwoOrMoreBuffers,
    Range(2, max_buffers_to_test));
INSTANTIATE_TEST_CASE_P(
    BufferScheduling,
    WithThreeOrMoreBuffers,
    Range(3, max_buffers_to_test));
INSTANTIATE_TEST_CASE_P(
    BufferScheduling,
    WithOneBuffer,
    Values(1));
INSTANTIATE_TEST_CASE_P(
    BufferScheduling,
    WithTwoBuffers,
    Values(2));
INSTANTIATE_TEST_CASE_P(
    BufferScheduling,
    WithThreeBuffers,
    Values(3));
