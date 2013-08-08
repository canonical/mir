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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/shell/mediating_display_changer.h"
#include "mir/shell/session.h"
#include "mir/compositor/compositor.h"

#include "mir_test_doubles/mock_display_changer.h"
#include "mir_test_doubles/mock_session.h"
#include "mir_test_doubles/stub_session.h"
#include "mir_test_doubles/mock_display.h"
#include "mir_test_doubles/null_display_configuration.h"
#include "mir_test/fake_shared.h"
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mf = mir::frontend;
namespace msh = mir::shell;

namespace
{
struct MockCompositor : public mc::Compositor
{
    MOCK_METHOD0(start, void());
    MOCK_METHOD0(stop, void());
};

}

struct MediatingDisplayChangerTest : public ::testing::Test
{
    void SetUp()
    {
        using namespace testing;
        ON_CALL(mock_display, configuration())
            .WillByDefault(Return(mt::fake_shared(default_config)));
    }
    mtd::MockDisplay mock_display;
    MockCompositor mock_compositor;
    
    mtd::NullDisplayConfiguration default_config;
};

TEST_F(MediatingDisplayChangerTest, display_info)
{
    using namespace testing;
    mtd::NullDisplayConfiguration conf;
    EXPECT_CALL(mock_display, configuration())
        .Times(1)
        .WillOnce(Return(mt::fake_shared(conf)));

    msh::MediatingDisplayChanger changer(mt::fake_shared(mock_display),
                                         mt::fake_shared(mock_compositor));
    auto returned_conf = changer.active_configuration();
    EXPECT_EQ(&conf, returned_conf.get());
}

namespace
{
struct StubShellSession : public msh::Session
{
    mf::SurfaceId create_surface(msh::SurfaceCreationParameters const&)
    {
        return mf::SurfaceId{0};
    }
    void destroy_surface(mf::SurfaceId)
    {
    }
    std::shared_ptr<mf::Surface> get_surface(mf::SurfaceId) const
    {
        return std::shared_ptr<mf::Surface>();
    }

    std::string name() const
    {
        return std::string();
    }
    void force_requests_to_complete()
    {
    }
    void hide()
    {
    }
    void show()
    {
    }
    int configure_surface(mf::SurfaceId, MirSurfaceAttrib, int)
    {
        return 0;
    }
    void take_snapshot(msh::SnapshotCallback const&)
    {
    }
    std::shared_ptr<msh::Surface> default_surface() const
    {
        return std::shared_ptr<msh::Surface>();
    }

};
}

namespace
{
MATCHER_P(ConfigPtrsMatch, config_ptr, "")
{
    return (&arg == config_ptr);
}

}
TEST_F(MediatingDisplayChangerTest, display_configure)
{
    using namespace testing;

    auto session1 = std::make_shared<StubShellSession>();
    auto session2 = std::make_shared<StubShellSession>();
    auto base_config = std::make_shared<mtd::NullDisplayConfiguration>();
    auto applied_config1 = std::make_shared<mtd::NullDisplayConfiguration>();
    auto applied_config2 = std::make_shared<mtd::NullDisplayConfiguration>();

    //get base config
    EXPECT_CALL(mock_display, configuration())
        .Times(1)
        .WillOnce(Return(base_config));

    //apply session config
    Sequence seq;
    EXPECT_CALL(mock_compositor, stop())
        .InSequence(seq);
    EXPECT_CALL(mock_display, configure(ConfigPtrsMatch(applied_config1.get())))
        .InSequence(seq);
    EXPECT_CALL(mock_compositor, start())
        .InSequence(seq);

    EXPECT_CALL(mock_compositor, stop())
        .InSequence(seq);
    EXPECT_CALL(mock_display, configure(ConfigPtrsMatch(applied_config2.get())))
        .InSequence(seq);
    EXPECT_CALL(mock_compositor, start())
        .InSequence(seq);

    //apply base config
    EXPECT_CALL(mock_compositor, stop())
        .InSequence(seq);
    EXPECT_CALL(mock_display, configure(ConfigPtrsMatch(base_config.get())))
        .InSequence(seq);
    EXPECT_CALL(mock_compositor, start())
        .InSequence(seq);

    msh::MediatingDisplayChanger changer(mt::fake_shared(mock_display),
                                         mt::fake_shared(mock_compositor));

    changer.store_configuration_for(session1, applied_config1);
    changer.store_configuration_for(session2, applied_config2);
    changer.apply_configuration_of(session1);
    changer.apply_configuration_of(session2);
    changer.remove_configuration_for(session2);
    changer.remove_configuration_for(session1);
}
