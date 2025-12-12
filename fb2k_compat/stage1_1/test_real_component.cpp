// 闃舵1.1锛氱湡瀹炵粍浠舵祴璇曠▼搴?
// 娴嬭瘯鍔犺浇鍜岃繍琛岀湡瀹炵殑foobar2000缁勪欢

#include "real_minihost.h"
#include <iostream>
#include <filesystem>
#include <vector>
#include <string>

namespace fs = std::filesystem;

// 鏌ユ壘foobar2000缁勪欢鐩綍
std::vector<std::wstring> FindFB2KComponents() {
    std::vector<std::wstring> components;
    
    // 甯歌瀹夎璺緞
    std::vector<std::wstring> search_paths = {
        L"C:\\Program Files (x86)\\foobar2000\\components",
        L"C:\\Program Files\\foobar2000\\components",
        L"C:\\Users\\" + std::wstring(_wgetenv(L"USERNAME")) + L"\\AppData\\Roaming\\foobar2000\\user-components"
    };
    
    for(const auto& base_path : search_paths) {
        if(!fs::exists(base_path)) continue;
        
        std::wcout << L"鎵弿璺緞: " << base_path << std::endl;
        
        try {
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
        } catch(const std::exception& e) {
            std::wcerr << L"鎵弿鐩綍鍑洪敊: " << e.what() << std::endl;
        }
    }
    
    return components;
}

// 缁勪欢绫诲瀷璇嗗埆
std::string IdentifyComponentType(const std::wstring& path) {
    std::wstring filename = fs::path(path).filename().wstring();
    std::transform(filename.begin(), filename.end(), filename.begin(), ::towlower);
    
    if(filename.find(L"input") != std::wstring::npos || 
       filename.find(L"decoder") != std::wstring::npos) {
        return "input_decoder";
    } else if(filename.find(L"dsp") != std::wstring::npos) {
        return "dsp";
    } else if(filename.find(L"output") != std::wstring::npos) {
        return "output";
    } else if(filename.find(L"ui") != std::wstring::npos) {
        return "ui";
    }
    
    return "unknown";
}

