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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */


#include "mir_test_framework/connected_client_headless_server.h"

#include "mir/scene/application_not_responding_detector.h"
#include "mir/scene/session.h"

#include "mir/test/signal.h"
#include "mir/test/validity_matchers.h"

#include <thread>
#include <atomic>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mtf = mir_test_framework;
namespace mt = mir::test;
namespace ms = mir::scene;

namespace
{
class ApplicationNotRespondingDetection : public mtf::ConnectedClientHeadlessServer
{
public:
    void SetUp() override
    {
        // Deliberately fail to start up the server so we can override server config
        // in each test.
    }

    void complete_setup()
    {
        mtf::ConnectedClientHeadlessServer::SetUp();
    }

    void tear_down()
    {
        mtf::ConnectedClientHeadlessServer::TearDown();
        torn_down = true;
    }

    void TearDown() override
    {
        if (!torn_down)
        {
            tear_down();
        }
    }

private:
    bool torn_down{false};
};

class DelegateObserver : public ms::ApplicationNotRespondingDetector::Observer
{
public:
    DelegateObserver(
        std::function<void(ms::Session const*)> const& unresponsive_delegate,
        std::function<void(ms::Session const*)> const& now_responsive_delegate)
        : unresponsive_delegate{unresponsive_delegate},
          now_responsive_delegate{now_responsive_delegate}
    {
    }

    void session_unresponsive(mir::scene::Session const* session) override
    {
        unresponsive_delegate(session);
    }

    void session_now_responsive(mir::scene::Session const* session) override
    {
        now_responsive_delegate(session);
    }

private:
    std::function<void(ms::Session const*)> unresponsive_delegate;
    std::function<void(ms::Session const*)> now_responsive_delegate;
};

class MockANRDetector : public ms::ApplicationNotRespondingDetector
{
public:
    MOCK_METHOD2(register_session, void(mir::scene::Session const*, std::function<void ()> const&));
    MOCK_METHOD1(unregister_session, void(mir::scene::Session const*));
    MOCK_METHOD1(pong_received, void(mir::scene::Session const*));

    MOCK_METHOD1(register_observer, void(std::shared_ptr<Observer> const&));
    MOCK_METHOD1(unregister_observer, void(std::shared_ptr<Observer> const&));
};
}

namespace
{
void black_hole_pong(MirConnection*, int32_t, void*)
{
}
}

TEST_F(ApplicationNotRespondingDetection, failure_to_pong_is_noticed)
{
    using namespace std::literals::chrono_literals;

    complete_setup();

    mt::Signal marked_as_unresponsive;
    auto anr_observer = std::make_shared<DelegateObserver>(
        [&marked_as_unresponsive](auto) { marked_as_unresponsive.raise(); },
        [](auto){}
    );

    server.the_application_not_responding_detector()->register_observer(anr_observer);

    mir_connection_set_ping_event_callback(connection, &black_hole_pong, nullptr);

    EXPECT_TRUE(marked_as_unresponsive.wait_for(1min));
}

TEST_F(ApplicationNotRespondingDetection, can_override_anr_detector)
{
    using namespace std::literals::chrono_literals;
    using namespace testing;

    auto anr_detector = std::make_shared<NiceMock<MockANRDetector>>();
    server.override_the_application_not_responding_detector([anr_detector]() { return anr_detector; });

    EXPECT_CALL(*anr_detector, register_session(_,_)).Times(AtLeast(1));

    complete_setup();
}

