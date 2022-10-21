/*
 * Copyright © 2022 Canonical Ltd.
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
 */

#include "src/server/frontend_wayland/shm_backing.h"

#include "mir_test_framework/mmap_wrapper.h"

#include <linux/memfd.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <boost/throw_exception.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <system_error>
#include <unistd.h>

namespace mtf = mir_test_framework;

namespace
{
bool error_indicates_tmpfile_not_supported(int error)
{
    return
        error == EISDIR ||        // Directory exists, but no support for O_TMPFILE
        error == ENOENT ||        // Directory doesn't exist, and no support for O_TMPFILE
        error == EOPNOTSUPP ||    // Filesystem that directory resides on does not support O_TMPFILE
        error == EINVAL;          // There apparently exists at least one development board that has a kernel
                                  // that incorrectly returns EINVAL. Yay.
}

auto make_shm_fd(size_t size) -> mir::Fd
{
    int fd = memfd_create("mir-shm-test", MFD_CLOEXEC);
    if (fd == -1 && errno == ENOSYS)
    {
        fd = open("/dev/shm", O_TMPFILE | O_RDWR | O_EXCL | O_CLOEXEC, S_IRWXU);

        // Workaround for filesystems that don't support O_TMPFILE
        if (fd == -1 && error_indicates_tmpfile_not_supported(errno))
        {
            char template_filename[] = "/dev/shm/test-shm-XXXXXX";
            fd = mkostemp(template_filename, O_CLOEXEC);
            if (fd != -1)
            {
                if (unlink(template_filename) < 0)
                {
                    close(fd);
                    fd = -1;
                }
            }
        }
    }

    if (fd == -1)
    {
        BOOST_THROW_EXCEPTION(
            std::system_error(errno, std::system_category(), "Failed to open temporary file"));
    }

    if (ftruncate(fd, size) == -1)
    {
        close(fd);
        BOOST_THROW_EXCEPTION(
            std::system_error(errno, std::system_category(), "Failed to resize temporary file"));
    }

    return mir::Fd{fd};
}

auto make_shm_fd_with_seals(size_t size, int seals) -> mir::Fd
{
    mir::Fd fd{memfd_create("mir-shm-test", MFD_CLOEXEC | MFD_ALLOW_SEALING)};
    if (fd == mir::Fd::invalid)
    {
        BOOST_THROW_EXCEPTION((std::system_error{errno, std::system_category(), "Failed to create memfd"}));
    }
    if (fcntl(fd, F_ADD_SEALS, seals) == -1)
    {
        BOOST_THROW_EXCEPTION((std::system_error{errno, std::system_category(), "Failed to seal memfd"}));
    }
    if (ftruncate(fd, size) == -1)
    {
        BOOST_THROW_EXCEPTION(
            std::system_error(errno, std::system_category(), "Failed to resize temporary file"));
    }
    return fd;
}
}

TEST(ShmBacking, can_get_rw_range_covering_whole_pool)
{
    using namespace testing;

    constexpr size_t const shm_size = 4000;
    auto shm_fd = make_shm_fd(shm_size);
    auto backing = mir::shm::rw_pool_from_fd(shm_fd, shm_size);

    auto mappable = backing->get_rw_range(0, shm_size);

    auto mapping = mappable->map_rw();

    constexpr std::byte const fill_value{0xab};
    ::memset(mapping->data(), std::to_integer<int>(fill_value), shm_size);
    for(auto i = 0; i < shm_size; ++i)
    {
        EXPECT_THAT((*mapping)[i], Eq(fill_value));
    }
}

TEST(ShmBacking, get_rw_range_checks_the_range_fits)
{
    using namespace testing;

    constexpr size_t const shm_size = 4000;
    auto shm_fd = make_shm_fd(shm_size);
    auto backing = mir::shm::rw_pool_from_fd(shm_fd, shm_size);

    // Check each range from [0, shm_size + 1] - [shm_size - 1, shm_size + 1]
    for (auto i = 0u; i < shm_size - 1; ++i)
    {
        EXPECT_THROW(
            backing->get_rw_range(i, shm_size + 1 - i),
            std::logic_error
        );
    }
}

