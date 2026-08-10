#include "fancy_ui/steppenface/application_ui.hpp"
#include "fancy_ui/theme.hpp"
#include "internal/application_chrome.hpp"
#include "internal/ui_asset_atlas.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

using namespace fancy_ui::steppenface;

namespace {

struct WorkspaceGeometrySnapshot {
  std::vector<std::pair<int, int>> selection;
  std::vector<std::pair<int, int>> focus;
  bool segments_aligned = false;
};

struct ToolbarSegmentGeometrySnapshot {
  std::vector<std::pair<int, int>> selection;
  std::vector<std::pair<int, int>> underline;
  std::vector<std::pair<int, int>> border;
};

WorkspaceGeometrySnapshot
WorkspaceSelectionGeometry(const WorkspaceKind workspace) {
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.DisplaySize = ImVec2(1280.0f, 720.0f);
  io.DeltaTime = 1.0f / 60.0f;
  ImFontConfig font_config;
  font_config.SizePixels = 16.0f;
  io.Fonts->AddFontDefault(&font_config);
  unsigned char *pixels = nullptr;
  int width = 0;
  int height = 0;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
  fancy_ui::ApplyTheme(fancy_ui::ResolvedTheme::Dark);

  ApplicationUi ui;
  ApplicationView view;
  view.application_bar.active_workspace = workspace;
  view.active_destination = workspace == WorkspaceKind::Model3d
                                ? Destination::Model
                                : Destination::CanvasObjects;
  const fancy_ui::ColorRgba selection =
      fancy_ui::PaletteFor(fancy_ui::ResolvedTheme::Dark).selection;
  const ImU32 selection_color = ImGui::ColorConvertFloat4ToU32(
      ImVec4(selection.red, selection.green, selection.blue, selection.alpha));
  const fancy_ui::ColorRgba focus =
      fancy_ui::PaletteFor(fancy_ui::ResolvedTheme::Dark).focus;
  const ImU32 focus_color = ImGui::ColorConvertFloat4ToU32(
      ImVec4(focus.red, focus.green, focus.blue, focus.alpha));
  const fancy_ui::ColorRgba rest =
      fancy_ui::PaletteFor(fancy_ui::ResolvedTheme::Dark).surface_raised;
  const ImU32 rest_color = ImGui::ColorConvertFloat4ToU32(
      ImVec4(rest.red, rest.green, rest.blue, rest.alpha));
  const auto collect_positions = [](const ImU32 color) {
    std::vector<ImVec2> positions;
    const ImDrawData *draw_data = ImGui::GetDrawData();
    for (int list_index = 0; list_index < draw_data->CmdListsCount;
         ++list_index) {
      const ImDrawList *draw_list = draw_data->CmdLists[list_index];
      for (const ImDrawVert &vertex : draw_list->VtxBuffer) {
        if (vertex.col == color && vertex.pos.x < 400.0f &&
            vertex.pos.y <= 40.0f) {
          positions.push_back(vertex.pos);
        }
      }
    }
    return positions;
  };

  for (int frame = 0; frame < 2; ++frame) {
    ImGui::NewFrame();
    (void)ui.Draw(view, {});
    ImGui::Render();
  }
  std::vector<ImVec2> positions = collect_positions(selection_color);
  const std::vector<ImVec2> rest_positions = collect_positions(rest_color);

  float minimum_x = std::numeric_limits<float>::max();
  float maximum_x = std::numeric_limits<float>::lowest();
  float minimum_y = std::numeric_limits<float>::max();
  float maximum_y = std::numeric_limits<float>::lowest();
  for (const ImVec2 position : positions) {
    minimum_x = std::min(minimum_x, position.x);
    maximum_x = std::max(maximum_x, position.x);
    minimum_y = std::min(minimum_y, position.y);
    maximum_y = std::max(maximum_y, position.y);
  }
  float rest_minimum_y = std::numeric_limits<float>::max();
  float rest_maximum_y = std::numeric_limits<float>::lowest();
  for (const ImVec2 position : rest_positions) {
    rest_minimum_y = std::min(rest_minimum_y, position.y);
    rest_maximum_y = std::max(rest_maximum_y, position.y);
  }

  io.AddMousePosEvent((minimum_x + maximum_x) * 0.5f,
                      (minimum_y + maximum_y) * 0.5f);
  io.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
  ImGui::NewFrame();
  (void)ui.Draw(view, {});
  ImGui::Render();
  io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
  ImGui::NewFrame();
  (void)ui.Draw(view, {});
  ImGui::Render();
  const auto normalize = [workspace, minimum_x, maximum_x, minimum_y](
                             const std::vector<ImVec2> &source_positions) {
    std::vector<std::pair<int, int>> normalized;
    normalized.reserve(source_positions.size());
    for (const ImVec2 position : source_positions) {
      const float local_x = workspace == WorkspaceKind::Model3d
                                ? position.x - minimum_x
                                : maximum_x - position.x;
      normalized.emplace_back(
          static_cast<int>(std::lround(local_x * 1000.0f)),
          static_cast<int>(std::lround((position.y - minimum_y) * 1000.0f)));
    }
    std::ranges::sort(normalized);
    return normalized;
  };

  WorkspaceGeometrySnapshot snapshot{
      .selection = normalize(positions),
      .focus = normalize(collect_positions(focus_color)),
      .segments_aligned = !rest_positions.empty() &&
                          rest_minimum_y == minimum_y &&
                          rest_maximum_y == maximum_y,
  };
  ImGui::DestroyContext();
  return snapshot;
}

ToolbarSegmentGeometrySnapshot
ToolbarSegmentGeometry(const std::size_t selected_index) {
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.DisplaySize = ImVec2(1280.0f, 720.0f);
  io.DeltaTime = 1.0f / 60.0f;
  ImFontConfig font_config;
  font_config.SizePixels = 16.0f;
  io.Fonts->AddFontDefault(&font_config);
  unsigned char *pixels = nullptr;
  int width = 0;
  int height = 0;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
  fancy_ui::ApplyTheme(fancy_ui::ResolvedTheme::Dark);

  const ToolbarSegmentedView segmented{
      .id = {.value = "test.segmented"},
      .choices =
          {
              {
                  .id = {.value = "test.segmented.left"},
                  .label = "Mode",
                  .selected = selected_index == 0,
              },
              {
                  .id = {.value = "test.segmented.right"},
                  .label = "Mode",
                  .selected = selected_index == 1,
              },
          },
  };
  fancy_ui::detail::UiAssetAtlas assets;
  fancy_ui::detail::ApplicationChrome chrome(assets);

  for (int frame = 0; frame < 2; ++frame) {
    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(320.0f, 120.0f), ImGuiCond_Always);
    ImGui::Begin("segmented-geometry", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoBackground);
    ImGui::SetCursorPos(ImVec2(40.0f, 44.0f));
    chrome.DrawToolbarSegmented(segmented, {}, {}, 160.0f);
    ImGui::End();
    ImGui::Render();
  }

