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
}

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
