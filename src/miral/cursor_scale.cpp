/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
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

#include "miral/cursor_scale.h"

#include "mir/options/option.h"
#include "mir/server.h"
#include "mir/shell/accessibility_manager.h"

#include <memory>

struct miral::CursorScale::Self
{
    Self(float default_scale): default_scale{default_scale} {}

    void scale(float new_scale)
    {
        scale_ = new_scale;

        if(accessibility_manager.expired())
            return;

        accessibility_manager.lock()->cursor_scale_changed(*scale_);
    }

    std::optional<float> scale_;
    float const default_scale;

    std::weak_ptr<mir::shell::AccessibilityManager> accessibility_manager;
};

miral::CursorScale::CursorScale()
    : self{std::make_shared<Self>(1.0)}
{
}

miral::CursorScale::CursorScale(float default_scale)
    : self{std::make_shared<Self>(default_scale)}
{
}

miral::CursorScale::~CursorScale() = default;

void miral::CursorScale::scale(float new_scale) const
{
    self->scale(new_scale);
}

void miral::CursorScale::operator()(mir::Server& server) const
{
    auto const* const cursor_scale_opt = "cursor-scale";
    server.add_configuration_option(
        cursor_scale_opt, "Scales the mouse cursor visually. Accepts any value in the range [0, 100]", self->default_scale);

    server.add_init_callback(
        [&server, this, cursor_scale_opt]
        {
            self->accessibility_manager = server.the_accessibility_manager();
            auto const& options = server.get_options();
            auto const scale = self->scale_.value_or(options->get<double>(cursor_scale_opt));

            self->scale(scale);
        });
}
