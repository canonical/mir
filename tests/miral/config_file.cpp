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

#include <miral/test_server.h>
#include <miral/config_file.h>
#include <miral/live_config_overrides_list.h>

#include <wayland_wrapper.h>
#include <gmock/gmock.h>

#include <format>
#include <fstream>
#include <unistd.h>

using miral::ConfigFile;
using namespace testing;

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

    enum class FailOnTimeout { yes, no };
    void wait_for_load(FailOnTimeout fail_on_timeout = FailOnTimeout::no)
    {
        std::unique_lock lock{mutex};

        if (!cv.wait_for(lock, std::chrono::milliseconds{10}, [this] { return !pending_loads; }))
        {
            switch (fail_on_timeout)
            {
            case FailOnTimeout::yes:
                FAIL() << "wait_for_load() timed out" << std::endl;
                break;
            case FailOnTimeout::no:
                break;
            }
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

    std::mutex mutex;

private:
    std::condition_variable cv;
    bool pending_loads = false;
};

struct TestConfigFile : PendingLoad, miral::TestServer
{
    TestConfigFile();
    MOCK_METHOD(void, load, (std::istream& in, std::filesystem::path path), ());

    std::optional<ConfigFile> reloading_config_file;

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
        ON_CALL(*this, load(_, _)).WillByDefault([this]{ notify_load(); });
    }

    void TearDown() override
    {
        reloading_config_file.reset();
        miral::TestServer::TearDown();
    }
};

char const* const home = "/tmp/test_reloading_config_file/home";
char const* const home_config = "/tmp/test_reloading_config_file/home/.config";
char const* const xdg_conf_home = "/tmp/test_reloading_config_file/xdg_conf_dir_home";
char const* const xdg_conf_dir0 = "/tmp/test_reloading_config_file/xdg_conf_dir_zero";
char const* const xdg_conf_dir1 = "/tmp/test_reloading_config_file/xdg_conf_dir_one";
char const* const xdg_conf_dir2 = "/tmp/test_reloading_config_file/xdg_conf_dir_two";

TestConfigFile::TestConfigFile()
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

TEST_F(TestConfigFile, with_reload_on_change_and_no_file_nothing_is_loaded)
{
    EXPECT_CALL(*this, load).Times(0);

    invoke_runner([this](miral::MirRunner& runner)
    {
        reloading_config_file = ConfigFile{
            runner,
            no_such_file,
            ConfigFile::Mode::reload_on_change,
            [this](std::istream& in, std::filesystem::path path) { load(in, path); }};
    });
}

TEST_F(TestConfigFile, with_reload_on_change_and_a_file_something_is_loaded)
{
    EXPECT_CALL(*this, load).Times(1);

    write_a_file();

    invoke_runner([this](miral::MirRunner& runner)
    {
        reloading_config_file = ConfigFile{
            runner,
            a_file,
            ConfigFile::Mode::reload_on_change,
            [this](std::istream& in, std::filesystem::path path) { load(in, path); }};
    });
    wait_for_load();
}

TEST_F(TestConfigFile, with_reload_on_change_when_a_file_is_written_something_is_loaded)
{
    invoke_runner([this](miral::MirRunner& runner)
    {
        reloading_config_file = ConfigFile{
            runner,
            a_file,
            ConfigFile::Mode::reload_on_change,
            [this](std::istream& in, std::filesystem::path path) { load(in, path); }};
    });

    EXPECT_CALL(*this, load).Times(1);

    write_a_file();
    wait_for_load();
}

TEST_F(TestConfigFile, with_reload_on_change_each_time_a_file_is_rewritten_something_is_loaded)
{
    auto const times = 42;

    EXPECT_CALL(*this, load).Times(times+1);

    write_a_file(); // Initial write

    invoke_runner([this](miral::MirRunner& runner)
    {
        reloading_config_file = ConfigFile{
            runner,
            a_file,
            ConfigFile::Mode::reload_on_change,
            [this](std::istream& in, std::filesystem::path path) { load(in, path); }};
    });

    for (auto i = 0; i != times; ++i)
    {
        wait_for_load();
        write_a_file();
    }

    wait_for_load();
}

TEST_F(TestConfigFile, with_reload_on_change_when_config_home_unset_a_file_in_home_config_is_loaded)
{
    add_to_environment("XDG_CONFIG_HOME", nullptr);
    EXPECT_CALL(*this, load(_, home_config/config_file)).Times(1);

    write_config_in(xdg_conf_home);
    write_config_in(home_config);

    invoke_runner([this](miral::MirRunner& runner)
    {
        reloading_config_file = ConfigFile{
            runner,
            config_file,
            ConfigFile::Mode::reload_on_change,
            [this](std::istream& in, std::filesystem::path path) { load(in, path); }};
    });

    wait_for_load();
}

TEST_F(TestConfigFile, with_reload_on_change_a_file_in_xdg_config_home_is_loaded)
{
    EXPECT_CALL(*this, load(_, xdg_conf_home/config_file)).Times(1);

    write_config_in(xdg_conf_dir0);
    write_config_in(xdg_conf_home);
    write_config_in(home_config);

    invoke_runner([this](miral::MirRunner& runner)
    {
        reloading_config_file = ConfigFile{
            runner,
            config_file,
            ConfigFile::Mode::reload_on_change,
            [this](std::istream& in, std::filesystem::path path) { load(in, path); }};
    });

    wait_for_load();
}

