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

#ifndef MIRAL_LIVE_CONFIG_BASIC_STORE_H
#define MIRAL_LIVE_CONFIG_BASIC_STORE_H

#include <miral/live_config.h>

#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <string_view>

namespace miral
{
namespace live_config
{
/// A basic, in-memory implementation of a live configuration store.
///
/// Stores attribute registrations and their current values, dispatching
/// handlers when values are updated. Unlike \ref IniFile, it does not parse
/// configuration files; instead, callers update values programmatically via
/// the \c update_*_key methods.
///
/// \remark Since MirAL 5.8
class BasicStore : public Store
{
public:
    BasicStore();

    void add_int_attribute(Key const& key, std::string_view description, HandleInt handler) override;
    void add_ints_attribute(Key const& key, std::string_view description, HandleInts handler) override;
    void add_bool_attribute(Key const& key, std::string_view description, HandleBool handler) override;
    void add_float_attribute(Key const& key, std::string_view description, HandleFloat handler) override;
    void add_floats_attribute(Key const& key, std::string_view description, HandleFloats handler) override;
    void add_string_attribute(Key const& key, std::string_view description, HandleString handler) override;
    void add_strings_attribute(Key const& key, std::string_view description, HandleStrings handler) override;

    void add_int_attribute(Key const& key, std::string_view description, int preset, HandleInt handler) override;
    void add_ints_attribute(
        Key const& key, std::string_view description, std::span<int const> preset, HandleInts handler) override;
    void add_bool_attribute(Key const& key, std::string_view description, bool preset, HandleBool handler) override;
    void add_float_attribute(Key const& key, std::string_view description, float preset, HandleFloat handler) override;
    void add_floats_attribute(
        Key const& key, std::string_view description, std::span<float const> preset, HandleFloats handler) override;
    void add_string_attribute(
        Key const& key, std::string_view description, std::string_view preset, HandleString handler) override;
    void add_strings_attribute(
        Key const& key,
        std::string_view description,
        std::span<std::string const> preset,
        HandleStrings handler) override;

    void on_done(HandleDone handler) override;

    bool update_key(Key const& key, std::string_view value, std::filesystem::path modification_path);

    void update_int_key(Key const& key, int value, std::filesystem::path modification_path);
    void update_bool_key(Key const& key, bool value, std::filesystem::path modification_path);
    void update_float_key(Key const& key, float value, std::filesystem::path modification_path);
    void update_string_key(Key const& key, std::string_view value, std::filesystem::path modification_path);

    void update_int_array_key(Key const& key, std::span<int const> values, std::filesystem::path modification_path);
    void update_bool_array_key(Key const& key, std::span<bool const> values, std::filesystem::path modification_path);
    void update_float_array_key(Key const& key, std::span<float const> values, std::filesystem::path modification_path);
    void update_string_array_key(
        Key const& key, std::span<std::string const> values, std::filesystem::path modification_path);

    void clear_array(Key const& key, std::filesystem::path modification_path);

    void clear_values();
    void done(std::span<std::filesystem::path const> paths) const;

private:
    struct Self;
    std::shared_ptr<Self> self;
};
}
}

#endif
