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
#include "mir_test/mock_android_alloc_device.h"

#include <system/graphics.h>
#include <stdexcept>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mcl=mir::client;
namespace geom=mir::geometry;

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
        
        mock_module = std::shared_ptr<MockRegistrarDevice>( new MockRegistrarDevice );
        mock_addr = (gralloc_module_t*) mock_module.get();
        
        width = 41;
        height = 43;
        top = 0;
        left = 1;
        auto t = geom::X(top);
        auto l = geom::Y(left);
        auto pt = geom::Point{t,l};
        rect = geom::Rectangle{ pt,
                               {geom::Width(width), geom::Height(height)}};

        int numDataICS = 8; /* typical ICS/JB number of ints */
        numFd = 4; /*example value */
        int width_position_in_handle = numFd + ics_width_offset;
        int height_position_in_handle = numFd + ics_height_offset;
        int format_position_in_handle = numFd + ics_format_offset;

        auto handle_raw = mock_generate_sane_android_handle(numFd, numDataICS);
        handle_raw->data[width_position_in_handle] = width;
        handle_raw->data[height_position_in_handle] = height;
        handle_raw->data[format_position_in_handle] = HAL_PIXEL_FORMAT_RGBA_8888;
        fake_handle = std::shared_ptr<native_handle_t>(handle_raw);
    }
   
    geom::Rectangle rect;
    uint32_t width, height, top, left; 
    int numFd;
    static const int ics_width_offset = 5;
    static const int ics_height_offset = 6;
    static const int ics_format_offset = 7;
    int width_position_in_handle;
    int height_position_in_handle;

    std::shared_ptr<const native_handle_t> fake_handle;
    std::shared_ptr<MockRegistrarDevice> mock_module;
    gralloc_module_t * mock_addr;
};

TEST_F(ClientAndroidRegistrarTest, registrar_registers_using_module)
{
    using namespace testing;
    mcl::AndroidRegistrarGralloc registrar(mock_module);

    EXPECT_CALL(*mock_module, registerBuffer_interface(mock_addr, _))
        .Times(1);

    registrar.register_buffer(fake_handle.get());
}

TEST_F(ClientAndroidRegistrarTest, registrar_registers_buffer_given)
{
    using namespace testing;
    mcl::AndroidRegistrarGralloc registrar(mock_module);

    EXPECT_CALL(*mock_module, registerBuffer_interface(_, fake_handle.get()))
        .Times(1);

    registrar.register_buffer(fake_handle.get());
}

TEST_F(ClientAndroidRegistrarTest, registrar_unregisters_using_module)
{
    using namespace testing;
    mcl::AndroidRegistrarGralloc registrar(mock_module);

    EXPECT_CALL(*mock_module, unregisterBuffer_interface(mock_addr, _))
        .Times(1);

    registrar.unregister_buffer(fake_handle.get());
}

TEST_F(ClientAndroidRegistrarTest, registrar_unregisters_buffer_given)
{
    using namespace testing;
    mcl::AndroidRegistrarGralloc registrar(mock_module);

    EXPECT_CALL(*mock_module, unregisterBuffer_interface(_, fake_handle.get()))
        .Times(1);

    registrar.unregister_buffer(fake_handle.get());
}

TEST_F(ClientAndroidRegistrarTest, region_is_cleaned_up_correctly)
{
    using namespace testing;
    mcl::AndroidRegistrarGralloc registrar(mock_module);

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

    registrar.secure_for_cpu(fake_handle, rect );

    EXPECT_EQ(gralloc_dev_freed, gralloc_dev_alloc);
    EXPECT_EQ(handle_alloc, handle_freed);
}

TEST_F(ClientAndroidRegistrarTest, region_lock_usage_set_correctly)
{
    using namespace testing;
    mcl::AndroidRegistrarGralloc registrar(mock_module);

    int usage = GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN;

    EXPECT_CALL(*mock_module, lock_interface(_,_,usage,_,_,_,_,_))
        .Times(1);

    registrar.secure_for_cpu(fake_handle, rect );

}

TEST_F(ClientAndroidRegistrarTest, region_locks_from_top_corner)
{
    using namespace testing;
    mcl::AndroidRegistrarGralloc registrar(mock_module);

    EXPECT_CALL(*mock_module, lock_interface(_,_,_,top,_,_,_,_))
        .Times(1);

    registrar.secure_for_cpu(fake_handle, rect );
}