TEST_F(TestConfigFile, with_reload_on_change_a_file_in_xdg_config_home_is_reloaded)
{
    EXPECT_CALL(*this, load(_, xdg_conf_home/config_file)).Times(2);

    write_config_in(xdg_conf_dir0);
    write_config_in(xdg_conf_home);
    write_config_in(home_config);

    invoke_runner([this](miral::MirRunner& runner)
    {
        reloading_config_file = ConfigFile{
            runner,
            config_file,
            ConfigFile::Mode::reload_on_change,
            [this](std::istream& in, std::filesystem::path path) { load(in, path); }};
    });

    wait_for_load();

    write_config_in(xdg_conf_dir0);
    write_config_in(xdg_conf_home);
    write_config_in(home_config);
    wait_for_load();
}

TEST_F(TestConfigFile, with_reload_on_change_a_config_in_xdg_conf_dir0_is_loaded)
{
    EXPECT_CALL(*this, load(_, xdg_conf_dir0/config_file)).Times(1);

    write_config_in(xdg_conf_dir0);

    invoke_runner([this](miral::MirRunner& runner)
    {
        reloading_config_file = ConfigFile{
            runner,
            config_file,
            ConfigFile::Mode::reload_on_change,
            [this](std::istream& in, std::filesystem::path path) { load(in, path); }};
    });

    wait_for_load();
}

TEST_F(TestConfigFile, with_reload_on_change_after_a_config_in_xdg_conf_dir0_is_loaded_a_new_config_in_xdg_conf_home_is_loaded)
{

    InSequence sequence;
    EXPECT_CALL(*this, load(_, xdg_conf_dir0/config_file)).Times(1);
    EXPECT_CALL(*this, load(_, xdg_conf_home/config_file)).Times(1);

    write_config_in(xdg_conf_dir2);
    write_config_in(xdg_conf_dir1);
    write_config_in(xdg_conf_dir0);

    invoke_runner([this](miral::MirRunner& runner)
    {
        reloading_config_file = ConfigFile{
            runner,
            config_file,
            ConfigFile::Mode::reload_on_change,
            [this](std::istream& in, std::filesystem::path path) { load(in, path); }};
    });

    wait_for_load();

    write_config_in(xdg_conf_home);
    wait_for_load();
}

TEST_F(TestConfigFile, with_reload_on_change_a_config_in_xdg_conf_dir0_is_loaded_in_preference_to_dir1_or_2)
{

    EXPECT_CALL(*this, load(_, xdg_conf_dir0/config_file)).Times(1);

    write_config_in(xdg_conf_dir2);
    write_config_in(xdg_conf_dir1);
    write_config_in(xdg_conf_dir0);

    invoke_runner([this](miral::MirRunner& runner)
    {
        reloading_config_file = ConfigFile{
            runner,
            config_file,
            ConfigFile::Mode::reload_on_change,
            [this](std::istream& in, std::filesystem::path path) { load(in, path); }};
    });

    wait_for_load();
}

TEST_F(TestConfigFile, with_reload_on_change_a_config_in_xdg_conf_dir1_is_loaded_in_preference_to_dir2)
{

    EXPECT_CALL(*this, load(_, xdg_conf_dir1/config_file)).Times(1);

    write_config_in(xdg_conf_dir2);
    write_config_in(xdg_conf_dir1);

    invoke_runner([this](miral::MirRunner& runner)
    {
        reloading_config_file = ConfigFile{
            runner,
            config_file,
            ConfigFile::Mode::reload_on_change,
            [this](std::istream& in, std::filesystem::path path) { load(in, path); }};
    });

    wait_for_load();
}

TEST_F(TestConfigFile, with_reload_on_change_a_config_in_xdg_conf_dir2_is_loaded)
{

    EXPECT_CALL(*this, load(_, xdg_conf_dir2/config_file)).Times(1);

    write_config_in(xdg_conf_dir2);

    invoke_runner([this](miral::MirRunner& runner)
    {
        reloading_config_file = ConfigFile{
            runner,
            config_file,
            ConfigFile::Mode::reload_on_change,
            [this](std::istream& in, std::filesystem::path path) { load(in, path); }};
    });

    wait_for_load();
}

TEST_F(TestConfigFile, with_no_reloading_and_no_file_nothing_is_loaded)
{
    EXPECT_CALL(*this, load).Times(0);

    invoke_runner([this](miral::MirRunner& runner)
    {
        reloading_config_file = ConfigFile{
            runner,
            no_such_file,
            ConfigFile::Mode::no_reloading,
            [this](std::istream& in, std::filesystem::path path) { load(in, path); }};
    });
}

TEST_F(TestConfigFile, with_no_reloading_and_a_file_something_is_loaded)
{
    EXPECT_CALL(*this, load).Times(1);

    write_a_file();

    invoke_runner([this](miral::MirRunner& runner)
    {
        reloading_config_file = ConfigFile{
            runner,
            a_file,
            ConfigFile::Mode::no_reloading,
            [this](std::istream& in, std::filesystem::path path) { load(in, path); }};
    });
    wait_for_load();
}

