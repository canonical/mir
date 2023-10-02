
#include "mir/graphics/platform.h"

#include <wayland-client.h>

namespace mir::graphics::wayland
{

class WlDisplayProvider : public DisplayInterfaceProvider
{
public:
    WlDisplayProvider(EGLDisplay dpy);

    WlDisplayProvider(
        WlDisplayProvider const& from,
        struct wl_surface* surface,
        geometry::Size size);

    auto get_egl_display() const -> EGLDisplay;

    auto maybe_create_interface(DisplayInterfaceBase::Tag const& type_tag)
        -> std::shared_ptr<DisplayInterfaceBase> override;

    class Framebuffer : public GenericEGLDisplayProvider::EGLFramebuffer
    {
    public:
        /**
         * Handle for an EGL output surface
         *
         * \note    This takes ownership of \param ctx and \param surf; when the
         *          final handle generated from this Framebuffer is released,
         *          the EGL resources \param ctx and \param surff will be freed.
         */
        Framebuffer(EGLDisplay dpy, EGLContext ctx, EGLSurface surf, geometry::Size size);

        auto size() const -> geometry::Size override;

        void make_current() override;
        void release_current() override;
        auto clone_handle() -> std::unique_ptr<GenericEGLDisplayProvider::EGLFramebuffer> override;

        void swap_buffers();
    private:
        class EGLState;
        Framebuffer(std::shared_ptr<EGLState const> surf, geometry::Size size);

        std::shared_ptr<EGLState const> const state;
        geometry::Size const size_;
    };
private:
    class EGLDisplayProvider;

    std::shared_ptr<EGLDisplayProvider> const egl_provider;
};
}
