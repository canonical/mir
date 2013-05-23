/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/shell/surface.h"
#include "mir/surfaces/surface.h"
#include "mir/shell/surface_creation_parameters.h"
#include "mir/surfaces/surface_stack_model.h"
#include "mir/input/input_channel.h"
#include "mir/shell/surface_builder.h"

#include "mir_test_doubles/null_buffer_bundle.h"
#include "mir_test_doubles/mock_buffer_bundle.h"
#include "mir_test_doubles/mock_buffer.h"
#include "mir_test_doubles/stub_buffer.h"
#include "mir_test/fake_shared.h"

#include <stdexcept>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace me = mir::events;
namespace ms = mir::surfaces;
namespace msh = mir::shell;
namespace mf = mir::frontend;
namespace mc = mir::compositor;
namespace mi = mir::input;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mt::doubles;

namespace
{
class StubSurfaceBuilder : public msh::SurfaceBuilder
{
public:
    StubSurfaceBuilder() :
        buffer_bundle(new mtd::NullBufferBundle()),
        dummy_surface()
    {
    }

    std::weak_ptr<ms::Surface> create_surface(msh::SurfaceCreationParameters const& )
    {
        dummy_surface = std::make_shared<ms::Surface>(msh::a_surface().name, msh::a_surface().top_left, buffer_bundle, []{});
        return dummy_surface;
    }

    void destroy_surface(std::weak_ptr<ms::Surface> const& )
    {
        reset_surface();
    }

    void reset_surface()
    {
        dummy_surface.reset();
    }

private:
    std::shared_ptr<ms::BufferBundle> const buffer_bundle;
    std::shared_ptr<ms::Surface> dummy_surface;
};

class MockSurfaceBuilder : public msh::SurfaceBuilder
{
public:
    MockSurfaceBuilder()
    {
        using namespace testing;
        ON_CALL(*this, create_surface(_)).
            WillByDefault(Invoke(&self, &StubSurfaceBuilder::create_surface));

        ON_CALL(*this, destroy_surface(_)).
            WillByDefault(Invoke(&self, &StubSurfaceBuilder::destroy_surface));
    }

    MOCK_METHOD1(create_surface, std::weak_ptr<ms::Surface> (const msh::SurfaceCreationParameters&));

    MOCK_METHOD1(destroy_surface, void (std::weak_ptr<ms::Surface> const&));

private:
    StubSurfaceBuilder self;
};

typedef testing::NiceMock<mtd::MockBufferBundle> StubBufferBundle;


struct MockInputChannel : public mi::InputChannel
{
    MOCK_CONST_METHOD0(client_fd, int());
    MOCK_CONST_METHOD0(server_fd, int());
};

struct ShellSurface : testing::Test
{
    std::shared_ptr<StubBufferBundle> const buffer_bundle;
    std::shared_ptr<mi::InputChannel> const null_input_channel;
    StubSurfaceBuilder surface_builder;

    ShellSurface() :
        buffer_bundle(std::make_shared<StubBufferBundle>()),
        null_input_channel(0)
    {
        using namespace testing;

        ON_CALL(*buffer_bundle, bundle_size()).WillByDefault(Return(geom::Size()));
        ON_CALL(*buffer_bundle, get_bundle_pixel_format()).WillByDefault(Return(geom::PixelFormat::abgr_8888));
        ON_CALL(*buffer_bundle, secure_client_buffer()).WillByDefault(Return(std::shared_ptr<mtd::StubBuffer>()));
    }
};
}

TEST_F(ShellSurface, creation_and_destruction)
{
    using namespace testing;

    msh::SurfaceCreationParameters const params;
    MockSurfaceBuilder surface_builder;

    InSequence sequence;
    EXPECT_CALL(surface_builder, create_surface(params)).Times(1);
    EXPECT_CALL(surface_builder, destroy_surface(_)).Times(1);

    msh::Surface test(
        mt::fake_shared(surface_builder),
        params,
        null_input_channel);
}

