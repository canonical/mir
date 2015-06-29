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
        {
            std::unique_lock<decltype(mutex)> lk(mutex);
            b = buffer;
            buffer = nullptr;
            age++;
            entries.emplace_back(BufferEntry{b->id(), age});
            b->write(reinterpret_cast<unsigned char const*>(&age), sizeof(age));
        }
        stream.swap_buffers(b,
            std::bind(&BufferQueueProducer::buffer_ready, this, std::placeholders::_1));
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
    bool has_buffer;
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
            entries.emplace_back(BufferEntry{b->id(), *reinterpret_cast<unsigned int const*>(p)});
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

/* Regression test for LP#1270964 */
//TEST_F(BufferQueueWithThreeBuffers, compositor_client_interleaved)
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
//TEST_F(BufferQueueTest, compositor_acquire_never_blocks_when_there_are_no_ready_buffers)
//TEST_F(BufferQueueTest, compositor_can_always_acquire_buffer)
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

#if 0
TEST_P(WithTwoOrMoreBuffers, overlapping_compositors_get_different_frames)
{
    // This test simulates bypass behaviour
    std::shared_ptr<mg::Buffer> compositor[2];

    auto handle = client_acquire_async(q);
    ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
    handle->release_buffer();
    compositor[0] = q.compositor_acquire(this);

    handle = client_acquire_async(q);
    ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
    handle->release_buffer();
    compositor[1] = q.compositor_acquire(this);

    for (int i = 0; i < 20; i++)
    {
        // Two compositors acquired, and they're always different...
        ASSERT_THAT(compositor[0]->id(), Ne(compositor[1]->id()));

        // One of the compositors (the oldest one) gets a new buffer...
        int oldest = i & 1;
        q.compositor_release(compositor[oldest]);
        auto handle = client_acquire_async(q);
        ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
        handle->release_buffer();
        compositor[oldest] = q.compositor_acquire(this);
    }

    q.compositor_release(compositor[0]);
    q.compositor_release(compositor[1]);
}

TEST_P(WithTwoOrMoreBuffers, overlapping_compositors_get_different_frames)
{
    struct InterlockingConsumer : ConsumerSystem
    {
        InterlockingConsumer(mc::BufferStream& stream) :
            stream(stream)
        {
        }
        void consume() override
        {
            buffer = stream.lock_compositor_buffer(this);
            buffer->read([this](unsigned char const* p) {
                entries.emplace_back(BufferEntry{buffer->id(), *reinterpret_cast<unsigned int const*>(p)});
            });
        }
        std::vector<BufferEntry> consumption_log()
        {
            return entries;
        }
        mc::BufferStream& stream;
        std::vector<BufferEntry> entries;
        std::shared_ptr<mg::Buffer> buffer;
    } consumer1(stream), consumer2(stream);

    std::vector<ScheduleEntry> schedule = {
        {1_t,  {},          {&consumer1}},
        {2_t,  {&producer}, {}},
        {3_t,  {},          {&consumer2}},
        {4_t,  {&producer}, {}},
        {5_t,  {},          {&consumer1}},
        {6_t,  {&producer}, {}},
        {7_t,  {},          {&consumer2}},
        {8_t,  {&producer}, {}},
        {9_t,  {},          {&consumer1}},
        {10_t,  {&producer}, {}},
        {11_t,  {},          {&consumer2}},
    };
    run_system(schedule);

    auto production_log = producer.production_log();
    auto consumption_log_1 = consumer1.consumption_log();
    auto consumption_log_2 = consumer2.consumption_log();
    for(auto l : production_log)
        printf("P %i\n", l.id);
    for(auto l : consumption_log_1)
        printf("c1 %i\n", l.id);
    for(auto l : consumption_log_2)
        printf("c2 %i\n", l.id);
}
#endif

//TEST_P(WithTwoOrMoreBuffers, framedropping_clients_get_all_buffers)
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

