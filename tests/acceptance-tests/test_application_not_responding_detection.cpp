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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */


#include "mir_test_framework/connected_client_headless_server.h"

#include "mir/scene/application_not_responding_detector.h"
#include "mir/scene/session.h"

#include <thread>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mtf = mir_test_framework;
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
    MOCK_METHOD2(register_session, void(mir::scene::Session const&, std::function<void ()> const&));
    MOCK_METHOD1(unregister_session, void(mir::scene::Session const&));
    MOCK_METHOD1(pong_received, void(mir::scene::Session const&));

    MOCK_METHOD1(register_observer, void(std::shared_ptr<Observer> const&));
};
}

TEST_F(ApplicationNotRespondingDetection, failure_to_pong_is_noticed)
{
    using namespace std::literals::chrono_literals;

    complete_setup();

    bool unresponsive_called{false};
    auto anr_observer = std::make_shared<DelegateObserver>(
        [&unresponsive_called](auto) { unresponsive_called = true; },
        [](auto){}
    );

    server.the_application_not_responding_detector()->register_observer(anr_observer);

    std::this_thread::sleep_for(3s);

    EXPECT_TRUE(unresponsive_called);
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
