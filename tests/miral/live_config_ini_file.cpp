/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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

#include <miral/live_config_ini_file.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mlc = miral::live_config;

////////////////////////////////////////////////////////////////////
// TODO mlc::IniFile implementation here temporarily (needs to move)

class mlc::IniFile::Self {};

mlc::IniFile::IniFile() :
    self{std::make_shared<Self>()}
{
}

mlc::IniFile::~IniFile() = default;

void mlc::IniFile::add_int_attribute(Key const& key, std::string_view description, HandleInt handler)
{
    (void)key; (void)description; (void)handler;
}

void mlc::IniFile::add_ints_attribute(Key const& key, std::string_view description, HandleInts handler) 
{
    (void)key; (void)description; (void)handler;
}

void mlc::IniFile::add_bool_attribute(Key const& key, std::string_view description, HandleBool handler)
{
    (void)key; (void)description; (void)handler;
}

void mlc::IniFile::add_bools_attribute(Key const& key, std::string_view description, HandleBools handler)
{
    (void)key; (void)description; (void)handler;
}

void mlc::IniFile::add_float_attribute(Key const& key, std::string_view description, HandleFloat handler)
{
    (void)key; (void)description; (void)handler;
}

void mlc::IniFile::add_floats_attribute(Key const& key, std::string_view description, HandleFloats handler)
{
    (void)key; (void)description; (void)handler;
}

void mlc::IniFile::add_string_attribute(Key const& key, std::string_view description, HandleString handler)
{
    (void)key; (void)description; (void)handler;
}

void mlc::IniFile::add_strings_attribute(Key const& key, std::string_view description, HandleStrings handler) 
{
    (void)key; (void)description; (void)handler;
}

void mlc::IniFile::add_int_attribute(Key const& key, std::string_view description, int preset, HandleInt handler) 
{
    (void)key; (void)description; (void)preset; (void)handler;
}

void mlc::IniFile::add_ints_attribute(Key const& key, std::string_view description, std::span<int const> preset, HandleInts handler)
{
    (void)key; (void)description; (void)preset; (void)handler;
}

void mlc::IniFile::add_bool_attribute(Key const& key, std::string_view description, bool preset, HandleBool handler)
{
    (void)key; (void)description; (void)preset; (void)handler;
}

void mlc::IniFile::add_bools_attribute(Key const& key, std::string_view description, std::span<bool const> preset, HandleBools handler)
{
    (void)key; (void)description; (void)preset; (void)handler;
}

void mlc::IniFile::add_float_attribute(Key const& key, std::string_view description, float preset, HandleFloat handler)
{
    (void)key; (void)description; (void)preset; (void)handler;
}

void mlc::IniFile::add_floats_attribute(Key const& key, std::string_view description, std::span<float const> preset, HandleFloats handler)
{
    (void)key; (void)description; (void)preset; (void)handler;
}

void mlc::IniFile::add_string_attribute(Key const& key, std::string_view description, std::string_view preset, HandleString handler)
{
    (void)key; (void)description; (void)preset; (void)handler;
}

void mlc::IniFile::add_strings_attribute(Key const& key, std::string_view description, std::span<std::string_view const> preset, HandleStrings handler)
{
    (void)key; (void)description; (void)preset; (void)handler;
}

void mlc::IniFile::on_done(HandleDone handler) 
{
    (void)handler;
}

// TODO mlc::IniFile implementation here temporarily (needs to move)
////////////////////////////////////////////////////////////////////

using namespace testing;

