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

#ifndef MIRAL_LIVE_CONFIG_H
#define MIRAL_LIVE_CONFIG_H

#include <functional>
#include <initializer_list>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace miral::live_config
{
/// Registration key for a configuration attribute. The key is essentially a tuple of
/// identifiers
///
/// \remark Since MirAL 5.4
class Key
{
public:
    /// To simplify mapping these identifiers to multiple configuration backends, each identifier
    /// within the key is non-empty, starts with [a..z], and contains only [a..z,0..9,_]
    Key(std::initializer_list<std::string_view const> key);

    /// Expose the key elements
    void with_key(std::function<void(std::vector<std::string> const& key)> const& f) const;

    /// The key represented as a "_" joined string
    auto to_string() const -> std::string;

    auto operator<=>(Key const& that) const -> std::strong_ordering;

private:
   struct State;
   std::shared_ptr<State> self;
};

/// Interface for adding attributes to a live configuration store.
///
/// The handlers should be called when the configuration is updated. There is
/// no requirement to check the previous value has changed.
///
/// The `value` provided to the Handlers is `optional` to support values being
/// explicitly unset. Store implementations default absent keys to `preset`.
///
/// The key is passed to the handler as it can be useful (e.g. for diagnostics)
///
/// This could be supported by various backends (an ini file, a YAML node, etc)
///
/// \remark Since MirAL 5.4
class Store
{
public:
    using HandleInt = std::function<void(Key const& key, std::optional<int> value)>;
    using HandleInts = std::function<void(Key const& key, std::optional<std::span<int const>> value)>;
    using HandleBool = std::function<void(Key const& key, std::optional<bool> value)>;
    using HandleBools = std::function<void(Key const& key, std::optional<std::span<bool const>> value)>;
    using HandleFloat = std::function<void(Key const& key, std::optional<float> value)>;
    using HandleFloats = std::function<void(Key const& key, std::optional<std::span<float const>> value)>;
    using HandleString = std::function<void(Key const& key, std::optional<std::string_view> value)>;
    using HandleStrings = std::function<void(Key const& key, std::optional<std::span<std::string const>> value)>;
    using HandleDone = std::function<void()>;

    virtual void add_int_attribute(Key const& key, std::string_view description, HandleInt handler) = 0;
    virtual void add_ints_attribute(Key const& key, std::string_view description, HandleInts handler) = 0;
    virtual void add_bool_attribute(Key const& key, std::string_view description, HandleBool handler) = 0;
    virtual void add_bools_attribute(Key const& key, std::string_view description, HandleBools handler) = 0;
    virtual void add_float_attribute(Key const& key, std::string_view description, HandleFloat handler) = 0;
    virtual void add_floats_attribute(Key const& key, std::string_view description, HandleFloats handler) = 0;
    virtual void add_string_attribute(Key const& key, std::string_view description, HandleString handler) = 0;
    virtual void add_strings_attribute(Key const& key, std::string_view description, HandleStrings handler) = 0;

    virtual void add_int_attribute(Key const& key, std::string_view description, int preset, HandleInt handler) = 0;
    virtual void add_ints_attribute(Key const& key, std::string_view description, std::span<int const> preset, HandleInts handler) = 0;
    virtual void add_bool_attribute(Key const& key, std::string_view description, bool preset, HandleBool handler) = 0;
    virtual void add_bools_attribute(Key const& key, std::string_view description, std::span<bool const> preset, HandleBools handler) = 0;
    virtual void add_float_attribute(Key const& key, std::string_view description, float preset, HandleFloat handler) = 0;
    virtual void add_floats_attribute(Key const& key, std::string_view description, std::span<float const> preset, HandleFloats handler) = 0;
    virtual void add_string_attribute(Key const& key, std::string_view description, std::string_view preset, HandleString handler) = 0;
    virtual void add_strings_attribute(Key const& key, std::string_view description, std::span<std::string const> preset, HandleStrings handler) = 0;

    /// Called following a set of related updates (e.g. a file reload) to allow
    /// multiple attributes to be updated transactionally
    virtual void on_done(HandleDone handler) = 0;

    virtual ~Store() = default;
};
}

#endif //MIRAL_LIVE_CONFIG_H
