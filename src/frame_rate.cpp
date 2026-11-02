// See frame_rate.h.
#include "frame_rate.h"

#include <string>

#include <rex/cvar.h>
#include <rex/logging.h>

REXCVAR_DEFINE_INT32(recomp_frame_rate, 30, "Recomp",
                     "Frame rate: 30 (original), 60, 120, or 0 for uncapped. Applies at start-up");

namespace frame_rate {

void apply() {
  int fps = REXCVAR_GET(recomp_frame_rate);
  if (fps != 0 && fps != 30 && fps != 60 && fps != 120) fps = 30;
  if (fps == 0) {
    rex::cvar::SetFlagByName("vsync", "false");
    rex::cvar::SetFlagByName("video_mode_refresh_rate", "60");
    REXLOG_INFO("Frame rate: uncapped (guest vblank pacing off)");
  } else {
    rex::cvar::SetFlagByName("vsync", "true");
    rex::cvar::SetFlagByName("video_mode_refresh_rate", std::to_string(fps * 2));
    REXLOG_INFO("Frame rate: {} FPS (guest refresh {} Hz)", fps, fps * 2);
  }
}

}  // namespace frame_rate
