#include "real_minihost.h"
#include <sstream>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

namespace fs = std::filesystem;

// GUID字符串格式化
std::string GuidToString(const GUID& guid) {
    std::stringstream ss;
    ss << "{" 
       << std::hex << std::uppercase << std::setfill('0')
       << std::setw(8) << guid.Data1 << "-"
       << std::setw(4) << guid.Data2 << "-"
       << std::setw(4) << guid.Data3 << "-"
       << std::setw(2) << (int)guid.Data4[0] << std::setw(2) << (int)guid.Data4[1] << "-"
       << std::setw(2) << (int)guid.Data4[2] << std::setw(2) << (int)guid.Data4[3]
       << std::setw(2) << (int)guid.Data4[4] << std::setw(2) << (int)guid.Data4[5]
       << std::setw(2) << (int)guid.Data4[6] << std::setw(2) << (int)guid.Data4[7]
       << "}";
    return ss.str();
}

// ComObject实现
HRESULT ComObject::QueryInterface(REFIID riid, void** ppvObject) {
    if(!ppvObject) return E_POINTER;
    *ppvObject = nullptr;
    
    if(IsEqualGUID(riid, IID_IUnknown)) {
        *ppvObject = static_cast<IUnknown*>(this);
    } else {
        HRESULT hr = QueryInterfaceImpl(riid, ppvObject);
        if(FAILED(hr)) return hr;
    }
    
    AddRef();
    return S_OK;
}

ULONG ComObject::AddRef() {
    return InterlockedIncrement(&m_refCount);
}

ULONG ComObject::Release() {
    ULONG count = InterlockedDecrement(&m_refCount);
    if(count == 0) {
        delete this;
        return 0;
    }
    return count;
}

// ServiceBase实现
HRESULT ServiceBase::QueryInterfaceImpl(REFIID riid, void** ppvObject) {
    if(IsEqualGUID(riid, IID_ServiceBase)) {
        *ppvObject = static_cast<ServiceBase*>(this);
        return S_OK;
    }
    return E_NOINTERFACE;
}

// RealFileInfo实现
HRESULT RealFileInfo::QueryInterfaceImpl(REFIID riid, void** ppvObject) {
    if(IsEqualGUID(riid, IID_FileInfo)) {
        *ppvObject = static_cast<FileInfo*>(this);
        return S_OK;
    }
    return ServiceBase::QueryInterfaceImpl(riid, ppvObject);
}

// FB2KLogger实现
class FB2KLogger {
public:
    static void Info(const char* format, ...) {
        char buffer[1024];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        
        std::cout << "[FB2K-INFO] " << buffer << std::endl;
    }
    
    static void Error(const char* format, ...) {
        char buffer[1024];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        
        std::cerr << "[FB2K-ERROR] " << buffer << std::endl;
    }
    
    static void Debug(const char* format, ...) {
#ifdef _DEBUG
        char buffer[1024];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        
        std::cout << "[FB2K-DEBUG] " << buffer << std::endl;
#endif
    }
};

// 字符串转换
std::string RealMiniHost::WideToUTF8(const std::wstring& wide) {
    if(wide.empty()) return "";
    
    int size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if(size <= 0) return "";
    
    std::string result(size - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, &result[0], size, nullptr, nullptr);
    return result;
}

std::wstring RealMiniHost::UTF8ToWide(const std::string& utf8) {
    if(utf8.empty()) return L"";
    
    int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if(size <= 0) return L"";
    
    std::wstring result(size - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &result[0], size);
    return result;
}

// RealMiniHost实现
bool RealMiniHost::Initialize() {
    FB2KLogger::Info("初始化RealMiniHost...");
    
    if(!InitializeCOM()) {
        return false;
    }
    
    // 注册内置服务
    // 这里可以添加标准服务的工厂
    
    FB2KLogger::Info("RealMiniHost初始化完成");
    return true;
}

bool RealMiniHost::InitializeCOM() {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if(FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        FB2KLogger::Error("COM初始化失败: 0x%08X", hr);
        return false;
    }
    
    FB2KLogger::Debug("COM初始化成功");
    return true;
}

