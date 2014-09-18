#ifndef COORDINATE_TRANSLATOR_H
#define COORDINATE_TRANSLATOR_H

#include "mir/geometry/point.h"
#include <memory>

namespace mir
{
namespace frontend
{
class Surface;

class CoordinateTranslator
{
public:
    virtual ~CoordinateTranslator() = default;

    virtual geometry::Point surface_to_screen(std::shared_ptr<Surface> surface,
                                              uint32_t x, uint32_t y) = 0;
};


}
}

#endif // COORDINATE_TRANSLATOR_H
