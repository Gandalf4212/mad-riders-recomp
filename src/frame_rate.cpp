// See frame_rate.h.
#include "frame_rate.h"

#include <string>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <timeapi.h>
#endif

#include <rex/cvar.h>
#include <rex/logging.h>

REXCVAR_DEFINE_INT32(recomp_frame_rate, 30, "Recomp",
                     "Frame rate: 30 (original), 60 or 120. Applies at start-up");

namespace frame_rate {

void raise_timer_resolution() {
#ifdef _WIN32
  // Windows' default timer tick is 15.6 ms, so ReXGlue's 1 ms sleeps (the guest
  // vblank thread, kernel waits) really last ~15.6 ms and frames arrive late:
  // 30 FPS became ~36 ms frames. Xenia raises the resolution the same way.
  static bool done = false;
  if (!done) {
    done = true;
    MMRESULT r = timeBeginPeriod(1);
    REXLOG_INFO("Timer resolution 1 ms: {}", r == TIMERR_NOERROR ? "ok" : "failed");
  }
#endif
}

void apply() {
  raise_timer_resolution();
  int fps = REXCVAR_GET(recomp_frame_rate);
  // 0 was "uncapped" in earlier builds: the game caps itself at 120 anyway, so it becomes 120.
  if (fps == 0) fps = 120;
  if (fps != 30 && fps != 60 && fps != 120) fps = 30;
  if (fps != REXCVAR_GET(recomp_frame_rate)) rex::cvar::SetFlagByName("recomp_frame_rate", std::to_string(fps));
  rex::cvar::SetFlagByName("vsync", "true");
  rex::cvar::SetFlagByName("video_mode_refresh_rate", std::to_string(fps * 2));
  REXLOG_INFO("Frame rate: {} FPS (guest refresh {} Hz)", fps, fps * 2);
}

}  // namespace frame_rate
