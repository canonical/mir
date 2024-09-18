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

#include "mir/scene/surface.h"
#include "mir/shell/decoration.h"
#include "mir/shell/decoration_manager.h"
#include "src/server/shell/decoration/basic_manager.h"

#include "mir/test/doubles/stub_observer_registrar.h"
#include "mir/test/doubles/stub_shell.h"

#include "gmock/gmock.h"
#include <boost/iostreams/detail/buffer.hpp>
#include <functional>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>

namespace ms = mir::scene;
namespace msh = mir::shell;
namespace msd = mir::shell::decoration;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

using namespace testing;

struct DestructionNotifier
{
    virtual ~DestructionNotifier() = default;
    virtual void decoration_destroyed(msd::Decoration*) = 0;
};

class MockDecoration
    : public msd::Decoration
{
public:
    MockDecoration(DestructionNotifier* mock)
        : mock{mock}
    {
    }

    ~MockDecoration()
    {
        if(mock)
            mock->decoration_destroyed(this);
    }

    MOCK_METHOD(void, handle_input_event, (std::shared_ptr<MirEvent const> const& /*event*/), (override));
    MOCK_METHOD(void, set_scale, (float), (override));
    MOCK_METHOD(void, attrib_changed, (mir::scene::Surface const* window_surface, MirWindowAttrib attrib, int value), (override));
    MOCK_METHOD(void, window_resized_to, (mir::scene::Surface const* window_surface, mir::geometry::Size const& window_size), (override));
    MOCK_METHOD(void, window_renamed, (mir::scene::Surface const* window_surface, std::string const& name), (override));

    DestructionNotifier* const mock;
};

class MockDecorationManager: public msd::BasicManager
{
public:
    MockDecorationManager(auto& registrar, DestructionNotifier* mock) :
        msd::BasicManager{registrar, nullptr, nullptr, nullptr}, mock{mock}
    {
    }

    auto decorated() -> std::unordered_map<mir::scene::Surface*, std::unique_ptr<msd::Decoration>>&
    {
        return decorations;
    }

    auto add_decoration(
        std::shared_ptr<mir::scene::Surface> const& surf)
    {
        decorations.emplace(surf.get(), std::make_unique<MockDecoration>(mock));
    }

    auto add_decoration(
        std::shared_ptr<mir::scene::Surface> const& surf, std::unique_ptr<msd::Decoration> deco)
    {
        decorations.emplace(surf.get(), std::move(deco));
    }

    MOCK_METHOD(void, decorate, (std::shared_ptr<mir::scene::Surface> const&), (override));

private:
    DestructionNotifier* const mock;
};

struct DecorationBasicManager
    : Test, DestructionNotifier
{
    void SetUp() override
    {
        manager.init(shell);
        EXPECT_CALL(manager, decorate)
            .Times(AnyNumber())
            .WillRepeatedly(
                [&](auto const& surface)
                {
                    manager.add_decoration(surface);
                });
        EXPECT_CALL(*this, decoration_destroyed(_))
            .Times(AnyNumber());
    }

    // user needs to wrap the raw pointer in a unique_ptr because GTest is dumb
    MOCK_METHOD(msd::Decoration*, create_decoration, ());
    MOCK_METHOD(void, decoration_destroyed, (msd::Decoration*), (override));

    std::shared_ptr<mir::ObserverRegistrar<mir::graphics::DisplayConfigurationObserver>>
        registrar{std::make_shared<mtd::StubObserverRegistrar<mir::graphics::DisplayConfigurationObserver>>()};

    MockDecorationManager manager{*registrar, this};
    std::shared_ptr<msh::Shell> shell{std::make_shared<mtd::StubShell>()};
};


TEST_F(DecorationBasicManager, calls_create_decoration)
{
    auto const surface = std::make_shared<mtd::StubSurface>();
    EXPECT_CALL(manager, decorate)
        .Times(1)
        .WillOnce(
            [&](auto& surface)
            {
                manager.add_decoration(surface);
            });
    manager.decorate(surface);
    EXPECT_EQ(manager.decorated().size(), 1);
}

TEST_F(DecorationBasicManager, decorating_multiple_surfaces_is_fine)
{
    auto const surface_a = std::make_shared<mtd::StubSurface>();
    auto const surface_b = std::make_shared<mtd::StubSurface>();
    manager.decorate(surface_a);
    manager.decorate(surface_b);
}

TEST_F(DecorationBasicManager, decorating_a_surface_is_idempotent)
{
    auto const surface = std::make_shared<mtd::StubSurface>();
    EXPECT_CALL(manager, decorate)
        .Times(3)
        .WillRepeatedly(
            [&](auto& surface)
            {
                manager.add_decoration(surface);
            });
    manager.decorate(surface);
    manager.decorate(surface);
    manager.decorate(surface);
    EXPECT_EQ(manager.decorated().size(), 1);
}

