// Frame-rate setting (recomp_frame_rate: 30, 60 or 120).
//
// Mad Riders presents every second guest vblank, so it runs at half the guest
// refresh rate: 60 Hz gives the original 30 FPS. The setting raises the guest
// vblank rate (video_mode_refresh_rate = 2 x FPS). The game caps itself at 120 FPS
// (an uncapped option reached 120 at most), so there is no higher setting. Applied at start-up.
#pragma once

namespace frame_rate {
void apply();
// Windows: 1 ms timer resolution (timeBeginPeriod). Called by apply().
void raise_timer_resolution();
}
