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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "src/platforms/mesa/server/kms/linux_virtual_terminal.h"
#include "src/server/report/null_report_factory.h"
#include "mir/graphics/event_handler_register.h"
#include "mir/anonymous_shm_file.h"

#include "mir/test/fake_shared.h"
#include "mir/test/doubles/mock_display_report.h"
#include "mir/test/doubles/mock_event_handler_register.h"
#include "mir/test/gmock_fixes.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "mir/test/gmock_fixes.h"

#include <stdexcept>

#include <linux/vt.h>
#include <linux/kd.h>
#include <fcntl.h>

namespace mg = mir::graphics;
namespace mgm = mir::graphics::mesa;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mr = mir::report;

#include <boost/exception/diagnostic_information.hpp>

namespace
{

class MockVTFileOperations : public mgm::VTFileOperations
{
public:
    ~MockVTFileOperations() noexcept {}
    MOCK_METHOD2(open, int(char const*, int));
    MOCK_METHOD1(close, int(int));
    MOCK_METHOD3(ioctl, int(int, int, int));
    MOCK_METHOD3(ioctl, int(int, int, void*));
    MOCK_METHOD3(tcsetattr, int(int, int, const struct termios*));
    MOCK_METHOD2(tcgetattr, int(int, struct termios*));
};

class MockPosixProcessOperations : public mgm::PosixProcessOperations
{
public:
    ~MockPosixProcessOperations() = default;
    MOCK_CONST_METHOD0(getpid, pid_t());
    MOCK_CONST_METHOD0(getppid, pid_t());
    MOCK_CONST_METHOD1(getpgid, pid_t(pid_t));
    MOCK_CONST_METHOD1(getsid, pid_t(pid_t));
    MOCK_METHOD2(setpgid, int(pid_t, pid_t));
    MOCK_METHOD0(setsid, pid_t());
};

// The default return values are appropriate, so
// Add a typedef to aid clarity.
typedef testing::NiceMock<MockPosixProcessOperations> StubPosixProcessOperations;

ACTION_TEMPLATE(SetIoctlPointee,
                HAS_1_TEMPLATE_PARAMS(typename, T),
                AND_1_VALUE_PARAMS(param))
{
    *static_cast<T*>(arg2) = param;
}

ACTION_TEMPLATE(SetTcAttrPointee,
                HAS_1_TEMPLATE_PARAMS(typename, T),
                AND_1_VALUE_PARAMS(param))
{
    *static_cast<T*>(arg1) = param;
}

MATCHER_P(ModeUsesSignal, sig, "")
{
    auto vtm = static_cast<vt_mode*>(arg);

    return vtm->mode == VT_PROCESS &&
           vtm->relsig == sig &&
           vtm->acqsig == sig;
}

MATCHER_P(VTModeMatches, mode, "")
{
    auto vtm = static_cast<vt_mode*>(arg);

    return vtm->mode == mode.mode &&
           vtm->waitv == mode.waitv &&
           vtm->relsig == mode.relsig &&
           vtm->acqsig == mode.acqsig &&
           vtm->frsig == mode.frsig;
}

}

class LinuxVirtualTerminalTest : public ::testing::Test
{
public:
    LinuxVirtualTerminalTest()
        : fake_vt_fd{5},
          fake_kd_mode{KD_TEXT},
          fake_vt_mode_auto{VT_AUTO, 0, 0, 0, 0},
          fake_vt_mode_process{VT_PROCESS, 0, SIGUSR1, SIGUSR2, 0},
          fake_kb_mode{K_RAW},
          fake_tc_attr()
    {
    }

    void set_up_expectations_for_current_vt_search(int vt_num)
    {
        using namespace testing;

        int const tmp_fd1{3};
        int const tmp_fd2{4};

        vt_stat vts = vt_stat();
        vts.v_active = vt_num;

        EXPECT_CALL(mock_fops, open(StrEq("/dev/tty"), _))
            .WillOnce(Return(tmp_fd1));
        EXPECT_CALL(mock_fops, ioctl(tmp_fd1, VT_GETSTATE, An<void*>()))
            .WillOnce(Return(-1));
        EXPECT_CALL(mock_fops, close(tmp_fd1))
            .WillOnce(Return(0));

        EXPECT_CALL(mock_fops, open(StrEq("/dev/tty0"), _))
            .WillOnce(Return(tmp_fd2));
        EXPECT_CALL(mock_fops, ioctl(tmp_fd2, VT_GETSTATE, An<void*>()))
            .WillOnce(DoAll(SetIoctlPointee<vt_stat>(vts), Return(0)));
        EXPECT_CALL(mock_fops, close(tmp_fd2))
            .WillOnce(Return(0));
    }

