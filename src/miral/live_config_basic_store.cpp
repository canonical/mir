#include "miral/live_config_basic_store.h"

#include <charconv>
#include <format>
#include <mir/log.h>

#include <algorithm>
#include <filesystem>
#include <list>
#include <map>
#include <ranges>

namespace mlc = miral::live_config;

struct miral::live_config::BasicStore::Self
{
public:
    Self() = default;

    void add_key(Key const& key, std::string_view description, std::optional<std::string> preset, HandleString handler);
    void add_key(
        Key const& key,
        std::string_view description,
        std::optional<std::vector<std::string>> preset,
        HandleStrings handler);

    struct ScalarValue
    {
        std::string string_value;
        std::filesystem::path path;
    };

    struct AttributeDetails
    {
        HandleString const handler;
        std::string const description;
        std::optional<std::string> const preset;
        std::optional<ScalarValue> value;
    };

    struct ArrayAttributeDetails
    {
        HandleStrings const handler;
        std::string const description;
        std::optional<std::vector<std::string>> const preset;
        std::vector<std::string> parsed_values;
        std::vector<std::filesystem::path> modification_locations;
        bool explicitly_cleared = false;
    };

    class ConfigState
    {
    public:
        auto add_scalar_attribute(Key const& key, AttributeDetails details) -> void
        {
            if (attribute_handlers.erase(key) || array_attribute_handlers.erase(key))
            {
                // if a key is registered multiple times, the last time is used: drop existing earlier registrations
                mir::log_warning("Config attribute handler for '%s' overwritten", key.to_string().c_str());
            }
            attribute_handlers.emplace(key, std::move(details));
        }

        void clear()
        {
            for (auto& [_, details] : attribute_handlers)
                details.value = std::nullopt;

            for (auto& [_, details] : array_attribute_handlers)
            {
                details.parsed_values.resize(0);
                details.modification_locations.resize(0);
                details.explicitly_cleared = false;
            }
        }

        bool is_scalar(Key const& key) const
        {
            return attribute_handlers.contains(key);
        }

        void update_scalar_value(Key const& key, std::string_view value, std::filesystem::path const& path)
        {
            auto& details = attribute_handlers.at(key);
            details.value = ScalarValue{std::string{value}, path};
        }

        bool is_array(Key const& key) const
        {
            return array_attribute_handlers.contains(key);
        }

        void update_array_value(Key const& key, std::string_view value, std::filesystem::path const& path)
        {
            auto& details = array_attribute_handlers.at(key);
            auto& parsed_values = details.parsed_values;
            auto& modification_locations = details.modification_locations;

            if (!std::ranges::contains(modification_locations, path))
                modification_locations.push_back(path);

            if (value.empty())
            {
                parsed_values.resize(0);
                details.explicitly_cleared = true;

                // Signal to the handler that the array was cleared. This is to fix the case
                // where we clear then append in the same file. The append would overwrite the
                // clear, and the client would recieve that as an append.
                details.handler(key, std::span<std::string const>{});
            }
            else
            {
                parsed_values.push_back(std::string{value});
                details.explicitly_cleared = false;
            }
        }

        void call_attribute_handlers() const
        {
            for (auto const& [key, details] : attribute_handlers)
                try
                {
                    auto const value = [&details] -> std::optional<std::string_view>
                    {
                        if (details.value)
                            return details.value->string_value;
                        else if (details.preset)
                            // string -> string_view -> optional<string_view>
                            return *details.preset;

                        return std::nullopt;
                    }();

                    details.handler(key, value);
                }
                catch (std::exception const& e)
                {
                    mir::log_warning(
                        "Error processing scalar '%s' with value '%s' set in file '%s': %s",
                        key.to_string().c_str(),
                        details.value ? details.value->string_value.c_str() : "",
                        details.value ? details.value->path.c_str() : "",
                        e.what());
                }

            for (auto const& [key, details] : array_attribute_handlers)
                try
                {
                    // If the user didn't explicitly clear the array and didn't provide any
                    // values, then we use the preset values.
                    if (!details.explicitly_cleared && details.parsed_values.empty())
                        details.handler(key, details.preset);
                    else
                        details.handler(key, details.parsed_values);
                }
                catch (std::exception const& e)
                {
                    // Unfortunately, the GCC version used in CI does not support std::ranges::to...
                    auto const array_values = details.parsed_values | std::views::join_with(',');
                    auto const array_values_str = std::string{array_values.begin(), array_values.end()};

                    std::string modification_locations_str;
                    std::ranges::copy(
                        details.modification_locations | std::views::transform([](auto const& p) { return p.string(); })
                            | std::views::join_with(','),
                        std::back_inserter(modification_locations_str));

                    mir::log_warning(
                        "Error processing array '%s' with values [%s] modified in files [%s]: %s",
                        key.to_string().c_str(),
                        array_values_str.c_str(),
                        modification_locations_str.c_str(),
                        e.what());
                }
        }

        void add_array_attribute(Key const& key, ArrayAttributeDetails details)
        {
            if (attribute_handlers.erase(key) || array_attribute_handlers.erase(key))
            {
                // if a key is registered multiple times, the last time is used: drop existing earlier registrations
                mir::log_warning("Config attribute handler for '%s' overwritten", key.to_string().c_str());
            }
            array_attribute_handlers.emplace(key, std::move(details));
        }

        void on_done(HandleDone handler)
        {
            done_handlers.push_back(std::move(handler));
        }

