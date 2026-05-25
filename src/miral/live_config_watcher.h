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

#ifndef MIRAL_LIVE_CONFIG_WATCHER
#define MIRAL_LIVE_CONFIG_WATCHER

#include <mir/fd.h>
#include <miral/runner.h>

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <set>
#include <vector>

class inotify_event;

namespace miral
{
namespace live_config
{
class OverridesList;
/// RAII wrapper for an inotify watch descriptor.
class InotifyWatch
{
public:
    InotifyWatch() = default;
    InotifyWatch(mir::Fd const& inotify_fd, std::filesystem::path const& path);

    ~InotifyWatch();
    InotifyWatch(InotifyWatch&&) noexcept;
    InotifyWatch& operator=(InotifyWatch&&) noexcept;
    InotifyWatch(InotifyWatch const&) = delete;
    InotifyWatch& operator=(InotifyWatch const&) = delete;

    explicit operator bool() const { return wd_ >= 0; }
    int value() const { return wd_; }
    void reset();

private:
    mir::Fd const* inotify_fd_{nullptr};
    int wd_{-1};
};

class Watcher : public std::enable_shared_from_this<Watcher>
{
public:
    Watcher();
    virtual ~Watcher() = default;
    Watcher(Watcher const&) = delete;
    Watcher& operator=(Watcher const&) = delete;
    Watcher(Watcher&&) = delete;
    Watcher& operator=(Watcher&&) = delete;
    void register_handler(MirRunner& runner);
    virtual void handler(int) = 0;

protected:
    virtual bool should_register() const = 0;
    void for_each_inotify_event(std::function<void(inotify_event const&)> const& callback) const;

    mir::Fd const inotify_fd;
    std::unique_ptr<miral::FdHandle> fd_handle;
};

auto config_directory(std::filesystem::path const& file) -> std::optional<std::filesystem::path>;
auto get_config_roots(std::filesystem::path const& file) -> std::vector<std::filesystem::path>;
auto collect_override_files(std::filesystem::path const& override_directory, std::string_view extension)
    -> std::set<std::filesystem::path>;
auto collect_paths(miral::live_config::OverridesList const& config_files) -> std::string;
auto open_file(std::filesystem::path const& path) -> std::unique_ptr<std::istream>;

}
}

#endif // MIRAL_LIVE_CONFIG_WATCHER
