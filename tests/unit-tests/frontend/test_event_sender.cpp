/*
 * Copyright © 2013 Canonical Ltd.
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

#include "mir_protobuf.pb.h"
#include "mir_protobuf_wire.pb.h"

#include "src/server/frontend/message_sender.h"
#include "src/server/frontend/event_sender.h"

#include "mir/events/event_builders.h"

#include "mir/test/display_config_matchers.h"
#include "mir/test/input_devices_matcher.h"
#include "mir/test/fake_shared.h"
#include "mir/test/doubles/stub_display_configuration.h"
#include "mir/test/doubles/stub_buffer.h"
#include "mir/test/doubles/stub_input_device.h"
#include "mir/test/doubles/mock_platform_ipc_operations.h"
#include "mir/input/device.h"
#include "mir/input/device_capability.h"
#include "mir/input/pointer_configuration.h"
#include "mir/input/touchpad_configuration.h"
#include "mir/variable_length_array.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mt = mir::test;
namespace mi = mir::input;
namespace mtd = mir::test::doubles;
namespace mf = mir::frontend;
namespace mfd = mf::detail;
namespace mev = mir::events;
namespace geom = mir::geometry;

namespace
{
struct StubDevice : mi::Device
{
    StubDevice(MirInputDeviceId id, mi::DeviceCapabilities caps, std::string const& name, std::string const& unique_id)
        : device_id(id), device_capabilities(caps), device_name(name), device_unique_id(unique_id) {}

    MirInputDeviceId id() const override
    {
        return device_id;
    }
    mi::DeviceCapabilities capabilities() const override
    {
        return device_capabilities;
    }
    std::string name() const override
    {
        return device_name;
    }
    std::string unique_id() const override
    {
        return device_unique_id;
    }
    mir::optional_value<mi::PointerConfiguration> pointer_configuration() const override
    {
        return {};
    }
    void apply_pointer_configuration(mi::PointerConfiguration const&) override
    {
    }

    mir::optional_value<mi::TouchpadConfiguration> touchpad_configuration() const override
    {
        return {};
    }
    void apply_touchpad_configuration(mi::TouchpadConfiguration const&) override
    {
    }

    MirInputDeviceId device_id;
    mi::DeviceCapabilities device_capabilities;
    std::string device_name;
    std::string device_unique_id;
};

struct MockMsgSender : public mf::MessageSender
{
    MOCK_METHOD3(send, void(char const*, size_t, mf::FdSets const&));
};
struct EventSender : public testing::Test
{
    EventSender()
        : event_sender(mt::fake_shared(mock_msg_sender), mt::fake_shared(mock_buffer_packer))
    {
    }
    MockMsgSender mock_msg_sender;
    mtd::MockPlatformIpcOperations mock_buffer_packer;
    mfd::EventSender event_sender;
};
}

TEST_F(EventSender, display_send)
{
    using namespace testing;

    mtd::StubDisplayConfig config;

    auto msg_validator = [&config](char const* data, size_t len){
        mir::protobuf::wire::Result wire;
        wire.ParseFromArray(data, len);
        std::string str = wire.events(0);
        mir::protobuf::EventSequence seq;
        seq.ParseFromString(str);
        EXPECT_THAT(seq.display_configuration(), mt::DisplayConfigMatches(std::cref(config)));
    };

    EXPECT_CALL(mock_msg_sender, send(_, _, _))
        .Times(1)
        .WillOnce(WithArgs<0,1>(Invoke(msg_validator)));

    event_sender.handle_display_config_change(config);
}

TEST_F(EventSender, sends_noninput_events)
{
    using namespace testing;

    auto surface_ev = mev::make_event(mf::SurfaceId{1}, mir_surface_attrib_focus, mir_surface_focused);
    auto resize_ev = mev::make_event(mf::SurfaceId{1}, {10, 10});

    EXPECT_CALL(mock_msg_sender, send(_, _, _))
        .Times(2);
    event_sender.handle_event(*surface_ev);
    event_sender.handle_event(*resize_ev);
}

TEST_F(EventSender, never_sends_input_events)
{
    using namespace testing;

    auto ev = mev::make_event(MirInputDeviceId(), std::chrono::nanoseconds(0), std::vector<uint8_t>{}, MirKeyboardAction(),
                              0, 0, MirInputEventModifiers());

    EXPECT_CALL(mock_msg_sender, send(_, _, _))
        .Times(0);
    event_sender.handle_event(*ev);
}

TEST_F(EventSender, packs_buffer_with_platform_packer)
{
    using namespace testing;
    mf::BufferStreamId id{8};
    auto msg_type = mir::graphics::BufferIpcMsgType::update_msg;
    mtd::StubBuffer buffer;

    InSequence seq;
    EXPECT_CALL(mock_buffer_packer, pack_buffer(_, Ref(buffer), msg_type));
    EXPECT_CALL(mock_msg_sender, send(_,_,_));
    event_sender.send_buffer(mf::BufferStreamId{}, buffer, msg_type);
}

TEST_F(EventSender, sends_input_devices)
{
    using namespace testing;

    std::vector<std::shared_ptr<mi::Device>> devices{
        std::make_shared<StubDevice>(3, mi::DeviceCapability::pointer | mi::DeviceCapability::touchpad, "touchpad",
                                     "5352"),
        std::make_shared<StubDevice>(23, mi::DeviceCapability::keyboard | mi::DeviceCapability::alpha_numeric,
                                     "keybaord", "7853")};

    auto msg_validator = [&devices](char const* data, size_t len){
        mir::protobuf::wire::Result wire;
        wire.ParseFromArray(data, len);
        std::string str = wire.events(0);
        mir::protobuf::EventSequence seq;
        seq.ParseFromString(str);
        EXPECT_THAT(seq.input_devices(), mt::InputDevicesMatch(devices));
    };

    EXPECT_CALL(mock_msg_sender, send(_, _, _))
        .Times(1)
        .WillOnce(WithArgs<0,1>(Invoke(msg_validator)));

    event_sender.handle_input_device_change(devices);
}

TEST_F(EventSender, sends_empty_sequence_of_devices)
{
    using namespace testing;

    std::vector<std::shared_ptr<mi::Device>> devices;

    auto msg_validator = [&devices](char const* data, size_t len){
        mir::protobuf::wire::Result wire;
        wire.ParseFromArray(data, len);
        std::string str = wire.events(0);
        mir::protobuf::EventSequence seq;
        seq.ParseFromString(str);
        EXPECT_THAT(seq.input_devices(), mt::InputDevicesMatch(devices));
    };

    EXPECT_CALL(mock_msg_sender, send(_, _, _))
        .Times(1)
        .WillOnce(WithArgs<0,1>(Invoke(msg_validator)));

    event_sender.handle_input_device_change(devices);
}

namespace
{
MATCHER_P2(MessageMatches, data, size, "")
{
    ASSERT_THAT(arg.size(), testing::Eq(size));
    (void)data;
    return true;
}
}

TEST_F(EventSender, can_send_error_buffer)
{
    using namespace testing;
    std::string error_msg = "error";
    mir::graphics::BufferProperties properties{
        { 10, 12 }, mir_pixel_format_abgr_8888, mir::graphics::BufferUsage::hardware};
    mir::protobuf::EventSequence expected_sequence;
    auto buffer = expected_sequence.mutable_buffer_request()->mutable_buffer();
    buffer->set_width(properties.size.width.as_int());
    buffer->set_height(properties.size.height.as_int());
    buffer->set_error(error_msg.c_str());
    mir::VariableLengthArray<1024>
        send_buffer{static_cast<size_t>(expected_sequence.ByteSize())};
    expected_sequence.SerializeWithCachedSizesToArray(send_buffer.data());
    mir::protobuf::wire::Result result;
    result.add_events(send_buffer.data(), send_buffer.size());
    send_buffer.resize(result.ByteSize());
    result.SerializeWithCachedSizesToArray(send_buffer.data());

    char const* data = nullptr;
    size_t length = 0;
    EXPECT_CALL(mock_msg_sender, send(_,_,_))
          .WillOnce(DoAll(SaveArg<0>(&data), SaveArg<1>(&length)));
    event_sender.error_buffer(properties, error_msg);

    ASSERT_THAT(length, Eq(send_buffer.size()));
}
