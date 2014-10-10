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
 * Authored by: Alberto Aguirre <alberto.aguirre@canonical.com>
 */

#include <mir_test/popen.h>

#include <boost/throw_exception.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream_buffer.hpp>

#include <system_error>

namespace mt = mir::test;
namespace io = boost::iostreams;

namespace
{
std::unique_ptr<std::streambuf> create_stream_buffer(FILE* raw_stream)
{
    if (raw_stream == nullptr)
    {
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(std::system_error(errno, std::system_category(), "popen failed")));
    }

    int fd = fileno(raw_stream);
    if (fd == -1)
    {
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::system_error(errno, std::system_category(), "invalid file stream")));
    }
    auto raw = new io::stream_buffer<io::file_descriptor_source>{
        fd, io::never_close_handle};
    return std::unique_ptr<std::streambuf>(raw);
}
}

mt::Popen::Popen(std::string const& cmd)
    : raw_stream{popen(cmd.c_str(), "r"), [](FILE* f){ pclose(f); }},
      stream_buffer{create_stream_buffer(raw_stream.get())},
      stream{stream_buffer.get()}
{
}

bool mt::Popen::get_line(std::string& line)
{
    return std::getline(stream, line);
}
