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

#include "src/client/android/android_client_buffer.h"
#include "src/client/android/android_registrar_gralloc.h"
#include "mir_test_doubles/mock_android_alloc_device.h"
#include "mir_test/fake_shared.h"

#include <system/graphics.h>
#include <stdexcept>
#include <unistd.h>
#include <fcntl.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mcl=mir::client;
namespace mcla=mir::client::android;
namespace geom=mir::geometry;

class ICSRegistrarInterface
{
public:
    virtual ~ICSRegistrarInterface() {}
    virtual int registerBuffer_interface(struct gralloc_module_t const* module, buffer_handle_t handle) const = 0;
    virtual int unregisterBuffer_interface(struct gralloc_module_t const* module, buffer_handle_t handle) const = 0;
    virtual int lock_interface(struct gralloc_module_t const* module, buffer_handle_t handle,
                     int usage, int l, int t, int w, int h, void** vaddr) const = 0;
    virtual int unlock_interface(struct gralloc_module_t const* module, buffer_handle_t handle) const = 0;
};

class MockRegistrarDevice : public ICSRegistrarInterface,
                            public gralloc_module_t
{
public:
    MockRegistrarDevice()
    {
        registerBuffer = hook_registerBuffer;
        unregisterBuffer = hook_unregisterBuffer;
        lock = hook_lock;
        unlock = hook_unlock;
    }

    MOCK_CONST_METHOD2(registerBuffer_interface, int (struct gralloc_module_t const*, buffer_handle_t));
    MOCK_CONST_METHOD2(unregisterBuffer_interface, int (struct gralloc_module_t const*, buffer_handle_t));
    MOCK_CONST_METHOD8(lock_interface, int(struct gralloc_module_t const*, buffer_handle_t,
                                     int, int, int, int, int, void**));
    MOCK_CONST_METHOD2(unlock_interface, int(struct gralloc_module_t const* module, buffer_handle_t handle));

    static int hook_registerBuffer(struct gralloc_module_t const* module, buffer_handle_t handle)
    {
        const MockRegistrarDevice *registrar = static_cast<const MockRegistrarDevice*>(module);
        return registrar->registerBuffer_interface(module, handle);
    }
    static int hook_unregisterBuffer(struct gralloc_module_t const* module, buffer_handle_t handle)
    {
        const MockRegistrarDevice *registrar = static_cast<const MockRegistrarDevice*>(module);
        return registrar->unregisterBuffer_interface(module, handle);
    }
    static int hook_lock(struct gralloc_module_t const* module, buffer_handle_t handle,
                     int usage, int l, int t, int w, int h, void** vaddr)
    {
        const MockRegistrarDevice *registrar = static_cast<const MockRegistrarDevice*>(module);
        return registrar->lock_interface(module, handle, usage, l, t, w, h, vaddr);

    }
    static int hook_unlock(struct gralloc_module_t const* module, buffer_handle_t handle)
    {
        const MockRegistrarDevice *registrar = static_cast<const MockRegistrarDevice*>(module);
        return registrar->unlock_interface(module, handle);
    }
};
typedef ::testing::NiceMock<MockRegistrarDevice> StubRegistrarDevice;

class ClientAndroidRegistrarTest : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        using namespace testing;

        mock_module = std::make_shared<NiceMock<MockRegistrarDevice>>();

        width = 41;
        height = 43;
        top = 0;
        left = 1;
        auto t = geom::X(top);
        auto l = geom::Y(left);
        auto pt = geom::Point{t,l};
        rect = geom::Rectangle{ pt, {width, height}};
        package = std::make_shared<MirBufferPackage>();
        package->fd_items = 4;
        package->data_items = 21;
        for(auto i=0; i < package->fd_items; i++)
            package->fd[i] = (i*4);
        for(auto i=0; i < package->data_items; i++)
            package->fd[i] = (i*3);
    }

    geom::Rectangle rect;
    uint32_t width, height, top, left;

    std::shared_ptr<MirBufferPackage> package;
    std::shared_ptr<MockRegistrarDevice> mock_module;
};

TEST_F(ClientAndroidRegistrarTest, client_buffer_converts_package)
{
    mcla::AndroidRegistrarGralloc registrar(mock_module);
    auto handle = registrar.register_buffer(package);

    ASSERT_NE(nullptr, handle);
    ASSERT_EQ(package->fd_items, handle->numFds);
    ASSERT_EQ(package->data_items, handle->numInts);
    for(auto i = 0; i < package->fd_items; i++)
        EXPECT_EQ(package->fd[i], handle->data[i]);
    for(auto i = 0; i < package->data_items; i++)
        EXPECT_EQ(package->data[i], handle->data[i + package->fd_items]);
}

TEST_F(ClientAndroidRegistrarTest, client_sets_correct_version)
{
    mcla::AndroidRegistrarGralloc registrar(mock_module);
    auto handle = registrar.register_buffer(package);
    EXPECT_EQ(handle->version, static_cast<int>(sizeof(native_handle_t)));
}

TEST_F(ClientAndroidRegistrarTest, registrar_registers_using_module)
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

    mcla::AndroidRegistrarGralloc registrar(mock_module);
    {
        auto handle = registrar.register_buffer(package);
        EXPECT_EQ(handle1, handle.get());
    }
    EXPECT_EQ(handle1, handle2);
}

