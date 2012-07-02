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

#ifndef MIR_MOCK_INPUT_DEVICE_H_
#define MIR_MOCK_INPUT_DEVICE_H_

#include "mock_input_event.h"

#include "mir/input/axis.h"
#include "mir/input/event_handler.h"
#include "mir/input/logical_device.h"
#include "mir/input/position_info.h"

#include <gmock/gmock.h>

namespace mir
{
namespace input
{

class MockInputDevice : public LogicalDevice
{
 public:
    MockInputDevice(EventHandler* h) : LogicalDevice(h),
            state{EventProducer::State::stopped}
    {
        using namespace testing;
        using ::testing::_;
        using ::testing::Return;
        using ::testing::Throw;

        static std::string name("MockDevice");
        static PositionInfo pi;
        
        ON_CALL(*this, get_name()).WillByDefault(ReturnRef(name));
        ON_CALL(*this, get_simultaneous_instances()).WillByDefault(Return(0));
        // ON_CALL(*this, is_button_supported(_)).WillByDefault(Return(false));
        ON_CALL(*this, get_position_info()).WillByDefault(ReturnRef(pi));
        ON_CALL(*this, get_axes()).WillByDefault(ReturnRef(axes));

        ON_CALL(*this, current_state()).WillByDefault(Return(state));
        ON_CALL(*this, start()).WillByDefault(Assign(&state,EventProducer::State::running));
        ON_CALL(*this, stop()).WillByDefault(Assign(&state,EventProducer::State::stopped));
    }

    // From EventProducer
    MOCK_CONST_METHOD0(current_state, EventProducer::State());
    MOCK_METHOD0(start, void());
    MOCK_METHOD0(stop, void());

    // From LogicalDevice
    typedef std::map<AxisType,Axis> AxisMap;
    MOCK_CONST_METHOD0(get_name, const std::string&());
    MOCK_CONST_METHOD0(get_simultaneous_instances, int());
    // MOCK_CONST_METHOD1(is_button_supported, bool(const Button&));
    MOCK_CONST_METHOD0(get_position_info, const PositionInfo&());
    MOCK_CONST_METHOD0(get_axes, const AxisMap&());
    
    void trigger_event()
    {
        MockInputEvent ev;
        announce_event(&ev);
    }

    MockInputEvent event;
    EventProducer::State state;
    std::map<AxisType, Axis> axes;

};

}
}

#endif // MIR_MOCK_INPUT_DEVICE_H_
