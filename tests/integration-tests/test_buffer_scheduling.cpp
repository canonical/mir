/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/frontend/event_sink.h"
#include "mir/frontend/buffer_sink.h"
#include "mir/renderer/sw/pixel_source.h"
#include "src/client/buffer_vault.h"
#include "src/client/buffer_factory.h"
#include "src/client/protobuf_to_native_buffer.h"
#include "src/client/connection_surface_map.h"
#include "src/server/compositor/stream.h"
#include "mir/test/doubles/stub_client_buffer_factory.h"
#include "mir/test/doubles/mock_client_buffer_factory.h"
#include "mir/test/doubles/stub_buffer_allocator.h"
#include "mir/test/fake_shared.h"
#include "mir_protobuf.pb.h"
#include <gtest/gtest.h>

namespace mt  = mir::test;
namespace mtd = mir::test::doubles;
namespace mcl = mir::client;
namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace mf = mir::frontend;
namespace mp = mir::protobuf;
namespace mrs = mir::renderer::software;
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
    virtual void reset_log() = 0;
    virtual geom::Size last_size()  = 0;
    virtual mg::BufferID current_id() = 0;
    virtual ~ProducerSystem() = default;
    ProducerSystem() = default;
    ProducerSystem(ProducerSystem const&) = delete;
    ProducerSystem& operator=(ProducerSystem const&) = delete;
};

struct ConsumerSystem
{
    virtual void consume() { consume_resource(); }
    virtual std::shared_ptr<mg::Buffer> consume_resource() = 0;
    virtual geom::Size last_size()  = 0;

    virtual void set_framedropping(bool) = 0;

    virtual std::vector<BufferEntry> consumption_log() = 0;
    ConsumerSystem() = default;
    virtual ~ConsumerSystem() = default;
    ConsumerSystem(ConsumerSystem const&) = delete;
    ConsumerSystem& operator=(ConsumerSystem const&) = delete;
};

struct StubIpcSystem
{
    void on_server_bound_transfer(std::function<void(mp::Buffer&)> fn)
    {
        server_bound_fn = fn;
    }

    void on_client_bound_transfer(std::function<void(mp::BufferRequest&)> fn)
    {
        client_bound_fn = fn;
        for(auto& b : buffers)
            client_bound_fn(b);
        buffers.clear();
    }

    void on_allocate(std::function<void(geom::Size)> fn)
    {
        allocate_fn = fn;
    }

    void on_resize_event(std::function<void(geom::Size)> fn)
    {
        resize_fn = fn;
    }

    void resize_event(geom::Size sz)
    {
        if (resize_fn)
            resize_fn(sz);
    }
    void server_bound_transfer(mp::Buffer& buffer)
    {
        if (server_bound_fn)
            server_bound_fn(buffer);
        last_submit = buffer.buffer_id();
    }

    int last_transferred_to_server()
    {
        return last_submit;
    }

    void client_bound_transfer(mp::BufferRequest& request)
    {
        if (client_bound_fn)
            client_bound_fn(request);
        else
            buffers.push_back(request);
    }

    void allocate(geom::Size sz)
    {
        if (allocate_fn)
            allocate_fn(sz);
    }

    std::function<void(geom::Size)> allocate_fn;
    std::function<void(geom::Size)> resize_fn;
    std::function<void(mp::BufferRequest&)> client_bound_fn;
    std::function<void(mp::Buffer&)> server_bound_fn;

    std::vector<mp::BufferRequest> buffers;
    int last_submit{0};
};

struct StubEventSink : public mf::EventSink
{
    StubEventSink(std::shared_ptr<StubIpcSystem> const& ipc) :
        ipc(ipc)
    {
    }

    void send_buffer(mf::BufferStreamId id, mg::Buffer& buffer, mg::BufferIpcMsgType)
    {
        mp::BufferRequest request;
        auto protobuffer = request.mutable_buffer();
        request.mutable_id()->set_value(id.as_value()); 
        protobuffer->set_buffer_id(buffer.id().as_value());
        protobuffer->set_width(buffer.size().width.as_int());
        protobuffer->set_height(buffer.size().height.as_int());
        ipc->client_bound_transfer(request);
    }
    void add_buffer(mg::Buffer& buffer)
    {
        mp::BufferRequest request;
        auto protobuffer = request.mutable_buffer();
        request.set_operation(mp::BufferOperation::add);
        protobuffer->set_buffer_id(buffer.id().as_value());
        protobuffer->set_width(buffer.size().width.as_int());
        protobuffer->set_height(buffer.size().height.as_int());
        ipc->client_bound_transfer(request);
    }
    void remove_buffer(mg::Buffer& buffer)
    {
        mp::BufferRequest request;
        auto protobuffer = request.mutable_buffer();
        request.set_operation(mp::BufferOperation::remove);
        protobuffer->set_buffer_id(buffer.id().as_value());
        protobuffer->set_width(buffer.size().width.as_int());
        protobuffer->set_height(buffer.size().height.as_int());
        ipc->client_bound_transfer(request);
    }
    void update_buffer(mg::Buffer& buffer)
    {
        mp::BufferRequest request;
        auto protobuffer = request.mutable_buffer();
        request.set_operation(mp::BufferOperation::update);
        protobuffer->set_buffer_id(buffer.id().as_value());
        protobuffer->set_width(buffer.size().width.as_int());
        protobuffer->set_height(buffer.size().height.as_int());
        ipc->client_bound_transfer(request);
    }
    void error_buffer(geom::Size, MirPixelFormat, std::string const&) {}
    void handle_event(mir::EventUPtr&&) {}
    void handle_lifecycle_event(MirLifecycleState) {}
    void handle_display_config_change(mg::DisplayConfiguration const&) {}
    void handle_error(mir::ClientVisibleError const&) {}
    void handle_input_config_change(MirInputConfig const&) {}
    void send_ping(int32_t) {}

