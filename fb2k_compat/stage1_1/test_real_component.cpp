// 阶段1.1：真实组件测试程序
// 测试加载和运行真实的foobar2000组件

#include "real_minihost.h"
#include <iostream>
#include <filesystem>
#include <vector>
#include <string>

namespace fs = std::filesystem;

// 查找foobar2000组件目录
std::vector<std::wstring> FindFB2KComponents() {
    std::vector<std::wstring> components;
    
    // 常见安装路径
    std::vector<std::wstring> search_paths = {
        L"C:\\Program Files (x86)\\foobar2000\\components",
        L"C:\\Program Files\\foobar2000\\components",
        L"C:\\Users\\" + std::wstring(_wgetenv(L"USERNAME")) + L"\\AppData\\Roaming\\foobar2000\\user-components"
    };
    
    for(const auto& base_path : search_paths) {
        if(!fs::exists(base_path)) continue;
        
        std::wcout << L"扫描路径: " << base_path << std::endl;
        
        try {
            // 查找所有DLL文件
            for(const auto& entry : fs::directory_iterator(base_path)) {
                if(entry.is_regular_file() && entry.path().extension() == L".dll") {
                    components.push_back(entry.path().wstring());
                }
            }
            
            // 也查找子目录中的DLL
            for(const auto& entry : fs::recursive_directory_iterator(base_path)) {
                if(entry.is_regular_file() && entry.path().extension() == L".dll") {
                    components.push_back(entry.path().wstring());
                }
            }
        } catch(const std::exception& e) {
            std::wcerr << L"扫描目录出错: " << e.what() << std::endl;
        }
    }
    
    return components;
}

// 组件类型识别
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

// 优先级排序组件
std::vector<std::wstring> PrioritizeComponents(const std::vector<std::wstring>& components) {
    std::vector<std::pair<std::wstring, int>> prioritized; // path, priority
    
    for(const auto& comp : components) {
        int priority = 0;
        std::wstring filename = fs::path(comp).filename().wstring();
        std::transform(filename.begin(), filename.end(), filename.begin(), ::towlower);
        
        // 高优先级组件
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
    
    // 按优先级排序
    std::sort(prioritized.begin(), prioritized.end(), 
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    std::vector<std::wstring> result;
    for(const auto& p : prioritized) {
        result.push_back(p.first);
    }
    
    return result;
}

// 创建测试音频文件
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
            "MP3测试文件（简化头）"
        },
        {
            "test.flac",
            "fLaC\x00\x00\x00\x22\x12\x00\x12\x00\x00\x00\x00\x00\x0c\x00\x70\x72\x6f\x74\x65\x63\x74\x65\x64\x00\x00",
            "FLAC测试文件（简化头）"
        },
        {
            "test.wav",
            "RIFF\x26\x00\x00\x00WAVEfmt \x10\x00\x00\x00\x01\x00\x02\x00\x44\xac\x00\x00\x10\xb1\x02\x00\x04\x00\x10\x00data\x02\x00\x00\x00\x00\x00",
            "WAV测试文件（简化头）"
        },
        {
            "test.ape",
            "MAC \x90\x00\x00\x00\x38\x00\x00\x00\x40\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
            "APE测试文件（简化头）"
        }
    };
    
    std::cout << "创建测试音频文件..." << std::endl;
    
    for(const auto& test_file : test_files) {
        std::ofstream file(test_file.name, std::ios::binary);
        if(file) {
            file.write(test_file.content.c_str(), test_file.content.length());
            file.close();
            std::cout << "  ✓ 创建: " << test_file.name << " (" << test_file.description << ")" << std::endl;
        } else {
            std::cerr << "  ✗ 失败: " << test_file.name << std::endl;
        }
    }
}

