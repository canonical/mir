/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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
#include "mir/client_visible_error.h"

#include "mir/test/display_config_matchers.h"
#include "mir/test/input_devices_matcher.h"
#include "mir/test/fake_shared.h"
#include "mir/test/doubles/stub_display_configuration.h"
#include "mir/test/doubles/stub_buffer.h"
#include "mir/test/doubles/stub_input_device.h"
#include "mir/test/doubles/mock_platform_ipc_operations.h"
#include "mir/input/device.h"
#include "mir/input/device_capability.h"
#include "mir/input/mir_input_config.h"
#include "mir/input/mir_input_config_serialization.h"
#include "mir/input/mir_pointer_config.h"
#include "mir/input/mir_touchpad_config.h"
#include "mir/variable_length_array.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <mir_protobuf.pb.h>

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
    mir::optional_value<MirPointerConfig> pointer_configuration() const override
    {
        return {};
    }
    void apply_pointer_configuration(MirPointerConfig const&) override
    {
    }

    mir::optional_value<MirTouchpadConfig> touchpad_configuration() const override
    {
        return {};
    }
    void apply_touchpad_configuration(MirTouchpadConfig const&) override
    {
    }

    mir::optional_value<MirKeyboardConfig> keyboard_configuration() const override
    {
        return {};
    }
    void apply_keyboard_configuration(MirKeyboardConfig const&) override
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

std::function<void(char const*, size_t, mir::frontend::FdSets)>
make_validator(std::function<void(mir::protobuf::EventSequence const&)> const& sequence_validator)
{
    return [sequence_validator](char const* data, size_t len, mir::frontend::FdSets)
        {
            mir::protobuf::wire::Result wire;
            wire.ParseFromArray(data, len);
            std::string str = wire.events(0);
            mir::protobuf::EventSequence seq;
            seq.ParseFromString(str);
            sequence_validator(seq);
        };
}
}

TEST_F(EventSender, display_send)
{
    using namespace testing;

    mtd::StubDisplayConfig config;

    auto msg_validator = make_validator(
        [&config](auto const& seq)
        {
            EXPECT_THAT(seq.display_configuration(), mt::DisplayConfigMatches(std::cref(config)));
        });

    EXPECT_CALL(mock_msg_sender, send(_, _, _))
        .Times(1)
        .WillOnce(Invoke(msg_validator));

    event_sender.handle_display_config_change(config);
}

TEST_F(EventSender, sends_noninput_events)
{
    using namespace testing;

    auto surface_ev = mev::make_event(mf::SurfaceId{1}, mir_window_attrib_focus, mir_window_focus_state_focused);
    auto resize_ev = mev::make_event(mf::SurfaceId{1}, {10, 10});

    EXPECT_CALL(mock_msg_sender, send(_, _, _))
        .Times(2);
    event_sender.handle_event(*surface_ev);
    event_sender.handle_event(*resize_ev);
}

TEST_F(EventSender, sends_input_events)
{
    using namespace testing;

    auto ev = mev::make_event(MirInputDeviceId(), std::chrono::nanoseconds(0), std::vector<uint8_t>{}, MirKeyboardAction(),
                              0, 0, MirInputEventModifiers());

    EXPECT_CALL(mock_msg_sender, send(_, _, _))
        .Times(1);
    event_sender.handle_event(*ev);
}

TEST_F(EventSender, packs_buffer_with_platform_packer)
{
    using namespace testing;
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

    MirInputDevice tpd(3, mi::DeviceCapability::pointer | mi::DeviceCapability::touchpad, "touchpad", "5352");
    MirInputDevice kbd(23, mi::DeviceCapability::keyboard | mi::DeviceCapability::alpha_numeric, "keyboard",
                                "5352");

    MirInputConfig devices;
    devices.add_device_config(tpd);
    devices.add_device_config(kbd);

    auto msg_validator = make_validator(
        [&devices](auto const& seq)
        {
            auto received_input_config = mi::deserialize_input_config(seq.input_configuration());
            EXPECT_THAT(received_input_config, Eq(devices));
        });

    EXPECT_CALL(mock_msg_sender, send(_, _, _))
        .Times(1)
        .WillOnce(Invoke(msg_validator));

    event_sender.handle_input_config_change(devices);
}

TEST_F(EventSender, sends_empty_sequence_of_devices)
{
    using namespace testing;

    MirInputConfig empty;

    auto msg_validator = make_validator(
        [&empty](auto const& seq)
        {
            auto received_input_config = mi::deserialize_input_config(seq.input_configuration());
            EXPECT_THAT(received_input_config, Eq(empty));
        });

    EXPECT_CALL(mock_msg_sender, send(_, _, _))
        .Times(1)
        .WillOnce(Invoke(msg_validator));

    event_sender.handle_input_config_change(empty);
}

TEST_F(EventSender, can_send_error_buffer)
{
    using namespace testing;
    std::string error_msg = "error";
    mir::graphics::BufferProperties properties{
        { 10, 12 }, mir_pixel_format_abgr_8888, mir::graphics::BufferUsage::hardware};
    mir::protobuf::EventSequence expected_sequence;
    expected_sequence.mutable_buffer_request()->set_operation(mir::protobuf::BufferOperation::add);
    auto buffer = expected_sequence.mutable_buffer_request()->mutable_buffer();
    buffer->set_width(properties.size.width.as_int());
    buffer->set_height(properties.size.height.as_int());
    buffer->set_error(error_msg.c_str());
    mir::VariableLengthArray<1024>
        expected_buffer{static_cast<size_t>(expected_sequence.ByteSize())};
    expected_sequence.SerializeWithCachedSizesToArray(expected_buffer.data());

    mir::protobuf::wire::Result result;
    result.add_events(expected_buffer.data(), expected_buffer.size());
    expected_buffer.resize(result.ByteSize());
    result.SerializeWithCachedSizesToArray(expected_buffer.data());

    std::vector<char> sent_buffer;
    EXPECT_CALL(mock_msg_sender, send(_,_,_))
          .WillOnce(Invoke([&](auto data, auto size, auto)
            {
                sent_buffer.resize(size);
                memcpy(sent_buffer.data(), data, size); 
            }));
    event_sender.error_buffer(properties.size, properties.format, error_msg);
    ASSERT_THAT(sent_buffer.size(), Eq(expected_buffer.size()));
    EXPECT_FALSE(memcmp(sent_buffer.data(), expected_buffer.data(), sent_buffer.size()));
}

TEST_F(EventSender, sends_errors)
{
    using namespace testing;

    class TestError : public mir::ClientVisibleError
    {
    public:
        TestError()
            : ClientVisibleError("An explosion of delight")
        {
        }

        MirErrorDomain domain() const noexcept override
        {
            return static_cast<MirErrorDomain>(32);
        }

        uint32_t code() const noexcept override
        {
            return 0xDEADBEEF;
        }
    } error;

    auto msg_validator = make_validator(
        [&error](auto const& seq)
        {
            EXPECT_TRUE(seq.has_structured_error());
            EXPECT_THAT(seq.structured_error().domain(), Eq(error.domain()));
            EXPECT_THAT(seq.structured_error().code(), Eq(error.code()));
        });

    EXPECT_CALL(mock_msg_sender, send(_, _, _))
        .Times(1)
        .WillOnce(Invoke(msg_validator));

    event_sender.handle_error(error);
}
