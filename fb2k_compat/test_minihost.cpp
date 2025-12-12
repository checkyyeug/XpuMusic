// 闃舵1娴嬭瘯绋嬪簭
// 娴嬭瘯鍔犺浇foobar2000缁勪欢骞惰В鐮侀煶棰戞枃浠?

#include "minihost.h"
#include <iostream>
#include <filesystem>
#include <vector>
#include <string>

namespace fs = std::filesystem;

// 鏌ユ壘foobar2000缁勪欢鐩綍
std::vector<std::wstring> find_fb2k_components() {
    std::vector<std::wstring> components;
    
    // 甯歌瀹夎璺緞
    std::vector<std::wstring> search_paths = {
        L"C:\\Program Files (x86)\\foobar2000\\components",
        L"C:\\Program Files\\foobar2000\\components",
        L"C:\\Users\\" + std::wstring(_wgetenv(L"USERNAME")) + L"\\AppData\\Roaming\\foobar2000\\user-components"
    };
    
    for(const auto& base_path : search_paths) {
        if(!fs::exists(base_path)) continue;
        
        // 鏌ユ壘鎵€鏈塂LL鏂囦欢
        for(const auto& entry : fs::directory_iterator(base_path)) {
            if(entry.is_regular_file() && entry.path().extension() == L".dll") {
                components.push_back(entry.path().wstring());
            }
        }
        
        // 涔熸煡鎵惧瓙鐩綍涓殑DLL
        for(const auto& entry : fs::recursive_directory_iterator(base_path)) {
            if(entry.is_regular_file() && entry.path().extension() == L".dll") {
                components.push_back(entry.path().wstring());
            }
        }
    }
    
    return components;
}

// 鏌ユ壘娴嬭瘯闊抽鏂囦欢
std::string find_test_audio_file() {
    // 浼樺厛浣跨敤椤圭洰鐩綍涓嬬殑娴嬭瘯鏂囦欢
    std::vector<std::string> test_files = {
        "test_440hz.wav",
        "input_44100Hz.wav", 
        "1khz.wav"
    };
    
    for(const auto& file : test_files) {
        if(fs::exists(file)) {
            return file;
        }
    }
    
    // 濡傛灉娌℃湁锛岃鐢ㄦ埛閫夋嫨
    std::cout << "璇疯緭鍏ヨ娴嬭瘯鐨勯煶棰戞枃浠惰矾寰? ";
    std::string path;
    std::getline(std::cin, path);
    return path;
}

int main(int argc, char* argv[]) {
    std::cout << "=== foobar2000 缁勪欢鍏煎娴嬭瘯绋嬪簭 ===" << std::endl;
    std::cout << "闃舵1锛氭渶灏忎富鏈烘帴鍙ｆ祴璇? << std::endl;
    std::cout << std::endl;
    
    // 鍒涘缓涓绘満
    mini_host host;
    if(!host.initialize()) {
        std::cerr << "鍒濆鍖栦富鏈哄け璐?" << std::endl;
        return 1;
    }
    
    std::cout << "姝ｅ湪鎼滅储 foobar2000 缁勪欢..." << std::endl;
    auto components = find_fb2k_components();
    
    if(components.empty()) {
        std::cout << "鏈壘鍒?foobar2000 缁勪欢!" << std::endl;
        std::cout << "璇锋墜鍔ㄦ寚瀹氱粍浠剁洰褰? ";
        std::string path;
        std::getline(std::cin, path);
        
        if(!fs::exists(path)) {
            std::cerr << "璺緞涓嶅瓨鍦?" << std::endl;
            return 1;
        }
        
        // 鎵嬪姩娣诲姞DLL鏂囦欢
        for(const auto& entry : fs::directory_iterator(path)) {
            if(entry.is_regular_file() && entry.path().extension() == ".dll") {
                components.push_back(entry.path().wstring());
            }
        }
    }
    
    std::cout << "鎵惧埌 " << components.size() << " 涓粍浠?" << std::endl;
    for(size_t i = 0; i < components.size(); ++i) {
        std::wcout << L"  [" << i << L"] " << components[i] << std::endl;
    }
    
    // 鍔犺浇鏍稿績缁勪欢锛堜紭鍏堝姞杞絠nput_std锛?
    std::cout << std::endl << "姝ｅ湪鍔犺浇缁勪欢..." << std::endl;
    
    int loaded_count = 0;
    for(const auto& component : components) {
        std::string name = wide_to_utf8(component);
        
        // 浼樺厛鍔犺浇input_std缁勪欢
        if(name.find("input_std") != std::string::npos || 
           name.find("foo_input_std") != std::string::npos) {
            std::cout << "浼樺厛鍔犺浇鏍稿績缁勪欢: " << name << std::endl;
            if(host.load_component(component)) {
                loaded_count++;
                break; // 鍏堝彧鍔犺浇涓€涓牳蹇冪粍浠惰繘琛屾祴璇?
            }
        }
    }
    
    // 濡傛灉娌℃壘鍒癷nput_std锛屽皾璇曞姞杞藉叾浠栫粍浠?
    if(loaded_count == 0 && !components.empty()) {
        std::cout << "灏濊瘯鍔犺浇绗竴涓粍浠? " << wide_to_utf8(components[0]) << std::endl;
        if(host.load_component(components[0])) {
            loaded_count++;
        }
    }
    
    std::cout << std::endl << "鎴愬姛鍔犺浇 " << loaded_count << " 涓粍浠? << std::endl;
    
    // 鏄剧ず宸插姞杞界殑缁勪欢
    auto loaded = host.get_loaded_components();
    for(const auto& name : loaded) {
        std::cout << "  - " << name << std::endl;
    }
    
    if(loaded.empty()) {
        std::cout << "娌℃湁缁勪欢琚姞杞斤紝娴嬭瘯缁撴潫銆? << std::endl;
        return 0;
    }
    
    // 娴嬭瘯瑙ｇ爜
    std::cout << std::endl << "鍑嗗娴嬭瘯瑙ｇ爜..." << std::endl;
    std::string test_file = find_test_audio_file();
    
    if(!fs::exists(test_file)) {
        std::cerr << "娴嬭瘯鏂囦欢涓嶅瓨鍦? " << test_file << std::endl;
        return 1;
    }
    
    std::cout << "浣跨敤娴嬭瘯鏂囦欢: " << test_file << std::endl;
    
    // 杩愯瑙ｇ爜娴嬭瘯
    std::cout << std::endl << "寮€濮嬭В鐮佹祴璇?.." << std::endl;
    bool success = host.test_decode(test_file);
    
    if(success) {
        std::cout << std::endl << "鉁?娴嬭瘯鎴愬姛! foobar2000 缁勪欢鍏煎灞傚伐浣滄甯搞€? << std::endl;
    } else {
        std::cout << std::endl << "鉂?娴嬭瘯澶辫触锛岃妫€鏌ラ敊璇俊鎭€? << std::endl;
    }
    
    return success ? 0 : 1;
}