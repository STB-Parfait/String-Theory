#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <atomic>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <conio.h>
#include <filesystem>
#include <fstream>

//song structure
struct SongData {
    ma_decoder decoder;
    std::atomic<bool> playing;
    std::atomic<bool> paused;
    std::atomic<float> volume;
    std::atomic<bool> finished;
    std::string title;
    std::string artist;
};

//clean blankspace in ID3
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

//clear the last 4 characters in a string
std::string simpleStringPolish(std::string* input) {
    std::string result;
    for (int i = 0; i < input->size() - 4; i++) {
        result += input->at(i);
    }
    return result;
}

//get v1 metadata
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

//get v2 metadata
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
        auto it = std::search(buffer.begin(), buffer.end(), frameID.begin(), frameID.end());
        if (it != buffer.end()) {
            auto textStart = it + 10;

            if (textStart < buffer.end()) {
                char encoding = *textStart;
                textStart++;

                if (encoding == 0 || encoding == 3) {
                    auto textEnd = textStart;
                    while (textEnd < buffer.end() && *textEnd != '\0') {
                        textEnd++;
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

// VERY IMPORTANT FUNCTION !!!
void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {

    auto* pSongData = (SongData*)pDevice->pUserData;

    if (pSongData->playing.load()) {
        ma_uint64 framesRead = 0;

        ma_decoder_read_pcm_frames(&pSongData->decoder, pOutput, frameCount, &framesRead);

        if (framesRead < frameCount) {
            pSongData->finished.store(true);
        }

        float* pFloatOutput = (float*)pOutput;
        float currentVolume = pSongData->volume.load();

        ma_uint32 totalSamplesRead = framesRead * pSongData->decoder.outputChannels;
        for (ma_uint32 i = 0; i < totalSamplesRead; i++) {
            pFloatOutput[i] *= currentVolume;
        }

        if (framesRead < frameCount) {
            ma_uint32 framesRemaining = frameCount - framesRead;
            size_t emptyBytes = framesRemaining * pDevice->playback.channels * sizeof(float);

            memset(pFloatOutput + totalSamplesRead, 0, emptyBytes);
        }
    } else {
        size_t bufferSizeInBytes = frameCount * pDevice->playback.channels * sizeof(float);
        memset(pOutput, 0, bufferSizeInBytes);
    }

}

//convert wstring into std::string
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

//get all .mp3 in a folder
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

void play(std::string& folderpath) {

    //customize terminal
    std::cout << "\033[?25l";                //hide cursor
    std::cout << "\033]0;String-Theory\007"; //window name
    std::cout << "\033[8;2;60t";             //window size

    //set UTF-8 characters for display on console
    SetConsoleOutputCP(CP_UTF8);
    std::setlocale(LC_ALL, ".UTF8");

    //keyboard input references
    const int ENTER = 13;
    const int SPACEBAR = 32;
    const int UP = 72;
    const int DOWN = 80;
    const int LEFT = 75;
    const int RIGHT = 77;
    const int EXTENTION_IDENTIFIER_A = 0;
    const int EXTENTION_IDENTIFIER_B = 224;

    std::vector<std::string> playlist = scrapeFolder(folderpath);

    int currentTrackIndex = 0;

    SongData song; //Song struct
    song.volume = 1.0f;
    song.playing = true;
    song.finished = false;
    song.paused = false;

    //setup encoder config
    ma_decoder_config decoder_config = ma_decoder_config_init(ma_format_f32, 0, 0);

    //check if it worked
    if (ma_decoder_init_file(playlist[currentTrackIndex].c_str(), &decoder_config, &song.decoder) != MA_SUCCESS) {
        std::cout << "Failed to load the audio file. Check the path!" << std::endl;
        return;
    }

    //get metadata for 1st song
    loadID3v2Tag(playlist[currentTrackIndex], song);

    //setup audio configs
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = song.decoder.outputFormat;
    config.playback.channels = song.decoder.outputChannels;
    config.sampleRate = song.decoder.outputSampleRate;
    config.dataCallback = data_callback;
    config.pUserData = &song;

    ma_device device;
    if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
        std::cout << "Failed to initialize playback device." << std::endl;
        ma_decoder_uninit(&song.decoder);
        return;
    }

    ma_device_start(&device); //start music

    std::cout << "[" << currentTrackIndex + 1  << "/"  << playlist.size() << "] Playing " << song.title << " by " << song.artist << " <volume: " << (song.volume * 100) << ">" << std::endl;

    //playlist loop
    while (currentTrackIndex < static_cast<int>(playlist.size())) {

        //handle song finish
        if (song.finished.load()) {
            currentTrackIndex++;

            if (currentTrackIndex >= playlist.size()) {
                system("cls");
                std::cout << "Finished!" << std::endl;
                break;
            }

            loadID3v2Tag(playlist[currentTrackIndex], song);
            system("cls");
            std::cout << "[" << currentTrackIndex + 1  << "/"  << playlist.size() << "] Playing " << song.title << " by " << song.artist << " <volume: " << (song.volume * 100) << ">" << std::endl;

            ma_device_stop(&device);

            ma_decoder_uninit(&song.decoder);

            ma_decoder_init_file(playlist[currentTrackIndex].c_str(), &decoder_config, &song.decoder);

            song.finished.store(false);
            ma_device_start(&device);
        }

        //input block
        if (_kbhit()) {
            int key = _getch();

            if (key == ENTER) {
                system("cls");
                std::cout << "Exiting..." << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                break;
            } else if (key == SPACEBAR) {
                if (!song.paused.load()) {
                    system("cls");
                    std::cout << "<paused>" << std::endl;
                    ma_device_stop(&device);
                    song.paused.store(true);
                } else {
                    system("cls");
                    std::cout << "[" << currentTrackIndex + 1  << "/"  << playlist.size() << "] Playing " << song.title << " by " << song.artist << " <volume: " << (song.volume * 100) << ">" << std::endl;
                    ma_device_start(&device);
                    song.paused.store(false);
                }
            } else if (key == EXTENTION_IDENTIFIER_A || key == EXTENTION_IDENTIFIER_B) {
                int extendedKey = _getch();
                if (extendedKey == LEFT) {
                    ma_uint64 cursor;
                    ma_decoder_get_cursor_in_pcm_frames(&song.decoder, &cursor);
                    double currentSecond = (double)cursor / song.decoder.outputSampleRate;
                    if (currentSecond > 2.0) {
                        ma_device_stop(&device);
                        ma_decoder_seek_to_pcm_frame(&song.decoder, 0);
                        ma_device_start(&device);
                    } else if (currentSecond <= 2 && currentTrackIndex != 0){
                        system("cls");
                        currentTrackIndex -= 2;
                        song.finished.store(true);
                    }
                } else if (extendedKey == RIGHT) {
                    system("cls");
                    song.finished.store(true);
                } else if (extendedKey == UP) {
                    if (song.volume < 1.0f) {
                        song.volume += 0.1f;
                        system("cls");
                        std::cout << "[" << currentTrackIndex + 1  << "/"  << playlist.size() << "] Playing " << song.title << " by " << song.artist << " <volume: " << (song.volume * 100) << ">" << std::endl;
                    }
                } else if (extendedKey == DOWN) {
                    if (song.volume > 0.0f) {
                        float vol = song.volume.load();
                        vol -= 0.1f;
                        if (vol < 0.05) { vol = 0.0f; }
                        vol = std::clamp(vol, 0.0f, 1.0f);
                        song.volume.store(vol);
                        system("cls");
                        std::cout << "[" << currentTrackIndex + 1  << "/"  << playlist.size() << "] Playing " << song.title << " by " << song.artist << " <volume: " << (song.volume * 100) << ">" << std::endl;
                    }
                }
            }
        }

        if (!song.paused.load()) {
            ma_uint64 cursorFrames, totalFrames;

            ma_decoder_get_cursor_in_pcm_frames(&song.decoder, &cursorFrames);
            ma_decoder_get_length_in_pcm_frames(&song.decoder, &totalFrames);

            int currentSeconds = (int)(cursorFrames / song.decoder.outputSampleRate);
            int totalSeconds = (int)(totalFrames / song.decoder.outputSampleRate);

            int curMin = currentSeconds / 60;
            int curSec = currentSeconds % 60;
            int totMin = totalSeconds / 60;
            int totSec = totalSeconds % 60;

            printf("\r[%02d:%02d/%02d:%02d] ", curMin, curSec, totMin, totSec);
            fflush(stdout);
        }

        //sleep between songs
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    //uninitialize device
    ma_device_uninit(&device);
    ma_decoder_uninit(&song.decoder);

}

int main() {
    std::string folder = "C:/Users/jeser/Music/Offline-MP3/tracks/Full";
    play(folder);
    return 0;
}