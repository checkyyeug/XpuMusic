/**
 * @file music_player_with_playlist.cpp
 * @brief Enhanced music player with playlist support
 * @date 2025-12-13
 */

#include "../playlist/playlist_manager.h"
#include "../audio/audio_output.h"
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <fstream>

using namespace xpumusic::playlist;

class EnhancedMusicPlayer {
private:
    std::unique_ptr<PlaylistManager> playlist_;
    std::unique_ptr<AudioOutput> audio_output_;
    bool is_playing_;
    bool should_stop_;
    std::thread playback_thread_;

public:
    EnhancedMusicPlayer()
        : is_playing_(false)
        , should_stop_(false) {

        playlist_ = std::make_unique<PlaylistManager>();
        audio_output_ = AudioOutputFactory::create();

        // Set up event callbacks
        setup_event_callbacks();
    }

    ~EnhancedMusicPlayer() {
        stop();
        if (playback_thread_.joinable()) {
            playback_thread_.join();
        }
    }

    void setup_event_callbacks() {
        // Playlist events
        playlist_->set_event_callback([this](const PlaylistEvent& event) {
            switch (event.type) {
                case PlaylistEvent::CurrentChanged:
                    if (is_playing_) {
                        load_and_play_current();
                    }
                    break;
                default:
                    break;
            }
        });

        // Audio player events (if available)
        // audio_player_->set_completion_callback([this]() {
        //     on_track_finished();
        // });
    }

    // Playlist management
    void add_file(const std::string& file_path) {
        playlist_->add_track(file_path);
        print_status();
    }

    void load_playlist(const std::string& playlist_path) {
        auto loaded = PlaylistParser::parse(playlist_path);
        if (loaded) {
            playlist_ = std::move(loaded);
            setup_event_callbacks();
            std::cout << "Loaded playlist: " << playlist_->get_track_count() << " tracks\n";
        } else {
            std::cout << "Failed to load playlist: " << playlist_path << std::endl;
        }
    }

    void save_playlist(const std::string& playlist_path) {
        std::string ext = std::filesystem::path(playlist_path).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        bool success = false;
        if (ext == ".pls") {
            success = playlist_->save_pls(playlist_path);
        } else {
            success = playlist_->save_m3u(playlist_path);
        }

        if (success) {
            std::cout << "Playlist saved to: " << playlist_path << std::endl;
        } else {
            std::cout << "Failed to save playlist" << std::endl;
        }
    }

    // Playback control
    void play() {
        if (!is_playing_) {
            if (playlist_->get_track_count() == 0) {
                std::cout << "Playlist is empty. Add some tracks first.\n";
                return;
            }

            is_playing_ = true;
            should_stop_ = false;

            if (playback_thread_.joinable()) {
                playback_thread_.join();
            }

            playback_thread_ = std::thread([this]() {
                playback_loop();
            });

            std::cout << "▶ Playing\n";
        }
        print_status();
    }

    void pause() {
        if (audio_output_) {
            audio_output_->pause();
        }
        std::cout << "⏸ Paused\n";
    }

    void stop() {
        should_stop_ = true;
        is_playing_ = false;

        if (audio_output_) {
            audio_output_->stop();
        }

        if (playback_thread_.joinable()) {
            playback_thread_.join();
        }

        std::cout << "⏹ Stopped\n";
    }

    void next() {
        playlist_->next();
        print_status();
    }

    void previous() {
        playlist_->previous();
        print_status();
    }

    void jump_to(size_t index) {
        if (index < playlist_->get_track_count()) {
            playlist_->jump_to(index);
            print_status();
        } else {
            std::cout << "Invalid track index\n";
        }
    }

    void set_playback_mode(PlaybackMode mode) {
        playlist_->set_playback_mode(mode);
        std::cout << "Playback mode: " << playback_mode_to_string(mode) << "\n";
    }

    void shuffle() {
        playlist_->set_playback_mode(PlaybackMode::Random);
        playlist_->set_shuffle_seed(std::chrono::steady_clock::now().time_since_epoch().count());
        std::cout << "Playlist shuffled\n";
    }

    // Display functions
    void print_status() {
        std::cout << "\n" << std::string(60, '=') << "\n";

        if (playlist_->get_track_count() == 0) {
            std::cout << "No tracks in playlist\n";
            std::cout << std::string(60, '=') << "\n";
            return;
        }

        // Current track
        const auto& current = playlist_->get_current_track();
        std::cout << (is_playing_ ? "▶ " : "⏸ ");
        std::cout << std::setw(40) << std::left << current.title.substr(0, 39);
        if (!current.artist.empty()) {
            std::cout << " - " << current.artist.substr(0, 15);
        }
        std::cout << "\n";

        // Playlist info
        std::cout << "Track " << (playlist_->get_current_index() + 1);
        std::cout << " of " << playlist_->get_track_count();
        std::cout << " | ";
        std::cout << std::fixed << std::setprecision(1);
        std::cout << "Duration: " << current.duration << "s";
        std::cout << " | ";
        std::cout << "Mode: " << playback_mode_to_string(playlist_->get_playback_mode());
        std::cout << "\n";

        // Progress bar (simplified)
        std::cout << "[";
        for (int i = 0; i < 30; ++i) {
            std::cout << (i == 15 ? "♪" : "-");
        }
        std::cout << "]\n";

        std::cout << std::string(60, '=') << "\n";
    }

