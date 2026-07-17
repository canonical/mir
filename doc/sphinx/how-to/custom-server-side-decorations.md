---
myst:
  html_meta:
    description: Customize MirAL server-side decoration appearance with DecorationStrategy and CustomDecorations.
---

(custom-server-side-decorations)=

# Custom server-side decorations

MirAL provides a limited, buffer-based API for customizing server-side decoration (SSD)
appearance. This is an in-process API: compositors subclass {class}`miral::DecorationStrategy`
and pass it to {class}`miral::CustomDecorations`.

For selecting SSD vs client-side decorations (CSD), see {ref}`specifying-csd-ssd-preference`.

## Naming: three different "decoration strategy" types

Mir uses three similarly named types for distinct concerns:

| Type                                                    | Role                                                                    |
| ------------------------------------------------------- | ----------------------------------------------------------------------- |
| {class}`miral::Decorations` / `mir::DecorationStrategy` | SSD vs CSD **negotiation** with clients                                 |
| {class}`miral::CustomDecorations`                       | Installs a custom SSD **renderer** (`the_decoration_renderer_strategy`) |
| {class}`miral::DecorationStrategy`                      | Public API your compositor implements to paint SSD buffers              |

You typically need both a {class}`miral::Decorations` policy **and**
{class}`miral::CustomDecorations` when replacing how server-side decorations are drawn.
`Decorations::prefer_ssd()` (or `always_ssd()`) ensures clients negotiate SSD; the custom
renderer then paints those buffers. The same applies with `prefer_csd()` or `always_csd()`
when you want clients to choose CSD by default but still supply custom SSD rendering for
windows that end up server-decorated.

## Quick start

```cpp
#include <miral/custom_decorations.h>
#include <miral/decorations.h>

class MyDecoration : public miral::DecorationStrategy { /* ... */ };

runner.run_with({
    miral::Decorations::prefer_ssd(),
    miral::CustomDecorations{std::make_shared<MyDecoration>()},
    // ...
});
```

See `examples/miral-custom-decorations/miral-custom-decorations.cpp` for a complete example
with visibly different green chrome and the default Mir button/title layout
(screenshot-worthy when validating the integration).

## API overview

Include {file}`miral/custom_decorations.h` for the full custom-decoration API:
{class}`miral::CustomDecorations`, {class}`miral::DecorationStrategy`,
{class}`miral::DecorationBuffers`, and {class}`miral::DecorationSurface`.

- {class}`miral::DecorationStrategy` — geometry, button placement, and rendering hooks.
- {class}`miral::DecorationBuffers` — provides `create_mapping()` during render calls.
  Compositor code receives a `DecorationBuffers&` in each `render_*` method.
- {class}`miral::DecorationSurface` — move-only render output returned from `render_*`
  methods. Paint via `DecorationBuffers::Mapping`, then return `std::move(pixels.surface)`.
  Public headers do not expose `mir::graphics::Buffer`.

Implementations must be **stateless**: every render method receives the current
{class}`miral::DecorationStrategy::WindowState` and
{class}`miral::DecorationStrategy::InputState`. Do not cache per-window state between calls.

The custom renderer strategy is fixed when the {class}`miral::MirRunner` is constructed;
it cannot be changed at runtime.

### WindowState and InputState

- `WindowState::window_id()` is an opaque per-window token stable for the lifetime of
  that decoration. Compositors may use it to key per-window render caches. It is not a
  {class}`miral::Window` handle and is not preserved if the underlying surface is destroyed
  and recreated.
- `InputState` exposes only the standard close/maximize/minimize buttons. Internal
  non-button hit regions (`input_shape` in the shell) are not available in v1.

## Buffer dimensions and HiDPI

Buffer pixel dimensions must match **logical decoration size × `WindowState::scale()`**.
Mir scales buffers back to logical coordinates when compositing. The built-in default
renderer follows the same rule.

Typical render flow:

```cpp
auto pixels = buffers.create_mapping(scaled_size, buffer_format());
auto* px = pixels.mapping.pixels32();
// ... paint ...
return std::move(pixels.surface);
```

Use {func}`miral::pack_color` when writing `argb_8888` pixels via
{class}`miral::DecorationBuffers::Mapping::pixels32`.

## Geometry

- `compute_size_with_decorations()` — client-visible size (what applications observe).
- `titlebar_height()`, `side_border_width()`, `bottom_border_height()` — internal
  geometry for rendering and input (defaults 24/4/4 pixels).

These can differ; both must be implemented consistently.

### Button placement

`button_placement(n, ws)` must return the hit-target rectangle for button index `n` in
**logical** `titlebar_height()` coordinates (before `WindowState::scale()` is applied to
the buffer). The shell calls this for `n = 0, 1, 2` mapping to close, maximize, and
minimize.

The built-in default strategy places `n = 0` at the **right** edge of the title bar (close
is rightmost). A typical right-aligned layout:

```cpp
auto button_placement(unsigned n, WindowState const& ws) const -> mir::geometry::Rectangle
{
    int const button_width = 26;
    int const button_height = 16;
    int const spacing = 4;
    int const margin = 4;
    int const side = ws.side_border_width().as_int();
    int const titlebar_w = ws.titlebar_width().as_int();

    int const x = titlebar_w - side - margin - (n + 1) * button_width - n * spacing;
    int const y = (ws.titlebar_height().as_int() - button_height) / 2;
    return {{x, y}, {button_width, button_height}};
}
```

For **left-aligned** buttons (as in some custom themes), anchor from the left instead:

```cpp
int const x = margin + n * (button_width + spacing);
```

Keep `button_placement()` consistent with wherever you paint button highlights and glyphs
in `render_titlebar()`; `InputState::buttons()[n].rect` uses these rectangles.

### Title text position

Window titles are not positioned automatically. Call
{func}`miral::DecorationStrategy::render_title_text` from `render_titlebar()` with a
**buffer-pixel** origin (logical offset × `WindowState::scale()`), the scaled font height,
and an ARGB color.

The built-in default uses logical `(8, 2)` from the title bar top-left. The example follows
that pattern:

```cpp
auto const text_h = scaled_pixels(14, ws.scale());
auto title_pos = mir::geometry::Point{
    mir::geometry::X{scaled_pixels(8, ws.scale())},
    mir::geometry::Y{scaled_pixels(2, ws.scale())}};
render_title_text(px, size, ws.window_name(), title_pos, mir::geometry::Height{text_h}, color);
```

To center the title vertically, use `(h - text_h) / 2` for the Y coordinate. To leave room
for left-aligned buttons, increase the X offset (for example `scaled_pixels(80, scale)`).

## Limitations (v1)

- Software buffers only (no GPU-accelerated or toolkit rendering).
- Closed button set: close, maximize, minimize. Custom chrome (e.g. always-on-top) is
  not supported.
- No drop shadows, animations, or extensible button types in this API.
- A future Wayland-client-based decoration model may supersede this for richer themes.

## Further reading

- {ref}`ref-introducing-the-miral-api` — MirAL overview section on custom SSD.
