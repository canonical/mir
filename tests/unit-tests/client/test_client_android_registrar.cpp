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

#include "mir_client/android_client_buffer.h"
#include "mir_client/mir_buffer_package.h"
#include "mir_client/android_registrar_gralloc.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mcl=mir::client;

class ICSRegistrarInterface
{
public:
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

class ClientAndroidRegistrarTest : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        mock_module = std::shared_ptr<gralloc_module_t>( &mock_reg_device );
        fake_handle = (native_handle_t*) 0x482;
    }

    native_handle_t * fake_handle;
    std::shared_ptr<gralloc_module_t> mock_module;
    MockRegistrarDevice mock_reg_device;
};

TEST_F(ClientAndroidRegistrarTest, registrar_registers_using_module)
{
    using namespace testing;
    mcl::AndroidRegistrarGralloc registrar(mock_module);

    EXPECT_CALL(mock_reg_device, registerBuffer_interface(mock_module.get(), _))
        .Times(1);

    registrar.register_buffer(fake_handle);
}

TEST_F(ClientAndroidRegistrarTest, registrar_registers_buffer_given)
{
    using namespace testing;
    mcl::AndroidRegistrarGralloc registrar(mock_module);

    EXPECT_CALL(mock_reg_device, registerBuffer_interface(_, fake_handle))
        .Times(1);

    registrar.register_buffer(fake_handle);
}

TEST_F(ClientAndroidRegistrarTest, registrar_unregisters_using_module)
{
    using namespace testing;
    mcl::AndroidRegistrarGralloc registrar(mock_module);

    EXPECT_CALL(mock_reg_device, unregisterBuffer_interface(mock_module.get(), _))
        .Times(1);

    registrar.unregister_buffer(fake_handle);
}

TEST_F(ClientAndroidRegistrarTest, registrar_unregisters_buffer_given)
{
    using namespace testing;
    mcl::AndroidRegistrarGralloc registrar(mock_module);

    EXPECT_CALL(mock_reg_device, unregisterBuffer_interface(_, fake_handle))
        .Times(1);

    registrar.unregister_buffer(fake_handle);
}

TEST_F(ClientAndroidRegistrarTest, region_is_cleaned_up_correctly)
{
    using namespace testing;
    mcl::AndroidRegistrarGralloc registrar(mock_module);

    const gralloc_module_t *gralloc_dev_alloc, *gralloc_dev_freed;
    const native_handle_t *handle_alloc, *handle_freed;

    EXPECT_CALL(mock_reg_device, lock_interface(_,_,_,_,_,_,_,_))
        .Times(1)
        .WillOnce(DoAll(
        SaveArg<0>(&gralloc_dev_alloc),
        SaveArg<1>(&handle_alloc),
        Return(0)));
    EXPECT_CALL(mock_reg_device, unlock_interface(_,_))
        .Times(1)
        .WillOnce(DoAll(
        SaveArg<0>(&gralloc_dev_freed),
        SaveArg<1>(&handle_freed),
        Return(0)));

    registrar.secure_for_cpu(fake_handle);

    EXPECT_EQ(gralloc_dev_freed, gralloc_dev_alloc);
    EXPECT_EQ(handle_alloc, handle_freed);
}
