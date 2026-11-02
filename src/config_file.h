// Saves mad_riders.toml through ReXGlue's cvar system, with the project banner on top.
#pragma once

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include <rex/cvar.h>

namespace recomp_ui {

inline constexpr const char* kConfigBanner =
    "#  __ __  ___  ___     ___  _  ___  ___  ___  ___ \n"
    "# |  \\  \\| . || . \\   | . \\| || . \\| __>| . \\/ __>\n"
    "# |     ||   || | |   |   /| || | || _> |   /\\__ \\\n"
    "# |_|_|_||_|_||___/   |_\\_\\|_||___/|___>|_\\_\\<___/\n"
    "#\n"
    "# Mad Riders recomp settings. Edited by the F1 menu; safe to edit by hand.\n";

inline void save_config(const std::filesystem::path& path) {
  if (path.empty()) return;
  rex::cvar::SaveConfig(path);
  std::ifstream in(path, std::ios::binary);
  if (!in) return;
  std::stringstream ss;
  ss << in.rdbuf();
  in.close();
  std::string body = ss.str();
  const std::string old_header = "# Auto-generated cvar configuration\n";
  if (body.rfind(old_header, 0) == 0) body.erase(0, old_header.size());
  if (body.rfind(kConfigBanner, 0) == 0) return;
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  out << kConfigBanner << "\n" << body;
}

}  // namespace recomp_ui
