/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "src/server/frontend/message_sender.h"
#include "src/server/frontend/buffering_message_sender.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mf = mir::frontend;

class MockMessageSender : public mf::MessageSender
{
public:
    MOCK_METHOD3(send, void(char const*, size_t, mf::FdSets const&));
};

TEST(BufferingMessageSender, sends_no_message_before_being_uncorked)
{
    using namespace testing;
    auto mock_sender = std::make_shared<NiceMock<MockMessageSender>>();

    mf::BufferingMessageSender sender{mock_sender};

    bool messages_sent{false};
    ON_CALL(*mock_sender, send(_,_,_))
        .WillByDefault(InvokeWithoutArgs([&messages_sent] { messages_sent = true;}));

    std::array<char, 44> data;
    sender.send(data.data(), data.size(), {});

    EXPECT_FALSE(messages_sent);
}

TEST(BufferingMessageSender, sends_messages_in_order_when_uncorked)
{
    using namespace testing;
    auto mock_sender = std::make_shared<NiceMock<MockMessageSender>>();

    std::vector<std::vector<char>> messages_sent;
    ON_CALL(*mock_sender, send(_,_,_))
        .WillByDefault(Invoke(
            [&messages_sent](char const* data, size_t length, auto)
            {
                messages_sent.emplace_back(std::vector<char>(data, data + length));
            }));

    mf::BufferingMessageSender sender{mock_sender};

    std::array<std::vector<char>, 9> messages;
    for (auto& message : messages)
    {
        auto content = std::to_string(reinterpret_cast<intptr_t>(&message));
        message = std::vector<char>(content.begin(), content.end());
    }

    for (auto const& message : messages)
    {
        sender.send(message.data(), message.size(), {});
    }

    ASSERT_THAT(messages_sent, IsEmpty());

    sender.uncork();

    EXPECT_THAT(messages_sent, ElementsAreArray(messages.data(), messages.size()));
}

TEST(BufferingMessageSender, messages_sent_after_uncork_proceed_normally)
{
    using namespace testing;
    auto mock_sender = std::make_shared<NiceMock<MockMessageSender>>();

    std::vector<std::vector<char>> messages_sent;
    ON_CALL(*mock_sender, send(_,_,_))
        .WillByDefault(Invoke(
            [&messages_sent](char const* data, size_t length, auto)
            {
                messages_sent.emplace_back(std::vector<char>(data, data + length));
            }));

    mf::BufferingMessageSender sender{mock_sender};

    std::array<std::vector<char>, 9> messages;
    for (auto& message : messages)
    {
        auto content = std::to_string(reinterpret_cast<intptr_t>(&message));
        message = std::vector<char>(content.begin(), content.end());
    }

    sender.uncork();

    for (auto const& message : messages)
    {
        sender.send(message.data(), message.size(), {});

        EXPECT_THAT(messages_sent.back(), Eq(message));
    }
}

TEST(BufferingMessageSender, calling_uncorked_twice_is_harmless)
{
    using namespace testing;
    auto mock_sender = std::make_shared<NiceMock<MockMessageSender>>();

    std::vector<std::vector<char>> messages_sent;
    ON_CALL(*mock_sender, send(_,_,_))
        .WillByDefault(Invoke(
            [&messages_sent](char const* data, size_t length, auto)
            {
                messages_sent.emplace_back(std::vector<char>(data, data + length));
            }));

    mf::BufferingMessageSender sender{mock_sender};

    std::array<std::vector<char>, 9> messages;
    for (auto& message : messages)
    {
        auto content = std::to_string(reinterpret_cast<intptr_t>(&message));
        message = std::vector<char>(content.begin(), content.end());
    }

    for (auto const& message : messages)
    {
        sender.send(message.data(), message.size(), {});
    }

    ASSERT_THAT(messages_sent, IsEmpty());

    sender.uncork();
    sender.uncork();

    EXPECT_THAT(messages_sent, ElementsAreArray(messages.data(), messages.size()));
}