    std::shared_ptr<StubIpcSystem> ipc;
};

//async semantics
struct ScheduledConsumer : ConsumerSystem
{
    ScheduledConsumer(std::shared_ptr<mc::Stream> stream) :
        stream(stream)
    {
    }

    std::shared_ptr<mg::Buffer> consume_resource() override
    {
        auto b = stream->lock_compositor_buffer(this);
        unsigned int age = 0;
        last_size_ = b->size();
        entries.emplace_back(BufferEntry{b->id(), age, Access::unblocked});
        return b;
    }

    std::vector<BufferEntry> consumption_log() override
    {
        return entries;
    }

    geom::Size last_size() override
    {
        return last_size_;
    }
    
    void set_framedropping(bool allow) override
    {
        stream->allow_framedropping(allow);
    }

    std::shared_ptr<mc::Stream> stream;
    std::vector<BufferEntry> entries;
    geom::Size last_size_;
};

struct ServerRequests : mcl::ServerBufferRequests
{
    ServerRequests(std::shared_ptr<StubIpcSystem> const stub_ipc) :
        ipc(stub_ipc)
    {
    }

    void allocate_buffer(geom::Size sz, MirPixelFormat, int)
    {
        ipc->allocate(sz);
    }

    void free_buffer(int)
    {
    }

    void submit_buffer(mcl::MirBuffer& buffer)
    {
        mp::Buffer buffer_req;
        buffer_req.set_buffer_id(buffer.rpc_id());
        ipc->server_bound_transfer(buffer_req);
    }
    std::shared_ptr<StubIpcSystem> ipc;
};

struct ScheduledProducer : ProducerSystem
{
    ScheduledProducer(std::shared_ptr<StubIpcSystem> const& ipc_stub, int nbuffers) :
        ipc(ipc_stub),
        map(std::make_shared<mcl::ConnectionSurfaceMap>()),
        factory(std::make_shared<mcl::BufferFactory>()),
        vault(
            std::make_shared<mtd::StubClientBufferFactory>(), factory,
            std::make_shared<ServerRequests>(ipc), map,
            geom::Size(100,100), mir_pixel_format_abgr_8888, 0, nbuffers)
    {
        ipc->on_client_bound_transfer([this](mp::BufferRequest& request){

            if (request.has_id())
            {
                auto& ipc_buffer = request.buffer();
                auto buffer = map->buffer(ipc_buffer.buffer_id());
                if (!buffer)
                {
                    buffer = factory->generate_buffer(ipc_buffer);
                    map->insert(ipc_buffer.buffer_id(), buffer); 
                    buffer->received();
                }
                else
                {
                    buffer->received(*mcl::protobuf_to_native_buffer(ipc_buffer));
                }
            }
            else if (request.has_operation() && request.operation() == mp::BufferOperation::add)
            {
                auto& ipc_buffer = request.buffer();
                std::shared_ptr<mcl::MirBuffer> buffer = factory->generate_buffer(ipc_buffer);
                map->insert(request.buffer().buffer_id(), buffer); 
                buffer->received();
            }
            else if (request.has_operation() && request.operation() == mp::BufferOperation::update)
            {
                auto buffer = map->buffer(request.buffer().buffer_id());
                buffer->received(*mcl::protobuf_to_native_buffer(request.buffer()));
            }            
            available++;

        });
        ipc->on_resize_event([this](geom::Size sz)
        {
            vault.set_size(sz);
        });
    }
    ~ScheduledProducer()
    {
        ipc->on_client_bound_transfer([](mp::BufferRequest&){});
    }

    bool can_produce()
    {
        return available > 0;
    }

    mg::BufferID current_id()
    {
        return current_id_;
    }