  const fancy_ui::SemanticPalette palette =
      fancy_ui::PaletteFor(fancy_ui::ResolvedTheme::Dark);
  const auto color = [](const fancy_ui::ColorRgba value) {
    return ImGui::ColorConvertFloat4ToU32(
        ImVec4(value.red, value.green, value.blue, value.alpha));
  };
  const ImU32 selection_color = color(palette.selection);
  const ImU32 focus_color = color(palette.focus);
  const ImU32 border_color = color(palette.border_strong);
  const auto collect_positions = [](const ImU32 target, const float minimum_y) {
    std::vector<ImVec2> positions;
    const ImDrawData *draw_data = ImGui::GetDrawData();
    for (int list_index = 0; list_index < draw_data->CmdListsCount;
         ++list_index) {
      const ImDrawList *draw_list = draw_data->CmdLists[list_index];
      for (const ImDrawVert &vertex : draw_list->VtxBuffer) {
        if (vertex.col == target && vertex.pos.y >= minimum_y) {
          positions.push_back(vertex.pos);
        }
      }
    }
    return positions;
  };

  const std::vector<ImVec2> border = collect_positions(border_color, 40.0f);
  float group_minimum_x = std::numeric_limits<float>::max();
  float group_maximum_x = std::numeric_limits<float>::lowest();
  float group_minimum_y = std::numeric_limits<float>::max();
  for (const ImVec2 position : border) {
    group_minimum_x = std::min(group_minimum_x, position.x);
    group_maximum_x = std::max(group_maximum_x, position.x);
    group_minimum_y = std::min(group_minimum_y, position.y);
  }

  const auto normalize = [selected_index, group_minimum_x, group_maximum_x,
                          group_minimum_y](
                             const std::vector<ImVec2> &positions) {
    std::vector<std::pair<int, int>> normalized;
    normalized.reserve(positions.size());
    for (const ImVec2 position : positions) {
      const float local_x = selected_index == 0 ? position.x - group_minimum_x
                                                : group_maximum_x - position.x;
      normalized.emplace_back(static_cast<int>(std::lround(local_x * 1000.0f)),
                              static_cast<int>(std::lround(
                                  (position.y - group_minimum_y) * 1000.0f)));
    }
    std::ranges::sort(normalized);
    return normalized;
  };

