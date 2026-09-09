/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mock_global_factory.h"
#include "wayland_rs_server_test.h"

#include "client.h"
#include "wayland.h"
#include "weak.h"

#include <mir/synchronised.h>
#include <mir/test/signal.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <wayland-client.h>

#include <sys/mman.h>
#include <sys/socket.h>

#include <chrono>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>

namespace mwrs = mir::wayland_rs;

using namespace testing;
using namespace std::chrono_literals;

namespace
{
/// A `wl_shm.format` a test server would never otherwise advertise, so a test
/// can be sure the event it observes is the one sent from `~TestShm`. It has to
/// be a value the protocol's format enum knows, because the generated event
/// setter maps anything else onto the enum's fallback.
uint32_t constexpr sentinel_format = mwrs::Shm::Format::yvu444;

/// Collects everything libwayland's client library logs. libwayland reports
/// several classes of server misbehaviour (notably a `wl_display.delete_id`
/// for an id it has already released) as a log line rather than a protocol
/// error, so an empty log is part of what these tests assert.
mir::Synchronised<std::vector<std::string>>& client_log()
{
    static mir::Synchronised<std::vector<std::string>> log;
    return log;
}

void record_client_log(char const* fmt, va_list args)
{
    char buffer[512];
    std::vsnprintf(buffer, sizeof buffer, fmt, args);
    client_log().lock()->push_back(buffer);
}

void print_client_log(char const* fmt, va_list args)
{
    std::vfprintf(stderr, fmt, args);
}

class TestClient : public mwrs::Client
{
public:
    explicit TestClient(mwrs::RawWlClient raw) :
        raw{std::move(raw)}
    {
    }

    auto raw_client() const -> mwrs::RawWlClient const& override { return raw; }
    auto is_being_destroyed() const -> bool override { return false; }
    auto client_session() const -> std::shared_ptr<mir::scene::Session> override { return nullptr; }
    auto next_serial(std::shared_ptr<MirEvent const>) -> uint32_t override { return 0; }
    auto event_for(uint32_t) -> std::optional<std::shared_ptr<MirEvent const>> override { return std::nullopt; }
    void set_output_geometry_scale(float) override {}
    auto output_geometry_scale() -> float override { return 1.0f; }

private:
    mwrs::RawWlClient raw;
};

struct ServerObservations
{
    mir::Synchronised<int> shm_destructions{0};
    mir::Synchronised<int> pool_destructions{0};
    mir::test::Signal shm_created;
    mir::test::Signal shm_destroyed;
    mir::test::Signal pool_created;
    mir::test::Signal pool_destroyed;

    /// Only touched on the Wayland thread.
    mwrs::Weak<mwrs::ShmPool> pool;
};

class TestShmPool : public mwrs::ShmPool
{
public:
    TestShmPool(
        std::shared_ptr<mwrs::Client> client,
        rust::Box<mwrs::ShmPoolMiddleware> instance,
        uint32_t object_id,
        ServerObservations& observations) :
        ShmPool{std::move(client), std::move(instance), object_id},
        observations{observations}
    {
    }

    ~TestShmPool()
    {
        ++*observations.pool_destructions.lock();
        observations.pool_destroyed.raise();
    }

    auto create_buffer(int32_t, int32_t, int32_t, int32_t, uint32_t, rust::Box<mwrs::BufferMiddleware>, uint32_t)
        -> std::shared_ptr<mwrs::Buffer> override
    {
        return nullptr;
    }

    void resize(int32_t) override {}

private:
    ServerObservations& observations;
};

class TestShm : public mwrs::Shm
{
public:
    TestShm(
        std::shared_ptr<mwrs::Client> client,
        rust::Box<mwrs::ShmMiddleware> instance,
        uint32_t object_id,
        ServerObservations& observations) :
        Shm{std::move(client), std::move(instance), object_id},
        observations{observations}
    {
    }

    /// Sends an event referring to this object from a destructor. This is only
    /// valid — and only reaches the client — if the underlying Wayland resource
    /// outlives the C++ destructor chain.
    ~TestShm()
    {
        send_format_event(sentinel_format);
        ++*observations.shm_destructions.lock();
        observations.shm_destroyed.raise();
    }