//TEST_P(WithTwoOrMoreBuffers, client_framerate_matches_compositor)
TEST_P(WithTwoOrMoreBuffers, nonframedropping_client_throttles_to_compositor_rate)
{
    int const reps = 50;
    int blocks = 0;
    for(auto i = 0; i < reps; i++)
    {
        producer.produce();
        if (!producer.can_produce()) blocks++;
        consumer.consume();
    }
    EXPECT_THAT(blocks, Ge(reps - nbuffers));
}

#if 0
/* Regression test LP: #1241369 / LP: #1241371 */
TEST_P(WithThreeOrMoreBuffers, slow_client_framerate_matches_compositor)
{
    /* BufferQueue can only satify this for nbuffers >= 3
     * since a client can only own up to nbuffers - 1 at any one time
     */
    unsigned long client_frames = 0;
    unsigned long const compose_frames = 100;
    auto const frame_time = std::chrono::milliseconds(16);

    q.allow_framedropping(false);

    std::atomic<bool> done(false);
    std::mutex sync;

    mt::AutoJoinThread monitor1([&]
    {
        for (unsigned long frame = 0; frame != compose_frames+3; frame++)
        {
            std::this_thread::sleep_for(frame_time);
            sync.lock();
            auto buf = q.compositor_acquire(this);
            q.compositor_release(buf);
            sync.unlock();

            if (frame == compose_frames)
            {
                // Tell the "client" to stop after compose_frames, but
                // don't stop rendering immediately to avoid blocking
                // if we rendered any twice
                done.store(true);
            }
        }
    });

    auto handle = client_acquire_async(q);
    ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
    handle->release_buffer();

    while (!done.load())
    {
        sync.lock();
        sync.unlock();
        auto handle = client_acquire_async(q);
        handle->wait_for(std::chrono::seconds(1));
        ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
        std::this_thread::sleep_for(frame_time);
        handle->release_buffer();
        client_frames++;
    }

    monitor1.stop();

    // Roughly compose_frames == client_frames within 20%
    ASSERT_THAT(client_frames, Gt(compose_frames * 0.8f));
    ASSERT_THAT(client_frames, Lt(compose_frames * 1.2f));
}
#endif


//TEST_P(WithAnyNumberOfBuffers, resize_affects_client_acquires_immediately)
TEST_P(WithAnyNumberOfBuffers, resize_affects_client_acquires_immediately)
{
    unsigned int const sizes_to_test{4};
    geom::Size new_size = properties.size;
    for(auto i = 0u; i < sizes_to_test; i++)
    {
        new_size = new_size * 2;
        queue.resize(new_size);
        producer.produce();
        consumer.consume();
        EXPECT_THAT(producer.last_size(), Eq(new_size));
    }
}

//TEST_P(WithAnyNumberOfBuffers, compositor_acquires_resized_frames)
TEST_P(WithAnyNumberOfBuffers, compositor_acquires_resized_frames)
{
    unsigned int const sizes_to_test{4};
    int const attempt_limit{100};
    geom::Size new_size = properties.size;
    for(auto i = 0u; i < sizes_to_test; i++)
    {
        new_size = new_size * 2;
        queue.resize(new_size);
        producer.produce();
        consumer.consume();
        producer.produce();

        int attempt_count = 0;
        while((consumer.last_size() != new_size) && (attempt_count++ < attempt_limit))
            consumer.consume();
        ASSERT_THAT(attempt_count, Lt(attempt_limit)) << "consumer never got the new size";
    }
}