TEST_F(TestConfigFile, with_no_reloading_when_a_file_is_written_nothing_is_loaded)
{
    invoke_runner([this](miral::MirRunner& runner)
    {
        reloading_config_file = ConfigFile{
            runner,
            a_file,
            ConfigFile::Mode::no_reloading,
            [this](std::istream& in, std::filesystem::path path) { load(in, path); }};
    });

    EXPECT_CALL(*this, load).Times(0);

    write_a_file();
    wait_for_load();
}

TEST_F(TestConfigFile, with_no_reloading_when_a_file_is_rewritten_nothing_is_reloaded)
{
    EXPECT_CALL(*this, load).Times(1);

    write_a_file(); // Initial write

    invoke_runner([this](miral::MirRunner& runner)
    {
        reloading_config_file = ConfigFile{
            runner,
            a_file,
            ConfigFile::Mode::no_reloading,
            [this](std::istream& in, std::filesystem::path path) { load(in, path); }};
    });

    wait_for_load();
    write_a_file();
    wait_for_load();
}

TEST_F(TestConfigFile, with_no_reloading_when_config_home_unset_a_file_in_home_config_is_loaded)
{
    add_to_environment("XDG_CONFIG_HOME", nullptr);
    EXPECT_CALL(*this, load(_, home_config/config_file)).Times(1);

    write_config_in(xdg_conf_home);
    write_config_in(home_config);

    invoke_runner([this](miral::MirRunner& runner)
    {
        reloading_config_file = ConfigFile{
            runner,
            config_file,
            ConfigFile::Mode::no_reloading,
            [this](std::istream& in, std::filesystem::path path) { load(in, path); }};
    });

    wait_for_load();
}

TEST_F(TestConfigFile, with_no_reloading_a_file_in_xdg_config_home_is_loaded)
{
    EXPECT_CALL(*this, load(_, xdg_conf_home/config_file)).Times(1);

    write_config_in(xdg_conf_dir0);
    write_config_in(xdg_conf_home);
    write_config_in(home_config);

    invoke_runner([this](miral::MirRunner& runner)
    {
        reloading_config_file = ConfigFile{
            runner,
            config_file,
            ConfigFile::Mode::no_reloading,
            [this](std::istream& in, std::filesystem::path path) { load(in, path); }};
    });

    wait_for_load();
}

TEST_F(TestConfigFile, with_no_reloading_a_file_in_xdg_config_home_is_not_reloaded)
{
    EXPECT_CALL(*this, load(_, xdg_conf_home/config_file)).Times(1);

    write_config_in(xdg_conf_dir0);
    write_config_in(xdg_conf_home);
    write_config_in(home_config);

    invoke_runner([this](miral::MirRunner& runner)
    {
        reloading_config_file = ConfigFile{
            runner,
            config_file,
            ConfigFile::Mode::no_reloading,
            [this](std::istream& in, std::filesystem::path path) { load(in, path); }};
    });

    wait_for_load();

    write_config_in(xdg_conf_dir0);
    write_config_in(xdg_conf_home);
    write_config_in(home_config);
    wait_for_load();
}

TEST_F(TestConfigFile, with_no_reloading_a_config_in_xdg_conf_dir0_is_loaded)
{
    EXPECT_CALL(*this, load(_, xdg_conf_dir0/config_file)).Times(1);

    write_config_in(xdg_conf_dir0);

    invoke_runner([this](miral::MirRunner& runner)
    {
        reloading_config_file = ConfigFile{
            runner,
            config_file,
            ConfigFile::Mode::no_reloading,
            [this](std::istream& in, std::filesystem::path path) { load(in, path); }};
    });

    wait_for_load();
}

TEST_F(TestConfigFile, with_no_reloading_after_a_config_in_xdg_conf_dir0_is_loaded_a_new_config_in_xdg_conf_home_is_not_loaded)
{
    InSequence sequence;
    EXPECT_CALL(*this, load(_, xdg_conf_dir0/config_file)).Times(1);
    EXPECT_CALL(*this, load(_, xdg_conf_home/config_file)).Times(0);

    write_config_in(xdg_conf_dir2);
    write_config_in(xdg_conf_dir1);
    write_config_in(xdg_conf_dir0);

    invoke_runner([this](miral::MirRunner& runner)
    {
        reloading_config_file = ConfigFile{
            runner,
            config_file,
            ConfigFile::Mode::no_reloading,
            [this](std::istream& in, std::filesystem::path path) { load(in, path); }};
    });

    wait_for_load();

    write_config_in(xdg_conf_home);
    wait_for_load();
}

TEST_F(TestConfigFile, with_no_reloading_a_config_in_xdg_conf_dir0_is_loaded_in_preference_to_dir1_or_2)
{
    EXPECT_CALL(*this, load(_, xdg_conf_dir0/config_file)).Times(1);

    write_config_in(xdg_conf_dir2);
    write_config_in(xdg_conf_dir1);
    write_config_in(xdg_conf_dir0);

    invoke_runner([this](miral::MirRunner& runner)
    {
        reloading_config_file = ConfigFile{
            runner,
            config_file,
            ConfigFile::Mode::no_reloading,
            [this](std::istream& in, std::filesystem::path path) { load(in, path); }};
    });

    wait_for_load();
}

