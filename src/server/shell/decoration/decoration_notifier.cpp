#include <mir/shell/decoration_notifier.h>

#include "mir/scene/null_surface_observer.h"
#include "mir/scene/surface.h"

namespace msd = mir::shell::decoration;

struct msd::DecorationNotifier::InputObserver : public mir::scene::NullSurfaceObserver
{
    InputObserver(Decoration* decoration) :
        decoration{decoration}
    {
    }

    /// Overrides from NullSurfaceObserver
    /// @{
    void input_consumed(mir::scene::Surface const*, std::shared_ptr<MirEvent const> const& event) override
    {
        if (mir_event_get_type(event.get()) != mir_event_type_input)
            return;

        decoration->handle_input_event(event);
    }
    /// @}

    Decoration* const decoration;
};

struct msd::DecorationNotifier::WindowSurfaceObserver : public mir::scene::NullSurfaceObserver
{
public:
    WindowSurfaceObserver(Decoration* const& decoration) :
        decoration{decoration}
    {
    }

    /// Overrides from NullSurfaceObserver
    /// @{
    void attrib_changed(mir::scene::Surface const* window_surface, MirWindowAttrib attrib, int value) override
    {
        decoration->attrib_changed(window_surface, attrib, value);
    }

    void window_resized_to(mir::scene::Surface const* window_surface, geometry::Size const& window_size) override
    {
        decoration->window_resized_to(window_surface, window_size);
    }

    void renamed(mir::scene::Surface const* window_surface, std::string const& name) override
    {
        decoration->window_renamed(window_surface, name);
    }
    /// @}

    Decoration* const decoration;
};

msd::DecorationNotifier::DecorationNotifier(
    std::shared_ptr<mir::scene::Surface> decoration_surface,
    std::shared_ptr<mir::scene::Surface> window_surface,
    Decoration* const decoration) :
    decoration_surface{decoration_surface},
    window_surface{window_surface},
    input_observer{std::make_shared<InputObserver>(decoration)},
    window_observer{std::make_shared<WindowSurfaceObserver>(decoration)}
{
    decoration_surface->register_interest(input_observer);
    window_surface->register_interest(window_observer);
}

msd::DecorationNotifier::~DecorationNotifier()
{
    decoration_surface->unregister_interest(*input_observer);
    window_surface->unregister_interest(*window_observer);
}

