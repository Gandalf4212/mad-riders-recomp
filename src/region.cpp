// Game region reported to the game (XGetGameRegion).
//
// ReXGlue answers 0xFFFF ("all regions"). Mad Riders maps the value to a
// region code at start-up (sub_823FE840): 0xFF -> "us", 0x101/0x102/0x1FC/0x1FF
// -> "jp", 0x201/0x2FE/0x2FF -> "eu", anything else -> none. With no region the
// North American ESRB notice (IntroSplashes\mad_riders_esrb.dds) is never shown.
// Default to North America (0xFF), as Xenia reports it.

#include <cstdint>

#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/logging.h>

REXCVAR_DEFINE_UINT32(recomp_game_region, 0xFF, "Recomp",
                      "XGetGameRegion value: 0xFF North America (ESRB notice), 0x2FF Europe, "
                      "0x1FF Japan, 0xFFFF all regions (ReXGlue's default: the game picks none)");

REX_HOOK_RAW(__imp__XGetGameRegion) {
  (void)base;
  const uint32_t region = REXCVAR_GET(recomp_game_region);
  static bool logged = false;
  if (!logged) {
    logged = true;
    REXLOG_INFO("XGetGameRegion -> {:#x}", region);
  }
  ctx.r3.u64 = region;
}
