// First-run setup from the player's own XBLA package - see package_setup.h.
//
// STFS layout handling (block addressing, hash tables, file table) adapted from
// afterburner-climax-recomp's package_setup.cpp:
//   MIT License, Copyright (c) 2026 darkuni
//   https://github.com/darkuni/afterburner-climax-recomp
// Changes here: streaming writes, extraction into a temp folder then rename,
// a file picker when no package is found, a progress window, and a
// package-name probe.

#include "package_setup.h"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <map>
#include <system_error>
#include <thread>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#ifdef _MSC_VER
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "gdi32.lib")
#endif
#endif

namespace fs = std::filesystem;

namespace package_setup {
namespace {

constexpr uint32_t kBlock = 0x1000;
constexpr uint32_t kXblaContentType = 0x000D0000;

uint32_t be32(const uint8_t* p) { return (uint32_t)p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3]; }
uint32_t le24(const uint8_t* p) { return p[0] | p[1] << 8 | p[2] << 16; }
uint32_t be24(const uint8_t* p) { return (uint32_t)p[0] << 16 | p[1] << 8 | p[2]; }

// ------------------------------------------------------------------ SHA-1
struct Sha1 {
  uint32_t h[5] = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0};
  uint8_t buf[64];
  size_t buf_len = 0;
  uint64_t total = 0;

  static uint32_t rol(uint32_t x, int n) { return (x << n) | (x >> (32 - n)); }
  void block(const uint8_t* p) {
    uint32_t w[80];
    for (int i = 0; i < 16; i++) w[i] = be32(p + 4 * i);
    for (int i = 16; i < 80; i++) w[i] = rol(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
    uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4];
    for (int i = 0; i < 80; i++) {
      uint32_t f, k;
      if (i < 20) { f = (b & c) | (~b & d); k = 0x5A827999; }
      else if (i < 40) { f = b ^ c ^ d; k = 0x6ED9EBA1; }
      else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDC; }
      else { f = b ^ c ^ d; k = 0xCA62C1D6; }
      uint32_t t = rol(a, 5) + f + e + k + w[i];
      e = d; d = c; c = rol(b, 30); b = a; a = t;
    }
    h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e;
  }
  void update(const uint8_t* p, size_t n) {
    total += n;
    while (n) {
      size_t take = std::min(n, 64 - buf_len);
      memcpy(buf + buf_len, p, take);
      buf_len += take; p += take; n -= take;
      if (buf_len == 64) { block(buf); buf_len = 0; }
    }
  }
  void final(uint8_t out[20]) {
    uint64_t bits = total * 8;
    uint8_t pad = 0x80;
    update(&pad, 1);
    uint8_t zero = 0;
    while (buf_len != 56) update(&zero, 1);
    uint8_t len[8];
    for (int i = 0; i < 8; i++) len[i] = (uint8_t)(bits >> (56 - 8 * i));
    update(len, 8);
    for (int i = 0; i < 5; i++)
      for (int k = 0; k < 4; k++) out[4 * i + k] = (uint8_t)(h[i] >> (24 - 8 * k));
  }
};

void sha1(const uint8_t* data, size_t len, uint8_t out[20]) {
  Sha1 s;
  s.update(data, len);
  s.final(out);
}

FILE* open_file(const fs::path& p, const char* mode) {
#ifdef _WIN32
  std::wstring wmode(mode, mode + strlen(mode));
  return _wfopen(p.wstring().c_str(), wmode.c_str());
#else
  return fopen(p.string().c_str(), mode);
#endif
}

// ------------------------------------------------------------------ STFS
struct Stfs {
  FILE* f = nullptr;
  uint64_t file_size = 0;
  uint8_t hdr[0x1800] = {0};
  uint32_t header_size = 0, title_id = 0, content_type = 0;
  uint32_t ft_block_count = 0, ft_block_num = 0, sex = 0, step0 = 0;
  uint64_t first_ht = 0;
  int bad_hashes = 0;

  ~Stfs() { if (f) fclose(f); }