#if 0
TEST_P(WithAnyNumberOfBuffers, compositor_acquires_resized_frames)
{
    mc::BufferQueue q(nbuffers, allocator, basic_properties, policy_factory);
    std::vector<mg::BufferID> history;

    const int width0 = 123;
    const int height0 = 456;
    const int dx = 2;
    const int dy = -3;
    int width = width0;
    int height = height0;
    int const nbuffers_to_use = q.buffers_free_for_client();
    ASSERT_THAT(nbuffers_to_use, Gt(0));

    int max_buffers{max_ownable_buffers(nbuffers)};
    for (int produce = 0; produce < max_buffers; ++produce)
    {
        geom::Size new_size{width, height};
        width += dx;
        height += dy;

        q.resize(new_size);
        auto handle = client_acquire_async(q);
        ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
        history.emplace_back(handle->id());
        auto buffer = handle->buffer();
        ASSERT_THAT(buffer->size(), Eq(new_size));
        handle->release_buffer();
    }

    width = width0;
    height = height0;

    ASSERT_THAT(history.size(), Eq(max_buffers));
    for (int consume = 0; consume < max_buffers; ++consume)
    {
        geom::Size expect_size{width, height};
        width += dx;
        height += dy;

        auto buffer = q.compositor_acquire(this);

        // Verify the compositor gets resized buffers, eventually
        ASSERT_THAT(buffer->size(), Eq(expect_size));

        // Verify the compositor gets buffers with *contents*, ie. that
        // they have not been resized prematurely and are empty.
        ASSERT_THAT(history[consume], Eq(buffer->id()));

        q.compositor_release(buffer);
    }

    // Verify the final buffer size sticks
    const geom::Size final_size{width - dx, height - dy};
    for (int unchanging = 0; unchanging < 100; ++unchanging)
    {
        auto buffer = q.compositor_acquire(this);
        ASSERT_THAT(buffer->size(), Eq(final_size));
        q.compositor_release(buffer);
    }
}

TEST_P(WithTwoOrMoreBuffers, framedropping_policy_never_drops_newest_frame)
{  // Regression test for LP: #1396006
    mtd::MockFrameDroppingPolicyFactory policy_factory;
    mc::BufferQueue q(nbuffers,
                      allocator,
                      basic_properties,
                      policy_factory);

    auto first = client_acquire_sync(q);
    q.client_release(first);

    // Start rendering one (don't finish)
    auto d = q.compositor_acquire(nullptr);
    ASSERT_EQ(first, d.get());

    auto second = client_acquire_sync(q);
    q.client_release(second);

    // Client waits for a new frame
    auto end = client_acquire_async(q);

    // Surface goes offscreen or occluded; trigger a timeout
    policy_factory.trigger_policies();

    // If the queue is still willing to drop under these difficult
    // circumstances (and we don't mind if it doesn't), then ensure
    // it's never the newest frame that's been discarded.
    // That could be catastrophic as you never know if a client ever
    // will produce another frame.
    if (end->has_acquired_buffer())
        ASSERT_NE(second, end->buffer());

    q.compositor_release(d);
}

TEST_P(WithTwoOrMoreBuffers, framedropping_surface_never_drops_newest_frame)
{  // Second regression test for LP: #1396006, LP: #1379685
    mc::BufferQueue q(nbuffers,
                      allocator,
                      basic_properties,
                      policy_factory);

    q.allow_framedropping(true);

    // Fill 'er up
    std::vector<mg::Buffer*> order;
    for (int f = 0; f < nbuffers; ++f)
    {
        auto b = client_acquire_sync(q);
        order.push_back(b);
        q.client_release(b);
    }

    // Composite all but one
    std::vector<std::shared_ptr<mg::Buffer>> compositing;
    for (int n = 0; n < nbuffers-1; ++n)
    {
        auto c = q.compositor_acquire(nullptr);
        compositing.push_back(c);
        ASSERT_EQ(order[n], c.get());
    }

    // Ensure it's not the newest frame that gets dropped to satisfy the
    // client.
    auto end = client_acquire_async(q);

    // The queue could solve this problem a few ways. It might choose to
    // defer framedropping till it's safe, or even allocate additional
    // buffers. We don't care which, just verify it's not losing the
    // latest frame. Because the screen could be indefinitely out of date
    // if that happens...
    ASSERT_TRUE(!end->has_acquired_buffer() ||
                end->buffer() != order.back());
}

