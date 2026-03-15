//
// Created by jeser on 03/03/2026.
//

#ifndef UNTITLED_MUSIC_H
#define UNTITLED_MUSIC_H

#include <atomic>
#include <string>

#include "miniaudio.h"

struct SongData {
    ma_decoder decoder{};
    std::atomic<bool> playing;
    std::atomic<bool> paused;
    std::atomic<float> volume;
    std::atomic<bool> finished;

    std::atomic<ma_uint64> currentFrame{0};
    ma_uint64 totalFrames{0};

    std::string title;
    std::string artist;
};

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);

void playFolder(std::string& folderpath);

#endif //UNTITLED_MUSIC_H