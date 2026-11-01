// Mad Riders keyboard layout (used by game_menu.cpp).
// Controller: left stick = steer / lean, RT = accelerate, LT = brake / reverse,
// X = boost. The other buttons are labelled by name until their in-game use is
// confirmed.
#pragma once

namespace recomp_ui::game_keys {

struct KeyBind {
  const char* cvar;
  const char* label;
  const char* default_key;
};

inline const KeyBind kKeyBinds[] = {
    {"keybind_lstick_up", "Lean forward (stick up)", "Up"},
    {"keybind_lstick_down", "Lean back (stick down)", "Down"},
    {"keybind_lstick_left", "Steer left (stick)", "Left"},
    {"keybind_lstick_right", "Steer right (stick)", "Right"},
    {"keybind_right_trigger", "Accelerate (RT)", "Z"},
    {"keybind_left_trigger", "Brake / reverse (LT)", "X"},
    {"keybind_x", "Boost (X)", "Space"},
    {"keybind_a", "A button", "C"},
    {"keybind_b", "B button", "V"},
    {"keybind_y", "Y button", "F"},
    {"keybind_left_shoulder", "LB", "Q"},
    {"keybind_right_shoulder", "RB", "W"},
    {"keybind_dpad_up", "D-pad up", "Up"},
    {"keybind_dpad_down", "D-pad down", "Down"},
    {"keybind_dpad_left", "D-pad left", "Left"},
    {"keybind_dpad_right", "D-pad right", "Right"},
    {"keybind_start", "Start / pause", "Return"},
    {"keybind_back", "Back", "Backspace"},
};

// Audio frames to buffer by default (SDK default is 8).
inline constexpr int kDefaultAudioFrames = 8;

inline const char* const kUnbound[] = {"keybind_lstick_press", "keybind_rstick_press",
                                       "keybind_guide"};

}  // namespace recomp_ui::game_keys