    void set_up_expectations_for_vt_setup(int vt_num, bool activate)
    {
        set_up_expectations_for_vt_setup(vt_num, activate, fake_vt_mode_auto);
    }

    void set_up_expectations_for_vt_setup(
        int vt_num,
        bool activate,
        vt_mode const& vtm)
    {
        set_up_expectations_for_vt_setup(
            vt_num,
            activate,
            vtm,
            true);
    }

    void set_up_expectations_for_vt_setup(
        int vt_num,
        bool activate,
        vt_mode const& vtm,
        bool set_graphics_mode_succeeds)
    {
        using namespace testing;

        std::stringstream ss;
        ss << "/dev/tty" << vt_num;

        EXPECT_CALL(mock_fops, open(StrEq(ss.str()), _))
            .WillOnce(Return(fake_vt_fd));

        if (activate)
        {
            EXPECT_CALL(mock_fops, ioctl(fake_vt_fd, VT_ACTIVATE, vt_num))
                    .WillOnce(Return(0));

            EXPECT_CALL(mock_fops, ioctl(fake_vt_fd, VT_WAITACTIVE, vt_num))
                    .WillOnce(Return(0));
        }

        EXPECT_CALL(mock_fops, ioctl(fake_vt_fd, KDGETMODE, An<void*>()))
            .WillOnce(DoAll(SetIoctlPointee<int>(fake_kd_mode), Return(0)));
        EXPECT_CALL(mock_fops, ioctl(fake_vt_fd, VT_GETMODE, An<void*>()))
            .WillOnce(DoAll(SetIoctlPointee<vt_mode>(vtm), Return(0)));
        EXPECT_CALL(mock_fops, ioctl(fake_vt_fd, KDGKBMODE, An<void*>()))
            .WillOnce(DoAll(SetIoctlPointee<int>(fake_kb_mode), Return(0)));
        EXPECT_CALL(mock_fops, ioctl(fake_vt_fd, KDSKBMODE, K_OFF))
            .WillOnce(Return(0));
        EXPECT_CALL(mock_fops, tcgetattr(fake_vt_fd, An<struct termios *>()))
            .WillOnce(DoAll(SetTcAttrPointee<struct termios>(fake_tc_attr), Return(0)));
        EXPECT_CALL(mock_fops, tcsetattr(fake_vt_fd, TCSANOW, An<const struct termios *>()))
            .WillOnce(Return(0));
        EXPECT_CALL(mock_fops, ioctl(fake_vt_fd, KDSETMODE, KD_GRAPHICS))
            .WillOnce(Return(set_graphics_mode_succeeds ? 0 : -1));
    }

    void set_up_expectations_for_switch_handler(int sig)
    {
        using namespace testing;

        EXPECT_CALL(mock_event_handler_register,
                    register_signal_handler_module_ptr(ElementsAre(sig), _))
            .WillOnce(SaveArg<1>(&sig_handler));
        EXPECT_CALL(mock_fops, ioctl(fake_vt_fd, VT_SETMODE,
                                     MatcherCast<void*>(ModeUsesSignal(sig))));
    }

    void set_up_expectations_for_vt_teardown()
    {
        set_up_expectations_for_vt_teardown(fake_vt_mode_auto);
    }

    void set_up_expectations_for_vt_teardown(vt_mode const& vt_mode)
    {
        using namespace testing;

        set_up_expectations_for_vt_restore(vt_mode);

        EXPECT_CALL(mock_fops, close(fake_vt_fd))
            .WillOnce(Return(0));
    }

