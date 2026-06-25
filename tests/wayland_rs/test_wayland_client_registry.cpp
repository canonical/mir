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

#include "wayland_rs/src/ffi.rs.h"
#include "wayland_client_registry.h"
#include "wayland_executor.h"
#include "client.h"

#include <mir_test_framework/temporary_environment_value.h>

#include <gtest/gtest.h>

#include <wayland-client.h>

#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <system_error>
#include <thread>
#include <unistd.h>
#include <vector>

namespace mrs = mir::wayland_rs;
namespace mtf = mir_test_framework;

using namespace std::chrono_literals;

namespace
{
/// A minimal concrete Client for tests. The registry only ever calls
/// `raw_client()` (to compare client identity); the remaining members are not
/// exercised, so they return inert values.
class FakeClient : public mrs::Client
{
public:
    explicit FakeClient(mrs::RawWlClient raw) :
        raw{std::move(raw)}
    {
    }

    auto raw_client() const -> mrs::RawWlClient const& override { return raw; }
    auto is_being_destroyed() const -> bool override { return false; }
    auto client_session() const -> std::shared_ptr<mir::scene::Session> override { return nullptr; }
    auto next_serial(std::shared_ptr<MirEvent const>) -> uint32_t override { return 0; }
    auto event_for(uint32_t) -> std::optional<std::shared_ptr<MirEvent const>> override { return std::nullopt; }
    void set_output_geometry_scale(float) override {}
    auto output_geometry_scale() -> float override { return 1.0f; }

private:
    mrs::RawWlClient raw;
};

/// Captures the raw `WaylandClient` the server observes for every connecting
/// client, so tests can build `Client`s from genuine Rust client handles
/// (which can only be minted by the server, never fabricated in C++).
class CapturingNotificationHandler : public mrs::WaylandServerNotificationHandler
{
public:
    void client_added(rust::Box<mrs::WaylandClient> wayland_client) override
    {
        std::lock_guard<std::mutex> lock{mutex};
        clients.push_back(std::move(wayland_client));
        cv.notify_all();
    }

    void client_removed(rust::Box<mrs::WaylandClientId>) override {}

    /// Wait until at least `index + 1` clients have connected, then return a
    /// fresh box referring to the client at `index`.
    auto wait_for_client(std::size_t index) -> mrs::RawWlClient
    {
        std::unique_lock<std::mutex> lock{mutex};
        if (!cv.wait_for(lock, 5s, [&] { return clients.size() > index; }))
            throw std::runtime_error{"Server did not observe the connecting client"};
        return clients[index]->clone_box();
    }

private:
    std::mutex mutex;
    std::condition_variable cv;
    std::vector<rust::Box<mrs::WaylandClient>> clients;
};

auto make_temp_runtime_dir() -> std::string
{
    char template_path[] = "/tmp/mir-wayland-rs-test-XXXXXX";
    auto const dir = ::mkdtemp(template_path);
    if (!dir)
        throw std::runtime_error{"Failed to create temporary XDG_RUNTIME_DIR"};

    ::chmod(dir, 0700);
    return dir;
}

class WaylandClientRegistryTest : public testing::Test
{
public:
    void SetUp() override
    {
        runtime_dir = make_temp_runtime_dir();
        xdg_runtime_dir.emplace("XDG_RUNTIME_DIR", runtime_dir.c_str());

        server.emplace(mrs::create_wayland_server());
        auto executor = std::make_unique<mrs::WaylandExecutor>(**server);

        auto handler_owned = std::make_unique<CapturingNotificationHandler>();
        handler = handler_owned.get();

        socket = "mir-wayland-rs-test-" + std::to_string(::getpid());

        server_thread = std::thread{
            [this, executor = std::move(executor), handler = std::move(handler_owned)]() mutable
            {
                (*server)->run(socket, nullptr, std::move(handler), std::move(executor));
            }};
    }