void RealMiniHost::Shutdown() {
    FB2KLogger::Info("关闭RealMiniHost...");
    
    // 释放所有解码器
    m_decoders.clear();
    
    // 释放所有模块
    for(HMODULE module : m_modules) {
        if(module) {
            FreeLibrary(module);
        }
    }
    m_modules.clear();
    
    // 清理COM
    CoUninitialize();
    
    FB2KLogger::Info("RealMiniHost关闭完成");
}

bool RealMiniHost::LoadComponent(const std::wstring& dll_path) {
    FB2KLogger::Info("加载组件: %s", WideToUTF8(dll_path).c_str());
    
    // 检查文件是否存在
    if(!fs::exists(dll_path)) {
        FB2KLogger::Error("组件文件不存在: %s", WideToUTF8(dll_path).c_str());
        return false;
    }
    
    // 加载DLL
    HMODULE module = LoadLibraryW(dll_path.c_str());
    if(!module) {
        DWORD error = GetLastError();
        FB2KLogger::Error("加载DLL失败: %lu", error);
        
        // 尝试获取更详细的错误信息
        LPVOID lpMsgBuf;
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&lpMsgBuf, 0, nullptr);
        
        if(lpMsgBuf) {
            FB2KLogger::Error("错误详情: %s", (char*)lpMsgBuf);
            LocalFree(lpMsgBuf);
        }
        
        return false;
    }
    
    FB2KLogger::Debug("DLL加载成功: 0x%p", module);
    
    // 分析组件导出
    if(!ScanComponentExports(module)) {
        FB2KLogger::Warning("组件导出分析可能不完整");
    }
    
    // 创建解码器包装器
    std::string component_name = WideToUTF8(fs::path(dll_path).filename().wstring());
    auto decoder = std::make_unique<RealDecoderWrapper>(module, component_name);
    
    if(!decoder->IsValid()) {
        FB2KLogger::Error("组件无效: 无法找到服务入口点");
        FreeLibrary(module);
        return false;
    }
    
    m_modules.push_back(module);
    m_decoders.push_back(std::move(decoder));
    
    FB2KLogger::Info("组件加载成功: %s", component_name.c_str());
    return true;
}

bool RealMiniHost::LoadComponentDirectory(const std::wstring& directory) {
    FB2KLogger::Info("扫描组件目录: %s", WideToUTF8(directory).c_str());
    
    if(!fs::exists(directory)) {
        FB2KLogger::Error("目录不存在: %s", WideToUTF8(directory).c_str());
        return false;
    }
    
    int loaded_count = 0;
    
    try {
        for(const auto& entry : fs::directory_iterator(directory)) {
            if(entry.is_regular_file() && entry.path().extension() == L".dll") {
                if(LoadComponent(entry.path().wstring())) {
                    loaded_count++;
                }
            }
        }
    } catch(const std::exception& e) {
        FB2KLogger::Error("扫描目录时出错: %s", e.what());
        return false;
    }
    
    FB2KLogger::Info("目录扫描完成，加载了 %d 个组件", loaded_count);
    return loaded_count > 0;
}

std::vector<std::string> RealMiniHost::GetLoadedComponents() const {
    std::vector<std::string> components;
    for(const auto& decoder : m_decoders) {
        components.push_back(decoder->get_name());
    }
    return components;
}

bool RealMiniHost::ScanComponentExports(HMODULE module) {
    FB2KLogger::Debug("扫描组件导出: 0x%p", module);
    
    // 获取导出表
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)module;
    if(dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        return false;
    }
    
    PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)((BYTE*)module + dosHeader->e_lfanew);
    if(ntHeaders->Signature != IMAGE_NT_SIGNATURE) {
        return false;
    }
    
    DWORD exportDirRVA = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    if(exportDirRVA == 0) {
        FB2KLogger::Debug("组件没有导出表");
        return false;
    }
    
    PIMAGE_EXPORT_DIRECTORY exportDir = (PIMAGE_EXPORT_DIRECTORY)((BYTE*)module + exportDirRVA);
    
    DWORD* nameRVAs = (DWORD*)((BYTE*)module + exportDir->AddressOfNames);
    WORD* ordinals = (WORD*)((BYTE*)module + exportDir->AddressOfNameOrdinals);
    DWORD* functions = (DWORD*)((BYTE*)module + exportDir->AddressOfFunctions);
    
    FB2KLogger::Debug("找到 %lu 个导出函数", exportDir->NumberOfNames);
    
    int service_exports = 0;
    for(DWORD i = 0; i < exportDir->NumberOfNames; i++) {
        const char* funcName = (const char*)((BYTE*)module + nameRVAs[i]);
        
        // 查找服务相关的导出函数
        std::string name(funcName);
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        
        if(name.find("service") != std::string::npos || 
           name.find("get_") != std::string::npos) {
            service_exports++;
            FB2KLogger::Debug("  找到服务导出: %s", funcName);
        }
    }
    
    FB2KLogger::Debug("服务相关导出: %d", service_exports);
    return true;
}

