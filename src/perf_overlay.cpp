// See perf_overlay.h.
#include "perf_overlay.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdio>

#include <rex/cvar.h>
#include <rex/hook.h>

#include <imgui.h>

REXCVAR_DEFINE_INT32(recomp_perf_overlay, 0, "Recomp",
                     "Performance overlay: 0 off, 1 FPS, 2 FPS + frame time graph");

namespace {

constexpr int kHistory = 120;
std::array<std::atomic<float>, kHistory> g_frame_ms{};
std::atomic<uint32_t> g_frames{0};
std::atomic<int64_t> g_last_us{0};

int64_t now_us() {
  return std::chrono::duration_cast<std::chrono::microseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

}  // namespace

// The game's present (calls VdSwap once per frame).
extern "C" void __imp__sub_82E6FFE0(PPCContext& __restrict ctx, uint8_t* base);
REX_HOOK_RAW(sub_82E6FFE0) {
  int64_t t = now_us();
  int64_t last = g_last_us.exchange(t, std::memory_order_relaxed);
  if (last) {
    uint32_t n = g_frames.fetch_add(1, std::memory_order_relaxed);
    g_frame_ms[n % kHistory].store(float(t - last) / 1000.0f, std::memory_order_relaxed);
  }
  __imp__sub_82E6FFE0(ctx, base);
}

namespace recomp_ui {

void PerfOverlay::OnDraw(ImGuiIO& io) {
  int mode = REXCVAR_GET(recomp_perf_overlay);
  if (mode <= 0) return;
  uint32_t n = g_frames.load(std::memory_order_relaxed);
  int count = int(std::min<uint32_t>(n, kHistory));
  if (!count) return;

  // Average over the most recent ~0.5 s of frames.
  float sum = 0, worst = 0;
  int used = 0;
  std::array<float, kHistory> hist{};
  for (int i = 0; i < count; i++) {
    float ms = g_frame_ms[(n - count + i) % kHistory].load(std::memory_order_relaxed);
    hist[i] = ms;
  }
  for (int i = count - 1; i >= 0 && sum < 500.0f; i--) {
    sum += hist[i];
    worst = std::max(worst, hist[i]);
    used++;
  }
  float avg = used ? sum / used : 0;
  // A stalled game (loading) shows the time since the last frame.
  float since = float(now_us() - g_last_us.load()) / 1000.0f;
  if (since > 250.0f) avg = since;

  char line[96];
  std::snprintf(line, sizeof line, "%.0f FPS  %.1f ms", avg > 0 ? 1000.0f / avg : 0.0f, avg);

  const float pad = 10.0f;
  ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - pad, pad), ImGuiCond_Always, ImVec2(1, 0));
  ImGui::SetNextWindowBgAlpha(0.45f);
  ImGui::Begin("##recomp_perf", nullptr,
               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                   ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                   ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoInputs);
  ImGui::TextUnformatted(line);
  if (mode >= 2) {
    std::snprintf(line, sizeof line, "worst %.1f ms", worst);
    ImGui::TextUnformatted(line);
    ImGui::PlotLines("##ft", hist.data(), count, 0, nullptr, 0.0f, 50.0f, ImVec2(220, 50));
  }
  ImGui::End();
}

}  // namespace recomp_ui
