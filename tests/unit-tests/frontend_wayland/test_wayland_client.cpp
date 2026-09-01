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

#include "src/server/frontend_wayland/wayland_client.h"
#include "client.h"

#include <mir/events/event_builders.h>
#include <mir/scene/session.h>
#include <mir/shell/shell.h>
#include <mir/test/doubles/stub_session.h>
#include <mir/test/doubles/stub_shell.h>

#include <rust/cxx.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <optional>

namespace mf = mir::frontend;
namespace mrs = mir::wayland_rs;
namespace ms = mir::scene;
namespace mev = mir::events;
namespace mtd = mir::test::doubles;

using namespace testing;

namespace
{
/// `mf::WaylandClient` only stores the raw client and hands it back from
/// `raw_client()`, so it is fine to back it with a `nullptr`.
auto empty_raw_client() -> mrs::RawWlClient { return rust::Box<mrs::WaylandClient>::from_raw(nullptr); }

auto make_event() -> std::shared_ptr<MirEvent const>
{
    return mev::make_key_event(
        MirInputDeviceId{0},
        std::chrono::nanoseconds{0},
        mir_keyboard_action_down,
        0,
        7,
        mir_input_event_modifier_none);
}

struct MockShell : mtd::StubShell
{
    MOCK_METHOD(void, close_session, (std::shared_ptr<ms::Session> const&), (override));
};

struct WaylandClientTest : Test
{
    std::shared_ptr<NiceMock<MockShell>> const shell = std::make_shared<NiceMock<MockShell>>();
    std::shared_ptr<ms::Session> const session = std::make_shared<mtd::StubSession>();
    mf::WaylandSerialSource const serial_source = std::make_shared<std::atomic<uint32_t>>(0);

    auto make_client(std::shared_ptr<ms::Session> client_session) -> std::unique_ptr<mf::WaylandClient>
    {
        return std::make_unique<mf::WaylandClient>(empty_raw_client(), std::move(client_session), shell, serial_source);
    }

    auto make_client() -> std::unique_ptr<mf::WaylandClient> { return make_client(session); }
};
}

TEST_F(WaylandClientTest, client_session_returns_constructed_session)
{
    auto const client = make_client();
    EXPECT_EQ(client->client_session(), session);
}

TEST_F(WaylandClientTest, is_not_being_destroyed_initially)
{
    auto const client = make_client();
    EXPECT_FALSE(client->is_being_destroyed());
}

TEST_F(WaylandClientTest, output_geometry_scale_defaults_to_one)
{
    auto const client = make_client();
    EXPECT_FLOAT_EQ(client->output_geometry_scale(), 1.0f);
}

TEST_F(WaylandClientTest, output_geometry_scale_returns_set_value)
{
    auto const client = make_client();
    client->set_output_geometry_scale(2.5f);
    EXPECT_FLOAT_EQ(client->output_geometry_scale(), 2.5f);
}

TEST_F(WaylandClientTest, next_serial_increments_shared_source)
{
    auto const client = make_client();
    EXPECT_EQ(client->next_serial(nullptr), 1u);
    EXPECT_EQ(client->next_serial(nullptr), 2u);
    EXPECT_EQ(client->next_serial(nullptr), 3u);
}

TEST_F(WaylandClientTest, serials_are_shared_across_clients)
{
    auto const client_a = make_client();
    auto const client_b = make_client();

    EXPECT_EQ(client_a->next_serial(nullptr), 1u);
    EXPECT_EQ(client_b->next_serial(nullptr), 2u);
    EXPECT_EQ(client_a->next_serial(nullptr), 3u);
}

TEST_F(WaylandClientTest, event_for_returns_event_for_known_serial)
{
    auto const client = make_client();
    auto const event = make_event();

    auto const serial = client->next_serial(event);

    auto const retrieved = client->event_for(serial);
    EXPECT_EQ(*retrieved, event);
}

TEST_F(WaylandClientTest, event_for_returns_null_event_for_serial_recorded_with_null)
{
    auto const client = make_client();

    auto const serial = client->next_serial(nullptr);

    auto const retrieved = client->event_for(serial);
    EXPECT_EQ(*retrieved, nullptr);
}

TEST_F(WaylandClientTest, event_for_returns_nullopt_for_unknown_serial)
{
    auto const client = make_client();
    auto const serial = client->next_serial(make_event());

    EXPECT_FALSE(client->event_for(serial + 1));
}

TEST_F(WaylandClientTest, event_for_evicts_oldest_beyond_capacity)
{
    auto const client = make_client();

    int const capacity = 100;

    auto const first_serial = client->next_serial(make_event());
    for (int i = 1; i < capacity; ++i)
        client->next_serial(make_event());

    // The buffer is now exactly full; the oldest entry is still present.
    ASSERT_TRUE(client->event_for(first_serial));

    // One more issuance pushes the oldest out.
    auto const newest_serial = client->next_serial(make_event());

    EXPECT_FALSE(client->event_for(first_serial));
    EXPECT_TRUE(client->event_for(newest_serial));
}

TEST_F(WaylandClientTest, mark_being_destroyed_flags_and_closes_session)
{
    auto const client = make_client();

    EXPECT_CALL(*shell, close_session(session)).Times(1);

    client->mark_being_destroyed();

    EXPECT_TRUE(client->is_being_destroyed());
}

TEST_F(WaylandClientTest, mark_being_destroyed_is_idempotent)
{
    auto const client = make_client();

    EXPECT_CALL(*shell, close_session(session)).Times(1);

    client->mark_being_destroyed();
    client->mark_being_destroyed();
}

TEST_F(WaylandClientTest, mark_being_destroyed_with_null_session_does_not_close)
{
    auto const client = make_client(nullptr);

    EXPECT_CALL(*shell, close_session(_)).Times(0);

    client->mark_being_destroyed();

    EXPECT_TRUE(client->is_being_destroyed());
}

TEST_F(WaylandClientTest, destructor_closes_session)
{
    auto client = make_client();

    EXPECT_CALL(*shell, close_session(session)).Times(1);

    client.reset();
}

TEST_F(WaylandClientTest, destructor_does_not_close_session_already_marked)
{
    auto client = make_client();

    EXPECT_CALL(*shell, close_session(session)).Times(1);

    client->mark_being_destroyed();
    client.reset();
}
