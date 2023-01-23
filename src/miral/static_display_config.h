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

#ifndef MIRAL_STATIC_DISPLAY_CONFIG_H_
#define MIRAL_STATIC_DISPLAY_CONFIG_H_

#include <mir/abnormal_exit.h>
#include <mir/graphics/default_display_configuration_policy.h>
#include <mir/graphics/display_configuration.h>
#include <mir/fd.h>

#include <map>
#include <mutex>
#include <optional>
#include <functional>
#include <iosfwd>
#include "mir/log.h"

namespace mir
{
class MainLoop;
class Server;
namespace shell { class DisplayConfigurationController; }
}

namespace miral
{
class YamlFileDisplayConfig : public mir::graphics::DisplayConfigurationPolicy
{
public:
    void load_config(std::istream& config_file, std::string const& error_prefix);

    virtual void apply_to(mir::graphics::DisplayConfiguration& conf);

    virtual void dump_config(std::function<void(std::ostream&)> const& print_template_config);

    void select_layout(std::string const& layout);

    auto list_layouts() const -> std::vector<std::string>;

private:
    std::string layout = "default";
    struct Config
    {
        bool  disabled = false;
        mir::optional_value<mir::geometry::Point>  position;
        mir::optional_value<mir::geometry::Size>   size;
        mir::optional_value<double> refresh;
        mir::optional_value<float>  scale;
        mir::optional_value<MirOrientation>  orientation;
        mir::optional_value<int> group_id;
    };
    using Id = std::tuple<mir::graphics::DisplayConfigurationCardId, MirOutputType, int>;

    using Id2Config = std::map<Id, Config>;
    using Layout2Id2Config = std::map<std::string, Id2Config>;
    Layout2Id2Config config;
};

class ReloadingYamlFileDisplayConfig : public YamlFileDisplayConfig
{
public:
    explicit ReloadingYamlFileDisplayConfig(std::string basename);
    ~ReloadingYamlFileDisplayConfig();
    std::string const basename;

    void init_auto_reload(mir::Server& server);

    void config_path(std::string newpath);

    void dump_config(std::function<void(std::ostream&)> const& print_template_config) override;

private:
    auto the_main_loop() const -> std::shared_ptr<mir::MainLoop>;

    auto the_display_configuration_controller() const -> std::shared_ptr<mir::shell::DisplayConfigurationController>;

    auto inotify_config_path() -> std::optional<mir::Fd>;

    void auto_reload();

    std::mutex mutable mutex;
    std::weak_ptr<mir::shell::DisplayConfigurationController> the_display_configuration_controller_;
    std::weak_ptr<mir::MainLoop> the_main_loop_;
    std::optional<std::string> config_path_;
    std::optional<int> config_path_wd;
};

class StaticDisplayConfig : public ReloadingYamlFileDisplayConfig
{
public:
    StaticDisplayConfig(std::string const& filename);
};
}

#endif //MIRAL_STATIC_DISPLAY_CONFIG_H_