TEST_P(WithTwoOrMoreBuffers, uncomposited_client_swaps_when_policy_triggered)
{
    mtd::MockFrameDroppingPolicyFactory policy_factory;
    mc::BufferQueue q(nbuffers,
                      allocator,
                      basic_properties,
                      policy_factory);

    for (int i = 0; i < max_ownable_buffers(nbuffers); i++)
    {
        auto client = client_acquire_sync(q);
        q.client_release(client);
    }

    auto handle = client_acquire_async(q);

    EXPECT_FALSE(handle->has_acquired_buffer());

    policy_factory.trigger_policies();
    EXPECT_TRUE(handle->has_acquired_buffer());
}

TEST_P(WithTwoOrMoreBuffers, partially_composited_client_swaps_when_policy_triggered)
{
    mtd::MockFrameDroppingPolicyFactory policy_factory;
    mc::BufferQueue q(nbuffers,
                      allocator,
                      basic_properties,
                      policy_factory);

    for (int i = 0; i < max_ownable_buffers(nbuffers); i++)
    {
        auto client = client_acquire_sync(q);
        q.client_release(client);
    }

    /* Queue up two pending swaps */
    auto first_swap = client_acquire_async(q);
    auto second_swap = client_acquire_async(q);

    ASSERT_FALSE(first_swap->has_acquired_buffer());
    ASSERT_FALSE(second_swap->has_acquired_buffer());

    q.compositor_acquire(nullptr);

    EXPECT_TRUE(first_swap->has_acquired_buffer());
    EXPECT_FALSE(second_swap->has_acquired_buffer());

    /* We have to release a client buffer here; framedropping or not,
     * a client can't have 2 buffers outstanding in the nbuffers = 2 case.
     */
    first_swap->release_buffer();

    policy_factory.trigger_policies();
    EXPECT_TRUE(second_swap->has_acquired_buffer());
}

TEST_F(BufferQueueWithOneBuffer, with_single_buffer_compositor_acquires_resized_frames_eventually)
{
    geom::Size const new_size{123,456};

    q.client_release(client_acquire_sync(q));
    q.resize(new_size);

    auto const handle = client_acquire_async(q);
    EXPECT_THAT(handle->has_acquired_buffer(), Eq(false));

    auto buf = q.compositor_acquire(this);
    q.compositor_release(buf);

    buf = q.compositor_acquire(this);
    EXPECT_THAT(buf->size(), Eq(new_size));
    q.compositor_release(buf);
}

TEST_F(BufferQueueWithTwoBuffers, double_buffered_client_is_not_blocked_prematurely)
{  // Regression test for LP: #1319765
    q.client_release(client_acquire_sync(q));
    auto a = q.compositor_acquire(this);
    q.client_release(client_acquire_sync(q));
    auto b = q.compositor_acquire(this);

    ASSERT_NE(a.get(), b.get());

    q.compositor_release(a);
    q.client_release(client_acquire_sync(q));

    q.compositor_release(b);

    /*
     * Update to the original test case; This additional compositor acquire
     * represents the fixing of LP: #1395581 in the compositor logic.
     */
    if (q.buffers_ready_for_compositor(this))
        q.compositor_release(q.compositor_acquire(this));

    auto handle = client_acquire_async(q);
    // With the fix, a buffer will be available instantaneously:
    ASSERT_TRUE(handle->has_acquired_buffer());
    handle->release_buffer();
}

TEST_F(BufferQueueWithTwoBuffers, composite_on_demand_never_deadlocks_with_2_buffers)
{  // Extended regression test for LP: #1319765
    for (int i = 0; i < 100; ++i)
    {
        auto x = client_acquire_async(q);
        ASSERT_TRUE(x->has_acquired_buffer());
        x->release_buffer();

        auto a = q.compositor_acquire(this);

        auto y = client_acquire_async(q);
        ASSERT_TRUE(y->has_acquired_buffer());
        y->release_buffer();

        auto b = q.compositor_acquire(this);
    
        ASSERT_NE(a.get(), b.get());
    
        q.compositor_release(a);

        auto w = client_acquire_async(q);
        ASSERT_TRUE(w->has_acquired_buffer());
        w->release_buffer();
    
        q.compositor_release(b);

        /*
         * Update to the original test case; This additional compositor acquire
         * represents the fixing of LP: #1395581 in the compositor logic.
         */
        if (q.buffers_ready_for_compositor(this))
            q.compositor_release(q.compositor_acquire(this));

        auto z = client_acquire_async(q);
        ASSERT_TRUE(z->has_acquired_buffer());
        z->release_buffer();

        q.compositor_release(q.compositor_acquire(this));
        q.compositor_release(q.compositor_acquire(this));
    }
}

