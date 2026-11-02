// Small FPS / frame-time readout in the top-right corner (F1 menu: Performance overlay).
// Frames are counted at the game's own present call (sub_82E6FFE0, which calls VdSwap),
// so it shows the game's frame rate, not the window's refresh rate.
#pragma once

#include <rex/ui/imgui_dialog.h>

namespace recomp_ui {

// Counts one game frame (called from the game's present hook).
void perf_on_frame();

class PerfOverlay : public rex::ui::ImGuiDialog {
 public:
  explicit PerfOverlay(rex::ui::ImGuiDrawer* drawer) : ImGuiDialog(drawer) {}

 protected:
  void OnDraw(ImGuiIO& io) override;
};

}  // namespace recomp_ui
