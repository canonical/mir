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
 * Authored by: Andreas Pokorny <andreas.pokorny@gmail.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_INPUT_READER_H_
#define MIR_TEST_DOUBLES_MOCK_INPUT_READER_H_

#include <InputReader.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{
struct MockInputReader : public droidinput::InputReaderInterface
{
public:
    MOCK_METHOD1(dump, void(droidinput::String8& dump));
    MOCK_METHOD0(monitor, void());
    MOCK_METHOD0(loopOnce, void());
    MOCK_METHOD1(getInputDevices, void(droidinput::Vector<droidinput::InputDeviceInfo>& outInputDevices));
    MOCK_METHOD3(getScanCodeState, int32_t(int32_t deviceId, uint32_t sourceMask,
            int32_t scanCode));
    MOCK_METHOD3(getKeyCodeState, int32_t(int32_t deviceId, uint32_t sourceMask,
            int32_t keyCode));
    MOCK_METHOD3(getSwitchState, int32_t(int32_t deviceId, uint32_t sourceMask,
            int32_t sw));
    MOCK_METHOD5(hasKeys, bool(int32_t deviceId, uint32_t sourceMask,
            size_t numCodes, const int32_t* keyCodes, uint8_t* outFlags));
    MOCK_METHOD1(requestRefreshConfiguration,void (uint32_t changes));

    MOCK_METHOD5(vibrate, void(int32_t deviceId, const std::chrono::nanoseconds* pattern, size_t patternSize,
            ssize_t repeat, int32_t token));
    MOCK_METHOD2(cancelVibrate, void(int32_t deviceId, int32_t token));
};

}
}
}

#endif