        void call_done_handlers(std::string const& paths) const
        {
            for (auto const& h : done_handlers)
                try
                {
                    h();
                }
                catch (std::exception const& e)
                {
                    mir::log_warning("Error processing file(s) [%s]: %s", paths.c_str(), e.what());
                }
        }

    private:
        std::map<Key, AttributeDetails> attribute_handlers;
        std::map<Key, ArrayAttributeDetails> array_attribute_handlers;
        std::list<HandleDone> done_handlers;
    };

    std::mutex mutex; // TODO locking
    ConfigState config_state;
};

mlc::BasicStore::BasicStore() :
    self{std::make_shared<Self>()}
{
}

void mlc::BasicStore::Self::add_key(Key const& key, std::string_view description, std::optional<std::string> preset, HandleString handler)
{
    std::lock_guard lock{mutex};
    config_state.add_scalar_attribute(key,  AttributeDetails{handler, std::string{description}, preset, std::nullopt});
}

void mlc::BasicStore::Self::add_key(
    Key const& key, std::string_view description, std::optional<std::vector<std::string>> preset, HandleStrings handler)
{
    std::lock_guard lock{mutex};
    config_state.add_array_attribute(
        key, ArrayAttributeDetails{handler, std::string{description}, preset, {}, {}, false});
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

void mlc::BasicStore::add_int_attribute(Key const& key, std::string_view description, HandleInt handler)
{
    self->add_key(key, description, std::nullopt,
        [handler](live_config::Key const& key, std::optional<std::string_view> val)
    {
        process_as<int>(handler, key, val);
    });
}

void mlc::BasicStore::add_ints_attribute(Key const& key, std::string_view description, HandleInts handler)
{
    self->add_key(key, description, std::nullopt, [handler](Key const& key, std::optional<std::span<std::string const>> val)
    {
        process_as<int>(handler, key, val);
    });
}

void mlc::BasicStore::add_bool_attribute(Key const& key, std::string_view description, HandleBool handler)
{
    self->add_key(key, description, std::nullopt,
        [handler](live_config::Key const& key, std::optional<std::string_view> val)
    {
        process_as<bool>(handler, key, val);
    });
}

void mlc::BasicStore::add_float_attribute(Key const& key, std::string_view description, HandleFloat handler)
{
    self->add_key(key, description, std::nullopt,
        [handler](live_config::Key const& key, std::optional<std::string_view> val)
    {
        process_as<float>(handler, key, val);
    });
}

void mlc::BasicStore::add_floats_attribute(Key const& key, std::string_view description, HandleFloats handler)
{
    self->add_key(key, description, std::nullopt, [handler](Key const& key, std::optional<std::span<std::string const>> val)
    {
        process_as<float>(handler, key, val);
    });
}

void mlc::BasicStore::add_string_attribute(Key const& key, std::string_view description, HandleString handler)
{
    self->add_key(key, description, std::nullopt, handler);
}

void mlc::BasicStore::add_strings_attribute(Key const& key, std::string_view description, HandleStrings handler)
{
    self->add_key(key, description, std::nullopt, handler);
}

void mlc::BasicStore::add_int_attribute(Key const& key, std::string_view description, int preset, HandleInt handler)
{
    self->add_key(key, description, std::to_string(preset),
        [handler](live_config::Key const& key, std::optional<std::string_view> val)
        {
            process_as<int>(handler, key, val);
        });
}

void mlc::BasicStore::add_ints_attribute(Key const& key, std::string_view description, std::span<int const> preset, HandleInts handler)
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

void mlc::BasicStore::add_bool_attribute(Key const& key, std::string_view description, bool preset, HandleBool handler)
{
    self->add_key(key, description, (preset ? "true" : "false"),
        [handler](live_config::Key const& key, std::optional<std::string_view> val)
        {
            process_as<bool>(handler, key, val);
        });
}

void mlc::BasicStore::add_float_attribute(Key const& key, std::string_view description, float preset, HandleFloat handler)
{
    self->add_key(key, description, std::to_string(preset),
        [handler](live_config::Key const& key, std::optional<std::string_view> val)
    {
        process_as<float>(handler, key, val);
    });
}

void mlc::BasicStore::add_floats_attribute(Key const& key, std::string_view description, std::span<float const> preset, HandleFloats handler)
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

void mlc::BasicStore::add_string_attribute(Key const& key, std::string_view description, std::string_view preset, HandleString handler)
{
    self->add_key(key, description, std::string{preset}, handler);
}

void mlc::BasicStore::add_strings_attribute(Key const& key, std::string_view description, std::span<std::string const> preset, HandleStrings handler)
{
    self->add_key(key, description, std::vector<std::string>{preset.begin(), preset.end()}, handler);
}

void mlc::BasicStore::on_done(HandleDone handler)
{
    self->config_state.on_done(std::move(handler));
}

bool mlc::BasicStore::is_scalar(Key const& key) const
{
    return self->config_state.is_scalar(key);
}

bool mlc::BasicStore::is_array(Key const& key) const
{
    return self->config_state.is_array(key);
}

void mlc::BasicStore::update_scalar_value(Key const& key, std::string_view value, std::filesystem::path const& path)
{
    self->config_state.update_scalar_value(key, value, path);
}

void mlc::BasicStore::update_array_value(Key const& key, std::string_view value, std::filesystem::path const& path)
{
    self->config_state.update_array_value(key, value, path);
}

void mlc::BasicStore::clear_values()
{
    self->config_state.clear();
}

void mlc::BasicStore::call_attribute_handlers() const
{
    self->config_state.call_attribute_handlers();
}

void mlc::BasicStore::call_done_handlers(std::string const& paths) const
{
    self->config_state.call_done_handlers(paths);
}
