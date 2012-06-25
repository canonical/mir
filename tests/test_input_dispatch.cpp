/*
 * Copyright © 2012 Canonical Ltd.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Canonical Ltd. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Canonical Ltd. makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * CANONICAL, LTD. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL CANONICAL, LTD. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#include "mir/time_source.h"
#include "mir/input/device.h"
#include "mir/input/dispatcher.h"
#include "mir/input/event.h"
#include "mir/input/filter.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mi = mir::input;

namespace
{

class MockEvent : public mi::Event {
};

class MockFilter : public mi::Filter
{
 public:
    MOCK_METHOD1(Accept, bool(mi::Event*));
};

class MockInputDevice : public mi::Device
{
 public:
    MockInputDevice(mi::EventHandler* h) : mi::Device(h)
    {
    }

    void TriggerEvent()
    {
        handler->OnEvent(&event);
    }

    MockEvent event;
};

class MockTimeSource : public mir::TimeSource
{
 public:
    MOCK_CONST_METHOD0(Sample, mir::Timestamp());
};
    
}

TEST(input_dispatch, incoming_input_triggers_filter)
{
    using namespace testing;

    mir::Timestamp ts;
    DefaultValue<mir::Timestamp>::Set(ts);
    
    MockTimeSource time_source;
    MockFilter filter;
    mi::Dispatcher dispatcher(&time_source,
                              &filter,
                              &filter,
                              &filter);
    
    MockInputDevice device(&dispatcher);

    EXPECT_CALL(filter, Accept(_)).Times(AtLeast(3));

    device.TriggerEvent();
}

TEST(input_dispatch, incoming_input_is_timestamped)
{
    using namespace testing;

    mir::Timestamp ts;
    DefaultValue<mir::Timestamp>::Set(ts);
    
    MockTimeSource time_source;
    MockFilter filter;
    mi::Dispatcher dispatcher(&time_source,
                              &filter,
                              &filter,
                              &filter);
    MockInputDevice device(&dispatcher);

    EXPECT_CALL(time_source, Sample()).Times(AtLeast(1));
    device.TriggerEvent();

    EXPECT_EQ(device.event.SystemTimestamp(), ts);
}