TEST(ShmBacking, get_rw_range_checks_handle_overflows)
{
    using namespace testing;

    constexpr size_t const shm_size = 4000;
    auto shm_fd = make_shm_fd(shm_size);
    auto backing = mir::shm::rw_pool_from_fd(shm_fd, shm_size);

    EXPECT_THROW(
        backing->get_rw_range(std::numeric_limits<size_t>::max() - 1, 2),
        std::logic_error
    );

    EXPECT_THROW(
        backing->get_rw_range(2, std::numeric_limits<size_t>::max() - 1),
        std::logic_error
    );
}

TEST(ShmBacking, unmaps_memory_on_destruction)
{
    using namespace testing;

    constexpr size_t const shm_size = 4000;
    auto shm_fd = make_shm_fd(shm_size);
    auto backing = mir::shm::rw_pool_from_fd(shm_fd, shm_size);

    auto range = backing->get_rw_range(0, shm_size);
    auto map = range->map_rw();

    void* const mapping_start = map->data();
    size_t const mapping_length = map->len();

    bool unmap_called{false};
    auto interposer = mtf::add_munmap_handler(
        [mapping_start, mapping_length, &unmap_called](void* addr, size_t len) -> std::optional<int>
        {
            if (addr == mapping_start && len == mapping_length)
            {
                unmap_called = true;
                return 0;
            }
            return {};
        });

    ASSERT_FALSE(unmap_called);
    // Destroy everything
    backing = nullptr;
    range = nullptr;
    map = nullptr;

    EXPECT_TRUE(unmap_called);
}

TEST(ShmBacking, two_rw_ranges_see_each_others_changes)
{
    using namespace testing;

    constexpr size_t const shm_size = 4000;
    auto shm_fd = make_shm_fd(shm_size);
    auto backing = mir::shm::rw_pool_from_fd(shm_fd, shm_size);

    auto range_one = backing->get_rw_range(0, shm_size);
    auto range_two = backing->get_rw_range(shm_size / 2, shm_size / 2);

    auto map_one = range_one->map_rw();
    auto map_two = range_two->map_rw();

    auto const mapping_one_fill = std::byte{0xaa};
    auto const mapping_two_fill = std::byte{0xce};
    ::memset(map_one->data(), std::to_integer<int>(mapping_one_fill), map_one->len());
    ::memset(map_two->data(), std::to_integer<int>(mapping_two_fill), map_two->len());

    for (auto const& a : *map_two)
    {
        EXPECT_THAT(a, Eq(mapping_two_fill));
    }

    for (auto i = 0; i < shm_size / 2; ++i)
    {
        EXPECT_THAT((*map_one)[i], Eq(mapping_one_fill));
    }
    for (auto i = shm_size / 2; i < shm_size; ++i)
    {
        EXPECT_THAT((*map_one)[i], Eq(mapping_two_fill));
    }
}

TEST(ShmBacking, range_stays_vaild_after_backing_destroyed)
{
    using namespace testing;

    constexpr size_t const shm_size = 4000;
    auto shm_fd = make_shm_fd(shm_size);
    auto backing = mir::shm::rw_pool_from_fd(shm_fd, shm_size);

    auto range = backing->get_rw_range(0, shm_size);

    backing = nullptr;

    auto map = range->map_rw();
    ::memset(map->data(), 's', map->len());

    for (auto const& a : *map)
    {
        EXPECT_THAT(a, Eq(std::byte{'s'}));
    }
}

TEST(ShmBacking, map_into_valid_memory_is_not_marked_as_faulted)
{
    using namespace testing;

    constexpr size_t const shm_size = 4000;
    auto shm_fd = make_shm_fd(shm_size);
    auto backing = mir::shm::rw_pool_from_fd(shm_fd, shm_size);

    auto range = backing->get_rw_range(0, shm_size);

    auto map = range->map_rw();
    ::memset(map->data(), 's', map->len());

    for (auto const& a : *map)
    {
        EXPECT_THAT(a, Eq(std::byte{'s'}));
    }

    EXPECT_FALSE(range->access_fault());
}

