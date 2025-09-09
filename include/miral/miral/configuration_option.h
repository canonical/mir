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

#ifndef MIRAL_CONFIGURATION_OPTION_H
#define MIRAL_CONFIGURATION_OPTION_H

#include <mir/optional_value.h>
#include <miral/lambda_as_function.h>

#include <functional>
#include <memory>
#include <string>

namespace mir { class Server; }

namespace miral
{
/// Add a user configuration option to Mir's option handling.
///
/// By default, the callback will be invoked following Mir initialization but
/// prior to the server starting. The value supplied to the callback will come
/// from the command line, environment variable, config file or the default.
///
/// \note Except for re-ordering implied by #miral::ConfigurationOption::pre_init()
/// the callbacks will be  invoked in the order supplied.
/// \remark Renamed from #miral::CommandLineOption in MirAL 3.6
class ConfigurationOption
{
public:
    /// Construct a configuration option for an `int` value.
    ///
    /// \param callback Callback when the value is received
    /// \param option Name of the option
    /// \param description Description of the option
    /// \param default_value Default value for the option if it is not provided
    ConfigurationOption(
        std::function<void(int value)> callback,
        std::string const& option,
        std::string const& description,
        int default_value);

    /// Construct a configuration option for a `double` value.
    ///
    /// \param callback Callback when the value is received
    /// \param option Name of the option
    /// \param description Description of the option
    /// \param default_value Default value for the option if it is not provided
    ConfigurationOption(
        std::function<void(double value)> callback,
        std::string const& option,
        std::string const& description,
        double default_value);

    /// Construct a configuration option for a `string` value.
    ///
    /// \param callback Callback when the value is received
    /// \param option Name of the option
    /// \param description Description of the option
    /// \param default_value Default value for the option if it is not provided
    ConfigurationOption(
        std::function<void(std::string const& value)> callback,
        std::string const& option,
        std::string const& description,
        std::string const& default_value);

    /// Construct a configuration option for a `string` value.
    ///
    /// \param callback Callback when the value is received
    /// \param option Name of the option
    /// \param description Description of the option
    /// \param default_value Default value for the option if it is not provided
    ConfigurationOption(
        std::function<void(std::string const& value)> callback,
        std::string const& option,
        std::string const& description,
        char const* default_value);

    /// Construct a configuration option for a `bool` value.
    ///
    /// \param callback Callback when the value is received
    /// \param option Name of the option
    /// \param description Description of the option
    /// \param default_value Default value for the option if it is not provided
    ConfigurationOption(
        std::function<void(bool value)> callback,
        std::string const& option,
        std::string const& description,
        bool default_value);

    /// Construct a configuration option for an optional `int` value.
    ///
    /// \param callback Callback when the value is received
    /// \param option Name of the option
    /// \param description Description of the option
    ConfigurationOption(
        std::function<void(mir::optional_value<int> const& value)> callback,
        std::string const& option,
        std::string const& description);

    /// Construct a configuration option for an optional `string` value.
    ///
    /// \param callback Callback when the value is received
    /// \param option Name of the option
    /// \param description Description of the option
    ConfigurationOption(
        std::function<void(mir::optional_value<std::string> const& value)> callback,
        std::string const& option,
        std::string const& description);

    /// Construct a configuration option for an optional `bool` value.
    ///
    /// \param callback Callback when the value is received
    /// \param option Name of the option
    /// \param description Description of the option
    ConfigurationOption(
        std::function<void(mir::optional_value<bool> const& value)> callback,
        std::string const& option,
        std::string const& description);

    /// Construct a configuration option for a `bool` value
    /// that will default to `false` if not provided.
    ///
    /// \param callback Callback when the value is received
    /// \param option Name of the option
    /// \param description Description of the option
    ConfigurationOption(
        std::function<void(bool is_set)> callback,
        std::string const& option,
        std::string const& description);

    /// Construct a configuration option for a `vector` of `string`s value.
    ///
    /// \param callback Callback when the value is received
    /// \param option Name of the option
    /// \param description Description of the option
    /// \remark Since MirAL 3.6
    ConfigurationOption(
        std::function<void(std::vector<std::string> const& values)> callback,
        std::string const& option,
        std::string const& description);

    // Construct a configuration option for a generic type value, given by `Lambda`.
    ///
    /// \param callback Callback when the value is received
    /// \param option Name of the option
    /// \param description Description of the option
    template<typename Lambda>
    ConfigurationOption(
            Lambda&& callback,
            std::string const& option,
            std::string const& description) :
            ConfigurationOption(lambda_as_function(std::forward<Lambda>(callback)), option, description) {}

    void operator()(mir::Server& server) const;

    // Call the callback *before* Mir initialization starts
    friend auto pre_init(ConfigurationOption const& clo) -> ConfigurationOption;

    ~ConfigurationOption();
    ConfigurationOption(ConfigurationOption const&);
    auto operator=(ConfigurationOption const&) -> ConfigurationOption&;

private:
    struct Self;
    std::shared_ptr<Self> self;
};

/// Update the option to be called back *before* Mir initialization starts
///
/// \param clo the option
/// \returns a configuration option
auto pre_init(ConfigurationOption const& clo) -> ConfigurationOption;
}

#endif //MIRAL_CONFIGURATION_OPTION_H
