// mad_riders - ReXGlue Recompiled Project
//
// App hooks: first-run setup from the player's package, GPU plugin,
// full-game license, first-launch defaults, and the F1/Esc settings menu.
// Structure follows afterburner-climax-recomp (MIT, (c) 2026 darkuni).

#pragma once

#include <cstdlib>
#include <filesystem>
#include <string>

#include <rex/cvar.h>
#include <rex/filesystem.h>
#include <rex/input/input_system.h>
#include <rex/logging.h>
#include <rex/rex_app.h>
#include <rex/runtime.h>
#include <rex/ui/keybinds.h>

#include <imgui.h>

#include "crash_handler.h"
#include "game_keys.h"
#include "game_menu.h"
#include "package_setup.h"

#ifndef MADRIDERS_BUILD
#define MADRIDERS_BUILD "local"
#endif

// The default.xex this build was recompiled from (Mad Riders XBLA, 0.0.0.2).
inline constexpr package_setup::GameInfo kMadRiders{
    "Mad Riders", 0x58411240u, "90349e4daf4812f27faf5c0a9e01edb5ff7d5219", 21446656u};

class MadRidersApp : public rex::ReXApp {
 public:
  using rex::ReXApp::ReXApp;

  static std::unique_ptr<rex::ui::WindowedApp> Create(
      rex::ui::WindowedAppContext& ctx) {
    return std::unique_ptr<MadRidersApp>(new MadRidersApp(ctx, "mad_riders",
        PPCImageConfig));
  }

  // Game data lives in "assets" next to the exe. On the first start it is made
  // from the player's own XBLA package (found automatically or picked in a
  // file dialog). --game_data_root still overrides it.
  void OnConfigurePaths(rex::PathConfig& paths) override {
    const auto exe_dir = rex::filesystem::GetExecutableFolder();
    crash_handler::install(exe_dir);
    if (paths.game_data_root.empty()) {
      paths.game_data_root = exe_dir / "assets";
    }
    if (!package_setup::ensure_assets(exe_dir, paths.game_data_root, kMadRiders)) {
      std::exit(1);
    }
    config_path_ = paths.config_path;
  }

  void OnPreSetup(rex::RuntimeConfig& config) override {
    REXLOG_INFO("Mad Riders recomp, build {}", MADRIDERS_BUILD);
    // GPU emulation is opt-in in ReXGlue; default to the Xenos plugin.
    if (config.gpu_plugin.empty()) {
      config.gpu_plugin = "xenos";
    }
    // XBLA titles run as the trial unless the license mask says purchased
    // (bit 0 = full game). Unlock unless the user set license_mask themselves.
    if (rex::cvar::Query<uint32_t>("license_mask") == 0) {
      rex::cvar::SetFlagByName("license_mask", "1");
      REXLOG_INFO("License: full game (license_mask=1)");
    }
    // First launch: windowed 1280x720, keyboard on, racing key layout.
    // After this everything comes from mad_riders.toml (edited by the F1 menu).
    if (!rex::cvar::Query<bool>("recomp_defaults_applied")) {
      rex::cvar::SetFlagByName("fullscreen", "false");
      rex::cvar::SetFlagByName("window_width", "1280");
      rex::cvar::SetFlagByName("window_height", "720");
      rex::cvar::SetFlagByName("mnk_mode", "true");
      rex::cvar::SetFlagByName("audio_maxqframes",
                               std::to_string(recomp_ui::game_keys::kDefaultAudioFrames));
      recomp_ui::GameMenu::ApplyDefaultKeys();
      rex::cvar::SetFlagByName("recomp_defaults_applied", "true");
      if (!config_path_.empty()) {
        rex::cvar::SaveConfig(config_path_);
      }
      REXLOG_INFO("First launch: wrote default settings to {}",
                  config_path_.filename().string());
    }
    // Defaults added by later builds, applied once on top of existing settings.
    if (rex::cvar::Query<int32_t>("recomp_defaults_version") < 2) {
      // Write the log out every second so the lines before a crash aren't lost.
      rex::cvar::SetFlagByName("log_flush_interval", "1");
      rex::cvar::SetFlagByName("recomp_defaults_version", "2");
      if (!config_path_.empty()) rex::cvar::SaveConfig(config_path_);
    }
  }

  void OnCreateDialogs(rex::ui::ImGuiDrawer* drawer) override {
    (void)drawer;
    rex::ui::RegisterBind("bind_menu", "F1", "Open/close the settings menu",
                          [this] { ToggleMenu(); });
    rex::ui::RegisterBind("bind_menu_alt", "Escape", "Open/close the settings menu",
                          [this] { ToggleMenu(); });
    rex::ui::RegisterBind("bind_fullscreen", "F11", "Toggle fullscreen", [this] {
      if (menu_ && menu_->capturing()) return;
      bool fs = !window()->IsFullscreen();
      rex::cvar::SetFlagByName("fullscreen", fs ? "true" : "false");
      window()->SetFullscreen(fs);
      if (!config_path_.empty()) rex::cvar::SaveConfig(config_path_);
    });
  }

  // The game must not see keys/buttons while the menu is open.
  void OnPostSetup() override {
    // Re-assert the crash filter in case SDL or a driver replaced it.
    crash_handler::install(rex::filesystem::GetExecutableFolder());
    auto* input_sys = static_cast<rex::input::InputSystem*>(runtime()->input_system());
    if (input_sys && imgui_drawer()) {
      input_sys->SetActiveCallback([this]() {
        if (menu_) return false;
        return !imgui_drawer()->GetIO().WantCaptureMouse;
      });
    }
  }

 private:
  void ToggleMenu() {
    if (menu_) {
      if (menu_->capturing()) return;  // Esc cancels the key capture instead
      delete menu_;                    // removes itself from the drawer
      menu_ = nullptr;
      return;
    }
    if (!imgui_drawer()) return;
    recomp_ui::MenuHost host;
    host.title = "MAD RIDERS";
    host.config_path = config_path_;
    host.set_fullscreen = [this](bool fs) {
      app_context().CallInUIThreadDeferred([this, fs] { window()->SetFullscreen(fs); });
    };
    host.quit = [this] {
      app_context().CallInUIThreadDeferred([this] { window()->RequestClose(); });
    };
    host.on_closed = [this] { menu_ = nullptr; };
    menu_ = new recomp_ui::GameMenu(imgui_drawer(), std::move(host));
  }

  std::filesystem::path config_path_;
  recomp_ui::GameMenu* menu_ = nullptr;  // owned by the ImGui drawer
};
