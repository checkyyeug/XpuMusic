#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <cmath>
#include <thread>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#endif

// WAV file header
struct WAVHeader {
    char riff[4];
    uint32_t size;
    char wave[4];
    char fmt[4];
    uint32_t fmt_size;
    uint16_t format;
    uint16_t channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits;
    char data[4];
    uint32_t data_size;
};

bool play_wav_native(const char* filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return false;
    }

    WAVHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));

    // Verify WAV format
    if (strncmp(header.riff, "RIFF", 4) != 0 ||
        strncmp(header.wave, "WAVE", 4) != 0 ||
        strncmp(header.fmt, "fmt ", 4) != 0) {
        std::cerr << "Error: Invalid WAV file format" << std::endl;
        return false;
    }

    // Skip to data chunk
    while (strncmp(header.data, "data", 4) != 0 && !file.eof()) {
        file.seekg(header.data_size - 16, std::ios::cur);
        file.read(header.data, 8);
        file.read(reinterpret_cast<char*>(&header.data_size), 4);
    }

    if (strncmp(header.data, "data", 4) != 0) {
        std::cerr << "Error: Data chunk not found" << std::endl;
        return false;
    }

    // Read audio data
    std::vector<char> audio_data(header.data_size);
    file.read(audio_data.data(), header.data_size);

    std::cout << "Playing: " << filename << std::endl;
    std::cout << "Format: " << header.sample_rate << " Hz, "
              << header.bits << "-bit, " << header.channels << " channels" << std::endl;
    std::cout << "Duration: " << (header.data_size / (header.sample_rate * header.channels * header.bits / 8))
              << " seconds" << std::endl;

#ifdef _WIN32
    // Play with Windows API
    WAVEFORMATEX wf;
    wf.wFormatTag = WAVE_FORMAT_PCM;
    wf.nChannels = header.channels;
    wf.nSamplesPerSec = header.sample_rate;
    wf.wBitsPerSample = header.bits;
    wf.nBlockAlign = wf.nChannels * wf.wBitsPerSample / 8;
    wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;
    wf.cbSize = 0;

    HWAVEOUT hWaveOut;
    if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &wf, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) {
        std::cerr << "Error: Cannot open audio device" << std::endl;
        return false;
    }

    // Prepare and play in chunks
    const int CHUNK_SIZE = 65536;
    WAVEHDR wh;
    ZeroMemory(&wh, sizeof(wh));
    wh.lpData = new char[CHUNK_SIZE];
    wh.dwBufferLength = CHUNK_SIZE;

    size_t bytes_played = 0;
    while (bytes_played < audio_data.size()) {
        size_t chunk = (std::min<size_t>)(CHUNK_SIZE, audio_data.size() - bytes_played);
        memcpy(wh.lpData, audio_data.data() + bytes_played, chunk);
        wh.dwBufferLength = chunk;
        wh.dwFlags = 0;

        if (waveOutPrepareHeader(hWaveOut, &wh, sizeof(wh)) != MMSYSERR_NOERROR) {
            std::cerr << "Error: Cannot prepare audio buffer" << std::endl;
            break;
        }

        if (waveOutWrite(hWaveOut, &wh, sizeof(wh)) != MMSYSERR_NOERROR) {
            std::cerr << "Error: Cannot write audio data" << std::endl;
            break;
        }

        // Wait for chunk to finish playing
        while (!(wh.dwFlags & WHDR_DONE)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        waveOutUnprepareHeader(hWaveOut, &wh, sizeof(wh));
        bytes_played += chunk;

        // Progress indicator
        int progress = (int)(100 * bytes_played / audio_data.size());
        std::cout << "\rProgress: " << progress << "%" << std::flush;
    }

    delete[] wh.lpData;
    waveOutClose(hWaveOut);
    std::cout << "\nPlayback completed!" << std::endl;

#else
    // Linux implementation would go here
    std::cout << "Linux implementation not available" << std::endl;
    return false;
#endif

    return true;
}

int main(int argc, char* argv[]) {
    const char* filename = (argc > 1) ? argv[1] : "test_1khz.wav";

    std::cout << "========================================" << std::endl;
    std::cout << "   FINAL WAV PLAYER - Native Format    " << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    std::cout << "File: " << filename << std::endl;

    if (!play_wav_native(filename)) {
        std::cerr << "Failed to play: " << filename << std::endl;
        return 1;
    }

    return 0;
}