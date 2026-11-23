// NoteRain Native — Standard MIDI File loader
#pragma once

#include <filesystem>
#include <string>

#include "core/song.h"

namespace nr {

/** Load a .mid/.midi file. Returns false and fills `error` on failure. */
bool loadMidiFile(const std::filesystem::path& path, Song& out, std::string& error);

/** Build the built-in practice demo (C major study, two hands). */
Song makeDemoSong();

} // namespace nr
