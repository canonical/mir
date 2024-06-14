#include "xdg_decoration_unstable_v1.h"

#include "mir/log.h"
#include "mir/shell/surface_specification.h"

#include "xdg-decoration-unstable-v1_wrapper.h"
#include "xdg_output_v1.h"
#include "xdg_shell_stable.h"

namespace mir
{
namespace frontend
{
class XdgDecorationManagerV1 : public wayland::XdgDecorationManagerV1
{
public:
    XdgDecorationManagerV1(wl_resource* resource);

    class Global : public wayland::XdgDecorationManagerV1::Global
    {
    public:
        Global(wl_display* display);

    private:
        void bind(wl_resource* new_zxdg_decoration_manager_v1) override;
    };

private:
    void get_toplevel_decoration(wl_resource* id, wl_resource* toplevel) override;
};

class XdgToplevelDecorationV1 : public wayland::XdgToplevelDecorationV1
{
public:
    XdgToplevelDecorationV1(wl_resource* id, mir::frontend::XdgToplevelStable* toplevel);

    void set_mode(uint32_t mode) override;
    void unset_mode() override;

private:
    void update_mode(uint32_t new_mode);

    static const uint32_t default_mode = Mode::client_side;

    mir::frontend::XdgToplevelStable* toplevel;
    uint32_t mode;
};
} // namespace frontend
} // namespace mir

auto mir::frontend::create_xdg_decoration_unstable_v1(wl_display* display)
    -> std::shared_ptr<mir::wayland::XdgDecorationManagerV1::Global>
{
    log_info("Creating XdgDecorationManagerV1::Global");
    return std::make_shared<XdgDecorationManagerV1::Global>(display);
}

mir::frontend::XdgDecorationManagerV1::Global::Global(wl_display* display)
    : wayland::XdgDecorationManagerV1::Global::Global{display, Version<1>{}}
{
}

void mir::frontend::XdgDecorationManagerV1::Global::bind(wl_resource* new_zxdg_decoration_manager_v1)
{
    log_info(__PRETTY_FUNCTION__);
    new XdgDecorationManagerV1{new_zxdg_decoration_manager_v1};
}

mir::frontend::XdgDecorationManagerV1::XdgDecorationManagerV1(wl_resource* resource)
    : mir::wayland::XdgDecorationManagerV1{resource, Version<1>{}}
{
}

void mir::frontend::XdgDecorationManagerV1::get_toplevel_decoration(wl_resource* id, wl_resource* toplevel)
{
    log_info(__PRETTY_FUNCTION__);
    auto* tl = mir::frontend::XdgToplevelStable::from(toplevel);
    if (tl)
    {
        new XdgToplevelDecorationV1{id, tl};
    }
}

mir::frontend::XdgToplevelDecorationV1::XdgToplevelDecorationV1(wl_resource* id,
                                                                mir::frontend::XdgToplevelStable* toplevel)
    : wayland::XdgToplevelDecorationV1{id, Version<1>{}}, toplevel{toplevel}
{
}

void mir::frontend::XdgToplevelDecorationV1::update_mode(uint32_t new_mode)
{
    log_info("Updating mode to: %s", new_mode == Mode::client_side ? "client-side" : "server-side");

    auto spec = shell::SurfaceSpecification{};

    mode = new_mode;
    switch (mode)
    {
    case Mode::client_side:
        spec.server_side_decorated = false;
        break;
    case Mode::server_side:
        spec.server_side_decorated = true;
        break;
    }

    this->toplevel->apply_spec(spec);
    send_configure_event(mode);
}

void mir::frontend::XdgToplevelDecorationV1::set_mode(uint32_t mode)
{
    log_info(__PRETTY_FUNCTION__);
    update_mode(mode);
}

void mir::frontend::XdgToplevelDecorationV1::unset_mode()
{
    log_info(__PRETTY_FUNCTION__);
    update_mode(default_mode);
}
