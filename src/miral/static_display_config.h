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
#include <mir/fd.h>
#include <mir/graphics/default_display_configuration_policy.h>
#include <mir/graphics/display_configuration.h>
#include <mir/log.h>

#include <functional>
#include <iosfwd>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <any>

namespace YAML
{
class Node;
}

namespace mir
{
class MainLoop;
class Server;
namespace shell { class DisplayConfigurationController; }
}

namespace YAML { class Node; }

namespace miral
{
class YamlFileDisplayConfig : public mir::graphics::DisplayConfigurationPolicy
{
public:
    void load_config(std::istream& config_file, std::string const& filename);

    void apply_to(mir::graphics::DisplayConfiguration& conf) override;
    virtual void confirm(mir::graphics::DisplayConfiguration const& conf) override;

    void select_layout(std::string const& layout);

    auto list_layouts() const -> std::vector<std::string>;

    auto layout_userdata(std::string const& key) -> std::optional<std::any const>;

    void add_output_attribute(std::string const& key);

    void layout_userdata_builder(
        std::string const& key,
        std::function<std::any(YAML::Node const&)> const& builder);

    static void serialize_configuration(std::ostream& out, mir::graphics::DisplayConfiguration& conf);

    static void apply_default_configuration(mir::graphics::DisplayConfiguration& conf);

private:
    std::mutex mutable mutex;
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
        std::map<std::string, std::optional<std::string>> custom_attribute;
    };

    enum class Property
    {
        Port,
        Vendor,
        Model,
        Product,
        Serial,
    };

    // Workaround for not allowing static initialization in class
    static std::map<Property, std::string const> const display_matching_properties();

    using Matchers = std::map<Property, std::string>;
    using Matchers2Config = std::vector<std::pair<Matchers, Config>>;
    struct LayoutConfig
    {
        Matchers2Config matchers2config;
        std::map<std::string, std::any> userdata;
    };

    std::map<std::string, LayoutConfig> config;

    std::set<std::string> custom_output_attributes;

    using LayoutUserDataBuilder = std::function<std::any(YAML::Node const& value)>;
    std::map<std::string, LayoutUserDataBuilder> layout_userdata_builder_funcs;

    void parse_configuration(YAML::Node const& node, Config& config, std::string const& error_prefix, std::string const& identifier);

    static void apply_to_output(mir::graphics::UserDisplayConfigurationOutput& conf_output, Config const& conf);

    static void serialize_output_configuration(
        std::ostream& out, mir::graphics::UserDisplayConfigurationOutput const& conf_output);
};

class ReloadingYamlFileDisplayConfig : public YamlFileDisplayConfig
{
public:
    explicit ReloadingYamlFileDisplayConfig(std::string basename);
    ~ReloadingYamlFileDisplayConfig();
    std::string const basename;

    void init_auto_reload(mir::Server& server);

    void config_path(std::string newpath);

    void confirm(mir::graphics::DisplayConfiguration const& conf) override;

    void check_for_layout_override();

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

// Monitors dirname(filename) for changes to basename(filename) and reload,
// does not monitor for the creation of dirname(filename)
class StaticDisplayConfig : public ReloadingYamlFileDisplayConfig
{
public:
    StaticDisplayConfig(std::string const& filename);
};
}

#endif //MIRAL_STATIC_DISPLAY_CONFIG_H_
