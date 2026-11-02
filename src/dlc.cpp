// DLC install: see dlc.h.
#include "dlc.h"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <mutex>
#include <thread>
#include <vector>

#include <rex/logging.h>
#include <rex/string/utf8.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xam/content_device.h>
#include <rex/system/xam/content_manager.h>
#include <rex/system/xcontent.h>

#include "package_setup.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <commdlg.h>
#endif

namespace fs = std::filesystem;
using rex::system::KernelState;
using rex::system::XContentType;
using rex::system::xam::DummyDeviceId;

namespace dlc {
namespace {

std::mutex g_status_mutex;
std::string g_status;
std::atomic<bool> g_busy{false};
std::mutex g_install_mutex;  // one install at a time (launch scan vs menu)

void set_status(std::string s) {
  std::lock_guard<std::mutex> lock(g_status_mutex);
  g_status = std::move(s);
}

std::string hex8(uint32_t v) {
  char b[16];
  snprintf(b, sizeof b, "%08X", v);
  return b;
}

fs::path install_dir(const fs::path& user_root, uint32_t title_id, const fs::path& package) {
  return fs::absolute(user_root) / "0000000000000000" / hex8(title_id) / hex8(kDlcContentType) /
         package.filename();
}

fs::path header_file(const fs::path& user_root, uint32_t title_id, const fs::path& package) {
  fs::path p = fs::absolute(user_root) / "0000000000000000" / hex8(title_id) / "Headers" /
               hex8(kDlcContentType) / package.filename();
  p += ".header";
  return p;
}

// Installed = the folder and its .header both exist (a failed install has no header).
bool is_installed(const fs::path& user_root, uint32_t title_id, const fs::path& package) {
  std::error_code ec;
  return fs::is_directory(install_dir(user_root, title_id, package), ec) &&
         fs::is_regular_file(header_file(user_root, title_id, package), ec);
}

// Checks the file is a DLC package for this game. Empty string = OK, else why not.
std::string check_package(const fs::path& package, uint32_t title_id,
                          package_setup::PackageInfo& info) {
  if (!package_setup::probe(package, info)) {
    return package.filename().string() + " is not an Xbox 360 package.";
  }
  if (info.title_id != title_id) {
    return "That package is for another game (title ID " + hex8(info.title_id) + ", " +
           (info.display_name.empty() ? std::string("unnamed") : info.display_name) +
           "), not Mad Riders (" + hex8(title_id) + ").";
  }
  if (info.content_type != kDlcContentType) {
    return "That package is not DLC (content type " + hex8(info.content_type) +
           "; DLC is 00000002).";
  }
  return {};
}

// Empty string on success, else an error message.
std::string install(KernelState* kernel, const fs::path& user_root, uint32_t title_id,
                    const fs::path& package, const std::string& name) {
  std::lock_guard<std::mutex> lock(g_install_mutex);
  if (!kernel || !kernel->content_manager()) return "The game is not running yet.";
  REXLOG_INFO("DLC: installing {} ({})", name, package.string());
  auto result = kernel->content_manager()->InstallContent(package);
  if (result != 0) {
    // Don't leave a half-extracted folder the game would still list.
    std::error_code ec;
    fs::remove_all(install_dir(user_root, title_id, package), ec);
    fs::remove(header_file(user_root, title_id, package), ec);
    REXLOG_ERROR("DLC: install of {} failed: {:08X}", name, result);
    return "Could not install " + name + " (error " + hex8(result) + ").";
  }
  REXLOG_INFO("DLC: installed {} into {}", name,
              install_dir(user_root, title_id, package).string());
  return {};
}

std::vector<fs::path> find_dlc(const fs::path& exe_dir, uint32_t title_id) {
  std::vector<fs::path> found;
  std::error_code ec;
  auto it = fs::recursive_directory_iterator(exe_dir, fs::directory_options::skip_permission_denied, ec);
  for (; !ec && it != fs::recursive_directory_iterator(); it.increment(ec)) {
    const auto& e = *it;
    if (e.is_directory(ec)) {
      auto n = e.path().filename().string();
      // Skip the extracted game, crash dumps, caches and memory snapshots.
      if (it.depth() >= 4 || n == "assets" || n == "assets.partial" || n == "cache" ||
          n.rfind("memdump_", 0) == 0) {
        it.disable_recursion_pending();
      }
      continue;
    }
    if (!e.is_regular_file(ec) || e.file_size(ec) < 0x2000) continue;
    if (e.path().has_extension()) continue;  // STFS packages have no extension
    package_setup::PackageInfo info;
    if (package_setup::probe(e.path(), info) && info.title_id == title_id &&
        info.content_type == kDlcContentType) {
      found.push_back(e.path());
    }
  }
  std::sort(found.begin(), found.end());
  return found;
}

#ifdef _WIN32
fs::path pick_file(const fs::path& start_dir) {
  wchar_t file[MAX_PATH * 4] = {};
  std::wstring dir = start_dir.wstring();
  OPENFILENAMEW ofn{};
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = GetForegroundWindow();  // the game window, so the dialog stays on top
  ofn.lpstrFilter = L"Xbox 360 DLC package (no extension)\0*\0All files\0*.*\0";
  ofn.lpstrFile = file;
  ofn.nMaxFile = (DWORD)(sizeof(file) / sizeof(file[0]));
  ofn.lpstrTitle = L"Mad Riders - select a DLC package (from 58411240\\00000002\\)";
  ofn.lpstrInitialDir = dir.c_str();
  ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_HIDEREADONLY;
  if (!GetOpenFileNameW(&ofn)) return {};
  return fs::path(file);
}
#else
fs::path pick_file(const fs::path&) { return {}; }
#endif

}  // namespace

int install_found(KernelState* kernel, const fs::path& user_root, const fs::path& exe_dir,
                  uint32_t title_id) {
  int installed = 0;
  for (const auto& package : find_dlc(exe_dir, title_id)) {
    if (is_installed(user_root, title_id, package)) continue;
    package_setup::PackageInfo info;
    package_setup::probe(package, info);
    std::string name = info.display_name.empty() ? package.filename().string() : info.display_name;
    if (install(kernel, user_root, title_id, package, name).empty()) installed++;
  }
  if (installed) REXLOG_INFO("DLC: installed {} package(s) found next to the exe", installed);
  return installed;
}

bool start_import(KernelState* kernel, const fs::path& user_root, const fs::path& exe_dir,
                  uint32_t title_id) {
  bool expected = false;
  if (!g_busy.compare_exchange_strong(expected, true)) return false;
  set_status("Choose a DLC package in the file window...");
  std::thread([=] {
    fs::path package = pick_file(exe_dir);
    if (package.empty()) {
      set_status("Import cancelled.");
    } else {
      package_setup::PackageInfo info;
      std::string err = check_package(package, title_id, info);
      std::string name = info.display_name.empty() ? package.filename().string() : info.display_name;
      if (err.empty() && is_installed(user_root, title_id, package)) {
        set_status(name + " is already installed.");
      } else if (!err.empty()) {
        set_status(err);
      } else {
        set_status("Installing " + name + "...");
        err = install(kernel, user_root, title_id, package, name);
        set_status(err.empty() ? "Installed " + name + ". Restart the game to use it." : err);
      }
    }
    g_busy = false;
  }).detach();
  return true;
}

std::string import_status() {
  std::lock_guard<std::mutex> lock(g_status_mutex);
  return g_status;
}

std::string installed_summary(KernelState* kernel, uint32_t title_id) {
  if (!kernel || !kernel->content_manager()) return {};
  auto items = kernel->content_manager()->ListContent(static_cast<uint32_t>(DummyDeviceId::HDD), 0,
                                                     XContentType::kMarketplaceContent, title_id);
  std::string out;
  for (const auto& item : items) {
    if (!out.empty()) out += ", ";
    out += rex::string::to_utf8(item.display_name());
  }
  return out;
}

}  // namespace dlc
