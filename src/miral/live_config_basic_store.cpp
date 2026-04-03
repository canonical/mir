/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

#include "live_config_basic_store.h"

#include <charconv>
#include <format>
#include <mir/log.h>

#include <algorithm>
#include <filesystem>
#include <list>
#include <map>
#include <ranges>

namespace mlc = miral::live_config;

namespace
{
using TypedScalar = mlc::BasicStore::TypedScalar;
using TypedArray = mlc::BasicStore::TypedArray;

struct ScalarValue
{
    TypedScalar typed_value;
    std::filesystem::path path;

};

struct AttributeDetails
{
    // Called at dispatch time with the stored typed value (or nullopt / preset).
    // Wraps the user's typed handler and performs the variant → T extraction.
    std::function<void(mlc::Key const&, std::optional<TypedScalar> const&)> dispatch;

    // Parses a raw string from a config file and stores it as a TypedScalar.
    // Used by the IniFile path (update_scalar_value).
    std::function<TypedScalar(std::string_view, mlc::Key const&)> parse;

    std::string const description;
    std::optional<TypedScalar> const preset;
    std::optional<ScalarValue> value;

    // Set when the file supplied a value that could not be parsed. In that
    // case the handler is not called at all (matching the old behaviour where
    // process_as threw and the exception was caught, suppressing the call).
    bool had_parse_error = false;
};

struct ArrayAttributeDetails
{
    // Called at dispatch time with the accumulated typed array (or nullopt / preset).
    std::function<void(mlc::Key const&, std::optional<TypedArray> const&)> dispatch;

    // Parses one raw string element from a config file and appends it into
    // parsed_values.  The function is responsible for pushing into the correct
    // inner vector of the TypedArray variant.
    std::function<void(TypedArray&, std::string_view, mlc::Key const&)> parse_and_append;

    // An empty TypedArray of the correct alternative, used when signalling a
    // clear to the dispatch function before values arrive in the same file.
    TypedArray const empty_typed;

    std::string const description;
    std::optional<TypedArray> const preset;
    TypedArray parsed_values;
    std::vector<std::filesystem::path> modification_locations;
    bool explicitly_cleared = false;
};

std::string to_string(TypedScalar const& typed_value)
{
    return std::visit(
        [](auto const& v) -> std::string
        {
            if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::string>)
                return v;
            else
                return std::to_string(v);
        },
        typed_value);
}

std::string to_string(TypedArray const& typed_array)
{
    return std::visit(
        [](auto const& vec) -> std::string
        {
            std::string result = "[";
            for (auto const& element : vec)
            {
                if constexpr (std::is_same_v<std::decay_t<decltype(element)>, std::string>)
                    result += element;
                else if constexpr (std::is_same_v<std::decay_t<decltype(element)>, bool>)
                    result += element ? "true" : "false";
                else
                    result += std::to_string(element);
                result += ", ";
            }
            result += "]";
            return result;
        },
        typed_array);
}

class ConfigState
{
public:
    void add_scalar_attribute(mlc::Key const& key, AttributeDetails details)
    {
        if (attribute_handlers.erase(key) || array_attribute_handlers.erase(key))
            mir::log_warning("Config attribute handler for '%s' overwritten", key.to_string().c_str());
        attribute_handlers.emplace(key, std::move(details));
    }

    void add_array_attribute(mlc::Key const& key, ArrayAttributeDetails details)
    {
        if (attribute_handlers.erase(key) || array_attribute_handlers.erase(key))
            mir::log_warning("Config attribute handler for '%s' overwritten", key.to_string().c_str());
        array_attribute_handlers.emplace(key, std::move(details));
    }

    void clear()
    {
        for (auto& [_, details] : attribute_handlers)
        {
            details.value = std::nullopt;
            details.had_parse_error = false;
        }

        for (auto& [_, details] : array_attribute_handlers)
        {
            // Reset to an empty vector of the correct element type.
            details.parsed_values = details.empty_typed;
            details.modification_locations.resize(0);
            details.explicitly_cleared = false;
        }
    }

    bool is_scalar(mlc::Key const& key) const
    {
        return attribute_handlers.contains(key);
    }

    bool is_array(mlc::Key const& key) const
    {
        return array_attribute_handlers.contains(key);
    }

    void update_scalar_value(mlc::Key const& key, std::string_view value, std::filesystem::path const& path)
    {
        auto& details = attribute_handlers.at(key);
        try
        {
            details.value = ScalarValue{details.parse(value, key), path};
            details.had_parse_error = false;
        }
        catch (std::exception const& e)
        {
            mir::log_warning(
                "Error parsing scalar '%s' with value '%s' in file '%s': %s",
                key.to_string().c_str(),
                std::string{value}.c_str(),
                path.c_str(),
                e.what());
            details.had_parse_error = true;
        }
    }