TEST_F(ShellSurface, creation_throws_means_no_destroy)
{
    using namespace testing;

    msh::SurfaceCreationParameters const params;
    MockSurfaceBuilder surface_builder;

    InSequence sequence;
    EXPECT_CALL(surface_builder, create_surface(params)).Times(1)
        .WillOnce(Throw(std::runtime_error(__PRETTY_FUNCTION__)));
    EXPECT_CALL(surface_builder, destroy_surface(_)).Times(Exactly(0));

    EXPECT_THROW({
        msh::Surface test(
            mt::fake_shared(surface_builder),
            params,
            null_input_channel);
    }, std::runtime_error);
}

TEST_F(ShellSurface, destroy)
{
    using namespace testing;
    MockSurfaceBuilder surface_builder;

    InSequence sequence;
    EXPECT_CALL(surface_builder, create_surface(_)).Times(AnyNumber());
    EXPECT_CALL(surface_builder, destroy_surface(_)).Times(0);

    msh::Surface test(
            mt::fake_shared(surface_builder),
            msh::a_surface(),
            null_input_channel);

    Mock::VerifyAndClearExpectations(&test);
    EXPECT_CALL(surface_builder, destroy_surface(_)).Times(1);

    // Check destroy_surface is called immediately
    test.destroy();

    Mock::VerifyAndClearExpectations(&test);
    EXPECT_CALL(surface_builder, destroy_surface(_)).Times(0);
}

TEST_F(ShellSurface, client_buffer_throw_behavior)
{
    msh::Surface test(
            mt::fake_shared(surface_builder),
            msh::a_surface(),
            null_input_channel);

    EXPECT_NO_THROW({
        test.client_buffer();
    });

    surface_builder.reset_surface();

    EXPECT_THROW({
        test.client_buffer();
    }, std::runtime_error);
}

TEST_F(ShellSurface, size_throw_behavior)
{
    msh::Surface test(
            mt::fake_shared(surface_builder),
            msh::a_surface(),
            null_input_channel);

    EXPECT_NO_THROW({
        test.size();
    });

    surface_builder.reset_surface();

    EXPECT_THROW({
        test.size();
    }, std::runtime_error);
}

TEST_F(ShellSurface, top_left_throw_behavior)
{
    msh::Surface test(
            mt::fake_shared(surface_builder),
            msh::a_surface(),
            null_input_channel);

    EXPECT_NO_THROW({
        test.top_left();
    });

    surface_builder.reset_surface();

    EXPECT_THROW({
        test.top_left();
    }, std::runtime_error);
}

TEST_F(ShellSurface, name_throw_behavior)
{
    msh::Surface test(
            mt::fake_shared(surface_builder),
            msh::a_surface(),
            null_input_channel);

    EXPECT_NO_THROW({
        test.name();
    });

    surface_builder.reset_surface();

    EXPECT_THROW({
        test.name();
    }, std::runtime_error);
}

TEST_F(ShellSurface, pixel_format_throw_behavior)
{
    msh::Surface test(
            mt::fake_shared(surface_builder),
            msh::a_surface(),
            null_input_channel);

    EXPECT_NO_THROW({
        test.pixel_format();
    });

    surface_builder.reset_surface();

    EXPECT_THROW({
        test.pixel_format();
    }, std::runtime_error);
}

TEST_F(ShellSurface, hide_throw_behavior)
{
    msh::Surface test(
            mt::fake_shared(surface_builder),
            msh::a_surface(),
            null_input_channel);

    EXPECT_NO_THROW({
        test.hide();
    });

    surface_builder.reset_surface();

    EXPECT_NO_THROW({
        test.hide();
    });
}

TEST_F(ShellSurface, show_throw_behavior)
{
    msh::Surface test(
            mt::fake_shared(surface_builder),
            msh::a_surface(),
            null_input_channel);

    EXPECT_NO_THROW({
        test.show();
    });

    surface_builder.reset_surface();

    EXPECT_NO_THROW({
        test.show();
    });
}

TEST_F(ShellSurface, destroy_throw_behavior)
{
    msh::Surface test(
            mt::fake_shared(surface_builder),
            msh::a_surface(),
            null_input_channel);

    EXPECT_NO_THROW({
        test.destroy();
    });

    surface_builder.reset_surface();

    EXPECT_NO_THROW({
        test.destroy();
    });
}

TEST_F(ShellSurface, force_request_to_complete_throw_behavior)
{
    msh::Surface test(
            mt::fake_shared(surface_builder),
            msh::a_surface(),
            null_input_channel);

    EXPECT_NO_THROW({
        test.force_requests_to_complete();
    });

    surface_builder.reset_surface();

    EXPECT_NO_THROW({
        test.force_requests_to_complete();
    });
}

