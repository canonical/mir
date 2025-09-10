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

/// Enables loading the display configuration from a file.
///
/// The display configuration file name is provided by #miral::MirRunner::display_config_file.
/// Mir searches for this file in each of the following directories in order, per the
///  XDG Base Directory Specification:
///
/// 1. `$XDG_CONFIG_HOME` or $HOME/.config`
/// 2. `$XDG_CONFIG_DIRS`
///
/// Once a file is found, the remaining directories are not searched.
///
/// If no file is found, a default display configuration will be written to either
/// `$XDG_CONFIG_HOME` or `$HOME/.config`. Please see this default file for more
/// information about content expected from this project.
///
/// Mir will continue to monitor the configuration file as it runs.
///
/// Additionally, the selected layout of the file may be overridden using a corresponding
/// file: miral::MirRunner::display_config_file() + "-layout", which will also be monitored.
class DisplayConfiguration
{
public:
    /// A class providing access to an arbitrary piece of data from the #miral::DisplayConfiguration.
    ///
    /// This is specifically useful when a user wants to extend the basic display configuration
    /// with some sort of custom payload (e.g. a user may want to extend the layout configuration
    /// with information describing the position and size of specific applications).
    ///
    /// \remark Since MirAL 5.3
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

        /// The type of the node.
        ///
        /// Once resolved, the user can call the appropriate methods on the object.
        /// For example, if the type is #Type::string, users can safely call #as_string.
        ///
        /// \returns the node type
        auto type() const -> Type;

        /// Returns the content of the node as a `std::string`.
        ///
        /// If the node does not have a #type of #Type::string, this will
        /// cause a fatal error.
        ///
        /// \returns the node as a string
        auto as_string() const -> std::string;

        // Returns the content of the node as an `int`.
        ///
        /// If the node does not have a #type of #Type::integer, this will
        /// cause a fatal error.
        ///
        /// \returns the node as an integer
        auto as_int() const -> int;

        /// Iterate over the children of this node.
        ///
        /// If the node does not have a #type of #Type::sequence, this will
        /// cause a fatal error.
        ///
        /// \param f function to all on each child node
        void for_each(std::function<void(Node const&)> const& f) const;

        /// Check if the node has a value at the given key.
        ///
        /// If the node does not have a #type of #Type::map, this will
        /// cause a fatal error.
        ///
        /// \param key the key to check for existence
        /// \returns `true` if the key exists, otherwise `false`
        auto has(std::string const& key) const -> bool;

        /// Get the node at the given key.
        ///
        /// If the node does not have a #type of #Type::map, this will
        /// cause a fatal error.
        ///
        /// \param key the key to get
        /// \returns the node at the key, or `std::nullopt` if none exists
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

    /// Construct the display configuration.
    ///
    /// The configuration file will be loaded from the provided mir runner via
    /// #miral::MirRunner::display_config_file.
    ///
    /// The configuration is loaded by #DisplayConfiguration::operator()(mir::Server&).
    ///
    /// \param mir_runner the mir runner
    explicit DisplayConfiguration(MirRunner const& mir_runner);

    /// Create a `--display-layout` configuration option.
    ///
    /// Note that this must be provided to #miral::MirRunner::run_with in order to
    /// work.
    ///
    /// \returns a new configuration option
    auto layout_option() -> ConfigurationOption;

    /// Select a layout from the configuration.
    ///
    /// If the \p layout is not present in the configuration, the 'default'
    /// layout is applied.
    ///
    /// \param layout the desired layout
    void select_layout(std::string const& layout);

    /// List the valid layouts from the configuration file.
    ///
    /// \returns the list of valid layouts
    auto list_layouts() -> std::vector<std::string>;

    /// Enable a custom output attribute in the configuration file.
    ///
    /// The value of this attribute will be made available through
    /// #miral::UserDisplayConfigurationOutput::custom_attribute at the
    /// \p key.
    ///
    /// The value in the YAML file is expected to be a string.
    ///
    /// \param key the key to enable
    /// \remark Since MirAL 3.8
    void add_output_attribute(std::string const& key);

    /// Retrieve the user data associated with the active layout
    /// for the provided \p key.
    ///
    /// Callers should provide this user data via
    /// #miral::DisplayConfiguration::layout_userdata_builder.
    ///
    /// \param key the key to retrieve
    /// \returns Arbitrary userdata, if any
    /// \remark Since MirAL 5.3
    auto layout_userdata(std::string const& key) const -> std::optional<std::any const>;

    /// Enable a custom layout attribute in the configuration file.
    ///
    /// The caller must provide the \p key of this custom attribute in
    /// addition to a \p builder function that will be used to build the custom
    /// payload that is associated with the layout. This function is
    /// provided with the raw details of the node at the provided key.
    ///
    /// The function can return any piece of data. This data may later be
    /// retrieved via #miral::DisplayConfiguration::layout_userdata.
    ///
    /// \param key the key for the data
    /// \param builder a builder function that takes the content of the node
    ///                at the key as a parameter and returns an arbitrary payload
    /// \remark Since MirAL 5.3
    /// \sa miral::DisplayConfiguration::Node - the node provided as a parameter to the builder
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