    void update_array_value(mlc::Key const& key, std::string_view value, std::filesystem::path const& path)
    {
        auto& details = array_attribute_handlers.at(key);
        auto& parsed_values = details.parsed_values;
        auto& modification_locations = details.modification_locations;

        if (!std::ranges::contains(modification_locations, path))
            modification_locations.push_back(path);

        if (value.empty())
        {
            parsed_values = details.empty_typed;
            details.explicitly_cleared = true;
            // Signal clear immediately so a subsequent append in the same file
            // does not silently become a pure append from the client's perspective.
            details.dispatch(key, details.empty_typed);
        }
        else
        {
            try
            {
                details.parse_and_append(parsed_values, value, key);
            }
            catch (std::exception const& e)
            {
                mir::log_warning(
                    "Error parsing array element for '%s' with value '%s' in file '%s': %s",
                    key.to_string().c_str(),
                    std::string{value}.c_str(),
                    path.c_str(),
                    e.what());
            }
            details.explicitly_cleared = false;
        }
    }

    void update_typed_value(mlc::Key const& key, TypedScalar value, std::filesystem::path const& path)
    {
        auto& details = attribute_handlers.at(key);
        details.value = ScalarValue{std::move(value), path};
    }

    void update_typed_array_value(mlc::Key const& key, TypedArray single_element, std::filesystem::path const& path)
    {
        auto& details = array_attribute_handlers.at(key);

        if (!std::ranges::contains(details.modification_locations, path))
            details.modification_locations.push_back(path);

        // Append the single element from single_element into parsed_values by
        // visiting both variants and pushing into the inner std::vector<T>.
        std::visit(
            [&](auto&& src_vec, auto& dst_vec)
            {
                using SrcT = std::decay_t<decltype(src_vec)>;
                using DstT = std::decay_t<decltype(dst_vec)>;
                if constexpr (std::is_same_v<SrcT, DstT>)
                {
                    dst_vec.insert(dst_vec.end(), src_vec.begin(), src_vec.end());
                }
                else
                {
                    auto const source_type = typeid(SrcT).name();
                    auto const dest_type = typeid(DstT).name();
                    mir::log_warning(
                        "Type mismatch when appending to array config key '%s' in file '%s. Source element type: "
                        "%s, destination element type: %s",
                        key.to_string().c_str(),
                        path.c_str(),
                        source_type,
                        dest_type);
                }
            },
            std::move(single_element),
            details.parsed_values);

        details.explicitly_cleared = false;
    }

    void clear_array_value(mlc::Key const& key)
    {
        auto& details = array_attribute_handlers.at(key);
        details.parsed_values = details.empty_typed;
        details.explicitly_cleared = true;
    }

    void call_attribute_handlers() const
    {
        for (auto const& [key, details] : attribute_handlers)
            try
            {
                // If the file provided an unparseable value, do not call the
                // handler at all (same behaviour as the old process_as throw path).
                if (details.had_parse_error)
                    continue;

                if (details.value)
                    details.dispatch(key, details.value->typed_value);
                else if (details.preset)
                    details.dispatch(key, *details.preset);
                else
                    details.dispatch(key, std::nullopt);
            }
            catch (std::exception const& e)
            {
                mir::log_warning(
                    "Error processing scalar '%s' with value '%s' set in file '%s': %s",
                    key.to_string().c_str(),
                    details.value ? to_string(details.value->typed_value).c_str() : "nullopt",
                    details.value ? details.value->path.c_str() : "",
                    e.what());
            }

        for (auto const& [key, details] : array_attribute_handlers)
            try
            {
                auto const is_empty = std::visit([](auto const& vec) { return vec.empty(); }, details.parsed_values);
                if (!details.explicitly_cleared && is_empty)
                    details.dispatch(key, details.preset);
                else
                    details.dispatch(key, details.parsed_values);
            }
            catch (std::exception const& e)
            {
                auto const array_values = to_string(details.parsed_values);

                std::string modification_locations_str;
                std::ranges::copy(
                    details.modification_locations | std::views::transform([](auto const& p) { return p.string(); })
                        | std::views::join_with(','),
                    std::back_inserter(modification_locations_str));

                mir::log_warning(
                    "Error processing array '%s' with values '%s' modified in files [%s]: %s",
                    key.to_string().c_str(),
                    array_values.c_str(),
                    modification_locations_str.c_str(),
                    e.what());
            }
    }

