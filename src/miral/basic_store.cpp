#include "basic_store.h"

#include <mir/log.h>

#include <list>
#include <map>
#include <mutex>
#include <set>

namespace mlc = miral::live_config;

class mlc::BasicStore::Self
{
public:
    Self() = default;

    void add_scalar_attribute(Key const& key, std::string_view description, HandleAttribute handler);
    void add_array_attribute(Key const& key, std::string_view description, HandleArrayAttribute handler);

    void update_key(Key const& key, std::string_view value, std::filesystem::path const& modification_path);
    bool clear_array(Key const& key);
    void do_transaction(std::function<void()> transaction_body);

    void on_done(std::function<void()> handler);

    void foreach_scalar_attribute(std::function<void(Key const&, std::string_view)> const& callback) const;
    void foreach_array_attribute(std::function<void(Key const&, std::string_view)> const& callback) const;

private:
    Self(Self const&) = delete;
    Self& operator=(Self const&) = delete;

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
        ArrayValue array;
    };

    mutable std::mutex mutex;
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

    array_attribute_handlers.emplace(key, ArrayAttributeDetails{handler, std::string{description}, ArrayValue{}});
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
        details.array.values.push_back(std::string{value});
        details.array.modification_paths.insert(modification_path);
    }
    else
    {
        mir::log_warning("Config key '%s' not recognized", key.to_string().c_str());
    }
}

auto mlc::BasicStore::Self::clear_array(Key const& key) -> bool
{
    if (auto const details_iter = array_attribute_handlers.find(key); details_iter != array_attribute_handlers.end())
    {
        auto& details = details_iter->second;
        details.array.values.clear();
        details.array.modification_paths.clear();
        details.array.clear_requested = true;
        return true;
    }
    return false;
}

void mlc::BasicStore::Self::do_transaction(std::function<void()> transaction_body)
{
    std::lock_guard lock{mutex};
    for (auto& [key, details] : attribute_handlers)
        details.value = std::nullopt;

    for (auto& [key, details] : array_attribute_handlers)
    {
        details.array.values.resize(0);
        details.array.modification_paths.clear();
        details.array.clear_requested = false;
    }

    transaction_body();

    for (auto const& [key, details] : attribute_handlers)
    {
        details.handler(key, details.value);
    }

    for (auto const& [key, details] : array_attribute_handlers)
    {
        details.handler(key, details.array);
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

void mlc::BasicStore::Self::foreach_scalar_attribute(
    std::function<void(Key const&, std::string_view)> const& callback) const
{
    std::lock_guard lock{mutex};
    for (auto const& [key, details] : attribute_handlers)
        callback(key, details.description);
}

void mlc::BasicStore::Self::foreach_array_attribute(
    std::function<void(Key const&, std::string_view)> const& callback) const
{
    std::lock_guard lock{mutex};
    for (auto const& [key, details] : array_attribute_handlers)
        callback(key, details.description);
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

auto mlc::BasicStore::clear_array(Key const& key) -> bool
{
    return self->clear_array(key);
}

void mlc::BasicStore::do_transaction(std::function<void()> transaction_body)
{
    self->do_transaction(std::move(transaction_body));
}

void mlc::BasicStore::foreach_scalar_attribute(
    std::function<void(Key const&, std::string_view)> const& callback) const
{
    self->foreach_scalar_attribute(callback);
}

void mlc::BasicStore::foreach_array_attribute(
    std::function<void(Key const&, std::string_view)> const& callback) const
{
    self->foreach_array_attribute(callback);
}
