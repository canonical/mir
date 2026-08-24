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

#include "wayland.h"
#include "fractional_scale_v1.h"
#include "weak.h"

#include <mir/graphics/display_configuration.h>
#include <unordered_map>
#include <utility>

namespace mir
{
namespace frontend
{

class FractionalScaleManagerV1 : public wayland::FractionalScaleManagerV1
{
public:
    FractionalScaleManagerV1(
        std::shared_ptr<wayland::Client> client,
        rust::Box<wayland::FractionalScaleManagerV1Middleware> instance,
        uint32_t object_id);

private:
    auto get_fractional_scale(
        wayland::Weak<wayland::Surface> const& surface,
        rust::Box<wayland::FractionalScaleV1Middleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<wayland::FractionalScaleV1> override;
};

class FractionalScaleV1 : public wayland::FractionalScaleV1
{
public:
  FractionalScaleV1(
      std::shared_ptr<wayland::Client> client,
      rust::Box<wayland::FractionalScaleV1Middleware> instance,
      uint32_t object_id);


  void output_entered(mir::graphics::DisplayConfigurationOutput const& config);
  void output_left(mir::graphics::DisplayConfigurationOutput const& config);
  void scale_change_on_output(mir::graphics::DisplayConfigurationOutput const& config);

private:
  // Houses a set of outputs the surface occupies
  using Id = mir::graphics::DisplayConfigurationOutputId;
  std::unordered_map<Id, float> surface_outputs;

  void recompute_scale();
};

}
}

#endif // MIR_FRONTEND_FRACTIONAL_SCALE_V1_H