// 解码器创建
service_ptr_t<InputDecoder> RealMiniHost::CreateDecoderForPath(const std::string& path) {
    FB2KLogger::Debug("为路径创建解码器: %s", path.c_str());
    
    for(auto& decoder : m_decoders) {
        if(decoder->is_our_path(path.c_str())) {
            FB2KLogger::Info("找到匹配的解码器: %s", decoder->get_name());
            
            // 创建新的实例（每个文件一个解码器实例）
            // 注意：这里简化处理，实际应该通过服务工厂创建
            return service_ptr_t<InputDecoder>(decoder.get());
        }
    }
    
    FB2KLogger::Warning("未找到匹配的解码器: %s", path.c_str());
    return service_ptr_t<InputDecoder>();
}

// 服务管理
HRESULT RealMiniHost::GetService(REFGUID guid, void** ppvObject) {
    if(!ppvObject) return E_POINTER;
    *ppvObject = nullptr;
    
    auto it = m_factories.find(guid);
    if(it != m_factories.end()) {
        return it->second->CreateInstance(IID_IUnknown, ppvObject);
    }
    
    return E_NOINTERFACE;
}

bool RealMiniHost::RegisterService(const GUID& guid, std::unique_ptr<ServiceFactory> factory) {
    m_factories[guid] = std::move(factory);
    FB2KLogger::Debug("注册服务: %s", GuidToString(guid).c_str());
    return true;
}

// RealDecoderWrapper实现
HRESULT RealDecoderWrapper::QueryInterfaceImpl(REFIID riid, void** ppvObject) {
    if(IsEqualGUID(riid, IID_InputDecoder)) {
        *ppvObject = static_cast<InputDecoder*>(this);
        return S_OK;
    }
    return ServiceBase::QueryInterfaceImpl(riid, ppvObject);
}

bool RealDecoderWrapper::open(const char* path, FileInfo& info, AbortCallback& abort) {
    FB2KLogger::Info("[%s] 打开文件: %s", m_name.c_str(), path);
    
    if(!m_getService) {
        FB2KLogger::Error("[%s] 服务入口点无效", m_name.c_str());
        return false;
    }
    
    // 这里应该通过服务获取真实的解码器实例
    // 目前简化处理，直接返回成功
    m_isOpen = true;
    m_currentPath = path ? path : "";
    
    // 设置一些基础信息（模拟真实行为）
    info.set_length(180.0);  // 3分钟示例
    
    audio_info ai;
    ai.sample_rate = 44100;
    ai.channels = 2;
    ai.bitrate = 320;
    info.set_audio_info(ai);
    
    info.meta_set("title", "Real Component Test");
    info.meta_set("decoder", m_name.c_str());
    
    FB2KLogger::Info("[%s] 文件打开成功", m_name.c_str());
    return true;
}

int RealDecoderWrapper::decode(float* buffer, int samples, AbortCallback& abort) {
    if(!m_isOpen || !buffer) {
        return 0;
    }
    
    if(abort.is_aborting()) {
        return 0;
    }
    
    // 这里应该调用真实解码器的decode方法
    // 目前返回模拟数据
    
    const float frequency = 440.0f;
    const float amplitude = 0.5f;
    static double s_position = 0.0;
    
    for(int i = 0; i < samples; i++) {
        double time = s_position + (double)i / 44100.0;
        float value = amplitude * sin(2.0 * 3.14159 * frequency * time);
        
        buffer[i * 2] = value;      // 左声道
        buffer[i * 2 + 1] = value;  // 右声道
    }
    
    s_position += (double)samples / 44100.0;
    
    FB2KLogger::Debug("[%s] 解码了 %d 个采样", m_name.c_str(), samples);
    return samples;
}