TEST(ShmBacking, read_from_invalid_memory_returns_0)
{
    using namespace testing;

    constexpr size_t const shm_size = 4000;
    constexpr size_t const claimed_size = shm_size * 2;    // Lie about our backing size
    auto shm_fd = make_shm_fd(shm_size);
    auto backing = mir::shm::rw_pool_from_fd(shm_fd, claimed_size);

    auto range = backing->get_rw_range(0, claimed_size);

    auto map = range->map_ro();

    for (auto const& a : *map)
    {
        EXPECT_THAT(a, Eq(std::byte{0}));
    }
}

TEST(ShmBacking, access_fault_is_true_after_invaild_read)
{
    using namespace testing;

    constexpr size_t const shm_size = 4000;
    constexpr size_t const claimed_size = shm_size * 2;    // Lie about our backing size
    auto shm_fd = make_shm_fd(shm_size);
    auto backing = mir::shm::rw_pool_from_fd(shm_fd, claimed_size);

    auto range = backing->get_rw_range(0, claimed_size);

    auto map = range->map_ro();

    for (auto const& a : *map)
    {
        EXPECT_THAT(a, Eq(std::byte{0}));
    }

    EXPECT_TRUE(range->access_fault());
}

TEST(ShmBacking, access_fault_is_true_after_invaild_write)
{
    using namespace testing;

    constexpr size_t const shm_size = 4000;
    constexpr size_t const claimed_size = shm_size * 2;    // Lie about our backing size
    auto shm_fd = make_shm_fd(shm_size);
    auto backing = mir::shm::rw_pool_from_fd(shm_fd, claimed_size);

    auto range = backing->get_rw_range(0, claimed_size);

    auto map = range->map_wo();

    ::memset(map->data(), 0xab, map->len());

    EXPECT_TRUE(range->access_fault());
}

TEST(ShmBacking, access_into_invalid_range_works_even_after_backing_destroyed)
{
    using namespace testing;

    constexpr size_t const shm_size = 4000;
    constexpr size_t const claimed_size = shm_size * 2;
    auto shm_fd = make_shm_fd(shm_size);
    auto backing = mir::shm::rw_pool_from_fd(shm_fd, claimed_size);

    auto range = backing->get_rw_range(0, claimed_size);

    backing = nullptr;

    auto map = range->map_rw();

    for (auto const& a : *map)
    {
        // We've not initialised anything, so we expect the kernel to 0-initialise
        EXPECT_THAT(a, Eq(std::byte{0}));
    }
}

MATCHER_P(SignalHandlerIsEqual, handler, "")
{
    using namespace testing;

    // TODO: Should test handler.sa_mask, but that requires introspecting the opaque sigset_t and is annoying
    bool is_equal = ExplainMatchResult(Eq(handler.sa_flags), arg.sa_flags, result_listener);
    if (handler.sa_flags & SA_SIGINFO)
    {
        is_equal &= ExplainMatchResult(Eq(handler.sa_sigaction), arg.sa_sigaction, result_listener);
    }
    else
    {
        is_equal &= ExplainMatchResult(Eq(handler.sa_handler), arg.sa_handler, result_listener);
    }
    return is_equal;
}

TEST(ShmBacking, doesnt_install_sigbus_handler_when_backing_is_safe)
{
    using namespace testing;

    constexpr size_t const shm_size = 4000;

    mir::Fd shm_fd;
    try
    {
        shm_fd = make_shm_fd_with_seals(shm_size, F_SEAL_SHRINK);
    }
    catch (std::system_error const&)
    {
        GTEST_SKIP();    // We can't allocate a memfd, so we can't test F_SEAL
    }

    // Store the initial SIGBUS handler
    struct sigaction initial_sigbus_handler;
    sigaction(SIGBUS, nullptr, &initial_sigbus_handler);

    // Construct a backing, a range from it, and map from the range.
    // This should install a SIGBUS handler if it were necessary
    auto backing = mir::shm::rw_pool_from_fd(shm_fd, shm_size);
    auto range = backing->get_rw_range(0, shm_size);
    auto map = range->map_rw();

    struct sigaction new_sigbus_handler;
    sigaction(SIGBUS, nullptr, &new_sigbus_handler);

    EXPECT_THAT(new_sigbus_handler, SignalHandlerIsEqual(initial_sigbus_handler));
}

