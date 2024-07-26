/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_FRONTEND_FRACTIONAL_SCALE_V1_H
#define MIR_FRONTEND_FRACTIONAL_SCALE_V1_H

#include "fractional-scale-v1_wrapper.h"
#include "output_manager.h"
#include <unordered_set>

namespace mir
{
namespace frontend
{

auto create_fractional_scale_v1(wl_display *display)
    -> std::shared_ptr<wayland::FractionalScaleManagerV1::Global>;

class FractionalScaleV1 : public wayland::FractionalScaleV1
{
public:
  FractionalScaleV1(struct wl_resource *resource);

  void output_entered(OutputInstance* output);
  void output_left(OutputInstance* output);

private:
  // Houses a set of outputs the surface occupies
  std::unordered_set<OutputInstance*> surface_outputs;

  void recompute_scale();
};

}
}

#endif // MIR_FRONTEND_FRACTIONAL_SCALE_V1_H