    void produce()
    {
        if (can_produce())
        {
            auto buffer = vault.withdraw().get();
            vault.deposit(buffer);
            vault.wire_transfer_outbound(buffer, []{});
            last_size_ = buffer->size();
            entries.emplace_back(BufferEntry{mg::BufferID{(unsigned int)ipc->last_transferred_to_server()}, age, Access::unblocked});
            available--;
        }
        else
        {
            entries.emplace_back(BufferEntry{mg::BufferID{2}, 0u, Access::blocked});
        }
    }

    std::vector<BufferEntry> production_log()
    {
        return entries;
    }

    geom::Size last_size()
    {
        return last_size_;
    }

    void reset_log()
    {
        entries.clear();
    }

    geom::Size last_size_;
    std::vector<BufferEntry> entries;
    std::shared_ptr<StubIpcSystem> ipc;
    std::shared_ptr<mcl::SurfaceMap> const map;
    std::shared_ptr<mcl::BufferFactory> factory;
    mcl::BufferVault vault;
    int max, cur;
    int available{0};
    unsigned int age{0};
    mg::BufferID current_id_;
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

int unique_ids_in(std::vector<BufferEntry> log)
{
    std::sort(log.begin(), log.end(),
        [](BufferEntry const& a, BufferEntry const& b) { return a.id < b.id; });
    auto it = std::unique(log.begin(), log.end(),
        [](BufferEntry const& a, BufferEntry const& b) { return a.id == b.id; } );
    return std::distance(log.begin(), it);
}

MATCHER(NeverBlocks, "")
{
    bool never_blocks = true;
    for(auto& e : arg)
        never_blocks &= (e.blockage == Access::unblocked);
    return never_blocks; 
}

class AutoSendBuffer : public mg::Buffer
{
public:
    AutoSendBuffer(
        std::shared_ptr<mg::Buffer> const& wrapped,
        std::shared_ptr<mf::BufferSink> const& sink)
        : wrapped{wrapped},
          sink{sink}
    {
    }

    ~AutoSendBuffer()
    {
        sink->update_buffer(*wrapped);
    }

    std::shared_ptr<mg::NativeBuffer> native_buffer_handle() const override
    {
        return wrapped->native_buffer_handle();
    }

    mg::BufferID id() const override
    {
        return wrapped->id();
    }

    geom::Size size() const override
    {
        return wrapped->size();
    }

    MirPixelFormat pixel_format() const override
    {
        return wrapped->pixel_format();
    }

    mg::NativeBufferBase *native_buffer_base() override
    {
        return wrapped->native_buffer_base();
    }

private:
    std::shared_ptr<mg::Buffer> const wrapped;
    std::shared_ptr<mf::BufferSink> const sink;
};

//test infrastructure
struct BufferScheduling : public Test, ::testing::WithParamInterface<int>
{
    BufferScheduling()
    {
        ipc = std::make_shared<StubIpcSystem>();
        sink = std::make_shared<StubEventSink>(ipc);
        auto submit_stream = std::make_shared<mc::Stream>(
            geom::Size{100,100},
            mir_pixel_format_abgr_8888);
        auto weak_stream = std::weak_ptr<mc::Stream>(submit_stream);
        ipc->on_server_bound_transfer(
            [weak_stream, this](mp::Buffer& buffer)
            {
                auto submit_stream = weak_stream.lock();
                if (!submit_stream)
                    return;
                mg::BufferID id{static_cast<unsigned int>(buffer.buffer_id())};
                submit_stream->submit_buffer(
                    std::make_shared<AutoSendBuffer>(map.at(id), sink));
            });
        ipc->on_allocate(
            [this](geom::Size sz)
            {
                auto const buffer = std::make_shared<mtd::StubBuffer>(sz);
                map[buffer->id()] = buffer;
                sink->add_buffer(*buffer);
            });

        consumer = std::make_unique<ScheduledConsumer>(submit_stream);
        second_consumer = std::make_unique<ScheduledConsumer>(submit_stream);
        third_consumer = std::make_unique<ScheduledConsumer>(submit_stream);
        producer = std::make_unique<ScheduledProducer>(ipc, GetParam());

        stream = submit_stream;
    }


    void resize(geom::Size sz)
    {
        producer->produce();
        ipc->resize_event(sz);
        consumer->consume();
    }

    void allow_framedropping()
    {
        consumer->set_framedropping(true);
    }

    void disallow_framedropping()
    {
        consumer->set_framedropping(false);
    }

    mtd::MockClientBufferFactory client_buffer_factory;
    mtd::StubBufferAllocator server_buffer_factory;
    mg::BufferProperties properties{geom::Size{3,3}, mir_pixel_format_abgr_8888, mg::BufferUsage::hardware};
    int nbuffers = GetParam();