TEST_P(WithTwoOrMoreBuffers, buffers_ready_is_not_underestimated)
{  // Regression test for LP: #1395581
    mc::BufferQueue q{nbuffers, allocator, basic_properties, policy_factory};

    // Produce frame 1
    q.client_release(client_acquire_sync(q));
    // Acquire frame 1
    auto a = q.compositor_acquire(this);

    // Produce frame 2
    q.client_release(client_acquire_sync(q));
    // Acquire frame 2
    auto b = q.compositor_acquire(this);

    // Release frame 1
    q.compositor_release(a);
    // Produce frame 3
    q.client_release(client_acquire_sync(q));
    // Release frame 2
    q.compositor_release(b);

    // Verify frame 3 is ready for the first compositor
    ASSERT_THAT(q.buffers_ready_for_compositor(this), Ge(1));
    auto c = q.compositor_acquire(this);

    // Verify frame 3 is ready for a second compositor
    int const that = 0;
    ASSERT_THAT(q.buffers_ready_for_compositor(&that), Ge(1));

    q.compositor_release(c);
}

TEST_P(WithTwoOrMoreBuffers, buffers_ready_eventually_reaches_zero)
{
    mc::BufferQueue q{nbuffers, allocator, basic_properties, policy_factory};

    const int nmonitors = 3;
    int monitor[nmonitors];

    for (int m = 0; m < nmonitors; ++m)
    {
        ASSERT_EQ(0, q.buffers_ready_for_compositor(&monitor[m]));
    }

    // Produce a frame
    q.client_release(client_acquire_sync(q));

    // Consume
    for (int m = 0; m < nmonitors; ++m)
    {
        ASSERT_NE(0, q.buffers_ready_for_compositor(&monitor[m]));

        // Double consume to account for the +1 that
        // buffers_ready_for_compositor adds to do dynamic performance
        // detection.
        q.compositor_release(q.compositor_acquire(&monitor[m]));
        q.compositor_release(q.compositor_acquire(&monitor[m]));

        ASSERT_EQ(0, q.buffers_ready_for_compositor(&monitor[m]));
    }
}

/* Regression test for LP: #1306464 */
TEST_F(BufferQueueWithThreeBuffers, framedropping_client_acquire_does_not_block_when_no_available_buffers)
{
    q.allow_framedropping(true);

    std::vector<std::shared_ptr<mg::Buffer>> buffers;

    /* The client can never own this acquired buffer */
    auto comp_buffer = q.compositor_acquire(this);
    buffers.push_back(comp_buffer);

    /* Let client release all possible buffers so they go into
     * the ready queue
     */
    for (int i = 0; i < nbuffers; ++i)
    {
        auto handle = client_acquire_async(q);
        ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
        /* Check the client never got the compositor buffer acquired above */
        ASSERT_THAT(handle->id(), Ne(comp_buffer->id()));
        handle->release_buffer();
    }

    /* Let the compositor acquire all ready buffers */
    for (int i = 0; i < nbuffers; ++i)
    {
        buffers.push_back(q.compositor_acquire(this));
    }

    /* At this point the queue has 0 free buffers and 0 ready buffers
     * so the next client request should not be satisfied until
     * a compositor releases its buffers */
    auto handle = client_acquire_async(q);
    /* ... unless the BufferQueue is overallocating. In that case it will
     * have succeeding in acquiring immediately.
     */ 
    if (!handle->has_acquired_buffer())
    {
        /* Release compositor buffers so that the client can get one */
        for (auto const& buffer : buffers)
        {
            q.compositor_release(buffer);
        }
        EXPECT_THAT(handle->has_acquired_buffer(), Eq(true));
    }
}

