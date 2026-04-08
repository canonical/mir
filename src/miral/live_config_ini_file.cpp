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

#include <miral/live_config_ini_file.h>
#include "live_config_basic_store.h"

#include <mir/log.h>

namespace mlc = miral::live_config;

struct mlc::IniFile::Self
{
    mlc::BasicStore store;
};

namespace
{
void parse_one_file(mlc::BasicStore& store, std::istream& istream, std::filesystem::path const& path)
{
    for (std::string line; std::getline(istream, line);)
    {
        if (!line.starts_with('#') && line.contains("="))
            try
            {
                auto const eq = line.find_first_of("=");
                auto const key = mlc::Key{line.substr(0, eq)};
                auto const value = line.substr(eq + 1);

                if(!store.update_key(key, value, path))
                {
                    mir::log_warning(
                        "Config key '%s' in file '%s' is not registered. Skipping.",
                        key.to_string().c_str(),
                        path.c_str());
                }
            }
            catch (std::exception const& e)
            {
                mir::log_warning("Error processing '%s': %s", path.c_str(), e.what());
            }
    }
}
} // namespace

mlc::IniFile::IniFile() :
    self{std::make_shared<Self>()}
{
}

mlc::IniFile::~IniFile() = default;

void mlc::IniFile::load_file(std::istream& istream, std::filesystem::path const& path)
{
    self->store.clear_values();
    parse_one_file(self->store, istream, path);
    self->store.done({&path, 1});
}

void mlc::IniFile::add_int_attribute(Key const& key, std::string_view description, HandleInt handler)
{
    self->store.add_int_attribute(key, description, std::move(handler));
}

void mlc::IniFile::add_ints_attribute(Key const& key, std::string_view description, HandleInts handler)
{
    self->store.add_ints_attribute(key, description, std::move(handler));
}

void mlc::IniFile::add_bool_attribute(Key const& key, std::string_view description, HandleBool handler)
{
    self->store.add_bool_attribute(key, description, std::move(handler));
}

void mlc::IniFile::add_float_attribute(Key const& key, std::string_view description, HandleFloat handler)
{
    self->store.add_float_attribute(key, description, std::move(handler));
}

void mlc::IniFile::add_floats_attribute(Key const& key, std::string_view description, HandleFloats handler)
{
    self->store.add_floats_attribute(key, description, std::move(handler));
}

void mlc::IniFile::add_string_attribute(Key const& key, std::string_view description, HandleString handler)
{
    self->store.add_string_attribute(key, description, std::move(handler));
}

void mlc::IniFile::add_strings_attribute(Key const& key, std::string_view description, HandleStrings handler)
{
    self->store.add_strings_attribute(key, description, std::move(handler));
}

void mlc::IniFile::add_int_attribute(Key const& key, std::string_view description, int preset, HandleInt handler)
{
    self->store.add_int_attribute(key, description, preset, std::move(handler));
}

void mlc::IniFile::add_ints_attribute(
    Key const& key, std::string_view description, std::span<int const> preset, HandleInts handler)
{
    self->store.add_ints_attribute(key, description, preset, std::move(handler));
}

void mlc::IniFile::add_bool_attribute(Key const& key, std::string_view description, bool preset, HandleBool handler)
{
    self->store.add_bool_attribute(key, description, preset, std::move(handler));
}

void mlc::IniFile::add_float_attribute(Key const& key, std::string_view description, float preset, HandleFloat handler)
{
    self->store.add_float_attribute(key, description, preset, std::move(handler));
}

void mlc::IniFile::add_floats_attribute(
    Key const& key, std::string_view description, std::span<float const> preset, HandleFloats handler)
{
    self->store.add_floats_attribute(key, description, preset, std::move(handler));
}

void mlc::IniFile::add_string_attribute(
    Key const& key, std::string_view description, std::string_view preset, HandleString handler)
{
    self->store.add_string_attribute(key, description, preset, std::move(handler));
}

void mlc::IniFile::add_strings_attribute(
    Key const& key, std::string_view description, std::span<std::string const> preset, HandleStrings handler)
{
    self->store.add_strings_attribute(key, description, preset, std::move(handler));
}

void mlc::IniFile::on_done(HandleDone handler)
{
    self->store.on_done(std::move(handler));
}
