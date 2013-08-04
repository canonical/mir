/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/frontend/session_mediator_report.h"
#include "mir/frontend/session_mediator.h"
#include "mir/frontend/resource_cache.h"
#include "mir/shell/application_session.h"
#include "mir/frontend/shell.h"
#include "mir/graphics/display.h"
#include "mir/graphics/drm_authenticator.h"
#include "mir/frontend/event_sink.h"

#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>

#include "mir_test_doubles/null_display_changer.h"
#include "mir_test_doubles/null_platform.h"
#include "mir_test_doubles/mock_session.h"
#include "mir_test_doubles/stub_shell.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace mp = mir::protobuf;
namespace msh = mir::shell;
namespace mtd = mir::test::doubles;

namespace
{

class StubGraphicBufferAllocator : public mg::GraphicBufferAllocator
{
public:
    std::shared_ptr<mg::Buffer> alloc_buffer(mg::BufferProperties const&)
    {
        return std::shared_ptr<mg::Buffer>();
    }

    virtual std::vector<geom::PixelFormat> supported_pixel_formats()
    {
        return std::vector<geom::PixelFormat>();
    }
};

class MockAuthenticatingPlatform : public mtd::NullPlatform, public mg::DRMAuthenticator
{
 public:
    std::shared_ptr<mg::GraphicBufferAllocator> create_buffer_allocator(
        const std::shared_ptr<mg::BufferInitializer>& /*buffer_initializer*/) override
    {
        return std::shared_ptr<StubGraphicBufferAllocator>();
    }

    MOCK_METHOD1(drm_auth_magic, void(drm_magic_t));
};

class NullEventSink : public mir::frontend::EventSink
{
public:
    void handle_event(MirEvent const& ) override {}
};

struct SessionMediatorGBMTest : public ::testing::Test
{
    SessionMediatorGBMTest()
        : shell{std::make_shared<mtd::StubShell>()},
          mock_platform{std::make_shared<MockAuthenticatingPlatform>()},
          display_changer{std::make_shared<mtd::NullDisplayChanger>()},
          buffer_allocator{std::make_shared<StubGraphicBufferAllocator>()},
          report{std::make_shared<mf::NullSessionMediatorReport>()},
          resource_cache{std::make_shared<mf::ResourceCache>()},
          mediator{shell, mock_platform, display_changer,
                   buffer_allocator, report,
                   std::make_shared<NullEventSink>(),
                   resource_cache},
          null_callback{google::protobuf::NewPermanentCallback(google::protobuf::DoNothing)}
    {
    }

    std::shared_ptr<mtd::StubShell> const shell;
    std::shared_ptr<MockAuthenticatingPlatform> const mock_platform;
    std::shared_ptr<msh::DisplayChanger> const display_changer;
    std::shared_ptr<mg::GraphicBufferAllocator> const buffer_allocator;
    std::shared_ptr<mf::SessionMediatorReport> const report;
    std::shared_ptr<mf::ResourceCache> const resource_cache;
    mf::SessionMediator mediator;

    std::unique_ptr<google::protobuf::Closure> null_callback;
};

}

TEST_F(SessionMediatorGBMTest, drm_auth_magic_uses_drm_authenticator)
{
    mp::ConnectParameters connect_parameters;
    mp::Connection connection;

    drm_magic_t const drm_magic{0x10111213};
    int const no_error{0};

    EXPECT_CALL(*mock_platform, drm_auth_magic(drm_magic))
        .Times(1);

    mediator.connect(nullptr, &connect_parameters, &connection, null_callback.get());

    mp::DRMMagic magic;
    mp::DRMAuthMagicStatus status;
    magic.set_magic(drm_magic);

    mediator.drm_auth_magic(nullptr, &magic, &status, null_callback.get());

    EXPECT_EQ(no_error, status.status_code());
}

TEST_F(SessionMediatorGBMTest, drm_auth_magic_sets_status_code_on_error)
{
    using namespace testing;

    mp::ConnectParameters connect_parameters;
    mp::Connection connection;

    drm_magic_t const drm_magic{0x10111213};
    int const error_number{667};

    EXPECT_CALL(*mock_platform, drm_auth_magic(drm_magic))
        .WillOnce(Throw(::boost::enable_error_info(std::exception())
            << boost::errinfo_errno(error_number)));

    mediator.connect(nullptr, &connect_parameters, &connection, null_callback.get());

    mp::DRMMagic magic;
    mp::DRMAuthMagicStatus status;
    magic.set_magic(drm_magic);

    mediator.drm_auth_magic(nullptr, &magic, &status, null_callback.get());

    EXPECT_EQ(error_number, status.status_code());
}