    /// `fd` is borrowed for the duration of the call, so it is not closed here.
    auto create_pool(int32_t, int32_t, rust::Box<mwrs::ShmPoolMiddleware> child_instance, uint32_t child_object_id)
        -> std::shared_ptr<mwrs::ShmPool> override
    {
        auto pool = std::make_shared<TestShmPool>(client, std::move(child_instance), child_object_id, observations);
        observations.pool = mwrs::Weak<mwrs::ShmPool>{pool};
        observations.pool_created.raise();
        return pool;
    }

private:
    ServerObservations& observations;
};

class TestGlobalFactory : public NiceMock<mwrs::test::MockGlobalFactory>
{
public:
    explicit TestGlobalFactory(ServerObservations& observations) :
        observations{observations}
    {
        ON_CALL(*this, can_view(_, _))
            .WillByDefault([](rust::Str interface_name, rust::Box<mwrs::WaylandClientId>)
                { return interface_name == "wl_shm"; });

        ON_CALL(*this, create_wl_shm(_, _, _))
            .WillByDefault(
                [this](rust::Box<mwrs::WaylandClient> client, rust::Box<mwrs::ShmMiddleware> instance, uint32_t object_id)
                {
                    auto shm = std::make_shared<TestShm>(
                        std::make_shared<TestClient>(std::move(client)),
                        std::move(instance),
                        object_id,
                        this->observations);

                    shm_weak = mwrs::Weak<mwrs::Shm>{shm};
                    shm_shared = shm;
                    this->observations.shm_created.raise();
                    return shm;
                });
    }

    mwrs::Weak<mwrs::Shm> shm_weak;
    std::weak_ptr<mwrs::Shm> shm_shared;

private:
    ServerObservations& observations;
};

auto make_socket_pair() -> std::pair<int, int>
{
    int fds[2]{-1, -1};
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0)
        throw std::runtime_error{"Failed to create socket pair"};

    return {fds[0], fds[1]};
}

class DestroyOrderingTest : public mwrs::test::RunningWaylandServerTest
{
public:
    auto make_global_factory() -> std::unique_ptr<mwrs::GlobalFactory> override
    {
        auto owned = std::make_unique<TestGlobalFactory>(observations);
        factory = owned.get();
        return owned;
    }

    void SetUp() override
    {
        client_log().lock()->clear();
        wl_log_set_handler_client(&record_client_log);

        RunningWaylandServerTest::SetUp();

        auto const [server_fd, client_fd] = make_socket_pair();

        // Ownership of the client end transfers to the wl_display. The server
        // end transfers to the server via insert_client.
        display = wl_display_connect_to_fd(client_fd);
        if (!display)
        {
            ::close(server_fd);
            FAIL() << "Failed to connect Wayland client to injected fd";
        }
        (*server)->insert_client(server_fd);

        auto* const registry = wl_display_get_registry(display);
        wl_registry_add_listener(registry, &registry_listener, this);
        ASSERT_NE(wl_display_roundtrip(display), -1);
        ASSERT_TRUE(shm) << "The server did not advertise wl_shm";

        wl_shm_add_listener(shm, &shm_listener, this);
        ASSERT_NE(wl_display_roundtrip(display), -1);
        ASSERT_TRUE(observations.shm_created.wait_for(5s)) << "The server did not create the wl_shm object";
    }

    void TearDown() override
    {
        if (shm)
            wl_shm_destroy(shm);
        if (display)
            wl_display_disconnect(display);

        RunningWaylandServerTest::TearDown();

        wl_log_set_handler_client(&print_client_log);
        client_log().lock()->clear();
    }

    /// Run `work` on the Wayland event loop and wait for it to complete.
    template<typename Work>
    void on_wayland_thread(Work&& work)
    {
        mir::test::Signal done;
        executor->spawn(
            [&]
            {
                work();
                done.raise();
            });
        ASSERT_TRUE(done.wait_for(5s)) << "Work scheduled on the Wayland event loop did not run";
    }

    auto formats() -> std::vector<uint32_t>
    {
        return *received_formats.lock();
    }

    auto client_log_contents() -> std::vector<std::string>
    {
        return *client_log().lock();
    }

    /// A memfd suitable for wl_shm.create_pool. The fd is handed to libwayland,
    /// which does not take ownership, so the caller closes it.
    static auto make_pool_fd() -> int
    {
        auto const fd = ::memfd_create("mir-wayland-rs-test-pool", MFD_CLOEXEC);
        if (fd < 0 || ::ftruncate(fd, 4) != 0)
            throw std::runtime_error{"Failed to create a memfd for wl_shm_pool"};

        return fd;
    }

