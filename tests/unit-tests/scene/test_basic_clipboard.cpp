/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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

#include "src/server/scene/basic_clipboard.h"
#include "mir/test/fake_shared.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

namespace ms = mir::scene;
namespace mt = mir::test;

struct MockClipboardSource : ms::DataExchangeSource
{
    auto mime_types() const -> std::vector<std::string> const& override
    {
        return types;
    }

    void initiate_send(std::string const&, mir::Fd const&) override
    {
    }

    void cancelled() override {}
    void dnd_drop_performed() override {}
    auto actions() -> uint32_t override { return 0; }
    void offer_accepted(const std::optional<std::string>&) override {}
    uint32_t offer_set_actions(uint32_t, uint32_t) override { return 0; }
    void dnd_finished() override {}

    std::vector<std::string> types;
};

struct MockClipboardObserver : ms::ClipboardObserver
{
    MOCK_METHOD(void, paste_source_set, (std::shared_ptr<ms::DataExchangeSource> const&));

    void drag_n_drop_source_set(const std::shared_ptr<ms::DataExchangeSource>&) override {}
    void drag_n_drop_source_cleared(const std::shared_ptr<ms::DataExchangeSource>&) override {}
    void end_of_dnd_gesture() override {}
};

struct BasicClipboardTest : Test
{
    ms::BasicClipboard clipboard;
    StrictMock<MockClipboardObserver> observer;
    std::shared_ptr<MockClipboardObserver> observer_ptr = mt::fake_shared(observer);
    MockClipboardSource mock_source_a;
    std::shared_ptr<MockClipboardSource> source_a = mt::fake_shared(mock_source_a);
    MockClipboardSource mock_source_b;
    std::shared_ptr<MockClipboardSource> source_b = mt::fake_shared(mock_source_b);
};

TEST_F(BasicClipboardTest, paste_source_correct_can_be_set)
{
    clipboard.set_paste_source(source_a);
    EXPECT_THAT(clipboard.paste_source(), Eq(source_a));
}

TEST_F(BasicClipboardTest, paste_source_can_be_changed)
{
    clipboard.set_paste_source(source_a);
    clipboard.set_paste_source(source_b);
    EXPECT_THAT(clipboard.paste_source(), Eq(source_b));
}

TEST_F(BasicClipboardTest, paste_source_can_be_cleared)
{
    clipboard.set_paste_source(source_a);
    clipboard.clear_paste_source();
    EXPECT_THAT(clipboard.paste_source(), Eq(nullptr));
}

TEST_F(BasicClipboardTest, paste_source_can_be_cleared_with_value)
{
    clipboard.set_paste_source(source_a);
    clipboard.clear_paste_source(*source_a);
    EXPECT_THAT(clipboard.paste_source(), Eq(nullptr));
}

TEST_F(BasicClipboardTest, clearing_old_paste_source_does_nothing)
{
    clipboard.set_paste_source(source_a);
    clipboard.set_paste_source(source_b);
    clipboard.clear_paste_source(*source_a);
    EXPECT_THAT(clipboard.paste_source(), Eq(source_b));
}

TEST_F(BasicClipboardTest, observer_notified_of_paste_source_set)
{
    Sequence seq;
    EXPECT_CALL(observer, paste_source_set(Eq(nullptr)));
    EXPECT_CALL(observer, paste_source_set(Eq(source_a)));

    clipboard.register_interest(observer_ptr);
    clipboard.set_paste_source(source_a);
}

TEST_F(BasicClipboardTest, observer_notified_of_paste_source_change)
{
    clipboard.set_paste_source(source_a);

    Sequence seq;
    EXPECT_CALL(observer, paste_source_set(Eq(source_a)));
    EXPECT_CALL(observer, paste_source_set(Eq(source_b)));

    clipboard.register_interest(observer_ptr);
    clipboard.set_paste_source(source_b);
}

TEST_F(BasicClipboardTest, observer_notified_of_paste_source_cleared)
{
    clipboard.set_paste_source(source_a);

    Sequence seq;
    EXPECT_CALL(observer, paste_source_set(Eq(nullptr)));
    EXPECT_CALL(observer, paste_source_set(Eq(source_a)));

    clipboard.register_interest(observer_ptr);
    clipboard.clear_paste_source();
}

TEST_F(BasicClipboardTest, observer_notified_of_paste_source_cleared_with_value)
{
    clipboard.set_paste_source(source_a);

    Sequence seq;
    EXPECT_CALL(observer, paste_source_set(Eq(source_a)));
    EXPECT_CALL(observer, paste_source_set(Eq(nullptr)));

    clipboard.register_interest(observer_ptr);
    clipboard.clear_paste_source(*source_a);
}

TEST_F(BasicClipboardTest, observer_not_notified_when_old_paste_source_cleared)
{
    clipboard.set_paste_source(source_a);
    clipboard.set_paste_source(source_b);

    EXPECT_CALL(observer, paste_source_set(Eq(source_b)));

    clipboard.register_interest(observer_ptr);
    // a is not longer the current paste source, so nothing should happen
    clipboard.clear_paste_source(*source_a);
}

TEST_F(BasicClipboardTest, can_set_paste_source_from_paste_source_observer_callback)
{
    EXPECT_CALL(observer, paste_source_set(_))
        .WillOnce([&](auto source)
        {
            EXPECT_THAT(source, Eq(nullptr));
        })
        .WillOnce([&](auto)
        {
            clipboard.set_paste_source(source_b);
        })
        .WillOnce([&](auto source)
        {
            EXPECT_THAT(source, Eq(source_b));
        });
    clipboard.register_interest(observer_ptr);
    clipboard.set_paste_source(source_a);
}
