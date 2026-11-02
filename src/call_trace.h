// Debug: records the registers of sampled calls to selected recompiled functions
// (the video decoder's motion compensation and other untested decoder routines),
// so they can be replayed offline against a reference interpreter together with
// an F8 memory snapshot. Off unless --recomp_call_trace=true.
//
// File format (call_trace.bin, little-endian): "MRT1", u32 record count, then per
// record: u32 func, u32 seq (call number), u64 calls_total, u64 gpr[32], u32 cr
// (8 nibbles, cr0 high), u32 xer_ca, u8 v[128][16] (host order as stored in PPCContext);
// then "CNT1", u32 n, n * (u32 func, u64 calls).
#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>

#include <rex/ppc/context.h>

namespace call_trace {

#include <functional>

// on_ready runs once (on a worker thread) a few seconds after the decoder is
// first used, so a snapshot can be taken automatically mid-video.
void set_enabled(bool on, std::function<void()> on_ready = {});
void record(uint32_t index, uint32_t func, const PPCContext& ctx);
// Writes call_trace.bin into dir. Returns false if tracing is off or nothing was recorded.
bool save(const std::filesystem::path& dir);

}  // namespace call_trace
