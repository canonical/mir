#include "mir/graphics/display_buffer.h"

#include "src/server/graphics/offscreen/display.h"
#include "mir/graphics/default_display_configuration_policy.h"
#include "mir/renderer/gl/render_target.h"
#include "src/server/report/null_report_factory.h"

#include "mir/test/doubles/mock_egl.h"
#include "mir/test/doubles/mock_gl.h"
#include "mir/test/as_render_target.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <stdexcept>

namespace mg=mir::graphics;
namespace mgo=mir::graphics::offscreen;
namespace mtd=mir::test::doubles;
namespace mr = mir::report;
namespace mt = mir::test;

namespace
{

class OffscreenDisplayTest : public ::testing::Test
{
public:
    OffscreenDisplayTest()
    {
        using namespace ::testing;

        ON_CALL(mock_egl, eglChooseConfig(_,_,_,1,_))
            .WillByDefault(DoAll(SetArgPointee<2>(mock_egl.fake_configs[0]),
                                 SetArgPointee<4>(1),
                                 Return(EGL_TRUE)));
        mock_egl.provide_egl_extensions();
        mock_gl.provide_gles_extensions();

        ON_CALL(mock_gl, glCheckFramebufferStatus(_))
            .WillByDefault(Return(GL_FRAMEBUFFER_COMPLETE));
    }

    ::testing::NiceMock<mtd::MockEGL> mock_egl;
    ::testing::NiceMock<mtd::MockGL> mock_gl;
    EGLNativeDisplayType const native_display{reinterpret_cast<EGLNativeDisplayType>(0x12345)};
};

}

TEST_F(OffscreenDisplayTest, uses_basic_platform_egl_native_display)
{
    using namespace ::testing;
    InSequence s;
    EXPECT_CALL(mock_egl, eglGetDisplay(native_display));
    EXPECT_CALL(mock_egl, eglInitialize(mock_egl.fake_egl_display, _, _));
    EXPECT_CALL(mock_egl, eglTerminate(mock_egl.fake_egl_display));

    mgo::Display display{
        native_display,
        std::make_shared<mg::CloneDisplayConfigurationPolicy>(),
        mr::null_display_report()};
}

TEST_F(OffscreenDisplayTest, orientation_normal)
{
    using namespace ::testing;
    mgo::Display display{
        native_display,
        std::make_shared<mg::CloneDisplayConfigurationPolicy>(),
        mr::null_display_report()};

    int count = 0;
    display.for_each_display_sync_group([&](mg::DisplaySyncGroup& group) {
        group.for_each_display_buffer([&](mg::DisplayBuffer& db) {
            ++count;
            EXPECT_EQ(glm::mat2(1), db.transformation());
        });
    });

    EXPECT_TRUE(count);
}

TEST_F(OffscreenDisplayTest, never_enables_predictive_bypass)
{
    mgo::Display display{
        native_display,
        std::make_shared<mg::CloneDisplayConfigurationPolicy>(),
        mr::null_display_report()};

    int groups = 0;
    display.for_each_display_sync_group([&](mg::DisplaySyncGroup& group){
        ++groups;
        EXPECT_EQ(0, group.recommended_sleep().count());
    });

    EXPECT_TRUE(groups);
}

TEST_F(OffscreenDisplayTest, makes_fbo_current_rendering_target)
{
    using namespace ::testing;

    GLuint const fbo{66};
    /* Creates GL framebuffer objects */
    EXPECT_CALL(mock_gl, glGenFramebuffers(1,_))
        .Times(AtLeast(1))
        .WillRepeatedly(SetArgPointee<1>(fbo));

    /* Provide unique EGL contexts */
    std::vector<int> contexts;
    EXPECT_CALL(mock_egl, eglCreateContext(_,_,_,_))
        .Times(AtLeast(1))
        .WillRepeatedly(WithoutArgs(Invoke(
            [&] ()
            {
                contexts.push_back(0);
                return reinterpret_cast<EGLContext>(&contexts.back());
            })));

    mgo::Display display{
        native_display,
        std::make_shared<mg::CloneDisplayConfigurationPolicy>(),
        mr::null_display_report()};

    Mock::VerifyAndClearExpectations(&mock_gl);

    /* Binds the GL framebuffer objects */
    display.for_each_display_sync_group([&](mg::DisplaySyncGroup& group) {
        group.for_each_display_buffer([&](mg::DisplayBuffer& db) {
            EXPECT_CALL(mock_egl, eglMakeCurrent(_,_,_,Ne(EGL_NO_CONTEXT)));
            EXPECT_CALL(mock_gl, glBindFramebuffer(_,Ne(0)));

            mt::as_render_target(db)->make_current();
            mt::as_render_target(db)->bind();

            Mock::VerifyAndClearExpectations(&mock_egl);
            Mock::VerifyAndClearExpectations(&mock_gl);
        });
    });

    /* Contexts are released at teardown */
    display.for_each_display_sync_group([&](mg::DisplaySyncGroup& group) {
        group.for_each_display_buffer([&](mg::DisplayBuffer&) {
            EXPECT_CALL(mock_egl, eglMakeCurrent(_,_,_,EGL_NO_CONTEXT));
        });
    });
}

TEST_F(OffscreenDisplayTest, restores_previous_state_on_fbo_setup_failure)
{
    using namespace ::testing;

    GLuint const old_fbo{66};
    GLuint const new_fbo{67};

    EXPECT_CALL(mock_gl, glGetIntegerv(GL_FRAMEBUFFER_BINDING, _))
        .WillOnce(SetArgPointee<1>(old_fbo));
    EXPECT_CALL(mock_gl, glGetIntegerv(GL_VIEWPORT, _));

    InSequence s;
    EXPECT_CALL(mock_gl, glGenFramebuffers(1,_))
        .WillOnce(SetArgPointee<1>(new_fbo));
    EXPECT_CALL(mock_gl, glBindFramebuffer(_,new_fbo));
    EXPECT_CALL(mock_gl, glCheckFramebufferStatus(_))
        .WillOnce(Return(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT));
    EXPECT_CALL(mock_gl, glBindFramebuffer(_,old_fbo));

    EXPECT_THROW({
        mgo::Display display(
            native_display,
            std::make_shared<mg::CloneDisplayConfigurationPolicy>(),
            mr::null_display_report());
    }, std::runtime_error);
}