  bool open(const fs::path& p) {
    f = open_file(p, "rb");
    if (!f) return false;
    if (fread(hdr, 1, sizeof hdr, f) != sizeof hdr) return false;
    if (memcmp(hdr, "LIVE", 4) && memcmp(hdr, "PIRS", 4) && memcmp(hdr, "CON ", 4)) return false;
    seek(0, SEEK_END);
    file_size = tell();
    header_size = be32(hdr + 0x340);
    content_type = be32(hdr + 0x344);
    title_id = be32(hdr + 0x360);
    const uint8_t* vd = hdr + 0x379;
    uint8_t block_sep = vd[2];
    ft_block_count = vd[3] | vd[4] << 8;
    ft_block_num = le24(vd + 5);
    sex = (~block_sep) & 1;  // 0 = one hash table per group, 1 = two
    step0 = sex == 0 ? 0xAB : 0xAC;
    first_ht = (header_size + 0xFFF) & 0xFFFFF000u;
    return true;
  }
  std::string display_name() const {
    // UTF-16BE title at 0x411 (first of 18 localized slots, 0x100 bytes each).
    std::string s;
    for (int i = 0; i < 0x80; i++) {
      uint16_t c = (uint16_t)(hdr[0x411 + 2 * i] << 8 | hdr[0x412 + 2 * i]);
      if (!c) break;
      s.push_back(c < 0x80 ? (char)c : '?');
    }
    return s;
  }
  void seek(uint64_t o, int whence = SEEK_SET) {
#ifdef _WIN32
    _fseeki64(f, (long long)o, whence);
#else
    fseeko(f, (off_t)o, whence);
#endif
  }
  uint64_t tell() {
#ifdef _WIN32
    return (uint64_t)_ftelli64(f);
#else
    return (uint64_t)ftello(f);
#endif
  }
  void read_at(uint64_t off, uint8_t* out, size_t n) {
    memset(out, 0, n);
    if (off >= file_size) return;
    seek(off);
    size_t got = fread(out, 1, (size_t)std::min<uint64_t>(n, file_size - off), f);
    (void)got;
  }
  uint64_t backing_block(uint64_t n) const {
    uint64_t r = (((n + 0xAA) / 0xAA) << sex) + n;
    if (n < 0xAA) return r;
    if (n < 0x70E4) return r + (((n + 0x70E4) / 0x70E4) << sex);
    return (1ull << sex) + r + (((n + 0x70E4) / 0x70E4) << sex);
  }
  uint64_t block_addr(uint64_t n) const { return (backing_block(n) << 12) + first_ht; }
  uint64_t l0_hash_block(uint64_t n) const {
    if (n < 0xAA) return 0;
    uint64_t num = (n / 0xAA) * step0;
    num += ((n / 0x70E4) + 1) << sex;
    if (n / 0x70E4 == 0) return num;
    return num + (1ull << sex);
  }
  // Hash entry: sha1[20], status, next block (24-bit BE).
  void hash_entry(uint64_t n, uint8_t e[24]) {
    uint64_t addr = (l0_hash_block(n) << 12) + first_ht + (n % 0xAA) * 0x18;
    read_at(addr, e, 24);
    if (sex == 1) {
      static const uint8_t zero[20] = {0};
      uint8_t e2[24];
      read_at(addr + 0x1000, e2, 24);
      if (!memcmp(e, zero, 20) && memcmp(e2, zero, 20)) memcpy(e, e2, 24);
    }
  }
  void read_block(uint64_t n, uint8_t* out) {
    read_at(block_addr(n), out, kBlock);
    uint8_t e[24], h[20];
    hash_entry(n, e);
    sha1(out, kBlock, h);
    if (memcmp(h, e, 20)) bad_hashes++;
  }
  // Next block of a file: consecutive files just count up, others follow the hash-table chain.
  uint64_t next(uint64_t n, bool consecutive) {
    if (consecutive) return n + 1;
    uint8_t e[24];
    hash_entry(n, e);
    return be24(e + 21);
  }
};

#ifdef _WIN32
std::wstring widen(const std::string& s) {
  if (s.empty()) return {};
  int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
  std::wstring w(n, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), w.data(), n);
  return w;
}
#endif

void show_message(const std::string& title, const std::string& text, bool error) {
  fprintf(stderr, "%s\n", text.c_str());
#ifdef _WIN32
  MessageBoxW(nullptr, widen(text).c_str(), widen(title).c_str(),
              MB_OK | (error ? MB_ICONERROR : MB_ICONINFORMATION));
#else
  (void)title; (void)error;
#endif
}

