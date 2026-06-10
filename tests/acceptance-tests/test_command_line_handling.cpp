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

#include <mir_test_framework/headless_test.h>
#include <mir/log.h>
#include <mir/logging/logger.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <sys/ucontext.h>

using namespace testing;

class MockLogger : public mir::logging::Logger
{
public:
    MOCK_METHOD(void, log, (mir::logging::Severity severity, std::string const&message, std::string const& component), (override));
};

struct CommandLineHandling : mir_test_framework::HeadlessTest
{
    MOCK_METHOD(void, callback, (std::vector<char const*> const&), ());

    std::function<void(int argc, char const* const* argv)> callback_functor =
        [this](int argc, char const* const* argv)
        {
            // Copy to a vector as ElementsAre() is convenient for checking
            std::vector<char const*> const copy{argv, argv+argc};
            callback(copy);
        };
};

TEST_F(CommandLineHandling, valid_options_are_accepted_by_default_callback)
{
    char const* argv[] =
     { "dummy-exe-name", "--enable-input", "off"};

    int const argc = std::distance(std::begin(argv), std::end(argv));

    server.set_command_line(argc, argv);

    server.apply_settings();
}

TEST_F(CommandLineHandling, unrecognised_tokens_cause_default_callback_to_throw)
{
    char const* argv[] =
     { "dummy-exe-name", "--file", "test", "--hello", "world", "--enable-input", "off"};

    int const argc = std::distance(std::begin(argv), std::end(argv));

    server.set_command_line(argc, argv);

    EXPECT_THROW(server.apply_settings(), std::runtime_error);
}

TEST_F(CommandLineHandling, valid_options_are_not_passed_to_callback)
{
    char const* argv[] =
     { "dummy-exe-name", "--enable-input", "off"};

    int const argc = std::distance(std::begin(argv), std::end(argv));

    server.set_command_line_handler(callback_functor);
    server.set_command_line(argc, argv);

    EXPECT_CALL(*this, callback(_)).Times(Exactly(0));

    server.apply_settings();
}

TEST_F(CommandLineHandling, unrecognised_tokens_are_passed_to_callback)
{
    char const* argv[] =
     { "dummy-exe-name", "--hello", "world", "--enable-input", "off"};

    int const argc = std::distance(std::begin(argv), std::end(argv));

    server.set_command_line_handler(callback_functor);
    server.set_command_line(argc, argv);

    EXPECT_CALL(*this, callback(ElementsAre(StrEq("--hello"), StrEq("world"))));

    server.apply_settings();
}

namespace ml = mir::logging;

TEST_F(CommandLineHandling, invalid_log_levels_are_ignored)
{
    char const* argv[] =
     { "dummy-exe-name",
       "--log-level=base",
       "--log-level=not_a_tag=info",
       "--log-level=input=not_a_severity",
       "--log-level=graphics=",};

    int const argc = std::distance(std::begin(argv), std::end(argv));

    server.set_command_line(argc, argv);

    server.apply_settings();

    server.get_options();
}

TEST_F(CommandLineHandling, can_set_multiple_different_log_levels)
{
    using namespace testing;
    char const* argv[] =
     { "dummy-exe-name", "--log-level=input=error", "--log-level=wayland=info"};

    int const argc = std::distance(std::begin(argv), std::end(argv));

    auto const logger = std::make_shared<testing::NiceMock<MockLogger>>();
    server.override_the_logger([logger]() { return logger; });
    server.set_command_line(argc, argv);
    server.apply_settings();

    server.get_options();

    std::string message = "hello";

    EXPECT_CALL(*logger, log(ml::Severity::error, message, "input")).Times(1);
    EXPECT_CALL(*logger, log(ml::Severity::warning, message, "input")).Times(0);
    EXPECT_CALL(*logger, log(ml::Severity::informational, message, "wayland")).Times(1);
    EXPECT_CALL(*logger, log(ml::Severity::debug, message, "wayland")).Times(0);

    mir::log_error({ml::input()}, message);
    mir::log_warning({ml::input()}, message);

    mir::log_info({ml::wayland()}, message);
    mir::log_debug({ml::wayland()}, message);

    // Replace Mir's logger so that we own the last references to our MockLogger
    // allowing it to be destroyed and verified after the test finishes.
    mir::logging::set_logger(std::make_shared<MockLogger>());
}

