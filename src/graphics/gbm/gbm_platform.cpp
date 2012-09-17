#include "mir/graphics/gbm/gbm_platform.h"

namespace mgg=mir::graphics::gbm;
namespace mg=mir::graphics;

mgg::GBMPlatform::GBMPlatform()
{
    drm.setup();
    gbm.setup(drm);
}

std::shared_ptr<mg::Platform> mg::create_platform()
{
    return std::make_shared<mgg::GBMPlatform>();
}
