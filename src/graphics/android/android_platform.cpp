#include <mir/graphics/platform.h>

namespace mg=mir::graphics;

std::shared_ptr<mg::Platform> mg::create_platform()
{
    return std::make_shared<mg::Platform>();
}