  ToolbarSegmentGeometrySnapshot snapshot{
      .selection =
          normalize(collect_positions(selection_color, group_minimum_y)),
      .underline = normalize(collect_positions(focus_color, 71.0f)),
      .border = normalize(border),
  };
  ImGui::DestroyContext();
  return snapshot;
}

} // namespace

TEST_CASE("application views own adapter-provided labels and identifiers") {
  ApplicationView view;
  view.revision = 17;
  view.application_bar.dirty_label = "Unsaved";
  view.explorer.rows.push_back({
      .id = {.value = "model.part.4"},
      .label = "Bracket",
      .secondary_label = "Part 4",
      .depth = 1,
      .selected = true,
  });

  REQUIRE(view.revision == 17);
  REQUIRE(view.application_bar.dirty_label == "Unsaved");
  REQUIRE(view.application_bar.document_dirty);
  REQUIRE(view.explorer.rows.front().id.value == "model.part.4");
  REQUIRE(view.explorer.rows.front().selected);
}

TEST_CASE("menu command lookup traverses nested submenus") {
  const CommandView rotate{
      .id = {.value = "object.rotate-clockwise"},
      .command = CommandId::RotateCW,
      .label = "Rotate clockwise",
  };
  ApplicationBarView application_bar;
  application_bar.menus.push_back({
      .id = {.value = "menu.object"},
      .label = "Object",
      .items =
          {
              {
                  .id = {.value = "object.transform"},
                  .kind = MenuItemKind::Submenu,
                  .label = "Transform",
                  .children =
                      {
                          {
                              .id = rotate.id,
                              .kind = MenuItemKind::Command,
                              .label = rotate.label,
                              .command = rotate,
                          },
                      },
              },
          },
  });

  const CommandView *found =
      FindMenuCommand(application_bar, CommandId::RotateCW);

  REQUIRE(found != nullptr);
  REQUIRE(found->id.value == "object.rotate-clockwise");
  REQUIRE(FindMenuCommand(application_bar, CommandId::Quit) == nullptr);
}

TEST_CASE("disabled commands carry precise missing backend contracts") {
  const CommandView command{
      .id = {.value = "file.export"},
      .command = CommandId::ExportFile,
      .label = "Export...",
      .availability =
          {
              .enabled = false,
              .disabled_reason = "Export jobs are not implemented",
              .missing_capability = BackendCapability::ExportJob,
          },
  };

  REQUIRE_FALSE(command.availability.enabled);
  REQUIRE(command.availability.missing_capability ==
          BackendCapability::ExportJob);
  REQUIRE_FALSE(command.availability.disabled_reason.empty());
}

TEST_CASE("product intents retain the source view revision") {
  UiIntent intent = InvokeCommand{
      .revision = 29,
      .command = CommandId::ZoomToFit,
  };

  REQUIRE(std::holds_alternative<InvokeCommand>(intent));
  REQUIRE(std::get<InvokeCommand>(intent).revision == 29);
}

TEST_CASE("toolbar contracts preserve typed order and edit targets") {
  ContextToolbarView toolbar;
  toolbar.items.emplace_back(ToolbarSegmentedView{
      .id = {.value = "model.selection-tool"},
      .choices =
          {
              {
                  .id = {.value = "model.selection.pointer"},
                  .label = "Pointer",
                  .selected = true,
                  .action =
                      {
                          .field = {.value = "model.selection-tool"},
                          .value = SelectionTool::Pointer,
                      },
              },
          },
  });
  toolbar.items.emplace_back(
      ToolbarSeparatorView{.id = {.value = "selection.separator"}});
  toolbar.items.emplace_back(ToolbarPopoverView{
      .id = {.value = "model.grid"},
      .label = "Grid: Mixed",
      .items =
          {
              ToolbarMenuItemView{
                  .id = {.value = "model.grid.25"},
                  .label = "25 mm",
                  .action =
                      {
                          .field = {.value = "model.grid-spacing"},
                          .value = std::int64_t{25},
                          .target = UiId{.value = "bed.2"},
                      },
              },
          },
  });

  REQUIRE(toolbar.items.size() == 3);
  REQUIRE(std::holds_alternative<ToolbarSegmentedView>(toolbar.items[0]));
  REQUIRE(std::holds_alternative<ToolbarSeparatorView>(toolbar.items[1]));
  const ToolbarPopoverView &grid =
      std::get<ToolbarPopoverView>(toolbar.items[2]);
  const ToolbarMenuItemView &spacing =
      std::get<ToolbarMenuItemView>(grid.items.front());
  REQUIRE(spacing.action.target->value == "bed.2");
  REQUIRE(std::get<std::int64_t>(spacing.action.value) == 25);
}