TEST_F(TestConfigFile, with_no_reloading_a_config_in_xdg_conf_dir1_is_loaded_in_preference_to_dir2)
{
    EXPECT_CALL(*this, load(_, xdg_conf_dir1/config_file)).Times(1);

    write_config_in(xdg_conf_dir2);
    write_config_in(xdg_conf_dir1);

    invoke_runner([this](miral::MirRunner& runner)
    {
        reloading_config_file = ConfigFile{
            runner,
            config_file,
            ConfigFile::Mode::no_reloading,
            [this](std::istream& in, std::filesystem::path path) { load(in, path); }};
    });

    wait_for_load();
}

TEST_F(TestConfigFile, with_no_reloading_a_config_in_xdg_conf_dir2_is_loaded)
{
    EXPECT_CALL(*this, load(_, xdg_conf_dir2/config_file)).Times(1);

    write_config_in(xdg_conf_dir2);

    invoke_runner([this](miral::MirRunner& runner)
    {
        reloading_config_file = ConfigFile{
            runner,
            config_file,
            ConfigFile::Mode::no_reloading,
            [this](std::istream& in, std::filesystem::path path) { load(in, path); }};
    });

    wait_for_load();
}


namespace
{
using namespace std::string_literals;

struct OverrideConfigTestBase : PendingLoad, miral::TestServer
{
    std::optional<ConfigFile> config;

    std::size_t last_load_stream_count{0};
    std::vector<std::filesystem::path> last_load_paths;
    std::vector<std::filesystem::path> last_fresh_paths;
    std::vector<std::filesystem::path> last_modified_paths;
    std::vector<std::filesystem::path> last_unchanged_paths;
    std::vector<std::filesystem::path> last_dropped_paths;
    int load_call_count{0};

    void record_load(miral::live_config::OverridesList const& overrides)
    {
        {
            std::lock_guard lock{PendingLoad::mutex};
            last_load_paths.clear();
            last_unchanged_paths.clear();
            last_fresh_paths.clear();
            last_modified_paths.clear();
            last_dropped_paths.clear();
            overrides.for_each(
                [&](auto const& p, auto&&) { last_unchanged_paths.push_back(p); last_load_paths.push_back(p); },
                [&](auto const& p, auto&&) { last_fresh_paths.push_back(p);     last_load_paths.push_back(p); },
                [&](auto const& p, auto&&) { last_modified_paths.push_back(p);  last_load_paths.push_back(p); },
                [&](auto const& p)         { last_dropped_paths.push_back(p); });
            last_load_stream_count = last_load_paths.size();
            ++load_call_count;
        }
        notify_load();
    }

    void SetUp() override
    {
        miral::TestServer::SetUp();
    }

    void TearDown() override
    {
        config.reset();
        miral::TestServer::TearDown();
    }
};

std::filesystem::path const override_config_file = "test_override.config";
auto const override_test_root = "/tmp/test_override_config_file/"s;
auto const override_xdg_conf_home = override_test_root + "xdg_conf_dir_home/"s;
auto const override_home = override_test_root + "home/"s;
auto const override_home_config = override_home + ".config/"s;

struct TestOverrideConfigFile : OverrideConfigTestBase
{
    TestOverrideConfigFile();

    auto override_dir() -> std::filesystem::path
    {
        return std::filesystem::path{override_xdg_conf_home} / (override_config_file.string() + ".d");
    }

    void write_base_config()
    {
        mark_pending();
        std::ofstream file{std::filesystem::path{override_xdg_conf_home} / override_config_file};
        file << "base content";
    }

    void write_override_file(std::string const& filename, std::string const& content = "override content")
    {
        mark_pending();
        std::filesystem::create_directories(override_dir());
        std::ofstream file{override_dir() / filename};
        file << content;
    }

    void write_real_file(std::filesystem::path const& filepath, std::string const& content = "real content")
    {
        mark_pending();
        std::filesystem::create_directories(filepath.parent_path());
        std::ofstream file{filepath};
        file << content;
    }

    void rewrite_base_config()
    {
        mark_pending();
        std::ofstream file{std::filesystem::path{override_xdg_conf_home} / override_config_file};
        file << "updated base content";
    }

    void delete_override_file(std::string const& filename)
    {
        mark_pending();
        std::filesystem::remove(override_dir() / filename);
    }

    void rename_override_file(std::string const& from, std::string const& to)
    {
        mark_pending();
        std::filesystem::rename(override_dir() / from, override_dir() / to);
    }

    void move_override_file_out(std::string const& filename)
    {
        mark_pending();
        std::filesystem::rename(override_dir() / filename, std::filesystem::path{override_test_root} / filename);
    }

    auto base_config_path() -> std::filesystem::path
    {
        return std::filesystem::path{override_xdg_conf_home} / override_config_file;
    }

    void delete_override_dir()
    {
        mark_pending();
        std::filesystem::remove_all(override_dir());
    }

    void make_config(miral::MirRunner& runner, ConfigFile::Mode mode, std::string extension = ".conf")
    {
        config = ConfigFile{
            runner,
            override_config_file,
            mode,
            [this](auto const& overrides) { record_load(overrides); },
            std::move(extension)};
    }

