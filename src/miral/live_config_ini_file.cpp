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

#include "miral/live_config_ini_file.h"

#include <mir/log.h>

#include <charconv>
#include <format>
#include <list>
#include <map>

namespace mlc = miral::live_config;

class mlc::IniFile::Self
{
public:
    Self() = default;

    void add_key(Key const& key, std::string_view description, std::optional<std::string> preset, HandleString handler);
    void add_key(Key const& key, std::string_view description, std::optional<std::vector<std::string>> preset, HandleStrings handler);

    void load_file(std::istream& istream, std::filesystem::path const& path);
    void on_done(HandleDone handler);

private:
    Self(Self const&) = delete;
    Self& operator=(Self const&) = delete;

    struct AttributeDetails
    {
        HandleString const handler;
        std::string const description;
        std::optional<std::string> const preset;
        std::optional<std::string> value;
    };

    struct ArrayAttributeDetails
    {
        HandleStrings const handler;
        std::string const description;
        std::optional<std::vector<std::string>> const preset;
        std::optional<std::vector<std::string>> value;
    };

    std::mutex mutex;
    std::map<Key, AttributeDetails> attribute_handlers;
    std::map<Key, ArrayAttributeDetails> array_attribute_handlers;
    std::list<HandleDone> done_handlers;
};

void mlc::IniFile::Self::add_key(
    Key const& key,
    std::string_view description,
    std::optional<std::string> preset,
    HandleString handler)
{
    std::lock_guard lock{mutex};

    if (attribute_handlers.erase(key) || array_attribute_handlers.erase(key))
    {
        // if a key is registered multiple times, the last time is used: drop existing earlier registrations
        mir::log_warning("Config attribute handler for '%s' overwritten", key.to_string().c_str());
    }

    attribute_handlers.emplace(key, AttributeDetails{handler, std::string{description}, preset, std::nullopt});
}

void miral::live_config::IniFile::Self::on_done(HandleDone handler)
{
    std::lock_guard lock{mutex};
    done_handlers.emplace_back(std::move(handler));
}

void mlc::IniFile::Self::add_key(Key const& key, std::string_view description,
    std::optional<std::vector<std::string>> preset, HandleStrings handler)
{
    std::lock_guard lock{mutex};

    if (attribute_handlers.erase(key) || array_attribute_handlers.erase(key))
    {
        // if a key is registered multiple times, the last time is used: drop existing earlier registrations
        mir::log_warning("Config attribute handler for '%s' overwritten", key.to_string().c_str());
    }

    array_attribute_handlers.emplace(key, ArrayAttributeDetails{handler, std::string{description}, preset, std::nullopt});
}

void mlc::IniFile::Self::load_file(std::istream& istream, std::filesystem::path const& path)
{
    std::lock_guard lock{mutex};

    for (auto& [key, details] : attribute_handlers)
    {
        details.value = std::nullopt;
    }

    for (auto& [key, details] : array_attribute_handlers)
    {
        details.value = std::nullopt;
    }

    for (std::string line; std::getline(istream, line);)
    {
        if (!line.starts_with('#') && line.contains("="))
        try
        {
            auto const eq = line.find_first_of("=");
            auto const key = Key{line.substr(0, eq)};
            auto const value = line.substr(eq+1);

            if (auto const details = attribute_handlers.find(key); details != attribute_handlers.end())
            {
                details->second.value = value;
            }
            else if (auto const details = array_attribute_handlers.find(key); details != array_attribute_handlers.end())
            {
                if (details->second.value)
                {
                    details->second.value.value().push_back(value);
                }
                else
                {
                    details->second.value = std::vector<std::string>{value};
                }
            }
        }
        catch (const std::exception& e)
        {
            mir::log_warning("Error processing '%s': %s", path.c_str(), e.what());
        }
    }

    for (auto const& [key, details] : attribute_handlers)
    try
    {
        details.handler(key, (details.value ? details.value : details.preset));
    }
    catch (const std::exception& e)
    {
        mir::log_warning("Error processing '%s': %s", path.c_str(), e.what());
    }

    for (auto const& [key, details] : array_attribute_handlers)
    try
    {
        details.handler(key, (details.value ? details.value : details.preset));
    }
    catch (const std::exception& e)
    {
        mir::log_warning("Error processing '%s': %s", path.c_str(), e.what());
    }

    for (auto const& h : done_handlers)
    try
    {
        h();
    }
    catch (const std::exception& e)
    {
        mir::log_warning("Error processing '%s': %s", path.c_str(), e.what());
    }
}

