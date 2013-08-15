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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#include "mir_test_framework/cross_process_sync.h"

#include <boost/exception/errinfo_errno.hpp>

#include <poll.h>
#include <unistd.h>

namespace mtf = mir_test_framework;

namespace
{
const int read_fd = 0;
const int write_fd = 1;

struct UnexpectedValueErrorInfoTag {};
typedef boost::error_info<UnexpectedValueErrorInfoTag, unsigned int> errinfo_unexpected_value;
}

mtf::CrossProcessSync::CrossProcessSync() : counter(0)
{
    if (::pipe(fds) < 0)
    {
        BOOST_THROW_EXCEPTION(
            ::boost::enable_error_info(std::runtime_error("Failed to create pipe"))
            << boost::errinfo_errno(errno));
    }
}

mtf::CrossProcessSync::CrossProcessSync(const CrossProcessSync& rhs) : counter(rhs.counter)
{
    fds[0] = ::dup(rhs.fds[0]);
    fds[1] = ::dup(rhs.fds[1]);
}

mtf::CrossProcessSync::~CrossProcessSync() noexcept
{
    ::close(fds[0]);
    ::close(fds[1]);
}

mtf::CrossProcessSync& mtf::CrossProcessSync::operator=(const mtf::CrossProcessSync& rhs)
{
    ::close(fds[0]);
    ::close(fds[1]);
    fds[0] = ::dup(rhs.fds[0]);
    fds[1] = ::dup(rhs.fds[1]);

    counter = rhs.counter;

    return *this;
}

void mtf::CrossProcessSync::try_signal_ready_for(const std::chrono::milliseconds& duration)
{
    static const short empty_revents = 0;
    pollfd poll_fd[1] = { { fds[write_fd], POLLOUT, empty_revents } };
    int rc = -1;

    if ((rc = ::poll(poll_fd, 1, duration.count())) < 0)
    {         
        BOOST_THROW_EXCEPTION(
            ::boost::enable_error_info(std::runtime_error("Error while polling pipe to become writable"))
            << boost::errinfo_errno(errno));
    }
    else if (rc == 0)
    {
        throw std::runtime_error("Poll on writefd for pipe timed out");
    }

    int value = 1;
    if (sizeof(value) != write(fds[write_fd], std::addressof(value), sizeof(value)))
    {
        BOOST_THROW_EXCEPTION(
            ::boost::enable_error_info(std::runtime_error("Error while writing to pipe"))
            << boost::errinfo_errno(errno));
    }
}

unsigned int mtf::CrossProcessSync::wait_for_signal_ready_for(const std::chrono::milliseconds& duration)
{
    static const short empty_revents = 0;
    pollfd poll_fd[1] = { { fds[read_fd], POLLIN, empty_revents } };
    int rc = -1;

    if ((rc = ::poll(poll_fd, 1, duration.count())) < 0)
    {
        BOOST_THROW_EXCEPTION(
            ::boost::enable_error_info(std::runtime_error("Error while polling pipe to become readable"))
            << boost::errinfo_errno(errno));
    }
    else if (rc == 0)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Poll on readfd for pipe timed out"));
    }

    int value;
    if (sizeof(value) != read(fds[read_fd], std::addressof(value), sizeof(value)))
    {
        BOOST_THROW_EXCEPTION(
            ::boost::enable_error_info(std::runtime_error("Error while reading from pipe"))
            << boost::errinfo_errno(errno));
    }

    if (value != 1)
    {
        BOOST_THROW_EXCEPTION(
            ::boost::enable_error_info(std::runtime_error("Read an unexpected value from pipe"))
            << errinfo_unexpected_value(value));
    }

    counter += value;

    return counter;
}

void mtf::CrossProcessSync::signal_ready()
{
    try_signal_ready_for(std::chrono::milliseconds{-1});
}

unsigned int mtf::CrossProcessSync::wait_for_signal_ready()
{
    return wait_for_signal_ready_for(std::chrono::milliseconds{-1});
}
