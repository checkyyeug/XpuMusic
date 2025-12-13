/**
 * @file playlist_demo.cpp
 * @brief Demonstration of playlist management features
 * @date 2025-12-13
 */

#include "../playlist/playlist_manager.h"
#include <iostream>
#include <iomanip>

using namespace xpumusic::playlist;

void print_current_track(const PlaylistManager& playlist) {
    if (playlist.get_track_count() == 0) {
        std::cout << "Playlist is empty" << std::endl;
        return;
    }

    const auto& track = playlist.get_current_track();
    std::cout << "\nNow playing: ";
    std::cout << track.title;
    if (!track.artist.empty()) {
        std::cout << " - " << track.artist;
    }
    std::cout << " (" << std::fixed << std::setprecision(2) << track.duration << "s)";
    std::cout << " [" << track.path << "]";
    std::cout << std::endl;
}

void print_playlist(const PlaylistManager& playlist) {
    std::cout << "\nPlaylist (" << playlist.get_track_count() << " tracks, ";
    std::cout << std::fixed << std::setprecision(1) << playlist.get_total_duration() / 60;
    std::cout << " minutes):\n";

    for (size_t i = 0; i < playlist.get_track_count(); ++i) {
        const auto& track = playlist.get_track(i);
        std::cout << (i == playlist.get_current_index() ? "▶ " : "  ");
        std::cout << std::setw(2) << (i + 1) << ". ";
        std::cout << std::setw(30) << std::left << track.title.substr(0, 29);
        if (!track.artist.empty()) {
            std::cout << " - " << track.artist.substr(0, 20);
        }
        std::cout << "\n";
    }
}

std::string playback_mode_to_string(PlaybackMode mode) {
    switch (mode) {
        case PlaybackMode::Sequential: return "Sequential";
        case PlaybackMode::Random: return "Random";
        case PlaybackMode::RepeatOne: return "Repeat One";
        case PlaybackMode::RepeatAll: return "Repeat All";
        case PlaybackMode::Shuffle: return "Shuffle";
        default: return "Unknown";
    }
}

