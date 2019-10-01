/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include <gtest/gtest.h>
#include <fcntl.h>
#include <boost/throw_exception.hpp>

#include "mir/graphics/platform.h"
#include "mir/graphics/platform_probe.h"
#include "mir/options/program_option.h"

#include "mir/raii.h"

#include "mir/test/doubles/mock_egl.h"
#if defined(MIR_BUILD_PLATFORM_MESA_KMS) || defined(MIR_BUILD_PLATFORM_MESA_X11)
#include "mir/test/doubles/mock_drm.h"
#endif
#ifdef MIR_BUILD_PLATFORM_MESA_KMS
#include "mir/test/doubles/mock_gbm.h"
#include "mir/test/doubles/mock_gl.h"
#endif
#include "mir/test/doubles/null_console_services.h"

#include "mir_test_framework/udev_environment.h"
#include "mir_test_framework/executable_path.h"

namespace mtd = mir::test::doubles;
namespace mtf = mir_test_framework;

namespace
{
const char describe_module[] = "describe_graphics_module";

std::vector<std::shared_ptr<mir::SharedLibrary>> available_platforms()
{
    std::vector<std::shared_ptr<mir::SharedLibrary>> modules;

#ifdef MIR_BUILD_PLATFORM_MESA_KMS
    modules.push_back(std::make_shared<mir::SharedLibrary>(mtf::server_platform("graphics-mesa-kms")));
#endif
    return modules;
}

void add_dummy_platform(std::vector<std::shared_ptr<mir::SharedLibrary>>& modules)
{
    modules.insert(modules.begin(), std::make_shared<mir::SharedLibrary>(mtf::server_platform("graphics-dummy.so")));
}

std::shared_ptr<void> ensure_mesa_probing_fails()
{
    return std::make_shared<mtf::UdevEnvironment>();
}

std::shared_ptr<void> ensure_mesa_probing_succeeds()
{
    using namespace testing;
    struct MockEnvironment {
        mtf::UdevEnvironment udev;
        testing::NiceMock<mtd::MockEGL> egl;
#ifdef MIR_BUILD_PLATFORM_MESA_KMS
        testing::NiceMock<mtd::MockGBM> gbm;
        testing::NiceMock<mtd::MockGL> gl;
#endif
    };
#ifdef MIR_BUILD_PLATFORM_MESA_KMS
    static auto const fake_gbm_device = reinterpret_cast<gbm_device*>(0xa1b2c3d4);
#endif
    static auto const fake_egl_display = reinterpret_cast<EGLDisplay>(0xeda);
    auto env = std::make_shared<MockEnvironment>();

    env->udev.add_standard_device("standard-drm-devices");
    ON_CALL(env->egl, eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS))
        .WillByDefault(Return("EGL_MESA_platform_gbm EGL_EXT_platform_base"));
#ifdef MIR_BUILD_PLATFORM_MESA_KMS
    ON_CALL(env->gbm, gbm_create_device(_))
        .WillByDefault(Return(fake_gbm_device));
    ON_CALL(env->egl, eglGetDisplay(fake_gbm_device))
        .WillByDefault(Return(fake_egl_display));
#endif
    ON_CALL(env->egl, eglInitialize(fake_egl_display, _, _))
        .WillByDefault(
            DoAll(
                SetArgPointee<1>(1),
                SetArgPointee<2>(4),
                Return(EGL_TRUE)));
#ifdef MIR_BUILD_PLATFORM_MESA_KMS
    ON_CALL(env->egl, eglGetConfigAttrib(_, env->egl.fake_configs[0], EGL_NATIVE_VISUAL_ID, _))
            .WillByDefault(
                DoAll(
                    SetArgPointee<3>(GBM_FORMAT_XRGB8888),
                    Return(EGL_TRUE)));
    ON_CALL(env->gl, glGetString(GL_RENDERER))
        .WillByDefault(Return(reinterpret_cast<GLubyte const*>("Not A Software Renderer, Honest!")));
#endif

    return env;
}

class StubConsoleServices : public mir::ConsoleServices
{
public:
    void
    register_switch_handlers(
        mir::graphics::EventHandlerRegister&,
        std::function<bool()> const&,
        std::function<bool()> const&) override
    {
    }

    void restore() override
    {
    }

    std::unique_ptr<mir::VTSwitcher> create_vt_switcher() override
    {
        BOOST_THROW_EXCEPTION((
            std::runtime_error{"StubConsoleServices does not support VT switching"}));
    }

