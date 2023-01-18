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

#include "miral/display_configuration.h"
#include "miral/runner.h"
#include "miral/command_line_option.h"
#include "static_display_config.h"

#include <mir/server.h>
#include <mir/shell/display_configuration_controller.h>
#include <mir/main_loop.h>
#include <mir/log.h>

#include <unistd.h>
#include <sys/inotify.h>

#include <boost/throw_exception.hpp>

#include <fstream>
#include <mutex>
#include <optional>
#include <sstream>

class miral::DisplayConfiguration::Self : public StaticDisplayConfig
{
public:
    Self(std::string const& basename) : basename{basename} { find_and_load_config(); }
    std::string const basename;

    void find_and_load_config()
    {
        std::string config_roots;

        if (auto config_home = getenv("XDG_CONFIG_HOME"))
            (config_roots = config_home) += ":";
        else if (auto home = getenv("HOME"))
            (config_roots = home) += "/.config:";

        if (auto config_dirs = getenv("XDG_CONFIG_DIRS"))
            config_roots += config_dirs;
        else
            config_roots += "/etc/xdg";

        decltype(config_path) new_config_path;
        std::istringstream config_stream(config_roots);

        /* Read options from config files */
        for (std::string config_root; getline(config_stream, config_root, ':');)
        {
            auto const& filename = config_root + "/" + basename;

            if (std::ifstream config_file{filename})
            {
                load_config(config_file, "ERROR: in display configuration file: '" + filename + "' : ");
                new_config_path = config_root;
                break;
            }
        }

        std::lock_guard lock{mutex};
        config_path = new_config_path;
    }

    void dump_config(std::function<void(std::ostream&)> const& print_template_config) override
    {
        std::string config_dir;

        if (auto config_home = getenv("XDG_CONFIG_HOME"))
            config_dir = config_home;
        else if (auto home = getenv("HOME"))
            (config_dir = home) += "/.config";

        if (config_dir.empty())
        {
            mir::log_debug("Nowhere to write display configuration template: Neither XDG_CONFIG_HOME or HOME is set");
        }
        else
        {
            auto const filename = config_dir + "/" + basename;
            decltype(config_path) new_config_path;

            if (access(filename.c_str(), F_OK))
            {
                std::ofstream out{filename};
                print_template_config(out);

                mir::log_debug(
                    "%s display configuration template: %s",
                    out ? "Written" : "Failed writing",
                    filename.c_str());

                if (out)
                {
                    new_config_path = config_dir;

                }

                std::lock_guard lock{mutex};
                config_path = new_config_path;
            }
        }

        StaticDisplayConfig::dump_config(print_template_config);
    }

    auto the_display_configuration_controller() const
    {
        std::lock_guard lock{mutex};
        return the_display_configuration_controller_.lock();
    }

    void the_display_configuration_controller(std::weak_ptr<mir::shell::DisplayConfigurationController> value)
    {
        std::lock_guard lock{mutex};
        the_display_configuration_controller_ = std::move(value);
    }

    auto the_main_loop() const
    {
        std::lock_guard lock{mutex};
        return the_main_loop_.lock();
    }

    void the_main_loop(std::weak_ptr<mir::MainLoop> value)
    {
        std::lock_guard lock{mutex};
        the_main_loop_ = std::move(value);
    }

    auto inotify_config_path() -> std::optional<mir::Fd>
    {
        std::lock_guard lock{mutex};

        if (!config_path) return std::nullopt;

        mir::Fd inotify_fd{inotify_init()};

        if (inotify_fd < 0)
            BOOST_THROW_EXCEPTION((std::system_error{errno, std::system_category(), "Failed to initialize inotify_fd"}));

        config_path_wd = inotify_add_watch(inotify_fd, config_path.value().c_str(), IN_CLOSE_WRITE|IN_CREATE|IN_DELETE);

        return inotify_fd;
    }

    void auto_reload()
    {
        if (auto const icf = inotify_config_path())
        {
            if (auto const ml = the_main_loop())
            {
                ml->register_fd_handler({icf.value()}, this, [icf=icf.value(), this] (int)
                    {
                        union
                        {
                            inotify_event event;
                            char buffer[sizeof(inotify_event) + NAME_MAX + 1];
                        } inotify_buffer;

                        if (read(icf, &inotify_buffer, sizeof(inotify_buffer)) < static_cast<ssize_t>(sizeof(inotify_event)))
                            return;

                        if (inotify_buffer.event.mask & (IN_CLOSE_WRITE | IN_CREATE)
                            && inotify_buffer.event.name == basename)
                        try
                        {
                            find_and_load_config();
                            if (auto const dcc = the_display_configuration_controller())
                            {
                                auto config = dcc->base_configuration();
                                apply_to(*config);
                                dcc->set_base_configuration(config);
                            }
                        }
                        catch (mir::AbnormalExit const& except)
                        {
                            mir::log_warning("Failed to reload display configuration: %s", except.what());
                        }
                    });
            }
        }
    }

    ~Self()
    {
        if (auto const ml = the_main_loop())
        {
            ml->unregister_fd_handler(this);
        }
    }
private:
    std::mutex mutable mutex;
    std::weak_ptr<mir::shell::DisplayConfigurationController> the_display_configuration_controller_;
    std::weak_ptr<mir::MainLoop> the_main_loop_;
    std::optional<std::string> config_path;
    std::optional<int> config_path_wd;
};

miral::DisplayConfiguration::DisplayConfiguration(MirRunner const& mir_runner) :
    self{std::make_shared<Self>(mir_runner.display_config_file())}
{
}

void miral::DisplayConfiguration::select_layout(std::string const& layout)
{
    self->select_layout(layout);
}

void miral::DisplayConfiguration::operator()(mir::Server& server) const
{
    namespace mg = mir::graphics;

    server.wrap_display_configuration_policy([this](std::shared_ptr<mg::DisplayConfigurationPolicy> const&)
        -> std::shared_ptr<mg::DisplayConfigurationPolicy>
        {
            return self;
        });

    server.add_init_callback([self=self, &server]
        {
            self->the_display_configuration_controller(server.the_display_configuration_controller());
            self->the_main_loop(server.the_main_loop());
            self->auto_reload();
        });
}

auto miral::DisplayConfiguration::layout_option() -> miral::ConfigurationOption
{
    return pre_init(ConfigurationOption{
        [this](std::string const& layout) { select_layout(layout); },
        "display-layout",
        "Display configuration layout from `" + self->basename + "'\n"
        "(Found in $XDG_CONFIG_HOME or $HOME/.config, followed by $XDG_CONFIG_DIRS)",
        "default"});
}

auto miral::DisplayConfiguration::list_layouts() -> std::vector<std::string>
{
    return self->list_layouts();
}

miral::DisplayConfiguration::~DisplayConfiguration() = default;

miral::DisplayConfiguration::DisplayConfiguration(miral::DisplayConfiguration const&) = default;

auto miral::DisplayConfiguration::operator=(miral::DisplayConfiguration const&) -> miral::DisplayConfiguration& = default;