int main() {
    std::cout << "===================================\n";
    std::cout << "XpuMusic Playlist Demo\n";
    std::cout << "===================================\n\n";

    // Create playlist manager
    auto playlist = std::make_unique<PlaylistManager>();

    // Set up event callback
    playlist->set_event_callback([](const PlaylistEvent& event) {
        switch (event.type) {
            case PlaylistEvent::TrackAdded:
                std::cout << "Added: " << event.track.title << std::endl;
                break;
            case PlaylistEvent::CurrentChanged:
                std::cout << "\n>>> Track changed to index " << event.track_index << std::endl;
                break;
            case PlaylistEvent::PlaybackModeChanged:
                std::cout << "Playback mode changed\n";
                break;
            default:
                break;
        }
    });

    // Add some demo tracks
    std::cout << "Adding tracks to playlist...\n";
    playlist->add_track("test_1khz.wav");
    playlist->add_track("test_audio.wav");

    // Create some demo tracks with metadata
    Track track1;
    track1.path = "music/sample1.mp3";
    track1.title = "Sample Track 1";
    track1.artist = "Demo Artist";
    track1.album = "Demo Album";
    track1.duration = 180.0;  // 3 minutes
    playlist->add_track(track1);

    Track track2;
    track2.path = "music/sample2.mp3";
    track2.title = "Sample Track 2";
    track2.artist = "Demo Artist";
    track2.album = "Demo Album";
    track2.duration = 240.0;  // 4 minutes
    playlist->add_track(track2);

    Track track3;
    track3.path = "music/sample3.mp3";
    track3.title = "Sample Track 3";
    track3.artist = "Another Artist";
    track3.album = "Different Album";
    track3.duration = 200.0;  // 3:20
    playlist->add_track(track3);

    // Display initial playlist
    print_playlist(*playlist);

    // Demo navigation
    std::cout << "\n=== Navigation Demo ===\n";
    print_current_track(*playlist);

    std::cout << "\nNext track...\n";
    playlist->next();
    print_current_track(*playlist);

    std::cout << "\nPrevious track...\n";
    playlist->previous();
    print_current_track(*playlist);

    // Demo playback modes
    std::cout << "\n=== Playback Modes Demo ===\n";

    std::vector<PlaybackMode> modes = {
        PlaybackMode::Sequential,
        PlaybackMode::Shuffle,
        PlaybackMode::Random,
        PlaybackMode::RepeatAll
    };

    for (auto mode : modes) {
        playlist->set_playback_mode(mode);
        std::cout << "\nPlayback mode: " << playback_mode_to_string(mode) << "\n";

        // Play through a few tracks
        for (int i = 0; i < 3 && playlist->get_track_count() > 0; ++i) {
            std::cout << "  ";
            print_current_track(*playlist);
            playlist->next();
        }

        // Reset to first track
        playlist->jump_to(0);
    }

    // Demo search and filter
    std::cout << "\n=== Search and Filter Demo ===\n";

    auto results = playlist->search("Sample");
    std::cout << "\nSearch results for 'Sample': " << results.size() << " tracks found\n";
    for (size_t idx : results) {
        const auto& track = playlist->get_track(idx);
        std::cout << "  - " << track.title;
        if (!track.artist.empty()) {
            std::cout << " by " << track.artist;
        }
        std::cout << "\n";
    }

    // Demo playlist persistence
    std::cout << "\n=== Playlist Persistence Demo ===\n";

    std::cout << "Saving playlist to demo.m3u...\n";
    if (playlist->save_m3u("demo.m3u")) {
        std::cout << "Playlist saved successfully\n";
    }

    std::cout << "Saving playlist to demo.pls...\n";
    if (playlist->save_pls("demo.pls")) {
        std::cout << "Playlist saved successfully\n";
    }

    // Demo playlist loading
    std::cout << "\nLoading playlist from demo.m3u...\n";
    auto loaded_playlist = PlaylistParser::parse("demo.m3u");
    if (loaded_playlist) {
        std::cout << "Loaded " << loaded_playlist->get_track_count() << " tracks\n";
        std::cout << "Total duration: " << loaded_playlist->get_total_duration() << " seconds\n";
    }

    // Demo track management
    std::cout << "\n=== Track Management Demo ===\n";

    std::cout << "\nMoving track 0 to position 2...\n";
    playlist->move_track(0, 2);
    print_playlist(*playlist);

    std::cout << "\nRemoving current track...\n";
    playlist->remove_current();
    print_playlist(*playlist);

    // Demo statistics
    std::cout << "\n=== Statistics ===\n";
    std::cout << "Total tracks: " << playlist->get_track_count() << "\n";
    std::cout << "Total duration: " << playlist->get_total_duration() << " seconds (";
    std::cout << std::fixed << std::setprecision(1) << playlist->get_total_duration() / 60 << " minutes)\n";
    std::cout << "Tracks played: " << playlist->get_played_count() << "\n";

    // Interactive demo
    std::cout << "\n=== Interactive Demo ===\n";
    std::cout << "Commands: n(ext), p(revious), j(ump) <index>, q(uit)\n";

    playlist->set_playback_mode(PlaybackMode::Sequential);
    playlist->jump_to(0);

    char cmd;
    while (std::cin >> cmd && cmd != 'q') {
        switch (cmd) {
            case 'n':
                if (playlist->has_next()) {
                    playlist->next();
                    print_current_track(*playlist);
                } else {
                    std::cout << "No next track\n";
                }
                break;

            case 'p':
                if (playlist->has_previous()) {
                    playlist->previous();
                    print_current_track(*playlist);
                } else {
                    std::cout << "No previous track\n";
                }
                break;

            case 'j': {
                size_t index;
                std::cin >> index;
                if (index > 0 && index <= playlist->get_track_count()) {
                    playlist->jump_to(index - 1);
                    print_current_track(*playlist);
                } else {
                    std::cout << "Invalid index\n";
                }
                break;
            }

            default:
                std::cout << "Unknown command\n";
                break;
        }
    }

    std::cout << "\nDemo complete!\n";
    return 0;
}