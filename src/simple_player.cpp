#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

// 鍖呭惈鍩虹SDK澶存枃浠?
#include "../compat/sdk_implementations/audio_output.h"
#include "../compat/sdk_implementations/audio_decoder.h"
#include "../compat/sdk_implementations/simple_dsp.h"

// 鍩虹闊抽澶勭悊
namespace mp {
    class SimplePlayer {
    private:
        std::unique_ptr<IAudioOutput> audio_output_;
        std::unique_ptr<IAudioDecoder> decoder_;
        std::unique_ptr<ISimpleDSP> dsp_;
        bool is_playing_;
        bool is_initialized_;

    public:
        SimplePlayer() : is_playing_(false), is_initialized_(false) {}
        
        ~SimplePlayer() {
            stop();
        }

        bool initialize() {
            std::cout << "=== XpuMusic Simple Player ===" << std::endl;
            std::cout << "Initializing audio system..." << std::endl;

            try {
                // 鍒涘缓闊抽杈撳嚭
                audio_output_ = create_audio_output();
                if (!audio_output_) {
                    std::cerr << "鉂?Failed to create audio output" << std::endl;
                    return false;
                }
                std::cout << "鉁?Audio output created" << std::endl;

                // 鍒涘缓瑙ｇ爜鍣?
                decoder_ = create_audio_decoder();
                if (!decoder_) {
                    std::cerr << "鉂?Failed to create audio decoder" << std::endl;
                    return false;
                }
                std::cout << "鉁?Audio decoder created" << std::endl;

                // 鍒涘缓DSP澶勭悊鍣?
                dsp_ = create_simple_dsp();
                if (!dsp_) {
                    std::cerr << "鉂?Failed to create DSP processor" << std::endl;
                    return false;
                }
                std::cout << "鉁?DSP processor created" << std::endl;

                is_initialized_ = true;
                std::cout << "鉁?Audio system initialized successfully!" << std::endl;
                return true;

            } catch (const std::exception& e) {
                std::cerr << "鉂?Initialization error: " << e.what() << std::endl;
                return false;
            }
        }

        bool play_file(const std::string& file_path) {
            if (!is_initialized_) {
                std::cerr << "鉂?Player not initialized" << std::endl;
                return false;
            }

            if (is_playing_) {
                stop();
            }

            std::cout << "\n馃幍 Loading file: " << file_path << std::endl;

            try {
                // 鎵撳紑闊抽鏂囦欢
                if (!decoder_->open_file(file_path)) {
                    std::cerr << "鉂?Failed to open file: " << file_path << std::endl;
                    return false;
                }
                std::cout << "鉁?File opened successfully" << std::endl;

                // 鑾峰彇闊抽淇℃伅
                auto info = decoder_->get_file_info();
                std::cout << "馃搳 Audio Info:" << std::endl;
                std::cout << "  Sample Rate: " << info.sample_rate << " Hz" << std::endl;
                std::cout << "  Channels: " << info.channels << std::endl;
                std::cout << "  Duration: " << info.duration_seconds << " seconds" << std::endl;
                std::cout << "  Format: " << info.format << std::endl;

                // 閰嶇疆闊抽杈撳嚭
                if (!audio_output_->open(info.sample_rate, info.channels, 16, 512)) {
                    std::cerr << "鉂?Failed to open audio output" << std::endl;
                    return false;
                }
                std::cout << "鉁?Audio output configured" << std::endl;

                // 寮€濮嬫挱鏀?
                is_playing_ = true;
                std::cout << "鈻讹笍  Playing..." << std::endl;

                // 绠€鍗曠殑鎾斁寰幆
                play_audio_loop();

                return true;

            } catch (const std::exception& e) {
                std::cerr << "鉂?Playback error: " << e.what() << std::endl;
                return false;
            }
        }

        void stop() {
            if (is_playing_) {
                is_playing_ = false;
                std::cout << "鈴癸笍  Stopping playback..." << std::endl;
                
                if (audio_output_) {
                    audio_output_->close();
                }
                if (decoder_) {
                    decoder_->close();
                }
                
                std::cout << "鉁?Playback stopped" << std::endl;
            }
        }

        void set_volume(float volume) {
            if (dsp_) {
                dsp_->set_volume(volume);
                std::cout << "馃攰 Volume set to: " << (volume * 100) << "%" << std::endl;
            }
        }

        bool is_playing() const {
            return is_playing_;
        }

        void show_help() {
            std::cout << "\n=== XpuMusic Player Commands ===" << std::endl;
            std::cout << "  play <file>  - Play audio file" << std::endl;
            std::cout << "  stop         - Stop playback" << std::endl;
            std::cout << "  volume <0-1> - Set volume (0-100%)" << std::endl;
            std::cout << "  status       - Show player status" << std::endl;
            std::cout << "  help         - Show this help" << std::endl;
            std::cout << "  quit         - Exit player" << std::endl;
            std::cout << "=================================" << std::endl;
        }

