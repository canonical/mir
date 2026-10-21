/*
 * Copyright © Canonical Ltd.
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

#include <string>
#include <fstream>

namespace mir
{
namespace server
{
namespace utils
{

static auto read_procfs_property(pid_t const client_pid,
                                 std::string const& property) -> std::string
{
    std::string val;
    auto const client_prop = "/proc/" + std::to_string(client_pid) + "/" + property;
    std::fstream prop_stream(client_prop, std::ios_base::in);
    prop_stream >> val;
    return val;
}

static auto create_session_name(pid_t const client_pid, int const client_fd) -> std::string
{
    std::string comm;
    try
    {
        comm = read_procfs_property(client_pid, "comm");
        if (!comm.empty() && comm.back() == '\n')
            comm.erase('\n');
    }
    catch (...)
    {
        comm = "unknown";
    }

    auto const to_hash = std::to_string(client_pid) +
                         ":" +
                         std::to_string(client_fd);
    auto const hashed = std::hash<std::string>{}(to_hash);

    return comm + ":" + std::to_string(hashed);
}

}
}
}