    void set_up_expectations_for_vt_restore(vt_mode const& vt_mode)
    {
        using namespace testing;

        EXPECT_CALL(mock_fops, tcsetattr(fake_vt_fd, TCSANOW, An<const struct termios *>()))
            .WillOnce(Return(0));
        EXPECT_CALL(mock_fops, ioctl(fake_vt_fd, KDSKBMODE, fake_kb_mode))
            .WillOnce(Return(0));
        EXPECT_CALL(mock_fops, ioctl(fake_vt_fd, KDSETMODE, fake_kd_mode))
            .WillOnce(Return(0));

        if (vt_mode.mode == VT_AUTO)
        {
            EXPECT_CALL(mock_fops, ioctl(fake_vt_fd, VT_SETMODE,
                                         Matcher<void*>(VTModeMatches(vt_mode))))
                .WillOnce(Return(0));
        }
        else
        {
            EXPECT_CALL(mock_fops, ioctl(fake_vt_fd, VT_SETMODE, An<void*>()))
                .Times(0);
        }
    }

    int const fake_vt_fd;
    int const fake_kd_mode;
    vt_mode const fake_vt_mode_auto;
    vt_mode const fake_vt_mode_process;
    int const fake_kb_mode;
    struct termios fake_tc_attr;
    std::function<void(int)> sig_handler;
    MockVTFileOperations mock_fops;
    mtd::MockEventHandlerRegister mock_event_handler_register;
};


TEST_F(LinuxVirtualTerminalTest, use_provided_vt)
{
    using namespace testing;

    int const vt_num{7};

    InSequence s;

    set_up_expectations_for_vt_setup(vt_num, true);
    set_up_expectations_for_vt_teardown();

    auto fops = mt::fake_shared<mgm::VTFileOperations>(mock_fops);
    auto pops = std::unique_ptr<mgm::PosixProcessOperations>(new StubPosixProcessOperations());
    auto null_report = mr::null_display_report();

    mgm::LinuxVirtualTerminal vt{fops, std::move(pops), vt_num, null_report};
}

TEST_F(LinuxVirtualTerminalTest, sets_up_current_vt)
{
    using namespace testing;

    int const vt_num{7};

    InSequence s;

    set_up_expectations_for_current_vt_search(vt_num);
    set_up_expectations_for_vt_setup(vt_num, false);
    set_up_expectations_for_vt_teardown();

    auto fops = mt::fake_shared<mgm::VTFileOperations>(mock_fops);
    auto pops = std::unique_ptr<mgm::PosixProcessOperations>(new StubPosixProcessOperations());
    auto null_report = mr::null_display_report();

    mgm::LinuxVirtualTerminal vt{fops, std::move(pops), 0, null_report};
}

TEST_F(LinuxVirtualTerminalTest, failure_to_find_current_vt_throws)
{
    using namespace testing;

    int const tmp_fd1{3};

    InSequence s;

    EXPECT_CALL(mock_fops, open(StrEq("/dev/tty"), _))
        .WillOnce(Return(tmp_fd1));
    EXPECT_CALL(mock_fops, ioctl(tmp_fd1, VT_GETSTATE, An<void*>()))
        .WillOnce(Return(-1));
    EXPECT_CALL(mock_fops, close(tmp_fd1))
        .WillOnce(Return(0));

    EXPECT_CALL(mock_fops, open(StrEq("/dev/tty0"), _))
        .WillOnce(Return(-1))
        .WillOnce(Return(-1));

    auto fops = mt::fake_shared<mgm::VTFileOperations>(mock_fops);
    auto pops = std::unique_ptr<mgm::PosixProcessOperations>(new StubPosixProcessOperations());
    auto null_report = mr::null_display_report();

    EXPECT_THROW({
        mgm::LinuxVirtualTerminal vt(fops, std::move(pops), 0, null_report);
    }, std::runtime_error);
}

TEST_F(LinuxVirtualTerminalTest, does_not_restore_vt_mode_if_vt_process)
{
    using namespace testing;

    int const vt_num{7};

    InSequence s;

    set_up_expectations_for_current_vt_search(vt_num);
    set_up_expectations_for_vt_setup(vt_num, false, fake_vt_mode_process);
    set_up_expectations_for_vt_teardown(fake_vt_mode_process);

    auto fops = mt::fake_shared<mgm::VTFileOperations>(mock_fops);
    auto pops = std::unique_ptr<mgm::PosixProcessOperations>(new StubPosixProcessOperations());
    auto null_report = mr::null_display_report();

    mgm::LinuxVirtualTerminal vt(fops, std::move(pops), 0, null_report);
}