TEST_P(WithTwoOrMoreBuffers, compositor_never_owns_client_buffers)
{
    static std::chrono::nanoseconds const time_for_client_to_acquire{1};

    mc::BufferQueue q(nbuffers, allocator, basic_properties, policy_factory);

    std::mutex client_buffer_guard;
    std::condition_variable client_buffer_cv;
    mg::Buffer* client_buffer = nullptr;
    std::atomic<bool> done(false);

    auto unblock = [&done]{ done = true; };
    mt::AutoUnblockThread unthrottled_compositor_thread(unblock, [&]
    {
        while (!done)
        {
            auto buffer = q.compositor_acquire(this);

            {
                std::unique_lock<std::mutex> lock(client_buffer_guard);

                if (client_buffer_cv.wait_for(
                    lock,
                    time_for_client_to_acquire,
                    [&]()->bool{ return client_buffer; }))
                {
                    ASSERT_THAT(buffer->id(), Ne(client_buffer->id()));
                }
            }

            std::this_thread::yield();
            q.compositor_release(buffer);
        }
    });

    for (int i = 0; i < 1000; ++i)
    {
        auto handle = client_acquire_async(q);
        handle->wait_for(std::chrono::seconds(1));
        ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));

        {
            std::lock_guard<std::mutex> lock(client_buffer_guard);
            client_buffer = handle->buffer();
            client_buffer_cv.notify_one();
        }

        std::this_thread::yield();

        std::lock_guard<std::mutex> lock(client_buffer_guard);
        handle->release_buffer();
        client_buffer = nullptr;
    }
}

TEST_P(WithTwoOrMoreBuffers, client_never_owns_compositor_buffers)
{
    mc::BufferQueue q(nbuffers, allocator, basic_properties, policy_factory);
    for (int i = 0; i < 100; ++i)
    {
        auto handle = client_acquire_async(q);
        ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));

        auto client_id = handle->id();
        std::vector<std::shared_ptr<mg::Buffer>> buffers;
        for (int j = 0; j < nbuffers; j++)
        {
            auto buffer = q.compositor_acquire(this);
            ASSERT_THAT(client_id, Ne(buffer->id()));
            buffers.push_back(buffer);
        }

        for (auto const& buffer: buffers)
            q.compositor_release(buffer);

        handle->release_buffer();

        /* Flush out one ready buffer */
        auto buffer = q.compositor_acquire(this);
        ASSERT_THAT(client_id, Eq(buffer->id()));
        q.compositor_release(buffer);
    }
}

/* Regression test for an issue brought up at:
 * http://code.launchpad.net/~albaguirre/mir/
 * alternative-switching-bundle-implementation/+merge/216606/comments/517048
 */
TEST_P(WithThreeOrMoreBuffers, buffers_are_not_lost)
{
    // This test is technically not valid with dynamic queue scaling on
    q.set_scaling_delay(-1);

    void const* main_compositor = reinterpret_cast<void const*>(0);
    void const* second_compositor = reinterpret_cast<void const*>(1);

    /* Hold a reference to current compositor buffer*/
    auto comp_buffer1 = q.compositor_acquire(main_compositor);

    int const prefill = q.buffers_free_for_client();
    ASSERT_THAT(prefill, Gt(0));
    for (int acquires = 0; acquires < prefill; ++acquires)
    {
        auto handle = client_acquire_async(q);
        ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
        handle->release_buffer();
    }

    /* Have a second compositor advance the current compositor buffer at least twice */
    for (int acquires = 0; acquires < nbuffers; ++acquires)
    {
        auto comp_buffer = q.compositor_acquire(second_compositor);
        q.compositor_release(comp_buffer);
    }
    q.compositor_release(comp_buffer1);

    /* An async client should still be able to cycle through all the available buffers */
    std::atomic<bool> done(false);
    auto unblock = [&done] { done = true; };
    mt::AutoUnblockThread compositor(unblock,
       unthrottled_compositor_thread, std::ref(q), std::ref(done));

    std::unordered_set<mg::Buffer *> unique_buffers_acquired;
    int const max_ownable_buffers = nbuffers - 1;
    for (int frame = 0; frame < max_ownable_buffers*2; frame++)
    {
        std::vector<mg::Buffer *> client_buffers;
        for (int acquires = 0; acquires < max_ownable_buffers; ++acquires)
        {
            auto handle = client_acquire_async(q);
            handle->wait_for(std::chrono::seconds(1));
            ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
            unique_buffers_acquired.insert(handle->buffer());
            client_buffers.push_back(handle->buffer());
        }

        for (auto const& buffer : client_buffers)
        {
            q.client_release(buffer);
        }
    }

    EXPECT_THAT(unique_buffers_acquired.size(), Eq(nbuffers));
}

