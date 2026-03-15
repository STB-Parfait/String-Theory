//
// Created by jeser on 03/03/2026.
//

#ifndef UNTITLED_ID3_H
#define UNTITLED_ID3_H

#include <string>

#include "music.h"

bool loadID3v1Tag(const std::string& filepath, SongData& data);

bool loadID3v2Tag(const std::filesystem::path& filepath, SongData& data);

#endif //UNTITLED_ID3_H