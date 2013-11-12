#include "mir/graphics/offscreen_platform.h"
#include "src/server/graphics/offscreen/display.h"
#include "mir/graphics/null_display_report.h"
#include "src/server/graphics/default_display_configuration_policy.h"
#include "mir/graphics/display_buffer.h"

#include "mir_test_doubles/mock_egl.h"
#include "mir_test_doubles/mock_gl.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mg=mir::graphics;
namespace mgo=mir::graphics::offscreen;
namespace mtd=mir::test::doubles;

namespace
{

class StubOffscreenPlatform : public mg::OffscreenPlatform
{
public:
    StubOffscreenPlatform(EGLNativeDisplayType native_display)
        : native_display{native_display}
    {
    }

    EGLNativeDisplayType egl_native_display() const
    {
        return native_display;
    }

private:
    EGLNativeDisplayType const native_display;
};

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

        const char* const egl_exts = "EGL_KHR_image EGL_KHR_image_base";
        const char* const gl_exts = "GL_OES_EGL_image";

        ON_CALL(mock_egl, eglQueryString(_,EGL_EXTENSIONS))
            .WillByDefault(Return(egl_exts));
        ON_CALL(mock_gl, glGetString(GL_EXTENSIONS))
            .WillByDefault(Return(reinterpret_cast<const GLubyte*>(gl_exts)));
    }

    ::testing::NiceMock<mtd::MockEGL> mock_egl;
    ::testing::NiceMock<mtd::MockGL> mock_gl;
};

}

TEST_F(OffscreenDisplayTest, uses_offscreen_platform_egl_native_display)
{
    using namespace ::testing;

    EGLNativeDisplayType const native_display{
        reinterpret_cast<EGLNativeDisplayType>(0x12345)};

    InSequence s;
    EXPECT_CALL(mock_egl, eglGetDisplay(native_display));
    EXPECT_CALL(mock_egl, eglInitialize(mock_egl.fake_egl_display, _, _));
    EXPECT_CALL(mock_egl, eglTerminate(mock_egl.fake_egl_display));

    mgo::Display display{
        std::make_shared<StubOffscreenPlatform>(native_display),
        std::make_shared<mg::DefaultDisplayConfigurationPolicy>(),
        std::make_shared<mg::NullDisplayReport>()};
}

TEST_F(OffscreenDisplayTest, makes_fbo_current_rendering_target)
{
    using namespace ::testing;

    GLuint const fbo{66};
    EGLNativeDisplayType const native_display{
        reinterpret_cast<EGLNativeDisplayType>(0x12345)};

    /* Creates GL framebuffer objects */
    EXPECT_CALL(mock_gl, glGenFramebuffers(1,_))
        .Times(AtLeast(1))
        .WillRepeatedly(SetArgPointee<1>(fbo));

    mgo::Display display{
        std::make_shared<StubOffscreenPlatform>(native_display),
        std::make_shared<mg::DefaultDisplayConfigurationPolicy>(),
        std::make_shared<mg::NullDisplayReport>()};

    Mock::VerifyAndClearExpectations(&mock_gl);

    /* Binds the GL framebuffer objects */
    display.for_each_display_buffer(
        [this](mg::DisplayBuffer& db)
        {
            EXPECT_CALL(mock_egl, eglMakeCurrent(_,_,_,Ne(EGL_NO_CONTEXT)));
            EXPECT_CALL(mock_gl, glBindFramebuffer(_,Ne(0)));

            db.make_current();

            Mock::VerifyAndClearExpectations(&mock_egl);
            Mock::VerifyAndClearExpectations(&mock_gl);
        });
}
