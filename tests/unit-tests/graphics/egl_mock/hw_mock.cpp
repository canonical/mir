
#include "mir_test/hw_mock.h"
#include <hardware/gralloc.h> 
#include <hardware/hwcomposer.h> 

namespace mt = mir::test;
namespace mtd = mir::test::doubles;

#include <system/window.h>
#include <memory>

namespace
{

class StubANativeWindow : public ANativeWindow
{
public:
    ~StubANativeWindow() {};
    StubANativeWindow()
        : fake_visual_id(5)
    {
        using namespace testing;
        query = hook_query;
    }

    static int hook_query(const ANativeWindow* anw, int code, int *ret)
    {
        const StubANativeWindow* mocker = static_cast<const StubANativeWindow*>(anw);
        return mocker->query_interface(anw, code, ret);
    }

    int query_interface(const ANativeWindow*,int,int* b) const
    {
        *b = fake_visual_id;
        return 0;
    }

    int fake_visual_id;
};

mt::HardwareAccessMock* global_hw_mock = NULL;
//std::shared_ptr<StubANativeWindow> global_android_fb_win;


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

    ad = std::make_shared<doubles::MockAllocDevice>();
    grmock = std::make_shared<mt::gralloc_module_mock>(&ad->common);


    hwc_methods.open = hwc_mock::hw_open;
    fake_hw_hwc_module.methods = &hwc_methods;
    fake_hwc_device = (hw_device_t*) &hwc_mock::fake_device;
    fake_hwc_device->version = HWC_DEVICE_API_VERSION_1_1;

    ON_CALL(*this, hw_get_module(StrEq(GRALLOC_HARDWARE_MODULE_ID),_))
        .WillByDefault(DoAll(SetArgPointee<1>(grmock.get()), Return(0)));
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