    void start_config(ConfigFile::Mode mode, std::string extension = ".conf")
    {
        invoke_runner(
            [this, mode, extension = std::move(extension)](miral::MirRunner& runner)
            {
                make_config(runner, mode, std::move(extension));
            });
    }

    void start_config(std::filesystem::path file, ConfigFile::Mode mode, std::string extension = ".conf")
    {
        invoke_runner(
            [this, file = std::move(file), mode, extension = std::move(extension)](miral::MirRunner& runner)
            {
                config = ConfigFile{
                    runner,
                    std::move(file),
                    mode,
                    [this](auto const& overrides) { record_load(overrides); },
                    std::move(extension)};
            });
    }
};

TestOverrideConfigFile::TestOverrideConfigFile()
{
    std::filesystem::remove_all(override_test_root);

    for (auto dir : {override_xdg_conf_home, override_home_config})
    {
        std::filesystem::create_directories(dir);
    }

    add_to_environment("HOME", override_home.c_str());
    add_to_environment("XDG_CONFIG_HOME", override_xdg_conf_home.c_str());
    add_to_environment("XDG_CONFIG_DIRS", "");
}
}

TEST_F(TestOverrideConfigFile, override_loader_with_no_file_is_not_called)
{
    start_config("no_such_file.config", ConfigFile::Mode::no_reloading);

    EXPECT_THAT(load_call_count, Eq(0));
}

TEST_F(TestOverrideConfigFile, override_loader_with_base_file_only_receives_one_stream)
{
    write_base_config();

    start_config(ConfigFile::Mode::no_reloading);

    wait_for_load();
    EXPECT_THAT(last_load_stream_count, Eq(1u));
}

TEST_F(TestOverrideConfigFile, override_loader_with_conf_files_receives_base_plus_overrides)
{
    write_base_config();
    write_override_file("10-first.conf");
    write_override_file("20-second.conf");

    start_config(ConfigFile::Mode::no_reloading);

    wait_for_load();
    EXPECT_THAT(last_load_stream_count, Eq(3u));
}

TEST_F(TestOverrideConfigFile, override_loader_ignores_non_conf_files)
{
    write_base_config();
    write_override_file("10-first.conf");
    write_override_file("README.txt");
    write_override_file("backup.conf.bak");

    start_config(ConfigFile::Mode::no_reloading);

    wait_for_load();
    EXPECT_THAT(last_load_stream_count, Eq(2u));
}

TEST_F(TestOverrideConfigFile, override_loader_sorts_override_files_lexicographically)
{
    write_base_config();
    write_override_file("30-third.conf");
    write_override_file("10-first.conf");
    write_override_file("20-second.conf");

    start_config(ConfigFile::Mode::no_reloading);

    wait_for_load();
    ASSERT_THAT(last_load_paths.size(), Eq(4u));
    // First path is the base config
    EXPECT_THAT(last_load_paths[0].filename(), Eq(override_config_file));
    // Overrides are sorted by filename
    EXPECT_THAT(last_load_paths[1].filename(), Eq("10-first.conf"));
    EXPECT_THAT(last_load_paths[2].filename(), Eq("20-second.conf"));
    EXPECT_THAT(last_load_paths[3].filename(), Eq("30-third.conf"));
}

TEST_F(TestOverrideConfigFile, reloads_when_base_config_is_rewritten)
{
    write_base_config();

    start_config(ConfigFile::Mode::reload_on_change);

    wait_for_load();
    EXPECT_THAT(load_call_count, Eq(1));

    rewrite_base_config();
    wait_for_load();
    EXPECT_THAT(load_call_count, Eq(2));
}

TEST_F(TestOverrideConfigFile, reloads_when_override_file_is_added)
{
    write_base_config();
    std::filesystem::create_directories(override_dir());

    start_config(ConfigFile::Mode::reload_on_change);

    wait_for_load();
    EXPECT_THAT(last_load_stream_count, Eq(1u));

    write_override_file("10-new.conf");
    wait_for_load();
    EXPECT_THAT(last_load_stream_count, Eq(2u));
}

TEST_F(TestOverrideConfigFile, reloads_when_override_file_is_modified)
{
    write_base_config();
    write_override_file("10-existing.conf", "original");

    start_config(ConfigFile::Mode::reload_on_change);

    wait_for_load();
    auto const count_after_initial = load_call_count;

    write_override_file("10-existing.conf", "modified");
    wait_for_load();
    EXPECT_THAT(load_call_count, Gt(count_after_initial));
}

TEST_F(TestOverrideConfigFile, reloads_when_override_dir_is_created_after_startup)
{
    write_base_config();
    // No override directory exists yet

    start_config(ConfigFile::Mode::reload_on_change);

    wait_for_load();
    EXPECT_THAT(last_load_stream_count, Eq(1u)); // base only

    // Now create the override directory with a file
    write_override_file("10-late.conf");
    wait_for_load();
    EXPECT_THAT(last_load_stream_count, Eq(2u)); // base + override
}

TEST_F(TestOverrideConfigFile, no_reloading_mode_does_not_reload_on_override_changes)
{
    write_base_config();
    write_override_file("10-first.conf");

    start_config(ConfigFile::Mode::no_reloading);

    wait_for_load();
    EXPECT_THAT(load_call_count, Eq(1));

    // Write more files — should NOT trigger a reload
    write_override_file("20-second.conf");
    wait_for_load();
    EXPECT_THAT(load_call_count, Eq(1));
}