    ServerObservations observations;
    TestGlobalFactory* factory{nullptr};
    wl_display* display{nullptr};
    wl_shm* shm{nullptr};
    mir::Synchronised<std::vector<uint32_t>> received_formats;

private:
    static void handle_global(void* data, wl_registry* registry, uint32_t name, char const* interface, uint32_t version)
    {
        auto* const self = static_cast<DestroyOrderingTest*>(data);
        if (std::string{interface} == wl_shm_interface.name)
            self->shm = static_cast<wl_shm*>(wl_registry_bind(registry, name, &wl_shm_interface, version));
    }

    static void handle_format(void* data, wl_shm*, uint32_t format)
    {
        static_cast<DestroyOrderingTest*>(data)->received_formats.lock()->push_back(format);
    }

    static constexpr wl_registry_listener registry_listener{&handle_global, nullptr};
    static constexpr wl_shm_listener shm_listener{&handle_format};
};
}

TEST_F(DestroyOrderingTest, event_sent_from_destructor_reaches_client_on_server_initiated_destroy)
{
    on_wayland_thread([this] { factory->shm_weak.value().destroy_and_delete(); });

    ASSERT_TRUE(observations.shm_destroyed.wait_for(5s)) << "The wl_shm object was never destroyed";
    ASSERT_NE(wl_display_roundtrip(display), -1);

    EXPECT_THAT(formats(), Contains(sentinel_format));
    EXPECT_THAT(*observations.shm_destructions.lock(), Eq(1));
    EXPECT_THAT(wl_display_get_error(display), Eq(0));
    EXPECT_THAT(client_log_contents(), IsEmpty());
}

TEST_F(DestroyOrderingTest, event_sent_from_destructor_reaches_client_when_destroy_is_deferred_by_a_strong_reference)
{
    // This test is similar to the previous one, with the caveat that the C++
    // object is dropped after destroy_and_delete() returns. We are asserting
    // here that we still have a "live" resource to send events on in this case.
    on_wayland_thread(
        [this]
        {
            auto const keep_alive = factory->shm_shared.lock();
            ASSERT_TRUE(keep_alive);

            keep_alive->destroy_and_delete();
            EXPECT_THAT(*observations.shm_destructions.lock(), Eq(0))
                << "destroy_and_delete() destroyed the object while a strong reference was held";
        });

    ASSERT_TRUE(observations.shm_destroyed.wait_for(5s)) << "The wl_shm object was never destroyed";
    ASSERT_NE(wl_display_roundtrip(display), -1);

    EXPECT_THAT(formats(), Contains(sentinel_format));
    EXPECT_THAT(*observations.shm_destructions.lock(), Eq(1));
    EXPECT_THAT(wl_display_get_error(display), Eq(0));
    EXPECT_THAT(client_log_contents(), IsEmpty());
}

TEST_F(DestroyOrderingTest, client_initiated_destroy_destroys_the_object_exactly_once)
{
    auto const fd = make_pool_fd();
    auto* const pool = wl_shm_create_pool(shm, fd, 4);
    ::close(fd);
    ASSERT_NE(wl_display_roundtrip(display), -1);
    ASSERT_TRUE(observations.pool_created.wait_for(5s)) << "The server did not create the wl_shm_pool object";

    wl_shm_pool_destroy(pool);
    ASSERT_NE(wl_display_roundtrip(display), -1);
    ASSERT_TRUE(observations.pool_destroyed.wait_for(5s)) << "The wl_shm_pool object was never destroyed";

    // The object is gone server-side, and the connection saw neither a protocol
    // error nor a duplicate delete_id.
    on_wayland_thread([this] { EXPECT_FALSE(observations.pool); });
    EXPECT_THAT(*observations.pool_destructions.lock(), Eq(1));
    EXPECT_THAT(wl_display_get_error(display), Eq(0));
    EXPECT_THAT(client_log_contents(), IsEmpty());

    // The connection is still usable afterwards.
    auto const second_fd = make_pool_fd();
    auto* const second_pool = wl_shm_create_pool(shm, second_fd, 4);
    ::close(second_fd);
    ASSERT_NE(wl_display_roundtrip(display), -1);
    EXPECT_THAT(wl_display_get_error(display), Eq(0));
    EXPECT_THAT(client_log_contents(), IsEmpty());

    wl_shm_pool_destroy(second_pool);
}
