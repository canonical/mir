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

namespace mlc = miral::live_config;

////////////////////////////////////////////////////////////////////
// TODO mlc::IniFile implementation here temporarily (needs to move)
#define MIR_LOG_COMPONENT "miral"
#include <mir/log.h>

#include <charconv>
#include <list>
#include <map>
#include <set>

class mlc::IniFile::Self
{
public:
    void add_key(live_config::Key const& key, std::string_view description, std::optional<std::string> preset, HandleString handler);

    void load_file(std::istream& istream);

private:
    struct KeyDetails
    {
        HandleString const handler;
        std::string const description;
        std::optional<std::string> const preset;
    };

    std::map<live_config::Key, KeyDetails> attribute_handlers;
    std::list<HandleDone> done_handlers;

    std::mutex config_mutex;
};

void miral::live_config::IniFile::Self::add_key(
    Key const& key,
    std::string_view description,
    std::optional<std::string> preset,
    HandleString handler)
{
    std::lock_guard lock{config_mutex};
    attribute_handlers.emplace(key, KeyDetails{handler, std::string{description}, preset});
}


void mlc::IniFile::Self::load_file(std::istream& istream)
{
    std::set<Key> keys_seen;
    std::lock_guard lock{config_mutex};

    for (std::string line; std::getline(istream, line);)
    {
        puts(line.c_str()); // TODO remove printf debugging

        if (line.contains("="))
        {
            auto const eq = line.find_first_of("=");
            auto const key = live_config::Key{line.substr(0, eq)};
            keys_seen.insert(key);
            auto const value = line.substr(eq+1);

            if (auto const details = attribute_handlers.find(key); details != attribute_handlers.end())
            {
                details->second.handler(details->first, value);
            }
        }
    }

    for (auto const& [key, details] : attribute_handlers)
    {
        if (keys_seen.find(key) == keys_seen.end())
        {
            details.handler(key, details.preset);
        }
    }

    // apply_config();
}

namespace
{
template<typename Type>
void process_as(std::function<void(mlc::Key const&, std::optional<Type>)> const& handler, mlc::Key const& key, std::optional<std::string_view> val)
{
    if (val)
    {
        Type parsed_val{};

        auto const [end, err] = std::from_chars(val->data(), val->data() + val->size(), parsed_val);

        if ((err == std::errc{}) && (end ==val->data() + val->size()))
        {
            handler(key, parsed_val);
        }
        else
        {
            mir::log_warning("Config key '%s' has invalid value: %s", key.to_string().c_str(), std::string(*val).c_str());
            handler(key, std::nullopt);
        }
    }
    else
    {
        handler(key, std::nullopt);
    }
}

template<>
void process_as<bool>(std::function<void(mlc::Key const&, std::optional<bool>)> const& handler, mlc::Key const& key, std::optional<std::string_view> val)
{
    puts(__PRETTY_FUNCTION__);
    if (val)
    {
        if (*val == "true")
        {
            handler(key, true);
        }
        else if (*val == "false")
        {
            handler(key, false);
        }
        else
        {
            mir::log_warning("Config key '%s' has invalid value: %s", key.to_string().c_str(), std::string(*val).c_str());
            handler(key, std::nullopt);
        }
    }
    else
    {
        handler(key, std::nullopt);
    }
}
}

mlc::IniFile::IniFile() :
    self{std::make_shared<Self>()}
{
}

mlc::IniFile::~IniFile() = default;

void mlc::IniFile::add_int_attribute(Key const& key, std::string_view description, HandleInt handler)
{
    self->add_key(key, description, std::nullopt,
        [handler](live_config::Key const& key, std::optional<std::string_view> val)
    {
        process_as<int>(handler, key, val);
    });
}

void mlc::IniFile::add_ints_attribute(Key const& key, std::string_view description, HandleInts handler) 
{
    (void)key; (void)description; (void)handler;
}

void mlc::IniFile::add_bool_attribute(Key const& key, std::string_view description, HandleBool handler)
{
    self->add_key(key, description, std::nullopt,
        [handler](live_config::Key const& key, std::optional<std::string_view> val)
    {
        process_as<bool>(handler, key, val);
    });
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
    self->add_key(key, description, std::to_string(preset),
        [handler](live_config::Key const& key, std::optional<std::string_view> val)
        {
            process_as<int>(handler, key, val);
        });
}

void mlc::IniFile::add_ints_attribute(Key const& key, std::string_view description, std::span<int const> preset, HandleInts handler)
{
    (void)key; (void)description; (void)preset; (void)handler;
}