bool ask_yes_no(const std::string& title, const std::string& text) {
#ifdef _WIN32
  return MessageBoxW(nullptr, widen(text).c_str(), widen(title).c_str(),
                     MB_YESNO | MB_ICONQUESTION) == IDYES;
#else
  fprintf(stderr, "%s\n", text.c_str());
  (void)title;
  return false;
#endif
}

// Lets the player point at the package file. Empty path = cancelled.
fs::path pick_package(const GameInfo& game, const fs::path& start_dir) {
#ifdef _WIN32
  wchar_t file[MAX_PATH * 4] = L"";
  std::wstring title = widen("Select your " + std::string(game.name) + " package file");
  std::wstring dir = start_dir.wstring();
  OPENFILENAMEW ofn{};
  ofn.lStructSize = sizeof(ofn);
  ofn.lpstrFilter = L"Xbox 360 package (no extension)\0*\0All files\0*.*\0";
  ofn.lpstrFile = file;
  ofn.nMaxFile = (DWORD)(sizeof(file) / sizeof(file[0]));
  ofn.lpstrTitle = title.c_str();
  ofn.lpstrInitialDir = dir.c_str();
  ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_HIDEREADONLY;
  if (!GetOpenFileNameW(&ofn)) return {};
  return fs::path(file);
#else
  (void)game; (void)start_dir;
  return {};
#endif
}

// Extracts with a small progress window (Windows) on a worker thread.
bool extract_with_progress(const fs::path& package, const fs::path& out, const std::string& game,
                           std::string& err, int* bad) {
#ifdef _WIN32
  INITCOMMONCONTROLSEX icc{sizeof(icc), ICC_PROGRESS_CLASS};
  InitCommonControlsEx(&icc);
  const int w = 460, h = 130;
  RECT wa;
  SystemParametersInfoW(SPI_GETWORKAREA, 0, &wa, 0);
  HWND wnd = CreateWindowExW(WS_EX_TOPMOST | WS_EX_DLGMODALFRAME, L"#32770",
                             widen(game + " - first-time setup").c_str(),
                             WS_POPUP | WS_CAPTION | WS_VISIBLE,
                             wa.left + (wa.right - wa.left - w) / 2,
                             wa.top + (wa.bottom - wa.top - h) / 2, w, h,
                             nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
  HWND label = CreateWindowExW(0, L"STATIC", L"Extracting game files from your package...",
                               WS_CHILD | WS_VISIBLE, 16, 14, w - 40, 20, wnd, nullptr, nullptr, nullptr);
  HWND bar = CreateWindowExW(0, PROGRESS_CLASSW, nullptr, WS_CHILD | WS_VISIBLE, 16, 42, w - 40, 22,
                             wnd, nullptr, nullptr, nullptr);
  HFONT font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
  SendMessageW(label, WM_SETFONT, (WPARAM)font, TRUE);
  SendMessageW(bar, PBM_SETRANGE32, 0, 1000);

  std::atomic<uint64_t> done{0}, total{1};
  std::atomic<bool> finished{false};
  bool ok = false;
  std::thread worker([&] {
    ok = extract(package, out, err, bad, [&](uint64_t d, uint64_t t) { done = d; total = t ? t : 1; });
    finished = true;
  });
  while (!finished) {
    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
    }
    uint64_t d = done, t = total;
    SendMessageW(bar, PBM_SETPOS, (WPARAM)(d * 1000 / t), 0);
    wchar_t text[128];
    swprintf(text, 128, L"Extracting game files from your package...  %llu / %llu MB",
             (unsigned long long)(d >> 20), (unsigned long long)(t >> 20));
    SetWindowTextW(label, text);
    Sleep(50);
  }
  worker.join();
  DestroyWindow(wnd);
  return ok;
#else
  (void)game;
  return extract(package, out, err, bad);
#endif
}

bool xex_matches(const fs::path& xex, const GameInfo& game, uint64_t& size, std::string& sha1_hex) {
  std::error_code ec;
  size = fs::file_size(xex, ec);
  sha1_hex = "(unreadable)";
  if (ec) return false;
  FILE* f = open_file(xex, "rb");
  if (!f) return false;
  Sha1 s;
  std::vector<uint8_t> buf(1 << 20);
  size_t n;
  while ((n = fread(buf.data(), 1, buf.size(), f)) > 0) s.update(buf.data(), n);
  fclose(f);
  uint8_t h[20];
  s.final(h);
  char hex[41];
  for (int i = 0; i < 20; i++) snprintf(hex + i * 2, 3, "%02x", h[i]);
  sha1_hex = hex;
  return size == game.xex_size && sha1_hex == game.xex_sha1_hex;
}

}  // namespace

