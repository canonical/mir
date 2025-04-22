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

#ifndef MIRAL_DISPLAY_CONFIGURATION_H
#define MIRAL_DISPLAY_CONFIGURATION_H

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <optional>
#include <any>

namespace mir { class Server; }

namespace miral
{
class MirRunner;
class ConfigurationOption;

/// Enable display configuration.
/// The config file (miral::MirRunner::display_config_file()) is located via
/// the XDG Base Directory Specification. Vis:
///($XDG_CONFIG_HOME or $HOME/.config followed by $XDG_CONFIG_DIRS)
/// \remark Since MirAL 2.4
/// \note From MirAL 3.8 will monitor the configuration file or, if none found,
/// for the creation of a file in $XDG_CONFIG_HOME or $HOME/.config. Changes
/// to this file will be reloaded. In addition, the selected layout may be
/// overridden using a corresponding file: display_config_file() + "-layout"
/// which will also be monitored and changes reloaded
class DisplayConfiguration
{
public:
    /// A class providing access to an arbitrary piece of data from the [DisplayConfiguration].
    /// This is specifically useful when a user wants to extend the based display configuration
    /// with some sort of custom payload (e.g. a user may want to extend the layout configuration
    /// with information describing the position and size of specific applications).
    /// \remark Since MirAL 5.4
    class Node
    {
    public:
        enum class Type
        {
            integer,
            string,
            sequence,
            map,
            unknown
        };

        auto type() const -> Type;
        auto as_string() const -> std::string;
        auto as_int() const -> int;
        void for_each(std::function<void(Node const&)> const& f) const;
        auto has(std::string const& key) const -> bool;
        auto at(std::string const& key) const -> std::optional<Node>;
        Node(Node&&) noexcept = default;
        Node& operator=(Node&&) noexcept = default;
        Node(Node const&) = delete;
        Node& operator=(Node const&) = delete;
        ~Node();

    private:
        friend DisplayConfiguration;
        struct Self;
        explicit Node(std::unique_ptr<Self>&& self);

        std::unique_ptr<Self> self;
    };

    explicit DisplayConfiguration(MirRunner const& mir_runner);

    /// Provide the default 'display-layout' configuration option
    auto layout_option() -> ConfigurationOption;

    /// Select a layout from the configuration
    void select_layout(std::string const& layout);

    /// List all layouts found in the config file
    auto list_layouts() -> std::vector<std::string>;

    /// Enable a custom output attribute in the .display YAML
    /// \remark Since MirAL 3.8
    void add_output_attribute(std::string const& key);

    /// Retrieve the user data associated with the active layout
    /// for this provided key. Callers should provide this user data
    /// via [layout_userdata_builder].
    /// \remark Since MirAL 5.4
    auto layout_userdata(std::string const& key) const -> std::optional<std::any const>;

    /// Enable a custom layout attribute in the .display YAML.
    /// The caller must provides the key of this custom attribute in
    /// addition to a function that will be used to build the custom
    /// payload that is associated with the layer. This function is
    /// provided with the raw details of the node at the provided key.
    /// The function must return any piece of data that may later be
    /// retrieved via [layout_userdata].
    /// \remark Since MirAL 5.4
    void layout_userdata_builder(
        std::string const& key,
        std::function<std::any(Node const& value)> const& builder) const;

    void operator()(mir::Server& server) const;

    ~DisplayConfiguration();
    DisplayConfiguration(DisplayConfiguration const&);
    auto operator=(DisplayConfiguration const&) -> DisplayConfiguration&;

private:
    struct Self;
    std::shared_ptr<Self> self;
};
}

#endif //MIRAL_DISPLAY_CONFIGURATION_H
