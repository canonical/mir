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

#ifndef MIR_INPUT_DISPATCH_FIXTURE_H
#define MIR_INPUT_DISPATCH_FIXTURE_H

#include "mir/input/dispatcher.h"
#include "mir/input/grab_filter.h"
#include "mir/input/shell_filter.h"
#include "mir/input/application_filter.h"
#include "mir/input/filter.h"
#include "mir/time_source.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>

namespace mir
{

class MockTimeSource : public mir::TimeSource
{
 public:
    MockTimeSource()
    {
        using namespace ::testing;

        ON_CALL(*this, sample()).WillByDefault(Invoke(this, &MockTimeSource::sample_hrc));
        EXPECT_CALL(*this, sample()).Times(AtLeast(0));
    }
    MOCK_CONST_METHOD0(sample, mir::Timestamp());

    mir::Timestamp sample_hrc() const
    {
        return boost::chrono::high_resolution_clock::now();
    }
};

namespace input
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

    MOCK_CONST_METHOD1(accept, void(Event*));

    void forward_accept(Event* event) { T::accept(event); }
};

class InputDispatchFixture : public ::testing::Test
{
 public:
    InputDispatchFixture()
		: mock_null_filter(new MockFilter<NullFilter>()),
		  mock_app_filter(new MockFilter<ApplicationFilter>(mock_null_filter)),
		  mock_shell_filter(new MockFilter<ShellFilter>(mock_app_filter)),
		  mock_grab_filter(new MockFilter<GrabFilter>(mock_shell_filter)),
		  dispatcher(&time_source, mock_grab_filter)
    {
        mir::Timestamp ts;
        ::testing::DefaultValue<mir::Timestamp>::Set(ts);
    }
    
    ~InputDispatchFixture()
    {
        ::testing::DefaultValue<mir::Timestamp>::Clear();
    }
 protected:
    MockTimeSource time_source;

    std::shared_ptr<MockFilter<NullFilter>> mock_null_filter;
    std::shared_ptr<MockFilter<ApplicationFilter>> mock_app_filter;
    std::shared_ptr<MockFilter<ShellFilter>> mock_shell_filter;
    std::shared_ptr<MockFilter<GrabFilter>> mock_grab_filter;
    Dispatcher dispatcher;
};

}
}

#endif // MIR_INPUT_DISPATCH_FIXTURE_H