TEST_F(LinuxVirtualTerminalTest, failure_to_set_graphics_mode_throws)
{
    using namespace testing;

    int const vt_num{7};

    InSequence s;

    set_up_expectations_for_current_vt_search(vt_num);
    set_up_expectations_for_vt_setup(vt_num, false, fake_vt_mode_auto, false);

    set_up_expectations_for_vt_teardown();

    auto fops = mt::fake_shared<mgm::VTFileOperations>(mock_fops);
    auto pops = std::unique_ptr<mgm::PosixProcessOperations>(new StubPosixProcessOperations());
    auto null_report = mr::null_display_report();

    EXPECT_THROW({
        mgm::LinuxVirtualTerminal vt(fops, std::move(pops), 0, null_report);
    }, std::runtime_error);
}

TEST_F(LinuxVirtualTerminalTest, uses_sigusr1_for_switch_handling)
{
    using namespace testing;

    int const vt_num{7};

    InSequence s;

    set_up_expectations_for_current_vt_search(vt_num);
    set_up_expectations_for_vt_setup(vt_num, false);
    set_up_expectations_for_switch_handler(SIGUSR1);
    set_up_expectations_for_vt_teardown();

    auto fops = mt::fake_shared<mgm::VTFileOperations>(mock_fops);
    auto pops = std::unique_ptr<mgm::PosixProcessOperations>(new StubPosixProcessOperations());
    auto null_report = mr::null_display_report();

    mgm::LinuxVirtualTerminal vt(fops, std::move(pops), 0, null_report);

    auto null_handler = [] { return true; };
    vt.register_switch_handlers(mock_event_handler_register, null_handler, null_handler);
}

TEST_F(LinuxVirtualTerminalTest, allows_vt_switch_on_switch_away_handler_success)
{
    using namespace testing;

    int const vt_num{7};
    int const allow_switch{1};

    InSequence s;

    set_up_expectations_for_current_vt_search(vt_num);
    set_up_expectations_for_vt_setup(vt_num, false);
    set_up_expectations_for_switch_handler(SIGUSR1);

    EXPECT_CALL(mock_fops, ioctl(fake_vt_fd, VT_RELDISP, allow_switch));

    set_up_expectations_for_vt_teardown();

    auto fops = mt::fake_shared<mgm::VTFileOperations>(mock_fops);
    auto pops = std::unique_ptr<mgm::PosixProcessOperations>(new StubPosixProcessOperations());
    auto null_report = mr::null_display_report();

    mgm::LinuxVirtualTerminal vt(fops, std::move(pops), 0, null_report);

    auto succeeding_handler = [] { return true; };
    vt.register_switch_handlers(mock_event_handler_register, succeeding_handler, succeeding_handler);

    /* Fake a VT switch away request */
    sig_handler(SIGUSR1);
}

TEST_F(LinuxVirtualTerminalTest, disallows_vt_switch_on_switch_away_handler_failure)
{
    using namespace testing;

    int const vt_num{7};
    int const disallow_switch{0};
    mtd::MockDisplayReport mock_report;

    InSequence s;

    set_up_expectations_for_current_vt_search(vt_num);
    set_up_expectations_for_vt_setup(vt_num, false);
    set_up_expectations_for_switch_handler(SIGUSR1);

    /* First switch away attempt */
    EXPECT_CALL(mock_report, report_vt_switch_away_failure());
    EXPECT_CALL(mock_fops, ioctl(fake_vt_fd, VT_RELDISP,
                                 MatcherCast<int>(disallow_switch)));

    /* Second switch away attempt */
    EXPECT_CALL(mock_report, report_vt_switch_away_failure());
    EXPECT_CALL(mock_fops, ioctl(fake_vt_fd, VT_RELDISP,
                                 MatcherCast<int>(disallow_switch)));

    set_up_expectations_for_vt_teardown();

    auto fops = mt::fake_shared<mgm::VTFileOperations>(mock_fops);
    auto pops = std::unique_ptr<mgm::PosixProcessOperations>(new StubPosixProcessOperations());

    mgm::LinuxVirtualTerminal vt(fops, std::move(pops), 0, mt::fake_shared(mock_report));

    auto failing_handler = [] { return false; };
    vt.register_switch_handlers(mock_event_handler_register, failing_handler, failing_handler);

    /* Fake a VT switch away request */
    sig_handler(SIGUSR1);
    /* Fake another VT switch away request */
    sig_handler(SIGUSR1);
}

