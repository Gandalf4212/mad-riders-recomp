// Writes mad_riders.toml: the project banner, every setting the F1 menu offers
// (always listed, with a comment, even when it has its default value), then any
// other setting that was changed from its default (key binds etc.).
//
// ReXGlue's own SaveConfig only writes values that differ from the default, so
// the menu settings would otherwise be missing from the file until changed.
#pragma once

#include <filesystem>
#include <fstream>
#include <set>
#include <string>

#include <rex/cvar.h>
#include <rex/logging.h>

namespace recomp_ui {

inline constexpr const char* kConfigBanner =
    "#  __ __  ___  ___     ___  _  ___  ___  ___  ___ \n"
    "# |  \\  \\| . || . \\   | . \\| || . \\| __>| . \\/ __>\n"
    "# |     ||   || | |   |   /| || | || _> |   /\\__ \\\n"
    "# |_|_|_||_|_||___/   |_\\_\\|_||___/|___>|_\\_\\<___/\n"
    "#\n"
    "# Mad Riders recomp settings. Changed by the F1 menu; safe to edit by hand\n"
    "# while the game is closed. Command-line options (--name=value) override it.\n";

struct ConfigItem {
  const char* name;     // cvar
  const char* comment;  // shown above it; empty = none
};

// Sections of settings that are always written.
struct ConfigSection {
  const char* title;
  std::initializer_list<ConfigItem> items;
};

inline const ConfigSection kConfigSections[] = {
    {"Display",
     {
         {"fullscreen", "true = borderless fullscreen, false = windowed (F11 switches)"},
         {"window_width", "Window size in windowed mode (0 = 1280)"},
         {"window_height", ""},
         {"present_letterbox", "true = keep 16:9 with black bars, false = stretch"},
     }},
    {"Frame rate and performance",
     {
         {"recomp_frame_rate", "30 (original), 60, 120, or 0 = uncapped"},
         {"recomp_perf_overlay", "0 = off, 1 = FPS, 2 = FPS + frame-time graph"},
     }},
    {"Graphics",
     {
         {"resolution_scale", "Render resolution: 1 = 1280x720, 2 = 2560x1440, 3 = 3840x2160 "
                              "(2/3 also smooth edges)"},
         {"swap_post_effect", "Anti-aliasing: \"none\", \"fxaa\" or \"fxaa_extreme\""},
         {"anisotropic_override", "Texture filtering: -1 game default, 0 off, 2 = 2x, 3 = 4x, "
                                  "4 = 8x, 5 = 16x"},
     }},
    {"Audio",
     {
         {"audio_maxqframes", "Audio buffer frames: 8, 16, 32 or 64 (bigger fixes crackling)"},
     }},
    {"Controls",
     {
         {"mnk_mode", "true = keyboard plays as controller 1 (keys under Edit keyboard keys)"},
     }},
    {"Game",
     {
         {"recomp_full_game", "true = purchased game, false = trial (shows the ESRB notice)"},
         {"recomp_game_region", "255 = North America, 767 = Europe, 511 = Japan"},
     }},
};

inline std::string toml_value(const rex::cvar::FlagEntry& e, const std::string& v) {
  return e.type == rex::cvar::FlagType::String ? "\"" + v + "\"" : v;
}

inline void save_config(const std::filesystem::path& path) {
  if (path.empty()) return;
  // Derived from recomp_frame_rate at start-up; not worth listing.
  std::set<std::string> written = {"vsync", "video_mode_refresh_rate"};
  std::string out = std::string(kConfigBanner) + "\n";
  auto& registry = rex::cvar::GetRegistry();
  auto find = [&](const std::string& name) -> const rex::cvar::FlagEntry* {
    for (const auto& e : registry)
      if (e.name == name) return &e;
    return nullptr;
  };
  for (const auto& section : kConfigSections) {
    out += "# ---- " + std::string(section.title) + "\n";
    for (const auto& item : section.items) {
      const auto* e = find(item.name);
      if (!e) continue;
      if (item.comment && *item.comment) out += "# " + std::string(item.comment) + "\n";
      out += e->name + " = " + toml_value(*e, e->getter()) + "\n";
      written.insert(e->name);
    }
    out += "\n";
  }
  std::string other;
  for (const auto& e : registry) {
    if (written.count(e.name) || e.getter() == e.default_value) continue;
    other += e.name + " = " + toml_value(e, e.getter()) + "\n";
  }
  if (!other.empty()) out += "# ---- Other changed settings (key binds, internal)\n" + other;
  std::ofstream f(path, std::ios::binary | std::ios::trunc);
  if (!f) {
    REXLOG_ERROR("Could not write {}", path.string());
    return;
  }
  f << out;
}

}  // namespace recomp_ui