// Test that dynamic queue scaling/throttling actually works
TEST_P(WithThreeOrMoreBuffers, queue_size_scales_with_client_performance)
{
    q.allow_framedropping(false);

    std::atomic<bool> done(false);
    auto unblock = [&done] { done = true; };

    // To emulate a "fast" client we use a "slow" compositor
    mt::AutoUnblockThread compositor(unblock,
       throttled_compositor_thread, std::ref(q), std::ref(done));

    std::unordered_set<mg::Buffer *> buffers_acquired;

    int const delay = q.scaling_delay();
    EXPECT_EQ(3, delay);  // expect a sane default

    for (int frame = 0; frame < 10; frame++)
    {
        auto handle = client_acquire_async(q);
        handle->wait_for(std::chrono::seconds(1));
        ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));

        if (frame > delay)
            buffers_acquired.insert(handle->buffer());
        handle->release_buffer();
    }
    // Expect double-buffers for fast clients
    EXPECT_THAT(buffers_acquired.size(), Eq(2));

    // Now check what happens if the client becomes slow...
    buffers_acquired.clear();
    for (int frame = 0; frame < 10; frame++)
    {
        auto handle = client_acquire_async(q);
        handle->wait_for(std::chrono::seconds(1));
        ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));

        if (frame > delay)
            buffers_acquired.insert(handle->buffer());

        // Client is just too slow to keep up:
        std::this_thread::sleep_for(throttled_compositor_rate * 1.5);

        handle->release_buffer();
    }
    // Expect at least triple buffers for sluggish clients
    EXPECT_THAT(buffers_acquired.size(), Ge(3));

    // And what happens if the client becomes fast again?...
    buffers_acquired.clear();
    for (int frame = 0; frame < 10; frame++)
    {
        auto handle = client_acquire_async(q);
        handle->wait_for(std::chrono::seconds(1));
        ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));

        if (frame > delay)
            buffers_acquired.insert(handle->buffer());
        handle->release_buffer();
    }
    // Expect double-buffers for fast clients
    EXPECT_THAT(buffers_acquired.size(), Eq(2));
}

TEST_P(WithThreeOrMoreBuffers, greedy_compositors_need_triple_buffers)
{
    /*
     * "Greedy" compositors means those that can hold multiple buffers from
     * the same client simultaneously or a single buffer for a long time.
     * This usually means bypass/overlays, but can also mean multi-monitor.
     */
    q.allow_framedropping(false);

    std::atomic<bool> done(false);
    auto unblock = [&done] { done = true; };

    mt::AutoUnblockThread compositor(unblock,
       overlapping_compositor_thread, std::ref(q), std::ref(done));

    std::unordered_set<mg::Buffer *> buffers_acquired;
    int const delay = q.scaling_delay();

    for (int frame = 0; frame < 10; frame++)
    {
        auto handle = client_acquire_async(q);
        handle->wait_for(std::chrono::seconds(1));
        ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));

        if (frame > delay)
            buffers_acquired.insert(handle->buffer());
        handle->release_buffer();
    }
    // Expect triple buffers for the whole time
    EXPECT_THAT(buffers_acquired.size(), Ge(3));
}

