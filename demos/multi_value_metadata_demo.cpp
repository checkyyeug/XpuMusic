/**
 * @file multi_value_metadata_demo.cpp
 * @brief Demo: Multi-value metadata functionality
 * @date 2025-12-09
 */

#include <iostream>
#include <vector>
#include "../compat/sdk_implementations/file_info_impl.h"
#include "../compat/sdk_implementations/common_includes.h"

using namespace xpumusic_sdk;

void print_separator(const char* title) {
    std::cout << "\n========================================\n";
    std::cout << title << "\n";
    std::cout << "========================================\n";
}

void demo_basic_metadata() {
    print_separator("Demo 1: Basic Metadata Operations");
    
    auto info = file_info_create();
    
    // Set single values (replaces existing)
    info->meta_set("title", "Bohemian Rhapsody");
    info->meta_set("album", "A Night at the Opera");
    info->meta_set("year", "1975");
    
    std::cout << "Title: " << info->meta_get("title", 0) << "\n";
    std::cout << "Album: " << info->meta_get("album", 0) << "\n";
    std::cout << "Year: " << info->meta_get("year", 0) << "\n";
}

void demo_multi_value_artists() {
    print_separator("Demo 2: Multi-Value Artists");
    
    auto info = file_info_create();
    
    // Add multiple artists (appends)
    info->meta_add("artist", "Queen");
    info->meta_add("artist", "Freddie Mercury");
    info->meta_add("artist", "Brian May");
    
    size_t artist_count = info->meta_get_count("artist");
    std::cout << "Number of artists: " << artist_count << "\n";
    
    std::cout << "Artists:\n";
    for (size_t i = 0; i < artist_count; ++i) {
        std::cout << "  [" << i << "] " << info->meta_get("artist", i) << "\n";
    }
}

void demo_genre_tags() {
    print_separator("Demo 3: Multiple Genres");
    
    auto info = file_info_create();
    
    // A song can belong to multiple genres
    info->meta_add("genre", "Rock");
    info->meta_add("genre", "Progressive Rock");
    info->meta_add("genre", "Classic Rock");
    
    size_t genre_count = info->meta_get_count("genre");
    std::cout << "Genres (" << genre_count << "):\n";
    
    for (size_t i = 0; i < genre_count; ++i) {
        std::cout << "  - " << info->meta_get("genre", i) << "\n";
    }
}

void demo_metadata_comparison() {
    print_separator("Demo 4: Metadata Comparison");
    
    auto info1 = file_info_create();
    auto info2 = file_info_create();
    
    info1->meta_add("artist", "The Beatles");
    info1->meta_add("artist", "Paul McCartney");
    
    info2->meta_add("artist", "The Beatles");
    info2->meta_add("artist", "Paul McCartney");
    
    std::cout << "info1 == info2: " << (info1->meta_equals(*info2) ? "YES" : "NO") << "\n";
    
    info2->meta_add("artist", "John Lennon");
    std::cout << "After adding John Lennon:\n";
    std::cout << "info1 == info2: " << (info1->meta_equals(*info2) ? "YES" : "NO") << "\n";
}

void demo_copy_and_merge() {
    print_separator("Demo 5: Copy and Merge");
    
    auto source = file_info_create();
    source->meta_add("artist", "Source Artist");
    source->meta_add("genre", "Source Genre");
    
    auto dest = file_info_create();
    dest->meta_add("artist", "Dest Artist");
    dest->meta_add("album", "Dest Album");
    
    std::cout << "Before merge:\n";
    std::cout << "  Dest artists: " << dest->meta_get_count("artist") << "\n";
    std::cout << "  Dest albums: " << dest->meta_get_count("album") << "\n";
    std::cout << "  Dest genres: " << dest->meta_get_count("genre") << "\n";
    
    dest->merge_from(*source);
    
    std::cout << "\nAfter merge:\n";
    std::cout << "  Dest artists: " << dest->meta_get_count("artist") << " (merged)\n";
    std::cout << "  Dest albums: " << dest->meta_get_count("album") << " (kept)\n";
    std::cout << "  Dest genres: " << dest->meta_get_count("genre") << " (added)\n";
}

void demo_field_normalization() {
    print_separator("Demo 6: Field Name Normalization");
    
    auto info = file_info_create();
    
    // These should all map to the same field
    info->meta_set("ARTIST", "Test Artist 1");
    info->meta_set("artist", "Test Artist 2");
    info->meta_set("Artist", "Test Artist 3");
    info->meta_set("  Artist  ", "Test Artist 4");
    
    std::cout << "Artist count (normalized): " << info->meta_get_count("artist") << "\n";
    std::cout << "Final value: " << info->meta_get("artist", 0) << "\n";
    
    std::cout << "\nDate/year aliasing:\n";
    info->meta_set("date", "2024");
    std::cout << "date->meta_get(\"year\"): " << info->meta_get("year", 0) << "\n";
}

void demo_enum_all_fields() {
    print_separator("Demo 7: Enumerate All Fields");
    
    auto info = file_info_create();
    
    info->meta_add("artist", "Artist 1");
    info->meta_add("artist", "Artist 2");
    info->meta_add("title", "Song Title");
    info->meta_add("album", "Album Name");
    info->meta_add("genre", "Rock");
    info->meta_add("genre", "Pop");
    
    auto fields = info->meta_enum_field_names();
    std::cout << "Total fields: " << fields.size() << "\n\n";
    
    for (const auto& field : fields) {
        size_t count = info->meta_get_count(field.c_str());
        std::cout << field << " (" << count << " value" << (count > 1 ? "s" : "") << "):\n";
        
        for (size_t i = 0; i < count; ++i) {
            std::cout << "  [" << i << "] \"" << info->meta_get(field.c_str(), i) << "\"\n";
        }
        std::cout << "\n";
    }
}

void demo_audio_info() {
    print_separator("Demo 8: Audio Information");
    
    auto info = file_info_create();
    
    // Set audio parameters
    audio_info_impl audio_params;
    audio_params.m_sample_rate = 44100;
    audio_params.m_channels = 2;
    audio_params.m_bitrate = 320000;
    audio_params.m_length = 225.5;  // 3:45.5
    
    info->set_audio_info(audio_params);
    
    const auto& retrieved = info->get_audio_info();
    std::cout << "Sample Rate: " << retrieved.m_sample_rate << " Hz\n";
    std::cout << "Channels: " << retrieved.m_channels << "\n";
    std::cout << "Bitrate: " << retrieved.m_bitrate << " bps\n";
    std::cout << "Duration: " << retrieved.m_length << " seconds\n";
}

int main() {
    std::cout << "\n鈺斺晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晽\n";
    std::cout << "鈺?    XpuMusic Multi-Value Metadata SDK Demo                鈺慭n";
    std::cout << "鈺氣晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨暆\n";
    
    demo_basic_metadata();
    demo_multi_value_artists();
    demo_genre_tags();
    demo_metadata_comparison();
    demo_copy_and_merge();
    demo_field_normalization();
    demo_enum_all_fields();
    demo_audio_info();
    
    print_separator("Demo Complete");
    std::cout << "\n鉁?All multi-value metadata features working!\n\n";
    
    return 0;
}
