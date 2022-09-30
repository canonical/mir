
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

#include <linux/memfd.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <boost/throw_exception.hpp>
#include <experimental/type_traits>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

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

int memfd_create(char const* name, unsigned int flags)
{
    return static_cast<int>(syscall(SYS_memfd_create, name, flags));
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
            char template_filename[] = "/dev/shm/wlcs-buffer-XXXXXX";
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
}

template<typename pool>
using can_get_wo_range =
    decltype(mir::shm::get_wo_range(pool{}, size_t{}, size_t{}));

template<typename pool>
using can_get_ro_range =
    decltype(mir::shm::get_ro_range(pool{}, mir::Fd{}, size_t{}));

template<typename pool>
using can_get_rw_range =
    decltype(mir::shm::get_rw_range(pool{}, mir::Fd{}, size_t{}));

TEST(ShmBacking, can_not_get_writeable_range_on_ro_pool)
{
    constexpr bool can_get_writable_range =
        std::experimental::is_detected_v<
            can_get_wo_range,
            decltype(mir::shm::make_shm_backing_store<PROT_READ>(mir::Fd{}, size_t{}))>;

    constexpr bool can_get_readwrite_range =
        std::experimental::is_detected_v<
            can_get_rw_range,
            decltype(mir::shm::make_shm_backing_store<PROT_READ>(mir::Fd{}, size_t{}))>;

    EXPECT_FALSE(can_get_writable_range);
    EXPECT_FALSE(can_get_readwrite_range);
}

TEST(ShmBacking, can_not_get_readable_range_on_wo_pool)
{
    constexpr bool can_get_readable_range =
        std::experimental::is_detected_v<
            can_get_ro_range,
            decltype(mir::shm::make_shm_backing_store<PROT_WRITE>(mir::Fd{}, size_t{}))>;

    constexpr bool can_get_readwrite_range =
        std::experimental::is_detected_v<
            can_get_rw_range,
            decltype(mir::shm::make_shm_backing_store<PROT_WRITE>(mir::Fd{}, size_t{}))>;

    EXPECT_FALSE(can_get_readable_range);
    EXPECT_FALSE(can_get_readwrite_range);
}

TEST(ShmBacking, can_get_rw_range_covering_whole_pool)
{
    using namespace testing;

    constexpr size_t const shm_size = 4000;
    auto shm_fd = make_shm_fd(shm_size);
    auto backing = mir::shm::make_shm_backing_store<PROT_READ | PROT_WRITE>(shm_fd, shm_size);

    auto mappable = mir::shm::get_rw_range(backing, 0, shm_size);

    auto mapping = mappable->map_rw();

    constexpr unsigned char const fill_value{0xab};
    ::memset(mapping->data(), fill_value, shm_size);
    for(auto i = 0; i < shm_size; ++i)
    {
        EXPECT_THAT(mapping->data()[i], Eq(fill_value));
    }
}

TEST(ShmBacking, get_rw_range_checks_the_range_fits)
{
    using namespace testing;

    constexpr size_t const shm_size = 4000;
    auto shm_fd = make_shm_fd(shm_size);
    auto backing = mir::shm::make_shm_backing_store<PROT_READ | PROT_WRITE>(shm_fd, shm_size);

    // Check each range from [0, shm_size + 1] - [shm_size - 1, shm_size + 1]
    for (auto i = 0u; i < shm_size - 1; ++i)
    {
        EXPECT_THROW(
            mir::shm::get_rw_range(backing, i, shm_size + 1 - i),
            std::runtime_error
        );
    }
}

TEST(ShmBacking, get_rw_range_checks_handle_overflows)
{
    using namespace testing;

    constexpr size_t const shm_size = 4000;
    auto shm_fd = make_shm_fd(shm_size);
    auto backing = mir::shm::make_shm_backing_store<PROT_READ | PROT_WRITE>(shm_fd, shm_size);

    EXPECT_THROW(
        mir::shm::get_rw_range(backing, std::numeric_limits<size_t>::max() - 1, 2),
        std::runtime_error
    );

    EXPECT_THROW(
        mir::shm::get_rw_range(backing, 2, std::numeric_limits<size_t>::max() - 1),
        std::runtime_error
    );
}

TEST(ShmBacking, two_rw_ranges_see_each_others_changes)
{
    using namespace testing;

    constexpr size_t const shm_size = 4000;
    auto shm_fd = make_shm_fd(shm_size);
    auto backing = mir::shm::make_shm_backing_store<PROT_READ | PROT_WRITE>(shm_fd, shm_size);

    auto range_one = mir::shm::get_rw_range(backing, 0, shm_size);
    auto range_two = mir::shm::get_rw_range(backing, shm_size / 2, shm_size / 2);

    auto map_one = range_one->map_rw();
    auto map_two = range_two->map_rw();

    int const mapping_one_fill = 0xaa;
    int const mapping_two_fill = 0xce;
    ::memset(map_one->data(), mapping_one_fill, map_one->len());
    ::memset(map_two->data(), mapping_two_fill, map_two->len());

    for (auto const& a : std::span(map_two->data(), map_two->len()))
    {
        EXPECT_THAT(a, Eq(mapping_two_fill));
    }

    for (auto i = 0; i < shm_size / 2; ++i)
    {
        EXPECT_THAT(map_one->data()[i], Eq(mapping_one_fill));
    }
    for (auto i = shm_size / 2; i < shm_size; ++i)
    {
        EXPECT_THAT(map_one->data()[i], Eq(mapping_two_fill));
    }
}