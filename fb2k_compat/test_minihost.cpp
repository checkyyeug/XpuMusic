// 阶段1测试程序
// 测试加载foobar2000组件并解码音频文件

#include "minihost.h"
#include <iostream>
#include <filesystem>
#include <vector>
#include <string>

namespace fs = std::filesystem;

// 查找foobar2000组件目录
std::vector<std::wstring> find_fb2k_components() {
    std::vector<std::wstring> components;
    
    // 常见安装路径
    std::vector<std::wstring> search_paths = {
        L"C:\\Program Files (x86)\\foobar2000\\components",
        L"C:\\Program Files\\foobar2000\\components",
        L"C:\\Users\\" + std::wstring(_wgetenv(L"USERNAME")) + L"\\AppData\\Roaming\\foobar2000\\user-components"
    };
    
    for(const auto& base_path : search_paths) {
        if(!fs::exists(base_path)) continue;
        
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
    }
    
    return components;
}

// 查找测试音频文件
std::string find_test_audio_file() {
    // 优先使用项目目录下的测试文件
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
    
    // 如果没有，让用户选择
    std::cout << "请输入要测试的音频文件路径: ";
    std::string path;
    std::getline(std::cin, path);
    return path;
}

int main(int argc, char* argv[]) {
    std::cout << "=== foobar2000 组件兼容测试程序 ===" << std::endl;
    std::cout << "阶段1：最小主机接口测试" << std::endl;
    std::cout << std::endl;
    
    // 创建主机
    mini_host host;
    if(!host.initialize()) {
        std::cerr << "初始化主机失败!" << std::endl;
        return 1;
    }
    
    std::cout << "正在搜索 foobar2000 组件..." << std::endl;
    auto components = find_fb2k_components();
    
    if(components.empty()) {
        std::cout << "未找到 foobar2000 组件!" << std::endl;
        std::cout << "请手动指定组件目录: ";
        std::string path;
        std::getline(std::cin, path);
        
        if(!fs::exists(path)) {
            std::cerr << "路径不存在!" << std::endl;
            return 1;
        }
        
        // 手动添加DLL文件
        for(const auto& entry : fs::directory_iterator(path)) {
            if(entry.is_regular_file() && entry.path().extension() == ".dll") {
                components.push_back(entry.path().wstring());
            }
        }
    }
    
    std::cout << "找到 " << components.size() << " 个组件:" << std::endl;
    for(size_t i = 0; i < components.size(); ++i) {
        std::wcout << L"  [" << i << L"] " << components[i] << std::endl;
    }
    
    // 加载核心组件（优先加载input_std）
    std::cout << std::endl << "正在加载组件..." << std::endl;
    
    int loaded_count = 0;
    for(const auto& component : components) {
        std::string name = wide_to_utf8(component);
        
        // 优先加载input_std组件
        if(name.find("input_std") != std::string::npos || 
           name.find("foo_input_std") != std::string::npos) {
            std::cout << "优先加载核心组件: " << name << std::endl;
            if(host.load_component(component)) {
                loaded_count++;
                break; // 先只加载一个核心组件进行测试
            }
        }
    }
    
    // 如果没找到input_std，尝试加载其他组件
    if(loaded_count == 0 && !components.empty()) {
        std::cout << "尝试加载第一个组件: " << wide_to_utf8(components[0]) << std::endl;
        if(host.load_component(components[0])) {
            loaded_count++;
        }
    }
    
    std::cout << std::endl << "成功加载 " << loaded_count << " 个组件" << std::endl;
    
    // 显示已加载的组件
    auto loaded = host.get_loaded_components();
    for(const auto& name : loaded) {
        std::cout << "  - " << name << std::endl;
    }
    
    if(loaded.empty()) {
        std::cout << "没有组件被加载，测试结束。" << std::endl;
        return 0;
    }
    
    // 测试解码
    std::cout << std::endl << "准备测试解码..." << std::endl;
    std::string test_file = find_test_audio_file();
    
    if(!fs::exists(test_file)) {
        std::cerr << "测试文件不存在: " << test_file << std::endl;
        return 1;
    }
    
    std::cout << "使用测试文件: " << test_file << std::endl;
    
    // 运行解码测试
    std::cout << std::endl << "开始解码测试..." << std::endl;
    bool success = host.test_decode(test_file);
    
    if(success) {
        std::cout << std::endl << "✅ 测试成功! foobar2000 组件兼容层工作正常。" << std::endl;
    } else {
        std::cout << std::endl << "❌ 测试失败，请检查错误信息。" << std::endl;
    }
    
    return success ? 0 : 1;
}