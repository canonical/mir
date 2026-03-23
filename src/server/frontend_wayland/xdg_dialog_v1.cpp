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

#include "xdg_dialog_v1.h"

#include <mir/wayland/protocol_error.h>

#include "xdg-dialog-v1_wrapper.h"
#include "xdg_shell_stable.h"

#include <memory>
#include <unordered_set>

namespace mir
{
namespace frontend
{
class ToplevelsWithDialogs
{
public:
    ToplevelsWithDialogs() = default;
    ToplevelsWithDialogs(ToplevelsWithDialogs const&) = delete;
    ToplevelsWithDialogs& operator=(ToplevelsWithDialogs const&) = delete;

    /// \return true if no duplicates existed before insertion, false otherwise.
    bool register_toplevel(wl_resource* toplevel)
    {
        auto [_, inserted] = toplevels_with_dialogs.insert(toplevel);
        return inserted;
    }

    /// \return true if the toplevel was still registered, false otherwise.
    bool unregister_toplevel(wl_resource* toplevel)
    {
        return toplevels_with_dialogs.erase(toplevel) > 0;
    }

private:
    std::unordered_set<wl_resource*> toplevels_with_dialogs;
};

class XdgDialogV1 : public wayland::XdgDialogV1
{
public:
    explicit XdgDialogV1(wl_resource* id);

    void set_modal() override;
    void unset_modal() override;
};

class XdgWmDialogV1 : public wayland::XdgWmDialogV1
{
public:
    XdgWmDialogV1(wl_resource* resource);

    class Global : public wayland::XdgWmDialogV1::Global
    {
    public:
        Global(wl_display* display);

    private:
        void bind(wl_resource* new_xdg_wm_dialog_v1) override;
    };

private:
    void get_xdg_dialog(wl_resource* id, wl_resource* toplevel) override;

    std::shared_ptr<ToplevelsWithDialogs> const toplevels_with_dialogs;
};
} // namespace frontend
} // namespace mir

auto mir::frontend::create_xdg_dialog_v1(wl_display* display)
    -> std::shared_ptr<mir::wayland::XdgWmDialogV1::Global>
{
    return std::make_shared<XdgWmDialogV1::Global>(display);
}

mir::frontend::XdgWmDialogV1::Global::Global(wl_display* display) :
    wayland::XdgWmDialogV1::Global::Global{display, Version<1>{}}
{
}

void mir::frontend::XdgWmDialogV1::Global::bind(wl_resource* new_xdg_wm_dialog_v1)
{
    new XdgWmDialogV1{new_xdg_wm_dialog_v1};
}

mir::frontend::XdgWmDialogV1::XdgWmDialogV1(wl_resource* resource) :
    wayland::XdgWmDialogV1{resource, Version<1>{}},
    toplevels_with_dialogs{std::make_shared<ToplevelsWithDialogs>()}
{
}

void mir::frontend::XdgWmDialogV1::get_xdg_dialog(wl_resource* id, wl_resource* toplevel)
{
    auto* tl = XdgToplevelStable::from(toplevel);
    if (!tl)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error(
            "Failed to obtain XdgToplevelStable from xdg_toplevel resource"));
    }

    if (!toplevels_with_dialogs->register_toplevel(toplevel))
    {
        BOOST_THROW_EXCEPTION(mir::wayland::ProtocolError(
            resource, Error::already_used, "xdg_dialog_v1 already created for this toplevel"));
    }

    auto* dialog = new XdgDialogV1{id};
    dialog->add_destroy_listener(
        [toplevels_with_dialogs = this->toplevels_with_dialogs, toplevel]()
        {
            toplevels_with_dialogs->unregister_toplevel(toplevel);
        });

    tl->add_destroy_listener(
        [toplevels_with_dialogs = this->toplevels_with_dialogs, toplevel]()
        {
            toplevels_with_dialogs->unregister_toplevel(toplevel);
        });

    tl->set_type(mir_window_type_dialog);
}

mir::frontend::XdgDialogV1::XdgDialogV1(wl_resource* id) :
    wayland::XdgDialogV1{id, Version<1>{}}
{
}

void mir::frontend::XdgDialogV1::set_modal()
{
    // In Mir, mir_window_type_dialog already represents a modal dialog.
    // No additional state change is needed.
}

void mir::frontend::XdgDialogV1::unset_modal()
{
    // Mir does not distinguish between modal and non-modal dialogs at the
    // window type level; the toplevel remains a dialog regardless.
}
