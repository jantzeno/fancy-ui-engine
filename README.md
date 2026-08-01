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
layouts scroll instead of omitting component groups. `--tab` accepts
`components`, `shell`, `settings`, `operations`, or `status`. The interactive
gallery exposes the state sheets as keyboard-navigable tabs. Activating
Application shell replaces the gallery chrome with a full-canvas preview in
the same native window.

For deterministic review artifacts, pass a PNG destination. The gallery
renders a few frames so the font and SVG atlas is installed before capture:

```sh
xmake run fancy_ui_component_gallery \
  --tab components \
  --theme dark --scale 1 \
  --screenshot /tmp/fancy-ui-components-dark.png
```

The forced hover, pressed, and focus examples are private to the gallery.
Public components always report actual Dear ImGui pointer and keyboard input.
The gallery's value controls retain their example state. Color swatches open a
transactional picker (Apply or Enter commits; Cancel or Escape rolls back),
with full Current/Original and compact layouts shown side by side. Tree rows
keep 32 px targets in a dense native hierarchy with connector lines, and own
expansion and Ctrl/Shift selection separately from inline color, action, and
visibility targets. Issue groups aggregate descendant visibility without
coupling it to issue selection or review navigation. The operation page covers
preview, running, paused, stopping, finalizing, completed, failed, and overflow
states with live disclosure and 160–240 px tray resizing. The status page
covers Canvas and 3D facts, long and narrow content, operation independence,
and the logarithmic 10–1600% Canvas zoom control.

The application-shell page renders the canonical nine-region shell, including
the production application menu and workspace-specific context toolbar. Its
application-bar controls independently collapse and restore Explorer,
operation details, and Inspector; the operation control and operation-strip
disclosure share one retained tray state. Explorer contains the shared
hierarchy sample, Workspace deliberately shows an empty-state surface, and
Inspector includes one live example from every component family allowed in
that region. Use the Workspace's `Back to component gallery` action to return
to the tab that opened the preview. An unclaimed Escape does the same; an open
menu, popup, text edit, or active control consumes the first Escape instead.

Shell screenshots use the canonical 1280 × 720 logical canvas, scaled by
`--scale`. Components screenshots remain 1280 × 1200, while Settings,
Operations, and Status use 1280 × 1440.

The Settings page opens one modeless 880 × 560 System Settings window with
General, Appearance, Machines, License, and Legal navigation. General,
Appearance, and Machines edit one staged transaction; theme selection previews
immediately and rolls back on discard. License actions are immediate, and
Legal remains read-only.

The workspace-level [SteppenFace UI reference](../docs/reference/fancy-ui/README.md)
defines design authority, mockup review, and native handoff. Generated HTML
captures are disposable workspace artifacts rather than library documentation.

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
