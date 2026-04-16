/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

#include <miral/config_aggregator.h>
#include <miral/config_file_store_adapter.h>
#include <miral/live_config_ini_file.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <filesystem>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

using namespace testing;
namespace mlc = miral::live_config;

namespace
{

auto make_source_factory()
{
    return [](std::unique_ptr<std::istream> stream, std::filesystem::path const& path)
        -> mlc::ConfigAggregator::Source
    {
        auto ini = std::make_shared<mlc::IniFile>();
        // Move stream into the load closure so it stays alive for deferred reloads.
        auto shared_stream = std::shared_ptr<std::istream>(std::move(stream));
        return {
            ini,
            [ini, shared_stream, p = path]() { ini->load_file(*shared_stream, p); },
            path,
        };
    };
}

} // namespace

struct ConfigFileStoreAdapterTest : Test
{
    mlc::ConfigAggregator aggregator{};
    miral::ConfigFileStoreAdapter adapter{aggregator, make_source_factory()};

    mlc::Key const int_key{"scope", "an_int"};
    mlc::Key const string_key{"scope", "a_string"};

    MOCK_METHOD(void, int_handler, (mlc::Key const& key, std::optional<int> value));
    MOCK_METHOD(void, string_handler, (mlc::Key const& key, std::optional<std::string_view> value));

    void SetUp() override
    {
        aggregator.add_int_attribute(
            int_key,
            "a scoped int",
            [this](auto const& key, auto value) { int_handler(key, value); });

        aggregator.add_string_attribute(
            string_key,
            "a scoped string",
            [this](auto const& key, auto value) { string_handler(key, value); });
    }

    /// Builds the span-compatible vector from (content, path) pairs and invokes the adapter.
    void call_adapter(std::vector<std::pair<std::string, std::filesystem::path>> entries)
    {
        std::vector<std::pair<std::unique_ptr<std::istream>, std::filesystem::path>> arg;
        arg.reserve(entries.size());
        for (auto& [content, path] : entries)
            arg.emplace_back(std::make_unique<std::istringstream>(std::move(content)), path);

        adapter(arg);
    }
};

TEST_F(ConfigFileStoreAdapterTest, empty_span_is_a_no_op)
{
    EXPECT_CALL(*this, int_handler(_, _)).Times(0);
    EXPECT_CALL(*this, string_handler(_, _)).Times(0);

    call_adapter({});
}

TEST_F(ConfigFileStoreAdapterTest, base_config_is_loaded_on_first_call)
{
    EXPECT_CALL(*this, int_handler(_, Optional(42)));

    call_adapter({{"scope_an_int=42\n", "base.conf"}});
}

TEST_F(ConfigFileStoreAdapterTest, base_config_is_updated_on_subsequent_calls)
{
    {
        InSequence seq;
        EXPECT_CALL(*this, int_handler(_, Optional(1)));
        EXPECT_CALL(*this, int_handler(_, Optional(2)));
    }

    call_adapter({{"scope_an_int=1\n", "base.conf"}});
    call_adapter({{"scope_an_int=2\n", "base.conf"}});
}

TEST_F(ConfigFileStoreAdapterTest, single_override_takes_precedence_over_base)
{
    EXPECT_CALL(*this, int_handler(_, Optional(99)));

    call_adapter({
        {"scope_an_int=1\n",  "base.conf"},
        {"scope_an_int=99\n", "override.conf"},
    });
}

TEST_F(ConfigFileStoreAdapterTest, later_override_takes_precedence_over_earlier_override)
{
    EXPECT_CALL(*this, int_handler(_, Optional(200)));

    call_adapter({
        {"scope_an_int=1\n",   "base.conf"},
        {"scope_an_int=100\n", "override_a.conf"},
        {"scope_an_int=200\n", "override_b.conf"},
    });
}

TEST_F(ConfigFileStoreAdapterTest, removed_override_falls_back_to_base)
{
    {
        InSequence seq;
        EXPECT_CALL(*this, int_handler(_, Optional(99)));
        EXPECT_CALL(*this, int_handler(_, Optional(1)));
    }

    call_adapter({
        {"scope_an_int=1\n",  "base.conf"},
        {"scope_an_int=99\n", "override.conf"},
    });
    call_adapter({{"scope_an_int=1\n", "base.conf"}});
}

TEST_F(ConfigFileStoreAdapterTest, new_override_added_on_second_call)
{
    {
        InSequence seq;
        EXPECT_CALL(*this, int_handler(_, Optional(1)));
        EXPECT_CALL(*this, int_handler(_, Optional(99)));
    }

    call_adapter({{"scope_an_int=1\n", "base.conf"}});
    call_adapter({
        {"scope_an_int=1\n",  "base.conf"},
        {"scope_an_int=99\n", "override.conf"},
    });
}

TEST_F(ConfigFileStoreAdapterTest, retained_override_is_updated)
{
    {
        InSequence seq;
        EXPECT_CALL(*this, int_handler(_, Optional(10)));
        EXPECT_CALL(*this, int_handler(_, Optional(20)));
    }

    call_adapter({
        {"scope_an_int=1\n",  "base.conf"},
        {"scope_an_int=10\n", "override.conf"},
    });
    call_adapter({
        {"scope_an_int=1\n",  "base.conf"},
        {"scope_an_int=20\n", "override.conf"},
    });
}

TEST_F(ConfigFileStoreAdapterTest, base_and_override_independent_keys)
{
    EXPECT_CALL(*this, int_handler(_, Optional(99)));
    EXPECT_CALL(*this, string_handler(_, Optional(std::string_view{"hello"})));

    call_adapter({
        {"scope_an_int=1\nscope_a_string=hello\n", "base.conf"},
        {"scope_an_int=99\n",                       "override.conf"},
    });
}