    void on_done(mlc::Store::HandleDone handler)
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
    std::map<mlc::Key, AttributeDetails> attribute_handlers;
    std::map<mlc::Key, ArrayAttributeDetails> array_attribute_handlers;
    std::list<mlc::Store::HandleDone> done_handlers;
};

/// Parse a string into T using std::from_chars. Throws on failure.
template <typename T> auto parse_scalar(std::string_view val, mlc::Key const& key) -> T
{
    T parsed{};
    auto const [end, err] = std::from_chars(val.data(), val.data() + val.size(), parsed);
    if ((err == std::errc{}) && (end == val.data() + val.size()))
        return parsed;
    throw std::runtime_error(std::format("Config key '{}' has invalid value: {}", key.to_string(), val));
}

template <> auto parse_scalar<bool>(std::string_view val, mlc::Key const& key) -> bool
{
    if (val == "true")
        return true;
    if (val == "false")
        return false;
    throw std::runtime_error(std::format("Config key '{}' has invalid value: {}", key.to_string(), val));
}

template <> auto parse_scalar<std::string>(std::string_view val, mlc::Key const&) -> std::string
{
    return std::string{val};
}

/// Dispatch a TypedScalar (or nullopt) to a typed handler, converting the
/// stored variant alternative to the expected type T. Throws if the stored
/// alternative is not compatible.
template <typename T>
void dispatch_scalar(
    std::function<void(mlc::Key const&, std::optional<T>)> const& handler,
    mlc::Key const& key,
    std::optional<TypedScalar> const& value)
{
    if (!value)
    {
        handler(key, std::nullopt);
        return;
    }

    if constexpr (std::is_same<T, std::string_view>::value)
        handler(key, std::string_view{std::get<std::string>(*value)});
    else
        handler(key, std::get<T>(*value));
}

template <typename T>
void dispatch_array(
    std::function<void(mlc::Key const&, std::optional<std::span<T const>>)> const& handler,
    mlc::Key const& key,
    std::optional<TypedArray> const& values)
{
    if (!values)
    {
        handler(key, std::nullopt);
        return;
    }
    auto const& vec = std::get<std::vector<T>>(*values);
    handler(key, std::span<T const>{vec});
}

/// Build an AttributeDetails for a scalar attribute of stored type StoredT
/// dispatched as DispatchT.  For int/bool/float: StoredT == DispatchT == T.
/// For strings: StoredT = std::string, DispatchT = std::string_view.
template <typename StoredT, typename DispatchT, typename Handler>
auto make_scalar_attribute(std::string_view description, std::optional<StoredT> preset, Handler handler)
    -> AttributeDetails
{
    auto const typed_preset = preset.transform([](auto const& p) { return TypedScalar{p}; });
    return {
        [handler](mlc::Key const& k, std::optional<TypedScalar> const& v)
        { dispatch_scalar<DispatchT>(handler, k, v); },
        [](std::string_view s, mlc::Key const& k) -> TypedScalar { return parse_scalar<StoredT>(s, k); },
        std::string{description},
        typed_preset,
        std::nullopt};
}

/// Build an ArrayAttributeDetails for an array attribute with element type T.
template <typename T, typename Handler>
auto make_array_attribute(std::string_view description, std::optional<std::span<T const>> preset, Handler handler)
    -> ArrayAttributeDetails
{
    auto const typed_preset = preset.transform([](auto p) { return TypedArray{std::vector<T>{p.begin(), p.end()}}; });
    return {
        [handler](mlc::Key const& k, std::optional<TypedArray> const& v) { dispatch_array<T>(handler, k, v); },
        [](TypedArray& arr, std::string_view s, mlc::Key const& k)
        { std::get<std::vector<T>>(arr).push_back(parse_scalar<T>(s, k)); },
        TypedArray{std::vector<T>{}},
        std::string{description},
        std::move(typed_preset),
        TypedArray{std::vector<T>{}},
        {},
        false};
}
}

struct miral::live_config::BasicStore::Self
{
public:
    Self() = default;

    std::mutex mutex; // TODO locking
    ConfigState config_state;
};

mlc::BasicStore::BasicStore() :
    self{std::make_shared<Self>()}
{
}

void mlc::BasicStore::add_int_attribute(Key const& key, std::string_view description, HandleInt handler)
{
    std::lock_guard lock{self->mutex};
    self->config_state.add_scalar_attribute(key, make_scalar_attribute<int, int>(description, std::nullopt, handler));
}

void mlc::BasicStore::add_int_attribute(Key const& key, std::string_view description, int preset, HandleInt handler)
{
    std::lock_guard lock{self->mutex};
    self->config_state.add_scalar_attribute(key, make_scalar_attribute<int, int>(description, preset, handler));
}

