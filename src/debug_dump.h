// Debug: F8 saves a snapshot of the game's memory, for finding decoded video
// frames (and other data) offline. Output goes to "memdump_<time>" next to the
// exe, split into files under 380 MB. Not shipped to players as a feature;
// harmless when unused.
#pragma once

#include <filesystem>

namespace rex::memory {
class Memory;
}

namespace debug_dump {

// Writes the snapshot synchronously. Returns the folder written, or empty on failure.
std::filesystem::path save_memory(rex::memory::Memory* memory, const std::filesystem::path& exe_dir);

}  // namespace debug_dump
