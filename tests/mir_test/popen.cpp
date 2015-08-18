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
 *              Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include <mir/test/popen.h>

#include <system_error>

namespace mt = mir::test;

mt::Popen::Popen(std::string const& cmd)
    : raw_stream{popen(cmd.c_str(), "r"), [](FILE* f){ pclose(f); }}
{
    if (!raw_stream)
        throw std::system_error(errno, std::system_category(),
                                "popen failed for `"+cmd+"'");
}

bool mt::Popen::get_line(std::string& line)
{
    FILE* in = raw_stream.get();
    if (!in)
        return false;

    char* got = 0;
    char buf[1024];

    line.clear();
    do
    {
        got = fgets(buf, sizeof buf, in);
        if (got)
            line.append(got);
    } while (got && !feof(in) && !line.empty() && line.back() != '\n');

    if (!line.empty() && line.back() == '\n')
        line.pop_back();

    return !line.empty() || !feof(in);
}