TEST_CASE("command lookup traverses toolbar popovers after application menus") {
  ApplicationView view;
  const CommandView menu_internal{
      .id = {.value = "edit.select-internal"},
      .command = CommandId::SelectInternalFaces,
      .label = "Select internal faces",
  };
  view.application_bar.menus.push_back({
      .id = {.value = "menu.edit"},
      .label = "Edit",
      .items =
          {
              {
                  .id = menu_internal.id,
                  .kind = MenuItemKind::Command,
                  .label = menu_internal.label,
                  .command = menu_internal,
              },
          },
  });
  view.context_toolbar.items.emplace_back(ToolbarPopoverView{
      .id = {.value = "model.select-faces"},
      .label = "Select faces",
      .items =
          {
              CommandView{
                  .id = {.value = "model.select-faces.external"},
                  .command = CommandId::SelectExternalFaces,
                  .label = "External Faces",
              },
              CommandView{
                  .id = {.value = "model.select-faces.internal"},
                  .command = CommandId::SelectInternalFaces,
                  .label = "Internal Faces",
                  .availability =
                      {
                          .enabled = false,
                          .disabled_reason = "No model is loaded.",
                      },
              },
          },
  });

  const CommandView *external =
      FindCommand(view, CommandId::SelectExternalFaces);
  const CommandView *menu_match =
      FindCommand(view, CommandId::SelectInternalFaces);
  REQUIRE(external != nullptr);
  REQUIRE(external->id.value == "model.select-faces.external");
  REQUIRE(external->availability.enabled);
  REQUIRE(menu_match != nullptr);
  REQUIRE(menu_match->id.value == "edit.select-internal");
  REQUIRE(menu_match->availability.enabled);

  view.application_bar.menus.clear();
  const CommandView *popover_match =
      FindCommand(view, CommandId::SelectInternalFaces);
  REQUIRE(popover_match != nullptr);
  REQUIRE_FALSE(popover_match->availability.enabled);
  REQUIRE(popover_match->id.value == "model.select-faces.internal");
  REQUIRE(popover_match->availability.disabled_reason == "No model is loaded.");
}

TEST_CASE(
    "toolbar popover command activation emits a revision-bearing intent") {
  const auto activate_command = [](const CommandId expected) {
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1280.0f, 720.0f);
    io.DeltaTime = 1.0f / 60.0f;
    ImFontConfig font_config;
    font_config.SizePixels = 16.0f;
    io.Fonts->AddFontDefault(&font_config);
    unsigned char *pixels = nullptr;
    int width = 0;
    int height = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    ApplicationUi ui;
    ApplicationView view;
    view.revision = 47;
    view.application_bar.document_dirty = false;
    view.context_toolbar.items.emplace_back(ToolbarPopoverView{
        .id = {.value = "model.select-faces"},
        .label = "Select faces",
        .items =
            {
                CommandView{
                    .id = {.value = "model.select-faces.choice"},
                    .command = expected,
                    .label = "Face choice",
                },
            },
    });
    const auto draw = [&]() {
      ImGui::NewFrame();
      FrameResult result = ui.Draw(view, {});
      ImGui::Render();
      return result;
    };
    const auto click = [&](const ImVec2 position) {
      io.AddMousePosEvent(position.x, position.y);
      io.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
      static_cast<void>(draw());
      io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
      return draw();
    };

    static_cast<void>(draw());
    static_cast<void>(draw());
    const FrameResult opened = click(ImVec2(72.0f, 64.0f));
    REQUIRE(opened.product_intents.empty());
    REQUIRE(draw().product_intents.empty());
    ImGuiWindow *popup = nullptr;
    for (ImGuiWindow *window : ImGui::GetCurrentContext()->Windows) {
      if (std::string_view{window->Name}.starts_with("##Popup_")) {
        popup = window;
      }
    }
    REQUIRE(popup != nullptr);
    const FrameResult activated =
        click(ImVec2(popup->Pos.x + popup->Size.x * 0.5f,
                     popup->Pos.y + popup->Size.y * 0.5f));
    REQUIRE(activated.product_intents.size() == 1);
    REQUIRE(std::holds_alternative<InvokeCommand>(
        activated.product_intents.front()));
    const InvokeCommand invoked =
        std::get<InvokeCommand>(activated.product_intents.front());
    ImGui::DestroyContext();
    return invoked;
  };

  const InvokeCommand external =
      activate_command(CommandId::SelectExternalFaces);
  REQUIRE(external.revision == 47);
  REQUIRE(external.command == CommandId::SelectExternalFaces);
  const InvokeCommand internal =
      activate_command(CommandId::SelectInternalFaces);
  REQUIRE(internal.revision == 47);
  REQUIRE(internal.command == CommandId::SelectInternalFaces);
}

