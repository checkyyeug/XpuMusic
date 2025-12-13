/**
 * @file foobar2000_modern_loader.h
 * @brief Modern foobar2000 DLL loader for music-player.exe
 * @date 2025-12-13
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>

#ifdef _WIN32
#include <windows.h>
#endif

// Forward declarations
struct AudioFormat {
    uint32_t sample_rate;
    uint16_t channels;
    uint16_t bits_per_sample;
};

/**
 * @brief Modern foobar2000 DLL loader
 *
 * This class handles loading and interfacing with modern foobar2000 components
 * (shared.dll, avcodec-fb2k-62.dll, etc.) to provide extended format support.
 */
class Foobar2000ModernLoader {
public:
    // Decoder interface
    class Decoder {
    public:
        virtual ~Decoder() = default;
        virtual bool open() = 0;
        virtual bool read(char* buffer, size_t size, size_t& bytes_read) = 0;
        virtual void close() = 0;
        virtual AudioFormat get_format() const = 0;
    };

    using DecoderPtr = std::unique_ptr<Decoder>;

    Foobar2000ModernLoader();
    ~Foobar2000ModernLoader();

    // Initialize the loader and load foobar2000 components
    bool initialize();

    // Check if a format is supported by foobar2000
    bool can_decode_format(const std::string& extension) const;

    // Create a decoder for a specific file
    DecoderPtr create_decoder(const std::string& filename);

    // Check if foobar2000 is available
    bool is_initialized() const { return initialized_; }

    // Shutdown and unload all components
    void shutdown();

private:
    bool load_modern_components();
    static std::string get_extension(const std::string& filename);

    bool initialized_;
    std::vector<std::pair<std::string, void*>> loaded_dlls_; // name -> handle

  // Windows-specific handles
#ifdef _WIN32
    void* shared_lib_;
#endif
};

/**
 * @brief Wrapper for foobar2000 decoder
 */
class Foobar2000DecoderWrapper : public Foobar2000ModernLoader::Decoder {
public:
    Foobar2000DecoderWrapper(const std::string& filename,
                            Foobar2000ModernLoader* loader);

    bool open() override;
    bool read(char* buffer, size_t size, size_t& bytes_read) override;
    void close() override;
    AudioFormat get_format() const override;

private:
    std::string filename_;
    Foobar2000ModernLoader* loader_;
};