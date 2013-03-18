
#include "mir_test/hw_mock.h"
#include <hardware/gralloc.h> 

namespace mt = mir::test;
namespace mtd = mir::test::doubles;

#include <system/window.h>
#include <memory>

namespace
{
mt::HardwareAccessMock* global_hw_mock = NULL;
}

mt::HardwareAccessMock::HardwareAccessMock()
{
    using namespace testing;
    assert(global_hw_mock == NULL && "Only one mock object per process is allowed");
    global_hw_mock = this;

    ad = std::make_shared<doubles::MockAllocDevice>();
    grmock = std::make_shared<mt::hardware_module_mock>(&ad->common);

    hd= std::make_shared<mt::MockHWCComposerDevice1>();
    hwmock = std::make_shared<mt::hardware_module_mock>(&hd->common);

    ON_CALL(*this, hw_get_module(StrEq(GRALLOC_HARDWARE_MODULE_ID),_))
        .WillByDefault(DoAll(SetArgPointee<1>(grmock.get()), Return(0)));
    ON_CALL(*this, hw_get_module(StrEq(HWC_HARDWARE_MODULE_ID),_))
        .WillByDefault(DoAll(SetArgPointee<1>(hwmock.get()), Return(0)));
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
