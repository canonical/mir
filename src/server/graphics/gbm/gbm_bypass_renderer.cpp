#include "gbm_bypass_renderer.h"
#include "mir/graphics/renderable.h"
#include "mir/surfaces/graphic_region.h"

using namespace mir::graphics::gbm;

void GBMBypassRenderer::render(std::function<void(std::shared_ptr<void> const&)> save_resource, Renderable& renderable)
{
    (void)save_resource;
    (void)renderable;
}

void GBMBypassRenderer::clear()
{
}