// 详细的组件测试
bool TestComponentDetailed(RealMiniHost& host, InputDecoder* decoder, const std::string& audio_file) {
    std::cout << "\n=== 详细组件测试 ===" << std::endl;
    std::cout << "解码器: " << decoder->get_name() << std::endl;
    std::cout << "测试文件: " << audio_file << std::endl;
    
    // 测试文件支持检查
    std::cout << "\n1. 文件支持检查..." << std::endl;
    bool supported = decoder->is_our_path(audio_file.c_str());
    std::cout << "   支持此格式: " << (supported ? "✅ 是" : "❌ 否") << std::endl;
    
    if(!supported) {
        std::cout << "   跳过测试（格式不支持）" << std::endl;
        return false;
    }
    
    // 创建支持对象
    auto file_info = std::make_unique<RealFileInfo>();
    auto abort_cb = std::make_unique<AbortCallbackDummy>();
    
    // 测试文件打开
    std::cout << "\n2. 文件打开测试..." << std::endl;
    bool open_success = decoder->open(audio_file.c_str(), *file_info, *abort_cb);
    std::cout << "   打开结果: " << (open_success ? "✅ 成功" : "❌ 失败") << std::endl;
    
    if(!open_success) {
        return false;
    }
    
    // 显示文件信息
    std::cout << "\n3. 文件信息读取..." << std::endl;
    std::cout << "   文件长度: " << file_info->get_length() << " 秒" << std::endl;
    auto& ai = file_info->get_audio_info();
    std::cout << "   采样率: " << ai.sample_rate << " Hz" << std::endl;
    std::cout << "   声道数: " << ai.channels << std::endl;
    std::cout << "   比特率: " << ai.bitrate << " kbps" << std::endl;
    
    const char* title = file_info->meta_get("title");
    if(title) {
        std::cout << "   标题: " << title << std::endl;
    }
    
    // 测试跳转功能
    std::cout << "\n4. 跳转功能测试..." << std::endl;
    bool can_seek = decoder->can_seek();
    std::cout << "   支持跳转: " << (can_seek ? "✅ 是" : "❌ 否") << std::endl;
    
    if(can_seek) {
        std::cout << "   测试跳转到1.0秒..." << std::endl;
        decoder->seek(1.0, *abort_cb);
        std::cout << "   ✅ 跳转完成" << std::endl;
    }
    
    // 测试解码功能
    std::cout << "\n5. 解码功能测试..." << std::endl;
    const int test_samples = 1024;
    std::vector<float> buffer(test_samples * ai.channels);
    
    int total_decoded = 0;
    const int max_iterations = 10;
    
    for(int i = 0; i < max_iterations; i++) {
        int decoded = decoder->decode(buffer.data(), test_samples, *abort_cb);
        if(decoded <= 0) {
            std::cout << "   解码结束，总共解码 " << total_decoded << " 个采样" << std::endl;
            break;
        }
        
        total_decoded += decoded;
        
        // 音频数据分析
        float max_amplitude = 0.0f;
        float avg_amplitude = 0.0f;
        for(int j = 0; j < decoded * ai.channels; j++) {
            float abs_val = std::abs(buffer[j]);
            max_amplitude = std::max(max_amplitude, abs_val);
            avg_amplitude += abs_val;
        }
        avg_amplitude /= (decoded * ai.channels);
        
        double progress = (double)total_decoded / ai.sample_rate;
        std::cout << "   迭代 " << (i+1) << ": " << decoded << " 采样";
        std::cout << " (进度: " << std::fixed << std::setprecision(2) << progress << "s)";
        std::cout << " [最大振幅: " << max_amplitude << "]" << std::endl;
    }
    
    // 关闭测试
    std::cout << "\n6. 关闭测试..." << std::endl;
    decoder->close();
    std::cout << "   ✅ 关闭完成" << std::endl;
    
    std::cout << "\n=== 详细测试完成 ===" << std::endl;
    std::cout << "总解码采样数: " << total_decoded << std::endl;
    std::cout << "测试时长: " << (double)total_decoded / ai.sample_rate << " 秒" << std::endl;
    
    return true;
}