TEST_F(CommandLineHandling, setting_parent_severity_sets_child_severities)
{
    using namespace testing;
    char const* argv[] =
     { "dummy-exe-name", "--log-level=parent=error"};

    int const argc = std::distance(std::begin(argv), std::end(argv));

    auto const& parent = ml::create_tag(ml::base(), "parent");
    auto const& child1 = ml::create_tag(parent, "dick");
    auto const& child2 = ml::create_tag(parent, "jane");

    auto const logger = std::make_shared<testing::NiceMock<MockLogger>>();
    server.override_the_logger([logger]() { return logger; });
    server.set_command_line(argc, argv);
    server.apply_settings();

    server.get_options();

    std::string message = "hello";
    EXPECT_CALL(*logger, log(ml::Severity::error, _, _)).Times(3);
    EXPECT_CALL(*logger, log(ml::Severity::warning, _, _)).Times(0);

    mir::log_error({parent}, message);
    mir::log_error({child1}, message);
    mir::log_error({child2}, message);

    mir::log_warning({parent}, message);
    mir::log_warning({child1}, message);
    mir::log_warning({child2}, message);

    // Replace Mir's logger so that we own the last references to our MockLogger
    // allowing it to be destroyed and verified after the test finishes.
    mir::logging::set_logger(std::make_shared<MockLogger>());
}

TEST_F(CommandLineHandling, subsequent_log_level_overrides_previous)
{
    using namespace testing;
    char const* argv[] =
     { "dummy-exe-name", "--log-level=base=critical", "--log-level=test_tag=debug"};

    auto const& tag = ml::create_tag(ml::base(), "test_tag");

    int const argc = std::distance(std::begin(argv), std::end(argv));

    auto const logger = std::make_shared<testing::NiceMock<MockLogger>>();
    server.override_the_logger([logger]() { return logger; });
    server.set_command_line(argc, argv);
    server.apply_settings();

    server.get_options();


    std::string message = "hello";
    EXPECT_CALL(*logger, log(ml::Severity::debug, message, "test_tag")).Times(1);
    EXPECT_CALL(*logger, log(_, _, "base")).Times(0);

    mir::log_debug({tag}, message);
    mir::log_error({ml::base()}, message);

    // Replace Mir's logger so that we own the last references to our MockLogger
    // allowing it to be destroyed and verified after the test finishes.
    mir::logging::set_logger(std::make_shared<MockLogger>());
}

TEST_F(CommandLineHandling, can_set_log_level_by_full_path)
{
    using namespace testing;
    char const* argv[] =
     { "dummy-exe-name", "--log-level", "base/graphics=debug"};

    int const argc = std::distance(std::begin(argv), std::end(argv));

    auto const logger = std::make_shared<testing::NiceMock<MockLogger>>();
    server.override_the_logger([logger]() { return logger; });
    server.set_command_line(argc, argv);
    server.apply_settings();

    server.get_options();


    std::string message = "hello";

    EXPECT_CALL(*logger, log(ml::Severity::debug, message, "graphics")).Times(1);
    EXPECT_CALL(*logger, log(_, _, "wayland")).Times(0);

    mir::log_debug({ml::graphics()}, message);
    // Validate that our default log level does produce output at debug level.
    mir::log_debug({ml::wayland()}, message);

    // Replace Mir's logger so that we own the last references to our MockLogger
    // allowing it to be destroyed and verified after the test finishes.
    mir::logging::set_logger(std::make_shared<MockLogger>());
}