    std::shared_ptr<mc::BufferStream> stream;
    std::shared_ptr<StubIpcSystem> ipc;
    std::shared_ptr<mf::BufferSink> sink;
    std::unique_ptr<ProducerSystem> producer;
    std::unique_ptr<ConsumerSystem> consumer;
    std::unique_ptr<ConsumerSystem> second_consumer;
    std::unique_ptr<ConsumerSystem> third_consumer;
    std::unordered_map<mg::BufferID, std::shared_ptr<mg::Buffer>> map;
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
        {1_t,   {producer.get()}, {}},
        {60_t,           {}, {consumer.get()}},
        {61_t,  {producer.get()}, {}},
        {120_t,          {}, {consumer.get()}},
        {121_t, {producer.get()}, {}},
        {180_t,          {}, {consumer.get()}},
    };
    run_system(schedule);

    auto production_log = producer->production_log();
    auto consumption_log = consumer->consumption_log();
    EXPECT_THAT(production_log, Not(IsEmpty()));
    EXPECT_THAT(consumption_log, Not(IsEmpty()));
    EXPECT_THAT(consumption_log, ContainerEq(production_log));
}

TEST_P(WithTwoOrMoreBuffers, framedropping_producers_dont_block)
{
    allow_framedropping();
    std::vector<ScheduleEntry> schedule = {
        {0_t,  {producer.get()}, {}},
        {61_t, {producer.get()}, {}},
        {62_t, {producer.get()}, {}},
        {63_t, {producer.get()}, {}},
        {64_t, {producer.get()}, {}},
        {90_t,          {}, {consumer.get()}},
        {91_t, {producer.get()}, {}},
        {92_t, {producer.get()}, {}},
        {93_t, {producer.get()}, {}},
        {94_t, {producer.get()}, {}},
        {120_t,         {}, {consumer.get()}},
    };
    run_system(schedule);
    auto production_log = producer->production_log();
    auto consumption_log = consumer->consumption_log();
    ASSERT_THAT(production_log, SizeIs(9));
    EXPECT_THAT(consumption_log, SizeIs(2));
}

TEST_P(WithThreeOrMoreBuffers, synchronous_overproducing_producers_has_all_buffers_consumed)
{
    std::vector<ScheduleEntry> schedule = {
        {1_t,   {producer.get()}, {}},
        {60_t,           {}, {consumer.get()}},
        {61_t,  {producer.get()}, {}},
        {62_t,  {producer.get()}, {}},
        {120_t,          {}, {consumer.get()}},
        {180_t,          {}, {consumer.get()}},
    };
    run_system(schedule);

    auto production_log = producer->production_log();
    auto consumption_log = consumer->consumption_log();
    EXPECT_THAT(production_log, Not(IsEmpty()));
    EXPECT_THAT(consumption_log, Not(IsEmpty()));
    EXPECT_THAT(consumption_log, ContainerEq(production_log));
}

/* Regression test for LP: #1210042 */
TEST_P(WithThreeOrMoreBuffers, consumers_dont_recycle_startup_buffer )
{
    std::vector<ScheduleEntry> schedule = {
        {1_t, {producer.get()}, {}},
        {2_t, {producer.get()}, {}},
        {3_t,          {}, {consumer.get()}},
    };
    run_system(schedule);

    auto production_log = producer->production_log();
    auto consumption_log = consumer->consumption_log();
    ASSERT_THAT(production_log, SizeIs(2)); 
    ASSERT_THAT(consumption_log, SizeIs(1));
    EXPECT_THAT(consumption_log[0], Eq(production_log[0])); 
}

TEST_P(WithTwoOrMoreBuffers, consumer_cycles_through_all_available_buffers)
{
    auto tick = 0_t;
    std::vector<ScheduleEntry> schedule;
    for(auto i = 0; i < nbuffers - 1; i++)
        schedule.emplace_back(ScheduleEntry{tick++, {producer.get()}, {}});
    run_system(schedule);
 
    for(auto i = 0; i < nbuffers; i++)
        schedule.emplace_back(ScheduleEntry{tick++, {producer.get()}, {consumer.get()}});
    run_system(schedule);

    auto consumption_log = consumer->consumption_log();
    std::sort(consumption_log.begin(), consumption_log.end(),
        [](BufferEntry const& a, BufferEntry const& b) { return a.id.as_value() > b.id.as_value(); });
    auto it = std::unique(consumption_log.begin(), consumption_log.end());
    consumption_log.erase(it, consumption_log.end());
    EXPECT_THAT(consumption_log, SizeIs(nbuffers));
}

TEST_P(WithAnyNumberOfBuffers, compositor_can_always_get_a_buffer)
{
    std::vector<ScheduleEntry> schedule = {
        {0_t, {producer.get()}, {}},
        {1_t, {},          {consumer.get()}},
        {2_t, {},          {consumer.get()}},
        {3_t, {},          {consumer.get()}},
        {5_t, {},          {consumer.get()}},
        {6_t, {},          {consumer.get()}},
    };
    run_system(schedule);

    auto consumption_log = consumer->consumption_log();
    ASSERT_THAT(consumption_log, SizeIs(5));
}