TEST_CASE("disabled toolbar popovers stay closed and emit no intent") {
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.DisplaySize = ImVec2(1280.0f, 720.0f);
  io.DeltaTime = 1.0f / 60.0f;
  ImFontConfig font_config;
  font_config.SizePixels = 16.0f;
  io.Fonts->AddFontDefault(&font_config);
  unsigned char *pixels = nullptr;
  int width = 0;
  int height = 0;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

  ApplicationUi ui;
  ApplicationView view;
  view.revision = 48;
  view.application_bar.document_dirty = false;
  view.context_toolbar.items.emplace_back(ToolbarPopoverView{
      .id = {.value = "model.select-faces"},
      .label = "Select faces",
      .availability =
          {
              .enabled = false,
              .disabled_reason = "No model is loaded.",
          },
      .items =
          {
              CommandView{
                  .id = {.value = "model.select-faces.external"},
                  .command = CommandId::SelectExternalFaces,
                  .label = "External Faces",
              },
          },
  });
  const auto draw = [&]() {
    ImGui::NewFrame();
    FrameResult result = ui.Draw(view, {});
    ImGui::Render();
    return result;
  };
  const auto click = [&](const ImVec2 position) {
    io.AddMousePosEvent(position.x, position.y);
    io.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
    static_cast<void>(draw());
    io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
    return draw();
  };

  static_cast<void>(draw());
  static_cast<void>(draw());
  const FrameResult clicked = click(ImVec2(72.0f, 64.0f));
  const FrameResult settled = draw();
  const bool popup_open = !GImGui->OpenPopupStack.empty();
  ImGui::DestroyContext();

  REQUIRE(clicked.product_intents.empty());
  REQUIRE(settled.product_intents.empty());
  REQUIRE_FALSE(popup_open);
}

TEST_CASE("field edits retain product target and typed mode values") {
  UiIntent intent = EditField{
      .revision = 31,
      .field = {.value = "model.grid-spacing"},
      .value = std::int64_t{10},
      .target = UiId{.value = "all"},
  };

  const EditField &edit = std::get<EditField>(intent);
  REQUIRE(edit.target->value == "all");
  REQUIRE(std::get<std::int64_t>(edit.value) == 10);
}

TEST_CASE("session panels start independently visible") {
  const SessionState session;

  REQUIRE(session.explorer_visible);
  REQUIRE(session.inspector_visible);
  REQUIRE(session.active_destination == Destination::Model);
}

TEST_CASE("workspace switching restores each workspace's last destination") {
  SessionState session;

  session.ActivateDestination(Destination::ModelBeds);
  session.ActivateDestination(Destination::CanvasGrain);
  session.ActivateWorkspace(WorkspaceKind::Model3d);
  REQUIRE(session.active_destination == Destination::ModelBeds);

  session.ActivateWorkspace(WorkspaceKind::Canvas);
  REQUIRE(session.active_destination == Destination::CanvasGrain);
}

TEST_CASE(
    "explorer filtering retains ancestors and reveals matching children") {
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.DisplaySize = ImVec2(1280.0f, 720.0f);
  io.DeltaTime = 1.0f / 60.0f;
  ImFontConfig font_config;
  font_config.SizePixels = 16.0f;
  io.Fonts->AddFontDefault(&font_config);
  unsigned char *pixels = nullptr;
  int width = 0;
  int height = 0;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

  ApplicationUi ui;
  SessionState session;
  session.explorer_queries[Destination::Model] = "needle";
  session.explorer_expanded_rows["assembly"] = false;
  ui.SetSession(std::move(session));

  ApplicationView view;
  view.active_destination = Destination::Model;
  view.explorer.rows = {
      {
          .id = {.value = "assembly"},
          .label = "Assembly",
          .expandable = true,
      },
      {
          .id = {.value = "matching-child"},
          .label = "Needle bracket",
          .depth = 1,
      },
  };

  ImGui::NewFrame();
  static_cast<void>(ui.Draw(view, {}));
  ImGui::Render();

  REQUIRE_FALSE(ui.session().explorer_expanded_rows.at("assembly"));
  REQUIRE(ui.session().explorer_expanded_rows.contains("matching-child"));
  ImGui::DestroyContext();
}