// ================================================================== public

bool probe(const fs::path& package, PackageInfo& info) {
  Stfs s;
  if (!s.open(package)) return false;
  info.title_id = s.title_id;
  info.content_type = s.content_type;
  info.display_name = s.display_name();
  return true;
}

bool extract(const fs::path& package, const fs::path& out, std::string& err, int* bad_hashes,
             const std::function<void(uint64_t, uint64_t)>& progress) {
  Stfs s;
  if (!s.open(package)) {
    err = package.filename().string() + " is not an Xbox 360 package";
    return false;
  }
  // File table.
  std::vector<uint8_t> raw;
  std::vector<uint8_t> blk(kBlock);
  for (uint64_t b = s.ft_block_num, i = 0; i < s.ft_block_count; i++, b = s.next(b, false)) {
    s.read_block(b, blk.data());
    raw.insert(raw.end(), blk.begin(), blk.end());
  }
  struct Ent {
    std::string name;
    bool consecutive, dir;
    uint32_t start, size;
    int parent;
  };
  std::map<int, Ent> ents;
  std::vector<int> order;
  uint64_t total = 0;
  for (size_t i = 0; i + 0x40 <= raw.size(); i += 0x40) {
    const uint8_t* e = raw.data() + i;
    int nl = e[0x28] & 0x3F;
    if (!nl) continue;
    Ent en{std::string((const char*)e, nl), (e[0x28] & 0x40) != 0, (e[0x28] & 0x80) != 0,
           le24(e + 0x2F), be32(e + 0x34), (int16_t)(e[0x32] << 8 | e[0x33])};
    // Names come from the file: refuse anything that could escape the output folder.
    if (en.name == "." || en.name == ".." || en.name.find_first_of("/\\:") != std::string::npos) {
      err = "package has an invalid file name";
      return false;
    }
    if (!en.dir) total += en.size;
    ents[(int)(i / 0x40)] = en;
    order.push_back((int)(i / 0x40));
  }
  auto path_of = [&](int idx) {
    std::vector<std::string> parts;
    for (int p = idx, guard = 0; p != -1 && ents.count(p) && guard < 64; guard++) {
      parts.push_back(ents[p].name);
      p = ents[p].parent;
    }
    fs::path r = out;
    for (auto it = parts.rbegin(); it != parts.rend(); ++it) r /= fs::path(std::u8string(it->begin(), it->end()));
    return r;
  };
  std::error_code ec;
  fs::create_directories(out, ec);
  uint64_t done = 0;
  for (int idx : order) {
    const Ent& e = ents[idx];
    fs::path p = path_of(idx);
    if (e.dir) {
      fs::create_directories(p, ec);
      continue;
    }
    fs::create_directories(p.parent_path(), ec);
    FILE* w = open_file(p, "wb");
    if (!w) {
      err = "cannot write " + p.string();
      return false;
    }
    uint64_t left = e.size;
    uint64_t b = e.start;
    bool ok = true;
    while (left) {
      s.read_block(b, blk.data());
      size_t n = (size_t)std::min<uint64_t>(left, kBlock);
      if (fwrite(blk.data(), 1, n, w) != n) { ok = false; break; }
      left -= n;
      done += n;
      if (left) b = s.next(b, e.consecutive);
      if (progress && (done & 0xFFFFF) < kBlock) progress(done, total);
    }
    if (fclose(w) != 0 || !ok) {
      err = "cannot write " + p.string() + " (disk full?)";
      return false;
    }
  }
  if (progress) progress(total, total);
  if (bad_hashes) *bad_hashes = s.bad_hashes;
  return true;
}

fs::path find_package(const fs::path& exe_dir, const fs::path& skip_dir, uint32_t title_id) {
  std::error_code ec;
  std::vector<fs::path> cands;
  auto it = fs::recursive_directory_iterator(exe_dir, fs::directory_options::skip_permission_denied, ec);
  for (; !ec && it != fs::recursive_directory_iterator(); it.increment(ec)) {
    const auto& e = *it;
    if (e.is_directory(ec)) {
      if (it.depth() >= 3 || e.path() == skip_dir) it.disable_recursion_pending();
      continue;
    }
    if (!e.is_regular_file(ec) || e.file_size(ec) < 0x2000) continue;
    PackageInfo info;
    if (probe(e.path(), info) && info.title_id == title_id && info.content_type == kXblaContentType)
      cands.push_back(e.path());
  }
  if (cands.empty()) return {};
  std::sort(cands.begin(), cands.end());
  return cands[0];
}

