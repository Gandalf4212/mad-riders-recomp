// Memory snapshot - see debug_dump.h.
//
// File format (big parts named part000.bin, part001.bin, ...), a stream of records:
//   "MRD1" magic at the start of every part
//   record: u32 kind (0 = guest virtual, 1 = guest physical), u32 guest address,
//           u32 size, then size bytes (guest memory, big-endian as in the console)
// All-zero 64 KB blocks are skipped. Virtual memory above 0x80000000 is the
// physical memory seen through other windows, so physical memory is saved once
// on its own instead.

#include "debug_dump.h"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>

#include <rex/logging.h>
#include <rex/system/xmemory.h>

namespace debug_dump {
namespace {

constexpr uint64_t kPartLimit = 380ull << 20;
constexpr uint32_t kBlock = 64 * 1024;
constexpr uint32_t kPhysicalSize = 512u << 20;

struct Writer {
  std::filesystem::path dir;
  FILE* f = nullptr;
  int part = 0;
  uint64_t in_part = 0, total = 0;
  uint32_t records = 0;

  bool next_part() {
    if (f) fclose(f);
    char name[32];
    snprintf(name, sizeof name, "part%03d.bin", part++);
    f = fopen((dir / name).string().c_str(), "wb");
    if (!f) return false;
    fwrite("MRD1", 1, 4, f);
    in_part = 4;
    return true;
  }
  bool record(uint32_t kind, uint32_t addr, const uint8_t* data, uint32_t size) {
    if (!f || in_part + size + 12 > kPartLimit)
      if (!next_part()) return false;
    uint32_t h[3] = {kind, addr, size};
    if (fwrite(h, 4, 3, f) != 3 || fwrite(data, 1, size, f) != size) return false;
    in_part += size + 12;
    total += size;
    records++;
    return true;
  }
  ~Writer() { if (f) fclose(f); }
};

bool all_zero(const uint8_t* p, uint32_t n) {
  for (uint32_t i = 0; i < n; i += 8) {
    uint64_t v;
    memcpy(&v, p + i, 8);
    if (v) return false;
  }
  return true;
}

// Saves [base, base+size) in 64 KB blocks, skipping all-zero ones.
bool save_range(Writer& w, uint32_t kind, uint32_t base, const uint8_t* host, uint32_t size) {
  for (uint32_t off = 0; off < size; off += kBlock) {
    uint32_t n = std::min(kBlock, size - off);
    if (all_zero(host + off, n)) continue;
    if (!w.record(kind, base + off, host + off, n)) return false;
  }
  return true;
}

}  // namespace

std::filesystem::path save_memory(rex::memory::Memory* memory, const std::filesystem::path& exe_dir) {
  if (!memory) return {};
  auto t0 = std::chrono::steady_clock::now();
  std::time_t now = std::time(nullptr);
  char stamp[32];
  std::strftime(stamp, sizeof stamp, "%Y%m%d_%H%M%S", std::localtime(&now));
  Writer w;
  w.dir = exe_dir / (std::string("memdump_") + stamp);
  std::error_code ec;
  std::filesystem::create_directories(w.dir, ec);
  if (ec) return {};

  // Guest virtual memory below 0x80000000: committed regions only.
  uint32_t vregions = 0;
  for (uint64_t addr = 0x00010000; addr < 0x80000000ull;) {
    auto* heap = memory->LookupHeap(static_cast<uint32_t>(addr));
    if (!heap) { addr += 0x10000; continue; }
    rex::memory::HeapAllocationInfo info{};
    if (!heap->QueryRegionInfo(static_cast<uint32_t>(addr), &info) || !info.region_size) {
      addr += heap->page_size();
      continue;
    }
    const bool committed = (info.state & rex::memory::kMemoryAllocationCommit) != 0;
    const bool readable = (info.protect & rex::memory::kMemoryProtectRead) != 0;
    if (committed && readable) {
      const uint8_t* host = memory->TranslateVirtual<const uint8_t*>(info.base_address);
      if (!save_range(w, 0, info.base_address, host, info.region_size)) return {};
      vregions++;
    }
    addr = static_cast<uint64_t>(info.base_address) + info.region_size;
  }
  // Guest physical memory (textures, GPU buffers, anything the game put there).
  if (!save_range(w, 1, 0, memory->TranslatePhysical<const uint8_t*>(0), kPhysicalSize)) return {};

  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - t0).count();
  REXLOG_INFO("Memory snapshot: {} ({} MB in {} parts, {} virtual regions, {} ms)",
              w.dir.filename().string(), w.total >> 20, w.part, vregions, ms);
  return w.dir;
}

}  // namespace debug_dump
