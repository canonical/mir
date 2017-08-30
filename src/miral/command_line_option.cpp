/*
 * Copyright © 2016 Canonical Ltd.
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
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "miral/command_line_option.h"

#include <mir/server.h>
#include <mir/options/option.h>

namespace
{
template<typename Type>
struct OptionType;

template<>
struct OptionType<bool>
{
    auto static constexpr value = mir::OptionType::boolean;
};

template<>
struct OptionType<void>
{
    auto static constexpr value = mir::OptionType::null;
};

template<>
struct OptionType<std::string>
{
    auto static constexpr value = mir::OptionType::string;
};

template<>
struct OptionType<int>
{
    auto static constexpr value = mir::OptionType::integer;
};
}

struct miral::CommandLineOption::Self
{
    template<typename Value_t>
    Self(std::function<void(Value_t value)> callback,
         std::string const& option,
         std::string const& description,
         Value_t default_value) :
        setup{[=](mir::Server& server)
                  { server.add_configuration_option(option, description, default_value); }},
        callback{[=](mir::Server& server)
                     { callback(server.get_options()->get<Value_t>(option.c_str())); }}
    {
    }

    template<typename Value_t>
    Self(std::function<void(Value_t const& value)> callback,
         std::string const& option,
         std::string const& description,
         Value_t const& default_value) :
        setup{[=](mir::Server& server)
                  { server.add_configuration_option(option, description, default_value); }},
        callback{[=](mir::Server& server)
                     { callback(server.get_options()->get<Value_t>(option.c_str())); }}
    {
    }

    template<typename Value_t>
    Self(std::function<void(mir::optional_value<Value_t> const& value)> callback,
         std::string const& option,
         std::string const& description) :
        setup{[=](mir::Server& server)
                  { server.add_configuration_option(option, description, OptionType<Value_t>::value); }},
        callback{[=](mir::Server& server)
                     {
                        mir::optional_value<Value_t> optional_value;
                        auto const options = server.get_options();
                        if (options->is_set(option.c_str()))
                            optional_value = server.get_options()->get<Value_t>(option.c_str());
                        callback(optional_value);
                     }}
    {
    }

    Self(std::function<void(bool is_set)> callback,
         std::string const& option,
         std::string const& description) :
        setup{[=](mir::Server& server)
                  { server.add_configuration_option(option, description, OptionType<void>::value); }},
        callback{[=](mir::Server& server)
                 {
                     auto const options = server.get_options();
                     callback(options->is_set(option.c_str()));
                 }}
    {
    }

    bool pre_init{false};
    std::function<void(mir::Server& server)> setup;
    std::function<void(mir::Server& server)> callback;
};

miral::CommandLineOption::CommandLineOption(
    std::function<void(int value)> callback,
    std::string const& option,
    std::string const& description,
    int default_value) :
    self{std::make_shared<Self>(callback, option, description, default_value)}
{
}

miral::CommandLineOption::CommandLineOption(
    std::function<void(double value)> callback,
    std::string const& option,
    std::string const& description,
    double default_value) :
    self{std::make_shared<Self>(callback, option, description, default_value)}
{
}

miral::CommandLineOption::CommandLineOption(
    std::function<void(std::string const& value)> callback,
    std::string const& option,
    std::string const& description,
    std::string const& default_value) :
    self{std::make_shared<Self>(callback, option, description, default_value)}
{
}

miral::CommandLineOption::CommandLineOption(
    std::function<void(std::string const& value)> callback,
    std::string const& option,
    std::string const& description,
    char const* default_value) :
    self{std::make_shared<Self>(callback, option, description, std::string{default_value})}
{
}

miral::CommandLineOption::CommandLineOption(
    std::function<void(bool value)> callback,
    std::string const& option,
    std::string const& description,
    bool default_value) :
    self{std::make_shared<Self>(callback, option, description, default_value)}
{
}

miral::CommandLineOption::CommandLineOption(
    std::function<void(mir::optional_value<int> const& value)> callback,
    std::string const& option,
    std::string const& description) :
    self{std::make_shared<Self>(callback, option, description)}
{
}

miral::CommandLineOption::CommandLineOption(
    std::function<void(mir::optional_value<std::string> const& value)> callback,
    std::string const& option,
    std::string const& description) :
    self{std::make_shared<Self>(callback, option, description)}
{
}

miral::CommandLineOption::CommandLineOption(
    std::function<void(mir::optional_value<bool> const& value)> callback,
    std::string const& option,
    std::string const& description) :
    self{std::make_shared<Self>(callback, option, description)}
{
}

miral::CommandLineOption::CommandLineOption(
    std::function<void(bool is_set)> callback,
    std::string const& option,
    std::string const& description) :
    self{std::make_shared<Self>(callback, option, description)}
{
}

auto miral::pre_init(CommandLineOption const& clo) -> CommandLineOption
{
    clo.self->pre_init = true;
    return clo;
}

void miral::CommandLineOption::operator()(mir::Server& server) const
{
    self->setup(server);

    if (self->pre_init)
        server.add_pre_init_callback([&]{ self->callback(server); });
    else
        server.add_init_callback([&]{ self->callback(server); });
}

miral::CommandLineOption::~CommandLineOption() = default;
miral::CommandLineOption::CommandLineOption(CommandLineOption const&) = default;
auto miral::CommandLineOption::operator=(CommandLineOption const&) -> CommandLineOption& = default;