TEST_F(ClientAndroidRegistrarTest, region_locks_from_left_corner)
{
    using namespace testing;
    mcl::AndroidRegistrarGralloc registrar(mock_module);

    EXPECT_CALL(*mock_module, lock_interface(_,_,_,_,left,_,_,_))
        .Times(1);

    registrar.secure_for_cpu(fake_handle, rect );
}

TEST_F(ClientAndroidRegistrarTest, region_locks_with_right_width)
{
    using namespace testing;
    mcl::AndroidRegistrarGralloc registrar(mock_module);

    EXPECT_CALL(*mock_module, lock_interface(_,_,_,_,_,width,_,_))
        .Times(1);

    registrar.secure_for_cpu(fake_handle, rect );
}

TEST_F(ClientAndroidRegistrarTest, region_locks_with_right_height)
{
    using namespace testing;
    mcl::AndroidRegistrarGralloc registrar(mock_module);

    EXPECT_CALL(*mock_module, lock_interface(_,_,_,_,_,_,height,_))
        .Times(1);

    registrar.secure_for_cpu(fake_handle, rect );
}

#if 0
move to buffer test
TEST_F(ClientAndroidRegistrarTest, memory_handle_is_constructed_with_right_vaddr)
{
    using namespace testing;
    mcl::AndroidRegistrarGralloc registrar(mock_module);

    void * vaddr_fake = (void*) 0x84982;
    EXPECT_CALL(*mock_module, lock_interface(_,_,_,_,_,_,_,_))
        .Times(1)
        .WillOnce(DoAll(
            SetArgPointee<7>(vaddr_fake),
            Return(0)));

    auto region = registrar.secure_for_cpu(fake_handle);
    EXPECT_EQ(vaddr_fake, region->vaddr);
}

TEST_F(ClientAndroidRegistrarTest, memory_handle_is_constructed_with_right_width)
{
    using namespace testing;
    mcl::AndroidRegistrarGralloc registrar(mock_module);

    auto region = registrar.secure_for_cpu(fake_handle);
    EXPECT_EQ(width, region->width.as_uint32_t());
}

TEST_F(ClientAndroidRegistrarTest, memory_handle_is_constructed_with_right_height)
{
    using namespace testing;
    mcl::AndroidRegistrarGralloc registrar(mock_module);

    auto region = registrar.secure_for_cpu(fake_handle);
    EXPECT_EQ(height, region->height.as_uint32_t());
}

TEST_F(ClientAndroidRegistrarTest, memory_handle_is_constructed_with_right_format)
{
    using namespace testing;
    mcl::AndroidRegistrarGralloc registrar(mock_module);

    auto region = registrar.secure_for_cpu(fake_handle);
    EXPECT_EQ(geom::PixelFormat::rgba_8888, region->format);
}
#endif

TEST_F(ClientAndroidRegistrarTest, register_failure)
{
    using namespace testing;
    EXPECT_CALL(*mock_module, registerBuffer_interface(_, _))
        .Times(1)
        .WillOnce(Return(-1));
    
    mcl::AndroidRegistrarGralloc registrar(mock_module);
    EXPECT_THROW({
        registrar.register_buffer(fake_handle.get());
    }, std::runtime_error);

}

TEST_F(ClientAndroidRegistrarTest, lock_failure)
{
    using namespace testing;
    EXPECT_CALL(*mock_module, lock_interface(_,_,_,_,_,_,_,_))
        .Times(1)
        .WillOnce(Return(-1));

    mcl::AndroidRegistrarGralloc registrar(mock_module);

    EXPECT_THROW({
        registrar.secure_for_cpu(fake_handle, rect );
    }, std::runtime_error);
}

/* unregister is called in destructor. should not throw */
TEST_F(ClientAndroidRegistrarTest, unregister_failure)
{
    using namespace testing;
    EXPECT_CALL(*mock_module, unregisterBuffer_interface(_, _))
        .Times(1)
        .WillOnce(Return(-1));

    mcl::AndroidRegistrarGralloc registrar(mock_module);

    EXPECT_NO_THROW({
        registrar.unregister_buffer(fake_handle.get());
    });
}

/* unlock is called in destructor. should not throw */
TEST_F(ClientAndroidRegistrarTest, unlock_failure)
{
    using namespace testing;
    EXPECT_CALL(*mock_module, unlock_interface(_,_))
        .Times(1)
        .WillOnce(Return(-1));

    mcl::AndroidRegistrarGralloc registrar(mock_module);

    auto region = registrar.secure_for_cpu(fake_handle, rect );

    EXPECT_NO_THROW({
        region.reset();
    });

}
