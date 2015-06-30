/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir/graphics/surfaceless_egl_context.h"
#include "mir/test/doubles/mock_egl.h"
#include <stdexcept>
#include <list>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mg = mir::graphics;
namespace mtd = mir::test::doubles;

class SurfacelessEGLContextTest  : public ::testing::Test
{
public:
    EGLDisplay const fake_display{reinterpret_cast<EGLDisplay>(0xfffaaafa)};
    EGLSurface const fake_surface{reinterpret_cast<EGLSurface>(0xdeadbeef)};
    EGLContext const fake_context{reinterpret_cast<EGLContext>(0xfaafbaaf)};

protected:
    virtual void SetUp()
    {
        using namespace testing;

        ON_CALL(mock_egl, eglCreatePbufferSurface(_,_,_))
            .WillByDefault(Return(fake_surface));
        ON_CALL(mock_egl, eglCreateContext(_,_,_,_))
            .WillByDefault(Return(fake_context));
        ON_CALL(mock_egl, eglGetCurrentContext())
             .WillByDefault(Return(EGL_NO_CONTEXT));
    }

    testing::NiceMock<mtd::MockEGL> mock_egl;
};

namespace
{

MATCHER(ConfigAttribContainsPBufferFlag, "")
{
    EGLint surface_type = 0;
    bool found_surface_type = false;
    std::list<std::string> pretty_surface;

    for (int i = 0; arg[i] != EGL_NONE; ++i)
    {
        if (arg[i] == EGL_SURFACE_TYPE)
        {
            surface_type = arg[i+1];
            found_surface_type = true;
        }
    }

    if (found_surface_type)
    {
        if (surface_type == EGL_DONT_CARE)
        {
            pretty_surface.push_back("EGL_DONT_CARE");
        }
        else
        {
            if (surface_type & EGL_MULTISAMPLE_RESOLVE_BOX_BIT)
            {
                pretty_surface.push_back("EGL_MULTISAMPLE_RESOLVE_BOX_BIT");
            }
            if (surface_type & EGL_PBUFFER_BIT)
            {
                pretty_surface.push_back("EGL_PBUFFER_BIT");
            }
            if (surface_type & EGL_PIXMAP_BIT)
            {
                pretty_surface.push_back("EGL_PIXMAP_BIT");
            }
            if (surface_type & EGL_SWAP_BEHAVIOR_PRESERVED_BIT)
            {
                pretty_surface.push_back("EGL_SWAP_BEHAVIOR_PRESERVED_BIT");
            }
            if (surface_type & EGL_VG_ALPHA_FORMAT_PRE_BIT)
            {
                pretty_surface.push_back("EGL_VG_ALPHA_FORMAT_PRE_BIT");
            }
            if (surface_type & EGL_VG_COLORSPACE_LINEAR_BIT)
            {
                pretty_surface.push_back("EGL_VG_COLORSPACE_LINEAR_BIT");
            }
            if (surface_type & EGL_WINDOW_BIT)
            {
                pretty_surface.push_back("EGL_WINDOW_BIT");
            }
        }
        std::string pretty_result = pretty_surface.back();
        pretty_surface.pop_back();
        for (auto& pretty : pretty_surface)
        {
            pretty_result += " | " + pretty;
        }
        *result_listener << "surface type is "<< pretty_result;
    }
    else
    {
        *result_listener << "no surface type set";
    }

    return found_surface_type &&
           (surface_type != EGL_DONT_CARE) &&
           (surface_type & EGL_PBUFFER_BIT);
}

}

TEST_F(SurfacelessEGLContextTest, UsesPBufferContainingAttribsListByDefault)
{
    using namespace testing;

    ON_CALL(mock_egl, eglQueryString(_,_))
        .WillByDefault(Return(""));

    EXPECT_CALL(mock_egl, eglChooseConfig(_, ConfigAttribContainsPBufferFlag(), _,_,_))
        .WillOnce(DoAll(SetArgPointee<4>(1), Return(EGL_TRUE)));

    mg::SurfacelessEGLContext ctx_noattrib(fake_display, EGL_NO_CONTEXT);
}

