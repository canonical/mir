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
char const* const no_such_file = "no/such/file";
char const* const a_file = "/tmp/a_file";

class PendingLoad
{
public:
    void mark_pending()
    {
        std::lock_guard lock{mutex};
        pending_loads = true;
    }

    void wait_for_load()
    {
        std::unique_lock lock{mutex};

        if (!cv.wait_for(lock, std::chrono::milliseconds{1000}, [this] { return !pending_loads; }))
        {
            std::cerr << "wait_for_load() failed" << std::endl;
        }
    }

    void notify_load()
    {
        {
            std::lock_guard lock{mutex};
            pending_loads = false;
        }

        cv.notify_one();
    }

private:
    std::mutex mutex;
    std::condition_variable cv;
    bool pending_loads = false;
};

struct TestReloadingConfigFile : miral::TestServer, PendingLoad
{
    TestReloadingConfigFile();
    MOCK_METHOD(void, load, (std::istream& in, std::filesystem::path path), ());

    std::optional<ReloadingConfigFile> reloading_config_file;

    void write_a_file()
    {
        mark_pending();
        std::ofstream file(a_file);
        file << "some content";
    }

    void SetUp() override
    {
        miral::TestServer::SetUp();
        ON_CALL(*this, load(testing::_, testing::_)).WillByDefault([this]{ notify_load(); });
    }
};

TestReloadingConfigFile::TestReloadingConfigFile()
{
    unlink(a_file);
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
    wait_for_load();
}

TEST_F(TestReloadingConfigFile, DISABLED_when_a_file_is_written_something_is_loaded)
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
    wait_for_load();
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

    for (auto i = 0; i != times; ++i)
    {
        wait_for_load();
        write_a_file();
    }

    wait_for_load();
}

// TODO - tests around the XDG Base Directory paths