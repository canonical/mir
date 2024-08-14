/*
* Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
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

#include "miral/test_server.h"
#include "miral/reloading_config_file.h"

#include <wayland_wrapper.h>
#include <gmock/gmock-function-mocker.h>

#include <fstream>

using miral::ReloadingConfigFile;

namespace
{
struct TestReloadingConfigFile : miral::TestServer
{
    TestReloadingConfigFile();
    MOCK_METHOD(void, load, (std::istream& in, std::filesystem::path path), ());

    std::optional<ReloadingConfigFile> reloading_config_file;
};

char const* const no_such_file = "no/such/file";
char const* const a_file = "/tmp/a_file";

TestReloadingConfigFile::TestReloadingConfigFile()
{
    unlink(a_file);
}

void write_a_file()
{
    std::ofstream file(a_file);
    file << "some content";
}
}

TEST_F(TestReloadingConfigFile, with_no_file_nothing_is_loaded)
{
    EXPECT_CALL(*this, load).Times(0);

    invoke_runner([this](miral::MirRunner& runner)
    {
        reloading_config_file = ReloadingConfigFile{
            runner,
            no_such_file,
            [this](std::istream& in, std::filesystem::path path) { load(in, path); }};
    });
}

TEST_F(TestReloadingConfigFile, with_a_file_something_is_loaded)
{
    EXPECT_CALL(*this, load).Times(1);

    write_a_file();

    invoke_runner([this](miral::MirRunner& runner)
    {
        reloading_config_file = ReloadingConfigFile{
            runner,
            a_file,
            [this](std::istream& in, std::filesystem::path path) { load(in, path); }};
    });
}

TEST_F(TestReloadingConfigFile, when_a_file_is_written_something_is_loaded)
{
    invoke_runner([this](miral::MirRunner& runner)
    {
        reloading_config_file = ReloadingConfigFile{
            runner,
            a_file,
            [this](std::istream& in, std::filesystem::path path) { load(in, path); }};
    });

    EXPECT_CALL(*this, load).Times(1);

    write_a_file();
}


TEST_F(TestReloadingConfigFile, each_time_a_file_is_rewritten_something_is_loaded)
{
    auto const times = 42;

    EXPECT_CALL(*this, load).Times(times+1);

    write_a_file(); // Initial write

    invoke_runner([this](miral::MirRunner& runner)
    {
        reloading_config_file = ReloadingConfigFile{
            runner,
            a_file,
            [this](std::istream& in, std::filesystem::path path) { load(in, path); }};
    });

    for (auto i = 0; i < times; ++i)
    {
        write_a_file();
    }
}