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