TEST_P(WithTwoOrMoreBuffers, compositor_double_rate_of_slow_client)
{
    mc::BufferQueue q(nbuffers, allocator, basic_properties, policy_factory);
    q.allow_framedropping(false);

    for (int frame = 0; frame < 10; frame++)
    {
        ASSERT_EQ(0, q.buffers_ready_for_compositor(this));
        q.client_release(client_acquire_sync(q));

        // Detecting a slow client requires scheduling at least one extra
        // frame...
        int nready = q.buffers_ready_for_compositor(this);
        ASSERT_EQ(2, nready);
        for (int i = 0; i < nready; ++i)
            q.compositor_release(q.compositor_acquire(this));
        ASSERT_EQ(0, q.buffers_ready_for_compositor(this));
    }
}

/*
 * This is a regression test for bug lp:1317801. This bug is a race and
 * very difficult to reproduce with pristine code. By carefully placing
 * a delay in the code, we can greatly increase the chances (100% for me)
 * that this test catches a regression. However these delays are not
 * acceptable for production use, so since the test and code in their
 * pristine state are highly unlikely to catch the issue, I have decided
 * to DISABLE the test to avoid giving a false sense of security.
 *
 * Apply the aforementioned delay, by adding
 * std::this_thread::sleep_for(std::chrono::milliseconds{20})
 * just before returning the acquired_buffer at the end of
 * BufferQueue::compositor_acquire().
 */
TEST_F(BufferQueueWithThreeBuffers, DISABLED_lp_1317801_regression_test)
{
    q.client_release(client_acquire_sync(q));

    mt::AutoJoinThread t{
        [&]
        {
            /* Use in conjuction with a 20ms delay in compositor_acquire() */
            std::this_thread::sleep_for(std::chrono::milliseconds{10});

            q.client_release(client_acquire_sync(q));
            q.client_release(client_acquire_sync(q));
        }};

    auto b = q.compositor_acquire(this);
    q.compositor_release(b);
}

TEST_P(WithAnyNumberOfBuffers, first_user_is_recorded)
{
    mc::BufferQueue q(nbuffers, allocator, basic_properties, policy_factory);

    auto comp = q.compositor_acquire(this);
    EXPECT_TRUE(q.is_a_current_buffer_user(this));
    q.compositor_release(comp);
}

TEST_F(BufferQueueWithThreeBuffers, gives_compositor_a_valid_buffer_after_dropping_old_buffers_without_clients)
{
    q.drop_old_buffers();

    auto comp = q.compositor_acquire(this);
    ASSERT_THAT(comp, Ne(nullptr));
}

TEST_F(BufferQueueWithThreeBuffers, gives_compositor_the_newest_buffer_after_dropping_old_buffers)
{
    auto handle1 = client_acquire_async(q);
    ASSERT_THAT(handle1->has_acquired_buffer(), Eq(true));
    handle1->release_buffer();

    auto handle2 = client_acquire_async(q);
    ASSERT_THAT(handle2->has_acquired_buffer(), Eq(true));
    handle2->release_buffer();

    q.drop_old_buffers();

    auto comp = q.compositor_acquire(this);
    ASSERT_THAT(comp->id(), Eq(handle2->id()));
    q.compositor_release(comp);

    comp = q.compositor_acquire(this);
    ASSERT_THAT(comp->id(), Eq(handle2->id()));
}

TEST_F(BufferQueueWithThreeBuffers, gives_new_compositor_the_newest_buffer_after_dropping_old_buffers)
{
    void const* const new_compositor_id{&nbuffers};

    auto handle1 = client_acquire_async(q);
    ASSERT_THAT(handle1->has_acquired_buffer(), Eq(true));
    handle1->release_buffer();

    auto comp = q.compositor_acquire(this);
    ASSERT_THAT(comp->id(), Eq(handle1->id()));
    q.compositor_release(comp);

    auto handle2 = client_acquire_async(q);
    ASSERT_THAT(handle2->has_acquired_buffer(), Eq(true));
    handle2->release_buffer();

    q.drop_old_buffers();

    auto comp2 = q.compositor_acquire(new_compositor_id);
    ASSERT_THAT(comp2->id(), Eq(handle2->id()));
}
#endif
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
