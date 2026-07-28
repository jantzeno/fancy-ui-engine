# fancy-ui-engine

`fancy_ui` is the Dear ImGui presentation library for this workspace. It owns
the semantic theme, shared controls, and the complete SteppenFace nine-region
desktop application UI.

The SteppenFace API is immediate-mode, typed, and value based:

```cpp
fancy_ui::steppenface::ApplicationView view = build_application_view();
fancy_ui::steppenface::SurfaceBindings surfaces = bind_product_surfaces();
const fancy_ui::steppenface::FrameResult result = ui.Draw(view, surfaces);
for (const fancy_ui::steppenface::UiIntent &intent : result.product_intents) {
  dispatch_after_revalidation(intent);
}
```

The product prepares labels, values, `CommandId` availability, disabled
reasons, and surface hosts before drawing. The library returns intents tagged
with the observed product revision; it does not own product stores, execute
product commands, or contain export/nesting rules.

The library is intentionally workspace-coupled. Its xmake target can depend on
the independent `im2d_ui` adapter, but `im2d` must never depend on `fancy_ui`.

## Component states

Shared components model availability, validation, selection, and mixed values
as separate inputs. Components resolve those inputs with the same precedence:
disabled, selected, validation, focus, pressed, hover, then rest. Disabled
controls use explicit palette roles instead of opacity so contrast remains
predictable in both themes.

The standalone gallery is the native review surface for the component-state
reference:

```sh
xmake build fancy_ui_component_gallery
xmake run fancy_ui_component_gallery --theme dark --scale 1
```

`--theme` accepts `light` or `dark`. `--scale` accepts values from `0.75` to
`2`, with `1`, `1.25`, `1.5`, and `2` as the standard review modes. Larger
layouts scroll instead of omitting component groups.

For deterministic review artifacts, pass a PNG destination. The gallery
renders a few frames so the font and SVG atlas is installed before capture:

```sh
xmake run fancy_ui_component_gallery \
  --theme dark --scale 1 \
  --screenshot /tmp/fancy-ui-components-dark.png
```

The forced hover, pressed, and focus examples are private to the gallery.
Public components always report actual Dear ImGui pointer and keyboard input.
The gallery's value controls retain their example state. Color swatches open a
transactional picker (Apply or Enter commits; Cancel or Escape rolls back),
tree rows own expansion and Ctrl/Shift selection separately from inline color,
action, and visibility targets, and issue groups aggregate descendant
visibility without coupling it to issue selection or review navigation.
The regenerated 1280×1024 references are stored in
[`docs/ui-mockups/260727/`](docs/ui-mockups/260727/).

## Boundaries

- Raw Dear ImGui widget calls belong in `src/components/`, `src/shell/`, and
  the private `src/steppenface/` application composer.
- `include/fancy_ui/steppenface/` exposes the product-neutral application
  contract.
- Public headers expose presentation values, typed intents, and surface
  callbacks without ImGui types.
- Product view-state builders and command handlers stay in the product
  repository.
- im2d canvas document algorithms stay in `im2d_canvas`; immediate-mode canvas
  drawing stays in `im2d_ui`.
