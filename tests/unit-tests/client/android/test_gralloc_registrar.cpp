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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "src/client/android/gralloc_registrar.h"
#include <stdexcept>
#include <fcntl.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mcla=mir::client::android;
namespace geom=mir::geometry;

class MockRegistrarDevice : public gralloc_module_t
{
public:
    MockRegistrarDevice()
    {
        registerBuffer = hook_registerBuffer;
        unregisterBuffer = hook_unregisterBuffer;
        lock = hook_lock;
        unlock = hook_unlock;
    }

    MOCK_CONST_METHOD2(registerBuffer_interface,
        int(struct gralloc_module_t const*, buffer_handle_t));
    MOCK_CONST_METHOD2(unregisterBuffer_interface,
        int(struct gralloc_module_t const*, buffer_handle_t));
    MOCK_CONST_METHOD8(lock_interface,
        int(struct gralloc_module_t const*, buffer_handle_t,
            int, int, int, int, int, void**));
    MOCK_CONST_METHOD2(unlock_interface,
        int(struct gralloc_module_t const*, buffer_handle_t));

    static int hook_registerBuffer(
        struct gralloc_module_t const* module, buffer_handle_t handle)
    {
        auto registrar = static_cast<const MockRegistrarDevice*>(module);
        return registrar->registerBuffer_interface(module, handle);
    }

    static int hook_unregisterBuffer(
        struct gralloc_module_t const* module, buffer_handle_t handle)
    {
        auto registrar = static_cast<const MockRegistrarDevice*>(module);
        return registrar->unregisterBuffer_interface(module, handle);
    }

    static int hook_lock(
        struct gralloc_module_t const* module, buffer_handle_t handle,
        int usage, int l, int t, int w, int h, void** vaddr)
    {
        auto registrar = static_cast<const MockRegistrarDevice*>(module);
        return registrar->lock_interface(
            module, handle, usage, l, t, w, h, vaddr);
    }

    static int hook_unlock(
        struct gralloc_module_t const* module, buffer_handle_t handle)
    {
        auto registrar = static_cast<const MockRegistrarDevice*>(module);
        return registrar->unlock_interface(module, handle);
    }
};

struct GrallocRegistrar : public ::testing::Test
{
    GrallocRegistrar() :
        mock_module(std::make_shared<testing::NiceMock<MockRegistrarDevice>>()),
        stub_package(std::make_shared<MirBufferPackage>()),
        stub_handle(std::make_shared<native_handle_t>())
    {
        stub_package->fd_items = 4;
        stub_package->data_items = 21;
        for(auto i=0; i < stub_package->fd_items; i++)
            stub_package->fd[i] = (i*4);
        for(auto i=0; i < stub_package->data_items; i++)
            stub_package->fd[i] = (i*3);
    }

    uint32_t const width{41};
    uint32_t const height{43};
    uint32_t const top{0};
    uint32_t const left{1};
    geom::Rectangle const rect{geom::Point{top, left}, geom::Size{width, height}};

    std::shared_ptr<MockRegistrarDevice> const mock_module;
    std::shared_ptr<MirBufferPackage> const stub_package;
    std::shared_ptr<native_handle_t> const stub_handle;
};

TEST_F(GrallocRegistrar, client_buffer_converts_stub_package)
{
    mcla::GrallocRegistrar registrar(mock_module);
    auto handle = registrar.register_buffer(stub_package);

    ASSERT_NE(nullptr, handle);
    ASSERT_EQ(stub_package->fd_items, handle->numFds);
    ASSERT_EQ(stub_package->data_items, handle->numInts);
    for(auto i = 0; i < stub_package->fd_items; i++)
        EXPECT_EQ(stub_package->fd[i], handle->data[i]);
    for(auto i = 0; i < stub_package->data_items; i++)
        EXPECT_EQ(stub_package->data[i], handle->data[i + stub_package->fd_items]);
}

TEST_F(GrallocRegistrar, client_sets_correct_version)
{
    mcla::GrallocRegistrar registrar(mock_module);
    auto handle = registrar.register_buffer(stub_package);
    EXPECT_EQ(handle->version, static_cast<int>(sizeof(native_handle_t)));
}

