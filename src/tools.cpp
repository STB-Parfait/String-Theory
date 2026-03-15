//
// Created by jeser on 03/03/2026.
//

#include "tools.h"

#include <filesystem>
#include <iostream>

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
    if (input->size() <= 4) return *input;
    return input->substr(0, input->size() - 4);
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

std::vector<std::filesystem::path> scrapeFolder(const std::string &absolutepath) {
    std::vector<std::filesystem::path> playlist;

    try {
        if (!std::filesystem::exists(absolutepath) || !std::filesystem::is_directory(absolutepath)) {
            std::cerr << "ERROR: Folder does not exist." << std::endl;
            return {};
        }

        for (const auto& entry : std::filesystem::directory_iterator(absolutepath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".mp3") {

                //legacy implementation
                //std::wstring widepath = std::filesystem::path(entry.path()).wstring();
                //playlist.push_back(convWideString(widepath));
                playlist.push_back(entry.path());
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << e.what() << std::endl;
        return {};
    }

    return playlist;
}