/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */
#ifndef MIR_TEST_DOUBLES_MOCK_ANDROID_ALLOC_DEVICE_H_
#define MIR_TEST_DOUBLES_MOCK_ANDROID_ALLOC_DEVICE_H_

#include <gmock/gmock.h>
#include <hardware/gralloc.h>

namespace mir
{
namespace test
{
namespace android
{
class ICSAllocInterface
{
public:
    virtual ~ICSAllocInterface() {/* TODO: make nothrow */}
    virtual int alloc_interface(alloc_device_t* dev, int w, int h,
                                int format, int usage, buffer_handle_t* handle, int* stride) = 0;
    virtual int free_interface(alloc_device_t* dev, buffer_handle_t handle) = 0;
    virtual int dump_interface(alloc_device_t* dev, char *buf, int len) = 0;

};
}
namespace doubles
{
class MockAllocDevice : public android::ICSAllocInterface,
    public alloc_device_t
{
public:

    MockAllocDevice()
    {
        using namespace testing;

        buffer_handle = mock_generate_sane_android_handle(43, 22);
        fake_stride = 300;

        alloc = hook_alloc;
        free = hook_free;
        dump = hook_dump;
        ON_CALL(*this, alloc_interface(_,_,_,_,_,_,_))
        .WillByDefault(DoAll(
                           SetArgPointee<5>(buffer_handle),
                           SetArgPointee<6>(fake_stride),
                           Return(0)));
        ON_CALL(*this, free_interface(_,_))
        .WillByDefault(Return(0));

    }

    ~MockAllocDevice()
    {
        ::free((void*)buffer_handle);
    }

    static int hook_alloc(alloc_device_t* mock_alloc,
                          int w, int h, int format, int usage,
                          buffer_handle_t* handle, int* stride)
    {
        MockAllocDevice* mocker = static_cast<MockAllocDevice*>(mock_alloc);
        return mocker->alloc_interface(mock_alloc, w, h, format, usage, handle, stride);
    }

    static int hook_free(alloc_device_t* mock_alloc, buffer_handle_t handle)
    {
        MockAllocDevice* mocker = static_cast<MockAllocDevice*>(mock_alloc);
        return mocker->free_interface(mock_alloc, handle);
    }

    static void hook_dump(alloc_device_t* mock_alloc, char* buf, int buf_len)
    {
        MockAllocDevice* mocker = static_cast<MockAllocDevice*>(mock_alloc);
        mocker->dump_interface(mock_alloc, buf, buf_len);
    }

    native_handle_t* mock_generate_sane_android_handle(int numFd, int numInt)
    {
        native_handle_t *handle;
        int total=numFd + numInt;
        int header_offset=3;

        handle = (native_handle_t*) malloc(sizeof(int) * (header_offset+ total));
        handle->version = 0x389;
        handle->numFds = numFd;
        handle->numInts = numInt;
        for(int i=0; i<total; i++)
        {
            handle->data[i] = i*3;
        }

        return handle;
    }

    MOCK_METHOD7(alloc_interface,  int(alloc_device_t*, int, int, int, int, buffer_handle_t*, int*));
    MOCK_METHOD2(free_interface, int(alloc_device_t*, buffer_handle_t));
    MOCK_METHOD3(dump_interface, int(alloc_device_t*, char*, int));

    native_handle_t* buffer_handle;
    unsigned int fake_stride;
};
}
}
} // namespace mir

#endif /* MIR_TEST_DOUBLES_MOCK_ANDROID_ALLOC_DEVICE_H_ */
