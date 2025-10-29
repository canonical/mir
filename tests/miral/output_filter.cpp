/*
* Copyright © Canonical Ltd.
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
 */

#include <miral/output_filter.h>

#include <miral/live_config_ini_file.h>
#include <miral/test_server.h>

namespace
{
struct OutputFilter : miral::TestServer
{
    OutputFilter()
    {
        start_server_in_setup = false;
    }
};
}

TEST_F(OutputFilter, register_when_default_constructed)
{
    miral::OutputFilter output_filter;
    add_server_init([&output_filter](mir::Server& server) { output_filter(server); });
    start_server();
}

TEST_F(OutputFilter, register_with_live_config)
{
    miral::live_config::IniFile ini_file;
    miral::OutputFilter output_filter{ini_file};
    add_server_init([&output_filter](mir::Server& server) { output_filter(server); });
    start_server();
}

TEST_F(OutputFilter, register_when_constructed_with_none)
{
    miral::OutputFilter output_filter{MirOutputFilter::mir_output_filter_none};
    add_server_init([&output_filter](mir::Server& server) { output_filter(server); });
    start_server();
}

TEST_F(OutputFilter, register_when_constructed_with_grayscale)
{
    miral::OutputFilter output_filter{MirOutputFilter::mir_output_filter_grayscale};
    add_server_init([&output_filter](mir::Server& server) { output_filter(server); });
    start_server();
}

TEST_F(OutputFilter, register_when_constructed_with_invert)
{
    miral::OutputFilter output_filter{MirOutputFilter::mir_output_filter_invert};
    add_server_init([&output_filter](mir::Server& server) { output_filter(server); });
    start_server();
}