    void TearDown() override
    {
        for (auto* display : displays)
            wl_display_disconnect(display);

        if (server)
            (*server)->stop();
        if (server_thread.joinable())
            server_thread.join();

        xdg_runtime_dir.reset();

        if (!runtime_dir.empty())
        {
            std::error_code ec;
            std::filesystem::remove_all(runtime_dir, ec);
        }
    }

    /// Connect a fresh real Wayland client and return the raw box the server saw
    /// for it. Clients are observed in connection order, so the nth call returns
    /// the nth distinct client.
    auto connect_client(std::size_t index) -> mrs::RawWlClient
    {
        // The server binds its socket asynchronously on its own thread; retry
        // until it is ready.
        auto const deadline = std::chrono::steady_clock::now() + 5s;
        wl_display* display{nullptr};
        while (!display && std::chrono::steady_clock::now() < deadline)
        {
            display = wl_display_connect(socket.c_str());
            if (!display)
                std::this_thread::sleep_for(50ms);
        }
        if (!display)
            throw std::runtime_error{"Failed to connect Wayland client"};

        displays.push_back(display);
        wl_display_roundtrip(display);

        return handler->wait_for_client(index);
    }

    std::string runtime_dir;
    std::optional<mtf::TemporaryEnvironmentValue> xdg_runtime_dir;
    std::optional<rust::Box<mrs::WaylandServer>> server;
    CapturingNotificationHandler* handler{nullptr};
    std::string socket;
    std::vector<wl_display*> displays;
    std::thread server_thread;
};
}

TEST_F(WaylandClientRegistryTest, from_resolves_registered_client_by_raw_client)
{
    mrs::WaylandClientRegistry registry;

    auto raw = connect_client(0);
    auto const query = raw->clone_box();
    auto const client = std::make_shared<FakeClient>(std::move(raw));
    registry.add_client(client);

    EXPECT_EQ(registry.from(query), client);
}

TEST_F(WaylandClientRegistryTest, from_resolves_registered_client_by_id)
{
    mrs::WaylandClientRegistry registry;

    auto raw = connect_client(0);
    auto const id = raw->id();
    auto const client = std::make_shared<FakeClient>(std::move(raw));
    registry.add_client(client);

    EXPECT_EQ(registry.from(id), client);
}

TEST_F(WaylandClientRegistryTest, from_returns_null_for_unregistered_client)
{
    mrs::WaylandClientRegistry registry;

    auto registered = connect_client(0);
    auto const other = connect_client(1);
    registry.add_client(std::make_shared<FakeClient>(std::move(registered)));

    EXPECT_EQ(registry.from(other), nullptr);
    EXPECT_EQ(registry.from(other->id()), nullptr);
}

TEST_F(WaylandClientRegistryTest, from_returns_null_when_registry_is_empty)
{
    mrs::WaylandClientRegistry registry;

    auto const raw = connect_client(0);

    EXPECT_EQ(registry.from(raw), nullptr);
    EXPECT_EQ(registry.from(raw->id()), nullptr);
}

TEST_F(WaylandClientRegistryTest, remove_client_removes_the_matching_client)
{
    mrs::WaylandClientRegistry registry;

    auto raw = connect_client(0);
    auto const id = raw->id();
    auto const client = std::make_shared<FakeClient>(std::move(raw));
    registry.add_client(client);
    ASSERT_EQ(registry.from(id), client);

    registry.remove_client(id);

    EXPECT_EQ(registry.from(id), nullptr);
}

TEST_F(WaylandClientRegistryTest, remove_client_leaves_other_clients_intact)
{
    mrs::WaylandClientRegistry registry;

    auto raw_a = connect_client(0);
    auto raw_b = connect_client(1);
    auto const id_a = raw_a->id();
    auto const id_b = raw_b->id();
    auto const client_b = std::make_shared<FakeClient>(std::move(raw_b));
    registry.add_client(std::make_shared<FakeClient>(std::move(raw_a)));
    registry.add_client(client_b);

    registry.remove_client(id_a);

    EXPECT_EQ(registry.from(id_a), nullptr);
    EXPECT_EQ(registry.from(id_b), client_b);
}