TEST_P(WithTwoOrMoreBuffers, compositor_doesnt_starve_from_slow_client)
{
    std::vector<ScheduleEntry> schedule = {
        {0_t,   {producer.get()}, {}},
        {1_t,   {},          {consumer.get()}},
        {60_t,  {},          {consumer.get()}},
        {120_t, {},          {consumer.get()}},
        {150_t, {producer.get()}, {}},
        {180_t, {},          {consumer.get()}},
        {240_t, {},          {consumer.get()}},
        {270_t, {producer.get()}, {}},
        {300_t, {},          {consumer.get()}},
        {360_t, {},          {consumer.get()}},
    };
    run_system(schedule);

    auto consumption_log = consumer->consumption_log();
    ASSERT_THAT(consumption_log, SizeIs(7));
    EXPECT_THAT(consumption_log[1], Eq(consumption_log[0]));
    EXPECT_THAT(consumption_log[2], Eq(consumption_log[0]));
    EXPECT_THAT(consumption_log[4], Eq(consumption_log[3]));
    EXPECT_THAT(consumption_log[6], Eq(consumption_log[5]));
}

TEST_P(WithTwoOrMoreBuffers, multiple_consumers_are_in_sync)
{
    std::vector<ScheduleEntry> schedule = {
        {0_t,     {producer.get()}, {}},
        {1_t,     {},          {consumer.get()}},
        {60_t,    {},          {consumer.get(), second_consumer.get()}},
        {119_t,   {},          {consumer.get()}},
        {120_t,   {},          {second_consumer.get()}},
        {130_t,   {producer.get()}, {}},
        {178_t,   {},          {consumer.get()}},
        {180_t,   {},          {second_consumer.get()}},
        {237_t,   {},          {consumer.get()}},
        {240_t,   {},          {second_consumer.get()}},
    };
    run_system(schedule);

    auto production_log = producer->production_log();
    auto consumption_log_1 = consumer->consumption_log();
    auto consumption_log_2 = second_consumer->consumption_log();
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
        {0_t,     {producer.get()}, {}},
        {1_t,     {producer.get()}, {}},
        {60_t,    {},          {consumer.get(), second_consumer.get()}},
        {61_t,    {},          {consumer.get(), second_consumer.get()}},
    };
    run_system(schedule);

    auto production_log = producer->production_log();
    auto consumption_log_1 = consumer->consumption_log();
    auto consumption_log_2 = second_consumer->consumption_log();
    EXPECT_THAT(consumption_log_1, Eq(production_log));
    EXPECT_THAT(consumption_log_2, Eq(production_log));
}

TEST_P(WithTwoOrMoreBuffers, framedropping_clients_dont_block)
{
    allow_framedropping();
    std::vector<ScheduleEntry> schedule;
    for (auto i = 0; i < nbuffers * 3; i++)
        schedule.emplace_back(ScheduleEntry{1_t, {producer.get()}, {}});
    run_system(schedule);

    auto production_log = producer->production_log();
    auto block_count = std::count_if(production_log.begin(), production_log.end(),
        [](BufferEntry const& e) { return e.blockage == Access::blocked; });
    EXPECT_THAT(block_count, Eq(0));
}

TEST_P(WithTwoOrMoreBuffers, nonframedropping_client_throttles_to_compositor_rate)
{
    unsigned int reps = 50;
    auto const expected_blocks = reps - nbuffers;
    std::vector<ScheduleEntry> schedule = {
        {1_t,  {producer.get(), producer.get()}, {consumer.get()}},
    };
    repeat_system_until(schedule, [&reps]{ return --reps != 0; });

    auto log = producer->production_log();
    auto block_count = std::count_if(log.begin(), log.end(),
        [](BufferEntry const& e) { return e.blockage == Access::blocked; });
    EXPECT_THAT(block_count, Ge(expected_blocks));
}

TEST_P(WithAnyNumberOfBuffers, resize_affects_client_acquires_immediately)
{
    unsigned int const sizes_to_test{4};
    geom::Size new_size = properties.size;
    producer->produce();
    consumer->consume();
    for(auto i = 0u; i < sizes_to_test; i++)
    {
        new_size = new_size * 2;
        resize(new_size);
        std::vector<ScheduleEntry> schedule = {{1_t,  {producer.get()}, {consumer.get()}}};
        run_system(schedule);
        EXPECT_THAT(producer->last_size(), Eq(new_size));
    }
}