TEST_F(TestOverrideConfigFile, no_reloading_mode_does_not_reload_on_base_config_change)
{
    write_base_config();

    start_config(ConfigFile::Mode::no_reloading);

    wait_for_load();
    EXPECT_THAT(load_call_count, Eq(1));

    rewrite_base_config();
    wait_for_load();
    EXPECT_THAT(load_call_count, Eq(1));
}

TEST_F(TestOverrideConfigFile, reloads_when_override_file_is_deleted)
{
    write_base_config();
    write_override_file("10-first.conf");
    write_override_file("20-second.conf");

    start_config(ConfigFile::Mode::reload_on_change);

    wait_for_load();
    EXPECT_THAT(last_load_stream_count, Eq(3u)); // base + 2 overrides

    delete_override_file("10-first.conf");
    wait_for_load(FailOnTimeout::yes);
    EXPECT_THAT(last_load_stream_count, Eq(2u)); // base + 1 remaining override
}

TEST_F(TestOverrideConfigFile, reloads_when_override_file_is_moved_out_of_directory)
{
    write_base_config();
    write_override_file("10-first.conf");

    start_config(ConfigFile::Mode::reload_on_change);

    wait_for_load();
    EXPECT_THAT(last_load_stream_count, Eq(2u)); // base + 1 override

    move_override_file_out("10-first.conf");
    wait_for_load(FailOnTimeout::yes);
    EXPECT_THAT(last_load_stream_count, Eq(1u)); // base only
}

TEST_F(TestOverrideConfigFile, reloads_when_override_file_is_renamed_to_non_conf)
{
    write_base_config();
    write_override_file("10-first.conf");

    start_config(ConfigFile::Mode::reload_on_change);

    wait_for_load();
    EXPECT_THAT(last_load_stream_count, Eq(2u)); // base + 1 override

    // Renaming a .conf file away should trigger a reload without it
    rename_override_file("10-first.conf", "10-first.conf.bak");
    wait_for_load(FailOnTimeout::yes);
    EXPECT_THAT(last_load_stream_count, Eq(1u)); // base only
}

TEST_F(TestOverrideConfigFile, reloads_when_override_dir_is_deleted)
{
    write_base_config();
    write_override_file("10-first.conf");

    start_config(ConfigFile::Mode::reload_on_change);

    wait_for_load();
    EXPECT_THAT(last_load_stream_count, Eq(2u)); // base + 1 override

    delete_override_dir();
    wait_for_load(FailOnTimeout::yes);
    EXPECT_THAT(last_load_stream_count, Eq(1u)); // base only
}

TEST_F(TestOverrideConfigFile, reloads_when_file_is_renamed_to_conf_in_override_directory)
{
    write_base_config();
    write_override_file("10-first.conf.tmp");

    start_config(ConfigFile::Mode::reload_on_change);

    wait_for_load();
    EXPECT_THAT(last_load_stream_count, Eq(1u)); // base only, .tmp is ignored

    // Atomically renaming a .tmp to .conf should trigger a reload including it
    rename_override_file("10-first.conf.tmp", "10-first.conf");
    wait_for_load();
    EXPECT_THAT(last_load_stream_count, Eq(2u)); // base + new override
}

TEST_F(TestOverrideConfigFile, missing_base_config_does_not_call_loader)
{
    // No base config file at all
    start_config(ConfigFile::Mode::no_reloading);

    EXPECT_THAT(load_call_count, Eq(0));
}

TEST_F(TestOverrideConfigFile, custom_extension_loads_matching_files)
{
    write_base_config();
    write_override_file("10-first.yaml");
    write_override_file("20-second.yaml");

    start_config(ConfigFile::Mode::no_reloading, ".yaml");

    wait_for_load();
    EXPECT_THAT(last_load_stream_count, Eq(3u)); // base + 2 .yaml overrides
}

TEST_F(TestOverrideConfigFile, custom_extension_ignores_conf_files)
{
    write_base_config();
    write_override_file("10-first.conf");  // should be ignored
    write_override_file("20-second.yaml"); // should be picked up

    start_config(ConfigFile::Mode::no_reloading, ".yaml");

    wait_for_load();
    EXPECT_THAT(last_load_stream_count, Eq(2u)); // base + 1 .yaml only
    EXPECT_THAT(last_fresh_paths.back().filename(), Eq("20-second.yaml"));
}

TEST_F(TestOverrideConfigFile, custom_extension_reloads_when_matching_file_is_added)
{
    write_base_config();
    std::filesystem::create_directories(override_dir());

    start_config(ConfigFile::Mode::reload_on_change, ".yaml");

    wait_for_load();
    EXPECT_THAT(last_load_stream_count, Eq(1u)); // base only

    write_override_file("10-new.yaml");
    wait_for_load();
    EXPECT_THAT(last_load_stream_count, Eq(2u)); // base + new .yaml
}

