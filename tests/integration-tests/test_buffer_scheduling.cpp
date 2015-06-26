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
struct BufferEntry
{
    mg::BufferID id;
    unsigned int age;
    bool operator==(BufferEntry const& b) const
    {
        return ((id == b.id) && (age == b.age));
    }
};

struct ProducerSystem
{
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

    void produce()
    {
        mg::Buffer* b = nullptr;
        {
            std::unique_lock<decltype(mutex)> lk(mutex);
            b = buffer;
            age++;
            entries.emplace_back(BufferEntry{b->id(), age});
            buffer->write(reinterpret_cast<unsigned char const*>(&age), sizeof(age));
        }
        stream.swap_buffers(b,
            std::bind(&BufferQueueProducer::buffer_ready, this, std::placeholders::_1));
    }

    std::vector<BufferEntry> production_log()
    {
        std::unique_lock<decltype(mutex)> lk(mutex);
        return entries;
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
        b->read([this, b](unsigned char const* p) {
            entries.emplace_back(BufferEntry{b->id(), *reinterpret_cast<unsigned int const*>(p)});
        });
    }

    std::vector<BufferEntry> consumption_log()
    {
        return entries;
    }
    mc::BufferStream& stream;
    std::vector<BufferEntry> entries;
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
};
struct WithAnyNumberOfBuffers : BufferScheduling {};
struct WithTwoOrMoreBuffers   : BufferScheduling {};
struct WithThreeOrMoreBuffers : BufferScheduling {};
struct WithOneBuffer : BufferScheduling {};
}

//TEST_F(BufferQueueTest, compositor_acquires_frames_in_order_for_synchronous_client)
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

//TEST_F(BufferQueueTest, framedropping_clients_never_block)
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

//TEST_F(BufferQueueTest, buffer_queue_of_one_is_supported)
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
//TEST_F(BufferQueueTest, clients_dont_recycle_startup_buffer)
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

//TEST_F(BufferQueueTest, clients_can_have_multiple_pending_completions)
TEST_P(WithAnyNumberOfBuffers, clients_can_acquire_multiple_buffers )
{
    
}

//TEST_F(BufferQueueTest, async_client_cycles_through_all_buffers)
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

//TEST_F(BufferQueueTest, compositor_can_acquire_and_release)
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


//TEST_F(BufferQueueTest, multiple_compositors_are_in_sync)
TEST_P(WithTwoOrMoreBuffers, multiple_consumers_are_in_sync)
{
    BufferQueueConsumer consumer1(stream); //ticks at 59hz
    BufferQueueConsumer consumer2(stream); //ticks at 60hz

    std::vector<ScheduleEntry> schedule = {
        {0_t,     {&producer}, {}},
        {1_t,     {},          {&consumer1}},
        {60_t,    {},          {&consumer1, &consumer2}},
        {119_t,   {},          {&consumer1}},
        {120_t,   {},          {&consumer2}},
        {130_t,   {&producer}, {}},
        {178_t,   {},          {&consumer1}},
        {180_t,   {},          {&consumer2}},
        {237_t,   {},          {&consumer1}},
        {240_t,   {},          {&consumer2}},
    };
    run_system(schedule);

    auto production_log = producer.production_log();
    auto consumption_log_1 = consumer1.consumption_log();
    auto consumption_log_2 = consumer2.consumption_log();
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

//TEST_F(BufferQueueTest, multiple_fast_compositors_are_in_sync)
TEST_P(WithThreeOrMoreBuffers, multiple_fast_compositors_are_in_sync)
{ 
    BufferQueueConsumer consumer1(stream); //ticks at 59hz
    BufferQueueConsumer consumer2(stream); //ticks at 60hz

    std::vector<ScheduleEntry> schedule = {
        {0_t,     {&producer}, {}},
        {1_t,     {&producer}, {}},
        {60_t,    {},          {&consumer1, &consumer2}},
        {61_t,    {},          {&consumer1, &consumer2}},
    };
    run_system(schedule);

    auto production_log = producer.production_log();
    auto consumption_log_1 = consumer1.consumption_log();
    auto consumption_log_2 = consumer2.consumption_log();
    EXPECT_THAT(consumption_log_1, Eq(production_log));
    EXPECT_THAT(consumption_log_2, Eq(production_log));
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
