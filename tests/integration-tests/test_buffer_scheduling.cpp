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
    virtual void consume() { consume_resource(); }
    virtual std::shared_ptr<mg::Buffer> consume_resource() = 0;

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

    mg::BufferID current_id()
    {
        if (buffer)
            return buffer->id();
        else
            return mg::BufferID{INT_MAX};
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

    void reset_log()
    {
        std::unique_lock<decltype(mutex)> lk(mutex);
        return entries.clear();
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

    std::shared_ptr<mg::Buffer> consume_resource() override
    {
        auto b = stream.lock_compositor_buffer(this);
        last_size_ = b->size();
        b->read([this, b](unsigned char const* p) {
            entries.emplace_back(BufferEntry{b->id(), *reinterpret_cast<unsigned int const*>(p), Access::unblocked});
        });
        return b;
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

size_t unique_ids_in(std::vector<BufferEntry> log)
{
    std::sort(log.begin(), log.end(),
        [](BufferEntry const& a, BufferEntry const& b) { return a.id < b.id; });
    auto it = std::unique(log.begin(), log.end(),
        [](BufferEntry const& a, BufferEntry const& b) { return a.id == b.id; } );
    return std::distance(log.begin(), it);
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
    auto tick = 0_t;
    std::vector<ScheduleEntry> schedule;
    for(auto i = 0; i < nbuffers; i++)
        schedule.emplace_back(ScheduleEntry{tick++, {&producer}, {&consumer}});
    run_system(schedule);

    auto production_log = producer.production_log();
    std::sort(production_log.begin(), production_log.end(),
        [](BufferEntry const& a, BufferEntry const& b) { return a.id.as_value() > b.id.as_value(); });
    auto it = std::unique(production_log.begin(), production_log.end());
    production_log.erase(it, production_log.end());
    EXPECT_THAT(production_log, SizeIs(nbuffers));
}

TEST_P(WithAnyNumberOfBuffers, compositor_can_always_get_a_buffer)
{
    std::vector<ScheduleEntry> schedule = {
        {0_t, {&producer}, {}},
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

TEST_P(WithTwoOrMoreBuffers, framedropping_clients_get_all_buffers_and_dont_block)
{
    queue.allow_framedropping(true);
    std::vector<ScheduleEntry> schedule;
    for (auto i = 0; i < nbuffers * 3; i++)
        schedule.emplace_back(ScheduleEntry{1_t, {&producer}, {}}); 
    run_system(schedule);

    auto production_log = producer.production_log();
    std::sort(production_log.begin(), production_log.end(),
        [](BufferEntry const& a, BufferEntry const& b) { return a.id.as_value() > b.id.as_value(); });
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
    std::array<std::shared_ptr<BufferQueueConsumer>, nmonitors> consumers { {
        std::make_shared<BufferQueueConsumer>(stream),
        std::make_shared<BufferQueueConsumer>(stream),
        std::make_shared<BufferQueueConsumer>(stream)
    } };

    for (auto const& consumer : consumers)
        EXPECT_EQ(0, queue.buffers_ready_for_compositor(consumer.get()));

    producer.produce();

    for (auto const& consumer : consumers)
    {
        ASSERT_NE(0, queue.buffers_ready_for_compositor(consumer.get()));

        // Double consume to account for the +1 that
        // buffers_ready_for_compositor adds to do dynamic performance
        // detection.
        consumer->consume();
        consumer->consume();

        ASSERT_EQ(0, queue.buffers_ready_for_compositor(consumer.get()));
    }
}

TEST_P(WithTwoOrMoreBuffers, clients_get_new_buffers_on_compositor_release)
{   // Regression test for LP: #1480164
    mtd::MockFrameDroppingPolicyFactory policy_factory;
    mc::BufferQueue queue{nbuffers, mt::fake_shared(server_buffer_factory),
                          properties, policy_factory};
    queue.allow_framedropping(false);

    mg::Buffer* client_buffer = nullptr;
    auto callback = [&](mg::Buffer* buffer)
    {
        client_buffer = buffer;
    };

    auto client_try_acquire = [&]() -> bool
    {
        queue.client_acquire(callback);
        return client_buffer != nullptr;
    };

    auto client_release = [&]()
    {
        EXPECT_TRUE(client_buffer);
        queue.client_release(client_buffer);
        client_buffer = nullptr;
    };

    // Skip over the first frame. The early release optimization is too
    // conservative to allow it to happen right at the start (so as to
    // maintain correct multimonitor frame rates if required).
    ASSERT_TRUE(client_try_acquire());
    client_release();
    queue.compositor_release(queue.compositor_acquire(this));

    auto onscreen = queue.compositor_acquire(this);

    bool blocking;
    do
    {
        blocking = !client_try_acquire();
        if (!blocking)
            client_release();
    } while (!blocking);

    for (int f = 0; f < 100; ++f)
    {
        ASSERT_FALSE(client_buffer);
        queue.compositor_release(onscreen);
        ASSERT_TRUE(client_buffer) << "frame# " << f;
        client_release();
        onscreen = queue.compositor_acquire(this);
        client_try_acquire();
    }
}

TEST_P(WithTwoOrMoreBuffers, short_buffer_holds_dont_overclock_multimonitor)
{   // Regression test related to LP: #1480164
    mtd::MockFrameDroppingPolicyFactory policy_factory;
    mc::BufferQueue queue{nbuffers, mt::fake_shared(server_buffer_factory),
                          properties, policy_factory};
    queue.allow_framedropping(false);

    const void* const leftid = "left";
    const void* const rightid = "right";

    mg::Buffer* client_buffer = nullptr;
    auto callback = [&](mg::Buffer* buffer)
    {
        client_buffer = buffer;
    };

    auto client_try_acquire = [&]() -> bool
    {
        queue.client_acquire(callback);
        return client_buffer != nullptr;
    };

    auto client_release = [&]()
    {
        EXPECT_TRUE(client_buffer);
        queue.client_release(client_buffer);
        client_buffer = nullptr;
    };

    // Skip over the first frame. The early release optimization is too
    // conservative to allow it to happen right at the start (so as to
    // maintain correct multimonitor frame rates if required).
    ASSERT_TRUE(client_try_acquire());
    client_release();
    queue.compositor_release(queue.compositor_acquire(leftid));

    auto left = queue.compositor_acquire(leftid);
    auto right = queue.compositor_acquire(rightid);

    bool blocking;
    do
    {
        blocking = !client_try_acquire();
        if (!blocking)
            client_release();
    } while (!blocking);

    for (int f = 0; f < 100; ++f)
    {
        ASSERT_FALSE(client_buffer);
        queue.compositor_release(left);
        queue.compositor_release(right);
        ASSERT_FALSE(client_buffer);
        left = queue.compositor_acquire(leftid);
        right = queue.compositor_acquire(rightid);
        ASSERT_TRUE(client_buffer);
        client_release();
        client_try_acquire();
    }
}

TEST_P(WithAnyNumberOfBuffers, compositor_inflates_ready_count_for_slow_clients)
{
    queue.set_scaling_delay(3);

    for (int frame = 0; frame < 10; frame++)
    {
        ASSERT_EQ(0, queue.buffers_ready_for_compositor(&consumer));
        producer.produce();

        // Detecting a slow client requires scheduling at least one extra
        // frame...
        int nready = queue.buffers_ready_for_compositor(&consumer);
        ASSERT_THAT(nready, Ge(2));
        for (int i = 0; i < nready; ++i)
            consumer.consume();
    }
}

TEST_P(WithAnyNumberOfBuffers, first_user_is_recorded)
{
    consumer.consume();
    EXPECT_TRUE(queue.is_a_current_buffer_user(&consumer));
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

TEST_P(WithTwoOrMoreBuffers, overlapping_compositors_get_different_frames)
{
    // This test simulates bypass behaviour
    // overlay/bypass code will need to acquire two buffers at once, as there's a brief period of time where a buffer 
    // is onscreen, and the compositor has to arrange for the next buffer to swap in.
    auto const num_simultaneous_consumptions = 2u;
    auto num_iterations = 20u;
    std::array<std::shared_ptr<mg::Buffer>, num_simultaneous_consumptions> compositor_resources;
    for (auto i = 0u; i < num_iterations; i++)
    {
        // One of the compositors (the oldest one) gets a new buffer...
        int oldest = i & 1;
        compositor_resources[oldest].reset();
        producer.produce();
        compositor_resources[oldest] = consumer.consume_resource();
    }

    // Two compositors acquired, and they're always different...
    auto log = consumer.consumption_log();
    for(auto i = 0u; i < log.size() - 1; i++)
        EXPECT_THAT(log[i].id, Ne(log[i+1].id));
}

// Regression test LP: #1241369 / LP: #1241371
// Test that a triple buffer or higher client can always provide a relatively up-to-date frame
// when its producing the buffer around the frame deadline
TEST_P(WithThreeOrMoreBuffers, slow_client_framerate_matches_compositor)
{
    // BufferQueue can only satify this for nbuffers >= 3
    // since a client can only own up to nbuffers - 1 at any one time
    auto const iterations = 10u;
    queue.allow_framedropping(false);

    //fill up queue at first
    for(auto i = 0; i < nbuffers - 1; i++)
        producer.produce();

    //a schedule that would block once per iteration for double buffering, but only once for >3 buffers
    std::vector<ScheduleEntry> schedule = {
        {0_t,  {}, {&consumer}},
        {59_t,  {&producer}, {}},
        {60_t,  {}, {&consumer}},
        {120_t,  {}, {&consumer}},
        {121_t,  {&producer}, {}},
        {179_t,  {&producer}, {}},
        {180_t,  {}, {&consumer}},
        {240_t,  {}, {&consumer}},
        {241_t,  {&producer}, {}},
        {300_t,  {}, {&consumer}},
    };

    auto count = 0u;
    repeat_system_until(schedule, [&]{ return count++ < schedule.size() * iterations; });

    auto log = producer.production_log();
    auto blockages = std::count_if(log.begin(), log.end(),
        [](BufferEntry const& e){ return e.blockage == Access::blocked; });
    EXPECT_THAT(blockages, Le(1));
}

//regression test for LP: #1396006, LP: #1379685
TEST_P(WithTwoOrMoreBuffers, framedropping_surface_never_drops_newest_frame)
{
    queue.allow_framedropping(true);

    for (int f = 0; f < nbuffers; ++f)
        producer.produce();

    for (int n = 0; n < nbuffers - 1; ++n)
        consumer.consume();

    // Ensure it's not the newest frame that gets dropped to satisfy the
    // client.
    producer.produce();
    consumer.consume();

    // The queue could solve this problem a few ways. It might choose to
    // defer framedropping till it's safe, or even allocate additional
    // buffers. We don't care which, just verify it's not losing the
    // latest frame. Because the screen could be indefinitely out of date
    // if that happens...
    auto producer_log = producer.production_log();
    auto consumer_log = consumer.consumption_log();
    EXPECT_TRUE(!producer.can_produce() || 
        (!producer_log.empty() && !consumer_log.empty() && producer_log.back() == consumer_log.back()));
}

/* Regression test for LP: #1306464 */
TEST_P(WithThreeBuffers, framedropping_client_acquire_does_not_block_when_no_available_buffers)
{
    queue.allow_framedropping(true);

    /* The client can never own this acquired buffer */
    auto comp_buffer = consumer.consume_resource();

    /* Let client release all possible buffers so they go into
     * the ready queue
     */
    for (int i = 0; i < nbuffers; ++i)
    {
        producer.produce();
        EXPECT_THAT(comp_buffer->id(), Ne(producer.current_id()));
    }

    /* Let the compositor acquire all ready buffers */
    for (int i = 0; i < nbuffers; ++i)
        consumer.consume();

    /* At this point the queue has 0 free buffers and 0 ready buffers
     * so the next client request should not be satisfied until
     * a compositor releases its buffers */
    /* ... unless the BufferQueue is overallocating. In that case it will
     * have succeeding in acquiring immediately.
     */
    EXPECT_TRUE(producer.can_produce());
}

TEST_P(WithTwoOrMoreBuffers, client_never_owns_compositor_buffers_and_vice_versa)
{
    for (int i = 0; i < 100; ++i)
    {
        auto buffer = consumer.consume_resource();
        producer.produce();
        EXPECT_THAT(buffer->id(), Ne(producer.current_id()));
    }
}

/* Regression test for an issue brought up at:
 * http://code.launchpad.net/~albaguirre/mir/
 * alternative-switching-bundle-implementation/+merge/216606/comments/517048
 */
TEST_P(WithThreeOrMoreBuffers, buffers_are_not_lost)
{
    // This test is technically not valid with dynamic queue scaling on
    // BufferQueue specific setup
    queue.set_scaling_delay(-1);

    const int nmonitors = 2;
    std::array<std::shared_ptr<BufferQueueConsumer>, nmonitors> consumers { {
        std::make_shared<BufferQueueConsumer>(stream),
        std::make_shared<BufferQueueConsumer>(stream)
    } };

    /* Hold a reference to current compositor buffer*/
    auto comp_buffer1 = consumers[0]->consume_resource();

    while (producer.can_produce())
        producer.produce();

    /* Have a second compositor advance the current compositor buffer at least twice */
    for (int acquires = 0; acquires < nbuffers; ++acquires)
        consumers[1]->consume();

    comp_buffer1.reset();

    /* An async client should still be able to cycle through all the available
     * buffers. "async" means any pattern other than "produce,consume,..."
     */
    int const max_ownable_buffers = nbuffers - 1;
    producer.reset_log();
    for (int frame = 0; frame < max_ownable_buffers * 2; frame++)
    {
        for (int drain = 0; drain < nbuffers; ++drain)
            consumers[0]->consume();
        while (producer.can_produce())
            producer.produce();
    }

    EXPECT_THAT(unique_ids_in(producer.production_log()), Eq(nbuffers));
}

// Test that dynamic queue scaling/throttling actually works
TEST_P(WithThreeOrMoreBuffers, queue_size_scales_with_client_performance)
{
    //BufferQueue specific for now
    int const discard = 3;
    queue.set_scaling_delay(discard);

    for (int frame = 0; frame < 20; frame++)
    {
        producer.produce();
        consumer.consume();
    }
    // Expect double-buffers as the steady state for fast clients
    auto log = producer.production_log();
    ASSERT_THAT(log.size(), Gt(discard));  // avoid the below erase crashing
    log.erase(log.begin(), log.begin() + discard);
    EXPECT_THAT(unique_ids_in(log), Eq(2));
    producer.reset_log();

    // Now check what happens if the client becomes slow...
    for (int frame = 0; frame < 20; frame++)
    {
        producer.produce();
        consumer.consume();
        consumer.consume();
    }

    log = producer.production_log();
    log.erase(log.begin(), log.begin() + discard);
    EXPECT_THAT(unique_ids_in(log), Ge(3));
    producer.reset_log();

    // And what happens if the client becomes fast again?...
    for (int frame = 0; frame < 20; frame++)
    {
        producer.produce();
        consumer.consume();
    }
    // Expect double-buffers as the steady state for fast clients
    log = producer.production_log();
    log.erase(log.begin(), log.begin() + discard);
    EXPECT_THAT(unique_ids_in(log), Eq(2));
}

//NOTE: compositors need 2 buffers in overlay/bypass cases, as they 
//briefly need to arrange the next buffer while the previous one is still held onscreen
TEST_P(WithThreeOrMoreBuffers, greedy_compositors_scale_to_triple_buffers)
{
    /*
     * "Greedy" compositors means those that can hold multiple buffers from
     * the same client simultaneously or a single buffer for a long time.
     * This usually means bypass/overlays, but can also mean multi-monitor.
     */
    queue.allow_framedropping(false);

    for (auto i = 0u; i < 20u; i++)
    {
        auto first = consumer.consume_resource();
        auto second = consumer.consume_resource();
        producer.produce();
    }

    EXPECT_THAT(unique_ids_in(producer.production_log()), Eq(3));
}

MATCHER_P(BufferIdIs, val, "")
{
    if (!arg) return false;
    return arg->id() == val;
}

TEST_P(WithAnyNumberOfBuffers, can_snapshot_repeatedly_without_blocking)
{
    producer.produce();
    consumer.consume();
    auto const num_snapshots = nbuffers * 2u;
    std::vector<std::shared_ptr<mg::Buffer>> snaps(num_snapshots);
    for(auto i = 0u; i < num_snapshots; i++)
    {
        snaps[i] = queue.snapshot_acquire();
        queue.snapshot_release(snaps[i]);
    }

    auto production_log = producer.production_log();
    ASSERT_THAT(production_log, SizeIs(1));
    EXPECT_THAT(snaps, Each(BufferIdIs(production_log.back().id)));
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
