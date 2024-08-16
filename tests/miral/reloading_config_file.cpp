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

#include <format>
#include <fstream>

using miral::ReloadingConfigFile;

namespace
{
char const* const no_such_file = "no/such/file";
char const* const a_file = "/tmp/test_reloading_config_file/a_file";
std::filesystem::path const config_file = "test_reloading_config_file.config";

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

    void write_config_in(std::filesystem::path path)
    {
        mark_pending();
        std::ofstream file(path/config_file);
        file << "some content";
    }

    void SetUp() override
    {
        miral::TestServer::SetUp();
        ON_CALL(*this, load(testing::_, testing::_)).WillByDefault([this]{ notify_load(); });
    }
};

char const* const home = "/tmp/test_reloading_config_file/home";
char const* const home_config = "/tmp/test_reloading_config_file/home/.config";
char const* const xdg_conf_home = "/tmp/test_reloading_config_file/xdg_conf_dir_home";
char const* const xdg_conf_dir0 = "/tmp/test_reloading_config_file/xdg_conf_dir_zero";
char const* const xdg_conf_dir1 = "/tmp/test_reloading_config_file/xdg_conf_dir_one";
char const* const xdg_conf_dir2 = "/tmp/test_reloading_config_file/xdg_conf_dir_two";

TestReloadingConfigFile::TestReloadingConfigFile()
{
    std::filesystem::remove_all("/tmp/test_reloading_config_file/");

    for (auto dir : {home_config, xdg_conf_home, xdg_conf_dir0, xdg_conf_dir1, xdg_conf_dir2})
    {
        std::filesystem::create_directories(dir);
    }

    add_to_environment("HOME", home);
    add_to_environment("XDG_CONFIG_HOME", xdg_conf_home);
    add_to_environment("XDG_CONFIG_DIRS", std::format("{}:{}:{}", xdg_conf_dir0, xdg_conf_dir1, xdg_conf_dir2).c_str());
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

TEST_F(TestReloadingConfigFile, when_config_home_unset_a_file_in_home_config_is_loaded)
{
    add_to_environment("XDG_CONFIG_HOME", nullptr);
    using testing::_;
    EXPECT_CALL(*this, load(_, home_config/config_file)).Times(1);

    write_config_in(xdg_conf_home);
    write_config_in(home_config);

    invoke_runner([this](miral::MirRunner& runner)
    {
        reloading_config_file = ReloadingConfigFile{
            runner,
            config_file,
            [this](std::istream& in, std::filesystem::path path) { load(in, path); }};
    });

    wait_for_load();
}

TEST_F(TestReloadingConfigFile, a_file_in_xdg_config_home_is_loaded)
{
    using testing::_;
    EXPECT_CALL(*this, load(_, xdg_conf_home/config_file)).Times(1);

    write_config_in(xdg_conf_dir0);
    write_config_in(xdg_conf_home);
    write_config_in(home_config);

    invoke_runner([this](miral::MirRunner& runner)
    {
        reloading_config_file = ReloadingConfigFile{
            runner,
            config_file,
            [this](std::istream& in, std::filesystem::path path) { load(in, path); }};
    });

    wait_for_load();
}

TEST_F(TestReloadingConfigFile, a_file_in_xdg_config_home_is_reloaded)
{
    using testing::_;
    EXPECT_CALL(*this, load(_, xdg_conf_home/config_file)).Times(2);

    write_config_in(xdg_conf_dir0);
    write_config_in(xdg_conf_home);
    write_config_in(home_config);

    invoke_runner([this](miral::MirRunner& runner)
    {
        reloading_config_file = ReloadingConfigFile{
            runner,
            config_file,
            [this](std::istream& in, std::filesystem::path path) { load(in, path); }};
    });

    wait_for_load();

    write_config_in(xdg_conf_dir0);
    write_config_in(xdg_conf_home);
    write_config_in(home_config);
    wait_for_load();
}

TEST_F(TestReloadingConfigFile, a_config_in_xdg_conf_dir0_is_loaded)
{
    using testing::_;
    EXPECT_CALL(*this, load(_, xdg_conf_dir0/config_file)).Times(1);

    write_config_in(xdg_conf_dir0);

    invoke_runner([this](miral::MirRunner& runner)
    {
        reloading_config_file = ReloadingConfigFile{
            runner,
            config_file,
            [this](std::istream& in, std::filesystem::path path) { load(in, path); }};
    });

    wait_for_load();
}

TEST_F(TestReloadingConfigFile, after_a_config_in_xdg_conf_dir0_is_loaded_a_new_config_in_xdg_conf_home_is_loaded)
{
    using testing::_;

    testing::InSequence sequence;
    EXPECT_CALL(*this, load(_, xdg_conf_dir0/config_file)).Times(1);
    EXPECT_CALL(*this, load(_, xdg_conf_home/config_file)).Times(1);

    write_config_in(xdg_conf_dir2);
    write_config_in(xdg_conf_dir1);
    write_config_in(xdg_conf_dir0);

    invoke_runner([this](miral::MirRunner& runner)
    {
        reloading_config_file = ReloadingConfigFile{
            runner,
            config_file,
            [this](std::istream& in, std::filesystem::path path) { load(in, path); }};
    });

    wait_for_load();

    write_config_in(xdg_conf_home);
    wait_for_load();
}

TEST_F(TestReloadingConfigFile, a_config_in_xdg_conf_dir0_is_loaded_in_preference_to_dir1_or_2)
{
    using testing::_;

    EXPECT_CALL(*this, load(_, xdg_conf_dir0/config_file)).Times(1);

    write_config_in(xdg_conf_dir2);
    write_config_in(xdg_conf_dir1);
    write_config_in(xdg_conf_dir0);

    invoke_runner([this](miral::MirRunner& runner)
    {
        reloading_config_file = ReloadingConfigFile{
            runner,
            config_file,
            [this](std::istream& in, std::filesystem::path path) { load(in, path); }};
    });

    wait_for_load();
}

TEST_F(TestReloadingConfigFile, a_config_in_xdg_conf_dir1_is_loaded_in_preference_to_dir2)
{
    using testing::_;

    EXPECT_CALL(*this, load(_, xdg_conf_dir1/config_file)).Times(1);

    write_config_in(xdg_conf_dir2);
    write_config_in(xdg_conf_dir1);

    invoke_runner([this](miral::MirRunner& runner)
    {
        reloading_config_file = ReloadingConfigFile{
            runner,
            config_file,
            [this](std::istream& in, std::filesystem::path path) { load(in, path); }};
    });

    wait_for_load();
}

TEST_F(TestReloadingConfigFile, a_config_in_xdg_conf_dir2_is_loaded)
{
    using testing::_;

    EXPECT_CALL(*this, load(_, xdg_conf_dir2/config_file)).Times(1);

    write_config_in(xdg_conf_dir2);

    invoke_runner([this](miral::MirRunner& runner)
    {
        reloading_config_file = ReloadingConfigFile{
            runner,
            config_file,
            [this](std::istream& in, std::filesystem::path path) { load(in, path); }};
    });

    wait_for_load();
}