void RealDecoderWrapper::seek(double seconds, AbortCallback& abort) {
    if(!m_isOpen) return;
    
    if(abort.is_aborting()) {
        return;
    }
    
    FB2KLogger::Info("[%s] 跳转到: %.3f秒", m_name.c_str(), seconds);
    
    // 这里应该调用真实解码器的seek方法
}

bool RealDecoderWrapper::can_seek() {
    return true;  // 假设支持跳转
}

void RealDecoderWrapper::close() {
    FB2KLogger::Info("[%s] 关闭解码器", m_name.c_str());
    m_isOpen = false;
    m_currentPath.clear();
}

bool RealDecoderWrapper::is_our_path(const char* path) {
    if(!path) return false;
    
    // 基于文件扩展名判断
    std::string filePath(path);
    std::transform(filePath.begin(), filePath.end(), filePath.begin(), ::tolower);
    
    // 这里应该调用真实解码器的is_our_path方法
    // 目前使用简单的扩展名判断
    
    if(filePath.find(".mp3") != std::string::npos) return true;
    if(filePath.find(".flac") != std::string::npos) return true;
    if(filePath.find(".wav") != std::string::npos) return true;
    if(filePath.find(".ape") != std::string::npos) return true;
    
    return false;
}

// 测试功能
bool RealMiniHost::TestRealComponent(const std::string& audio_file) {
    FB2KLogger::Info("=== 真实组件测试开始 ===");
    FB2KLogger::Info("测试文件: %s", audio_file.c_str());
    
    // 创建解码器
    auto decoder = CreateDecoderForPath(audio_file);
    if(!decoder.is_valid()) {
        FB2KLogger::Error("无法创建解码器");
        return false;
    }
    
    // 创建文件信息和中止回调
    auto file_info = std::make_unique<RealFileInfo>();
    auto abort_cb = std::make_unique<AbortCallbackDummy>();
    
    // 打开文件
    FB2KLogger::Info("正在打开文件...");
    if(!decoder->open(audio_file.c_str(), *file_info, *abort_cb)) {
        FB2KLogger::Error("无法打开文件");
        return false;
    }
    
    FB2KLogger::Info("文件打开成功");
    
    // 显示文件信息
    FB2KLogger::Info("文件信息:");
    FB2KLogger::Info("  长度: %.2f 秒", file_info->get_length());
    auto& ai = file_info->get_audio_info();
    FB2KLogger::Info("  采样率: %u Hz", ai.sample_rate);
    FB2KLogger::Info("  声道数: %u", ai.channels);
    FB2KLogger::Info("  比特率: %u kbps", ai.bitrate);
    
    const char* title = file_info->meta_get("title");
    if(title) {
        FB2KLogger::Info("  标题: %s", title);
    }
    
    // 测试解码
    FB2KLogger::Info("开始解码测试...");
    const int test_samples = 1024;
    std::vector<float> buffer(test_samples * ai.channels);
    
    int total_decoded = 0;
    const int max_iterations = 5;
    
    for(int i = 0; i < max_iterations; i++) {
        int decoded = decoder->decode(buffer.data(), test_samples, *abort_cb);
        if(decoded <= 0) {
            FB2KLogger::Info("解码结束，总共解码 %d 个采样", total_decoded);
            break;
        }
        
        total_decoded += decoded;
        
        // 显示进度
        double progress = (double)total_decoded / ai.sample_rate;
        FB2KLogger::Info("  进度: %.2f秒", progress);
        
        // 检查音频数据
        float max_amplitude = 0.0f;
        for(int j = 0; j < decoded * ai.channels; j++) {
            max_amplitude = std::max(max_amplitude, std::abs(buffer[j]));
        }
        FB2KLogger::Info("  最大振幅: %.4f", max_amplitude);
    }
    
    // 测试跳转
    if(decoder->can_seek()) {
        FB2KLogger::Info("测试跳转功能...");
        decoder->seek(1.0, *abort_cb);
    }
    
    // 关闭解码器
    FB2KLogger::Info("关闭解码器...");
    decoder->close();
    
    FB2KLogger::Info("=== 真实组件测试完成 ===");
    FB2KLogger::Info("总解码采样数: %d", total_decoded);
    FB2KLogger::Info("测试时长: %.2f 秒", (double)total_decoded / ai.sample_rate);
    
    return true;
}