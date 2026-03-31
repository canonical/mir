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

#include "miral/live_config.h"
#include <filesystem>

namespace miral
{
namespace live_config
{
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

    bool is_scalar(Key const& key) const;
    bool is_array(Key const& key) const;

    void update_scalar_value(Key const& key, std::string_view value, std::filesystem::path const& path);
    void update_array_value(Key const& key, std::string_view value, std::filesystem::path const& path);

    void clear_values();
    void call_attribute_handlers() const;
    void call_done_handlers(std::string const& paths) const;

private:
    struct Self;
    std::shared_ptr<Self> self;
};
}
}

#endif
