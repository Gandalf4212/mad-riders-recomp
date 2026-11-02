// See call_trace.h.
#include "call_trace.h"

#include <array>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <vector>

#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/logging.h>

REXCVAR_DEFINE_BOOL(recomp_call_trace, false, "Recomp",
                    "Debug: record recent calls to the video decoder routines (saved with the F8 snapshot)");

namespace call_trace {
namespace {

constexpr int kSlots = 16;  // most recent calls kept per function

struct Rec {
  uint32_t func, seq;
  uint64_t total;
  uint64_t gpr[32];
  uint32_t cr, ca;
  uint8_t v[128][16];
};

std::atomic<bool> g_on{false};

}  // namespace
}  // namespace call_trace

#include "call_trace_hooks.inc"

namespace call_trace {
namespace {

struct Store {
  std::array<std::atomic<uint64_t>, kTracedCount> calls{};
  std::array<std::array<Rec, kSlots>, kTracedCount> recs{};
};

Store& store() {
  static Store* s = new Store();  // ~37 MB, only allocated when tracing is used
  return *s;
}

uint32_t pack_cr(const PPCContext& c) {
  const PPCCRRegister* f[8] = {&c.cr0, &c.cr1, &c.cr2, &c.cr3, &c.cr4, &c.cr5, &c.cr6, &c.cr7};
  uint32_t v = 0;
  for (int i = 0; i < 8; i++) {
    uint32_t n = (f[i]->lt ? 8 : 0) | (f[i]->gt ? 4 : 0) | (f[i]->eq ? 2 : 0) | (f[i]->so ? 1 : 0);
    v |= n << (4 * (7 - i));
  }
  return v;
}

}  // namespace

void set_enabled(bool on) {
  if (on) store();
  g_on.store(on, std::memory_order_relaxed);
  if (on) REXLOG_INFO("Call trace on: {} decoder functions wrapped", kTracedCount);
}

void record(uint32_t index, uint32_t func, const PPCContext& ctx) {
  if (!g_on.load(std::memory_order_relaxed)) return;
  auto& s = store();
  uint64_t n = s.calls[index].fetch_add(1, std::memory_order_relaxed);
  Rec& r = s.recs[index][n % kSlots];
  r.func = 0;  // marks the slot as being written
  r.seq = uint32_t(n);
  r.total = n + 1;
  const PPCRegister* g[32] = {&ctx.r0,  &ctx.r1,  &ctx.r2,  &ctx.r3,  &ctx.r4,  &ctx.r5,  &ctx.r6,
                              &ctx.r7,  &ctx.r8,  &ctx.r9,  &ctx.r10, &ctx.r11, &ctx.r12, &ctx.r13,
                              &ctx.r14, &ctx.r15, &ctx.r16, &ctx.r17, &ctx.r18, &ctx.r19, &ctx.r20,
                              &ctx.r21, &ctx.r22, &ctx.r23, &ctx.r24, &ctx.r25, &ctx.r26, &ctx.r27,
                              &ctx.r28, &ctx.r29, &ctx.r30, &ctx.r31};
  for (int i = 0; i < 32; i++) r.gpr[i] = g[i]->u64;
  r.cr = pack_cr(ctx);
  r.ca = ctx.xer.ca;
  std::memcpy(r.v, &ctx.v0, sizeof(r.v));
  r.func = func;
}

bool save(const std::filesystem::path& dir) {
  if (!g_on.load()) return false;
  auto& s = store();
  std::vector<const Rec*> out;
  for (uint32_t i = 0; i < kTracedCount; i++) {
    uint64_t n = s.calls[i].load();
    for (uint64_t k = 0; k < std::min<uint64_t>(n, kSlots); k++) {
      const Rec& r = s.recs[i][k];
      if (r.func) out.push_back(&r);
    }
  }
  if (out.empty()) return false;
  std::filesystem::path p = dir / "call_trace.bin";
  FILE* f = nullptr;
#ifdef _WIN32
  _wfopen_s(&f, p.c_str(), L"wb");
#else
  f = std::fopen(p.c_str(), "wb");
#endif
  if (!f) return false;
  std::fwrite("MRT1", 1, 4, f);
  uint32_t count = uint32_t(out.size());
  std::fwrite(&count, 4, 1, f);
  for (const Rec* r : out) std::fwrite(r, sizeof(Rec), 1, f);
  std::fclose(f);
  REXLOG_INFO("Call trace: {} records written to {}", count, p.string());
  return true;
}

}  // namespace call_trace
