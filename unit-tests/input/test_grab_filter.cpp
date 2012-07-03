/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#include "mir/application.h"
#include "mir/application_manager.h"
#include "mir/input/event.h"
#include "mir/input/grab_filter.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mi = mir::input;

namespace
{
template<typename T>
class MockFilter : public T
{
 public:
    MockFilter()
    {
        using namespace testing;

        ON_CALL(*this, accept(_)).WillByDefault(Invoke(this, &MockFilter::forward_accept));
    }

    template<typename... Types>
    MockFilter(Types&&... args) : T(std::forward<Types>(args)...)
    {
        using namespace testing;

        ON_CALL(*this, accept(_)).WillByDefault(Invoke(this, &MockFilter::forward_accept));
    }

    MOCK_CONST_METHOD1(accept, void(mi::Event*));

    void forward_accept(mi::Event* event) const { T::accept(event); }
};

class MockApplication : public mir::Application
{
 public:
    MockApplication(mir::ApplicationManager* manager) : Application(manager)
    {
    }

    MOCK_METHOD1(on_event, void(mi::Event*));
};

class MockEventHandler : public mi::EventHandler
{
 public:

    MOCK_METHOD1(on_event, void(mi::Event*));
};

struct MockApplicationManager : public mir::ApplicationManager
{
    MOCK_METHOD0(get_grabbing_application, std::weak_ptr<mir::Application>());
};

class DummyEvent : public mi::Event {};
}


TEST(GrabFilter, register_and_deregister_a_grab)
{
    mi::GrabFilter grab_filter{std::make_shared<mi::NullFilter>()};

    std::shared_ptr<mi::EventHandler> event_handler{std::make_shared<MockEventHandler>()};

    mi::GrabHandle grab_handle(grab_filter.push_grab(event_handler));

	grab_filter.release_grab(grab_handle);
}

TEST(GrabFilter, events_are_forwarded_to_next_filter)
{
	using namespace testing;
	typedef MockFilter<mi::NullFilter> MockNullFilter;

    std::shared_ptr<MockNullFilter> mock_null_filter{std::make_shared<MockNullFilter>()};

	mi::GrabFilter grab_filter{mock_null_filter};

    EXPECT_CALL(*mock_null_filter, accept(_)).Times(1);

    DummyEvent dummy_event;
    mi::Event* event = &dummy_event;
    grab_filter.accept(event);
}

TEST(GrabFilter, if_a_grab_is_registered_events_are_grabbed_not_forwarded)
{
	using namespace testing;
	typedef MockFilter<mi::NullFilter> MockNullFilter;

    std::shared_ptr<MockNullFilter> mock_null_filter{std::make_shared<MockNullFilter>()};

	mi::GrabFilter grab_filter{mock_null_filter};

    std::shared_ptr<MockEventHandler> mock_event_handler{std::make_shared<MockEventHandler>()};
    std::shared_ptr<mi::EventHandler> event_handler{mock_event_handler};

    mi::GrabHandle grab_handle(grab_filter.push_grab(event_handler));

    EXPECT_CALL(*mock_null_filter, accept(_)).Times(0);
    EXPECT_CALL(*mock_event_handler, on_event(_)).Times(1);

    DummyEvent dummy_event;
    mi::Event* event = &dummy_event;
    grab_filter.accept(event);

	grab_filter.release_grab(grab_handle);
}

TEST(GrabFilter, after_a_grab_is_released_events_are_forwarded_not_grabbed)
{
	using namespace testing;
	typedef MockFilter<mi::NullFilter> MockNullFilter;

    std::shared_ptr<MockNullFilter> mock_null_filter{std::make_shared<MockNullFilter>()};

	mi::GrabFilter grab_filter{mock_null_filter};

    std::shared_ptr<MockEventHandler> mock_event_handler{std::make_shared<MockEventHandler>()};
    std::shared_ptr<mi::EventHandler> event_handler{mock_event_handler};

    mi::GrabHandle grab_handle(grab_filter.push_grab(event_handler));

	grab_filter.release_grab(grab_handle);

	EXPECT_CALL(*mock_null_filter, accept(_)).Times(1);
    EXPECT_CALL(*mock_event_handler, on_event(_)).Times(0);

    DummyEvent dummy_event;
    mi::Event* event = &dummy_event;
    grab_filter.accept(event);
}
