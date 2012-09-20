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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mcl=mir::client;

class ICSRegistrarInterface
{
public:
    virtual int registerBuffer_interface(struct gralloc_module_t const* module, buffer_handle_t handle) = 0;
    virtual int unregisterBuffer_interface(struct gralloc_module_t const* module, buffer_handle_t handle) = 0;
    virtual int lock_interface(struct gralloc_module_t const* module, buffer_handle_t handle,
                     int usage, int l, int t, int w, int h, void** vaddr) = 0;
    virtual int unlock_interface(struct gralloc_module_t const* module, buffer_handle_t handle) = 0;
};

class MockRegistrarDevice : public ICSRegistrarInterface, 
                            public gralloc_module_t
{
public
    MockRegistrarDevice()
    {
        registerBuffer = hook_registerBuffer;
        unregisterBuffer = hook_unregisterBuffer;
        lock = hook_lock;
        unlock = hook_unlock;
    }

    MOCK_METHOD2(registerBuffer_interface, int (struct gralloc_module_t const*, buffer_handle_t));
    MOCK_METHOD2(unregisterBuffer_interface, int (struct gralloc_module_t const*, buffer_handle_t));
    MOCK_METHOD8(lock_interface, int(struct gralloc_module_t const*, buffer_handle_t,
                                     int, int, int, int, int, void**));
    MOCK_METHOD2(unlock_interface, int(struct gralloc_module_t const* module, buffer_handle_t handle));

    static int hook_registerBuffer(struct gralloc_module_t const* module, buffer_handle_t handle)
    {
        MockRegistrarDevice *registrar = static_cast<MockRegistrarDevice*>(module);
        return registrar->registerBufferInterface(module, handle);
    }
    static int hook_unregisterBuffer(struct gralloc_module_t const* module, buffer_handle_t handle)
    {

    }
    static int hook_lock(struct gralloc_module_t const* module, buffer_handle_t handle,
                     int usage, int l, int t, int w, int h, void** vaddr)
    {

    }
    static int hook_unlock(struct gralloc_module_t const* module, buffer_handle_t handle)
    {

    }
};

class ClientAndroidRegistrarTest : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
    }
    MockRegistrarDevice mock_device;
};

TEST_F(ClientAndroidRegistrarTest, registrar_init)
{
}