TEST_F(GrallocRegistrar, registrar_registers_using_module)
{
    using namespace testing;
    native_handle_t const* handle1 = nullptr;
    native_handle_t const* handle2 = nullptr;

    EXPECT_CALL(*mock_module, registerBuffer_interface(mock_module.get(),_))
        .Times(1)
        .WillOnce(DoAll(SaveArg<1>(&handle1), Return(0)));
    EXPECT_CALL(*mock_module, unregisterBuffer_interface(mock_module.get(),_))
        .Times(1)
        .WillOnce(DoAll(SaveArg<1>(&handle2), Return(0)));

    mcla::GrallocRegistrar registrar(mock_module);
    {
        auto handle = registrar.register_buffer(stub_package);
        EXPECT_EQ(handle1, handle.get());
    }
    EXPECT_EQ(handle1, handle2);
}

TEST_F(GrallocRegistrar, registrar_frees_fds)
{
    using namespace testing;
    auto stub_package = std::make_shared<MirBufferPackage>();
    stub_package->data_items = 0;
    stub_package->fd_items = 2;

    int ret = pipe(static_cast<int*>(stub_package->fd));
    {
        mcla::GrallocRegistrar registrar(mock_module);
        auto handle = registrar.register_buffer(stub_package);
    }

    EXPECT_EQ(0, ret);
    EXPECT_EQ(-1, fcntl(stub_package->fd[0], F_GETFD));
    EXPECT_EQ(-1, fcntl(stub_package->fd[1], F_GETFD));
}


TEST_F(GrallocRegistrar, register_failure)
{
    using namespace testing;
    EXPECT_CALL(*mock_module, registerBuffer_interface(_, _))
        .Times(1)
        .WillOnce(Return(-1));
    EXPECT_CALL(*mock_module, unregisterBuffer_interface(_,_))
        .Times(0);

    mcla::GrallocRegistrar registrar(mock_module);
    EXPECT_THROW({
        registrar.register_buffer(stub_package);
    }, std::runtime_error);

}

TEST_F(GrallocRegistrar, region_is_cleaned_up_correctly)
{
    using namespace testing;
    mcla::GrallocRegistrar registrar(mock_module);

    gralloc_module_t const* gralloc_dev_alloc{nullptr};
    gralloc_module_t const* gralloc_dev_freed{nullptr};
    native_handle_t const* handle_alloc{nullptr};
    native_handle_t const* handle_freed{nullptr};

    Sequence seq;
    EXPECT_CALL(*mock_module, lock_interface(_,_,_,_,_,_,_,_))
        .InSequence(seq)
        .WillOnce(DoAll(
            SaveArg<0>(&gralloc_dev_alloc),
            SaveArg<1>(&handle_alloc),
            Return(0)));
    EXPECT_CALL(*mock_module, unlock_interface(_,_))
        .InSequence(seq)
        .WillOnce(DoAll(
            SaveArg<0>(&gralloc_dev_freed),
            SaveArg<1>(&handle_freed),
            Return(0)));

    registrar.secure_for_cpu(stub_handle, rect);

    EXPECT_EQ(gralloc_dev_freed, gralloc_dev_alloc);
    EXPECT_EQ(handle_alloc, handle_freed);
}

TEST_F(GrallocRegistrar, maps_buffer_for_cpu_correctly)
{
    EXPECT_CALL(*mock_module, lock_interface(
        mock_module.get(),
        stub_handle.get(),
        GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN,
        top,
        left,
        width,
        height,
        testing::_))
        .Times(1);

    mcla::GrallocRegistrar registrar(mock_module);
    registrar.secure_for_cpu(stub_handle, rect);
}

TEST_F(GrallocRegistrar, lock_failure_throws)
{
    using namespace testing;
    EXPECT_CALL(*mock_module, lock_interface(_,_,_,_,_,_,_,_))
        .Times(1)
        .WillOnce(Return(-1));

    mcla::GrallocRegistrar registrar(mock_module);

    EXPECT_THROW({
        registrar.secure_for_cpu(stub_handle, rect);
    }, std::runtime_error);
}

/* unlock is called in destructor. should not throw */
TEST_F(GrallocRegistrar, unlock_failure_doesnt_throw)
{
    using namespace testing;
    EXPECT_CALL(*mock_module, unlock_interface(_,_))
        .Times(1)
        .WillOnce(Return(-1));
    mcla::GrallocRegistrar registrar(mock_module);

    auto region = registrar.secure_for_cpu(stub_handle, rect);
    EXPECT_NO_THROW({
        region.reset();
    });
}
