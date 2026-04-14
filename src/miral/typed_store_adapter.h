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

#ifndef MIRAL_TYPED_STORE_ADAPTER_H
#define MIRAL_TYPED_STORE_ADAPTER_H

#include "basic_store.h"

#include <miral/live_config.h>

#include <span>
#include <string_view>

namespace miral::live_config
{
class TypedStoreAdapter
{
public:
    explicit TypedStoreAdapter(BasicStore& store);

    void add_int_attribute(Key const& key, std::string_view description, Store::HandleInt handler);
    void add_ints_attribute(Key const& key, std::string_view description, Store::HandleInts handler);
    void add_bool_attribute(Key const& key, std::string_view description, Store::HandleBool handler);
    void add_float_attribute(Key const& key, std::string_view description, Store::HandleFloat handler);
    void add_floats_attribute(Key const& key, std::string_view description, Store::HandleFloats handler);
    void add_string_attribute(Key const& key, std::string_view description, Store::HandleString handler);
    void add_strings_attribute(Key const& key, std::string_view description, Store::HandleStrings handler);

    void add_int_attribute(Key const& key, std::string_view description, int preset, Store::HandleInt handler);
    void add_ints_attribute(Key const& key, std::string_view description, std::span<int const> preset, Store::HandleInts handler);
    void add_bool_attribute(Key const& key, std::string_view description, bool preset, Store::HandleBool handler);
    void add_float_attribute(Key const& key, std::string_view description, float preset, Store::HandleFloat handler);
    void add_floats_attribute(Key const& key, std::string_view description, std::span<float const> preset, Store::HandleFloats handler);
    void add_string_attribute(Key const& key, std::string_view description, std::string_view preset, Store::HandleString handler);
    void add_strings_attribute(Key const& key, std::string_view description, std::span<std::string const> preset, Store::HandleStrings handler);

private:
    BasicStore& store_;
};
}

#endif //MIRAL_TYPED_STORE_ADAPTER_H
