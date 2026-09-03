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

#include <miral/live_config_ini_file_with_overrides.h>
#include <miral/live_config_overrides_list.h>
#include "basic_store.h"
#include "live_config_ini_file_common.h"

#include <mir/log.h>

#include <memory>

namespace mlc = miral::live_config;

class mlc::IniFileWithOverrides::Self : public mlc::BasicStore
{
public:
    void load(OverridesList const& changes);
};

void mlc::IniFileWithOverrides::Self::load(OverridesList const& changes)
{
    do_transaction(
        [&]
        {
            auto const parse_ini_from_changes = [&](std::filesystem::path const& path, std::istream& istream)
            {
                parse_ini(
                    istream,
                    path,
                    [this](auto const& key, auto value, auto const& path) { update_key(key, value, path); });
            };

            changes.for_each(
                parse_ini_from_changes,
                parse_ini_from_changes,
                parse_ini_from_changes,
                [](auto&&...) {} // dropped: no need to handle
            );
        });
}

mlc::IniFileWithOverrides::IniFileWithOverrides() :
    self{std::make_shared<Self>()}
{
}

mlc::IniFileWithOverrides::~IniFileWithOverrides() = default;

void mlc::IniFileWithOverrides::add_int_attribute(Key const& key, std::string_view description, HandleInt handler)
{
    self->add_int_attribute(key, description, std::move(handler));
}

void mlc::IniFileWithOverrides::add_ints_attribute(Key const& key, std::string_view description, HandleInts handler)
{
    self->add_ints_attribute(key, description, std::move(handler));
}

void mlc::IniFileWithOverrides::add_bool_attribute(Key const& key, std::string_view description, HandleBool handler)
{
    self->add_bool_attribute(key, description, std::move(handler));
}

void mlc::IniFileWithOverrides::add_float_attribute(Key const& key, std::string_view description, HandleFloat handler)
{
    self->add_float_attribute(key, description, std::move(handler));
}

void mlc::IniFileWithOverrides::add_floats_attribute(Key const& key, std::string_view description, HandleFloats handler)
{
    self->add_floats_attribute(key, description, std::move(handler));
}

void mlc::IniFileWithOverrides::add_string_attribute(Key const& key, std::string_view description, HandleString handler)
{
    self->add_string_attribute(key, description, std::move(handler));
}

void mlc::IniFileWithOverrides::add_strings_attribute(Key const& key, std::string_view description, HandleStrings handler)
{
    self->add_strings_attribute(key, description, std::move(handler));
}

void mlc::IniFileWithOverrides::add_int_attribute(
    Key const& key, std::string_view description, int preset, HandleInt handler)
{
    self->add_int_attribute(key, description, preset, std::move(handler));
}

void mlc::IniFileWithOverrides::add_ints_attribute(
    Key const& key, std::string_view description, std::span<int const> preset, HandleInts handler)
{
    self->add_ints_attribute(key, description, preset, std::move(handler));
}

void mlc::IniFileWithOverrides::add_bool_attribute(
    Key const& key, std::string_view description, bool preset, HandleBool handler)
{
    self->add_bool_attribute(key, description, preset, std::move(handler));
}

void mlc::IniFileWithOverrides::add_float_attribute(
    Key const& key, std::string_view description, float preset, HandleFloat handler)
{
    self->add_float_attribute(key, description, preset, std::move(handler));
}

void mlc::IniFileWithOverrides::add_floats_attribute(
    Key const& key, std::string_view description, std::span<float const> preset, HandleFloats handler)
{
    self->add_floats_attribute(key, description, preset, std::move(handler));
}

void mlc::IniFileWithOverrides::add_string_attribute(
    Key const& key, std::string_view description, std::string_view preset, HandleString handler)
{
    self->add_string_attribute(key, description, preset, std::move(handler));
}

void mlc::IniFileWithOverrides::add_strings_attribute(
    Key const& key,
    std::string_view description,
    std::span<std::string const> preset,
    HandleStrings handler)
{
    self->add_strings_attribute(key, description, preset, std::move(handler));
}

void mlc::IniFileWithOverrides::on_done(HandleDone handler)
{
    self->on_done(std::move(handler));
}

void mlc::IniFileWithOverrides::load(OverridesList const& changes)
{
    self->load(changes);
}
