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

#include <source_location>
#include <wayland_wrapper.h>
#include <gmock/gmock-function-mocker.h>

#include <format>
#include <fstream>

using miral::ConfigFile;

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
                std::cerr << "wait_for_load() timed out" << std::endl;
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

private:
    std::mutex mutex;
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
        ON_CALL(*this, load(testing::_, testing::_)).WillByDefault([this]{ notify_load(); });
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
    using testing::_;
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
    using testing::_;
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
    using testing::_;
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
    using testing::_;
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
    using testing::_;

    testing::InSequence sequence;
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
    using testing::_;

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
    using testing::_;

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
    using testing::_;

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
    using testing::_;
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
    using testing::_;
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
    using testing::_;
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
    using testing::_;
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
    using testing::_;

    testing::InSequence sequence;
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
    using testing::_;

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
    using testing::_;

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
    using testing::_;

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
// The override config file name must match the pattern: <name>.d/*.conf
// For a config file named "test_override.config", the override directory is "test_override.config.d/"
std::filesystem::path const override_config_file = "test_override.config";
auto const override_test_root = "/tmp/test_override_config_file/"s;
auto const override_xdg_conf_home = override_test_root + "xdg_conf_dir_home/"s;
auto const override_home = override_test_root + "home/"s;
auto const override_home_config = override_home + ".config/"s;
// A directory to hold real files that symlinks can point to
auto const symlink_targets_dir = override_test_root + "symlink_targets/"s;

struct TestOverrideConfigFile : PendingLoad, miral::TestServer
{
    TestOverrideConfigFile();

    std::optional<ConfigFile> config;

    // Track calls to the OverrideLoader
    std::mutex load_mutex;
    std::size_t last_load_stream_count{0};
    std::vector<std::filesystem::path> last_load_paths;
    int load_call_count{0};

    void record_load(std::span<std::pair<std::unique_ptr<std::istream>, std::filesystem::path>> streams)
    {
        {
            std::lock_guard lock{load_mutex};
            last_load_stream_count = streams.size();
            last_load_paths.clear();
            for (auto const& [_, p] : streams)
                last_load_paths.push_back(p);
            ++load_call_count;
        }
        notify_load();
    }

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

    void create_override_dir_symlink(std::filesystem::path const& target)
    {
        mark_pending();
        auto const link_path = override_dir();
        std::filesystem::remove_all(link_path);
        std::filesystem::create_symlink(target, link_path);
    }