TEST_F(TestOverrideConfigFile, reloads_when_override_dir_is_moved_into_place_after_startup)
{
    write_base_config();
    // No override directory exists yet

    start_config(ConfigFile::Mode::reload_on_change);

    wait_for_load();
    EXPECT_THAT(last_load_stream_count, Eq(1u)); // base only

    // Build the override directory at a temporary location outside the watched path
    auto const temp_dir = std::filesystem::path{override_test_root} / "temp_override_dir";
    std::filesystem::create_directories(temp_dir);
    {
        std::ofstream file{temp_dir / "10-moved.conf"};
        file << "moved content";
    }

    // Atomically move the temp dir into place; this generates IN_MOVED_TO, not IN_CREATE
    mark_pending();
    std::filesystem::rename(temp_dir, override_dir());
    wait_for_load(FailOnTimeout::yes);

    EXPECT_THAT(last_load_stream_count, Eq(2u)); // base + moved override
    EXPECT_THAT(last_fresh_paths, ElementsAre(override_dir() / "10-moved.conf"));
    EXPECT_THAT(last_unchanged_paths, ElementsAre(base_config_path()));
}

TEST_F(TestOverrideConfigFile, no_reload_when_empty_dir_is_moved_into_place)
{
    write_base_config();

    start_config(ConfigFile::Mode::reload_on_change);

    wait_for_load();
    auto const count_after_initial = load_call_count;

    auto const temp_dir = std::filesystem::path{override_test_root} / "empty_override_dir";
    std::filesystem::create_directories(temp_dir);
    mark_pending();
    std::filesystem::rename(temp_dir, override_dir());
    wait_for_load(FailOnTimeout::no);

    EXPECT_THAT(load_call_count, Eq(count_after_initial));
}

TEST_F(TestOverrideConfigFile, override_loader_ignores_regular_file_at_override_directory_path)
{
    write_base_config();
    // Place a regular file where the override directory would normally live
    {
        std::ofstream file{override_dir()};
        file << "not a directory";
    }

    start_config(ConfigFile::Mode::no_reloading);

    wait_for_load();
    EXPECT_THAT(last_load_stream_count, Eq(1u)); // base only; file masquerading as dir is ignored
}

TEST_F(TestOverrideConfigFile, custom_extension_does_not_reload_when_conf_file_is_added)
{
    write_base_config();
    std::filesystem::create_directories(override_dir());

    start_config(ConfigFile::Mode::reload_on_change, ".yaml");

    wait_for_load();
    auto const count_after_initial = load_call_count;

    write_override_file("10-ignored.conf");
    wait_for_load(FailOnTimeout::no);
    EXPECT_THAT(load_call_count, Eq(count_after_initial));
}

TEST_F(TestOverrideConfigFile, override_dir_moved_into_place_with_file_triggers_exactly_one_reload)
{
    write_base_config();

    start_config(ConfigFile::Mode::reload_on_change);

    wait_for_load();
    auto const count_after_initial = load_call_count;

    // Build the override directory at a temporary location outside the watched path
    auto const temp_dir = std::filesystem::path{override_test_root} / "temp_dir_single_reload";
    std::filesystem::create_directories(temp_dir);
    {
        std::ofstream file{temp_dir / "10-moved.conf"};
        file << "moved content";
    }

    // Atomically rename into place — generates IN_MOVED_TO for the directory, not
    // individual IN_CLOSE_WRITE events for the files inside, so there is no second
    // event that could trigger a spurious reload.
    mark_pending();
    std::filesystem::rename(temp_dir, override_dir());
    wait_for_load(FailOnTimeout::yes);

    EXPECT_THAT(load_call_count, Eq(count_after_initial + 1));
    EXPECT_THAT(last_fresh_paths, ElementsAre(override_dir() / "10-moved.conf"));
    EXPECT_THAT(last_unchanged_paths, ElementsAre(base_config_path()));
}

TEST_F(TestOverrideConfigFile, unreadable_override_directory_is_treated_as_empty_during_initial_load)
{
    if (geteuid() == 0)
        GTEST_SKIP() << "Cannot test permission errors as root";

    write_base_config();
    std::filesystem::create_directories(override_dir());
    {
        std::ofstream f{override_dir() / "10-blocked.conf"};
        f << "content";
    }

    std::filesystem::permissions(override_dir(), std::filesystem::perms::none);

    // RAII guard: restore permissions so the directory can be cleaned up between tests
    struct PermissionRestorer
    {
        std::filesystem::path const dir;
        ~PermissionRestorer() noexcept
        {
            std::error_code ec;
            std::filesystem::permissions(dir, std::filesystem::perms::all, ec);
        }
    } const restorer{override_dir()};

    // ConfigFile construction must not throw; an unreadable override directory should
    // be treated as having no overrides rather than propagating filesystem_error
    EXPECT_NO_THROW(start_config(ConfigFile::Mode::no_reloading));

    wait_for_load(FailOnTimeout::no);
    // Only the base config should have been loaded; the unreadable directory contributes nothing
    EXPECT_THAT(last_load_stream_count, Eq(1u));
}

namespace
{
using namespace std::string_literals;

auto const multi_root_test_root = "/tmp/test_multi_root_config_file/"s;
auto const multi_root_user_dir = multi_root_test_root + "user/"s;
auto const multi_root_system_dir = multi_root_test_root + "system/"s;
std::filesystem::path const multi_root_config_file = "test_multi_root.config";

struct TestMultiRootConfigFile : OverrideConfigTestBase
{
    TestMultiRootConfigFile();