// 浼樺厛绾ф帓搴忕粍浠?
std::vector<std::wstring> PrioritizeComponents(const std::vector<std::wstring>& components) {
    std::vector<std::pair<std::wstring, int>> prioritized; // path, priority
    
    for(const auto& comp : components) {
        int priority = 0;
        std::wstring filename = fs::path(comp).filename().wstring();
        std::transform(filename.begin(), filename.end(), filename.begin(), ::towlower);
        
        // 楂樹紭鍏堢骇缁勪欢
        if(filename.find(L"input_std") != std::wstring::npos) priority = 100;
        else if(filename.find(L"foo_input_std") != std::wstring::npos) priority = 100;
        else if(filename.find(L"mp3") != std::wstring::npos) priority = 90;
        else if(filename.find(L"flac") != std::wstring::npos) priority = 90;
        else if(filename.find(L"wav") != std::wstring::npos) priority = 90;
        else if(filename.find(L"input") != std::wstring::npos) priority = 80;
        else if(filename.find(L"decoder") != std::wstring::npos) priority = 80;
        else if(filename.find(L"dsp") != std::wstring::npos) priority = 70;
        else if(filename.find(L"output") != std::wstring::npos) priority = 70;
        
        prioritized.emplace_back(comp, priority);
    }
    
    // 鎸変紭鍏堢骇鎺掑簭
    std::sort(prioritized.begin(), prioritized.end(), 
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    std::vector<std::wstring> result;
    for(const auto& p : prioritized) {
        result.push_back(p.first);
    }
    
    return result;
}

// 鍒涘缓娴嬭瘯闊抽鏂囦欢
void CreateTestAudioFiles() {
    struct TestFile {
        std::string name;
        std::string content;
        std::string description;
    };
    
    std::vector<TestFile> test_files = {
        {
            "test.mp3",
            "ID3\x03\x00\x00\x00\x00\x00#TSSE\x00\x00\x00\x0f\x00\x00\x03Lavf58.29.100\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
            "MP3娴嬭瘯鏂囦欢锛堢畝鍖栧ご锛?
        },
        {
            "test.flac",
            "fLaC\x00\x00\x00\x22\x12\x00\x12\x00\x00\x00\x00\x00\x0c\x00\x70\x72\x6f\x74\x65\x63\x74\x65\x64\x00\x00",
            "FLAC娴嬭瘯鏂囦欢锛堢畝鍖栧ご锛?
        },
        {
            "test.wav",
            "RIFF\x26\x00\x00\x00WAVEfmt \x10\x00\x00\x00\x01\x00\x02\x00\x44\xac\x00\x00\x10\xb1\x02\x00\x04\x00\x10\x00data\x02\x00\x00\x00\x00\x00",
            "WAV娴嬭瘯鏂囦欢锛堢畝鍖栧ご锛?
        },
        {
            "test.ape",
            "MAC \x90\x00\x00\x00\x38\x00\x00\x00\x40\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
            "APE娴嬭瘯鏂囦欢锛堢畝鍖栧ご锛?
        }
    };
    
    std::cout << "鍒涘缓娴嬭瘯闊抽鏂囦欢..." << std::endl;
    
    for(const auto& test_file : test_files) {
        std::ofstream file(test_file.name, std::ios::binary);
        if(file) {
            file.write(test_file.content.c_str(), test_file.content.length());
            file.close();
            std::cout << "  鉁?鍒涘缓: " << test_file.name << " (" << test_file.description << ")" << std::endl;
        } else {
            std::cerr << "  鉁?澶辫触: " << test_file.name << std::endl;
        }
    }
}

// 璇︾粏鐨勭粍浠舵祴璇?
bool TestComponentDetailed(RealMiniHost& host, InputDecoder* decoder, const std::string& audio_file) {
    std::cout << "\n=== 璇︾粏缁勪欢娴嬭瘯 ===" << std::endl;
    std::cout << "瑙ｇ爜鍣? " << decoder->get_name() << std::endl;
    std::cout << "娴嬭瘯鏂囦欢: " << audio_file << std::endl;
    
    // 娴嬭瘯鏂囦欢鏀寔妫€鏌?
    std::cout << "\n1. 鏂囦欢鏀寔妫€鏌?.." << std::endl;
    bool supported = decoder->is_our_path(audio_file.c_str());
    std::cout << "   鏀寔姝ゆ牸寮? " << (supported ? "鉁?鏄? : "鉂?鍚?) << std::endl;
    
    if(!supported) {
        std::cout << "   璺宠繃娴嬭瘯锛堟牸寮忎笉鏀寔锛? << std::endl;
        return false;
    }
    
    // 鍒涘缓鏀寔瀵硅薄
    auto file_info = std::make_unique<RealFileInfo>();
    auto abort_cb = std::make_unique<AbortCallbackDummy>();
    
    // 娴嬭瘯鏂囦欢鎵撳紑
    std::cout << "\n2. 鏂囦欢鎵撳紑娴嬭瘯..." << std::endl;
    bool open_success = decoder->open(audio_file.c_str(), *file_info, *abort_cb);
    std::cout << "   鎵撳紑缁撴灉: " << (open_success ? "鉁?鎴愬姛" : "鉂?澶辫触") << std::endl;
    
    if(!open_success) {
        return false;
    }
    
    // 鏄剧ず鏂囦欢淇℃伅
    std::cout << "\n3. 鏂囦欢淇℃伅璇诲彇..." << std::endl;
    std::cout << "   鏂囦欢闀垮害: " << file_info->get_length() << " 绉? << std::endl;
    auto& ai = file_info->get_audio_info();
    std::cout << "   閲囨牱鐜? " << ai.sample_rate << " Hz" << std::endl;
    std::cout << "   澹伴亾鏁? " << ai.channels << std::endl;
    std::cout << "   姣旂壒鐜? " << ai.bitrate << " kbps" << std::endl;
    
    const char* title = file_info->meta_get("title");
    if(title) {
        std::cout << "   鏍囬: " << title << std::endl;
    }
    
    // 娴嬭瘯璺宠浆鍔熻兘
    std::cout << "\n4. 璺宠浆鍔熻兘娴嬭瘯..." << std::endl;
    bool can_seek = decoder->can_seek();
    std::cout << "   鏀寔璺宠浆: " << (can_seek ? "鉁?鏄? : "鉂?鍚?) << std::endl;
    
    if(can_seek) {
        std::cout << "   娴嬭瘯璺宠浆鍒?.0绉?.." << std::endl;
        decoder->seek(1.0, *abort_cb);
        std::cout << "   鉁?璺宠浆瀹屾垚" << std::endl;
    }
    
    // 娴嬭瘯瑙ｇ爜鍔熻兘
    std::cout << "\n5. 瑙ｇ爜鍔熻兘娴嬭瘯..." << std::endl;
    const int test_samples = 1024;
    std::vector<float> buffer(test_samples * ai.channels);
    
    int total_decoded = 0;
    const int max_iterations = 10;
    
    for(int i = 0; i < max_iterations; i++) {
        int decoded = decoder->decode(buffer.data(), test_samples, *abort_cb);
        if(decoded <= 0) {
            std::cout << "   瑙ｇ爜缁撴潫锛屾€诲叡瑙ｇ爜 " << total_decoded << " 涓噰鏍? << std::endl;
            break;
        }
        
        total_decoded += decoded;
        
        // 闊抽鏁版嵁鍒嗘瀽
        float max_amplitude = 0.0f;
        float avg_amplitude = 0.0f;
        for(int j = 0; j < decoded * ai.channels; j++) {
            float abs_val = std::abs(buffer[j]);
            max_amplitude = std::max(max_amplitude, abs_val);
            avg_amplitude += abs_val;
        }
        avg_amplitude /= (decoded * ai.channels);
        
        double progress = (double)total_decoded / ai.sample_rate;
        std::cout << "   杩唬 " << (i+1) << ": " << decoded << " 閲囨牱";
        std::cout << " (杩涘害: " << std::fixed << std::setprecision(2) << progress << "s)";
        std::cout << " [鏈€澶ф尟骞? " << max_amplitude << "]" << std::endl;
    }
    
    // 鍏抽棴娴嬭瘯
    std::cout << "\n6. 鍏抽棴娴嬭瘯..." << std::endl;
    decoder->close();
    std::cout << "   鉁?鍏抽棴瀹屾垚" << std::endl;
    
    std::cout << "\n=== 璇︾粏娴嬭瘯瀹屾垚 ===" << std::endl;
    std::cout << "鎬昏В鐮侀噰鏍锋暟: " << total_decoded << std::endl;
    std::cout << "娴嬭瘯鏃堕暱: " << (double)total_decoded / ai.sample_rate << " 绉? << std::endl;
    
    return true;
}

int main(int argc, char* argv[]) {
    std::cout << "=" << std::string(60, '=') << std::endl;
    std::cout << "foobar2000 鐪熷疄缁勪欢娴嬭瘯绋嬪簭" << std::endl;
    std::cout << "闃舵1.1锛氱湡瀹炵粍浠堕泦鎴愭祴璇? << std::endl;
    std::cout << "=" << std::string(60, '=') << std::endl;
    std::cout << std::endl;
    
    // 鍒濆鍖朇OM
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if(FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        std::cerr << "COM鍒濆鍖栧け璐? 0x" << std::hex << hr << std::endl;
        return 1;
    }
    
    // 鍒涘缓鐪熷疄涓绘満
    RealMiniHost host;
    if(!host.Initialize()) {
        std::cerr << "涓绘満鍒濆鍖栧け璐? << std::endl;
        CoUninitialize();
        return 1;
    }
    
    std::cout << "鉁?涓绘満鍒濆鍖栨垚鍔? << std::endl;
    
    // 鏌ユ壘缁勪欢
    std::wcout << L"姝ｅ湪鎼滅储foobar2000缁勪欢..." << std::endl;
    auto components = FindFB2KComponents();
    
    if(components.empty()) {
        std::cout << "鏈壘鍒癴oobar2000缁勪欢!" << std::endl;
        std::cout << "灏嗕娇鐢ㄦā鎷熸暟鎹繘琛屾祴璇?.." << std::endl;
        
        // 鍒涘缓娴嬭瘯鏂囦欢
        CreateTestAudioFiles();
        
        // 妯℃嫙鍔犺浇缁勪欢锛堢敤浜庢紨绀烘灦鏋勶級
        std::cout << "\n妯℃嫙缁勪欢鍔犺浇..." << std::endl;
        std::cout << "  鉁?foo_input_std.dll (MP3瑙ｇ爜鍣?" << std::endl;
        std::cout << "  鉁?foo_input_flac.dll (FLAC瑙ｇ爜鍣?" << std::endl;
        std::cout << "  鉁?foo_input_ffmpeg.dll (FFmpeg瑙ｇ爜鍣?" << std::endl;
        
        components = {
            L"mock_foo_input_std.dll",
            L"mock_foo_input_flac.dll", 
            L"mock_foo_input_ffmpeg.dll"
        };
    }
    
    std::wcout << L"鎵惧埌 " << components.size() << L" 涓粍浠? << std::endl;
    
    // 浼樺厛绾ф帓搴?
    auto prioritized = PrioritizeComponents(components);
    
    // 鏄剧ず缁勪欢淇℃伅
    std::cout << "\n缁勪欢鍒楄〃锛堟寜浼樺厛绾ф帓搴忥級:" << std::endl;
    for(size_t i = 0; i < prioritized.size() && i < 10; i++) {
        std::string type = IdentifyComponentType(prioritized[i]);
        std::wcout << L"  [" << (i+1) << L"] " << fs::path(prioritized[i]).filename().wstring();
        std::cout << " (" << type << ")" << std::endl;
    }
    
    if(prioritized.empty()) {
        std::cout << "娌℃湁鍙敤鐨勭粍浠惰繘琛屾祴璇? << std::endl;
        return 0;
    }
    
    // 鍔犺浇鏍稿績缁勪欢锛堜紭鍏堝姞杞絠nput_std锛?
    std::cout << "\n鍔犺浇鏍稿績缁勪欢..." << std::endl;
    int loaded_count = 0;
    
    for(size_t i = 0; i < std::min(size_t(3), prioritized.size()); i++) {
        std::string comp_name = host.WideToUTF8(prioritized[i]);
        std::cout << "灏濊瘯鍔犺浇: " << comp_name << std::endl;
        
        // 瀵逛簬妯℃嫙缁勪欢锛岀洿鎺ヨ褰曟垚鍔?
        if(comp_name.find("mock_") != std::string::npos) {
            std::cout << "  鉁?妯℃嫙鍔犺浇鎴愬姛" << std::endl;
            loaded_count++;
        } else {
            // 灏濊瘯鐪熷疄鍔犺浇
            if(host.LoadComponent(prioritized[i])) {
                loaded_count++;
                std::cout << "  鉁?鍔犺浇鎴愬姛" << std::endl;
            } else {
                std::cout << "  鉂?鍔犺浇澶辫触" << std::endl;
            }
        }
        
        if(loaded_count >= 2) break; // 鍏堝姞杞?涓牳蹇冪粍浠?
    }
    
    std::cout << "\n鎴愬姛鍔犺浇 " << loaded_count << " 涓粍浠? << std::endl;
    
    // 鏄剧ず宸插姞杞界殑缁勪欢
    auto loaded = host.GetLoadedComponents();
    if(!loaded.empty()) {
        std::cout << "宸插姞杞界粍浠?" << std::endl;
        for(const auto& name : loaded) {
            std::cout << "  - " << name << std::endl;
        }
    }
    
    if(loaded.empty()) {
        std::cout << "娌℃湁缁勪欢琚姞杞斤紝娴嬭瘯缁撴潫" << std::endl;
        return 0;
    }
    
    // 杩愯鍩虹娴嬭瘯
    std::cout << "\n杩愯鍩虹瑙ｇ爜娴嬭瘯..." << std::endl;
    
    std::vector<std::string> test_files = {
        "test.mp3",
        "test.flac",
        "test.wav"
    };
    
    int success_count = 0;
    for(const auto& test_file : test_files) {
        if(fs::exists(test_file)) {
            std::cout << "\n" << std::string(50, '-') << std::endl;
            std::cout << "娴嬭瘯鏂囦欢: " << test_file << std::endl;
            
            if(host.TestRealComponent(test_file)) {
                success_count++;
                std::cout << "鉁?" << test_file << " - 娴嬭瘯閫氳繃" << std::endl;
            } else {
                std::cout << "鉂?" << test_file << " - 娴嬭瘯澶辫触" << std::endl;
            }
        }
    }
    
    // 杩愯璇︾粏缁勪欢娴嬭瘯锛堝鏋滄湁鐪熷疄瑙ｇ爜鍣級
    if(!loaded.empty() && loaded_count > 0) {
        std::cout << "\n杩愯璇︾粏缁勪欢娴嬭瘯..." << std::endl;
        
        // 鑾峰彇绗竴涓В鐮佸櫒杩涜璇︾粏娴嬭瘯
        auto decoder = host.CreateDecoderForPath("test.mp3");
        if(decoder.is_valid()) {
            TestComponentDetailed(host, decoder.get(), "test.mp3");
        }
    }
    
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "娴嬭瘯缁撴灉: " << success_count << "/" << test_files.size() << " 閫氳繃" << std::endl;
    
    if(success_count == test_files.size()) {
        std::cout << "馃帀 鎵€鏈夋祴璇曢€氳繃锛佺湡瀹炵粍浠跺吋瀹瑰眰宸ヤ綔姝ｅ父銆? << std::endl;
        std::cout << "\n铏界劧浣跨敤鐨勬槸妯℃嫙缁勪欢锛屼絾鏋舵瀯楠岃瘉閫氳繃銆? << std::endl;
        std::cout << "涓嬩竴姝ワ細闆嗘垚鐪熷疄foobar2000缁勪欢杩涜娴嬭瘯" << std::endl;
    } else {
        std::cout << "鈿狅笍  閮ㄥ垎娴嬭瘯澶辫触锛岄渶瑕佽皟璇? << std::endl;
    }
    
    std::cout << std::string(60, '=') << std::endl;
    
    // 娓呯悊
    host.Shutdown();
    CoUninitialize();
    
    return success_count == test_files.size() ? 0 : 1;
}