#pragma once
#include <vector>
#include <fstream>
#include <algorithm>
#include <opencv2/opencv.hpp>
using namespace std;
using namespace cv;
#define sz(x) ((int)size(x))
#define all(x) begin(x),end(x)
template <typename T> using vec=vector<T>;
template <typename T> void vprint(T st, T nd) {auto it=st;while (next(it)!=nd){cout<<*it<<' ';it=next(it);}cout<<*it<<'\n';}

#define W 1080
#define H 1920
#define R1 228
#define R2 1682
#define FPS 30

// frame to time
inline float frm2t(int frame) {
    return (float)frame/FPS;
}

// time to frame
inline int t2frm(float t) {
    return t*FPS;
}

inline double wav_dur(const std::string &filename) {
    // Open the file in binary mode
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return -1;
    }

    // Read and verify the RIFF header
    char riff[4];
    file.read(riff, 4);
    if (std::strncmp(riff, "RIFF", 4) != 0) {
        std::cerr << "Not a valid RIFF file." << std::endl;
        return -1;
    }

    // Skip the next 4 bytes (overall file size) and read the WAVE header
    file.seekg(4, std::ios::cur);
    char wave[4];
    file.read(wave, 4);
    if (std::strncmp(wave, "WAVE", 4) != 0) {
        std::cerr << "Not a valid WAVE file." << std::endl;
        return -1;
    }

    // Read chunks until the "fmt " chunk is found.
    char chunkId[4];
    uint32_t chunkSize = 0;
    bool fmtFound = false;
    while (file.read(chunkId, 4)) {
        file.read(reinterpret_cast<char*>(&chunkSize), sizeof(chunkSize));
        if (std::strncmp(chunkId, "fmt ", 4) == 0) {
            fmtFound = true;
            break;
        }
        // Skip this chunk (its content) if not "fmt "
        file.seekg(chunkSize, std::ios::cur);
    }
    if (!fmtFound) {
        std::cerr << "\"fmt \" chunk not found." << std::endl;
        return -1;
    }

    // Read the fmt chunk data:
    // - audioFormat (2 bytes)
    // - numChannels (2 bytes)
    // - sampleRate (4 bytes)
    // - byteRate (4 bytes)
    // - blockAlign (2 bytes)
    // - bitsPerSample (2 bytes)
    uint16_t audioFormat, numChannels, blockAlign, bitsPerSample;
    uint32_t sampleRate, byteRate;
    file.read(reinterpret_cast<char*>(&audioFormat), sizeof(audioFormat));
    file.read(reinterpret_cast<char*>(&numChannels), sizeof(numChannels));
    file.read(reinterpret_cast<char*>(&sampleRate), sizeof(sampleRate));
    file.read(reinterpret_cast<char*>(&byteRate), sizeof(byteRate));
    file.read(reinterpret_cast<char*>(&blockAlign), sizeof(blockAlign));
    file.read(reinterpret_cast<char*>(&bitsPerSample), sizeof(bitsPerSample));
    
    // If fmt chunk has extra data, skip it
    if (chunkSize > 16) {
        file.seekg(chunkSize - 16, std::ios::cur);
    }

    // Now search for the "data" chunk, which holds the sound samples.
    bool dataFound = false;
    while (file.read(chunkId, 4)) {
        file.read(reinterpret_cast<char*>(&chunkSize), sizeof(chunkSize));
        if (std::strncmp(chunkId, "data", 4) == 0) {
            dataFound = true;
            break;
        }
        // Skip any extra chunks that are not "data"
        file.seekg(chunkSize, std::ios::cur);
    }
    if (!dataFound) {
        std::cerr << "\"data\" chunk not found." << std::endl;
        return -1;
    }

    // At this point, chunkSize contains the size (in bytes) of the actual audio data.
    uint32_t dataSize = chunkSize;

    // Calculate duration in seconds:
    // Duration = (data size in bytes) / (byte rate in bytes per second)
    double durationSeconds = static_cast<double>(dataSize) / byteRate;
    return durationSeconds;
}

inline string tts_preproc(string s) {
    replace(all(s),'\n',' ');
    return s;
}

inline double tts_dur(const string &s) {
    system(("espeak-ng \""+tts_preproc(s)+"\" -w out/ttsdur.wav").c_str());
    double res=wav_dur("out/ttsdur.wav");
    system("rm out/ttsdur.wav");
    return res;
}
