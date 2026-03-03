//
// Created by jeser on 03/03/2026.
//

#include <fstream>
#include <string>

#include "music.h"
#include "tools.h"
#include "id3.h"

bool loadID3v1Tag(const std::string& filepath, SongData& data) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) return false;

    file.seekg(-128, std::ios::end);

    char buffer[128];
    file.read(buffer, 128);

    if (std::string(buffer, 3) == "TAG") {
        data.title = cleanID3String(buffer + 3, 30);
        data.artist = cleanID3String(buffer + 33, 30);
        return true;
    }

    data.title = filepath;
    data.artist = "Unknown";
    return false;
}

bool loadID3v2Tag(const std::string& filepath, SongData& data) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) return false;

    char header[10];
    file.read(header, 10);
    if (std::string(header, 3) != "ID3") {
        data.title = filepath;
        data.artist = "Unknown";
        return false;
    }

    std::vector<char> buffer(8192);
    file.read(buffer.data(), buffer.size());

    auto findTag = [&](const std::string& frameID) -> std::string {
        auto it = std::ranges::search(buffer, frameID).begin();
        if (it != buffer.end()) {
            auto textStart = it + 10;

            if (textStart < buffer.end()) {
                char encoding = *textStart;
                ++textStart;

                if (encoding == 0 || encoding == 3) {
                    auto textEnd = textStart;
                    while (textEnd < buffer.end() && *textEnd != '\0') {
                        ++textEnd;
                    }
                    return std::string(textStart, textEnd);
                } else {
                    return "[UTF-16 Encoded - Cannot Read Manually]";
                }
            }
        }
        return "";
    };

    std::string title = findTag("TIT2");
    std::string artist = findTag("TPE1");

    bool foundAnything = false;
    if (!title.empty()) { data.title = simpleStringPolish(&title); foundAnything = true; }
    if (!artist.empty()) { data.artist = simpleStringPolish(&artist); foundAnything = true; }

    return foundAnything;
}
