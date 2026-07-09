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

#include "xdg_wm_dialog_v1.h"

#include "client.h"
#include "protocol_error.h"
#include "weak.h"
#include "xdg_shell_stable.h"

#include <cstdint>
#include <memory>
#include <unordered_set>
#include <utility>

namespace mir
{
namespace frontend
{
namespace
{
namespace mw = mir::wayland_rs;

class ToplevelsWithDialogs
{
public:
    ToplevelsWithDialogs() = default;
    ToplevelsWithDialogs(ToplevelsWithDialogs const&) = delete;
    ToplevelsWithDialogs& operator=(ToplevelsWithDialogs const&) = delete;

    /// \return true if no duplicates existed before insertion, false otherwise.
    bool register_toplevel(XdgToplevelStable* toplevel)
    {
        auto [_, inserted] = toplevels_with_dialogs.insert(toplevel);
        return inserted;
    }

    /// \return true if the toplevel was still registered, false otherwise.
    bool unregister_toplevel(XdgToplevelStable* toplevel)
    {
        return toplevels_with_dialogs.erase(toplevel) > 0;
    }

private:
    std::unordered_set<XdgToplevelStable*> toplevels_with_dialogs;
};

class XdgDialogV1 : public mw::XdgDialogV1
{
public:
    XdgDialogV1(
        std::shared_ptr<mw::Client> client,
        rust::Box<mw::XdgDialogV1Middleware> instance,
        uint32_t object_id);

    void set_modal() override;
    void unset_modal() override;
};

class XdgWmDialogV1 : public mw::XdgWmDialogV1
{
public:
    XdgWmDialogV1(
        std::shared_ptr<mw::Client> client,
        rust::Box<mw::XdgWmDialogV1Middleware> instance,
        uint32_t object_id);

private:
    auto get_xdg_dialog(
        mw::Weak<mw::XdgToplevel> const& toplevel,
        rust::Box<mw::XdgDialogV1Middleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<mw::XdgDialogV1> override;

    std::shared_ptr<ToplevelsWithDialogs> const toplevels_with_dialogs;
};
}
} // namespace frontend
} // namespace mir

namespace mf = mir::frontend;
namespace mw = mir::wayland_rs;

auto mf::create_xdg_wm_dialog_v1(
    std::shared_ptr<mw::Client> client,
    rust::Box<mw::XdgWmDialogV1Middleware> instance,
    uint32_t object_id) -> std::shared_ptr<mw::XdgWmDialogV1>
{
    return std::make_shared<XdgWmDialogV1>(std::move(client), std::move(instance), object_id);
}

mf::XdgWmDialogV1::XdgWmDialogV1(
    std::shared_ptr<mw::Client> client,
    rust::Box<mw::XdgWmDialogV1Middleware> instance,
    uint32_t object_id) :
    mw::XdgWmDialogV1{std::move(client), std::move(instance), object_id},
    toplevels_with_dialogs{std::make_shared<ToplevelsWithDialogs>()}
{
}

auto mf::XdgWmDialogV1::get_xdg_dialog(
    mw::Weak<mw::XdgToplevel> const& toplevel,
    rust::Box<mw::XdgDialogV1Middleware> child_instance,
    uint32_t child_object_id) -> std::shared_ptr<mw::XdgDialogV1>
{
    using Error = mw::XdgWmDialogV1::Error;

    auto* tl = XdgToplevelStable::from(toplevel);
    if (!tl)
    {
        throw std::runtime_error("Failed to obtain XdgToplevelStable from xdg_toplevel resource");
    }

    auto dialog = std::make_shared<XdgDialogV1>(client, std::move(child_instance), child_object_id);
    if (!toplevels_with_dialogs->register_toplevel(tl))
    {
        throw mw::ProtocolError{
            object_id(), Error::already_used, "xdg_dialog_v1 already created for this toplevel"};
    }

    dialog->add_destroy_listener(
        [toplevels_with_dialogs = this->toplevels_with_dialogs, tl]()
        {
            toplevels_with_dialogs->unregister_toplevel(tl);
        });

    static_cast<mw::XdgToplevel*>(tl)->add_destroy_listener(
        [toplevels_with_dialogs = this->toplevels_with_dialogs, tl]()
        {
            toplevels_with_dialogs->unregister_toplevel(tl);
        });

    tl->set_type(mir_window_type_dialog);

    return dialog;
}

mf::XdgDialogV1::XdgDialogV1(
    std::shared_ptr<mw::Client> client,
    rust::Box<mw::XdgDialogV1Middleware> instance,
    uint32_t object_id) :
    mw::XdgDialogV1{std::move(client), std::move(instance), object_id}
{
}

void mf::XdgDialogV1::set_modal()
{
    // In Mir, mir_window_type_dialog already represents a modal dialog.
    // No additional state change is needed.
}

void mf::XdgDialogV1::unset_modal()
{
    // Mir does not distinguish between modal and non-modal dialogs at the
    // window type level; the toplevel remains a dialog regardless.
}