TEST_CASE("application bar reserves forty logical pixels") {
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.DisplaySize = ImVec2(1280.0f, 720.0f);
  io.DeltaTime = 1.0f / 60.0f;
  ImFontConfig font_config;
  font_config.SizePixels = 16.0f;
  io.Fonts->AddFontDefault(&font_config);
  unsigned char *pixels = nullptr;
  int width = 0;
  int height = 0;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

  ApplicationUi ui;
  const ApplicationView view;
  for (int frame = 0; frame < 2; ++frame) {
    ImGui::NewFrame();
    (void)ui.Draw(view, {});
    ImGui::EndFrame();
  }

  REQUIRE(ImGui::GetMainViewport()->WorkPos.y == Catch::Approx(40.0f));
  ImGui::DestroyContext();
}

TEST_CASE(
    "application bar controls share one inset and toggle retained regions") {
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.DisplaySize = ImVec2(1280.0f, 720.0f);
  io.DeltaTime = 1.0f / 60.0f;
  io.Fonts->AddFontDefault();
  unsigned char *pixels = nullptr;
  int width = 0;
  int height = 0;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

  ApplicationUi ui;
  ApplicationView view;
  view.operation = OperationView{
      .id = {.value = "operation.search"},
      .title = "Search running",
      .summary = "Iteration 24",
  };
  const auto draw = [&]() {
    ImGui::NewFrame();
    const FrameResult result = ui.Draw(view, {});
    ImGui::Render();
    return result;
  };
  const auto activate = [&](const float x, const float y) {
    io.AddMousePosEvent(x, y);
    io.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
    static_cast<void>(draw());
    io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
    return draw();
  };

  static_cast<void>(draw());
  static_cast<void>(draw());

  for (const float x : {1188.0f, 1220.0f, 1252.0f}) {
    REQUIRE_FALSE(activate(x, 2.0f).layout_changed);
  }

  REQUIRE(activate(1188.0f, 34.0f).layout_changed);
  REQUIRE_FALSE(ui.session().explorer_visible);
  REQUIRE(activate(1220.0f, 34.0f).layout_changed);
  REQUIRE(ui.session().operation_tray_visible);
  REQUIRE(activate(1252.0f, 34.0f).layout_changed);
  REQUIRE_FALSE(ui.session().inspector_visible);

  view.operation.reset();
  const FrameResult disabled_operation = activate(1220.0f, 34.0f);
  REQUIRE_FALSE(disabled_operation.layout_changed);
  REQUIRE(ui.session().operation_tray_visible);

  ImGui::DestroyContext();
}

