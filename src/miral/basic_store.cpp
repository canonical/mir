#include "basic_store.h"

#include <mir/log.h>

#include <boost/throw_exception.hpp>

#include <format>
#include <list>
#include <map>
#include <mutex>
#include <ranges>
#include <set>
#include <stdexcept>

namespace mlc = miral::live_config;

namespace
{
template <typename Range> auto join_comma(Range const& strings) -> std::string
{
    auto const sep = std::string_view{", "};
    auto comma_separated = strings | std::views::join_with(sep);

    // LEGACY(24.04) - `std::ranges::to` is not yet widely available, so we need to copy the result into a string
    // manually if it's not available. Remove this workaround once we drop 24.04 support.
#ifdef __cpp_lib_ranges_to_container
    return comma_separated | std::ranges::to<std::string>();
#else
    std::string temp;
    std::ranges::copy(comma_separated, std::back_inserter(temp));

    return temp;
#endif
}

class ParsingError : public std::runtime_error
{
public:
    explicit ParsingError(mlc::Key const& key, std::string_view value) :
        std::runtime_error(std::format("Config key '{}' has invalid value: {}", key, value))
    {
    }
};

class NoValidValuesError : public std::runtime_error
{
public:
    explicit NoValidValuesError(mlc::Key const& key, std::span<std::string const> values) :
        std::runtime_error(
            std::format("All values assigned to config key '{}' are invalid ([{}]).", key, join_comma(values)))
    {
    }
};
}

class mlc::BasicStore::Self
{
public:
    Self() = default;

    void add_key(Key const& key, std::string_view description, std::optional<std::string> preset, HandleString handler);
    void add_key(Key const& key, std::string_view description, std::optional<std::vector<std::string>> preset, HandleStrings handler);

    void update_key(Key const& key, std::string_view value, std::filesystem::path const& modification_path);
    template <typename T> void set_value(Key const& key, std::span<T const> values)
    {
        if (auto array_iter = array_attribute_handlers.find(key); array_iter != array_attribute_handlers.end())
        {
            auto& details = array_iter->second;
            auto const str_values = values | std::views::transform([](auto const& v) -> std::string {
                if constexpr (!std::is_same_v<T, std::string>) {
                    return std::to_string(v);
                } else {
                    return v;
                }
            });
            details.parsed_values = std::vector<std::string>{str_values.begin(), str_values.end()};
        }
    }

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
    {
        details.parsed_values.resize(0);
        details.modification_paths.clear();
    }

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
        catch (ParsingError const& pe)
        {
            auto const path = details.value.transform([&](auto const& v) { return v.modification_path.string(); })
                                  .value_or("never set");

            if (auto const preset = details.preset)
            {
                mir::log_warning(
                    "Parsing error: %s in file %s. Using preset value '%s' instead.",
                    pe.what(),
                    path.c_str(),
                    preset->c_str());

                details.handler(key, details.preset);
            }
            else
            {
                mir::log_warning(
                    "Parsing error: %s in file %s, but no preset value. Using nullopt instead.",
                    pe.what(),
                    path.c_str());
                details.handler(key, std::nullopt);
            }
        }
        catch (std::exception const& e)
        {
            auto const path = details.value.transform([&](auto const& v) { return v.modification_path.string(); })
                                  .value_or("never set");
            // `std::string` is required to get a `c_str` from a string view.
            auto const value_str = std::string{maybe_value.value_or("unset")};

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
        catch (NoValidValuesError const& nvv)
        {
            auto const path = [&]
            {
                if (details.modification_paths.empty())
                    return std::string{"never set"};

                return join_comma(
                    details.modification_paths | std::views::transform([](auto const& p) { return p.string(); }));
            }();

            if (auto const preset = details.preset)
            {
                auto const preset_str = join_comma(*details.preset);

                mir::log_warning(
                    "Parsing error: %s in file %s. Using preset value(s) '[%s]' instead.",
                    nvv.what(),
                    path.c_str(),
                    preset_str.c_str());

                details.handler(key, preset);
            }
            else
            {
                mir::log_warning(
                    "Parsing error: %s in file %s, but no preset value. Using nullopt instead.",
                    nvv.what(),
                    path.c_str());
                details.handler(key, std::nullopt);
            }
        }
        catch (std::exception const& e)
        {
            auto const path = [&]
            {
                if (details.modification_paths.empty())
                    return std::string{"never set"};

                return join_comma(
                    std::ranges::views::transform(details.modification_paths, [](auto const& p) { return p.string(); }));
            }();
            auto const value_str = maybe_value.transform([](auto const& v) { return join_comma(v); }).value_or("unset");

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
            BOOST_THROW_EXCEPTION(ParsingError(key, *val));
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
            BOOST_THROW_EXCEPTION(ParsingError(key, *val));
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

        if (!val->empty() && parsed_vals.empty())
        {
            BOOST_THROW_EXCEPTION(NoValidValuesError(key, *val));
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

void mlc::BasicStore::set_ints_value(Key const& key, std::span<int const> values)
{
    self->set_value(key, values);
}

void mlc::BasicStore::set_floats_value(Key const& key, std::span<float const> values)
{
    self->set_value(key, values);
}

void mlc::BasicStore::set_strings_value(Key const& key, std::span<std::string const> values)
{
    self->set_value(key, values);
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