TEST_F(DecorationBasicManager, undecorate_unknown_surface_is_fine)
{
    auto const surface_a = std::make_shared<mtd::StubSurface>();
    auto const surface_b = std::make_shared<mtd::StubSurface>();
    EXPECT_CALL(manager, decorate)
        .Times(0);
    manager.undecorate(surface_a);
    manager.undecorate(surface_b);
}

TEST_F(DecorationBasicManager, undecorate_unknown_surface_is_fine_when_there_are_decorations)
{
    auto const surface_a = std::make_shared<mtd::StubSurface>();
    auto const surface_b = std::make_shared<mtd::StubSurface>();
    auto const surface_c = std::make_shared<mtd::StubSurface>();
    auto const surface_d = std::make_shared<mtd::StubSurface>();
    manager.decorate(surface_a);
    manager.decorate(surface_b);
    manager.undecorate(surface_c);
    manager.undecorate(surface_d);
}

TEST_F(DecorationBasicManager, undecorate_works)
{
    auto const surface_a = std::make_shared<mtd::StubSurface>();
    auto const surface_b = std::make_shared<mtd::StubSurface>();
    EXPECT_CALL(manager, decorate)
        .Times(2)
        .WillRepeatedly(
            [&](auto& surface)
            {
                manager.add_decoration(surface);
            });
    manager.decorate(surface_a);
    manager.decorate(surface_b);
    EXPECT_CALL(*this, decoration_destroyed(_))
        .Times(Exactly(1));
    manager.undecorate(surface_a);
    Mock::VerifyAndClearExpectations(this);
}

TEST_F(DecorationBasicManager, undecorate_all_works)
{
    auto decoration_a = std::make_unique<MockDecoration>(this);
    auto decoration_b = std::make_unique<MockDecoration>(this);
    auto const surface_a = std::make_shared<mtd::StubSurface>();
    auto const surface_b = std::make_shared<mtd::StubSurface>();
    EXPECT_CALL(*this, decoration_destroyed(decoration_a.get()))
        .Times(1);
    EXPECT_CALL(*this, decoration_destroyed(decoration_b.get()))
        .Times(1);
    EXPECT_CALL(manager, decorate)
        .Times(2)
        .WillOnce(
            [&](auto& surface)
            {
                manager.add_decoration(surface, std::move(decoration_a));
            })
        .WillOnce(
            [&](auto& surface)
            {
                manager.add_decoration(surface, std::move(decoration_b));
            });
    manager.decorate(surface_a);
    manager.decorate(surface_b);
    manager.undecorate_all();
    Mock::VerifyAndClearExpectations(this);
}

TEST_F(DecorationBasicManager, does_not_create_decorations_while_locked)
{
    auto decoration_a = std::make_unique<MockDecoration>(this);
    auto decoration_b = std::make_unique<MockDecoration>(this);
    auto const surface_a = std::make_shared<mtd::StubSurface>();
    auto const surface_b = std::make_shared<mtd::StubSurface>();
    EXPECT_CALL(manager, decorate)
        .Times(2)
        .WillOnce(Invoke(
            [&]()
            {
                manager.decorate(surface_b); // 2
                manager.decorated()[surface_a.get()] = std::move(decoration_a);
            }))
        .WillOnce(Invoke(
            [&]()
            {
                manager.decorated()[surface_b.get()] = std::move(decoration_b);
            }));
    manager.decorate(surface_a); // 1
}

TEST_F(DecorationBasicManager, does_not_destroy_decorations_while_locked)
{
    auto decoration_a = std::make_unique<MockDecoration>(this);
    auto decoration_b = std::make_unique<MockDecoration>(this);
    auto const surface_a = std::make_shared<mtd::StubSurface>();
    auto const surface_b = std::make_shared<mtd::StubSurface>();
    EXPECT_CALL(manager, decorate)
        .Times(2)
        .WillOnce(Invoke([&](auto const&){ return std::move(decoration_a); }))
        .WillOnce(Invoke([&](auto const&){ return std::move(decoration_b); }));
    EXPECT_CALL(*this, decoration_destroyed(decoration_a.get()))
        .Times(1)
        .WillOnce(Invoke([&](auto)
            {
                manager.undecorate(surface_b);
            }));
    EXPECT_CALL(*this, decoration_destroyed(decoration_b.get()))
        .Times(1);
    manager.decorate(surface_a);
    manager.decorate(surface_b);
    manager.undecorate(surface_a);
    Mock::VerifyAndClearExpectations(this);
}