TEST_CASE("application menus use compact dark popups and rounded triggers") {
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.DisplaySize = ImVec2(1280.0f, 720.0f);
  io.DeltaTime = 1.0f / 60.0f;
  ImFontConfig font_config;
  font_config.SizePixels = 16.0f;
  io.Fonts->AddFontDefault(&font_config);
  unsigned char *pixels = nullptr;
  int width = 0;
  int height = 0;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

  const CommandView import_files{
      .id = {.value = "file.import-files"},
      .command = CommandId::ImportFiles,
      .label = "Import Files...",
  };
  const CommandView quit{
      .id = {.value = "file.quit"},
      .command = CommandId::Quit,
      .label = "Quit",
  };
  ApplicationView view;
  view.application_bar.menus.push_back({
      .id = {.value = "menu.file"},
      .label = "File",
      .items =
          {
              {
                  .id = import_files.id,
                  .kind = MenuItemKind::Command,
                  .label = import_files.label,
                  .command = import_files,
              },
              {
                  .id = quit.id,
                  .kind = MenuItemKind::Command,
                  .label = quit.label,
                  .command = quit,
              },
          },
  });

  ApplicationUi ui;
  const fancy_ui::SemanticPalette palette =
      fancy_ui::PaletteFor(fancy_ui::ResolvedTheme::Dark);
  const ImU32 hover_color = ImGui::ColorConvertFloat4ToU32(
      ImVec4(palette.control_hover.red, palette.control_hover.green,
             palette.control_hover.blue, palette.control_hover.alpha));
  io.AddMousePosEvent(30.0f, 20.0f);
  ImGui::NewFrame();
  (void)ui.Draw(view, {});
  ImGui::Render();
  ImGui::NewFrame();
  (void)ui.Draw(view, {});
  ImGui::Render();

  std::size_t hover_vertex_count = 0;
  float hover_minimum_y = std::numeric_limits<float>::max();
  float hover_maximum_y = std::numeric_limits<float>::lowest();
  for (int list_index = 0; list_index < ImGui::GetDrawData()->CmdListsCount;
       ++list_index) {
    const ImDrawList *draw_list = ImGui::GetDrawData()->CmdLists[list_index];
    for (const ImDrawVert &vertex : draw_list->VtxBuffer) {
      if ((vertex.col & ~IM_COL32_A_MASK) == (hover_color & ~IM_COL32_A_MASK) &&
          vertex.pos.y <= 40.0f) {
        ++hover_vertex_count;
        hover_minimum_y = std::min(hover_minimum_y, vertex.pos.y);
        hover_maximum_y = std::max(hover_maximum_y, vertex.pos.y);
      }
    }
  }
  REQUIRE(hover_vertex_count > 4);
  REQUIRE(hover_minimum_y == Catch::Approx(4.0f).margin(0.51f));
  REQUIRE(hover_maximum_y == Catch::Approx(36.0f).margin(0.51f));

  io.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
  ImGui::NewFrame();
  (void)ui.Draw(view, {});
  ImGui::Render();
  io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
  ImGui::NewFrame();
  (void)ui.Draw(view, {});
  ImGui::Render();

  ImGuiWindow *popup = ImGui::FindWindowByName("File###Menu_00");
  REQUIRE(popup != nullptr);
  REQUIRE(popup->WasActive);
  REQUIRE(popup->WindowPadding.x == Catch::Approx(10.0f));
  REQUIRE(popup->WindowPadding.y == Catch::Approx(8.0f));
  REQUIRE(popup->ContentSize.y == Catch::Approx(44.0f));

  const ImU32 popup_color = ImGui::ColorConvertFloat4ToU32(ImVec4(
      palette.application_surface.red, palette.application_surface.green,
      palette.application_surface.blue, palette.application_surface.alpha));
  REQUIRE(std::ranges::any_of(popup->DrawList->VtxBuffer,
                              [popup_color](const ImDrawVert &vertex) {
                                return vertex.col == popup_color;
                              }));
  ImGui::DestroyContext();
}

TEST_CASE("workspace switcher selected segments have mirrored geometry") {
  const auto model_geometry =
      WorkspaceSelectionGeometry(WorkspaceKind::Model3d);
  const auto canvas_geometry =
      WorkspaceSelectionGeometry(WorkspaceKind::Canvas);

  REQUIRE_FALSE(model_geometry.selection.empty());
  REQUIRE(model_geometry.selection == canvas_geometry.selection);
  REQUIRE_FALSE(model_geometry.focus.empty());
  REQUIRE(model_geometry.focus == canvas_geometry.focus);
  REQUIRE(model_geometry.segments_aligned);
  REQUIRE(canvas_geometry.segments_aligned);
}

TEST_CASE("toolbar segmented controls share connected mirrored geometry") {
  const ToolbarSegmentGeometrySnapshot left = ToolbarSegmentGeometry(0);
  const ToolbarSegmentGeometrySnapshot right = ToolbarSegmentGeometry(1);

  CAPTURE(left.selection.size(), left.underline.size(), left.border.size());
  REQUIRE_FALSE(left.selection.empty());
  REQUIRE(left.selection.size() == right.selection.size());
  for (std::size_t index = 0; index < left.selection.size(); ++index) {
    CAPTURE(index, left.selection[index].first, left.selection[index].second,
            right.selection[index].first, right.selection[index].second);
    REQUIRE(left.selection[index] == right.selection[index]);
  }
  REQUIRE_FALSE(left.underline.empty());
  REQUIRE(left.underline == right.underline);
  REQUIRE_FALSE(left.border.empty());
  REQUIRE(left.border == right.border);
}

