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

#include "native_window_logger.h"
#include <system/window.h>
#include <iostream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;
namespace mga = mir::graphics::android;

struct ANWLogger : Test
{
    ANWLogger()
    {
        default_cout_buffer = std::cout.rdbuf();
        std::cout.rdbuf(test_stream.rdbuf());
    }
    virtual ~ANWLogger()
    {
        std::cout.rdbuf(default_cout_buffer);
    }
    decltype(std::cout.rdbuf()) default_cout_buffer;
    std::ostringstream test_stream;
    mga::ConsoleNativeWindowLogger logger;
    ANativeWindow anw;
    ANativeWindowBuffer anwb;
    int dummy_fd = 421;
    int dummy_cleared_fd = -421;

};

TEST_F(ANWLogger, logs_buffer_events)
{
    std::stringstream expected;
    expected << 
        "window (" << &anw << "): queueBuffer: " << &anwb << ", fence: " << dummy_fd << "\n"
        "window (" << &anw << "): dequeueBuffer: " << &anwb << ", fence: none\n"
        "window (" << &anw << "): cancelBuffer: " << &anwb << ", fence: none\n"
        "window (" << &anw << "): dequeueBuffer_deprecated: " << &anwb << "\n"
        "window (" << &anw << "): cancelBuffer_deprecated: " << &anwb << "\n";

    logger.buffer_event(mga::BufferEvent::Queue, &anw, &anwb, dummy_fd);
    logger.buffer_event(mga::BufferEvent::Dequeue, &anw, &anwb, dummy_cleared_fd);
    logger.buffer_event(mga::BufferEvent::Cancel, &anw, &anwb, dummy_cleared_fd);
    logger.buffer_event(mga::BufferEvent::Dequeue, &anw, &anwb);
    logger.buffer_event(mga::BufferEvent::Cancel, &anw, &anwb);
    EXPECT_THAT(expected.str(), StrEq(test_stream.str())); 
}

TEST_F(ANWLogger, logs_perform_events)
{
    std::stringstream expected;
    expected << 
        "window (" << &anw << "): perform: NATIVE_WINDOW_SET_USAGE: 0x734, 0x858\n"
        "window (" << &anw << "): perform: NATIVE_WINDOW_SET_BUFFERS_TIMESTAMP: 0x313\n";

    logger.perform_event(&anw, NATIVE_WINDOW_SET_USAGE, {0x734, 0x858} );
    logger.perform_event(&anw, NATIVE_WINDOW_SET_BUFFERS_TIMESTAMP, {0x313} );
    EXPECT_THAT(test_stream.str(), StrEq(expected.str()));
}

TEST_F(ANWLogger, logs_query_events)
{
    int width = 380;
    unsigned int type = 0x2; 
    std::stringstream expected;
    expected << 
        "window (" << &anw << "): query NATIVE_WINDOW_WIDTH: result: 380\n"
        "window (" << &anw << "): query NATIVE_WINDOW_CONCRETE_TYPE: result: 0x2\n";
    logger.query_event(&anw, NATIVE_WINDOW_WIDTH, width);
    logger.query_event(&anw, NATIVE_WINDOW_CONCRETE_TYPE, type);
    EXPECT_THAT(expected.str(), StrEq(test_stream.str())); 
}
