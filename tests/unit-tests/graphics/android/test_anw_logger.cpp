/*
 * Copyright © 2014 Canonical Ltd.
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

#include "native_window_report.h"
#include "mir/logging/logger.h"
#include <system/window.h>
#include <iostream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;
namespace mga = mir::graphics::android;
namespace ml = mir::logging;

namespace
{

struct MockLogger : ml::Logger
{
    MOCK_METHOD3(log, void(ml::Severity, std::string const&, std::string const&));
};

struct ANWLogger : Test
{
    std::shared_ptr<MockLogger> mock_logger = std::make_shared<MockLogger>();
    mga::ConsoleNativeWindowReport report{mock_logger};
    ANativeWindow anw;
    ANativeWindowBuffer anwb;
    int dummy_fd = 421;
    int dummy_cleared_fd = -421;

};
}

TEST_F(ANWLogger, logs_buffer_events)
{
    std::stringstream expected1;
    std::stringstream expected2;
    std::stringstream expected3;
    std::stringstream expected4;
    std::stringstream expected5;
    expected1 << "addr (" << &anw << "): queueBuffer: " << &anwb << ", fence: " << dummy_fd;
    expected2 << "addr (" << &anw << "): dequeueBuffer: " << &anwb << ", fence: none";
    expected3 << "addr (" << &anw << "): cancelBuffer: " << &anwb << ", fence: none";
    expected4 << "addr (" << &anw << "): dequeueBuffer_deprecated: " << &anwb;
    expected5 << "addr (" << &anw << "): cancelBuffer_deprecated: " << &anwb;

    InSequence seq;
    EXPECT_CALL(*mock_logger, log(_,StrEq(expected1.str()),_));
    EXPECT_CALL(*mock_logger, log(_,StrEq(expected2.str()),_));
    EXPECT_CALL(*mock_logger, log(_,StrEq(expected3.str()),_));
    EXPECT_CALL(*mock_logger, log(_,StrEq(expected4.str()),_));
    EXPECT_CALL(*mock_logger, log(_,StrEq(expected5.str()),_));

    report.buffer_event(mga::BufferEvent::Queue, &anw, &anwb, dummy_fd);
    report.buffer_event(mga::BufferEvent::Dequeue, &anw, &anwb, dummy_cleared_fd);
    report.buffer_event(mga::BufferEvent::Cancel, &anw, &anwb, dummy_cleared_fd);
    report.buffer_event(mga::BufferEvent::Dequeue, &anw, &anwb);
    report.buffer_event(mga::BufferEvent::Cancel, &anw, &anwb);
}

TEST_F(ANWLogger, logs_perform_events)
{
    std::stringstream expected1;
    std::stringstream expected2;
    expected1 << "addr (" << &anw << "): perform: NATIVE_WINDOW_SET_USAGE: 0x734, 0x858";
    expected2 << "addr (" << &anw << "): perform: NATIVE_WINDOW_SET_BUFFERS_TIMESTAMP: 0x313";

    InSequence seq;
    EXPECT_CALL(*mock_logger, log(_,StrEq(expected1.str()),_));
    EXPECT_CALL(*mock_logger, log(_,StrEq(expected2.str()),_));

    report.perform_event(&anw, NATIVE_WINDOW_SET_USAGE, {0x734, 0x858} );
    report.perform_event(&anw, NATIVE_WINDOW_SET_BUFFERS_TIMESTAMP, {0x313} );
}

TEST_F(ANWLogger, logs_query_events)
{
    unsigned int width = 0x380;
    unsigned int type = 0x2; 
    std::stringstream expected1;
    std::stringstream expected2;
    expected1 << "addr (" << &anw << "): query: NATIVE_WINDOW_WIDTH: result: 0x380";
    expected2 << "addr (" << &anw << "): query: NATIVE_WINDOW_CONCRETE_TYPE: result: 0x2";

    InSequence seq;
    EXPECT_CALL(*mock_logger, log(_,StrEq(expected1.str()),_));
    EXPECT_CALL(*mock_logger, log(_,StrEq(expected2.str()),_));

    report.query_event(&anw, NATIVE_WINDOW_WIDTH, width);
    report.query_event(&anw, NATIVE_WINDOW_CONCRETE_TYPE, type);
}