TEST_CASE("workspace scope and tool segments share styling and hit targets") {
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.DisplaySize = ImVec2(640.0f, 240.0f);
  io.DeltaTime = 1.0f / 60.0f;
  io.Fonts->AddFontDefault();
  unsigned char *pixels = nullptr;
  int width = 0;
  int height = 0;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
  fancy_ui::ApplyTheme(fancy_ui::ResolvedTheme::Dark);

  fancy_ui::detail::UiAssetAtlas assets;
  fancy_ui::detail::ApplicationChrome chrome(assets);
  const ToolbarSegmentedView segmented{
      .id = {.value = "test.segment-style"},
      .choices =
          {
              {.id = {.value = "test.segment.rest"}, .label = "A"},
              {.id = {.value = "test.segment.selected"},
               .label = "B",
               .selected = true},
              {.id = {.value = "test.segment.hover"},
               .label = "C",
               .selected = true},
              {.id = {.value = "test.segment.pressed"},
               .label = "D",
               .selected = true},
              {.id = {.value = "test.segment.disabled"},
               .label = "E",
               .action = {.availability =
                              {
                                  .enabled = false,
                                  .disabled_reason = "Unavailable",
                              }}},
          },
  };
  constexpr std::array previews{
      fancy_ui::detail::InteractionPreview::Rest,
      fancy_ui::detail::InteractionPreview::Rest,
      fancy_ui::detail::InteractionPreview::Hovered,
      fancy_ui::detail::InteractionPreview::Pressed,
      fancy_ui::detail::InteractionPreview::Rest,
  };

  ImGui::NewFrame();
  ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
  ImGui::SetNextWindowSize(ImVec2(520.0f, 64.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::Begin("segment-style", nullptr,
               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoSavedSettings);
  ImGui::PopStyleVar();
  chrome.DrawToolbarSegmented(segmented, {}, previews, 500.0f);
  const float segment_width =
      ImGui::GetItemRectMax().x - ImGui::GetItemRectMin().x;
  const ImDrawList *style_draw_list = ImGui::GetWindowDrawList();
  ImGui::End();
  ImGui::Render();

  const fancy_ui::SemanticPalette palette =
      fancy_ui::PaletteFor(fancy_ui::ResolvedTheme::Dark);
  const auto color = [](const fancy_ui::ColorRgba value) {
    return ImGui::ColorConvertFloat4ToU32(
        ImVec4(value.red, value.green, value.blue, value.alpha));
  };
  const auto has_color = [style_draw_list](const ImU32 expected) {
    return std::ranges::any_of(style_draw_list->VtxBuffer,
                               [expected](const ImDrawVert &vertex) {
                                 return vertex.col == expected;
                               });
  };

  REQUIRE(segment_width == Catch::Approx(100.0f));
  REQUIRE(has_color(color(palette.surface_raised)));
  REQUIRE(has_color(color(palette.selection)));
  REQUIRE(has_color(color(palette.control_hover)));
  REQUIRE(has_color(color(palette.control_pressed)));
  REQUIRE(has_color(color(palette.control_disabled_fill)));
  REQUIRE(has_color(color(palette.border_strong)));
  REQUIRE(has_color(color(palette.border)));
  REQUIRE(has_color(color(palette.text_primary)));
  REQUIRE(has_color(color(palette.text_disabled)));
  REQUIRE_FALSE(has_color(color(palette.text_secondary)));
  REQUIRE(std::ranges::any_of(
      style_draw_list->VtxBuffer,
      [focus = color(palette.focus)](const ImDrawVert &vertex) {
        return vertex.col == focus && vertex.pos.y < 8.0f;
      }));

  std::optional<WorkspaceKind> activated_workspace;
  const fancy_ui::detail::ApplicationChromeCallbacks callbacks{
      .activate_workspace =
          [&activated_workspace](const WorkspaceKind workspace) {
            activated_workspace = workspace;
          },
  };
  float workspace_segment_width = 0.0f;
  const auto draw_workspace = [&] {
    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(300.0f, 64.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("workspace-hit-target", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings);
    ImGui::PopStyleVar();
    chrome.DrawWorkspaceSwitcher({}, callbacks, {}, 138.0f);
    workspace_segment_width =
        ImGui::GetItemRectMax().x - ImGui::GetItemRectMin().x;
    ImGui::End();
    ImGui::Render();
  };

  io.AddMousePosEvent(270.0f, 16.0f);
  draw_workspace();
  io.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
  draw_workspace();
  io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
  draw_workspace();

  REQUIRE(workspace_segment_width == Catch::Approx(138.0f));
  REQUIRE(activated_workspace == WorkspaceKind::Canvas);
  ImGui::DestroyContext();
}

static_assert(!std::is_convertible_v<fancy_ui::TextureHandle, bool>);
