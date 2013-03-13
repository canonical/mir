
#include "mir_test/hw_mock.h"
#include <hardware/gralloc.h> 
#include <hardware/hwcomposer.h> 

namespace mt = mir::test;

namespace
{
mt::HardwareAccessMock* global_hw_mock = NULL;

namespace gralloc_mock
{


//todo: use the mock for the alloc device
struct alloc_device_t fake_device;
static struct native_handle fake_handle;

int hw_gralloc_alloc(struct alloc_device_t*,
            int, int, int, int,
            buffer_handle_t* handle, int* stride)
{
    *handle = (buffer_handle_t) &fake_handle;
    *stride = 44; 
    return 0;
}

int hw_gralloc_free(struct alloc_device_t*,
        buffer_handle_t)
{
    return 0;
}

int hw_close(struct hw_device_t*)
{
    return 0;
}

static int hw_open(const struct hw_module_t*, const char*, struct hw_device_t** device)
{
    fake_device.common.close = hw_close;
    fake_device.alloc = hw_gralloc_alloc;    
    fake_device.free = hw_gralloc_free;    
 
    *device = (hw_device_t*) &fake_device;
    return 0;
}

}


namespace hwc_mock
{

struct hw_device_t fake_device;

int hw_close(struct hw_device_t*)
{
    return 0;
}

static int hw_open(const struct hw_module_t*, const char*, struct hw_device_t** device)
{
    fake_device.close = hw_close;
    fake_device.version = HWC_DEVICE_API_VERSION_1_1;
 
    *device = (hw_device_t*) &fake_device;
    return 0;
}

}
}

mt::HardwareAccessMock::HardwareAccessMock()
{
    using namespace testing;
    assert(global_hw_mock == NULL && "Only one mock object per process is allowed");
    global_hw_mock = this;

    gr_methods.open = gralloc_mock::hw_open; 
    fake_hw_gr_module.methods = &gr_methods;
    fake_gr_device = (hw_device_t*) &gralloc_mock::fake_device;

    hwc_methods.open = hwc_mock::hw_open;
    fake_hw_hwc_module.methods = &hwc_methods;
    fake_hwc_device = &hwc_mock::fake_device;

    ON_CALL(*this, hw_get_module(StrEq(GRALLOC_HARDWARE_MODULE_ID),_))
        .WillByDefault(DoAll(SetArgPointee<1>(&fake_hw_gr_module), Return(0)));
    ON_CALL(*this, hw_get_module(StrEq(HWC_HARDWARE_MODULE_ID),_))
        .WillByDefault(DoAll(SetArgPointee<1>(&fake_hw_hwc_module), Return(0)));
}

mt::HardwareAccessMock::~HardwareAccessMock()
{
    global_hw_mock = NULL;
}

int hw_get_module(const char *id, const struct hw_module_t **module)
{
    if (!global_hw_mock)
    {
        ADD_FAILURE_AT(__FILE__,__LINE__); \
        return -1;
    }
    int rc =  global_hw_mock->hw_get_module(id, module);
    return rc;
}