    void create_file_symlink(std::filesystem::path const& target, std::string const& link_name)
    {
        mark_pending();
        std::filesystem::create_directories(override_dir());
        auto const link_path = override_dir() / link_name;
        std::filesystem::remove(link_path);
        std::filesystem::create_symlink(target, link_path);
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
        std::filesystem::rename(override_dir() / filename, std::filesystem::path{symlink_targets_dir} / filename);
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

TestOverrideConfigFile::TestOverrideConfigFile()
{
    std::filesystem::remove_all(override_test_root);

    for (auto dir : {override_xdg_conf_home, override_home_config, symlink_targets_dir})
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
    invoke_runner([this](miral::MirRunner& runner)
    {
        config = ConfigFile{
            runner,
            "no_such_file.config",
            ConfigFile::Mode::no_reloading,
            [this](auto streams) { record_load(streams); }};
    });

    EXPECT_THAT(load_call_count, testing::Eq(0));
}

TEST_F(TestOverrideConfigFile, override_loader_with_base_file_only_receives_one_stream)
{
    write_base_config();

    invoke_runner([this](miral::MirRunner& runner)
    {
        config = ConfigFile{
            runner,
            override_config_file,
            ConfigFile::Mode::no_reloading,
            [this](auto streams) { record_load(streams); }};
    });

    wait_for_load();
    EXPECT_THAT(last_load_stream_count, testing::Eq(1u));
}

TEST_F(TestOverrideConfigFile, override_loader_with_conf_files_receives_base_plus_overrides)
{
    write_base_config();
    write_override_file("10-first.conf");
    write_override_file("20-second.conf");

    invoke_runner([this](miral::MirRunner& runner)
    {
        config = ConfigFile{
            runner,
            override_config_file,
            ConfigFile::Mode::no_reloading,
            [this](auto streams) { record_load(streams); }};
    });

    wait_for_load();
    EXPECT_THAT(last_load_stream_count, testing::Eq(3u));
}

TEST_F(TestOverrideConfigFile, override_loader_ignores_non_conf_files)
{
    write_base_config();
    write_override_file("10-first.conf");
    write_override_file("README.txt");
    write_override_file("backup.conf.bak");

    invoke_runner([this](miral::MirRunner& runner)
    {
        config = ConfigFile{
            runner,
            override_config_file,
            ConfigFile::Mode::no_reloading,
            [this](auto streams) { record_load(streams); }};
    });

    wait_for_load();
    EXPECT_THAT(last_load_stream_count, testing::Eq(2u));
}

TEST_F(TestOverrideConfigFile, override_loader_sorts_override_files_lexicographically)
{
    write_base_config();
    write_override_file("30-third.conf");
    write_override_file("10-first.conf");
    write_override_file("20-second.conf");

    invoke_runner([this](miral::MirRunner& runner)
    {
        config = ConfigFile{
            runner,
            override_config_file,
            ConfigFile::Mode::no_reloading,
            [this](auto streams) { record_load(streams); }};
    });

    wait_for_load();
    ASSERT_THAT(last_load_paths.size(), testing::Eq(4u));
    // First path is the base config
    EXPECT_THAT(last_load_paths[0].filename(), testing::Eq(override_config_file));
    // Overrides are sorted by filename
    EXPECT_THAT(last_load_paths[1].filename(), testing::Eq("10-first.conf"));
    EXPECT_THAT(last_load_paths[2].filename(), testing::Eq("20-second.conf"));
    EXPECT_THAT(last_load_paths[3].filename(), testing::Eq("30-third.conf"));
}

TEST_F(TestOverrideConfigFile, override_dir_symlink_is_followed_on_initial_load)
{
    write_base_config();

    // Create a real directory with override files
    auto const real_dir = std::filesystem::path{symlink_targets_dir} / "real_overrides";
    std::filesystem::create_directories(real_dir);
    { std::ofstream{real_dir / "10-override.conf"} << "override content"; }

    // Symlink the override directory to the real directory
    create_override_dir_symlink(real_dir);

    invoke_runner([this](miral::MirRunner& runner)
    {
        config = ConfigFile{
            runner,
            override_config_file,
            ConfigFile::Mode::no_reloading,
            [this](auto streams) { record_load(streams); }};
    });

    wait_for_load();
    EXPECT_THAT(last_load_stream_count, testing::Eq(2u));
}

TEST_F(TestOverrideConfigFile, symlinked_override_files_are_followed_on_initial_load)
{
    write_base_config();
    std::filesystem::create_directories(override_dir());

    // Create a real file outside the override directory
    auto const real_file = std::filesystem::path{symlink_targets_dir} / "real_override.conf";
    { std::ofstream{real_file} << "real override content"; }

    // Symlink it into the override directory
    create_file_symlink(real_file, "10-override.conf");

    invoke_runner([this](miral::MirRunner& runner)
    {
        config = ConfigFile{
            runner,
            override_config_file,
            ConfigFile::Mode::no_reloading,
            [this](auto streams) { record_load(streams); }};
    });

    wait_for_load();
    EXPECT_THAT(last_load_stream_count, testing::Eq(2u));
}

TEST_F(TestOverrideConfigFile, reloads_when_base_config_is_rewritten)
{
    write_base_config();

    invoke_runner([this](miral::MirRunner& runner)
    {
        config = ConfigFile{
            runner,
            override_config_file,
            ConfigFile::Mode::reload_on_change,
            [this](auto streams) { record_load(streams); }};
    });

    wait_for_load();
    EXPECT_THAT(load_call_count, testing::Eq(1));

    rewrite_base_config();
    wait_for_load();
    EXPECT_THAT(load_call_count, testing::Eq(2));
}

TEST_F(TestOverrideConfigFile, reloads_when_override_file_is_added)
{
    write_base_config();
    std::filesystem::create_directories(override_dir());

    invoke_runner([this](miral::MirRunner& runner)
    {
        config = ConfigFile{
            runner,
            override_config_file,
            ConfigFile::Mode::reload_on_change,
            [this](auto streams) { record_load(streams); }};
    });

    wait_for_load();
    EXPECT_THAT(last_load_stream_count, testing::Eq(1u));

    write_override_file("10-new.conf");
    wait_for_load();
    EXPECT_THAT(last_load_stream_count, testing::Eq(2u));
}

TEST_F(TestOverrideConfigFile, reloads_when_override_file_is_modified)
{
    write_base_config();
    write_override_file("10-existing.conf", "original");

    invoke_runner([this](miral::MirRunner& runner)
    {
        config = ConfigFile{
            runner,
            override_config_file,
            ConfigFile::Mode::reload_on_change,
            [this](auto streams) { record_load(streams); }};
    });

    wait_for_load();
    auto const count_after_initial = load_call_count;

    write_override_file("10-existing.conf", "modified");
    wait_for_load();
    EXPECT_THAT(load_call_count, testing::Gt(count_after_initial));
}

TEST_F(TestOverrideConfigFile, reloads_when_override_dir_symlink_target_changes)
{
    write_base_config();

    auto const real_dir_a = std::filesystem::path{symlink_targets_dir} / "overrides_a";
    auto const real_dir_b = std::filesystem::path{symlink_targets_dir} / "overrides_b";
    std::filesystem::create_directories(real_dir_a);
    std::filesystem::create_directories(real_dir_b);
    { std::ofstream{real_dir_a / "10-a.conf"} << "a content"; }
    { std::ofstream{real_dir_b / "10-b.conf"} << "b content"; }
    { std::ofstream{real_dir_b / "20-b.conf"} << "b2 content"; }

    create_override_dir_symlink(real_dir_a);

    invoke_runner([this](miral::MirRunner& runner)
    {
        config = ConfigFile{
            runner,
            override_config_file,
            ConfigFile::Mode::reload_on_change,
            [this](auto streams) { record_load(streams); }};
    });

    wait_for_load();
    EXPECT_THAT(last_load_stream_count, testing::Eq(2u)); // base + 1 from dir_a

    // Re-point the symlink to a different directory
    create_override_dir_symlink(real_dir_b);
    wait_for_load();
    EXPECT_THAT(last_load_stream_count, testing::Eq(3u)); // base + 2 from dir_b
}

TEST_F(TestOverrideConfigFile, reloads_when_file_in_symlinked_override_dir_changes)
{
    write_base_config();

    auto const real_dir = std::filesystem::path{symlink_targets_dir} / "overrides_watched";
    std::filesystem::create_directories(real_dir);
    { std::ofstream{real_dir / "10-override.conf"} << "original"; }

    create_override_dir_symlink(real_dir);

    invoke_runner([this](miral::MirRunner& runner)
    {
        config = ConfigFile{
            runner,
            override_config_file,
            ConfigFile::Mode::reload_on_change,
            [this](auto streams) { record_load(streams); }};
    });

    wait_for_load();
    auto const count_after_initial = load_call_count;

    // Modify a file inside the real directory that the symlink points to
    {
        mark_pending();
        std::ofstream{real_dir / "10-override.conf"} << "modified";
    }
    wait_for_load();
    EXPECT_THAT(load_call_count, testing::Gt(count_after_initial));
}

TEST_F(TestOverrideConfigFile, reloads_when_file_added_to_symlinked_override_dir)
{
    write_base_config();

    auto const real_dir = std::filesystem::path{symlink_targets_dir} / "overrides_add";
    std::filesystem::create_directories(real_dir);

    create_override_dir_symlink(real_dir);

    invoke_runner([this](miral::MirRunner& runner)
    {
        config = ConfigFile{
            runner,
            override_config_file,
            ConfigFile::Mode::reload_on_change,
            [this](auto streams) { record_load(streams); }};
    });

    wait_for_load();
    EXPECT_THAT(last_load_stream_count, testing::Eq(1u)); // base only

    // Add a new file in the real directory
    {
        mark_pending();
        std::ofstream{real_dir / "10-new.conf"} << "new content";
    }
    wait_for_load();
    EXPECT_THAT(last_load_stream_count, testing::Eq(2u)); // base + new
}

TEST_F(TestOverrideConfigFile, reloads_when_symlinked_override_file_target_is_modified)
{
    write_base_config();
    std::filesystem::create_directories(override_dir());

    auto const real_file = std::filesystem::path{symlink_targets_dir} / "real_override.conf";
    { std::ofstream{real_file} << "original content"; }

    create_file_symlink(real_file, "10-override.conf");

    invoke_runner([this](miral::MirRunner& runner)
    {
        config = ConfigFile{
            runner,
            override_config_file,
            ConfigFile::Mode::reload_on_change,
            [this](auto streams) { record_load(streams); }};
    });

    wait_for_load();
    auto const count_after_initial = load_call_count;

    // Modify the real file that the symlink points to
    write_real_file(real_file, "modified content");
    wait_for_load(FailOnTimeout::yes);
    EXPECT_THAT(load_call_count, testing::Gt(count_after_initial));
}

TEST_F(TestOverrideConfigFile, reloads_when_override_file_symlink_is_repointed)
{
    write_base_config();
    std::filesystem::create_directories(override_dir());

    auto const real_file_a = std::filesystem::path{symlink_targets_dir} / "override_a.conf";
    auto const real_file_b = std::filesystem::path{symlink_targets_dir} / "override_b.conf";
    { std::ofstream{real_file_a} << "content a"; }
    { std::ofstream{real_file_b} << "content b"; }

    create_file_symlink(real_file_a, "10-override.conf");

    invoke_runner([this](miral::MirRunner& runner)
    {
        config = ConfigFile{
            runner,
            override_config_file,
            ConfigFile::Mode::reload_on_change,
            [this](auto streams) { record_load(streams); }};
    });

    wait_for_load();
    auto const count_after_initial = load_call_count;

    // Re-point the symlink to a different file
    create_file_symlink(real_file_b, "10-override.conf");
    wait_for_load();
    EXPECT_THAT(load_call_count, testing::Gt(count_after_initial));
}

TEST_F(TestOverrideConfigFile, reloads_when_override_dir_is_created_after_startup)
{
    write_base_config();
    // No override directory exists yet

    invoke_runner([this](miral::MirRunner& runner)
    {
        config = ConfigFile{
            runner,
            override_config_file,
            ConfigFile::Mode::reload_on_change,
            [this](auto streams) { record_load(streams); }};
    });

    wait_for_load();
    EXPECT_THAT(last_load_stream_count, testing::Eq(1u)); // base only

    // Now create the override directory with a file
    write_override_file("10-late.conf");
    wait_for_load();
    EXPECT_THAT(last_load_stream_count, testing::Eq(2u)); // base + override
}

TEST_F(TestOverrideConfigFile, reloads_when_override_dir_symlink_is_created_after_startup)
{
    write_base_config();
    // No override directory or symlink exists yet

    auto const real_dir = std::filesystem::path{symlink_targets_dir} / "late_overrides";
    std::filesystem::create_directories(real_dir);
    { std::ofstream{real_dir / "10-late.conf"} << "late content"; }

    invoke_runner([this](miral::MirRunner& runner)
    {
        config = ConfigFile{
            runner,
            override_config_file,
            ConfigFile::Mode::reload_on_change,
            [this](auto streams) { record_load(streams); }};
    });

    wait_for_load();
    EXPECT_THAT(last_load_stream_count, testing::Eq(1u)); // base only

    // Create a symlink to the real directory
    create_override_dir_symlink(real_dir);
    wait_for_load();
    EXPECT_THAT(last_load_stream_count, testing::Eq(2u)); // base + 1 from real_dir
}

TEST_F(TestOverrideConfigFile, dangling_override_dir_symlink_loads_base_only)
{
    write_base_config();

    // Create a symlink to a nonexistent directory
    auto const link_path = override_dir();
    std::filesystem::create_symlink("/tmp/test_override_config_file/nonexistent", link_path);

    invoke_runner([this](miral::MirRunner& runner)
    {
        config = ConfigFile{
            runner,
            override_config_file,
            ConfigFile::Mode::no_reloading,
            [this](auto streams) { record_load(streams); }};
    });

    wait_for_load();
    EXPECT_THAT(last_load_stream_count, testing::Eq(1u)); // base only, dangling symlink is skipped
}

TEST_F(TestOverrideConfigFile, dangling_file_symlink_in_override_dir_is_skipped)
{
    write_base_config();
    std::filesystem::create_directories(override_dir());

    write_override_file("10-real.conf");

    // Create a dangling symlink
    std::filesystem::create_symlink("/tmp/test_override_config_file/nonexistent.conf", override_dir() / "20-dangling.conf");

    invoke_runner([this](miral::MirRunner& runner)
    {
        config = ConfigFile{
            runner,
            override_config_file,
            ConfigFile::Mode::no_reloading,
            [this](auto streams) { record_load(streams); }};
    });

    wait_for_load();
    EXPECT_THAT(last_load_stream_count, testing::Eq(2u)); // base + 10-real.conf, dangling symlink skipped
}

TEST_F(TestOverrideConfigFile, no_reloading_mode_does_not_reload_on_override_changes)
{
    write_base_config();
    write_override_file("10-first.conf");

    invoke_runner([this](miral::MirRunner& runner)
    {
        config = ConfigFile{
            runner,
            override_config_file,
            ConfigFile::Mode::no_reloading,
            [this](auto streams) { record_load(streams); }};
    });

    wait_for_load();
    EXPECT_THAT(load_call_count, testing::Eq(1));

    // Write more files — should NOT trigger a reload
    write_override_file("20-second.conf");
    wait_for_load();
    EXPECT_THAT(load_call_count, testing::Eq(1));
}

TEST_F(TestOverrideConfigFile, no_reloading_mode_does_not_reload_on_base_config_change)
{
    write_base_config();

    invoke_runner([this](miral::MirRunner& runner)
    {
        config = ConfigFile{
            runner,
            override_config_file,
            ConfigFile::Mode::no_reloading,
            [this](auto streams) { record_load(streams); }};
    });

    wait_for_load();
    EXPECT_THAT(load_call_count, testing::Eq(1));

    rewrite_base_config();
    wait_for_load();
    EXPECT_THAT(load_call_count, testing::Eq(1));
}

TEST_F(TestOverrideConfigFile, reloads_when_override_file_is_deleted)
{
    write_base_config();
    write_override_file("10-first.conf");
    write_override_file("20-second.conf");

    invoke_runner([this](miral::MirRunner& runner)
    {
        config = ConfigFile{
            runner,
            override_config_file,
            ConfigFile::Mode::reload_on_change,
            [this](auto streams) { record_load(streams); }};
    });

    wait_for_load();
    EXPECT_THAT(last_load_stream_count, testing::Eq(3u)); // base + 2 overrides

    delete_override_file("10-first.conf");
    wait_for_load(FailOnTimeout::yes);
    EXPECT_THAT(last_load_stream_count, testing::Eq(2u)); // base + 1 remaining override
}

TEST_F(TestOverrideConfigFile, reloads_when_override_file_is_moved_out_of_directory)
{
    write_base_config();
    write_override_file("10-first.conf");

    invoke_runner([this](miral::MirRunner& runner)
    {
        config = ConfigFile{
            runner,
            override_config_file,
            ConfigFile::Mode::reload_on_change,
            [this](auto streams) { record_load(streams); }};
    });

    wait_for_load();
    EXPECT_THAT(last_load_stream_count, testing::Eq(2u)); // base + 1 override

    move_override_file_out("10-first.conf");
    wait_for_load(FailOnTimeout::yes);
    EXPECT_THAT(last_load_stream_count, testing::Eq(1u)); // base only
}

TEST_F(TestOverrideConfigFile, reloads_when_override_file_is_renamed_to_non_conf)
{
    write_base_config();
    write_override_file("10-first.conf");

    invoke_runner([this](miral::MirRunner& runner)
    {
        config = ConfigFile{
            runner,
            override_config_file,
            ConfigFile::Mode::reload_on_change,
            [this](auto streams) { record_load(streams); }};
    });

    wait_for_load();
    EXPECT_THAT(last_load_stream_count, testing::Eq(2u)); // base + 1 override

    // Renaming a .conf file away should trigger a reload without it
    rename_override_file("10-first.conf", "10-first.conf.bak");
    wait_for_load(FailOnTimeout::yes);
    EXPECT_THAT(last_load_stream_count, testing::Eq(1u)); // base only
}

TEST_F(TestOverrideConfigFile, reloads_when_file_is_renamed_to_conf_in_override_directory)
{
    write_base_config();
    write_override_file("10-first.conf.tmp");

    invoke_runner([this](miral::MirRunner& runner)
    {
        config = ConfigFile{
            runner,
            override_config_file,
            ConfigFile::Mode::reload_on_change,
            [this](auto streams) { record_load(streams); }};
    });

    wait_for_load();
    EXPECT_THAT(last_load_stream_count, testing::Eq(1u)); // base only, .tmp is ignored

    // Atomically renaming a .tmp to .conf should trigger a reload including it
    rename_override_file("10-first.conf.tmp", "10-first.conf");
    wait_for_load();
    EXPECT_THAT(last_load_stream_count, testing::Eq(2u)); // base + new override
}