TEST_F(SurfacelessEGLContextTest, KeepsPBufferInAttribsList)
{
    using namespace testing;

    const EGLint attribs_with_pbuffer[] =
    {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT | EGL_WINDOW_BIT,
        EGL_NONE
    };

    ON_CALL(mock_egl, eglQueryString(_,_))
        .WillByDefault(Return(""));

    EXPECT_CALL(mock_egl, eglChooseConfig(_, ConfigAttribContainsPBufferFlag(), _,_,_))
        .WillOnce(DoAll(SetArgPointee<4>(1), Return(EGL_TRUE)));

    mg::SurfacelessEGLContext ctx_attribs_with_pbuffer(fake_display, attribs_with_pbuffer, EGL_NO_CONTEXT);
}

TEST_F(SurfacelessEGLContextTest, AddsPBufferToAttribList)
{
    using namespace testing;

    const EGLint attribs_without_surface_type[] =
    {
        EGL_ALPHA_SIZE, 8,
        EGL_CLIENT_APIS, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    ON_CALL(mock_egl, eglQueryString(_,_))
        .WillByDefault(Return(""));

    EXPECT_CALL(mock_egl, eglChooseConfig(_, ConfigAttribContainsPBufferFlag(), _,_,_))
        .WillOnce(DoAll(SetArgPointee<4>(1), Return(EGL_TRUE)));

    mg::SurfacelessEGLContext ctx_attribs_without_surface_type(fake_display, attribs_without_surface_type, EGL_NO_CONTEXT);
}

TEST_F(SurfacelessEGLContextTest, AddsPBufferWhenNonPBufferSurfaceTypeRequested)
{
    using namespace testing;

    const EGLint attribs_without_pbuffer[] =
    {
        EGL_SURFACE_TYPE, EGL_DONT_CARE,
        EGL_ALPHA_SIZE, 8,
        EGL_CLIENT_APIS, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    ON_CALL(mock_egl, eglQueryString(_,_))
        .WillByDefault(Return(""));

    EXPECT_CALL(mock_egl, eglChooseConfig(_, ConfigAttribContainsPBufferFlag(), _,_,_))
        .WillOnce(DoAll(SetArgPointee<4>(1), Return(EGL_TRUE)));

    mg::SurfacelessEGLContext ctx_attribs_without_pbuffer(fake_display, attribs_without_pbuffer, EGL_NO_CONTEXT);
}

TEST_F(SurfacelessEGLContextTest, DoesNotRequestPBufferWithNoAttrib)
{
    using namespace testing;

    ON_CALL(mock_egl, eglQueryString(_,_))
        .WillByDefault(Return("EGL_KHR_surfaceless_context"));

    EXPECT_CALL(mock_egl, eglChooseConfig(_, Not(ConfigAttribContainsPBufferFlag()), _,_,_))
        .WillOnce(DoAll(SetArgPointee<4>(1), Return(EGL_TRUE)));

    mg::SurfacelessEGLContext ctx_noattrib(fake_display, EGL_NO_CONTEXT);
}

TEST_F(SurfacelessEGLContextTest, DoesNotAddPBufferToAttribList)
{
    using namespace testing;

    const EGLint attribs_without_surface_type[] =
    {
        EGL_ALPHA_SIZE, 8,
        EGL_CLIENT_APIS, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    ON_CALL(mock_egl, eglQueryString(_,_))
        .WillByDefault(Return("EGL_KHR_surfaceless_context"));

    EXPECT_CALL(mock_egl, eglChooseConfig(_, Not(ConfigAttribContainsPBufferFlag()), _,_,_))
        .WillOnce(DoAll(SetArgPointee<4>(1), Return(EGL_TRUE)));

    mg::SurfacelessEGLContext ctx_attribs_without_surface_type(fake_display, attribs_without_surface_type, EGL_NO_CONTEXT);
}

TEST_F(SurfacelessEGLContextTest, DoesNotAddPBufferToSurfaceTypeRequest)
{
    using namespace testing;

    const EGLint attribs_with_surface_type[] =
    {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_ALPHA_SIZE, 8,
        EGL_CLIENT_APIS, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    ON_CALL(mock_egl, eglQueryString(_,_))
        .WillByDefault(Return("EGL_KHR_surfaceless_context"));

    EXPECT_CALL(mock_egl, eglChooseConfig(_, Not(ConfigAttribContainsPBufferFlag()), _,_,_))
        .WillOnce(DoAll(SetArgPointee<4>(1), Return(EGL_TRUE)));

    mg::SurfacelessEGLContext ctx_attribs_with_surface_type(fake_display, attribs_with_surface_type, EGL_NO_CONTEXT);
}