TEST_F(ApplicationNotRespondingDetection, can_wrap_anr_detector)
{
    using namespace std::literals::chrono_literals;
    using namespace testing;

    int registration_count{0};
    int unregister_count{0};
    server.wrap_application_not_responding_detector(
        [&registration_count, &unregister_count](auto wrapee)
        {
            auto anr_detector = std::make_shared<NiceMock<MockANRDetector>>();
            ON_CALL(*anr_detector, register_session(_,_))
                .WillByDefault(Invoke([&registration_count, wrapee](auto a, auto b)
                    {
                        ++registration_count;
                        wrapee->register_session(a, b);
                    }));
            ON_CALL(*anr_detector, unregister_session(_))
                .WillByDefault(Invoke([&unregister_count, wrapee](auto a)
                    {
                        ++unregister_count;
                        wrapee->unregister_session(a);
                    }));
            ON_CALL(*anr_detector, pong_received(_))
                .WillByDefault(Invoke([wrapee](auto a) { wrapee->pong_received(a); }));

            ON_CALL(*anr_detector, register_observer(_))
                .WillByDefault(Invoke([wrapee](auto observer) { wrapee->register_observer(observer); }));
            ON_CALL(*anr_detector, unregister_observer(_))
                .WillByDefault(Invoke([wrapee](auto observer) { wrapee->unregister_observer(observer); }));

            return anr_detector;
        });

    EXPECT_THAT(registration_count, Eq(0));
    EXPECT_THAT(unregister_count, Eq(0));

    complete_setup();

    EXPECT_THAT(registration_count, Eq(1));
    EXPECT_THAT(unregister_count, Eq(0));

    // Verify that we wrap rather than replace by testing the default behaviour.
    mt::Signal marked_as_unresponsive;
    auto anr_observer = std::make_shared<DelegateObserver>(
        [&marked_as_unresponsive](auto) { marked_as_unresponsive.raise(); },
        [](auto){}
    );

    server.the_application_not_responding_detector()->register_observer(anr_observer);

    mir_connection_set_ping_event_callback(connection, &black_hole_pong, nullptr);

    EXPECT_TRUE(marked_as_unresponsive.wait_for(1min));


    tear_down();

    EXPECT_THAT(registration_count, Eq(1));
    EXPECT_THAT(unregister_count, Eq(1));
}

TEST_F(ApplicationNotRespondingDetection, each_new_client_is_registered)
{
    using namespace std::literals;
    using namespace testing;

    constexpr int const client_start_count = 10;
    constexpr int const expected_count = client_start_count + 1;  // Test fixture also connects.

    auto anr_detector = std::make_shared<NiceMock<MockANRDetector>>();
    server.override_the_application_not_responding_detector([anr_detector]() { return anr_detector; });

    EXPECT_CALL(*anr_detector, register_session(_,_)).Times(expected_count);
    // ConnectiedClientHeadlessServer::TearDown guarantees that the initial client is
    // disconnected before the server is torn down, so it should also be unregistered.
    EXPECT_CALL(*anr_detector, unregister_session(_)).Times(expected_count);

    complete_setup();

    for (int i = 0; i < client_start_count; ++i)
    {
        mir_connection_release(mir_connect_sync(new_connection().c_str(), ("Test client "s + std::to_string(i)).c_str()));
    }
}

namespace
{
struct PingContext
{
    int ping_count;
    mt::Signal pung_thrice;
};

void respond_to_ping(MirConnection* connection, int32_t, void* context)
{
    auto ping_context = reinterpret_cast<PingContext*>(context);
    ping_context->ping_count++;

    mir_connection_pong(connection, 0);

    if (ping_context->ping_count > 2)
    {
        // If a client does not respond to a ping for a whole ping cycle it is marked
        // as unresponsive. Wait for 2 ping cycles to ensure that the test has
        // had time to process the pongs.
        ping_context->pung_thrice.raise();
    }
}
}

TEST_F(ApplicationNotRespondingDetection, responding_client_is_not_marked_as_unresponsive)
{
    using namespace std::literals::chrono_literals;

    complete_setup();

    bool unresponsive_called{false};
    auto anr_observer = std::make_shared<DelegateObserver>(
        [&unresponsive_called](auto) { unresponsive_called = true; },
        [](auto){}
    );

    server.the_application_not_responding_detector()->register_observer(anr_observer);

    auto ping_context = std::make_unique<PingContext>();
    mir_connection_set_ping_event_callback(connection, &respond_to_ping, ping_context.get());

    EXPECT_TRUE(ping_context->pung_thrice.wait_for(1min));
    EXPECT_FALSE(unresponsive_called);
}

namespace
{
void ping_counting_blackhole_pong(MirConnection*, int32_t, void* ctx)
{
    auto count = reinterpret_cast<std::atomic<int>*>(ctx);
    (*count)++;
}
}

TEST_F(ApplicationNotRespondingDetection, unresponsive_client_stops_receiving_pings)
{
    using namespace std::literals::chrono_literals;
    using namespace testing;

    complete_setup();

    std::atomic<int> count{0};
    mir_connection_set_ping_event_callback(connection, &ping_counting_blackhole_pong, &count);

    auto responsive_connection = mir_connect_sync(new_connection().c_str(), "Aardvarks");
    ASSERT_THAT(responsive_connection, IsValid());

    auto ping_context = std::make_unique<PingContext>();
    mir_connection_set_ping_event_callback(responsive_connection, &respond_to_ping, ping_context.get());

    EXPECT_TRUE(ping_context->pung_thrice.wait_for(10s));

    EXPECT_THAT(count, Eq(1));

    mir_connection_release(responsive_connection);
}
