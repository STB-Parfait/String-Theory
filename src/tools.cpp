//
// Created by jeser on 03/03/2026.
//

#include "tools.h"

#include <filesystem>

std::string cleanID3String(const char* buffer, size_t length) {
    std::string s(buffer, length);

    size_t endpos = s.find_last_not_of('\0');
    if (endpos != std::string::npos) {
        s = s.substr(0, endpos + 1);
    } else {
        s.clear();
    }
    return s;
}

std::string simpleStringPolish(const std::string* input) {
    std::string result;
    for (int i = 0; i < input->size() - 4; i++) {
        result += input->at(i);
    }
    return result;
}

std::string convWideString(const std::wstring& wideString) {
    if (wideString.empty()) return "";

    size_t len = wcstombs(nullptr, wideString.c_str(), 0);

    if (len == static_cast<size_t>(-1)) {
        return "Encoding error.";
    }

    std::vector<char> buffer(len + 1);

    wcstombs(buffer.data(), wideString.c_str(), buffer.size());

    std::string result = buffer.data();

    return result;

}

std::vector<std::string> scrapeFolder(const std::string& absolutepath) {
    std::vector<std::string> playlist;

    try {
        if (!std::filesystem::exists(absolutepath) || !std::filesystem::is_directory(absolutepath)) {
            return {"ERROR: Folder does not exist or is not directory."};
        }

        for (const auto& entry : std::filesystem::directory_iterator(absolutepath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".mp3") {
                std::wstring widepath = std::filesystem::path(entry.path()).wstring();
                playlist.push_back(convWideString(widepath));
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        return {"ERROR: " + std::string(e.what())};
    }

    if (playlist.empty()) {
        return {"ERROR: No .mp3 files found."};
    }

    return playlist;
}