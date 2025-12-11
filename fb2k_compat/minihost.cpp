#include "minihost.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdarg>
#include <algorithm>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

namespace fb2k {

// GUID定义
// {E92063D0-C149-4B31-BF37-5F5C9D013C6A}
static const GUID IID_IInputDecoder = 
    { 0xe92063d0, 0xc149, 0x4b31, { 0xbf, 0x37, 0x5f, 0x5c, 0x9d, 0x01, 0x3c, 0x6a } };

// {9A1D5E4F-3B7C-4A2E-8F5C-1D9E6B3A2C4D}  
static const GUID CLSID_InputDecoderService =
    { 0x9a1d5e4f, 0x3b7c, 0x4a2e, { 0x8f, 0x5c, 0x1d, 0x9e, 0x6b, 0x3a, 0x2c, 0x4d } };

} // namespace fb2k

// 字符串转换
std::string wide_to_utf8(const std::wstring& wide) {
    if(wide.empty()) return "";
    
    int size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if(size <= 0) return "";
    
    std::string result(size - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, &result[0], size, nullptr, nullptr);
    return result;
}

std::wstring utf8_to_wide(const std::string& utf8) {
    if(utf8.empty()) return L"";
    
    int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if(size <= 0) return L"";
    
    std::wstring result(size - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &result[0], size);
    return result;
}

// 日志函数
void log_info(const char* format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    std::cout << "[INFO] " << buffer << std::endl;
}

void log_error(const char* format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    std::cerr << "[ERROR] " << buffer << std::endl;
}

// mini_host实现
bool mini_host::initialize() {
    log_info("Initializing mini host...");
    
    // 初始化COM
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if(FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        log_error("Failed to initialize COM: 0x%08X", hr);
        return false;
    }
    
    log_info("COM initialized successfully");
    return true;
}

bool mini_host::load_component(const std::wstring& dll_path) {
    log_info("Loading component: %s", wide_to_utf8(dll_path).c_str());
    
    // 加载DLL
    HMODULE module = LoadLibraryW(dll_path.c_str());
    if(!module) {
        log_error("Failed to load DLL: %lu", GetLastError());
        return false;
    }
    
    // 查找服务入口点
    auto get_service = (fb2k::get_service_func)GetProcAddress(module, "fb2k_get_service");
    if(!get_service) {
        // 尝试其他常见的入口点名
        get_service = (fb2k::get_service_func)GetProcAddress(module, "get_service");
        if(!get_service) {
            get_service = (fb2k::get_service_func)GetProcAddress(module, "_fb2k_get_service@8");
        }
    }
    
    if(!get_service) {
        log_error("No service entry point found in DLL");
        FreeLibrary(module);
        return false;
    }
    
    // 测试获取输入解码器服务
    void* service_ptr = nullptr;
    HRESULT hr = get_service(fb2k::CLSID_InputDecoderService, &service_ptr);
    if(FAILED(hr) || !service_ptr) {
        log_error("Failed to get input decoder service: 0x%08X", hr);
        FreeLibrary(module);
        return false;
    }
    
    // 获取组件名称
    std::string component_name = wide_to_utf8(dll_path);
    size_t pos = component_name.find_last_of("/\\");
    if(pos != std::string::npos) {
        component_name = component_name.substr(pos + 1);
    }
    pos = component_name.find_last_of('.');
    if(pos != std::string::npos) {
        component_name = component_name.substr(0, pos);
    }
    
    loaded_modules_.push_back(module);
    component_names_.push_back(component_name);
    get_service_ = get_service;
    
    log_info("Component loaded successfully: %s", component_name.c_str());
    return true;
}

mini_host::decoder_ptr mini_host::create_decoder_for_path(const std::string& path) {
    if(!get_service_) {
        log_error("No service factory available");
        return {};
    }
    
    // 获取输入解码器服务
    void* service_ptr = nullptr;
    HRESULT hr = get_service_(fb2k::CLSID_InputDecoderService, &service_ptr);
    if(FAILED(hr) || !service_ptr) {
        log_error("Failed to get input decoder service: 0x%08X", hr);
        return {};
    }
    
    // 尝试转换为输入解码器
    fb2k::input_decoder* decoder = static_cast<fb2k::input_decoder*>(service_ptr);
    
    // 检查是否支持该路径
    if(!decoder->is_our_path(path.c_str())) {
        log_info("Decoder does not support path: %s", path.c_str());
        decoder->Release();
        return {};
    }
    
    log_info("Created decoder for: %s", path.c_str());
    return mini_host::decoder_ptr(decoder);
}

std::vector<std::string> mini_host::get_loaded_components() const {
    return component_names_;
}

bool mini_host::test_decode(const std::string& audio_file) {
    log_info("Testing decode for: %s", audio_file.c_str());
    
    // 创建解码器
    auto decoder = create_decoder_for_path(audio_file);
    if(!decoder.is_valid()) {
        log_error("Failed to create decoder");
        return false;
    }
    
    // 创建文件信息对象
    fb2k::file_info_impl file_info;
    fb2k::abort_callback_dummy abort_cb;
    
    // 打开文件
    if(!decoder->open(audio_file.c_str(), file_info, abort_cb)) {
        log_error("Failed to open file");
        return false;
    }
    
    log_info("File opened successfully");
    log_info("  Length: %.2f seconds", file_info.get_length());
    log_info("  Sample rate: %u Hz", file_info.get_audio_info().sample_rate);
    log_info("  Channels: %u", file_info.get_audio_info().channels);
    log_info("  Bitrate: %u kbps", file_info.get_audio_info().bitrate);
    
    // 测试解码一小段
    const int test_samples = 1024;
    std::vector<float> buffer(test_samples * file_info.get_audio_info().channels);
    
    int decoded = decoder->decode(buffer.data(), test_samples, abort_cb);
    if(decoded <= 0) {
        log_error("Failed to decode audio");
        decoder->close();
        return false;
    }
    
    log_info("Successfully decoded %d samples", decoded);
    
    // 关闭解码器
    decoder->close();
    log_info("Decode test completed successfully");
    
    return true;
}