TEST_P(WithAnyNumberOfBuffers, compositor_acquires_resized_frames)
{
    unsigned int const sizes_to_test{4};
    int const attempt_limit{100};
    geom::Size new_size = properties.size;
    producer->produce();
    for(auto i = 0u; i < sizes_to_test; i++)
    {
        new_size = new_size * 2;
        consumer->consume();
        resize(new_size);

        std::vector<ScheduleEntry> schedule = {
            {1_t,  {producer.get()}, {}},
            {2_t,  {}, {consumer.get()}},
            {3_t,  {producer.get()}, {}},
        };
        run_system(schedule);

        int attempt_count = 0;
        schedule = {{2_t,  {}, {consumer.get()}}};
        repeat_system_until(schedule, [&] {
            return (consumer->last_size() != new_size) && (attempt_count++ < attempt_limit); });

        ASSERT_THAT(attempt_count, Lt(attempt_limit)) << "consumer never got the new size";
    }
}

TEST_P(WithTwoOrMoreBuffers, client_is_not_woken_by_compositor_release)
{
    // If early release is accidentally active, make sure we see it. But it
    // requires a dummy frame first:
    producer->produce();
    auto onscreen = stream->lock_compositor_buffer(this);
    onscreen.reset();

    while (producer->can_produce())
        producer->produce();

    ASSERT_FALSE(producer->can_produce());
    onscreen = stream->lock_compositor_buffer(this);

    // This varies between NBS and BufferQueue. Should it?
    if (producer->can_produce())
        producer->produce();
    ASSERT_FALSE(producer->can_produce());

    onscreen.reset();
    ASSERT_FALSE(producer->can_produce());
}

// Regression test for LP: #1319765
TEST_P(WithTwoBuffers, client_is_not_blocked_prematurely)
{
    producer->produce();
    auto a = stream->lock_compositor_buffer(this);
    producer->produce();
    auto b = stream->lock_compositor_buffer(this);

    ASSERT_NE(a.get(), b.get());

    a.reset();
    producer->produce();
    b.reset();

    /*
     * Update to the original test case; This additional compositor acquire
     * represents the fixing of LP: #1395581 in the compositor logic.
     */
    if (stream->buffers_ready_for_compositor(this))
        stream->lock_compositor_buffer(this);

    // With the fix, a buffer will be available instantaneously:
    EXPECT_TRUE(producer->can_produce());
}

// Extended regression test for LP: #1319765
TEST_P(WithTwoBuffers, composite_on_demand_never_deadlocks)
{
    for (int i = 0; i < 100; ++i)
    {
        producer->produce();
        auto a = stream->lock_compositor_buffer(this);
        producer->produce();
        auto b = stream->lock_compositor_buffer(this);
    
        ASSERT_NE(a, b);

        a.reset(); 
        producer->produce();
        b.reset(); 

        /*
         * Update to the original test case; This additional compositor acquire
         * represents the fixing of LP: #1395581 in the compositor logic.
         */
        if (stream->buffers_ready_for_compositor(this))
            stream->lock_compositor_buffer(this);

        EXPECT_TRUE(producer->can_produce());

        consumer->consume();
        consumer->consume();
    }
}

// Regression test for LP: #1395581
TEST_P(WithTwoOrMoreBuffers, buffers_ready_is_not_underestimated)
{
    // Produce frame 1
    producer->produce();
    // Acquire frame 1
    auto a = stream->lock_compositor_buffer(this);

    // Produce frame 2
    producer->produce();
    // Acquire frame 2
    auto b = stream->lock_compositor_buffer(this);
    // Release frame 1
    a.reset();
    // Produce frame 3
    producer->produce();
    // Release frame 2
    b.reset();

    // Verify frame 3 is ready for the first compositor
    EXPECT_THAT(stream->buffers_ready_for_compositor(this), Ge(1));
    auto c = stream->lock_compositor_buffer(this);

    // Verify frame 3 is ready for a second compositor
    int const other_compositor_id = 0;
    ASSERT_THAT(stream->buffers_ready_for_compositor(&other_compositor_id), Ge(1));

    c.reset();
}

TEST_P(WithTwoOrMoreBuffers, buffers_ready_eventually_reaches_zero)
{
    const int nmonitors = 3;
    std::array<ConsumerSystem*, nmonitors> consumers { {
        consumer.get(),
        second_consumer.get(),
        third_consumer.get()
    } };

    for (auto const& consumer : consumers)
        EXPECT_EQ(0, stream->buffers_ready_for_compositor(consumer));

    producer->produce();
    for (auto consumer : consumers)
    {
        ASSERT_NE(0, stream->buffers_ready_for_compositor(consumer));

        // Multi-consume to account for the extra that
        // buffers_ready_for_compositor adds to do dynamic performance
        // detection.
        int const max_extra_scheduling = 50;
        for (int c = 0; c < max_extra_scheduling; ++c)
            consumer->consume();

        ASSERT_EQ(0, stream->buffers_ready_for_compositor(consumer));
    }
}

