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

#include "live_config_watcher.h"

#include <mir/log.h>
#include <miral/live_config_overrides_list.h>

#include <boost/throw_exception.hpp>

#include <sys/inotify.h>
#include <unistd.h>

#include <array>
#include <cstring>
#include <format>
#include <fstream>
#include <ranges>
#include <span>
#include <sstream>

namespace mlc = miral::live_config;
namespace fs = std::filesystem;
using path = fs::path;

mlc::InotifyWatch::InotifyWatch(mir::Fd const& inotify_fd, path const& p) :
    inotify_fd_{&inotify_fd}
{
    if (inotify_fd < 0)
        BOOST_THROW_EXCEPTION(
            (std::system_error{errno, std::system_category(), "Failed to initialize inotify fd"}));

    wd_ = inotify_add_watch(
        inotify_fd, p.c_str(), IN_CLOSE_WRITE | IN_CREATE | IN_MOVED_TO | IN_DELETE | IN_MOVED_FROM);

    if (wd_ < 0)
    {
        mir::log_warning("Failed to add inotify watch for '%s': %s", p.c_str(), strerror(errno));
        wd_ = -1;
        inotify_fd_ = nullptr;
    }
}

mlc::InotifyWatch::~InotifyWatch()
{
    reset();
}

mlc::InotifyWatch::InotifyWatch(InotifyWatch&& other) noexcept :
    inotify_fd_{other.inotify_fd_},
    wd_{other.wd_}
{
    other.inotify_fd_ = nullptr;
    other.wd_ = -1;
}

auto mlc::InotifyWatch::operator=(InotifyWatch&& other) noexcept -> InotifyWatch&
{
    if (this != &other)
    {
        reset();
        inotify_fd_ = other.inotify_fd_;
        wd_ = other.wd_;
        other.inotify_fd_ = nullptr;
        other.wd_ = -1;
    }
    return *this;
}

void mlc::InotifyWatch::reset()
{
    if (wd_ >= 0 && inotify_fd_ && inotify_rm_watch(*inotify_fd_, wd_) != 0)
        mir::log_warning("Failed to remove inotify watch (wd=%d): %s", wd_, strerror(errno));
    inotify_fd_ = nullptr;
    wd_ = -1;
}


mlc::Watcher::Watcher() :
    inotify_fd{inotify_init1(IN_CLOEXEC)}
{
}

void mlc::Watcher::register_handler(miral::MirRunner& runner)
{
    if (should_register())
    {
        auto const weak_this = weak_from_this();
        fd_handle = runner.register_fd_handler(
            inotify_fd,
            [weak_this](int fd)
            {
                if (auto const strong_this = weak_this.lock())
                {
                    strong_this->handler(fd);
                }
                else
                {
                    mir::log_debug("Watcher has been removed, but handler invoked");
                }
            });
    }
}

void mlc::Watcher::for_each_inotify_event(std::function<void(inotify_event const&)> const& callback) const
{
    // std::array is standard-layout, so its address equals its first data member's
    // address (no padding at the beginning). data() returns addressof(front()),
    // which is that same address. Therefore, alignas(inotify_event) guarantees
    // buffer.data() is aligned to alignof(inotify_event).
    // References from the C++ standard (https://eel.is/c++draft):
    // - [array.overview]/2: std::array is an aggregate (and thus standard-layout)
    // - [class.mem.general]/31: a standard-layout object's address equals its
    //   first non-static data member's address (note 11: no padding at beginning)
    // - [array.members]/2: data() == addressof(front()) for non-empty arrays
    // - [sequence.reqmts]/73: front() returns *begin() (the first element)
    alignas(inotify_event) std::array<std::byte, sizeof(inotify_event) + NAME_MAX + 1> buffer = {};

    auto const readsize = read(inotify_fd, buffer.data(), buffer.size());
    if (readsize < static_cast<ssize_t>(sizeof(inotify_event)))
        return;

    auto remaining = std::span{buffer}.first(static_cast<std::size_t>(readsize));
    while (remaining.size() >= sizeof(inotify_event))
    {
        // LEGACY(Clang) Replace with std::start_lifetime_as<inotify_event const> once Clang supports it (C++23, P2590).
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        auto const& event = reinterpret_cast<inotify_event const&>(*remaining.data());

        // The kernel guarantees event.len is padded to maintain inotify_event
        // alignment for subsequent events (see inotify(7): "The name field
        // [...] is padded with null bytes to the next inotify_event structure
        // boundary"), so all subsequent reinterpret_casts are safe.
        auto const event_size = sizeof(inotify_event) + event.len;
        if (remaining.size() < event_size)
            break;
        callback(event);
        remaining = remaining.subspan(event_size);
    }
}

auto mlc::config_directory(path const& file) -> std::optional<path>
{
    if (file.has_parent_path())
    {
        return file.parent_path();
    }
    else if (auto config_home = getenv("XDG_CONFIG_HOME"))
    {
        return config_home;
    }
    else if (auto home = getenv("HOME"))
    {
        return path(home) / ".config";
    }
    else
    {
        return std::nullopt;
    }
}

auto mlc::get_config_roots(path const& file) -> std::vector<path>
{
    std::vector<path> config_roots;
    std::set<path> seen;

    auto const try_push = [&](path const& entry)
    {
        auto const not_already_seen = seen.insert(fs::weakly_canonical(entry)).second;
        if (not_already_seen)
            config_roots.push_back(entry);
    };

    if (auto const directory = config_directory(file))
        try_push(*directory);

    if (auto config_dirs = getenv("XDG_CONFIG_DIRS"))
    {
        std::istringstream config_stream{config_dirs};
        for (std::string config_root; getline(config_stream, config_root, ':');)
        {
            if (!config_root.empty())
                try_push(config_root);
        }
    }
    else
    {
        try_push("/etc/xdg");
    }

    return config_roots;
}

auto mlc::collect_override_files(path const& override_directory, std::string_view extension) -> std::set<path>
{
    std::error_code ec;
    if (!fs::exists(override_directory, ec) || !fs::is_directory(override_directory, ec))
        return {};

    fs::directory_iterator dir{override_directory, ec};
    if (ec)
    {
        mir::log_warning(
            "Failed to read override directory '%s': %s", override_directory.c_str(), ec.message().c_str());
        return {};
    }

    auto matching = dir
                  | std::views::filter(
                        [extension](auto const& f)
                        {
                            std::error_code ec;
                            return fs::is_regular_file(f, ec) && f.path().extension() == extension;
                        })
                  | std::views::transform([](auto const& f) { return f.path(); });

    std::set<path> result;
    std::ranges::copy(matching, std::inserter(result, result.end()));
    return result;
}

auto mlc::collect_paths(mlc::OverridesList const& config_files) -> std::string
{
    std::string result;
    auto const append = [&](std::string_view label, auto const& path, auto&&...)
    {
        if (!result.empty())
            result += ", ";
        result += std::format("{} ({})", path.string(), label);
    };

    config_files.for_each(
        [&](auto const& p, auto&&...) { append("unchanged", p); },
        [&](auto const& p, auto&&...) { append("new", p); },
        [&](auto const& p, auto&&...) { append("modified", p); },
        [&](auto const& p, auto&&...) { append("dropped", p); });

    return result;
}

auto mlc::open_file(std::filesystem::path const& path) -> std::unique_ptr<std::istream>
{
    auto stream = std::make_unique<std::ifstream>(path);
    if (!stream->is_open())
        return nullptr;
    return stream;
}