    void print_playlist() {
        if (playlist_->get_track_count() == 0) {
            std::cout << "Playlist is empty\n";
            return;
        }

        std::cout << "\nPlaylist (" << playlist_->get_track_count() << " tracks):\n";
        std::cout << std::string(60, '-') << "\n";

        size_t current = playlist_->get_current_index();
        for (size_t i = 0; i < playlist_->get_track_count(); ++i) {
            const auto& track = playlist_->get_track(i);

            std::cout << (i == current ? "▶ " : "  ");
            std::cout << std::setw(3) << (i + 1) << ". ";
            std::cout << std::setw(35) << std::left << track.title.substr(0, 34);

            if (!track.artist.empty()) {
                std::cout << " - " << track.artist.substr(0, 15);
            }

            if (track.duration > 0) {
                std::cout << " (" << std::fixed << std::setprecision(1) << track.duration << "s)";
            }

            std::cout << "\n";
        }

        std::cout << std::string(60, '-') << "\n";
        std::cout << "Total duration: " << std::fixed << std::setprecision(1);
        std::cout << playlist_->get_total_duration() / 60 << " minutes\n";
    }

    void print_help() {
        std::cout << "\nCommands:\n";
        std::cout << "  play/pause     - Play or pause current track\n";
        std::cout << "  stop           - Stop playback\n";
        std::cout << "  next           - Next track\n";
        std::cout << "  prev           - Previous track\n";
        std::cout << "  jump <n>       - Jump to track number n\n";
        std::cout << "  add <file>     - Add file to playlist\n";
        std::cout << "  load <file>    - Load playlist file\n";
        std::cout << "  save <file>    - Save playlist to file\n";
        std::cout << "  list           - Show all tracks\n";
        std::cout << "  shuffle        - Shuffle playlist\n";
        std::cout << "  mode <mode>    - Set playback mode:\n";
        std::cout << "                  seq(ential), rand(om), repeat1, repeatall, shuffle\n";
        std::cout << "  status         - Show current status\n";
        std::cout << "  help           - Show this help\n";
        std::cout << "  quit/exit      - Exit player\n";
    }

private:
    void load_and_play_current() {
        if (playlist_->get_track_count() == 0) return;

        const auto& track = playlist_->get_current_track();

        // In a real implementation, you would:
        // 1. Load the audio file
        // 2. Start playback through audio_player_
        // 3. Monitor playback progress

        std::cout << "\nLoading: " << track.title << "\n";
        std::cout << "From: " << track.path << "\n";

        // Simulate loading
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Start playback
        if (audio_player_) {
            audio_player_->load(track.path.c_str());
            audio_player_->play();
        }
    }

    void playback_loop() {
        while (!should_stop_) {
            if (is_playing_) {
                // In a real implementation, you would:
                // 1. Check if current track has finished
                // 2. If yes, go to next track based on playback mode
                // 3. Update UI with progress

                // Simulate track progress
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }

    void on_track_finished() {
        if (is_playing_) {
            // Handle track completion based on playback mode
            if (playlist_->has_next()) {
                playlist_->next();
            } else {
                // Handle end of playlist
                PlaybackMode mode = playlist_->get_playback_mode();
                if (mode == PlaybackMode::RepeatAll) {
                    playlist_->jump_to(0);
                    playlist_->next();
                } else if (mode == PlaybackMode::RepeatOne) {
                    // Stay on current track
                } else {
                    // Stop playback
                    is_playing_ = false;
                    std::cout << "\nEnd of playlist\n";
                }
            }
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
};

int main(int argc, char* argv[]) {
    std::cout << "===================================\n";
    std::cout << "XpuMusic Enhanced Player v1.0\n";
    std::cout << "===================================\n\n";

    EnhancedMusicPlayer player;

    // Load command line arguments as initial tracks
    if (argc > 1) {
        std::cout << "Loading tracks from command line...\n";
        for (int i = 1; i < argc; ++i) {
            player.add_file(argv[i]);
        }
    }

    std::cout << "Type 'help' for commands\n\n";

    // Command loop
    std::string command;
    std::string arg;

    while (true) {
        std::cout << "> ";
        std::cin >> command;

        if (command == "quit" || command == "exit") {
            break;
        }
        else if (command == "play") {
            player.play();
        }
        else if (command == "pause") {
            player.pause();
        }
        else if (command == "stop") {
            player.stop();
        }
        else if (command == "next") {
            player.next();
        }
        else if (command == "prev") {
            player.previous();
        }
        else if (command == "jump") {
            std::cin >> arg;
            try {
                size_t index = std::stoul(arg) - 1;
                player.jump_to(index);
            } catch (...) {
                std::cout << "Invalid track number\n";
            }
        }
        else if (command == "add") {
            std::cin >> arg;
            player.add_file(arg);
        }
        else if (command == "load") {
            std::cin >> arg;
            player.load_playlist(arg);
        }
        else if (command == "save") {
            std::cin >> arg;
            player.save_playlist(arg);
        }
        else if (command == "list") {
            player.print_playlist();
        }
        else if (command == "shuffle") {
            player.shuffle();
        }
        else if (command == "mode") {
            std::cin >> arg;
            if (arg == "seq" || arg == "sequential") {
                player.set_playback_mode(PlaybackMode::Sequential);
            }
            else if (arg == "rand" || arg == "random") {
                player.set_playback_mode(PlaybackMode::Random);
            }
            else if (arg == "repeat1") {
                player.set_playback_mode(PlaybackMode::RepeatOne);
            }
            else if (arg == "repeatall") {
                player.set_playback_mode(PlaybackMode::RepeatAll);
            }
            else if (arg == "shuffle") {
                player.set_playback_mode(PlaybackMode::Shuffle);
            }
            else {
                std::cout << "Unknown mode. Use: seq, rand, repeat1, repeatall, shuffle\n";
            }
        }
        else if (command == "status") {
            player.print_status();
        }
        else if (command == "help") {
            player.print_help();
        }
        else {
            std::cout << "Unknown command. Type 'help' for available commands.\n";
        }
    }

    player.stop();
    std::cout << "\nGoodbye!\n";
    return 0;
}