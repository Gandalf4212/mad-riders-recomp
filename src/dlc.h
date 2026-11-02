// Downloadable content (Xbox LIVE marketplace packages, content type 00000002).
//
// The player's own DLC packages (e.g. 58411240\00000002\35842B90...) are
// installed with ReXGlue's ContentManager::InstallContent, which extracts the
// package into <user data>\0000000000000000\58411240\00000002\<package name>\
// and writes the .header the game's content enumeration reads.
//
// Two ways in:
//  - automatic: packages in a "dlc" folder next to the exe, or anywhere up to
//    4 folders below it (e.g. 58411240\00000002\<file>), are installed at launch;
//  - the "Import DLC" row in the F1 menu opens a file dialog.
#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace rex::system {
class KernelState;
}

namespace dlc {

inline constexpr uint32_t kDlcContentType = 0x00000002;

// Installs every DLC package for title_id found next to the exe that isn't
// installed yet. Call once the XEX is loaded (the kernel knows the title ID).
// Returns how many were installed.
// user_root is the runtime's user data root (where ReXGlue keeps content).
int install_found(rex::system::KernelState* kernel, const std::filesystem::path& user_root,
                  const std::filesystem::path& exe_dir, uint32_t title_id);

// Starts the "Import DLC" flow on a worker thread: file dialog, checks, install.
// False if one is already running.
bool start_import(rex::system::KernelState* kernel, const std::filesystem::path& user_root,
                  const std::filesystem::path& exe_dir, uint32_t title_id);

// Status line for the menu: empty when nothing has happened yet.
std::string import_status();

// Names of the installed DLC packages (display names), comma separated.
std::string installed_summary(rex::system::KernelState* kernel, uint32_t title_id);

}  // namespace dlc