TEST_P(WithThreeBuffers, gives_compositor_a_valid_buffer_after_dropping_old_buffers_without_clients)
{
    producer->produce();
    stream->drop_old_buffers();
    consumer->consume();
    EXPECT_THAT(consumer->consumption_log(), SizeIs(1));
}

TEST_P(WithThreeBuffers, gives_new_compositor_the_newest_buffer_after_dropping_old_buffers)
{
    producer->produce();
    consumer->consume();
    producer->produce();
    stream->drop_old_buffers();

    second_consumer->consume();

    auto production_log = producer->production_log();
    auto consumption_log = consumer->consumption_log();
    auto second_consumption_log = second_consumer->consumption_log();
    ASSERT_THAT(production_log, SizeIs(2));
    ASSERT_THAT(consumption_log, SizeIs(1));
    ASSERT_THAT(second_consumption_log, SizeIs(1));

    EXPECT_THAT(production_log[0], Eq(consumption_log[0]));
    EXPECT_THAT(production_log[1], Eq(second_consumption_log[0]));
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
        producer->produce();
        compositor_resources[oldest] = consumer->consume_resource();
    }

    // Two compositors acquired, and they're always different...
    auto log = consumer->consumption_log();
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
    disallow_framedropping();

    //fill up queue at first
    for(auto i = 0; i < nbuffers - 1; i++)
        producer->produce();

    //a schedule that would block once per iteration for double buffering, but only once for >3 buffers
    std::vector<ScheduleEntry> schedule = {
        {0_t,  {}, {consumer.get()}},
        {59_t,  {producer.get()}, {}},
        {60_t,  {}, {consumer.get()}},
        {120_t,  {}, {consumer.get()}},
        {121_t,  {producer.get()}, {}},
        {179_t,  {producer.get()}, {}},
        {180_t,  {}, {consumer.get()}},
        {240_t,  {}, {consumer.get()}},
        {241_t,  {producer.get()}, {}},
        {300_t,  {}, {consumer.get()}},
    };

    auto count = 0u;
    repeat_system_until(schedule, [&]{ return count++ < schedule.size() * iterations; });

    auto log = producer->production_log();
    auto blockages = std::count_if(log.begin(), log.end(),
        [](BufferEntry const& e){ return e.blockage == Access::blocked; });
    EXPECT_THAT(blockages, Le(1));
}

//regression test for LP: #1396006, LP: #1379685
TEST_P(WithTwoOrMoreBuffers, framedropping_surface_never_drops_newest_frame)
{
    allow_framedropping();

    for (int f = 0; f < nbuffers; ++f)
        producer->produce();

    for (int n = 0; n < nbuffers - 1; ++n)
        consumer->consume();

    // Ensure it's not the newest frame that gets dropped to satisfy the
    // client.
    producer->produce();
    consumer->consume();

    // The queue could solve this problem a few ways. It might choose to
    // defer framedropping till it's safe, or even allocate additional
    // buffers. We don't care which, just verify it's not losing the
    // latest frame. Because the screen could be indefinitely out of date
    // if that happens...
    auto producer_log = producer->production_log();
    auto consumer_log = consumer->consumption_log();
    EXPECT_TRUE(!producer->can_produce() || 
        (!producer_log.empty() && !consumer_log.empty() && producer_log.back() == consumer_log.back()));
}

/* Regression test for LP: #1306464 */
TEST_P(WithThreeBuffers, framedropping_client_acquire_does_not_block_when_no_available_buffers)
{
    allow_framedropping();
    producer->produce();

    /* The client can never own this acquired buffer */
    auto comp_buffer = consumer->consume_resource();

    /* Let client release all possible buffers so they go into
     * the ready queue
     */
    for (int i = 0; i < nbuffers; ++i)
    {
        producer->produce();
        EXPECT_THAT(comp_buffer->id(), Ne(producer->current_id()));
    }

    /* Let the compositor acquire all ready buffers */
    for (int i = 0; i < nbuffers; ++i)
        consumer->consume();

    /* At this point the queue has 0 free buffers and 0 ready buffers
     * so the next client request should not be satisfied until
     * a compositor releases its buffers */
    /* ... unless the BufferQueue is overallocating. In that case it will
     * have succeeding in acquiring immediately.
     */
    EXPECT_TRUE(producer->can_produce());
}

TEST_P(WithTwoOrMoreBuffers, client_never_owns_compositor_buffers_and_vice_versa)
{
    producer->produce();
    for (int i = 0; i < 100; ++i)
    {
        auto buffer = consumer->consume_resource();
        producer->produce();
        EXPECT_THAT(buffer->id(), Ne(producer->current_id()));
    }
}

/* Regression test for an issue brought up at:
 * http://code.launchpad.net/~albaguirre/mir/
 * alternative-switching-bundle-implementation/+merge/216606/comments/517048
 */
