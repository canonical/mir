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

#include <gtest/gtest.h>
#include <fcntl.h>
#include <boost/throw_exception.hpp>

#include "mir/graphics/platform.h"
#include "src/server/graphics/platform_probe.h"
#include "mir/options/program_option.h"
#include "mir/udev/wrapper.h"

#include "mir/raii.h"

#include "mir/test/doubles/mock_egl.h"
#if defined(MIR_BUILD_PLATFORM_GBM_KMS)
#include "mir/test/doubles/mock_drm.h"
#endif
#ifdef MIR_BUILD_PLATFORM_GBM_KMS
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

#ifdef MIR_BUILD_PLATFORM_GBM_KMS
    modules.push_back(std::make_shared<mir::SharedLibrary>(mtf::server_platform("graphics-gbm-kms")));
#endif
    return modules;
}

void add_dummy_platform(std::vector<std::shared_ptr<mir::SharedLibrary>>& modules)
{
    modules.insert(modules.begin(), std::make_shared<mir::SharedLibrary>(mtf::server_platform("graphics-dummy.so")));
}

void add_broken_platform(std::vector<std::shared_ptr<mir::SharedLibrary>>& modules)
{
    modules.insert(modules.begin(), std::make_shared<mir::SharedLibrary>(mtf::server_platform("graphics-throw.so")));
}

[[maybe_unused]]
std::shared_ptr<void> ensure_mesa_probing_fails()
{
    return std::make_shared<mtf::UdevEnvironment>();
}

[[maybe_unused]]
std::shared_ptr<void> ensure_mesa_probing_succeeds()
{
    using namespace testing;
    struct MockEnvironment {
        mtf::UdevEnvironment udev;
        testing::NiceMock<mtd::MockEGL> egl;
#ifdef MIR_BUILD_PLATFORM_GBM_KMS
        testing::NiceMock<mtd::MockGBM> gbm;
        testing::NiceMock<mtd::MockGL> gl;
#endif
    };
#ifdef MIR_BUILD_PLATFORM_GBM_KMS
    static auto const fake_gbm_device = reinterpret_cast<gbm_device*>(0xa1b2c3d4);
#endif
    static auto const fake_egl_display = reinterpret_cast<EGLDisplay>(0xeda);
    auto env = std::make_shared<MockEnvironment>();

    env->udev.add_standard_device("standard-drm-devices");
    ON_CALL(env->egl, eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS))
        .WillByDefault(Return("EGL_MESA_platform_gbm EGL_EXT_platform_base"));
#ifdef MIR_BUILD_PLATFORM_GBM_KMS
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
#ifdef MIR_BUILD_PLATFORM_GBM_KMS
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

    void register_lock_handler(
        std::function<void()> const&, std::function<void()> const&) override
    {
    }
};

class ServerPlatformProbeMockDRM : public ::testing::Test
{
#if defined(MIR_BUILD_PLATFORM_GBM_KMS)
public:
    ::testing::NiceMock<mtd::MockDRM> mock_drm;
#endif
};

}

TEST(ServerPlatformProbe, ConstructingWithNoModulesIsAnError)
{
    std::vector<std::shared_ptr<mir::SharedLibrary>> empty_modules;
    mir::options::ProgramOption options;

    EXPECT_THROW(mir::graphics::display_modules_for_device(empty_modules, options, nullptr),
                 std::runtime_error);
}

#ifdef MIR_BUILD_PLATFORM_GBM_KMS
TEST_F(ServerPlatformProbeMockDRM, LoadsMesaPlatformWhenDrmMasterCanBeAcquired)
{
    using namespace testing;
    mir::options::ProgramOption options;
    auto fake_mesa = ensure_mesa_probing_succeeds();

    auto modules = available_platforms();

    auto selection_result = mir::graphics::display_modules_for_device(
        modules,
        options,
        std::make_shared<StubConsoleServices>());

    std::vector<std::string> found_platforms;
    for (auto& [device, module] : selection_result)
    {
        auto descriptor = module->load_function<mir::graphics::DescribeModule>(describe_module);
        auto description = descriptor();
        found_platforms.emplace_back(description->name);
    }

    EXPECT_THAT(found_platforms, Contains(HasSubstr("gbm-kms")));
}

TEST_F(ServerPlatformProbeMockDRM, DoesNotLoadDummyPlatformWhenBetterPlatformExists)
{
    using namespace testing;
    mir::options::ProgramOption options;
    auto fake_mesa = ensure_mesa_probing_succeeds();

    auto modules = available_platforms();
    add_dummy_platform(modules);
    add_broken_platform(modules);

    auto selection_result = mir::graphics::display_modules_for_device(
        modules,
        options,
        std::make_shared<StubConsoleServices>());

    EXPECT_THAT(selection_result, Not(IsEmpty()));
    for (auto& [device, module] : selection_result)
    {
        EXPECT_THAT(device.support_level, Gt(mir::graphics::probe::dummy));
    }
}
#endif

TEST(ServerPlatformProbe, ThrowsExceptionWhenNothingProbesSuccessfully)
{
    using namespace testing;
    mir::options::ProgramOption options;
    auto block_mesa = ensure_mesa_probing_fails();

    auto modules = available_platforms();
    add_broken_platform(modules);

    EXPECT_THROW(
        mir::graphics::display_modules_for_device(
            modules,
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

    auto selection_result = mir::graphics::display_modules_for_device(
        modules,
        options,
        std::make_shared<mtd::NullConsoleServices>());
    ASSERT_THAT(selection_result, Not(IsEmpty()));

    std::vector<std::string> loaded_descriptors;
    for (auto const& [device, module] : selection_result)
    {
        auto descriptor = module->load_function<mir::graphics::DescribeModule>(describe_module);
        loaded_descriptors.emplace_back(descriptor()->name);
    }

    EXPECT_THAT(loaded_descriptors, Contains(HasSubstr("mir:stub-graphics")));
}

TEST(ServerPlatformProbe, IgnoresNonPlatformModules)
{
    using namespace testing;
    mir::options::ProgramOption options;

    std::vector<std::shared_ptr<mir::SharedLibrary>> modules;
    add_dummy_platform(modules);

    // NOTE: We want to load something that doesn't link with libmirplatform,
    modules.push_back(std::make_shared<mir::SharedLibrary>(mtf::library_path() + "/libmircore.so"));

    auto selected_modules = mir::graphics::display_modules_for_device(
        modules,
        options,
        std::make_shared<StubConsoleServices>());
    EXPECT_THAT(selected_modules, Not(IsEmpty()));
}
