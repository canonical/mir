/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIRAL_DECORATION_DECORATION_ADAPTER_H
#define MIRAL_DECORATION_DECORATION_ADAPTER_H

#include "mir/shell/decoration_notifier.h"

#include <functional>
#include <memory>


namespace miral::decoration
{
class DecorationBuilder;
class DecorationAdapter : public mir::shell::decoration::Decoration
{
public:

    void handle_input_event(std::shared_ptr<MirEvent const> const& /*event*/) final override;
    void set_scale(float new_scale) final override;
    void attrib_changed(mir::scene::Surface const* window_surface, MirWindowAttrib attrib, int value) final override;
    void window_resized_to(
        mir::scene::Surface const* window_surface, mir::geometry::Size const& window_size) final override;
    void window_renamed(mir::scene::Surface const* window_surface, std::string const& name) final override;

private:
    friend DecorationBuilder;
    DecorationAdapter(
        std::shared_ptr<mir::scene::Surface> decoration_surface, std::shared_ptr<mir::scene::Surface> window_surface);

    std::function<void(std::shared_ptr<MirEvent const> const&)> on_handle_input_event;
    std::function<void(float new_scale)> on_set_scale;
    std::function<void(mir::scene::Surface const* window_surface, MirWindowAttrib attrib, int value)> on_attrib_changed;
    std::function<void(mir::scene::Surface const* window_surface, mir::geometry::Size const& window_size)>
        on_window_resized_to;
    std::function<void(mir::scene::Surface const* window_surface, std::string const& name)> on_window_renamed;

    mir::shell::decoration::DecorationNotifier decoration_notifier;
};

class DecorationBuilder
{
public:
    static auto build(
        std::shared_ptr<mir::scene::Surface> decoration_surface,
        std::shared_ptr<mir::scene::Surface> window_surface,
        std::function<void(std::shared_ptr<MirEvent const> const&)> on_handle_input_event,
        std::function<void(float new_scale)> on_set_scale,
        std::function<void(mir::scene::Surface const* window_surface, MirWindowAttrib attrib, int value)>
            on_attrib_changed,
        std::function<void(mir::scene::Surface const* window_surface, mir::geometry::Size const& window_size)>
            on_window_resized_to,
        std::function<void(mir::scene::Surface const* window_surface, std::string const& name)> on_window_renamed)
        -> DecorationBuilder;

    auto done() -> std::unique_ptr<DecorationAdapter>;

private:
    DecorationBuilder(
        std::shared_ptr<mir::scene::Surface> decoration_surface, std::shared_ptr<mir::scene::Surface> window_surface);

    std::unique_ptr<DecorationAdapter> adapter;
};
}
#endif