TEST_F(ClientAndroidRegistrarTest, registrar_frees_fds)
{
    using namespace testing;
    StubRegistrarDevice stub_module;
    auto package = std::make_shared<MirBufferPackage>();
    package->data_items = 0;
    package->fd_items = 2;

    int ret = pipe(static_cast<int*>(package->fd));
    {
        mcla::AndroidRegistrarGralloc registrar(mir::test::fake_shared(stub_module));
        auto handle = registrar.register_buffer(package);
    }

    EXPECT_EQ(0, ret);
    EXPECT_EQ(-1, fcntl(package->fd[0], F_GETFD));
    EXPECT_EQ(-1, fcntl(package->fd[1], F_GETFD));
}


TEST_F(ClientAndroidRegistrarTest, register_failure)
{
    using namespace testing;
    EXPECT_CALL(*mock_module, registerBuffer_interface(_, _))
        .Times(1)
        .WillOnce(Return(-1));
    EXPECT_CALL(*mock_module, unregisterBuffer_interface(_,_))
        .Times(0);

    mcla::AndroidRegistrarGralloc registrar(mock_module);
    EXPECT_THROW({
        registrar.register_buffer(package);
    }, std::runtime_error);

}

class AndroidSoftwareRegionTest : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        using namespace testing;

        mock_module = std::make_shared<NiceMock<MockRegistrarDevice>>();

        width = 41;
        height = 43;
        top = 0;
        left = 1;
        auto t = geom::X(top);
        auto l = geom::Y(left);
        auto pt = geom::Point{t,l};
        rect = geom::Rectangle{ pt, {width, height}};
        fake_package = std::make_shared<native_handle_t>();
    }

    geom::Rectangle rect;
    uint32_t width, height, top, left;

    std::shared_ptr<native_handle_t> fake_package;
    std::shared_ptr<MockRegistrarDevice> mock_module;
};

TEST_F(AndroidSoftwareRegionTest, region_is_cleaned_up_correctly)
{
    using namespace testing;
    mcla::AndroidRegistrarGralloc registrar(mock_module);

    const gralloc_module_t *gralloc_dev_alloc, *gralloc_dev_freed;
    const native_handle_t *handle_alloc, *handle_freed;

    EXPECT_CALL(*mock_module, lock_interface(_,_,_,_,_,_,_,_))
        .Times(1)
        .WillOnce(DoAll(
        SaveArg<0>(&gralloc_dev_alloc),
        SaveArg<1>(&handle_alloc),
        Return(0)));
    EXPECT_CALL(*mock_module, unlock_interface(_,_))
        .Times(1)
        .WillOnce(DoAll(
        SaveArg<0>(&gralloc_dev_freed),
        SaveArg<1>(&handle_freed),
        Return(0)));

    registrar.secure_for_cpu(fake_package, rect);

    EXPECT_EQ(gralloc_dev_freed, gralloc_dev_alloc);
    EXPECT_EQ(handle_alloc, handle_freed);
}

TEST_F(AndroidSoftwareRegionTest, region_lock_usage_set_correctly)
{
    using namespace testing;
    mcla::AndroidRegistrarGralloc registrar(mock_module);

    int usage = GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN;

    EXPECT_CALL(*mock_module, lock_interface(_,_,usage,_,_,_,_,_))
        .Times(1);

    registrar.secure_for_cpu(fake_package, rect);

}

TEST_F(AndroidSoftwareRegionTest, region_locks_from_top_corner)
{
    using namespace testing;
    mcla::AndroidRegistrarGralloc registrar(mock_module);

    EXPECT_CALL(*mock_module, lock_interface(_,_,_,top,_,_,_,_))
        .Times(1);

    registrar.secure_for_cpu(fake_package, rect);
}

TEST_F(AndroidSoftwareRegionTest, region_locks_from_left_corner)
{
    using namespace testing;
    mcla::AndroidRegistrarGralloc registrar(mock_module);

    EXPECT_CALL(*mock_module, lock_interface(_,_,_,_,left,_,_,_))
        .Times(1);

    registrar.secure_for_cpu(fake_package, rect);
}

TEST_F(AndroidSoftwareRegionTest, region_locks_with_right_width)
{
    using namespace testing;
    mcla::AndroidRegistrarGralloc registrar(mock_module);

    EXPECT_CALL(*mock_module, lock_interface(_,_,_,_,_,width,_,_))
        .Times(1);

    registrar.secure_for_cpu(fake_package, rect);
}

TEST_F(AndroidSoftwareRegionTest, region_locks_with_right_height)
{
    using namespace testing;
    mcla::AndroidRegistrarGralloc registrar(mock_module);

    EXPECT_CALL(*mock_module, lock_interface(_,_,_,_,_,_,height,_))
        .Times(1);

    registrar.secure_for_cpu(fake_package, rect);
}


TEST_F(AndroidSoftwareRegionTest, lock_failure)
{
    using namespace testing;
    EXPECT_CALL(*mock_module, lock_interface(_,_,_,_,_,_,_,_))
        .Times(1)
        .WillOnce(Return(-1));

    mcla::AndroidRegistrarGralloc registrar(mock_module);

    EXPECT_THROW({
        registrar.secure_for_cpu(fake_package, rect);
    }, std::runtime_error);
}

/* unlock is called in destructor. should not throw */
TEST_F(AndroidSoftwareRegionTest, unlock_failure)
{
    using namespace testing;
    EXPECT_CALL(*mock_module, unlock_interface(_,_))
        .Times(1)
        .WillOnce(Return(-1));

    mcla::AndroidRegistrarGralloc registrar(mock_module);

    auto region = registrar.secure_for_cpu(fake_package, rect);

    EXPECT_NO_THROW({
        region.reset();
    });
}
