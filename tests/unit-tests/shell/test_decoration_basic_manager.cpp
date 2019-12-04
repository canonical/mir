/*
 * Copyright © 2019 Canonical Ltd.
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
 *
 * Authored by: William Wold <william.wold@canonical.com>
 */

#include "src/server/shell/decoration/basic_manager.h"
#include "src/server/shell/decoration/decoration.h"

#include "mir/test/doubles/stub_shell.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace ms = mir::scene;
namespace msh = mir::shell;
namespace msd = mir::shell::decoration;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

using namespace testing;

class StubDecoration
    : public msd::Decoration
{
};

struct DecorationBasicManager
    : Test
{
    void SetUp() override
    {
        manager.init(shell);
        EXPECT_CALL(*this, build_decoration())
            .Times(AnyNumber())
            .WillRepeatedly(Invoke([](){ return new StubDecoration; }));
        EXPECT_CALL(*this, decoration_destroyed(_))
            .Times(AnyNumber());
    }

    // user needs to wrap the raw pointer in a unique_ptr because GTest is dumb
    MOCK_METHOD0(build_decoration, msd::Decoration*());

    MOCK_METHOD1(decoration_destroyed, void(msd::Decoration*));

    msd::BasicManager manager{[this](
            std::shared_ptr<msh::Shell> const&,
            std::shared_ptr<ms::Session> const&,
            std::shared_ptr<ms::Surface> const&) -> std::unique_ptr<msd::Decoration>
        {
            return std::unique_ptr<msd::Decoration>(build_decoration());
        }};
    std::shared_ptr<msh::Shell> shell{std::make_shared<mtd::StubShell>()};
};

class MockDecoration
    : public msd::Decoration
{
public:
    MockDecoration(DecorationBasicManager* mock)
        : mock{mock}
    {
    }

    ~MockDecoration()
    {
        mock->decoration_destroyed(this);
    }

    DecorationBasicManager* const mock;
};

TEST_F(DecorationBasicManager, calls_build_decoration)
{
    auto const session = std::make_shared<mtd::StubSession>();
    auto const surface = std::make_shared<mtd::StubSurface>();
    EXPECT_CALL(*this, build_decoration())
        .Times(1);
    manager.decorate(session, surface);
}

TEST_F(DecorationBasicManager, decorating_multiple_surfaces_is_fine)
{
    auto const session = std::make_shared<mtd::StubSession>();
    auto const surface_a = std::make_shared<mtd::StubSurface>();
    auto const surface_b = std::make_shared<mtd::StubSurface>();
    manager.decorate(session, surface_a);
    manager.decorate(session, surface_b);
}

TEST_F(DecorationBasicManager, decorating_a_surface_is_idempotent)
{
    auto const session = std::make_shared<mtd::StubSession>();
    auto const surface = std::make_shared<mtd::StubSurface>();
    EXPECT_CALL(*this, build_decoration())
        .Times(1);
    manager.decorate(session, surface);
    manager.decorate(session, surface);
    manager.decorate(session, surface);
}

TEST_F(DecorationBasicManager, undecorate_unknown_surface_is_fine)
{
    auto const surface_a = std::make_shared<mtd::StubSurface>();
    auto const surface_b = std::make_shared<mtd::StubSurface>();
    EXPECT_CALL(*this, build_decoration())
        .Times(0);
    manager.undecorate(surface_a);
    manager.undecorate(surface_b);
}

TEST_F(DecorationBasicManager, undecorate_unknown_surface_is_fine_when_there_are_decorations)
{
    auto const session = std::make_shared<mtd::StubSession>();
    auto const surface_a = std::make_shared<mtd::StubSurface>();
    auto const surface_b = std::make_shared<mtd::StubSurface>();
    auto const surface_c = std::make_shared<mtd::StubSurface>();
    auto const surface_d = std::make_shared<mtd::StubSurface>();
    manager.decorate(session, surface_a);
    manager.decorate(session, surface_b);
    manager.undecorate(surface_c);
    manager.undecorate(surface_d);
}