TEST_F(ShellSurface, advance_client_buffer_throw_behavior)
{
    msh::Surface test(
            mt::fake_shared(surface_builder),
            msh::a_surface(),
            null_input_channel);

    EXPECT_NO_THROW({
        test.advance_client_buffer();
    });

    surface_builder.reset_surface();

    EXPECT_NO_THROW({
        test.advance_client_buffer();
    });
}

TEST_F(ShellSurface, surfaces_with_input_channel_supports_input)
{
    using namespace testing;
    const int testing_client_fd = 17;

    MockInputChannel mock_package;
    msh::Surface proxy_surface(mt::fake_shared(surface_builder), msh::a_surface(), null_input_channel);
    msh::Surface input_proxy_surface(mt::fake_shared(surface_builder), msh::a_surface(), mt::fake_shared(mock_package));

    EXPECT_CALL(mock_package, client_fd()).Times(1).WillOnce(Return(testing_client_fd));

    EXPECT_TRUE(input_proxy_surface.supports_input());
    EXPECT_FALSE(proxy_surface.supports_input());

    EXPECT_EQ(testing_client_fd, input_proxy_surface.client_input_fd());

    EXPECT_THROW({
            proxy_surface.client_input_fd();
    }, std::logic_error);
}

TEST_F(ShellSurface, attributes)
{
    using namespace testing;

    msh::Surface surf(
            mt::fake_shared(surface_builder),
            msh::a_surface(),
            null_input_channel);

    EXPECT_THROW({
        surf.configure(static_cast<MirSurfaceAttrib>(111), 222);
    }, std::logic_error);
}

TEST_F(ShellSurface, types)
{
    using namespace testing;

    msh::Surface surf(
            mt::fake_shared(surface_builder),
            msh::a_surface(),
            null_input_channel);

    EXPECT_EQ(mir_surface_type_normal, surf.type());

    EXPECT_EQ(mir_surface_type_utility,
              surf.configure(mir_surface_attrib_type,
                             mir_surface_type_utility));
    EXPECT_EQ(mir_surface_type_utility, surf.type());

    EXPECT_THROW({
        surf.configure(mir_surface_attrib_type, 999);
    }, std::logic_error);
    EXPECT_THROW({
        surf.configure(mir_surface_attrib_type, -1);
    }, std::logic_error);
    EXPECT_EQ(mir_surface_type_utility, surf.type());

    EXPECT_EQ(mir_surface_type_dialog,
              surf.configure(mir_surface_attrib_type,
                             mir_surface_type_dialog));
    EXPECT_EQ(mir_surface_type_dialog, surf.type());

    EXPECT_EQ(mir_surface_type_freestyle,
              surf.configure(mir_surface_attrib_type,
                             mir_surface_type_freestyle));
    EXPECT_EQ(mir_surface_type_freestyle, surf.type());
}

TEST_F(ShellSurface, states)
{
    using namespace testing;

    msh::Surface surf(
            mt::fake_shared(surface_builder),
            msh::a_surface(),
            null_input_channel);

    EXPECT_EQ(mir_surface_state_restored, surf.state());

    EXPECT_EQ(mir_surface_state_vertmaximized,
              surf.configure(mir_surface_attrib_state,
                             mir_surface_state_vertmaximized));
    EXPECT_EQ(mir_surface_state_vertmaximized, surf.state());

    EXPECT_THROW({
        surf.configure(mir_surface_attrib_state, 999);
    }, std::logic_error);
    EXPECT_THROW({
        surf.configure(mir_surface_attrib_state, -1);
    }, std::logic_error);
    EXPECT_EQ(mir_surface_state_vertmaximized, surf.state());

    EXPECT_EQ(mir_surface_state_minimized,
              surf.configure(mir_surface_attrib_state,
                             mir_surface_state_minimized));
    EXPECT_EQ(mir_surface_state_minimized, surf.state());

    EXPECT_EQ(mir_surface_state_fullscreen,
              surf.configure(mir_surface_attrib_state,
                             mir_surface_state_fullscreen));
    EXPECT_EQ(mir_surface_state_fullscreen, surf.state());
}

