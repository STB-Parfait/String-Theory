//
// Created by jeser on 03/03/2026.
//

#ifndef UNTITLED_TOOLS_H
#define UNTITLED_TOOLS_H

#include <string>
#include <vector>
#include <filesystem>
#include <filesystem>

std::string cleanID3String(const char* buffer, size_t length);

std::string simpleStringPolish(const std::string* input);

std::string convWideString(const std::wstring& wideString);

std::vector<std::filesystem::path> scrapeFolder(const std::string &absolutepath);

#endif //UNTITLED_TOOLS_H