    auto user_override_dir() -> std::filesystem::path
    {
        return std::filesystem::path{multi_root_user_dir} / (multi_root_config_file.string() + ".d");
    }

    auto system_override_dir() -> std::filesystem::path
    {
        return std::filesystem::path{multi_root_system_dir} / (multi_root_config_file.string() + ".d");
    }

    auto base_config_path() -> std::filesystem::path
    {
        return std::filesystem::path{multi_root_user_dir} / multi_root_config_file;
    }

    auto system_base_config_path() -> std::filesystem::path
    {
        return std::filesystem::path{multi_root_system_dir} / multi_root_config_file;
    }

    void write_base_config()
    {
        mark_pending();
        std::ofstream file{base_config_path()};
        file << "base content";
    }

    void write_system_base_config(std::string const& content = "system base content")
    {
        mark_pending();
        std::ofstream file{system_base_config_path()};
        file << content;
    }

    void write_user_override(std::string const& filename, std::string const& content = "user-content")
    {
        mark_pending();
        std::filesystem::create_directories(user_override_dir());
        std::ofstream file{user_override_dir() / filename};
        file << content;
    }

    void write_system_override(std::string const& filename, std::string const& content = "system-content")
    {
        mark_pending();
        std::filesystem::create_directories(system_override_dir());
        std::ofstream file{system_override_dir() / filename};
        file << content;
    }

    void make_config(miral::MirRunner& runner, ConfigFile::Mode mode)
    {
        config = ConfigFile{
            runner,
            multi_root_config_file,
            mode,
            [this](auto const& overrides) { record_load(overrides); },
            ".conf"};
    }

    void start_config(ConfigFile::Mode mode)
    {
        invoke_runner([this, mode](miral::MirRunner& runner) { make_config(runner, mode); });
    }
};

TestMultiRootConfigFile::TestMultiRootConfigFile()
{
    std::filesystem::remove_all(multi_root_test_root);

    for (auto const& dir : {multi_root_user_dir, multi_root_system_dir})
        std::filesystem::create_directories(dir);

    add_to_environment("XDG_CONFIG_HOME", multi_root_user_dir.c_str());
    add_to_environment("XDG_CONFIG_DIRS", multi_root_system_dir.c_str());
}
}

TEST_F(TestMultiRootConfigFile, initial_load_includes_system_override_when_base_config_is_in_user_dir)
{
    write_system_override("10-system.conf");
    write_base_config();

    start_config(ConfigFile::Mode::no_reloading);

    wait_for_load();
    EXPECT_THAT(last_load_stream_count, Eq(2u));
    EXPECT_THAT(last_load_paths, ElementsAre(base_config_path(), system_override_dir() / "10-system.conf"));
}

TEST_F(TestMultiRootConfigFile, initial_load_includes_overrides_from_all_roots)
{
    write_system_override("10-system.conf");
    write_user_override("20-user.conf");
    write_base_config();

    start_config(ConfigFile::Mode::no_reloading);

    wait_for_load();
    EXPECT_THAT(
        last_load_paths,
        ElementsAre(
            base_config_path(), system_override_dir() / "10-system.conf", user_override_dir() / "20-user.conf"));
}

TEST_F(TestMultiRootConfigFile, reload_includes_same_files_as_initial_load)
{
    write_system_override("10-system.conf");
    write_user_override("20-user.conf");
    write_base_config();

    start_config(ConfigFile::Mode::reload_on_change);

    wait_for_load();

    // Trigger a reload by rewriting the base config
    mark_pending();
    {
        std::ofstream file{base_config_path()};
        file << "updated base content";
    }
    wait_for_load();

    EXPECT_THAT(
        last_load_paths,
        ElementsAre(
            base_config_path(), system_override_dir() / "10-system.conf", user_override_dir() / "20-user.conf"));
}

TEST_F(TestMultiRootConfigFile, initial_load_includes_user_overrides_when_base_config_is_in_system_dir)
{
    write_system_base_config();
    write_user_override("20-user.conf");

    start_config(ConfigFile::Mode::no_reloading);

    wait_for_load();
    EXPECT_THAT(
        last_load_paths,
        ElementsAre(system_base_config_path(), user_override_dir() / "20-user.conf"));
}

TEST_F(TestMultiRootConfigFile, initial_load_includes_overrides_from_both_roots_when_base_config_is_in_system_dir)
{
    write_system_base_config();
    write_system_override("10-system.conf");
    write_user_override("20-user.conf");

    start_config(ConfigFile::Mode::no_reloading);

    wait_for_load();
    EXPECT_THAT(
        last_load_paths,
        ElementsAre(
            system_base_config_path(),
            system_override_dir() / "10-system.conf",
            user_override_dir() / "20-user.conf"));
}

TEST_F(TestMultiRootConfigFile, reload_matches_initial_load_when_base_config_is_in_system_dir)
{
    write_system_base_config();
    write_system_override("10-system.conf");
    write_user_override("20-user.conf");

    start_config(ConfigFile::Mode::reload_on_change);

    wait_for_load();
    auto const initial_paths = last_load_paths;

    // Trigger a reload by rewriting the system base config
    write_system_base_config("updated system base");
    wait_for_load();

    EXPECT_THAT(last_load_paths, Eq(initial_paths));
}
