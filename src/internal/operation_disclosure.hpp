#pragma once

namespace fancy_ui::detail {

class UiAssetAtlas;

[[nodiscard]] bool DrawOperationDisclosure(UiAssetAtlas &assets, bool expanded,
                                           bool available = true);

} // namespace fancy_ui::detail