void mlc::IniFile::add_bool_attribute(Key const& key, std::string_view description, bool preset, HandleBool handler)
{
    self->add_key(key, description, (preset ? "true" : "false"),
        [handler](live_config::Key const& key, std::optional<std::string_view> val)
        {
            process_as<bool>(handler, key, val);
        });
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

void mlc::IniFile::add_strings_attribute(Key const& key, std::string_view description, std::span<std::string const> preset, HandleStrings handler)
{
    (void)key; (void)description; (void)preset; (void)handler;
}

void mlc::IniFile::on_done(HandleDone handler) 
{
    (void)handler;
}

void mlc::IniFile::load_file(std::istream& istream, std::filesystem::path const& /*path*/)
{
    self->load_file(istream);
}

// TODO mlc::IniFile implementation here temporarily (needs to move)
////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace testing;

struct LiveConfigIniFile : Test
{
    mlc::IniFile ini_file;

    MOCK_METHOD(void, int_handler, (mlc::Key const& key, std::optional<int> value));
    MOCK_METHOD(void, bool_handler, (mlc::Key const& key, std::optional<bool> value));

    mlc::Key const a_key{"a_scope", "an_int"};
    mlc::Key const another_key{"another_scope", "an_int"};
    mlc::Key const a_true_key{"a_scope", "a_bool"};
    mlc::Key const a_false_key{"a_scope", "another_bool"};

    std::istringstream istream{
        a_key.to_string()+"=42\n" +
        another_key.to_string()+"=64\n" +
        a_true_key.to_string()+"=true\n" +
        a_false_key.to_string()+"=false\n"
    };
};

TEST_F(LiveConfigIniFile, an_integer_value_is_handled)
{
    ini_file.add_int_attribute(a_key, "a scoped int", [this](auto... args) { int_handler(args...); });

    EXPECT_CALL(*this, int_handler(a_key, std::optional<int>{42}));

    ini_file.load_file(istream, std::filesystem::path{});
}

TEST_F(LiveConfigIniFile, another_integer_value_is_handled)
{
    ini_file.add_int_attribute(a_key, "a scoped int", [this](auto... args) { int_handler(args...); });
    ini_file.add_int_attribute(another_key, "another scoped int", [this](auto... args) { int_handler(args...); });

    EXPECT_CALL(*this, int_handler(a_key, std::optional<int>{42}));
    EXPECT_CALL(*this, int_handler(another_key, std::optional<int>{64}));

    ini_file.load_file(istream, std::filesystem::path{});
}

TEST_F(LiveConfigIniFile, a_bad_integer_value_is_handled_as_nullopt)
{
    std::istringstream bad_istream{
        a_key.to_string()+"=42x"};

    ini_file.add_int_attribute(a_key, "a scoped int", [this](auto... args) { int_handler(args...); });

    EXPECT_CALL(*this, int_handler(a_key, {std::nullopt}));

    ini_file.load_file(bad_istream, std::filesystem::path{});
}

TEST_F(LiveConfigIniFile, a_preset_integer_value_is_handled)
{
    mlc::Key const my_key{"my_scope", "thirteen"};

    ini_file.add_int_attribute(my_key, "a scoped int", 13, [this](auto... args) { int_handler(args...); });

    EXPECT_CALL(*this, int_handler(my_key, std::optional<int>{13}));

    ini_file.load_file(istream, std::filesystem::path{});
}

TEST_F(LiveConfigIniFile, a_preset_integer_value_handles_value_from_file)
{
    ini_file.add_int_attribute(a_key, "a scoped int", 13, [this](auto... args) { int_handler(args...); });

    EXPECT_CALL(*this, int_handler(a_key, std::optional<int>{42}));

    ini_file.load_file(istream, std::filesystem::path{});
}

TEST_F(LiveConfigIniFile, a_bool_value_is_handled)
{
    mlc::Key const my_key{"foo"};
    ini_file.add_bool_attribute(a_true_key, "a scoped bool", [this](auto... args) { bool_handler(args...); });
    ini_file.add_bool_attribute(a_false_key, "a scoped bool", [this](auto... args) { bool_handler(args...); });

    EXPECT_CALL(*this, bool_handler(a_true_key, std::optional<bool>{true}));
    EXPECT_CALL(*this, bool_handler(a_false_key, std::optional<bool>{false}));

    ini_file.load_file(istream, std::filesystem::path{});
}

TEST_F(LiveConfigIniFile, a_bool_preset_is_handled)
{
    mlc::Key const my_key{"foo"};
    ini_file.add_bool_attribute(a_true_key, "a scoped bool", false, [this](auto... args) { bool_handler(args...); });
    ini_file.add_bool_attribute(my_key, "a scoped bool", false, [this](auto... args) { bool_handler(args...); });

    EXPECT_CALL(*this, bool_handler(a_true_key, std::optional<bool>{true}));
    EXPECT_CALL(*this, bool_handler(my_key, std::optional<bool>{false}));

    ini_file.load_file(istream, std::filesystem::path{});
}
