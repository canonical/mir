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

class mlc::IniFile::Self
{
public:
    Self() = default;
    
    void add_key(Key const& key, std::string_view description, std::optional<std::string> preset, HandleString handler);
    void add_key(Key const& key, std::string_view description, std::optional<std::vector<std::string>> preset, HandleStrings handler);

    void load_file(std::istream& istream);

private:
    Self(Self const&) = delete;
    Self& operator=(Self const&) = delete;

    struct AttributeDetails
    {
        HandleString const handler;
        std::string const description;
        std::optional<std::string> const preset;
        std::optional<std::string> value;
    };

    struct ArrayAttributeDetails
    {
        HandleStrings const handler;
        std::string const description;
        std::optional<std::vector<std::string>> const preset;
        std::optional<std::vector<std::string>> value;
    };

    std::mutex mutex;
    std::map<Key, AttributeDetails> attribute_handlers;
    std::map<Key, ArrayAttributeDetails> array_attribute_handlers;
    std::list<HandleDone> done_handlers;
};

void mlc::IniFile::Self::add_key(
    Key const& key,
    std::string_view description,
    std::optional<std::string> preset,
    HandleString handler)
{
    std::lock_guard lock{mutex};
    attribute_handlers.emplace(key, AttributeDetails{handler, std::string{description}, preset, std::nullopt});
}

void mlc::IniFile::Self::add_key(Key const& key, std::string_view description,
    std::optional<std::vector<std::string>> preset, HandleStrings handler)
{
    std::lock_guard lock{mutex};
    array_attribute_handlers.emplace(key, ArrayAttributeDetails{handler, std::string{description}, preset, std::nullopt});
}

