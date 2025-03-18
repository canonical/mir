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
#include "mir/log.h"

#include <memory>

struct miral::CursorScale::Self
{
    Self() {}

    Self(float default_scale): scale{default_scale} {}

    void set_scale(float new_scale)
    {
        scale = new_scale;
    }

    void apply_scale() const
    {
        if(accessibility_manager.expired())
        {
            mir::log_warning("AccessibilityManager not available. Will not set cursor scale");
            return;
        }

        if(!scale)
        {
            mir::log_warning("No scale value set. Will not set cursor scale");
            return;
        }

        accessibility_manager.lock()->cursor_scale_changed(*scale);
    }

    std::optional<float> scale;
    std::weak_ptr<mir::shell::AccessibilityManager> accessibility_manager;
};

miral::CursorScale::CursorScale()
    : self{std::make_shared<Self>()}
{
}

miral::CursorScale::CursorScale(float default_scale)
    : self{std::make_shared<Self>(default_scale)}
{
}

miral::CursorScale::~CursorScale() = default;

void miral::CursorScale::set_scale(float new_scale) const
{
    self->set_scale(new_scale);
}

void miral::CursorScale::apply_scale() const
{
    self->apply_scale();
}

void miral::CursorScale::operator()(mir::Server& server) const
{
    auto const* const cursor_scale_opt = "cursor-scale";
    server.add_configuration_option(
        cursor_scale_opt, "Scales the mouse cursor visually. Accepts any value in the range [0, 100]", 1.0);

    server.add_init_callback(
        [&server, this, cursor_scale_opt]
        {
            self->accessibility_manager = server.the_accessibility_manager();
            auto const& options = server.get_options();
            auto const scale = self->scale.value_or(options->get<double>(cursor_scale_opt));

            self->set_scale(scale);
        });
}