TEST(ShmBacking, can_resize_pool)
{
    using namespace testing;

    constexpr size_t const initial_size = 4000;
    constexpr size_t const new_size = initial_size + 400;

    auto shm_fd = make_shm_fd(initial_size);
    auto backing = mir::shm::rw_pool_from_fd(shm_fd, initial_size);

    if (ftruncate(shm_fd, new_size) == -1)
    {
        BOOST_THROW_EXCEPTION((std::system_error{errno, std::system_category(), "Failed to resize shm fd"}));
    }
    backing->resize(new_size);

    auto range = backing->get_rw_range(0, new_size);
    auto map = range->map_rw();

    std::byte const expected_content{0xfa};
    ::memset(map->data(), std::to_integer<int>(expected_content), map->len());

    for(auto const& a : *map)
    {
        EXPECT_THAT(a, Eq(expected_content));
    }
}

TEST(ShmBacking, ranges_remain_valid_after_resize)
{
    using namespace testing;

    constexpr size_t const initial_size = 4000;
    constexpr size_t const new_size = initial_size + 400;

    auto shm_fd = make_shm_fd(initial_size);
    auto backing = mir::shm::rw_pool_from_fd(shm_fd, initial_size);

    // *First*, get a range from the pool...
    auto range = backing->get_rw_range(0, initial_size);

    if (ftruncate(shm_fd, new_size) == -1)
    {
        BOOST_THROW_EXCEPTION((std::system_error{errno, std::system_category(), "Failed to resize shm fd"}));
    }
    // ...then resize the pool...
    backing->resize(new_size);

    // ...and map the range.
    auto map = range->map_rw();

    std::byte const expected_content{0xfa};
    ::memset(map->data(), std::to_integer<int>(expected_content), map->len());

    for(auto const& a : *map)
    {
        EXPECT_THAT(a, Eq(expected_content));
    }
}

TEST(ShmBacking, mapping_remains_valid_after_resize)
{
    using namespace testing;

    constexpr size_t const initial_size = 4000;
    constexpr size_t const new_size = initial_size + 400;

    auto shm_fd = make_shm_fd(initial_size);
    auto backing = mir::shm::rw_pool_from_fd(shm_fd, initial_size);

    // *First*, get a range from the pool...
    auto range = backing->get_rw_range(0, initial_size);
    // ...and map the range.
    auto map = range->map_rw();

    if (ftruncate(shm_fd, new_size) == -1)
    {
        BOOST_THROW_EXCEPTION((std::system_error{errno, std::system_category(), "Failed to resize shm fd"}));
    }
    // ...*then* resize the pool...
    backing->resize(new_size);

    std::byte const expected_content{0xfa};
    ::memset(map->data(), std::to_integer<int>(expected_content), map->len());

    for(auto const& a : *map)
    {
        EXPECT_THAT(a, Eq(expected_content));
    }
}

TEST(ShmBacking, resize_rechecks_backing_size)
{
    using namespace testing;

    constexpr size_t const shm_size = 4000;

    mir::Fd shm_fd;
    try
    {
        shm_fd = make_shm_fd_with_seals(shm_size, F_SEAL_SHRINK);
    }
    catch (std::system_error const&)
    {
        GTEST_SKIP();    // We can't allocate a memfd, so we can't test F_SEAL
    }

    // Verifyably claim that we're shm_size...
    auto backing = mir::shm::rw_pool_from_fd(shm_fd, shm_size);
    // ...verify we can fill the mapping...
    std::byte const fill{0xae};
    {
        auto range = backing->get_rw_range(0, shm_size);
        auto map = range->map_wo();
        for (auto& byte : *map)
        {
            byte = fill;
        }
    }
    // ...and then lie that we're now bigger...
    backing->resize(shm_size * 2);

    // ...now, check that our lies can't crash the process.
    auto range = backing->get_rw_range(0, shm_size * 2);
    auto map = range->map_rw();

    for (auto i = 0; i < shm_size; ++i)
    {
        EXPECT_THAT((*map)[i], Eq(fill));
    }
    for (auto i = shm_size; i < shm_size * 2; ++i)
    {
        EXPECT_THAT((*map)[i], Eq(std::byte{0}));
    }
}
