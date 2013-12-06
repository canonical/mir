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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "src/platform/graphics/mesa/anonymous_shm_file.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <boost/throw_exception.hpp>
#include <stdexcept>
#include <string>
#include <atomic>
#include <thread>

#include <cstdlib>
#include <unistd.h>
#include <sys/inotify.h>

namespace mg = mir::graphics;
namespace mgm = mir::graphics::mesa;

namespace
{

class TemporaryEnvironmentValue
{
public:
    TemporaryEnvironmentValue(char const* name, char const* value)
        : name{name},
          has_old_value{getenv(name) != nullptr},
          old_value{has_old_value ? getenv(name) : ""}
    {
        if (value)
            setenv(name, value, overwrite);
        else
            unsetenv(name);
    }

    ~TemporaryEnvironmentValue()
    {
        if (has_old_value)
            setenv(name.c_str(), old_value.c_str(), overwrite);
        else
            unsetenv(name.c_str());
    }

private:
    static int const overwrite = 1;
    std::string const name;
    bool const has_old_value;
    std::string const old_value;
};

class TemporaryDirectory
{
public:
    TemporaryDirectory()
    {
        char tmpl[] = "/tmp/mir-test-dir-XXXXXX";
        if (mkdtemp(tmpl) == nullptr)
        {
            BOOST_THROW_EXCEPTION(
                std::runtime_error("Couldn't create temporary test directory"));
        }
        path_ = tmpl;
    }

    ~TemporaryDirectory()
    {
        rmdir(path());
    }

    char const* path() const
    {
        return path_.c_str();
    }

private:
    std::string path_;
};

class PathWatcher
{
public:
    PathWatcher(const char* path)
        : inotify_fd{inotify_init1(IN_NONBLOCK)},
          watch_fd{inotify_add_watch(inotify_fd, path,
                                     IN_CREATE | IN_DELETE)}
    {
        /* TODO: RAII fd */
        if (inotify_fd < 0)
        {
            BOOST_THROW_EXCEPTION(
                std::runtime_error("Couldn't create inotify fd"));
        }
        if (watch_fd < 0)
        {
            close(inotify_fd);
            BOOST_THROW_EXCEPTION(
                std::runtime_error("Couldn't create watch fd"));
        }
    }

    ~PathWatcher()
    {
        inotify_rm_watch(inotify_fd, watch_fd);
        close(inotify_fd);
    }

    void process_events() const
    {
        while (process_events_step()) continue;
    }

    MOCK_CONST_METHOD1(file_created, void(char const*));
    MOCK_CONST_METHOD1(file_deleted, void(char const*));
    
private:
    bool process_events_step() const
    {
        size_t const max_path_size = 1024;
        size_t const buffer_size = sizeof(struct inotify_event) + max_path_size;
        uint8_t buffer[buffer_size];

        auto nread = read(inotify_fd, buffer, buffer_size); 

        if (nread < 0)
        {
            if (errno == EWOULDBLOCK)
            {
                return false;
            }
            else
            {
                BOOST_THROW_EXCEPTION(
                    std::runtime_error("Couldn't read from inotify fd"));
            }
        } 

        ssize_t i = 0;
        while (i < nread)
        {
            auto event = reinterpret_cast<struct inotify_event*>(&buffer[i]);
            if (event->len)
            {
                if (event->mask & IN_CREATE)
                {
                    file_created(event->name);
                }

                if (event->mask & IN_DELETE)
                {
                    file_deleted(event->name);
                } 
            }
            i += sizeof(struct inotify_event) + event->len;
        }

        return true;
    }

    int const inotify_fd;
    int const watch_fd;
};

}

TEST(AnonymousShmFile, is_created_and_deleted_in_xdg_runtime_dir)
{
    using namespace testing;
    
    TemporaryDirectory const temp_dir;
    TemporaryEnvironmentValue const env{"XDG_RUNTIME_DIR", temp_dir.path()};
    PathWatcher const path_watcher{temp_dir.path()};
    size_t const file_size{100};

    InSequence s;
    EXPECT_CALL(path_watcher, file_created(StartsWith("mir-buffer-")));
    EXPECT_CALL(path_watcher, file_deleted(StartsWith("mir-buffer-")));

    mgm::AnonymousShmFile shm_file{file_size};

    path_watcher.process_events();
}

TEST(AnonymousShmFile, is_created_and_deleted_in_tmp_dir)
{
    using namespace testing;
    
    TemporaryEnvironmentValue const env{"XDG_RUNTIME_DIR", nullptr};
    PathWatcher const path_watcher{"/tmp"};
    size_t const file_size{100};

    InSequence s;
    EXPECT_CALL(path_watcher, file_created(StartsWith("mir-buffer-")));
    EXPECT_CALL(path_watcher, file_deleted(StartsWith("mir-buffer-")));

    mgm::AnonymousShmFile shm_file{file_size};

    path_watcher.process_events();
}

TEST(AnonymousShmFile, has_correct_size)
{
    using namespace testing;
    
    TemporaryDirectory const temp_dir;
    TemporaryEnvironmentValue const env{"XDG_RUNTIME_DIR", temp_dir.path()};
    size_t const file_size{100};

    mgm::AnonymousShmFile shm_file{file_size};

    struct stat stat;
    fstat(shm_file.fd(), &stat);

    EXPECT_EQ(static_cast<off_t>(file_size), stat.st_size);
}

TEST(AnonymousShmFile, writing_to_base_ptr_writes_to_file)
{
    using namespace testing;
    
    TemporaryDirectory const temp_dir;
    TemporaryEnvironmentValue const env{"XDG_RUNTIME_DIR", temp_dir.path()};
    size_t const file_size{100};

    mgm::AnonymousShmFile shm_file{file_size};

    auto base_ptr = reinterpret_cast<uint8_t*>(shm_file.base_ptr());

    for (size_t i = 0; i < file_size; i++)
    {
        base_ptr[i] = i;
    }

    std::vector<unsigned char> buffer(file_size);

    EXPECT_EQ(static_cast<ssize_t>(file_size),
              read(shm_file.fd(), buffer.data(), file_size));

    for (size_t i = 0; i < file_size; i++)
    {
        EXPECT_EQ(base_ptr[i], buffer[i]) << "i=" << i;
    }
}