        void show_status() {
            std::cout << "\n=== Player Status ===" << std::endl;
            std::cout << "Initialized: " << (is_initialized_ ? "Yes" : "No") << std::endl;
            std::cout << "Playing: " << (is_playing_ ? "Yes" : "No") << std::endl;
            
            if (decoder_ && decoder_->is_open()) {
                auto info = decoder_->get_file_info();
                std::cout << "Current file: " << info.file_path << std::endl;
                std::cout << "Position: " << decoder_->get_position_seconds() << " / " << info.duration_seconds << " seconds" << std::endl;
            }
            
            std::cout << "=====================" << std::endl;
        }

    private:
        void play_audio_loop() {
            const int buffer_size = 1024;
            std::vector<float> audio_buffer(buffer_size * 2); // 绔嬩綋澹?
            
            while (is_playing_) {
                // 浠庤В鐮佸櫒璇诲彇闊抽鏁版嵁
                int frames_read = decoder_->read_samples(audio_buffer.data(), buffer_size);
                
                if (frames_read <= 0) {
                    // 鏂囦欢缁撴潫
                    std::cout << "\n馃弫 End of file reached" << std::endl;
                    break;
                }
                
                // 搴旂敤DSP澶勭悊
                if (dsp_) {
                    dsp_->process_buffer(audio_buffer.data(), frames_read, 2);
                }
                
                // 鍐欏叆闊抽杈撳嚭
                int frames_written = audio_output_->write(audio_buffer.data(), frames_read * 2, 16);
                
                if (frames_written <= 0) {
                    std::cerr << "鉂?Audio output error" << std::endl;
                    break;
                }
                
                // 灏忓欢杩熼伩鍏岰PU鍗犵敤杩囬珮
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            
            if (is_playing_) {
                std::cout << "\n鉁?Playback completed" << std::endl;
                stop();
            }
        }
    };
}

// 涓诲嚱鏁?
int main(int argc, char* argv[]) {
    std::cout << "馃幍 XpuMusic Simple Player v1.0" << std::endl;
    std::cout << "================================" << std::endl;
    std::cout << "A simple audio player with foobar2000 compatibility" << std::endl;
    std::cout << "================================" << std::endl;

    mp::SimplePlayer player;
    
    // 鍒濆鍖栨挱鏀惧櫒
    if (!player.initialize()) {
        std::cerr << "鉂?Failed to initialize player" << std::endl;
        return 1;
    }

    player.show_help();

    // 鍛戒护琛屾ā寮?
    if (argc > 1) {
        std::string command = argv[1];
        
        if (command == "play" && argc > 2) {
            std::string file_path = argv[2];
            player.play_file(file_path);
            
            // 绛夊緟鎾斁瀹屾垚
            while (player.is_playing()) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            
        } else if (command == "help") {
            player.show_help();
        } else {
            std::cout << "Usage: " << argv[0] << " play <audio_file>" << std::endl;
            std::cout << "       " << argv[0] << " help" << std::endl;
        }
        
        return 0;
    }

    // 浜や簰寮忔ā寮?
    std::cout << "\n馃挕 Enter commands (type 'help' for available commands):" << std::endl;
    
    std::string command;
    while (true) {
        std::cout << "\n> ";
        std::getline(std::cin, command);
        
        if (command.empty()) continue;
        
        std::istringstream iss(command);
        std::string cmd;
        iss >> cmd;
        
        if (cmd == "quit" || cmd == "exit") {
            break;
        } else if (cmd == "help") {
            player.show_help();
        } else if (cmd == "status") {
            player.show_status();
        } else if (cmd == "stop") {
            player.stop();
        } else if (cmd == "play") {
            std::string file_path;
            std::getline(iss, file_path);
            
            // 鍘婚櫎鍓嶅绌烘牸
            file_path.erase(0, file_path.find_first_not_of(" \t"));
            
            if (!file_path.empty()) {
                std::thread play_thread([&player, file_path]() {
                    player.play_file(file_path);
                });
                play_thread.detach();
            } else {
                std::cout << "鉂?Please specify a file path" << std::endl;
            }
        } else if (cmd == "volume") {
            float volume;
            iss >> volume;
            
            if (volume >= 0.0f && volume <= 1.0f) {
                player.set_volume(volume);
            } else {
                std::cout << "鉂?Volume must be between 0.0 and 1.0" << std::endl;
            }
        } else {
            std::cout << "鉂?Unknown command: " << cmd << std::endl;
            std::cout << "Type 'help' for available commands" << std::endl;
        }
    }

    std::cout << "\n馃憢 Shutting down player..." << std::endl;
    player.stop();
    std::cout << "鉁?Player shutdown complete" << std::endl;
    
    return 0;
}