void mlc::IniFile::Self::load_file(std::istream& istream)
{
    std::lock_guard lock{mutex};

    for (auto& [key, details] : attribute_handlers)
    {
        details.value = std::nullopt;
    }

    for (auto& [key, details] : array_attribute_handlers)
    {
        details.value = std::nullopt;
    }

    for (std::string line; std::getline(istream, line);)
    {
        puts(line.c_str()); // TODO remove printf debugging

        if (line.contains("="))
        {
            auto const eq = line.find_first_of("=");
            auto const key = Key{line.substr(0, eq)};
            auto const value = line.substr(eq+1);

            if (auto const details = attribute_handlers.find(key); details != attribute_handlers.end())
            {
                details->second.value = value;
            }

            if (auto const details = array_attribute_handlers.find(key); details != array_attribute_handlers.end())
            {
                if (details->second.value)
                {
                    details->second.value.value().push_back(value);
                }
                else
                {
                    details->second.value = std::vector<std::string>{value};
                }
            }
        }
    }

    for (auto const& [key, details] : attribute_handlers)
    {
        details.handler(key, (details.value ? details.value : details.preset));
    }


    for (auto const& [key, details] : array_attribute_handlers)
    {
        details.handler(key, (details.value ? details.value : details.preset));
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

template<typename Type>
void process_as(std::function<void(mlc::Key const&, std::optional<std::span<Type>>)> const& handler, mlc::Key const& key,
    std::optional<std::span<std::string const>> val)
{
    if (val)
    {
        std::vector<Type> parsed_vals;

        for (auto const& v : *val)
        {
            Type parsed_val{};
            auto const [end, err] = std::from_chars(v.data(), v.data() + v.size(), parsed_val);

            if ((err == std::errc{}) && (end == v.data() + v.size()))
            {
                parsed_vals.push_back(parsed_val);
            }
            else
            {
                mir::log_warning("Config key '%s' has invalid value: %s", key.to_string().c_str(), v.c_str());
            }
        }

        handler(key, parsed_vals);
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
    self->add_key(key, description, std::nullopt, [handler](Key const& key, std::optional<std::span<std::string const>> val)
    {
        process_as<int>(handler, key, val);
    });
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
    self->add_key(key, description, std::nullopt,
        [handler](live_config::Key const& key, std::optional<std::string_view> val)
    {
        process_as<float>(handler, key, val);
    });
}

void mlc::IniFile::add_floats_attribute(Key const& key, std::string_view description, HandleFloats handler)
{
    (void)key; (void)description; (void)handler;
}

void mlc::IniFile::add_string_attribute(Key const& key, std::string_view description, HandleString handler)
{
    self->add_key(key, description, std::nullopt, handler);
}

void mlc::IniFile::add_strings_attribute(Key const& key, std::string_view description, HandleStrings handler) 
{
    self->add_key(key, description, std::nullopt, handler);
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
    self->add_key(key, description, std::to_string(preset),
        [handler](live_config::Key const& key, std::optional<std::string_view> val)
    {
        process_as<float>(handler, key, val);
    });
}

void mlc::IniFile::add_floats_attribute(Key const& key, std::string_view description, std::span<float const> preset, HandleFloats handler)
{
    (void)key; (void)description; (void)preset; (void)handler;
}

void mlc::IniFile::add_string_attribute(Key const& key, std::string_view description, std::string_view preset, HandleString handler)
{
    self->add_key(key, description, std::string{preset}, handler);
}

void mlc::IniFile::add_strings_attribute(Key const& key, std::string_view description, std::span<std::string const> preset, HandleStrings handler)
{
    self->add_key(key, description, std::vector<std::string>{preset.begin(), preset.end()}, handler);
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
using namespace std::string_literals;

struct LiveConfigIniFile : Test
{
    mlc::IniFile ini_file;

    MOCK_METHOD(void, int_handler, (mlc::Key const& key, std::optional<int> value));
    MOCK_METHOD(void, bool_handler, (mlc::Key const& key, std::optional<bool> value));
    MOCK_METHOD(void, float_handler, (mlc::Key const& key, std::optional<float> value));
    MOCK_METHOD(void, string_handler, (mlc::Key const& key, std::optional<std::string_view const> value));
    MOCK_METHOD(void, strings_handler, (mlc::Key const& key, std::optional<std::span<std::string const>> value));
    MOCK_METHOD(void, ints_handler, (mlc::Key const& key, std::optional<std::span<int const>> value));

    mlc::Key const a_key{"a_scope", "an_int"};
    mlc::Key const another_key{"another_scope", "an_int"};
    mlc::Key const a_true_key{"a_scope", "a_bool"};
    mlc::Key const a_false_key{"a_scope", "another_bool"};
    mlc::Key const a_string_key{"a_string"};
    mlc::Key const another_string_key{"another_string"};
    mlc::Key const a_real_key{"a_float"};
    mlc::Key const another_real_key{"another_float"};
    mlc::Key const a_strings_key{"strings"};
    mlc::Key const another_strings_key{"more_strings"};
    mlc::Key const an_ints_key{"ints"};
    mlc::Key const another_ints_key{"more_ints"};

    std::istringstream istream{
        a_key.to_string()+"=42\n" +
        another_key.to_string()+"=64\n" +
        a_true_key.to_string()+"=true\n" +
        a_false_key.to_string()+"=false\n" +
        a_string_key.to_string()+"=foo\n" +
        another_string_key.to_string()+"=bar baz\n" +
        a_real_key.to_string()+"=3.5\n" +
        another_real_key.to_string()+"=1e2\n" +
        a_strings_key.to_string()+"=foo\n" +
        a_strings_key.to_string()+"=bar\n" +
        another_strings_key.to_string()+"=foo bar\n" +
        another_strings_key.to_string()+"=baz\n" +
        an_ints_key.to_string()+"=1\n" +
        an_ints_key.to_string()+"=2\n" +
        another_ints_key.to_string()+"=3\n" +
        another_ints_key.to_string()+"=5\n"
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

TEST_F(LiveConfigIniFile, a_string_value_is_handled)
{
    ini_file.add_string_attribute(a_string_key, "a scoped string", [this](auto... args) { string_handler(args...); });
    ini_file.add_string_attribute(another_string_key, "a scoped string", [this](auto... args) { string_handler(args...); });

    EXPECT_CALL(*this, string_handler(a_string_key, std::optional<std::string_view const>{"foo"}));
    EXPECT_CALL(*this, string_handler(another_string_key, std::optional<std::string_view const>{"bar baz"}));

    ini_file.load_file(istream, std::filesystem::path{});
}

TEST_F(LiveConfigIniFile, a_string_preset_is_handled)
{
    mlc::Key const my_key{"foo"};
    ini_file.add_string_attribute(a_string_key, "a scoped string", "ignored", [this](auto... args) { string_handler(args...); });
    ini_file.add_string_attribute(my_key, "a scoped string", "(none)", [this](auto... args) { string_handler(args...); });

    EXPECT_CALL(*this, string_handler(a_string_key, std::optional<std::string_view const>{"foo"}));
    EXPECT_CALL(*this, string_handler(my_key, std::optional<std::string_view const>{"(none)"}));

    ini_file.load_file(istream, std::filesystem::path{});
}

TEST_F(LiveConfigIniFile, a_float_value_is_handled)
{
    ini_file.add_float_attribute(a_real_key, "a float", [this](auto... args) { float_handler(args...); });
    ini_file.add_float_attribute(another_real_key, "another float", [this](auto... args) { float_handler(args...); });

    EXPECT_CALL(*this, float_handler(a_real_key, {3.5}));
    EXPECT_CALL(*this, float_handler(another_real_key, {100.0}));

    ini_file.load_file(istream, std::filesystem::path{});
}

TEST_F(LiveConfigIniFile, a_float_preset_is_handled)
{
    mlc::Key const my_key{"foo"};
    ini_file.add_float_attribute(a_real_key, "a float", -7.0, [this](auto... args) { float_handler(args...); });
    ini_file.add_float_attribute(my_key, "a scoped bool", 23.0, [this](auto... args) { float_handler(args...); });

    EXPECT_CALL(*this, float_handler(a_real_key, {3.5}));
    EXPECT_CALL(*this, float_handler(my_key, {23}));

    ini_file.load_file(istream, std::filesystem::path{});
}

TEST_F(LiveConfigIniFile, a_strings_value_is_handled)
{
    ini_file.add_strings_attribute(a_strings_key, "strings", [this](auto... args) { strings_handler(args...); });
    ini_file.add_strings_attribute(another_strings_key, "more strings", [this](auto... args) { strings_handler(args...); });

    EXPECT_CALL(*this, strings_handler(a_strings_key, Optional(ElementsAre("foo", "bar"))));
    EXPECT_CALL(*this, strings_handler(another_strings_key, Optional(ElementsAre("foo bar", "baz"))));

    ini_file.load_file(istream, std::filesystem::path{});
}

TEST_F(LiveConfigIniFile, a_strings_preset_is_handled)
{
    mlc::Key const my_key{"foo"};
    ini_file.add_strings_attribute(a_strings_key, "strings", {{"ignored"s}}, [this](auto... args) { strings_handler(args...); });
    ini_file.add_strings_attribute(my_key, "a scoped string", {{"(none)"s}}, [this](auto... args) { strings_handler(args...); });

    EXPECT_CALL(*this, strings_handler(a_strings_key, Optional(ElementsAre("foo", "bar"))));
    EXPECT_CALL(*this, strings_handler(my_key, Optional(ElementsAre("(none)"))));

    ini_file.load_file(istream, std::filesystem::path{});
}

TEST_F(LiveConfigIniFile, an_ints_value_is_handled)
{
    ini_file.add_ints_attribute(an_ints_key, "ints", [this](auto... args) { ints_handler(args...); });
    ini_file.add_ints_attribute(another_ints_key, "more ints", [this](auto... args) { ints_handler(args...); });

    EXPECT_CALL(*this, ints_handler(an_ints_key, Optional(ElementsAre(1, 2))));
    EXPECT_CALL(*this, ints_handler(another_ints_key, Optional(ElementsAre(3, 5))));

    ini_file.load_file(istream, std::filesystem::path{});
}