bool ensure_assets(const fs::path& exe_dir, const fs::path& assets, const GameInfo& game) {
  const std::string name = game.name;
  std::error_code ec;
  if (!fs::exists(assets / "default.xex", ec)) {
    fs::path package = find_package(exe_dir, assets, game.title_id);
    while (package.empty()) {
      char tid[16];
      snprintf(tid, sizeof tid, "%08X", game.title_id);
      if (!ask_yes_no(name,
                      "Game files not found.\n\n"
                      "This needs your own " + name + " Xbox Live Arcade package: the file from your "
                      "console's storage, found in " + tid + "\\000D0000\\ (a long name with no "
                      "extension, e.g. 0CE0DC5A...).\n\n"
                      "Select it now? It is extracted into an \"assets\" folder next to this exe "
                      "(about 1.5 GB), once."))
        return false;
      fs::path picked = pick_package(game, exe_dir);
      if (picked.empty()) continue;  // cancelled the dialog: ask again
      PackageInfo info;
      if (!probe(picked, info)) {
        show_message(name, picked.filename().string() + " is not an Xbox 360 package.", true);
        continue;
      }
      if (info.title_id != game.title_id || info.content_type != kXblaContentType) {
        char got[16];
        snprintf(got, sizeof got, "%08X", info.title_id);
        show_message(name, "That package is " +
                               (info.display_name.empty() ? std::string("another game") : info.display_name) +
                               " (title ID " + got + "), not " + name + " (" + tid + ").",
                     true);
        continue;
      }
      package = picked;
    }

    // Extract next to the final folder, then rename, so a failed or cancelled run leaves nothing half-done.
    fs::path tmp = assets;
    tmp += ".partial";
    fs::remove_all(tmp, ec);
    std::string err;
    int bad = 0;
    if (!extract_with_progress(package, tmp, name, err, &bad)) {
      fs::remove_all(tmp, ec);
      show_message(name, "Could not extract " + package.filename().string() + ":\n" + err, true);
      return false;
    }
    if (!fs::exists(tmp / "default.xex", ec)) {
      fs::remove_all(tmp, ec);
      show_message(name, package.filename().string() + " has no default.xex.", true);
      return false;
    }
    if (bad) fprintf(stderr, "[setup] %s: %d blocks failed their hash check\n",
                     package.filename().string().c_str(), bad);
    fs::remove_all(assets, ec);
    fs::rename(tmp, assets, ec);
    if (ec) {
      show_message(name, "Could not create the assets folder: " + ec.message(), true);
      return false;
    }
  }

  uint64_t size = 0;
  std::string got;
  if (xex_matches(assets / "default.xex", game, size, got)) return true;
  show_message(name,
               "This build needs a different version of the game files.\n\n"
               "It was made from " + name + " with a default.xex of " + std::to_string(game.xex_size) +
                   " bytes (SHA-1 " + game.xex_sha1_hex + ").\n\n"
               "Your default.xex is " + std::to_string(size) + " bytes (SHA-1 " + got + ").\n\n"
               "Use the original, unpatched XBLA package. To start over, delete the \"assets\" "
               "folder next to the exe.",
               true);
  return false;
}

}  // namespace package_setup

#ifdef PACKAGE_SETUP_TOOL
// Test driver: package_setup_tool <package> <outdir>
int main(int argc, char** argv) {
  if (argc < 3) {
    fprintf(stderr, "usage: %s <package> <outdir>\n", argv[0]);
    return 2;
  }
  package_setup::PackageInfo info;
  if (!package_setup::probe(argv[1], info)) { fprintf(stderr, "not a package\n"); return 1; }
  fprintf(stderr, "title %08X type %08X name '%s'\n", info.title_id, info.content_type,
          info.display_name.c_str());
  std::string err;
  int bad = 0;
  if (!package_setup::extract(argv[1], argv[2], err, &bad)) {
    fprintf(stderr, "failed: %s\n", err.c_str());
    return 1;
  }
  fprintf(stderr, "ok, %d bad block hashes\n", bad);
  return 0;
}
#endif
