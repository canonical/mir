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

#include "basic_store.h"
#include "typed_store_adapter.h"

#include <miral/live_config_ini_file.h>

#include <mir/log.h>


namespace mlc = miral::live_config;

class mlc::IniFile::Self: public mlc::BasicStore
{
public:
    void load_file(std::istream& istream, std::filesystem::path const& path);
    TypedStoreAdapter adapter{*this};
};

namespace
{
auto trim_start_and_end(std::string_view string) -> std::string_view
{
    constexpr auto whitespace = " \t\n\r\f\v";

    auto const first_non_space = string.find_first_not_of(whitespace);
    if (first_non_space == std::string::npos)
    {
        return string.substr(0, 0);
    }

    auto const last_non_space = string.find_last_not_of(whitespace);
    return string.substr(first_non_space, last_non_space - first_non_space + 1);
}
}

void mlc::IniFile::Self::load_file(std::istream& istream, std::filesystem::path const& path)
{
    do_transaction(
        [&]
        {
            for (std::string line; std::getline(istream, line);)
            {
                auto const line_view = std::string_view{line};
                if (!line_view.starts_with('#') && line_view.contains("="))
                    try
                    {
                        auto const eq = line_view.find_first_of("=");
                        auto const key = Key{trim_start_and_end(line_view.substr(0, eq))};
                        auto const value = trim_start_and_end(line_view.substr(eq + 1));

                        if (!value.empty())
                            update_key(key, value, path);
                        else if (!clear_array(key))
                            update_key(key, value, path);
                    }
                    catch (std::exception const& e)
                    {
                        mir::log_warning("Error processing '%s': %s", path.c_str(), e.what());
                    }
            }

        });
}

mlc::IniFile::IniFile() :
    self{std::make_shared<Self>()}
{
}

mlc::IniFile::~IniFile() = default;

void mlc::IniFile::add_int_attribute(Key const& key, std::string_view description, HandleInt handler)
{
    self->adapter.add_int_attribute(key, description, std::move(handler));
}

void mlc::IniFile::add_ints_attribute(Key const& key, std::string_view description, HandleInts handler)
{
    self->adapter.add_ints_attribute(key, description, std::move(handler));
}

void mlc::IniFile::add_bool_attribute(Key const& key, std::string_view description, HandleBool handler)
{
    self->adapter.add_bool_attribute(key, description, std::move(handler));
}

void mlc::IniFile::add_float_attribute(Key const& key, std::string_view description, HandleFloat handler)
{
    self->adapter.add_float_attribute(key, description, std::move(handler));
}

void mlc::IniFile::add_floats_attribute(Key const& key, std::string_view description, HandleFloats handler)
{
    self->adapter.add_floats_attribute(key, description, std::move(handler));
}

void mlc::IniFile::add_string_attribute(Key const& key, std::string_view description, HandleString handler)
{
    self->adapter.add_string_attribute(key, description, std::move(handler));
}

void mlc::IniFile::add_strings_attribute(Key const& key, std::string_view description, HandleStrings handler)
{
    self->adapter.add_strings_attribute(key, description, std::move(handler));
}

void mlc::IniFile::add_int_attribute(Key const& key, std::string_view description, int preset, HandleInt handler)
{
    self->adapter.add_int_attribute(key, description, preset, std::move(handler));
}

void mlc::IniFile::add_ints_attribute(
    Key const& key, std::string_view description, std::span<int const> preset, HandleInts handler)
{
    self->adapter.add_ints_attribute(key, description, preset, std::move(handler));
}

void mlc::IniFile::add_bool_attribute(Key const& key, std::string_view description, bool preset, HandleBool handler)
{
    self->adapter.add_bool_attribute(key, description, preset, std::move(handler));
}

void mlc::IniFile::add_float_attribute(Key const& key, std::string_view description, float preset, HandleFloat handler)
{
    self->adapter.add_float_attribute(key, description, preset, std::move(handler));
}

void mlc::IniFile::add_floats_attribute(
    Key const& key, std::string_view description, std::span<float const> preset, HandleFloats handler)
{
    self->adapter.add_floats_attribute(key, description, preset, std::move(handler));
}

void mlc::IniFile::add_string_attribute(
    Key const& key, std::string_view description, std::string_view preset, HandleString handler)
{
    self->adapter.add_string_attribute(key, description, preset, std::move(handler));
}

void mlc::IniFile::add_strings_attribute(
    Key const& key,
    std::string_view description,
    std::span<std::string const> preset,
    HandleStrings handler)
{
    self->adapter.add_strings_attribute(key, description, preset, std::move(handler));
}

void mlc::IniFile::on_done(HandleDone handler)
{
    self->on_done(std::move(handler));
}

void mlc::IniFile::load_file(std::istream& istream, std::filesystem::path const& path)
{
    self->load_file(istream, path);
}
