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

#include "mir/client_platform.h"
#include "mir/shared_library.h"
#include "mir/raii.h"
#include "src/platforms/mesa/client/mesa_native_display_container.h"
#include "mir_test_framework/client_platform_factory.h"

#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/mesa/native_display.h"
#include "mir_toolkit/mesa/platform_operation.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mcl = mir::client;
namespace mclm = mir::client::mesa;
namespace mtf = mir_test_framework;

namespace
{

struct StubClientContext : mcl::ClientContext
{
    void populate_server_package(MirPlatformPackage& platform_package) override
    {
        platform_package.data_items = 0;
        platform_package.fd_items = 1;
    }

};

struct MesaClientPlatformTest : testing::Test
{
    MirPlatformMessage* set_gbm_device(gbm_device* dev)
    {
        auto request_msg = mir::raii::deleter_for(
            mir_platform_message_create(MirMesaPlatformOperation::set_gbm_device),
            &mir_platform_message_release);
        MirMesaSetGBMDeviceRequest const request{dev};
        mir_platform_message_set_data(request_msg.get(), &request, sizeof(request));

        return platform->platform_operation(request_msg.get());
    }

    StubClientContext client_context;
    std::shared_ptr<mir::client::ClientPlatform> platform =
        mtf::create_mesa_client_platform(&client_context);
};

}

TEST_F(MesaClientPlatformTest, egl_native_display_is_valid_until_released)
{
    auto platform_lib = mtf::get_platform_library();

    MirMesaEGLNativeDisplay* nd;
    {
        std::shared_ptr<EGLNativeDisplayType> native_display = platform->create_egl_native_display();

        nd = reinterpret_cast<MirMesaEGLNativeDisplay*>(*native_display);
        auto validate = platform_lib->load_function<MirBool(*)(MirMesaEGLNativeDisplay*)>("mir_client_mesa_egl_native_display_is_valid");
        EXPECT_EQ(MIR_MESA_TRUE, validate(nd));
    }
    EXPECT_EQ(MIR_MESA_FALSE, mclm::mir_client_mesa_egl_native_display_is_valid(nd));
}

TEST_F(MesaClientPlatformTest, handles_set_gbm_device_platform_operation)
{
    using namespace testing;

    int const success{0};
    auto const gbm_dev_dummy = reinterpret_cast<gbm_device*>(this);

    auto response_msg = mir::raii::deleter_for(
        set_gbm_device(gbm_dev_dummy),
        &mir_platform_message_release);

    ASSERT_THAT(response_msg, NotNull());
    auto const response_data = mir_platform_message_get_data(response_msg.get());
    ASSERT_THAT(response_data.size, Eq(sizeof(MirMesaSetGBMDeviceResponse)));
    auto const response_ptr =
        reinterpret_cast<MirMesaSetGBMDeviceResponse const*>(response_data.data);
    EXPECT_THAT(response_ptr->status, Eq(success));
}

TEST_F(MesaClientPlatformTest, appends_gbm_device_to_platform_package)
{
    using namespace testing;

    MirPlatformPackage pkg;
    platform->populate(pkg);
    int const previous_data_count{pkg.data_items};
    auto const gbm_dev_dummy = reinterpret_cast<gbm_device*>(this);

    auto response_msg = mir::raii::deleter_for(
        set_gbm_device(gbm_dev_dummy),
        &mir_platform_message_release);

    platform->populate(pkg);
    EXPECT_THAT(pkg.data_items, Eq(previous_data_count + (sizeof(gbm_dev_dummy) / sizeof(int))));
    EXPECT_THAT(reinterpret_cast<struct gbm_device*>(pkg.data[previous_data_count]),
                Eq(gbm_dev_dummy));
}
