#include "basic_store.h"

#include <mir/log.h>

#include <algorithm>
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

    void add_scalar_attribute(Key const& key, std::string_view description, HandleAttribute handler);
    void add_array_attribute(Key const& key, std::string_view description, HandleArrayAttribute handler);

    void update_key(Key const& key, std::string_view value, std::filesystem::path const& modification_path);
    void do_transaction(std::function<void()> transaction_body);

    void on_done(std::function<void()> handler);

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
        HandleAttribute const handler;
        std::string const description;
        std::optional<ScalarValue> value;
    };

    struct ArrayAttributeDetails
    {
        HandleArrayAttribute const handler;
        std::string const description;
        std::vector<std::string> parsed_values;
        std::set<std::filesystem::path> modification_paths;
    };

    std::mutex mutex;
    std::map<Key, AttributeDetails> attribute_handlers;
    std::map<Key, ArrayAttributeDetails> array_attribute_handlers;
    std::list<std::function<void()>> done_handlers;
};

void mlc::BasicStore::Self::add_scalar_attribute(Key const& key, std::string_view description, HandleAttribute handler)
{
    std::lock_guard lock{mutex};

    if (attribute_handlers.erase(key) || array_attribute_handlers.erase(key))
    {
        // if a key is registered multiple times, the last time is used: drop existing earlier registrations
        mir::log_warning("Config attribute handler for '%s' overwritten", key.to_string().c_str());
    }

    attribute_handlers.emplace(key, AttributeDetails{handler, std::string{description}, std::nullopt});
}

void mlc::BasicStore::Self::on_done(std::function<void()> handler)
{
    std::lock_guard lock{mutex};
    done_handlers.emplace_back(std::move(handler));
}

void mlc::BasicStore::Self::add_array_attribute(
    Key const& key, std::string_view description, HandleArrayAttribute handler)
{
    std::lock_guard lock{mutex};

    if (attribute_handlers.erase(key) || array_attribute_handlers.erase(key))
    {
        // if a key is registered multiple times, the last time is used: drop existing earlier registrations
        mir::log_warning("Config attribute handler for '%s' overwritten", key.to_string().c_str());
    }

    array_attribute_handlers.emplace(
        key, ArrayAttributeDetails{handler, std::string{description}, std::vector<std::string>{}, {}});
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
        auto const maybe_value = details.value.transform(
            [](ScalarValue const& v)
            {
                return v.value;
            });

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



mlc::BasicStore::BasicStore() :
    self{std::make_shared<Self>()}
{
}

mlc::BasicStore::~BasicStore() = default;

void mlc::BasicStore::add_scalar_attribute(Key const& key, std::string_view description, HandleAttribute handler)
{
    self->add_scalar_attribute(key, description,  handler);
}

void mlc::BasicStore::add_array_attribute(Key const& key, std::string_view description, HandleArrayAttribute handler)
{
    self->add_array_attribute(key, description, handler);
}

void mlc::BasicStore::on_done(std::function<void()> handler)
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