TEST_F(LinuxVirtualTerminalTest, reports_failed_vt_switch_back_attempt)
{
    using namespace testing;

    int const vt_num{7};
    int const allow_switch{1};
    mtd::MockDisplayReport mock_report;

    InSequence s;

    set_up_expectations_for_current_vt_search(vt_num);
    set_up_expectations_for_vt_setup(vt_num, false);
    set_up_expectations_for_switch_handler(SIGUSR1);

    /* Switch away */
    EXPECT_CALL(mock_fops, ioctl(fake_vt_fd, VT_RELDISP, allow_switch));

    /* Switch back */
    EXPECT_CALL(mock_report, report_vt_switch_back_failure());
    EXPECT_CALL(mock_fops, ioctl(fake_vt_fd, VT_RELDISP, VT_ACKACQ));

    set_up_expectations_for_vt_teardown();

    auto fops = mt::fake_shared<mgm::VTFileOperations>(mock_fops);
    auto pops = std::unique_ptr<mgm::PosixProcessOperations>(new StubPosixProcessOperations());

    mgm::LinuxVirtualTerminal vt(fops, std::move(pops), 0, mt::fake_shared(mock_report));

    auto succeeding_handler = [] { return true; };
    auto failing_handler = [] { return false; };
    vt.register_switch_handlers(mock_event_handler_register, succeeding_handler, failing_handler);

    /* Fake a VT switch away request */
    sig_handler(SIGUSR1);
    /* Fake a VT switch back request */
    sig_handler(SIGUSR1);
}

TEST_F(LinuxVirtualTerminalTest, does_not_try_to_reaquire_session_leader)
{
    using namespace testing;

    int const vt_num{7};

    InSequence s;

    auto fops = mt::fake_shared<mgm::VTFileOperations>(mock_fops);
    auto pops = std::unique_ptr<NiceMock<MockPosixProcessOperations>>(new NiceMock<MockPosixProcessOperations>());
    auto null_report = mr::null_display_report();

    pid_t const mockpid{1234};

    ON_CALL(*pops, getpid()).WillByDefault(Return(mockpid));
    ON_CALL(*pops, getsid(Eq(0))).WillByDefault(Return(mockpid));
    ON_CALL(*pops, getsid(Eq(mockpid))).WillByDefault(Return(mockpid));

    EXPECT_CALL(*pops, setpgid(_,_)).Times(0);
    EXPECT_CALL(*pops, setsid()).Times(0);

    set_up_expectations_for_vt_setup(vt_num, true);
    set_up_expectations_for_vt_teardown();

    mgm::LinuxVirtualTerminal vt{fops, std::move(pops), vt_num, null_report};
}

TEST_F(LinuxVirtualTerminalTest, relinquishes_group_leader_before_claiming_session_leader)
{
    using namespace testing;

    int const vt_num{7};

    InSequence s;

    auto fops = mt::fake_shared<mgm::VTFileOperations>(mock_fops);
    auto pops = std::unique_ptr<NiceMock<MockPosixProcessOperations>>(new NiceMock<MockPosixProcessOperations>());
    auto null_report = mr::null_display_report();

    pid_t const mockpid{1234};
    pid_t const mock_parent_pid{4567};

    ON_CALL(*pops, getpid()).WillByDefault(Return(mockpid));

    ON_CALL(*pops, getpgid(Eq(0))).WillByDefault(Return(mockpid));
    ON_CALL(*pops, getpgid(Eq(mockpid))).WillByDefault(Return(mockpid));
    ON_CALL(*pops, getpgid(Eq(mock_parent_pid))).WillByDefault(Return(mock_parent_pid));

    ON_CALL(*pops, getppid()).WillByDefault(Return(mock_parent_pid));

    ON_CALL(*pops, getsid(Eq(0))).WillByDefault(Return(1));
    ON_CALL(*pops, getsid(Eq(mockpid))).WillByDefault(Return(1));

    EXPECT_CALL(*pops, setpgid(Eq(0), Eq(mock_parent_pid)))
        .Times(1)
        .WillOnce(Return(0));
    EXPECT_CALL(*pops, setsid())
        .Times(1)
        .WillOnce(Return(0));

    set_up_expectations_for_vt_setup(vt_num, true);
    set_up_expectations_for_vt_teardown();

    mgm::LinuxVirtualTerminal vt{fops, std::move(pops), vt_num, null_report};
}

