#include "basic_store.h"

#include <mir/log.h>

#include <algorithm>
#include <format>
#include <list>
#include <map>
#include <mutex>
#include <ranges>
#include <set>

namespace mlc = miral::live_config;

class mlc::BasicStore::Self
{
public:
    Self() = default;

    void add_key(Key const& key, std::string_view description, std::optional<std::string> preset, HandleString handler);
    void add_key(Key const& key, std::string_view description, std::optional<std::vector<std::string>> preset, HandleStrings handler);

    void update_key(Key const& key, std::string_view value, std::filesystem::path const& modification_path);
    void do_transaction(std::function<void()> transaction_body);

    void on_done(HandleDone handler);

private:
    Self(Self const&) = delete;
    Self& operator=(Self const&) = delete;

    struct ScalarValue
    {
        std::string const value;
        std::filesystem::path const modification_path;
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
        std::set<std::filesystem::path> modification_paths;
    };

    std::mutex mutex;
    std::map<Key, AttributeDetails> attribute_handlers;
    std::map<Key, ArrayAttributeDetails> array_attribute_handlers;
    std::list<HandleDone> done_handlers;
};

void mlc::BasicStore::Self::add_key(
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

void mlc::BasicStore::Self::on_done(HandleDone handler)
{
    std::lock_guard lock{mutex};
    done_handlers.emplace_back(std::move(handler));
}

void mlc::BasicStore::Self::add_key(Key const& key, std::string_view description,
    std::optional<std::vector<std::string>> preset, HandleStrings handler)
{
    std::lock_guard lock{mutex};

    if (attribute_handlers.erase(key) || array_attribute_handlers.erase(key))
    {
        // if a key is registered multiple times, the last time is used: drop existing earlier registrations
        mir::log_warning("Config attribute handler for '%s' overwritten", key.to_string().c_str());
    }

    array_attribute_handlers.emplace(
        key, ArrayAttributeDetails{handler, std::string{description}, preset, std::vector<std::string>{}, {}});
}

void mlc::BasicStore::Self::update_key(Key const& key, std::string_view value, std::filesystem::path const& modification_path)
{
    if (auto const details_iter = attribute_handlers.find(key); details_iter != attribute_handlers.end())
    {
        auto& details = details_iter->second;
        details.value.emplace(ScalarValue{std::string{value}, modification_path});
    }
    else if (auto const details_iter = array_attribute_handlers.find(key); details_iter != array_attribute_handlers.end())
    {
        auto& details = details_iter->second;
        details.parsed_values.push_back(std::string{value});
        details.modification_paths.insert(modification_path);
    }
    else
    {
        mir::log_warning("Config key '%s' not recognized", key.to_string().c_str());
    }
}

void mlc::BasicStore::Self::do_transaction(std::function<void()> transaction_body)
{
    std::lock_guard lock{mutex};
    for (auto& [key, details] : attribute_handlers)
        details.value = std::nullopt;

    for (auto& [key, details] : array_attribute_handlers)
        details.parsed_values.resize(0);

    transaction_body();

    for (auto const& [key, details] : attribute_handlers)
    {
        auto const maybe_value = [&]() -> std::optional<std::string_view>
        {
            if (details.value)
                return details.value->value;
            else if (details.preset)
                return details.preset;
            else
                return std::nullopt;
        }();

        try
        {
            details.handler(key, maybe_value);
        }
        catch (std::exception const& e)
        {
            auto const path =
                details.value.transform([&](auto const& v) { return v.modification_path.string(); })
                    .value_or("never set");

            // `std::string` is required to get a `c_str` from a string view.
            auto const value_str = std::string{maybe_value.transform([](auto const& v) { return v; }).value_or("unset")};

            mir::log_warning(
                "Error processing key '%s' with value '%s' in file '%s': %s",
                key.to_string().c_str(),
                value_str.c_str(),
                path.c_str(),
                e.what());
        }
    }

    for (auto const& [key, details] : array_attribute_handlers)
    {
        auto const maybe_value = [&]() -> std::optional<std::vector<std::string>>
        {
            if (!details.parsed_values.empty())
                return details.parsed_values;
            else if (details.preset)
                return details.preset;
            else
                return std::nullopt;
        }();

        try
        {
            details.handler(key, maybe_value);
        }
        catch (std::exception const& e)
        {
            auto const path = [&]
            {
                // TODO Not all CI compilers support `std::ranges::to`
                auto comma_separated =
                    std::ranges::views::transform(details.modification_paths, [](auto const& p) { return p.string(); })
                    | std::views::join_with(',');

                std::string temp;
                std::ranges::copy(comma_separated, std::back_inserter(temp));

                return temp;
            }();

            auto const vector_to_string = [](auto const& v)
            {
                auto const comma_separated = v | std::views::join_with(',');

                std::string temp;
                std::ranges::copy(comma_separated, std::back_inserter(temp));

                return temp;
            };

            auto const value_str = maybe_value.transform(vector_to_string).value_or("unset");
            mir::log_warning(
                "Error processing key '%s' with values [%s] in file '%s': %s",
                key.to_string().c_str(),
                value_str.c_str(),
                path.c_str(),
                e.what());
        }
    }

    for (auto const& h : done_handlers)
        try
        {
            h();
        }
        catch (std::exception const& e)
        {
            mir::log_warning("Error processing done handlers: %s", e.what());
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
            throw std::runtime_error(std::format("Config key '{}' has invalid value: {}", key, *val));
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
            throw std::runtime_error(std::format("Config key '{}' has invalid value: {}", key, *val));
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

mlc::BasicStore::BasicStore() :
    self{std::make_shared<Self>()}
{
}

mlc::BasicStore::~BasicStore() = default;

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
    self->on_done(std::move(handler));
}

void mlc::BasicStore::update_key(Key const& key, std::string_view value, std::filesystem::path const& modification_path)
{
    self->update_key(key, value, modification_path);
}

void mlc::BasicStore::do_transaction(std::function<void()> transaction_body)
{
    self->do_transaction(std::move(transaction_body));
}