TEST_P(WithThreeOrMoreBuffers, buffers_are_not_lost)
{
    const int nmonitors = 2;
    std::array<ConsumerSystem*, nmonitors> consumers { {
        consumer.get(),
        second_consumer.get(),
    } };
    producer->produce();

    /* Hold a reference to current compositor buffer*/
    auto comp_buffer1 = consumers[0]->consume_resource();

    while (producer->can_produce())
        producer->produce();

    /* Have a second compositor advance the current compositor buffer at least twice */
    for (int acquires = 0; acquires < nbuffers; ++acquires)
        consumers[1]->consume();

    comp_buffer1.reset();

    /* An async client should still be able to cycle through all the available
     * buffers. "async" means any pattern other than "produce,consume,..."
     */
    int const max_ownable_buffers = nbuffers - 1;
    producer->reset_log();
    for (int frame = 0; frame < max_ownable_buffers * 2; frame++)
    {
        for (int drain = 0; drain < nbuffers; ++drain)
            consumers[0]->consume();
        while (producer->can_produce())
            producer->produce();
    }

    EXPECT_THAT(unique_ids_in(producer->production_log()), Eq(nbuffers));
}

// Test that dynamic queue scaling/throttling actually works
TEST_P(WithThreeOrMoreBuffers, queue_size_scales_with_client_performance)
{
    auto const discard = 3u;

    for (int frame = 0; frame < 20; frame++)
    {
        producer->produce();
        consumer->consume();
    }
    // Expect double-buffers as the steady state for fast clients
    auto log = producer->production_log();
    ASSERT_THAT(log.size(), Gt(discard));  // avoid the below erase crashing
    log.erase(log.begin(), log.begin() + discard);
    EXPECT_THAT(log, NeverBlocks());

    producer->reset_log();

    //put server-side pressure on the buffer count
    std::shared_ptr<mg::Buffer> a;
    std::shared_ptr<mg::Buffer> b;
    producer->produce();
    producer->produce();
    for (int frame = 0; frame < 20; frame++)
    {
        producer->produce();
        a = consumer->consume_resource();
        b = consumer->consume_resource();
    }
    a.reset();
    b.reset();

    log = producer->production_log();
    EXPECT_THAT(unique_ids_in(log), Ge(3));
    producer->reset_log();

    // And what happens if the client becomes fast again?...
    for (int frame = 0; frame < 20; frame++)
    {
        producer->produce();
        consumer->consume();
    }
    // Expect double-buffers as the steady state for fast clients
    log = producer->production_log();
    log.erase(log.begin(), log.begin() + discard);
    EXPECT_THAT(log, NeverBlocks());
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

    std::shared_ptr<mg::Buffer> first;
    std::shared_ptr<mg::Buffer> second;
    producer->produce();
    producer->produce();
    for (auto i = 0u; i < 100u; i++)
    {
        first = consumer->consume_resource();
        second = consumer->consume_resource();
        producer->produce();
    }

    EXPECT_THAT(unique_ids_in(producer->production_log()), Ge(3));
}

TEST_P(WithAnyNumberOfBuffers, can_snapshot_repeatedly_without_blocking)
{
    producer->produce();
    consumer->consume();
    auto const num_snapshots = nbuffers * 2u;
    std::vector<mg::BufferID> snaps(num_snapshots);
    for(auto i = 0u; i < num_snapshots; i++)
    {
        stream->with_most_recent_buffer_do([i, &snaps](mg::Buffer& buffer)
        {
            snaps[i] = buffer.id();
        });
    }

    auto production_log = producer->production_log();
    ASSERT_THAT(production_log, SizeIs(1));
    EXPECT_THAT(snaps, Each(production_log.back().id));
}

//LP: #1578159
//If given the choice best to prefer buffers that the compositor used furthest in the past
//so that we avert any waits on synchronization internal to the buffers or platform.
TEST_P(WithThreeOrMoreBuffers, prefers_fifo_ordering_when_distributing_buffers_to_driver)
{ 
    producer->produce();
    producer->produce();
    consumer->consume();
    consumer->consume();
    producer->produce();

    auto production_log = producer->production_log();
    auto consumption_log = consumer->consumption_log();
    ASSERT_THAT(production_log, Not(IsEmpty()));
    ASSERT_THAT(consumption_log, Not(IsEmpty()));
    EXPECT_THAT(production_log.back().id, Ne(consumption_log.front().id));
}

int const max_buffers_to_test{5};
INSTANTIATE_TEST_CASE_P(
    BufferScheduling,
    WithAnyNumberOfBuffers,
    Range(2, max_buffers_to_test));
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
    WithTwoBuffers,
    Values(2));
INSTANTIATE_TEST_CASE_P(
    BufferScheduling,
    WithThreeBuffers,
    Values(3));
