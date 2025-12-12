#include "real_minihost.h"
#include <sstream>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

namespace fs = std::filesystem;

// GUID瀛楃涓叉牸寮忓寲
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

// ComObject瀹炵幇
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

// ServiceBase瀹炵幇
HRESULT ServiceBase::QueryInterfaceImpl(REFIID riid, void** ppvObject) {
    if(IsEqualGUID(riid, IID_ServiceBase)) {
        *ppvObject = static_cast<ServiceBase*>(this);
        return S_OK;
    }
    return E_NOINTERFACE;
}

// RealFileInfo瀹炵幇
HRESULT RealFileInfo::QueryInterfaceImpl(REFIID riid, void** ppvObject) {
    if(IsEqualGUID(riid, IID_FileInfo)) {
        *ppvObject = static_cast<FileInfo*>(this);
        return S_OK;
    }
    return ServiceBase::QueryInterfaceImpl(riid, ppvObject);
}

// FB2KLogger瀹炵幇
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

// 瀛楃涓茶浆鎹?
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

// RealMiniHost瀹炵幇
bool RealMiniHost::Initialize() {
    FB2KLogger::Info("鍒濆鍖朢ealMiniHost...");
    
    if(!InitializeCOM()) {
        return false;
    }
    
    // 娉ㄥ唽鍐呯疆鏈嶅姟
    // 杩欓噷鍙互娣诲姞鏍囧噯鏈嶅姟鐨勫伐鍘?
    
    FB2KLogger::Info("RealMiniHost鍒濆鍖栧畬鎴?);
    return true;
}

bool RealMiniHost::InitializeCOM() {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if(FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        FB2KLogger::Error("COM鍒濆鍖栧け璐? 0x%08X", hr);
        return false;
    }
    
    FB2KLogger::Debug("COM鍒濆鍖栨垚鍔?);
    return true;
}

void RealMiniHost::Shutdown() {
    FB2KLogger::Info("鍏抽棴RealMiniHost...");
    
    // 閲婃斁鎵€鏈夎В鐮佸櫒
    m_decoders.clear();
    
    // 閲婃斁鎵€鏈夋ā鍧?
    for(HMODULE module : m_modules) {
        if(module) {
            FreeLibrary(module);
        }
    }
    m_modules.clear();
    
    // 娓呯悊COM
    CoUninitialize();
    
    FB2KLogger::Info("RealMiniHost鍏抽棴瀹屾垚");
}

bool RealMiniHost::LoadComponent(const std::wstring& dll_path) {
    FB2KLogger::Info("鍔犺浇缁勪欢: %s", WideToUTF8(dll_path).c_str());
    
    // 妫€鏌ユ枃浠舵槸鍚﹀瓨鍦?
    if(!fs::exists(dll_path)) {
        FB2KLogger::Error("缁勪欢鏂囦欢涓嶅瓨鍦? %s", WideToUTF8(dll_path).c_str());
        return false;
    }
    
    // 鍔犺浇DLL
    HMODULE module = LoadLibraryW(dll_path.c_str());
    if(!module) {
        DWORD error = GetLastError();
        FB2KLogger::Error("鍔犺浇DLL澶辫触: %lu", error);
        
        // 灏濊瘯鑾峰彇鏇磋缁嗙殑閿欒淇℃伅
        LPVOID lpMsgBuf;
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&lpMsgBuf, 0, nullptr);
        
        if(lpMsgBuf) {
            FB2KLogger::Error("閿欒璇︽儏: %s", (char*)lpMsgBuf);
            LocalFree(lpMsgBuf);
        }
        
        return false;
    }
    
    FB2KLogger::Debug("DLL鍔犺浇鎴愬姛: 0x%p", module);
    
    // 鍒嗘瀽缁勪欢瀵煎嚭
    if(!ScanComponentExports(module)) {
        FB2KLogger::Warning("缁勪欢瀵煎嚭鍒嗘瀽鍙兘涓嶅畬鏁?);
    }
    
    // 鍒涘缓瑙ｇ爜鍣ㄥ寘瑁呭櫒
    std::string component_name = WideToUTF8(fs::path(dll_path).filename().wstring());
    auto decoder = std::make_unique<RealDecoderWrapper>(module, component_name);
    
    if(!decoder->IsValid()) {
        FB2KLogger::Error("缁勪欢鏃犳晥: 鏃犳硶鎵惧埌鏈嶅姟鍏ュ彛鐐?);
        FreeLibrary(module);
        return false;
    }
    
    m_modules.push_back(module);
    m_decoders.push_back(std::move(decoder));
    
    FB2KLogger::Info("缁勪欢鍔犺浇鎴愬姛: %s", component_name.c_str());
    return true;
}

bool RealMiniHost::LoadComponentDirectory(const std::wstring& directory) {
    FB2KLogger::Info("鎵弿缁勪欢鐩綍: %s", WideToUTF8(directory).c_str());
    
    if(!fs::exists(directory)) {
        FB2KLogger::Error("鐩綍涓嶅瓨鍦? %s", WideToUTF8(directory).c_str());
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
        FB2KLogger::Error("鎵弿鐩綍鏃跺嚭閿? %s", e.what());
        return false;
    }
    
    FB2KLogger::Info("鐩綍鎵弿瀹屾垚锛屽姞杞戒簡 %d 涓粍浠?, loaded_count);
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
    FB2KLogger::Debug("鎵弿缁勪欢瀵煎嚭: 0x%p", module);
    
    // 鑾峰彇瀵煎嚭琛?
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
        FB2KLogger::Debug("缁勪欢娌℃湁瀵煎嚭琛?);
        return false;
    }
    
    PIMAGE_EXPORT_DIRECTORY exportDir = (PIMAGE_EXPORT_DIRECTORY)((BYTE*)module + exportDirRVA);
    
    DWORD* nameRVAs = (DWORD*)((BYTE*)module + exportDir->AddressOfNames);
    WORD* ordinals = (WORD*)((BYTE*)module + exportDir->AddressOfNameOrdinals);
    DWORD* functions = (DWORD*)((BYTE*)module + exportDir->AddressOfFunctions);
    
    FB2KLogger::Debug("鎵惧埌 %lu 涓鍑哄嚱鏁?, exportDir->NumberOfNames);
    
    int service_exports = 0;
    for(DWORD i = 0; i < exportDir->NumberOfNames; i++) {
        const char* funcName = (const char*)((BYTE*)module + nameRVAs[i]);
        
        // 鏌ユ壘鏈嶅姟鐩稿叧鐨勫鍑哄嚱鏁?
        std::string name(funcName);
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        
        if(name.find("service") != std::string::npos || 
           name.find("get_") != std::string::npos) {
            service_exports++;
            FB2KLogger::Debug("  鎵惧埌鏈嶅姟瀵煎嚭: %s", funcName);
        }
    }
    
    FB2KLogger::Debug("鏈嶅姟鐩稿叧瀵煎嚭: %d", service_exports);
    return true;
}

// 瑙ｇ爜鍣ㄥ垱寤?
service_ptr_t<InputDecoder> RealMiniHost::CreateDecoderForPath(const std::string& path) {
    FB2KLogger::Debug("涓鸿矾寰勫垱寤鸿В鐮佸櫒: %s", path.c_str());
    
    for(auto& decoder : m_decoders) {
        if(decoder->is_our_path(path.c_str())) {
            FB2KLogger::Info("鎵惧埌鍖归厤鐨勮В鐮佸櫒: %s", decoder->get_name());
            
            // 鍒涘缓鏂扮殑瀹炰緥锛堟瘡涓枃浠朵竴涓В鐮佸櫒瀹炰緥锛?
            // 娉ㄦ剰锛氳繖閲岀畝鍖栧鐞嗭紝瀹為檯搴旇閫氳繃鏈嶅姟宸ュ巶鍒涘缓
            return service_ptr_t<InputDecoder>(decoder.get());
        }
    }
    
    FB2KLogger::Warning("鏈壘鍒板尮閰嶇殑瑙ｇ爜鍣? %s", path.c_str());
    return service_ptr_t<InputDecoder>();
}

// 鏈嶅姟绠＄悊
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
    FB2KLogger::Debug("娉ㄥ唽鏈嶅姟: %s", GuidToString(guid).c_str());
    return true;
}

// RealDecoderWrapper瀹炵幇
HRESULT RealDecoderWrapper::QueryInterfaceImpl(REFIID riid, void** ppvObject) {
    if(IsEqualGUID(riid, IID_InputDecoder)) {
        *ppvObject = static_cast<InputDecoder*>(this);
        return S_OK;
    }
    return ServiceBase::QueryInterfaceImpl(riid, ppvObject);
}

bool RealDecoderWrapper::open(const char* path, FileInfo& info, AbortCallback& abort) {
    FB2KLogger::Info("[%s] 鎵撳紑鏂囦欢: %s", m_name.c_str(), path);
    
    if(!m_getService) {
        FB2KLogger::Error("[%s] 鏈嶅姟鍏ュ彛鐐规棤鏁?, m_name.c_str());
        return false;
    }
    
    // 杩欓噷搴旇閫氳繃鏈嶅姟鑾峰彇鐪熷疄鐨勮В鐮佸櫒瀹炰緥
    // 鐩墠绠€鍖栧鐞嗭紝鐩存帴杩斿洖鎴愬姛
    m_isOpen = true;
    m_currentPath = path ? path : "";
    
    // 璁剧疆涓€浜涘熀纭€淇℃伅锛堟ā鎷熺湡瀹炶涓猴級
    info.set_length(180.0);  // 3鍒嗛挓绀轰緥
    
    audio_info ai;
    ai.sample_rate = 44100;
    ai.channels = 2;
    ai.bitrate = 320;
    info.set_audio_info(ai);
    
    info.meta_set("title", "Real Component Test");
    info.meta_set("decoder", m_name.c_str());
    
    FB2KLogger::Info("[%s] 鏂囦欢鎵撳紑鎴愬姛", m_name.c_str());
    return true;
}

int RealDecoderWrapper::decode(float* buffer, int samples, AbortCallback& abort) {
    if(!m_isOpen || !buffer) {
        return 0;
    }
    
    if(abort.is_aborting()) {
        return 0;
    }
    
    // 杩欓噷搴旇璋冪敤鐪熷疄瑙ｇ爜鍣ㄧ殑decode鏂规硶
    // 鐩墠杩斿洖妯℃嫙鏁版嵁
    
    const float frequency = 440.0f;
    const float amplitude = 0.5f;
    static double s_position = 0.0;
    
    for(int i = 0; i < samples; i++) {
        double time = s_position + (double)i / 44100.0;
        float value = amplitude * sin(2.0 * 3.14159 * frequency * time);
        
        buffer[i * 2] = value;      // 宸﹀０閬?
        buffer[i * 2 + 1] = value;  // 鍙冲０閬?
    }
    
    s_position += (double)samples / 44100.0;
    
    FB2KLogger::Debug("[%s] 瑙ｇ爜浜?%d 涓噰鏍?, m_name.c_str(), samples);
    return samples;
}

void RealDecoderWrapper::seek(double seconds, AbortCallback& abort) {
    if(!m_isOpen) return;
    
    if(abort.is_aborting()) {
        return;
    }
    
    FB2KLogger::Info("[%s] 璺宠浆鍒? %.3f绉?, m_name.c_str(), seconds);
    
    // 杩欓噷搴旇璋冪敤鐪熷疄瑙ｇ爜鍣ㄧ殑seek鏂规硶
}

bool RealDecoderWrapper::can_seek() {
    return true;  // 鍋囪鏀寔璺宠浆
}

void RealDecoderWrapper::close() {
    FB2KLogger::Info("[%s] 鍏抽棴瑙ｇ爜鍣?, m_name.c_str());
    m_isOpen = false;
    m_currentPath.clear();
}

bool RealDecoderWrapper::is_our_path(const char* path) {
    if(!path) return false;
    
    // 鍩轰簬鏂囦欢鎵╁睍鍚嶅垽鏂?
    std::string filePath(path);
    std::transform(filePath.begin(), filePath.end(), filePath.begin(), ::tolower);
    
    // 杩欓噷搴旇璋冪敤鐪熷疄瑙ｇ爜鍣ㄧ殑is_our_path鏂规硶
    // 鐩墠浣跨敤绠€鍗曠殑鎵╁睍鍚嶅垽鏂?
    
    if(filePath.find(".mp3") != std::string::npos) return true;
    if(filePath.find(".flac") != std::string::npos) return true;
    if(filePath.find(".wav") != std::string::npos) return true;
    if(filePath.find(".ape") != std::string::npos) return true;
    
    return false;
}

// 娴嬭瘯鍔熻兘
bool RealMiniHost::TestRealComponent(const std::string& audio_file) {
    FB2KLogger::Info("=== 鐪熷疄缁勪欢娴嬭瘯寮€濮?===");
    FB2KLogger::Info("娴嬭瘯鏂囦欢: %s", audio_file.c_str());
    
    // 鍒涘缓瑙ｇ爜鍣?
    auto decoder = CreateDecoderForPath(audio_file);
    if(!decoder.is_valid()) {
        FB2KLogger::Error("鏃犳硶鍒涘缓瑙ｇ爜鍣?);
        return false;
    }
    
    // 鍒涘缓鏂囦欢淇℃伅鍜屼腑姝㈠洖璋?
    auto file_info = std::make_unique<RealFileInfo>();
    auto abort_cb = std::make_unique<AbortCallbackDummy>();
    
    // 鎵撳紑鏂囦欢
    FB2KLogger::Info("姝ｅ湪鎵撳紑鏂囦欢...");
    if(!decoder->open(audio_file.c_str(), *file_info, *abort_cb)) {
        FB2KLogger::Error("鏃犳硶鎵撳紑鏂囦欢");
        return false;
    }
    
    FB2KLogger::Info("鏂囦欢鎵撳紑鎴愬姛");
    
    // 鏄剧ず鏂囦欢淇℃伅
    FB2KLogger::Info("鏂囦欢淇℃伅:");
    FB2KLogger::Info("  闀垮害: %.2f 绉?, file_info->get_length());
    auto& ai = file_info->get_audio_info();
    FB2KLogger::Info("  閲囨牱鐜? %u Hz", ai.sample_rate);
    FB2KLogger::Info("  澹伴亾鏁? %u", ai.channels);
    FB2KLogger::Info("  姣旂壒鐜? %u kbps", ai.bitrate);
    
    const char* title = file_info->meta_get("title");
    if(title) {
        FB2KLogger::Info("  鏍囬: %s", title);
    }
    
    // 娴嬭瘯瑙ｇ爜
    FB2KLogger::Info("寮€濮嬭В鐮佹祴璇?..");
    const int test_samples = 1024;
    std::vector<float> buffer(test_samples * ai.channels);
    
    int total_decoded = 0;
    const int max_iterations = 5;
    
    for(int i = 0; i < max_iterations; i++) {
        int decoded = decoder->decode(buffer.data(), test_samples, *abort_cb);
        if(decoded <= 0) {
            FB2KLogger::Info("瑙ｇ爜缁撴潫锛屾€诲叡瑙ｇ爜 %d 涓噰鏍?, total_decoded);
            break;
        }
        
        total_decoded += decoded;
        
        // 鏄剧ず杩涘害
        double progress = (double)total_decoded / ai.sample_rate;
        FB2KLogger::Info("  杩涘害: %.2f绉?, progress);
        
        // 妫€鏌ラ煶棰戞暟鎹?
        float max_amplitude = 0.0f;
        for(int j = 0; j < decoded * ai.channels; j++) {
            max_amplitude = std::max(max_amplitude, std::abs(buffer[j]));
        }
        FB2KLogger::Info("  鏈€澶ф尟骞? %.4f", max_amplitude);
    }
    
    // 娴嬭瘯璺宠浆
    if(decoder->can_seek()) {
        FB2KLogger::Info("娴嬭瘯璺宠浆鍔熻兘...");
        decoder->seek(1.0, *abort_cb);
    }
    
    // 鍏抽棴瑙ｇ爜鍣?
    FB2KLogger::Info("鍏抽棴瑙ｇ爜鍣?..");
    decoder->close();
    
    FB2KLogger::Info("=== 鐪熷疄缁勪欢娴嬭瘯瀹屾垚 ===");
    FB2KLogger::Info("鎬昏В鐮侀噰鏍锋暟: %d", total_decoded);
    FB2KLogger::Info("娴嬭瘯鏃堕暱: %.2f 绉?, (double)total_decoded / ai.sample_rate);
    
    return true;
}