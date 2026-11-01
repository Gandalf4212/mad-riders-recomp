// First-run setup: builds the "assets" folder from the player's own Mad Riders
// XBLA package (the Xbox 360 LIVE file, e.g. 58411240\000D0000\0CE0DC5A...).
//
// STFS reader adapted from afterburner-climax-recomp (MIT, (c) 2026 darkuni):
// https://github.com/darkuni/afterburner-climax-recomp
#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>

namespace package_setup {

// What a package is, without extracting it.
struct PackageInfo {
  uint32_t title_id = 0;
  uint32_t content_type = 0;  // 0x000D0000 = Xbox Live Arcade title
  std::string display_name;   // from the package header, may be empty
};

// Reads the STFS header. False if the file isn't a LIVE/PIRS/CON package.
bool probe(const std::filesystem::path& package, PackageInfo& info);

// Extracts every file in the package into out. progress(done_bytes, total_bytes)
// is called as data is written. bad_hashes counts blocks whose SHA-1 didn't match.
bool extract(const std::filesystem::path& package, const std::filesystem::path& out,
             std::string& err, int* bad_hashes = nullptr,
             const std::function<void(uint64_t, uint64_t)>& progress = {});

// Looks next to the exe (and up to 3 folders down, e.g. 58411240\000D0000\<file>)
// for a package with this title ID. Empty path if none.
std::filesystem::path find_package(const std::filesystem::path& exe_dir,
                                   const std::filesystem::path& skip_dir, uint32_t title_id);

struct GameInfo {
  const char* name;           // "Mad Riders"
  uint32_t title_id;          // 0x58411240
  const char* xex_sha1_hex;   // the default.xex this build was recompiled from
  uint64_t xex_size;
};

// Runs before the game starts. If assets\default.xex is missing it finds the
// package (or asks the player to pick it), extracts it with a progress window,
// then checks default.xex is the version this build was made from.
// Returns false (after telling the player why) when the game can't start.
bool ensure_assets(const std::filesystem::path& exe_dir, const std::filesystem::path& assets,
                   const GameInfo& game);

}  // namespace package_setup