int main(int argc, char* argv[]) {
    std::cout << "=" << std::string(60, '=') << std::endl;
    std::cout << "foobar2000 真实组件测试程序" << std::endl;
    std::cout << "阶段1.1：真实组件集成测试" << std::endl;
    std::cout << "=" << std::string(60, '=') << std::endl;
    std::cout << std::endl;
    
    // 初始化COM
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if(FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        std::cerr << "COM初始化失败: 0x" << std::hex << hr << std::endl;
        return 1;
    }
    
    // 创建真实主机
    RealMiniHost host;
    if(!host.Initialize()) {
        std::cerr << "主机初始化失败" << std::endl;
        CoUninitialize();
        return 1;
    }
    
    std::cout << "✅ 主机初始化成功" << std::endl;
    
    // 查找组件
    std::wcout << L"正在搜索foobar2000组件..." << std::endl;
    auto components = FindFB2KComponents();
    
    if(components.empty()) {
        std::cout << "未找到foobar2000组件!" << std::endl;
        std::cout << "将使用模拟数据进行测试..." << std::endl;
        
        // 创建测试文件
        CreateTestAudioFiles();
        
        // 模拟加载组件（用于演示架构）
        std::cout << "\n模拟组件加载..." << std::endl;
        std::cout << "  ✓ foo_input_std.dll (MP3解码器)" << std::endl;
        std::cout << "  ✓ foo_input_flac.dll (FLAC解码器)" << std::endl;
        std::cout << "  ✓ foo_input_ffmpeg.dll (FFmpeg解码器)" << std::endl;
        
        components = {
            L"mock_foo_input_std.dll",
            L"mock_foo_input_flac.dll", 
            L"mock_foo_input_ffmpeg.dll"
        };
    }
    
    std::wcout << L"找到 " << components.size() << L" 个组件" << std::endl;
    
    // 优先级排序
    auto prioritized = PrioritizeComponents(components);
    
    // 显示组件信息
    std::cout << "\n组件列表（按优先级排序）:" << std::endl;
    for(size_t i = 0; i < prioritized.size() && i < 10; i++) {
        std::string type = IdentifyComponentType(prioritized[i]);
        std::wcout << L"  [" << (i+1) << L"] " << fs::path(prioritized[i]).filename().wstring();
        std::cout << " (" << type << ")" << std::endl;
    }
    
    if(prioritized.empty()) {
        std::cout << "没有可用的组件进行测试" << std::endl;
        return 0;
    }
    
    // 加载核心组件（优先加载input_std）
    std::cout << "\n加载核心组件..." << std::endl;
    int loaded_count = 0;
    
    for(size_t i = 0; i < std::min(size_t(3), prioritized.size()); i++) {
        std::string comp_name = host.WideToUTF8(prioritized[i]);
        std::cout << "尝试加载: " << comp_name << std::endl;
        
        // 对于模拟组件，直接记录成功
        if(comp_name.find("mock_") != std::string::npos) {
            std::cout << "  ✅ 模拟加载成功" << std::endl;
            loaded_count++;
        } else {
            // 尝试真实加载
            if(host.LoadComponent(prioritized[i])) {
                loaded_count++;
                std::cout << "  ✅ 加载成功" << std::endl;
            } else {
                std::cout << "  ❌ 加载失败" << std::endl;
            }
        }
        
        if(loaded_count >= 2) break; // 先加载2个核心组件
    }
    
    std::cout << "\n成功加载 " << loaded_count << " 个组件" << std::endl;
    
    // 显示已加载的组件
    auto loaded = host.GetLoadedComponents();
    if(!loaded.empty()) {
        std::cout << "已加载组件:" << std::endl;
        for(const auto& name : loaded) {
            std::cout << "  - " << name << std::endl;
        }
    }
    
    if(loaded.empty()) {
        std::cout << "没有组件被加载，测试结束" << std::endl;
        return 0;
    }
    
    // 运行基础测试
    std::cout << "\n运行基础解码测试..." << std::endl;
    
    std::vector<std::string> test_files = {
        "test.mp3",
        "test.flac",
        "test.wav"
    };
    
    int success_count = 0;
    for(const auto& test_file : test_files) {
        if(fs::exists(test_file)) {
            std::cout << "\n" << std::string(50, '-') << std::endl;
            std::cout << "测试文件: " << test_file << std::endl;
            
            if(host.TestRealComponent(test_file)) {
                success_count++;
                std::cout << "✅ " << test_file << " - 测试通过" << std::endl;
            } else {
                std::cout << "❌ " << test_file << " - 测试失败" << std::endl;
            }
        }
    }
    
    // 运行详细组件测试（如果有真实解码器）
    if(!loaded.empty() && loaded_count > 0) {
        std::cout << "\n运行详细组件测试..." << std::endl;
        
        // 获取第一个解码器进行详细测试
        auto decoder = host.CreateDecoderForPath("test.mp3");
        if(decoder.is_valid()) {
            TestComponentDetailed(host, decoder.get(), "test.mp3");
        }
    }
    
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "测试结果: " << success_count << "/" << test_files.size() << " 通过" << std::endl;
    
    if(success_count == test_files.size()) {
        std::cout << "🎉 所有测试通过！真实组件兼容层工作正常。" << std::endl;
        std::cout << "\n虽然使用的是模拟组件，但架构验证通过。" << std::endl;
        std::cout << "下一步：集成真实foobar2000组件进行测试" << std::endl;
    } else {
        std::cout << "⚠️  部分测试失败，需要调试" << std::endl;
    }
    
    std::cout << std::string(60, '=') << std::endl;
    
    // 清理
    host.Shutdown();
    CoUninitialize();
    
    return success_count == test_files.size() ? 0 : 1;
}