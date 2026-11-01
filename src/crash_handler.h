// Crash reporting for test builds.
//
// On an unhandled exception (Windows): logs the exception, the faulting
// function and a stack trace (recompiled functions are named sub_<guest
// address>, so the trace points straight at the game code), writes
// mad_riders_crash.dmp next to the exe, flushes the log and tells the player.
// Needs mad_riders.pdb next to the exe for function names.
#pragma once

#include <filesystem>

namespace crash_handler {

// Safe to call more than once (later calls re-assert our filter in case a
// library replaced it).
void install(const std::filesystem::path& exe_dir);

}  // namespace crash_handler