TEST_F(LinuxVirtualTerminalTest, exception_if_setting_process_group_fails)
{
    using namespace testing;

    int const vt_num{7};

    InSequence s;

    auto fops = mt::fake_shared<mgm::VTFileOperations>(mock_fops);
    auto pops = std::unique_ptr<NiceMock<MockPosixProcessOperations>>(new NiceMock<MockPosixProcessOperations>());
    auto null_report = mr::null_display_report();

    pid_t const mockpid{1234};
    pid_t const mock_parent_pid{4567};

    ON_CALL(*pops, getpid()).WillByDefault(Return(mockpid));

    ON_CALL(*pops, getpgid(Eq(0))).WillByDefault(Return(mockpid));
    ON_CALL(*pops, getpgid(Eq(mockpid))).WillByDefault(Return(mockpid));
    ON_CALL(*pops, getpgid(Eq(mock_parent_pid))).WillByDefault(Return(mock_parent_pid));

    ON_CALL(*pops, getppid()).WillByDefault(Return(mock_parent_pid));

    ON_CALL(*pops, getsid(Eq(0))).WillByDefault(Return(1));
    ON_CALL(*pops, getsid(Eq(mockpid))).WillByDefault(Return(1));

    EXPECT_CALL(*pops, setpgid(Eq(0), Eq(mock_parent_pid)))
        .Times(1)
        .WillOnce(Return(-1));

    EXPECT_THROW({
        mgm::LinuxVirtualTerminal vt(fops, std::move(pops), vt_num, null_report);
    }, std::runtime_error);
}

TEST_F(LinuxVirtualTerminalTest, exception_if_becoming_session_leader_fails)
{
    using namespace testing;

    int const vt_num{7};

    InSequence s;

    auto fops = mt::fake_shared<mgm::VTFileOperations>(mock_fops);
    auto pops = std::unique_ptr<NiceMock<MockPosixProcessOperations>>(new NiceMock<MockPosixProcessOperations>());
    auto null_report = mr::null_display_report();

    pid_t const mockpid{1234};
    pid_t const mock_parent_pid{4567};

    ON_CALL(*pops, getpid()).WillByDefault(Return(mockpid));

    ON_CALL(*pops, getpgid(Eq(0))).WillByDefault(Return(mockpid));
    ON_CALL(*pops, getpgid(Eq(mockpid))).WillByDefault(Return(mockpid));
    ON_CALL(*pops, getpgid(Eq(mock_parent_pid))).WillByDefault(Return(mock_parent_pid));

    ON_CALL(*pops, getppid()).WillByDefault(Return(mock_parent_pid));

    ON_CALL(*pops, getsid(Eq(0))).WillByDefault(Return(1));
    ON_CALL(*pops, getsid(Eq(mockpid))).WillByDefault(Return(1));

    EXPECT_CALL(*pops, setpgid(Eq(0), Eq(mock_parent_pid)))
        .Times(1)
        .WillOnce(Return(0));
    EXPECT_CALL(*pops, setsid())
        .Times(1)
        .WillOnce(Return(-1));

    EXPECT_THROW({
        mgm::LinuxVirtualTerminal vt(fops, std::move(pops), vt_num, null_report);
    }, std::runtime_error);
}

TEST_F(LinuxVirtualTerminalTest, restores_keyboard_and_graphics)
{
    using namespace testing;

    int const vt_num{7};

    InSequence s;

    set_up_expectations_for_vt_setup(vt_num, true);

    set_up_expectations_for_vt_restore(fake_vt_mode_auto);

    auto fops = mt::fake_shared<mgm::VTFileOperations>(mock_fops);
    auto pops = std::unique_ptr<mgm::PosixProcessOperations>(new StubPosixProcessOperations());
    auto null_report = mr::null_display_report();

    mgm::LinuxVirtualTerminal vt(fops, std::move(pops), vt_num, null_report);

    vt.restore();

    Mock::VerifyAndClearExpectations(&mock_fops);

    set_up_expectations_for_vt_teardown();
}

TEST_F(LinuxVirtualTerminalTest, throws_expected_error_when_opening_file_fails)
{
    using namespace testing;

    auto fops = std::make_shared<NiceMock<MockVTFileOperations>>();
    auto pops = std::make_unique<StubPosixProcessOperations>();
    auto null_report = mr::null_display_report();

    mgm::LinuxVirtualTerminal vt(fops, std::move(pops), 7, null_report);

    int const major = 123;
    int const minor = 321;
    char const* const expected_filename = "/sys/dev/char/123:321/uevent";

    EXPECT_CALL(*fops, open(StrEq(expected_filename), O_RDONLY | O_CLOEXEC))
        .WillOnce(Return(-1));

    try
    {
        vt.acquire_device(major, minor);
    }
    catch (std::system_error const&)
    {
    }
}

