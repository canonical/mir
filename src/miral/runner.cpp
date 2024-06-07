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

#include "miral/runner.h"
#include "system_compositor_window_manager.h"
#include "fd_manager.h"
#include "input_device_config.h"
#include "join_client_threads.h"
#include "launch_app.h"

#include <mir/server.h>
#include <mir/main_loop.h>
#include <mir/report_exception.h>
#include <mir/options/option.h>

#include <algorithm>
#include <mutex>
#include <thread>


namespace mo = mir::options;

namespace
{
inline auto filename(std::string path) -> std::string
{
    // Remove the annoying ".bin" extension on Mir development binaries
    auto const bin = path.rfind(".bin");
    if (bin+4 == path.size())
        path.erase(bin);

    return path.substr(path.rfind('/')+1);
}
}

struct miral::MirRunner::Self
{
    Self(int argc, char const* argv[], std::string const& config_file) :
        argc(argc), argv(argv), config_file{config_file}, display_config_file{filename(argv[0])+ ".display"} {}

    auto run_with(std::initializer_list<std::function<void(::mir::Server&)>> options) -> int;

    int const argc;
    char const** const argv;
    std::string const config_file;
    std::string const display_config_file;

    std::mutex mutex;
    std::function<void()> start_callback{[]{}};
    std::function<void()> stop_callback{[this]{ join_client_threads(weak_server.lock().get()); }};
    std::function<void()> exception_handler{static_cast<void(*)()>(mir::report_exception)};
    std::weak_ptr<mir::Server> weak_server;
    std::shared_ptr<miral::FdManager> const fd_manager = std::make_shared<miral::FdManager>();

    struct SignalInfo
    {
        std::initializer_list<int> signals;
        std::function<void(int)> const handler;
    };
    
    std::vector<SignalInfo> signal_backlog;
};

miral::MirRunner::MirRunner(int argc, char const* argv[]) :
    self{std::make_unique<Self>(argc, argv, filename(argv[0]) + ".config")}
{
}

miral::MirRunner::MirRunner(int argc, char const* argv[], char const* config_file) :
    self{std::make_unique<Self>(argc, argv, config_file)}
{
}

miral::MirRunner::~MirRunner() = default;

namespace
{
auto const env_hacks = "env-hacks";

void enable_env_hacks(::mir::Server& server)
{
    server.add_configuration_option(
        env_hacks, "Colon separated list of environment variable settings", mir::OptionType::string);
}

void apply_env_hacks(::mir::Server& server)
{
    if (auto const options = server.get_options())
    {
        if (options->is_set(env_hacks))
        {
            auto const value = options->get<std::string>(env_hacks);

            for (auto i = begin(value); i != end(value); )
            {
                auto const j = find(i, end(value), ':');

                auto equals = find(i, j, '=');

                auto const key = std::string(i, equals);
                if (j != equals) ++equals;
                auto const val = std::string(equals, j);

                setenv(key.c_str(), val.c_str(), true);

                if ((i = j) != end(value)) ++i;
            }
        }
    }
}
}  // namespace

auto miral::MirRunner::Self::run_with(std::initializer_list<std::function<void(::mir::Server&)>> options)
-> int
try
{
    auto const server = std::make_shared<mir::Server>();

    {
        std::lock_guard lock{mutex};

        server->set_config_filename(config_file);
        server->set_exception_handler(exception_handler);

        enable_env_hacks(*server);

        // TODO: This is a band-aid solution, and perhaps not the best one. The mir core libraries provide a default
        // window manager, but mirAL would like to provide a different default window manager.
        server->override_the_window_manager_builder([&server](mir::shell::FocusController* focus_controller) -> std::shared_ptr<mir::shell::WindowManager>
        {
            return std::make_shared<SystemCompositorWindowManager>(
                focus_controller,
                server->the_shell_display_layout(),
                server->the_session_coordinator(),
                *server->the_display_configuration_observer_registrar()
            );
        });

        add_input_device_configuration_options_to(*server);
        for (auto& option : options)
            option(*server);

        server->add_stop_callback(std::move(stop_callback));

        // Provide the command line and run the server
        server->set_command_line(argc, argv);

        server->apply_settings();
        apply_env_hacks(*server);

        weak_server = server;
    }

    server->add_init_callback([server, this]
    {
        // By enqueuing the notification code in the main loop, we are
        // ensuring that the server has really and fully started.
        auto const main_loop = server->the_main_loop();
        main_loop->enqueue(this, std::move(start_callback));

        for (auto const& signal : signal_backlog)
        {
            main_loop->register_signal_handler(signal.signals, signal.handler);
        }
        signal_backlog.clear();

        fd_manager->set_main_loop(main_loop);
    });

    server->run();

    return server->exited_normally() ? EXIT_SUCCESS : EXIT_FAILURE;
}
catch (...)
{
    exception_handler();
    return EXIT_FAILURE;
}

void miral::MirRunner::add_start_callback(std::function<void()> const& start_callback)
{
    std::lock_guard lock{self->mutex};
    auto const& existing = self->start_callback;

    auto const updated = [=]
        {
            existing();
            start_callback();
        };

    self->start_callback = updated;
}

void miral::MirRunner::add_stop_callback(std::function<void()> const& stop_callback)
{
    std::lock_guard lock{self->mutex};
    auto const& existing = self->stop_callback;

    auto const updated = [=]
        {
            stop_callback();
            existing();
        };

    self->stop_callback = updated;
}

void miral::MirRunner::register_signal_handler(
    std::initializer_list<int> signals,
    std::function<void(int)> const& handler)
{
    std::lock_guard lock{self->mutex};
    
    if (auto const server = self->weak_server.lock())
    {
        auto const main_loop = server->the_main_loop();
        main_loop->register_signal_handler(signals, handler);
    }
    else
    {
        auto const signal_info = Self::SignalInfo{signals, handler};
        self->signal_backlog.push_back(signal_info);
    }
}

auto miral::MirRunner::register_fd_handler(mir::Fd fd, std::function<void(int)> const& handler)
-> std::unique_ptr<miral::FdHandle>
{
    return self->fd_manager->register_handler(fd, handler);
}

void miral::MirRunner::set_exception_handler(std::function<void()> const& handler)
{
    std::lock_guard lock{self->mutex};
    self->exception_handler = handler;
}

auto miral::MirRunner::run_with(std::initializer_list<std::function<void(::mir::Server&)>> options)
-> int
{
    return self->run_with(options);
}

void miral::MirRunner::stop()
{
    std::lock_guard lock{self->mutex};

    if (auto const server = self->weak_server.lock())
    {
        server->the_main_loop()->enqueue(this, [server] { server->stop(); });
    }
}

auto miral::MirRunner::config_file() const -> std::string
{
    return self->config_file;
}

auto miral::MirRunner::display_config_file() const -> std::string
{
    return self->display_config_file;
}

auto miral::MirRunner::wayland_display() const -> mir::optional_value<std::string>
{
    std::lock_guard lock{self->mutex};

    if (auto const server = self->weak_server.lock())
    {
        return server->wayland_display();
    }

    return {};
}

auto miral::MirRunner::x11_display() const -> mir::optional_value<std::string>
{
    std::lock_guard lock{self->mutex};

    if (auto const server = self->weak_server.lock())
    {
        return server->x11_display();
    }

    return {};
}
