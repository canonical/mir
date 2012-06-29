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
 * Authored by: Chase Douglas <chase.douglas@canonical.com>
 */

#include "mir/input/dispatcher.h"
#include "mir/input/evemu/device.h"
#include "mir/time_source.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mi = mir::input;
namespace mie = mir::input::evemu;

namespace
{

struct MockEventHandler : public mi::EventHandler
{
    MOCK_METHOD1(on_event, void(mi::Event*));
};

}

TEST(Device, EvemuFile)
{
    using namespace testing;

    MockEventHandler event_handler;
    mie::EvemuDevice device(
        TEST_RECORDINGS_DIR "quanta_touchscreen/device.prop",
        &event_handler);
        
    EXPECT_TRUE(device.get_name().compare("QUANTA OpticalTouchScreen (Virtual Test Device)") == 0)
        << "Device name is: \"" << device.get_name() << "\"";
    EXPECT_EQ(10, device.get_simultaneous_instances());
    // EXPECT_EQ(0, device.get_buttons().size());

    const mi::PositionInfo& pi = device.get_position_info();
    EXPECT_EQ(mi::Mode::absolute, pi.mode);
    /* FIXME: Can't test absolute position ranges yet, need mapping to screen coords */

    ASSERT_THROW(
        device.get_axis_for_type(mi::AxisType::vertical_scroll),
        mi::NoAxisForTypeException);

    ASSERT_THROW(
        device.get_axis_for_type(mi::AxisType::horizontal_scroll),
        mi::NoAxisForTypeException);
}