TEST_F(LinuxVirtualTerminalTest, throws_error_when_parsing_fails)
{
    using namespace testing;

    auto fops = std::make_shared<NiceMock<MockVTFileOperations>>();
    auto pops = std::make_unique<StubPosixProcessOperations>();
    auto null_report = mr::null_display_report();

    mgm::LinuxVirtualTerminal vt(fops, std::move(pops), 7, null_report);

    char const* const expected_filename = "/sys/dev/char/55:61/uevent";
    char const* const uevent_content =
        "MAJOR=56\n"
        "MINOR=61\n"
        "I_AINT_NO_DEVNAME=fb0";

    mir::AnonymousShmFile uevent{strlen(uevent_content)};
    ::memcpy(uevent.base_ptr(), uevent_content, strlen(uevent_content));

    EXPECT_CALL(*fops, open(StrEq(expected_filename), O_RDONLY | O_CLOEXEC))
        .WillOnce(Return(uevent.fd()));

    try
    {
        vt.acquire_device(55, 61);
    }
    catch (std::runtime_error const&)
    {
    }
}

TEST_F(LinuxVirtualTerminalTest, opens_correct_device_node)
{
    using namespace testing;

    auto fops = std::make_shared<NiceMock<MockVTFileOperations>>();
    auto pops = std::make_unique<StubPosixProcessOperations>();
    auto null_report = mr::null_display_report();

    mgm::LinuxVirtualTerminal vt(fops, std::move(pops), 7, null_report);

    char const* const expected_filename = "/sys/dev/char/55:61/uevent";
    char const* const uevent_content =
        "MAJOR=55\n"
        "MINOR=61\n"
        "DEVNAME=fb0";

    mir::AnonymousShmFile uevent{strlen(uevent_content)};
    ::memcpy(uevent.base_ptr(), uevent_content, strlen(uevent_content));

    EXPECT_CALL(*fops, open(StrEq(expected_filename), O_RDONLY | O_CLOEXEC))
        .WillOnce(Return(uevent.fd()));
    EXPECT_CALL(*fops, open(StrEq("/dev/fb0"), O_RDWR | O_CLOEXEC))
        .WillOnce(Return(-1));

    auto device = vt.acquire_device(55, 61);

    try
    {
        device.get();
    }
    catch(...)
    {
        // boost::unique_future appears to lose the type of the exception?
    }
}

TEST_F(LinuxVirtualTerminalTest, callback_receives_correct_device_fd)
{
    using namespace testing;

    auto fops = std::make_shared<NiceMock<MockVTFileOperations>>();
    auto pops = std::make_unique<StubPosixProcessOperations>();
    auto null_report = mr::null_display_report();

    mgm::LinuxVirtualTerminal vt(fops, std::move(pops), 7, null_report);

    char const* const expected_filename = "/sys/dev/char/55:61/uevent";
    char const uevent_content[] =
        "MAJOR=55\n"
        "MINOR=61\n"
        "DEVNAME=input/event3";

    mir::AnonymousShmFile uevent{sizeof(uevent_content)};
    ::memcpy(uevent.base_ptr(), uevent_content, sizeof(uevent_content));

    EXPECT_CALL(*fops, open(StrEq(expected_filename), O_RDONLY | O_CLOEXEC))
        .WillOnce(Return(uevent.fd()));

    char const device_content[] =
        "I am the very model of a modern major general";
    mir::AnonymousShmFile fake_device_node(sizeof(device_content));
    ::memcpy(fake_device_node.base_ptr(), device_content, sizeof(device_content));

    EXPECT_CALL(*fops, open(StrEq("/dev/input/event3"), O_RDWR | O_CLOEXEC))
        .WillOnce(Return(fake_device_node.fd()));

    auto fd = vt.acquire_device(55, 61).get();
    char buffer[sizeof(device_content)];

    auto read_bytes = read(fd, buffer, sizeof(device_content));
    EXPECT_THAT(read_bytes, Eq(sizeof(device_content)));
    EXPECT_THAT(buffer, StrEq(device_content));
}