void mlc::BasicStore::add_ints_attribute(Key const& key, std::string_view description, HandleInts handler)
{
    std::lock_guard lock{self->mutex};
    self->config_state.add_array_attribute(key, make_array_attribute<int>(description, std::nullopt, handler));
}

void mlc::BasicStore::add_ints_attribute(
    Key const& key, std::string_view description, std::span<int const> preset, HandleInts handler)
{
    std::lock_guard lock{self->mutex};
    self->config_state.add_array_attribute(key, make_array_attribute<int>(description, preset, handler));
}

void mlc::BasicStore::add_bool_attribute(Key const& key, std::string_view description, HandleBool handler)
{
    std::lock_guard lock{self->mutex};
    self->config_state.add_scalar_attribute(key, make_scalar_attribute<bool, bool>(description, std::nullopt, handler));
}

void mlc::BasicStore::add_bool_attribute(Key const& key, std::string_view description, bool preset, HandleBool handler)
{
    std::lock_guard lock{self->mutex};
    self->config_state.add_scalar_attribute(key, make_scalar_attribute<bool, bool>(description, preset, handler));
}

void mlc::BasicStore::add_float_attribute(Key const& key, std::string_view description, HandleFloat handler)
{
    std::lock_guard lock{self->mutex};
    self->config_state.add_scalar_attribute(
        key, make_scalar_attribute<float, float>(description, std::nullopt, handler));
}

void mlc::BasicStore::add_float_attribute(
    Key const& key, std::string_view description, float preset, HandleFloat handler)
{
    std::lock_guard lock{self->mutex};
    self->config_state.add_scalar_attribute(key, make_scalar_attribute<float, float>(description, preset, handler));
}

void mlc::BasicStore::add_floats_attribute(Key const& key, std::string_view description, HandleFloats handler)
{
    std::lock_guard lock{self->mutex};
    self->config_state.add_array_attribute(key, make_array_attribute<float>(description, std::nullopt, handler));
}

void mlc::BasicStore::add_floats_attribute(
    Key const& key, std::string_view description, std::span<float const> preset, HandleFloats handler)
{
    std::lock_guard lock{self->mutex};
    self->config_state.add_array_attribute(key, make_array_attribute<float>(description, preset, handler));
}

void mlc::BasicStore::add_string_attribute(Key const& key, std::string_view description, HandleString handler)
{
    std::lock_guard lock{self->mutex};
    self->config_state.add_scalar_attribute(
        key, make_scalar_attribute<std::string, std::string_view>(description, std::nullopt, handler));
}

void mlc::BasicStore::add_string_attribute(
    Key const& key, std::string_view description, std::string_view preset, HandleString handler)
{
    std::lock_guard lock{self->mutex};
    self->config_state.add_scalar_attribute(
        key, make_scalar_attribute<std::string, std::string_view>(description, std::string{preset}, handler));
}

void mlc::BasicStore::add_strings_attribute(Key const& key, std::string_view description, HandleStrings handler)
{
    std::lock_guard lock{self->mutex};
    self->config_state.add_array_attribute(key, make_array_attribute<std::string>(description, std::nullopt, handler));
}

void mlc::BasicStore::add_strings_attribute(
    Key const& key, std::string_view description, std::span<std::string const> preset, HandleStrings handler)
{
    std::lock_guard lock{self->mutex};
    self->config_state.add_array_attribute(key, make_array_attribute<std::string>(description, preset, handler));
}

void mlc::BasicStore::on_done(HandleDone handler)
{
    std::lock_guard lock{self->mutex};
    self->config_state.on_done(std::move(handler));
}

bool mlc::BasicStore::update_key(Key const& key, std::string_view value, std::filesystem::path modification_path)
{
    if (self->config_state.is_scalar(key))
    {
        self->config_state.update_scalar_value(key, value, modification_path);
        return true;
    }

    if (self->config_state.is_array(key))
    {
        self->config_state.update_array_value(key, value, modification_path);
        return true;
    }

    return false;
}

void mlc::BasicStore::update_typed_value_impl(
    Key const& key, TypedScalar typed_value, std::filesystem::path const& path)
{
    self->config_state.update_typed_value(key, std::move(typed_value), path);
}

void mlc::BasicStore::update_typed_array_value_impl(
    Key const& key, TypedArray single_element, std::filesystem::path const& path)
{
    self->config_state.update_typed_array_value(key, std::move(single_element), path);
}

void mlc::BasicStore::clear_array(Key const& key)
{
    self->config_state.clear_array_value(key);
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
