#if 0
/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "src/platforms/evdev/libinput_device.h"

#include "mir/input/input_sink.h"
#include "mir_test_doubles/mock_libinput.h"
#include "mir_test/fake_shared.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mie = mir::input::evdev;
namespace mtd = mir::test::doubles;
namespace mt = mir::test;

namespace
{
struct StubInputSink : public mir::input::InputSink
{
    void handle_input(MirEvent&) override {}
    void confine_pointer(mir::geometry::Point&) override {}
    mir::geometry::Rectangle bounding_rectangle() const override {return {};}
};

struct LibInputWrapper : public ::testing::Test
{
    ::testing::NiceMock<mtd::MockLibInput> mock_libinput;
    StubInputSink stub_sink;
    libinput* fake_address = reinterpret_cast<libinput*>(0xF4C3);
    libinput_device* fake_device = reinterpret_cast<libinput_device*>(0xF4C4);
    libinput_event* fake_event = reinterpret_cast<libinput_event*>(0xF4C6);
    const int libinput_fd = 23;

    void ensure_init_works()
    {
        using namespace ::testing;
        ON_CALL(mock_libinput, libinput_path_create_context(_,_))
            .WillByDefault(Return(fake_address));
        ON_CALL(mock_libinput, libinput_get_fd(fake_address))
            .WillByDefault(Return(libinput_fd));
    }

    void create_device_on_path_request()
    {
        using namespace ::testing;
        ON_CALL(mock_libinput, libinput_path_add_device(fake_address,_))
            .WillByDefault(Return(fake_device));
    }

    LibInputWrapper()
    {
        using namespace ::testing;
        //ON_CALL(mock_register, register_fd_handler_(_,_,_))
         //   .WillByDefault(SaveArg<2>(&event_callback));
    }

    std::function<void(int)> event_callback;
};

}

TEST_F(LibInputWrapper, tracks_a_libinput_path_context)
{
    using namespace ::testing;
    EXPECT_CALL(mock_libinput, libinput_path_create_context(_,_))
        .WillRepeatedly(Return(fake_address));
    EXPECT_CALL(mock_libinput, libinput_unref(fake_address));

    mie::LibInputWrapper wrapper;
}

TEST_F(LibInputWrapper, registers_libinput_fd_on_first_started_device)
{
    using namespace ::testing;
    ensure_init_works();
    mie::LibInputWrapper wrapper;
    EXPECT_CALL(mock_libinput, libinput_get_fd(fake_address))
        .Times(1);
    //EXPECT_CALL(mock_register, register_fd_handler_(_, &wrapper,_)).Times(1);

    mie::LibInputDevice dev1(mt::fake_shared(wrapper), "dev1");
    mie::LibInputDevice dev2(mt::fake_shared(wrapper), "dev2");;
    dev1.start(&stub_sink);
    dev2.start(&stub_sink);
    dev1.start(&stub_sink);
}

TEST_F(LibInputWrapper, unrregisters_libinput_fd_on_last_active_device)
{
    using namespace ::testing;
    ensure_init_works();
    mie::LibInputWrapper wrapper;
    EXPECT_CALL(mock_libinput, libinput_get_fd(fake_address))
        .Times(1);
    //EXPECT_CALL(mock_register, register_fd_handler_(_, &wrapper,_)).Times(1);
    //EXPECT_CALL(mock_register, unregister_fd_handler(&wrapper)).Times(1);

    mie::LibInputDevice dev1(mt::fake_shared(wrapper), "dev1");
    mie::LibInputDevice dev2(mt::fake_shared(wrapper), "dev2");;
    dev1.start(&stub_sink);
    dev2.start(&stub_sink);

    dev1.stop();
    dev2.stop();
    dev1.stop();
}

TEST_F(LibInputWrapper, receives_and_destroys_events)
{
    using namespace ::testing;
    ensure_init_works();
    create_device_on_path_request();
    mie::LibInputWrapper wrapper;
    mie::LibInputDevice dev(mt::fake_shared(wrapper), "dev1");
    dev.start(&stub_sink);

    InSequence seq;
    EXPECT_CALL(mock_libinput, libinput_dispatch(fake_address));
    EXPECT_CALL(mock_libinput, libinput_get_event(fake_address))
        .WillOnce(Return(fake_event));
    EXPECT_CALL(mock_libinput, libinput_event_get_device(fake_event))
        .WillOnce(Return(fake_device));
    EXPECT_CALL(mock_libinput, libinput_event_get_type(fake_event))
        .WillOnce(Return(LIBINPUT_EVENT_NONE));
    EXPECT_CALL(mock_libinput, libinput_get_event(fake_address))
        .WillOnce(Return(nullptr));
    EXPECT_CALL(mock_libinput, libinput_event_destroy(fake_event));

    event_callback(libinput_fd);
}
#endif
