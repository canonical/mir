/*
 * Copyright © 2018 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "src/server/console/minimal_console_services.h"
#include "mir_test_framework/open_wrapper.h"
#include "mir/test/doubles/mock_drm.h"
#include "mir/test/doubles/simple_device_observer.h"

#include "mir/anonymous_shm_file.h"

#include <fcntl.h>
#include <drm.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mtf = mir_test_framework;
namespace mtd = mir::test::doubles;

namespace
{
bool flag_is_set(int flag, int bitfield)
{
    return (flag & bitfield) == flag;
}

std::string uevent_content_for_device(
    int major, int minor,
    char const* device_name)
{
    std::stringstream content;

    if (strncmp(device_name, "/dev/", strlen("/dev/")) != 0)
    {
        throw std::logic_error{"device_name is expected to be the fully-qualified /dev/foo path"};
    }

    content
        << "MAJOR=" << major << "\n"
        << "MINOR=" << minor << "\n"
        << "DEVNAME=" << device_name + strlen ("/dev/") << "\n";

    return content.str();
}
}

using namespace testing;

class MinimalConsoleServicesTest : public testing::Test
{
public:
    NiceMock<mtd::MockDRM> drm;

    mir::Fd set_expectations_for_uevent_probe_of_device(
        int major, int minor,
        char const* device_name)
    {
        set_expectations_for_uevent_probe(
            major, minor,
            uevent_content_for_device(major, minor, device_name).c_str());

        mir::Fd stub_fd{::open("/dev/null", O_RDWR | O_CLOEXEC)};
        expectations.emplace_back(
            mtf::add_open_handler(
                [stub_fd, device_path = std::string{device_name}](char const* path, int, mode_t)
                    -> std::experimental::optional<int>
                {
                    if (device_path == path)
                    {
                        return static_cast<int>(stub_fd);
                    }
                    return {};
                }));
        return stub_fd;
    }

    mir::Fd set_expectations_for_uevent_probe_of_drm(int minor, char const* device_name)
    {
        auto const uevent_content =
            uevent_content_for_device(226, minor, device_name) +
            "DEVTYPE=drm_minor\n";

        set_expectations_for_uevent_probe(226, minor, uevent_content.c_str());

        mir::Fd stub_fd{::open("/dev/null", O_RDWR | O_CLOEXEC)};
        ON_CALL(drm, open(StrEq(device_name), _, _)).WillByDefault(
            InvokeWithoutArgs([stub_fd]() { return stub_fd; }));
        return stub_fd;
    }
private:
    void set_expectations_for_uevent_probe(int major, int minor, char const* sysfile_content)
    {
        using namespace testing;

        std::stringstream expected_filename;
        expected_filename << "/sys/dev/char/" << major << ":" << minor << "/uevent";

        auto uevent = std::make_shared<mir::AnonymousShmFile>(strlen(sysfile_content));
        ::memcpy(uevent->base_ptr(), sysfile_content, strlen(sysfile_content));

        expectations.emplace_back(
            mtf::add_open_handler(
                [expected_filename = expected_filename.str(), uevent](char const* path, int flags, mode_t)
                    -> std::experimental::optional<int>
                {
                    if (expected_filename == path)
                    {
                        if (flag_is_set(O_RDONLY, flags) && flag_is_set(O_CLOEXEC, flags))
                        {
                            return uevent->fd();
                        }
                    }
                    return {};
                }));
    }

    std::vector<mtf::OpenHandlerHandle> expectations;
};

TEST_F(MinimalConsoleServicesTest, calls_drm_set_master_if_not_already_master)
{
    int drm_fd = set_expectations_for_uevent_probe_of_drm(5, "/dev/dri/card5");

#ifdef MIR_LIBDRM_HAS_IS_MASTER
    ON_CALL(drm, drmIsMaster(drm_fd))
        .WillByDefault(Return(0));
#else
    // Older versions of libdrm do not provides drmIsMaster()
    // drmIsMaster checks the ATTACHMODE ioctl, detecting EPERM as not master.
    ON_CALL(drm, drmIoctl(drm_fd, DRM_IOCTL_MODE_ATTACHMODE, _))
        .WillByDefault(SetErrnoAndReturn(EPERM, -1));
#endif

    EXPECT_CALL(drm, drmSetMaster(drm_fd));

    mir::MinimalConsoleServices services;
    bool activated{false};
    auto observer = std::make_unique<mtd::SimpleDeviceObserver>(
        [&activated](auto&& fd)
        {
            EXPECT_THAT(fd, Not(Eq(mir::Fd::invalid)));
            activated = true;
        },
        [](){},
        [](){});

    services.acquire_device(226, 5, std::move(observer));

    EXPECT_TRUE(activated);
}

TEST_F(MinimalConsoleServicesTest, failure_to_set_master_is_fatal)
{
    int drm_fd = set_expectations_for_uevent_probe_of_drm(5, "/dev/dri/card5");

#ifdef MIR_LIBDRM_HAS_IS_MASTER
    ON_CALL(drm, drmIsMaster(drm_fd))
        .WillByDefault(Return(0));
#else
    // Older versions of libdrm do not provides drmIsMaster()
    // drmIsMaster checks the ATTACHMODE ioctl, detecting EPERM as not master.
    ON_CALL(drm, drmIoctl(drm_fd, DRM_IOCTL_MODE_ATTACHMODE, _))
        .WillByDefault(SetErrnoAndReturn(EPERM, -1));
#endif

    ON_CALL(drm, drmSetMaster(drm_fd)).WillByDefault(Return(-EPERM));

    mir::MinimalConsoleServices services;
    auto observer = std::make_unique<mtd::SimpleDeviceObserver>(
        [](auto){},
        [](){},
        [](){});

    try
    {
        services.acquire_device(226, 5, std::move(observer)).get();
        FAIL();
    }
    catch (std::runtime_error const& err)
    {
        EXPECT_THAT(err.what(), HasSubstr("Failed to acquire DRM master"));
    }
}

TEST_F(MinimalConsoleServicesTest, does_not_call_set_master_if_already_master)
{
    int drm_fd = set_expectations_for_uevent_probe_of_drm(5, "/dev/dri/card5");

#ifdef MIR_LIBDRM_HAS_IS_MASTER
    ON_CALL(drm, drmIsMaster(drm_fd))
        .WillByDefault(Return(1));
#else
    // Older versions of libdrm do not provides drmIsMaster()
    // drmIsMaster checks the ATTACHMODE ioctl, detecting EPERM as not master.
    ON_CALL(drm, drmIoctl(drm_fd, DRM_IOCTL_MODE_ATTACHMODE, _))
        .WillByDefault(SetErrnoAndReturn(EINVAL, -1));
#endif

    ON_CALL(drm, drmSetMaster(drm_fd)).WillByDefault(Return(-EPERM));

    mir::MinimalConsoleServices services;
    bool activated{false};
    auto observer = std::make_unique<mtd::SimpleDeviceObserver>(
        [&activated](auto&& fd)
        {
            EXPECT_THAT(fd, Not(Eq(mir::Fd::invalid)));
            activated = true;
        },
        [](){},
        [](){});

    services.acquire_device(226, 5, std::move(observer));

    EXPECT_TRUE(activated);
}

TEST_F(MinimalConsoleServicesTest, doesnt_try_to_set_master_on_non_drm_devices)
{
    int device_fd = set_expectations_for_uevent_probe_of_device(33, 22, "/dev/input/event2");
    EXPECT_CALL(drm, drmSetMaster(device_fd)).Times(0);

    mir::MinimalConsoleServices services;
    bool activated{false};
    auto observer = std::make_unique<mtd::SimpleDeviceObserver>(
        [&activated](auto&& fd)
        {
            EXPECT_THAT(fd, Not(Eq(mir::Fd::invalid)));
            activated = true;
        },
        [](){},
        [](){});

    services.acquire_device(33, 22, std::move(observer));

    EXPECT_TRUE(activated);
}

TEST_F(MinimalConsoleServicesTest, failure_to_open_device_node_returns_exceptional_future)
{
    char const* device_path = "/dev/input/event3";
    set_expectations_for_uevent_probe_of_device(33, 5, device_path);

    auto error_on_device_open = mtf::add_open_handler(
        [device_path](char const* path, int, mode_t) -> std::experimental::optional<int>
        {
            if (!strcmp(device_path, path))
            {
                errno = ENODEV;
                return {-1};
            }
            return {};
        });

    mir::MinimalConsoleServices services;
    bool activated{false};
    auto observer = std::make_unique<mtd::SimpleDeviceObserver>(
        [&activated](auto&&)
        {
            activated = true;
        },
        [](){},
        [](){});

    auto device = services.acquire_device(33, 5, std::move(observer));

    bool exception_caught{false};
    try
    {
        device.get();
    }
    catch (std::system_error const& err)
    {
        EXPECT_THAT(err.code(), Eq(std::error_code{ENODEV, std::system_category()}));
        exception_caught = true;
    }

    EXPECT_FALSE(activated);
    EXPECT_TRUE(exception_caught);
}

TEST_F(MinimalConsoleServicesTest, failure_to_open_sys_file_results_in_immediate_exception)
{
    set_expectations_for_uevent_probe_of_device(33, 5, "/dev/input/event2");

    auto error_on_device_open = mtf::add_open_handler(
        [](char const* path, int, mode_t) -> std::experimental::optional<int>
        {
            if (!strncmp("/sys", path, strlen("/sys")))
            {
                errno = EINVAL;
                return {-1};
            }
            return {};
        });

    mir::MinimalConsoleServices services;
    bool activated{false};
    auto observer = std::make_unique<mtd::SimpleDeviceObserver>(
        [&activated](auto&&)
        {
            activated = true;
        },
        [](){},
        [](){});

    bool exception_caught{false};
    try
    {
        services.acquire_device(33, 5, std::move(observer));
    }
    catch (std::system_error const& err)
    {
        EXPECT_THAT(err.code(), Eq(std::error_code{EINVAL, std::system_category()}));
        exception_caught = true;
    }

    EXPECT_FALSE(activated);
    EXPECT_TRUE(exception_caught);
}

namespace
{
MATCHER_P(FlagIsSet, flag, "")
{
    return (arg & flag) == flag;
}
}

TEST_F(MinimalConsoleServicesTest, opens_input_devices_in_nonblocking_mode)
{
    char const* device_path = "/dev/input/event22";
    set_expectations_for_uevent_probe_of_device(33, 22, device_path);

    auto open_handler = mtf::add_open_handler(
        [device_path, fd = mir::Fd{::open("/dev/null", O_RDWR)}](
            char const* path,
            int flags,
            mode_t) -> std::experimental::optional<int>
        {
            if (strcmp(path, device_path))
            {
                return {};
            }

            EXPECT_THAT(flags, FlagIsSet(O_RDWR));
            EXPECT_THAT(flags, FlagIsSet(O_CLOEXEC));
            EXPECT_THAT(flags, FlagIsSet(O_NONBLOCK));

            return static_cast<int>(fd);
        });

    mir::MinimalConsoleServices services;
    bool activated{false};
    auto observer = std::make_unique<mtd::SimpleDeviceObserver>(
        [&activated](auto&& fd)
        {
            EXPECT_THAT(fd, Not(Eq(mir::Fd::invalid)));
            activated = true;
        },
        [](){},
        [](){});

    services.acquire_device(33, 22, std::move(observer));

    EXPECT_TRUE(activated);
}

TEST_F(MinimalConsoleServicesTest, does_not_open_drm_devices_in_nonblocking_mode)
{
    char const* device_path = "/dev/dri/card2";
    set_expectations_for_uevent_probe_of_drm(2, device_path);

    auto open_handler = mtf::add_open_handler(
        [device_path, fd = mir::Fd{::open("/dev/null", O_RDWR)}](
            char const* path,
            int flags,
            mode_t) -> std::experimental::optional<int>
        {
            if (strcmp(path, device_path))
            {
                return {};
            }

            EXPECT_THAT(flags, FlagIsSet(O_RDWR));
            EXPECT_THAT(flags, FlagIsSet(O_CLOEXEC));
            EXPECT_THAT(flags, Not(FlagIsSet(O_NONBLOCK)));

            return static_cast<int>(fd);
        });

    mir::MinimalConsoleServices services;
    bool activated{false};
    auto observer = std::make_unique<mtd::SimpleDeviceObserver>(
        [&activated](auto&& fd)
        {
            EXPECT_THAT(fd, Not(Eq(mir::Fd::invalid)));
            activated = true;
        },
        [](){},
        [](){});

    services.acquire_device(226, 2, std::move(observer));

    EXPECT_TRUE(activated);
}