    std::future<std::unique_ptr<mir::Device>> acquire_device(
        int major, int minor,
        std::unique_ptr<mir::Device::Observer> observer) override
    {
        /* NOTE: This uses the behaviour that MockDRM will intercept any open() call
         * under /dev/dri/
         */
        std::stringstream filename;
        filename << "/dev/dri/" << major << ":" << minor;
        observer->activated(mir::Fd{::open(filename.str().c_str(), O_RDWR | O_CLOEXEC)});
        std::promise<std::unique_ptr<mir::Device>> promise;
        // The Device is *just* a handle; there's no reason for anything to dereference it
        promise.set_value(nullptr);
        return promise.get_future();
    }
};

class ServerPlatformProbeMockDRM : public ::testing::Test
{
#if defined(MIR_BUILD_PLATFORM_MESA_KMS) || defined(MIR_BUILD_PLATFORM_MESA_X11)
public:
    ::testing::NiceMock<mtd::MockDRM> mock_drm;
#endif
};

}

TEST(ServerPlatformProbe, ConstructingWithNoModulesIsAnError)
{
    std::vector<std::shared_ptr<mir::SharedLibrary>> empty_modules;
    mir::options::ProgramOption options;

    EXPECT_THROW(mir::graphics::module_for_device(empty_modules, options, nullptr),
                 std::runtime_error);
}

#ifdef MIR_BUILD_PLATFORM_MESA_KMS
TEST_F(ServerPlatformProbeMockDRM, LoadsMesaPlatformWhenDrmMasterCanBeAcquired)
{
    using namespace testing;
    mir::options::ProgramOption options;
    auto fake_mesa = ensure_mesa_probing_succeeds();

    auto modules = available_platforms();

    auto module = mir::graphics::module_for_device(
        modules,
        options,
        std::make_shared<StubConsoleServices>());
    ASSERT_NE(nullptr, module);

    auto descriptor = module->load_function<mir::graphics::DescribeModule>(describe_module);
    auto description = descriptor();

    EXPECT_THAT(description->name, HasSubstr("mesa-kms"));
}

//LP: #1526225, LP: #1526505, LP: #1515558, LP: #1526209
TEST_F(ServerPlatformProbeMockDRM, returns_kms_platform_when_nested)
{
    using namespace testing;
    ON_CALL(mock_drm, drmSetMaster(_))
        .WillByDefault(Return(-1));

    mir::options::ProgramOption options;
    boost::program_options::options_description desc("");
    desc.add_options()
        ("host-socket", boost::program_options::value<std::string>(), "Host socket filename");
    std::array<char const*, 3> args {{ "./aserver", "--host-socket", "/dev/null" }};
    options.parse_arguments(desc, args.size(), args.data());

    auto block_mesa = ensure_mesa_probing_succeeds();

    auto modules = available_platforms();

    auto module = mir::graphics::module_for_device(
        modules,
        options,
        std::make_shared<StubConsoleServices>());
    ASSERT_NE(nullptr, module);

    auto descriptor = module->load_function<mir::graphics::DescribeModule>(describe_module);
    auto description = descriptor();

    EXPECT_THAT(description->name, HasSubstr("mesa-kms"));
}
#endif

TEST(ServerPlatformProbe, ThrowsExceptionWhenNothingProbesSuccessfully)
{
    using namespace testing;
    mir::options::ProgramOption options;
    auto block_mesa = ensure_mesa_probing_fails();

    EXPECT_THROW(
        mir::graphics::module_for_device(
            available_platforms(),
            options,
            std::make_shared<mtd::NullConsoleServices>()),
        std::runtime_error);
}

TEST(ServerPlatformProbe, LoadsSupportedModuleWhenNoBestModule)
{
    using namespace testing;
    mir::options::ProgramOption options;
    auto block_mesa = ensure_mesa_probing_fails();

    auto modules = available_platforms();
    add_dummy_platform(modules);

    auto module = mir::graphics::module_for_device(
        modules,
        options,
        std::make_shared<mtd::NullConsoleServices>());
    ASSERT_NE(nullptr, module);

    auto descriptor = module->load_function<mir::graphics::DescribeModule>(describe_module);
    auto description = descriptor();

    EXPECT_THAT(description->name, HasSubstr("mir:stub-graphics"));
}

TEST_F(ServerPlatformProbeMockDRM, IgnoresNonPlatformModules)
{
    using namespace testing;
    mir::options::ProgramOption options;
    auto ensure_mesa = ensure_mesa_probing_succeeds();

    auto modules = available_platforms();
    add_dummy_platform(modules);

    // NOTE: We want to load something that doesn't link with libmirplatform,
    // due to protobuf throwing a screaming hissy fit if it gets loaded twice.
    modules.push_back(std::make_shared<mir::SharedLibrary>(mtf::client_platform("dummy.so")));

    auto module = mir::graphics::module_for_device(
        modules,
        options,
        std::make_shared<StubConsoleServices>());
    EXPECT_NE(nullptr, module);
}
