# fancy-ui-engine

`fancy_ui` is the Dear ImGui presentation library for this workspace. It owns
the semantic theme, shared controls, and the nine-region desktop application
shell used by the export application.

The public API is immediate-mode and value based:

```cpp
const fancy_ui::ButtonResult result = fancy_ui::Button({
    .id = "start-export",
    .label = "Start export",
    .variant = fancy_ui::ButtonVariant::Primary,
    .availability = export_availability,
});
if (result.activated) {
  commands.push(CommandType::StartExport);
}
```

Callers prepare labels, values, availability, and disabled reasons before
drawing. The library returns interaction results; it does not own product
stores, execute product commands, or contain export/nesting rules.

The library is intentionally workspace-coupled. Its xmake target can depend on
the independent `im2d_ui` adapter, but `im2d` must never depend on `fancy_ui`.

## Boundaries

- Raw Dear ImGui widget calls for the redesigned shell belong in
  `src/components/`.
- `src/shell/` composes layout from component and region contracts.
- Public headers expose only presentation values and callbacks.
- Product view-state builders and command handlers stay in the product
  repository.
- im2d canvas document algorithms stay in `im2d_canvas`; immediate-mode canvas
  drawing stays in `im2d_ui`.