TEST_F(DecorationBasicManager, undecorate_works)
{
    auto decoration_a = std::make_unique<MockDecoration>(this);
    auto decoration_b = std::make_unique<MockDecoration>(this);
    auto const session = std::make_shared<mtd::StubSession>();
    auto const surface_a = std::make_shared<mtd::StubSurface>();
    auto const surface_b = std::make_shared<mtd::StubSurface>();
    EXPECT_CALL(*this, decoration_destroyed(decoration_a.get()))
        .Times(1);
    EXPECT_CALL(*this, decoration_destroyed(decoration_b.get()))
        .Times(0);
    EXPECT_CALL(*this, build_decoration())
        .Times(2)
        .WillOnce(Invoke([&](){ return decoration_a.release(); }))
        .WillOnce(Invoke([&](){ return decoration_b.release(); }));
    manager.decorate(session, surface_a);
    manager.decorate(session, surface_b);
    manager.undecorate(surface_a);
    Mock::VerifyAndClearExpectations(this);
    EXPECT_CALL(*this, decoration_destroyed(_))
        .Times(AnyNumber());
}

TEST_F(DecorationBasicManager, undecorate_all_works)
{
    auto decoration_a = std::make_unique<MockDecoration>(this);
    auto decoration_b = std::make_unique<MockDecoration>(this);
    auto const session = std::make_shared<mtd::StubSession>();
    auto const surface_a = std::make_shared<mtd::StubSurface>();
    auto const surface_b = std::make_shared<mtd::StubSurface>();
    EXPECT_CALL(*this, decoration_destroyed(decoration_a.get()))
        .Times(1);
    EXPECT_CALL(*this, decoration_destroyed(decoration_b.get()))
        .Times(1);
    EXPECT_CALL(*this, build_decoration())
        .Times(2)
        .WillOnce(Invoke([&](){ return decoration_a.release(); }))
        .WillOnce(Invoke([&](){ return decoration_b.release(); }));
    manager.decorate(session, surface_a);
    manager.decorate(session, surface_b);
    manager.undecorate_all();
    Mock::VerifyAndClearExpectations(this);
}

TEST_F(DecorationBasicManager, does_not_build_decorations_while_locked)
{
    auto decoration_a = std::make_unique<MockDecoration>(this);
    auto decoration_b = std::make_unique<MockDecoration>(this);
    auto const session = std::make_shared<mtd::StubSession>();
    auto const surface_a = std::make_shared<mtd::StubSurface>();
    auto const surface_b = std::make_shared<mtd::StubSurface>();
    EXPECT_CALL(*this, build_decoration())
        .Times(2)
        .WillOnce(Invoke([&]()
            {
                manager.decorate(session, surface_b);
                return decoration_a.release();
            }))
        .WillOnce(Invoke([&](){ return decoration_b.release(); }));
    manager.decorate(session, surface_a);
}

TEST_F(DecorationBasicManager, does_not_destroy_decorations_while_locked)
{
    auto decoration_a = std::make_unique<MockDecoration>(this);
    auto decoration_b = std::make_unique<MockDecoration>(this);
    auto const session = std::make_shared<mtd::StubSession>();
    auto const surface_a = std::make_shared<mtd::StubSurface>();
    auto const surface_b = std::make_shared<mtd::StubSurface>();
    EXPECT_CALL(*this, build_decoration())
        .Times(2)
        .WillOnce(Invoke([&](){ return decoration_a.release(); }))
        .WillOnce(Invoke([&](){ return decoration_b.release(); }));
    EXPECT_CALL(*this, decoration_destroyed(decoration_a.get()))
        .Times(1)
        .WillOnce(Invoke([&](auto)
            {
                manager.undecorate(surface_b);
            }));
    EXPECT_CALL(*this, decoration_destroyed(decoration_b.get()))
        .Times(1);
    manager.decorate(session, surface_a);
    manager.decorate(session, surface_b);
    manager.undecorate(surface_a);
    Mock::VerifyAndClearExpectations(this);
}
