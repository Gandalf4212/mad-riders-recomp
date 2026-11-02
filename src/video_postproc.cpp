// WMV decoder post-processing level (debug / fix candidate).
//
// The game's WMV decoder picks a post-processing (deblocking / deringing) level
// 0..4 every frame in sub_8315A258. The level the game asks for is the low
// nibble of decoder+15620 (-1 = automatic, chosen from measured decode times);
// the level in use is decoder+15612. In the recomp the automatic choice sits at
// 0 (no post-processing), which may be what leaves the videos grainy.
// --recomp_video_postproc=N (0..4) forces level N; -1 keeps the game's choice.
#include <cstdint>
#include <cstring>

#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/logging.h>

REXCVAR_DEFINE_INT32(recomp_video_postproc, -1, "Recomp",
                     "Video post-processing level 0-4 (deblock/dering) for the WMV decoder; "
                     "-1 = the game's automatic choice");

namespace {
uint32_t load_be32(uint8_t* base, uint32_t a) {
  uint32_t v;
  std::memcpy(&v, base + a, 4);
  return __builtin_bswap32(v);
}
void store_be32(uint8_t* base, uint32_t a, uint32_t v) {
  v = __builtin_bswap32(v);
  std::memcpy(base + a, &v, 4);
}
}  // namespace

extern "C" void __imp__sub_8315A258(PPCContext& __restrict ctx, uint8_t* base);

REX_HOOK_RAW(sub_8315A258) {
  const uint32_t dec = ctx.r3.u32;
  const int32_t forced = REXCVAR_GET(recomp_video_postproc);
  if (forced >= 0 && forced <= 4 && dec) {
    uint32_t req = load_be32(base, dec + 15620);
    store_be32(base, dec + 15620, (req & ~0xFu) | uint32_t(forced));
  }
  __imp__sub_8315A258(ctx, base);
  if (dec) {
    static int32_t last = -100;
    int32_t level = int32_t(load_be32(base, dec + 15612));
    if (level != last) {
      last = level;
      REXLOG_INFO("Video post-processing level {} (requested {}, {})", level,
                  int32_t(load_be32(base, dec + 15620) << 28) >> 28,
                  forced >= 0 ? "forced" : "game's choice");
    }
  }
}