namespace
{
template<typename Type>
void process_as(std::function<void(mlc::Key const&, std::optional<Type>)> const& handler, mlc::Key const& key, std::optional<std::string_view> val)
{
    if (val)
    {
        Type parsed_val{};

        auto const [end, err] = std::from_chars(val->data(), val->data() + val->size(), parsed_val);

        if ((err == std::errc{}) && (end ==val->data() + val->size()))
        {
            handler(key, parsed_val);
        }
        else
        {
            throw std::runtime_error(std::format("Config key '{}' has invalid value: {}", key.to_string(), *val));
        }
    }
    else
    {
        handler(key, std::nullopt);
    }
}

template<>
void process_as<bool>(std::function<void(mlc::Key const&, std::optional<bool>)> const& handler, mlc::Key const& key, std::optional<std::string_view> val)
{
    if (val)
    {
        if (*val == "true")
        {
            handler(key, true);
        }
        else if (*val == "false")
        {
            handler(key, false);
        }
        else
        {
            throw std::runtime_error(std::format("Config key '{}' has invalid value: {}", key.to_string(), *val));
        }
    }
    else
    {
        handler(key, std::nullopt);
    }
}

template<typename Type>
void process_as(std::function<void(mlc::Key const&, std::optional<std::span<Type>>)> const& handler, mlc::Key const& key,
    std::optional<std::span<std::string const>> val)
{
    if (val)
    {
        std::vector<Type> parsed_vals;

        for (auto const& v : *val)
        {
            Type parsed_val{};
            auto const [end, err] = std::from_chars(v.data(), v.data() + v.size(), parsed_val);

            if ((err == std::errc{}) && (end == v.data() + v.size()))
            {
                parsed_vals.push_back(parsed_val);
            }
            else
            {
                mir::log_warning("Config key '%s' has invalid value: %s", key.to_string().c_str(), v.c_str());
            }
        }

        handler(key, parsed_vals);
    }
    else
    {
        handler(key, std::nullopt);
    }
}
}

mlc::IniFile::IniFile() :
    self{std::make_shared<Self>()}
{
}

mlc::IniFile::~IniFile() = default;

void mlc::IniFile::add_int_attribute(Key const& key, std::string_view description, HandleInt handler)
{
    self->add_key(key, description, std::nullopt,
        [handler](live_config::Key const& key, std::optional<std::string_view> val)
    {
        process_as<int>(handler, key, val);
    });
}

void mlc::IniFile::add_ints_attribute(Key const& key, std::string_view description, HandleInts handler)
{
    self->add_key(key, description, std::nullopt, [handler](Key const& key, std::optional<std::span<std::string const>> val)
    {
        process_as<int>(handler, key, val);
    });
}

void mlc::IniFile::add_bool_attribute(Key const& key, std::string_view description, HandleBool handler)
{
    self->add_key(key, description, std::nullopt,
        [handler](live_config::Key const& key, std::optional<std::string_view> val)
    {
        process_as<bool>(handler, key, val);
    });
}

void mlc::IniFile::add_float_attribute(Key const& key, std::string_view description, HandleFloat handler)
{
    self->add_key(key, description, std::nullopt,
        [handler](live_config::Key const& key, std::optional<std::string_view> val)
    {
        process_as<float>(handler, key, val);
    });
}

void mlc::IniFile::add_floats_attribute(Key const& key, std::string_view description, HandleFloats handler)
{
    self->add_key(key, description, std::nullopt, [handler](Key const& key, std::optional<std::span<std::string const>> val)
    {
        process_as<float>(handler, key, val);
    });
}

void mlc::IniFile::add_string_attribute(Key const& key, std::string_view description, HandleString handler)
{
    self->add_key(key, description, std::nullopt, handler);
}

void mlc::IniFile::add_strings_attribute(Key const& key, std::string_view description, HandleStrings handler)
{
    self->add_key(key, description, std::nullopt, handler);
}

void mlc::IniFile::add_int_attribute(Key const& key, std::string_view description, int preset, HandleInt handler)
{
    self->add_key(key, description, std::to_string(preset),
        [handler](live_config::Key const& key, std::optional<std::string_view> val)
        {
            process_as<int>(handler, key, val);
        });
}

void mlc::IniFile::add_ints_attribute(Key const& key, std::string_view description, std::span<int const> preset, HandleInts handler)
{
    std::vector<std::string> str_preset;
    str_preset.reserve(preset.size());
    for (auto const& p : preset)
    {
        str_preset.emplace_back(std::to_string(p));
    }

    self->add_key(key, description, std::move(str_preset), [handler](Key const& key, std::optional<std::span<std::string const>> val)
    {
        process_as<int>(handler, key, val);
    });
}

void mlc::IniFile::add_bool_attribute(Key const& key, std::string_view description, bool preset, HandleBool handler)
{
    self->add_key(key, description, (preset ? "true" : "false"),
        [handler](live_config::Key const& key, std::optional<std::string_view> val)
        {
            process_as<bool>(handler, key, val);
        });
}

void mlc::IniFile::add_float_attribute(Key const& key, std::string_view description, float preset, HandleFloat handler)
{
    self->add_key(key, description, std::to_string(preset),
        [handler](live_config::Key const& key, std::optional<std::string_view> val)
    {
        process_as<float>(handler, key, val);
    });
}

void mlc::IniFile::add_floats_attribute(Key const& key, std::string_view description, std::span<float const> preset, HandleFloats handler)
{
    std::vector<std::string> str_preset;
    str_preset.reserve(preset.size());
    for (auto const& p : preset)
    {
        str_preset.emplace_back(std::to_string(p));
    }

    self->add_key(key, description, std::move(str_preset), [handler](Key const& key, std::optional<std::span<std::string const>> val)
    {
        process_as<float>(handler, key, val);
    });
}

void mlc::IniFile::add_string_attribute(Key const& key, std::string_view description, std::string_view preset, HandleString handler)
{
    self->add_key(key, description, std::string{preset}, handler);
}

void mlc::IniFile::add_strings_attribute(Key const& key, std::string_view description, std::span<std::string const> preset, HandleStrings handler)
{
    self->add_key(key, description, std::vector<std::string>{preset.begin(), preset.end()}, handler);
}

void mlc::IniFile::on_done(HandleDone handler)
{
    self->on_done(std::move(handler));
}

void mlc::IniFile::load_file(std::istream& istream, std::filesystem::path const& path)
{
    self->load_file(istream, path);
}
