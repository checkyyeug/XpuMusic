#include "fb2k_component_system.h"
#include <windows.h>
#include <shlwapi.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <set>

#pragma comment(lib, "shlwapi.lib")

namespace fb2k {

// 缁勪欢绠＄悊鍣ㄥ疄鐜?
fb2k_component_manager_impl::fb2k_component_manager_impl() {
    std::cout << "[FB2K Component] 鍒涘缓缁勪欢绠＄悊鍣? << std::endl;
}

fb2k_component_manager_impl::~fb2k_component_manager_impl() {
    std::cout << "[FB2K Component] 閿€姣佺粍浠剁鐞嗗櫒" << std::endl;
    UnloadAllComponents();
}

HRESULT fb2k_component_manager_impl::do_initialize() {
    std::cout << "[FB2K Component] 鍒濆鍖栫粍浠剁鐞嗗櫒" << std::endl;
    
    // 鍒涘缓鎻掍欢鍔犺浇鍣?
    plugin_loader_ = std::make_unique<fb2k_plugin_loader_impl>();
    
    // 璁剧疆榛樿缁勪欢鐩綍
    char buffer[MAX_PATH];
    if (GetModuleFileNameA(nullptr, buffer, MAX_PATH) > 0) {
        std::string exe_path = buffer;
        size_t last_slash = exe_path.find_last_of("\\/");
        if (last_slash != std::string::npos) {
            components_directory_ = exe_path.substr(0, last_slash) + "\\components";
        }
    }
    
    // 鎵弿榛樿缁勪欢鐩綍
    if (!components_directory_.empty() && std::filesystem::exists(components_directory_)) {
        ScanComponents(components_directory_.c_str());
    }
    
    return S_OK;
}

HRESULT fb2k_component_manager_impl::do_shutdown() {
    std::cout << "[FB2K Component] 鍏抽棴缁勪欢绠＄悊鍣? << std::endl;
    UnloadAllComponents();
    plugin_loader_.reset();
    return S_OK;
}

HRESULT fb2k_component_manager_impl::ScanComponents(const char* directory) {
    if (!directory) return E_POINTER;
    
    std::string dir_path = directory;
    if (!std::filesystem::exists(dir_path)) {
        add_error("缁勪欢鐩綍涓嶅瓨鍦? " + dir_path);
        return E_FAIL;
    }
    
    std::cout << "[FB2K Component] 鎵弿缁勪欢鐩綍: " << dir_path << std::endl;
    
    return scan_component_directory(dir_path) ? S_OK : E_FAIL;
}

HRESULT fb2k_component_manager_impl::EnumComponents(component_info** components, DWORD* count) {
    if (!count) return E_POINTER;
    
    std::lock_guard<std::mutex> lock(components_mutex_);
    
    DWORD component_count = static_cast<DWORD>(components_.size());
    if (!components) {
        *count = component_count;
        return S_OK;
    }
    
    // 杩欓噷搴旇鍒嗛厤鍐呭瓨骞跺～鍏呯粍浠朵俊鎭?
    // 绠€鍖栧疄鐜帮細鐩存帴杩斿洖鏁伴噺
    *count = component_count;
    return S_OK;
}

HRESULT fb2k_component_manager_impl::GetComponentCount(DWORD* count) {
    if (!count) return E_POINTER;
    
    std::lock_guard<std::mutex> lock(components_mutex_);
    *count = static_cast<DWORD>(components_.size());
    return S_OK;
}

HRESULT fb2k_component_manager_impl::FindComponent(const char* guid, IFB2KComponent** component) {
    if (!guid || !component) return E_POINTER;
    
    std::lock_guard<std::mutex> lock(components_mutex_);
    
    auto it = components_.find(guid);
    if (it == components_.end()) {
        return E_FAIL;
    }
    
    if (it->second.component) {
        it->second.component->AddRef();
        *component = it->second.component;
        return S_OK;
    }
    
    return E_FAIL;
}

HRESULT fb2k_component_manager_impl::LoadComponent(const char* file_path) {
    if (!file_path) return E_POINTER;
    
    return load_component_file(file_path) ? S_OK : E_FAIL;
}

HRESULT fb2k_component_manager_impl::UnloadComponent(const char* guid) {
    if (!guid) return E_POINTER;
    
    return unload_component_internal(guid) ? S_OK : E_FAIL;
}

HRESULT fb2k_component_manager_impl::ReloadComponent(const char* guid) {
    if (!guid) return E_POINTER;
    
    std::lock_guard<std::mutex> lock(components_mutex_);
    
    auto it = components_.find(guid);
    if (it == components_.end()) {
        return E_FAIL;
    }
    
    const std::string& file_path = it->second.info.file_path;
    
    // 鍏堝嵏杞?
    if (!unload_component_internal(guid)) {
        return E_FAIL;
    }
    
    // 鍐嶉噸鏂板姞杞?
    return load_component_file(file_path) ? S_OK : E_FAIL;
}

HRESULT fb2k_component_manager_impl::LoadAllComponents() {
    std::lock_guard<std::mutex> lock(components_mutex_);
    
    std::cout << "[FB2K Component] 鍔犺浇鎵€鏈夌粍浠?.." << std::endl;
    
    bool all_success = true;
    std::vector<std::string> load_order;
    
    // 鎸夊姞杞介『搴忔帓搴?
    for (const auto& [guid, entry] : components_) {
        if (!entry.info.is_loaded && entry.info.is_enabled) {
            load_order.push_back(guid);
        }
    }
    
    // 鎸夊姞杞介『搴忔帓搴?
    std::sort(load_order.begin(), load_order.end(), 
        [this](const std::string& a, const std::string& b) {
            return components_[a].info.load_order < components_[b].info.load_order;
        });
    
    // 瑙ｆ瀽渚濊禆鍏崇郴
    if (!resolve_dependencies()) {
        add_error("渚濊禆鍏崇郴瑙ｆ瀽澶辫触");
        all_success = false;
    }
    
    // 鍔犺浇缁勪欢
    for (const std::string& guid : load_order) {
        auto& entry = components_[guid];
        
        if (!entry.dependency_satisfied) {
            std::cout << "[FB2K Component] 璺宠繃鍔犺浇锛堜緷璧栨湭婊¤冻锛? " << entry.info.name << std::endl;
            continue;
        }
        
        std::cout << "[FB2K Component] 鍔犺浇缁勪欢: " << entry.info.name << std::endl;
        
        // 杩欓噷搴旇瀹為檯鍔犺浇缁勪欢
        // 绠€鍖栧疄鐜帮細鏍囪涓哄凡鍔犺浇
        entry.info.is_loaded = true;
        
        // 璋冪敤缁勪欢鐨勫垵濮嬪寲
        if (entry.component) {
            IFB2KInitQuit* init_quit = nullptr;
            if (SUCCEEDED(entry.component->QueryInterface(IID_IFB2KInitQuit, (void**)&init_quit))) {
                init_quit->OnInit();
                init_quit->Release();
            }
        }
    }
    
    // 璋冪敤鎵€鏈夌粍浠剁殑OnSystemInit
    for (auto& [guid, entry] : components_) {
        if (entry.info.is_loaded && entry.component) {
            IFB2KInitQuit* init_quit = nullptr;
            if (SUCCEEDED(entry.component->QueryInterface(IID_IFB2KInitQuit, (void**)&init_quit))) {
                init_quit->OnSystemInit();
                init_quit->Release();
            }
        }
    }
    
    std::cout << "[FB2K Component] 缁勪欢鍔犺浇瀹屾垚" << std::endl;
    return all_success ? S_OK : S_FALSE;
}

HRESULT fb2k_component_manager_impl::UnloadAllComponents() {
    std::lock_guard<std::mutex> lock(components_mutex_);
    
    std::cout << "[FB2K Component] 鍗歌浇鎵€鏈夌粍浠?.." << std::endl;
    
    // 鍙嶅悜鍗歌浇锛堟寜鍔犺浇椤哄簭鐨勯€嗗簭锛?
    std::vector<std::string> unload_order;
    for (const auto& [guid, entry] : components_) {
        if (entry.info.is_loaded) {
            unload_order.push_back(guid);
        }
    }
    
    // 鎸夊姞杞介『搴忕殑閫嗗簭鎺掑簭
    std::sort(unload_order.begin(), unload_order.end(),
        [this](const std::string& a, const std::string& b) {
            return components_[a].info.load_order > components_[b].info.load_order;
        });
    
    // 璋冪敤鎵€鏈夌粍浠剁殑OnSystemQuit
    for (const std::string& guid : unload_order) {
        auto& entry = components_[guid];
        if (entry.component) {
            IFB2KInitQuit* init_quit = nullptr;
            if (SUCCEEDED(entry.component->QueryInterface(IID_IFB2KInitQuit, (void**)&init_quit))) {
                init_quit->OnSystemQuit();
                init_quit->Release();
            }
        }
    }
    
    // 鍗歌浇缁勪欢
    for (const std::string& guid : unload_order) {
        auto& entry = components_[guid];
        
        std::cout << "[FB2K Component] 鍗歌浇缁勪欢: " << entry.info.name << std::endl;
        
        // 璋冪敤缁勪欢鐨勯€€鍑?
        if (entry.component) {
            IFB2KInitQuit* init_quit = nullptr;
            if (SUCCEEDED(entry.component->QueryInterface(IID_IFB2KInitQuit, (void**)&init_quit))) {
                init_quit->OnQuit();
                init_quit->Release();
            }
            
            entry.component->Release();
            entry.component = nullptr;
        }
        
        // 鍗歌浇DLL
        if (entry.dll_handle) {
            FreeLibrary(entry.dll_handle);
            entry.dll_handle = nullptr;
        }
        
        entry.info.is_loaded = false;
    }
    
    std::cout << "[FB2K Component] 缁勪欢鍗歌浇瀹屾垚" << std::endl;
    return S_OK;
}

HRESULT fb2k_component_manager_impl::EnableComponent(const char* guid, bool enable) {
    if (!guid) return E_POINTER;
    
    std::lock_guard<std::mutex> lock(components_mutex_);
    
    auto it = components_.find(guid);
    if (it == components_.end()) {
        return E_FAIL;
    }
    
    it->second.info.is_enabled = enable;
    
    if (it->second.component) {
        it->second.component->EnableService(enable);
    }
    
    std::cout << "[FB2K Component] " << (enable ? "鍚敤" : "绂佺敤") << "缁勪欢: " << it->second.info.name << std::endl;
    return S_OK;
}

HRESULT fb2k_component_manager_impl::IsComponentEnabled(const char* guid, bool* enabled) {
    if (!guid || !enabled) return E_POINTER;
    
    std::lock_guard<std::mutex> lock(components_mutex_);
    
    auto it = components_.find(guid);
    if (it == components_.end()) {
        return E_FAIL;
    }
    
    *enabled = it->second.info.is_enabled;
    return S_OK;
}

HRESULT fb2k_component_manager_impl::SetComponentLoadOrder(const char* guid, DWORD order) {
    if (!guid) return E_POINTER;
    
    std::lock_guard<std::mutex> lock(components_mutex_);
    
    auto it = components_.find(guid);
    if (it == components_.end()) {
        return E_FAIL;
    }
    
    it->second.info.load_order = order;
    return S_OK;
}

HRESULT fb2k_component_manager_impl::GetComponentLoadOrder(const char* guid, DWORD* order) {
    if (!guid || !order) return E_POINTER;
    
    std::lock_guard<std::mutex> lock(components_mutex_);
    
    auto it = components_.find(guid);
    if (it == components_.end()) {
        return E_FAIL;
    }
    
    *order = it->second.info.load_order;
    return S_OK;
}

HRESULT fb2k_component_manager_impl::GetComponentsByType(component_type type, IFB2KComponent*** components, DWORD* count) {
    if (!count) return E_POINTER;
    
    std::lock_guard<std::mutex> lock(components_mutex_);
    
    std::vector<IFB2KComponent*> type_components;
    for (const auto& [guid, entry] : components_) {
        if (entry.info.type == type && entry.component) {
            entry.component->AddRef();
            type_components.push_back(entry.component);
        }
    }
    
    if (!components) {
        *count = static_cast<DWORD>(type_components.size());
        return S_OK;
    }
    
    // 杩欓噷搴旇鍒嗛厤鍐呭瓨骞惰繑鍥炵粍浠舵暟缁?
    *count = static_cast<DWORD>(type_components.size());
    return S_OK;
}

HRESULT fb2k_component_manager_impl::GetComponentTypes(component_type** types, DWORD* count) {
    if (!count) return E_POINTER;
    
    static component_type all_types[] = {
        component_type::input,
        component_type::output,
        component_type::dsp,
        component_type::visualisation,
        component_type::general,
        component_type::context_menu,
        component_type::toolbar,
        component_type::playlist_view,
        component_type::album_art,
        component_type::library,
        component_type::tagger,
        component_type::encoder
    };
    
    if (!types) {
        *count = sizeof(all_types) / sizeof(all_types[0]);
        return S_OK;
    }
    
    *types = all_types;
    *count = sizeof(all_types) / sizeof(all_types[0]);
    return S_OK;
}

HRESULT fb2k_component_manager_impl::GetLastErrorMessage(char** message) {
    if (!message) return E_POINTER;
    
    std::lock_guard<std::mutex> lock(error_mutex_);
    
    if (error_log_.empty()) {
        *message = nullptr;
        return S_FALSE;
    }
    
    // 杩斿洖鏈€鍚庝竴鏉￠敊璇秷鎭?
    std::string last_error = error_log_.back();
    char* error_copy = new char[last_error.length() + 1];
    strcpy(error_copy, last_error.c_str());
    
    *message = error_copy;
    return S_OK;
}

HRESULT fb2k_component_manager_impl::ClearErrorLog() {
    clear_error_log_internal();
    return S_OK;
}

HRESULT fb2k_component_manager_impl::GetErrorLog(char** log, DWORD* count) {
    if (!count) return E_POINTER;
    
    std::lock_guard<std::mutex> lock(error_mutex_);
    
    *count = static_cast<DWORD>(error_log_.size());
    if (!log) return S_OK;
    
    // 杩欓噷搴旇鍒嗛厤鍐呭瓨骞惰繑鍥為敊璇棩蹇?
    return S_OK;
}

// 绉佹湁杈呭姪鏂规硶
bool fb2k_component_manager_impl::scan_component_directory(const std::string& directory) {
    try {
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                std::string file_path = entry.path().string();
                std::string extension = entry.path().extension().string();
                
                // 妫€鏌ユ枃浠舵墿灞曞悕
                std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
                
                if (extension == ".dll" || extension == ".fb2k-component") {
                    load_component_file(file_path);
                }
            }
        }
        return true;
    } catch (const std::exception& e) {
        add_error("鎵弿缁勪欢鐩綍澶辫触: " + std::string(e.what()));
        return false;
    }
}

bool fb2k_component_manager_impl::load_component_file(const std::string& file_path) {
    try {
        component_info info;
        if (!extract_component_info(file_path, info)) {
            return false;
        }
        
        std::lock_guard<std::mutex> lock(components_mutex_);
        
        // 妫€鏌ユ槸鍚﹀凡瀛樺湪
        if (components_.find(info.guid) != components_.end()) {
            std::cout << "[FB2K Component] 缁勪欢宸插瓨鍦紝璺宠繃: " << info.name << std::endl;
            return false;
        }
        
        component_entry entry;
        entry.info = info;
        entry.component = nullptr;
        entry.dll_handle = nullptr;
        entry.dependency_satisfied = false;
        
        // 瑙ｆ瀽渚濊禆鍏崇郴
        if (!info.dependencies.empty()) {
            std::stringstream ss(info.dependencies);
            std::string dep;
            while (std::getline(ss, dep, ',')) {
                // 鍘婚櫎绌烘牸
                dep.erase(0, dep.find_first_not_of(" \t"));
                dep.erase(dep.find_last_not_of(" \t") + 1);
                if (!dep.empty()) {
                    entry.dependencies.push_back(dep);
                }
            }
        }
        
        components_[info.guid] = entry;
        
        std::cout << "[FB2K Component] 鍙戠幇缁勪欢: " << info.name << " (" << info.guid << ")" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        add_error("鍔犺浇缁勪欢鏂囦欢澶辫触: " + std::string(e.what()));
        return false;
    }
}

bool fb2k_component_manager_impl::unload_component_internal(const std::string& guid) {
    std::lock_guard<std::mutex> lock(components_mutex_);
    
    auto it = components_.find(guid);
    if (it == components_.end()) {
        return false;
    }
    
    auto& entry = it->second;
    
    if (entry.info.is_loaded) {
        // 璋冪敤缁勪欢鐨勯€€鍑?
        if (entry.component) {
            IFB2KInitQuit* init_quit = nullptr;
            if (SUCCEEDED(entry.component->QueryInterface(IID_IFB2KInitQuit, (void**)&init_quit))) {
                init_quit->OnQuit();
                init_quit->Release();
            }
            
            entry.component->Release();
            entry.component = nullptr;
        }
        
        // 鍗歌浇DLL
        if (entry.dll_handle) {
            FreeLibrary(entry.dll_handle);
            entry.dll_handle = nullptr;
        }
        
        entry.info.is_loaded = false;
    }
    
    components_.erase(it);
    return true;
}

bool fb2k_component_manager_impl::check_component_dependencies(const component_entry& entry) {
    for (const std::string& dep : entry.dependencies) {
        auto it = components_.find(dep);
        if (it == components_.end() || !it->second.info.is_loaded) {
            return false;
        }
    }
    return true;
}

bool fb2k_component_manager_impl::resolve_dependencies() {
    bool changed = true;
    bool all_resolved = true;
    
    while (changed) {
        changed = false;
        
        for (auto& [guid, entry] : components_) {
            if (!entry.dependency_satisfied && !entry.dependencies.empty()) {
                bool satisfied = check_component_dependencies(entry);
                if (satisfied) {
                    entry.dependency_satisfied = true;
                    changed = true;
                    std::cout << "[FB2K Component] 渚濊禆鍏崇郴婊¤冻: " << entry.info.name << std::endl;
                } else {
                    all_resolved = false;
                }
            }
        }
    }
    
    return all_resolved;
}

void fb2k_component_manager_impl::add_error(const std::string& error) {
    std::lock_guard<std::mutex> lock(error_mutex_);
    error_log_.push_back(error);
    std::cerr << "[FB2K Component] 閿欒: " << error << std::endl;
}

void fb2k_component_manager_impl::clear_error_log_internal() {
    std::lock_guard<std::mutex> lock(error_mutex_);
    error_log_.clear();
}

component_type fb2k_component_manager_impl::detect_component_type(const std::string& file_path) {
    // 鏍规嵁鏂囦欢鍚嶅拰璺緞鎺ㄦ柇缁勪欢绫诲瀷
    std::string filename = std::filesystem::path(file_path).filename().string();
    std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
    
    if (filename.find("input") != std::string::npos) return component_type::input;
    if (filename.find("output") != std::string::npos) return component_type::output;
    if (filename.find("dsp") != std::string::npos) return component_type::dsp;
    if (filename.find("visual") != std::string::npos) return component_type::visualisation;
    if (filename.find("foo_") == 0) return component_type::general;
    
    return component_type::unknown;
}

bool fb2k_component_manager_impl::extract_component_info(const std::string& file_path, component_info& info) {
    // 绠€鍖栧疄鐜帮細浠庢枃浠跺悕鎻愬彇鍩烘湰淇℃伅
    std::filesystem::path path(file_path);
    
    info.file_path = file_path;
    info.name = path.stem().string();
    info.version = "1.0.0";
    info.description = "foobar2000缁勪欢";
    info.author = "Unknown";
    info.guid = "{" + info.name + "-0000-0000-0000-000000000000}";
    info.type = detect_component_type(file_path);
    info.is_loaded = false;
    info.is_enabled = true;
    info.load_order = 1000;
    info.dependencies = "";
    info.last_modified = std::filesystem::last_write_time(path);
    info.file_size = std::filesystem::file_size(path);
    
    // 灏濊瘯浠嶥LL鑾峰彇鐗堟湰淇℃伅
    DWORD dummy;
    DWORD version_info_size = GetFileVersionInfoSizeA(file_path.c_str(), &dummy);
    if (version_info_size > 0) {
        std::vector<BYTE> version_data(version_info_size);
        if (GetFileVersionInfoA(file_path.c_str(), 0, version_info_size, version_data.data())) {
            VS_FIXEDFILEINFO* file_info = nullptr;
            UINT file_info_size = sizeof(VS_FIXEDFILEINFO);
            
            if (VerQueryValueA(version_data.data(), "\\", (LPVOID*)&file_info, &file_info_size)) {
                if (file_info) {
                    std::stringstream version;
                    version << HIWORD(file_info->dwFileVersionMS) << "."
                           << LOWORD(file_info->dwFileVersionMS) << "."
                           << HIWORD(file_info->dwFileVersionLS) << "."
                           << LOWORD(file_info->dwFileVersionLS);
                    info.version = version.str();
                }
            }
            
            // 鑾峰彇浜у搧鍚嶇О鍜屾弿杩?
            struct LANGANDCODEPAGE {
                WORD wLanguage;
                WORD wCodePage;
            } *lpTranslate;
            
            UINT cbTranslate;
            if (VerQueryValueA(version_data.data(), "\\VarFileInfo\\Translation", (LPVOID*)&lpTranslate, &cbTranslate)) {
                if (cbTranslate >= sizeof(LANGANDCODEPAGE)) {
                    char sub_block[256];
                    sprintf_s(sub_block, "\\StringFileInfo\\%04x%04x\\ProductName", 
                             lpTranslate[0].wLanguage, lpTranslate[0].wCodePage);
                    
                    char* product_name = nullptr;
                    UINT product_name_size = 0;
                    if (VerQueryValueA(version_data.data(), sub_block, (LPVOID*)&product_name, &product_name_size)) {
                        if (product_name && product_name_size > 0) {
                            info.name = product_name;
                        }
                    }
                    
                    sprintf_s(sub_block, "\\StringFileInfo\\%04x%04x\\Comments", 
                             lpTranslate[0].wLanguage, lpTranslate[0].wCodePage);
                    
                    char* comments = nullptr;
                    UINT comments_size = 0;
                    if (VerQueryValueA(version_data.data(), sub_block, (LPVOID*)&comments, &comments_size)) {
                        if (comments && comments_size > 0) {
                            info.description = comments;
                        }
                    }
                }
            }
        }
    }
    
    return true;
}

// 鎻掍欢鍔犺浇鍣ㄥ疄鐜?
fb2k_plugin_loader_impl::fb2k_plugin_loader_impl() {
    std::cout << "[FB2K Plugin] 鍒涘缓鎻掍欢鍔犺浇鍣? << std::endl;
}

fb2k_plugin_loader_impl::~fb2k_plugin_loader_impl() {
    std::cout << "[FB2K Plugin] 閿€姣佹彃浠跺姞杞藉櫒" << std::endl;
    
    // 鍗歌浇鎵€鏈夋彃浠?
    std::lock_guard<std::mutex> lock(plugins_mutex_);
    for (auto& [path, info] : loaded_plugins_) {
        if (info.handle) {
            // 璋冪敤鎻掍欢閫€鍑哄嚱鏁?
            typedef HRESULT (*FB2KQuitPluginFunc)();
            auto quit_func = (FB2KQuitPluginFunc)GetProcAddress(info.handle, "FB2KQuitPlugin");
            if (quit_func) {
                quit_func();
            }
            
            FreeLibrary(info.handle);
        }
    }
}

HRESULT fb2k_plugin_loader_impl::LoadPlugin(const char* dll_path) {
    if (!dll_path) return E_POINTER;
    
    std::lock_guard<std::mutex> lock(plugins_mutex_);
    
    // 妫€鏌ユ槸鍚﹀凡鍔犺浇
    auto it = loaded_plugins_.find(dll_path);
    if (it != loaded_plugins_.end()) {
        return S_OK; // 宸插姞杞?
    }
    
    plugin_info info;
    if (!load_plugin_internal(dll_path, info)) {
        return E_FAIL;
    }
    
    loaded_plugins_[dll_path] = info;
    std::cout << "[FB2K Plugin] 鎻掍欢鍔犺浇鎴愬姛: " << dll_path << std::endl;
    
    return S_OK;
}

HRESULT fb2k_plugin_loader_impl::UnloadPlugin(const char* dll_path) {
    if (!dll_path) return E_POINTER;
    
    return unload_plugin_internal(dll_path) ? S_OK : E_FAIL;
}

HRESULT fb2k_plugin_loader_impl::IsPluginLoaded(const char* dll_path, bool* loaded) {
    if (!dll_path || !loaded) return E_POINTER;
    
    std::lock_guard<std::mutex> lock(plugins_mutex_);
    *loaded = (loaded_plugins_.find(dll_path) != loaded_plugins_.end());
    
    return S_OK;
}

HRESULT fb2k_plugin_loader_impl::GetComponentsFromPlugin(const char* dll_path, IFB2KComponent*** components, DWORD* count) {
    if (!dll_path || !count) return E_POINTER;
    
    std::lock_guard<std::mutex> lock(plugins_mutex_);
    
    auto it = loaded_plugins_.find(dll_path);
    if (it == loaded_plugins_.end()) {
        return E_FAIL;
    }
    
    const plugin_info& info = it->second;
    *count = static_cast<DWORD>(info.component_guids.size());
    
    if (!components) return S_OK;
    
    // 杩欓噷搴旇鍒涘缓缁勪欢瀹炰緥骞惰繑鍥?
    // 绠€鍖栧疄鐜?
    return S_OK;
}

HRESULT fb2k_plugin_loader_impl::GetPluginInfo(const char* dll_path, component_info* info) {
    if (!dll_path || !info) return E_POINTER;
    
    std::lock_guard<std::mutex> lock(plugins_mutex_);
    
    auto it = loaded_plugins_.find(dll_path);
    if (it == loaded_plugins_.end()) {
        return E_FAIL;
    }
    
    const plugin_info& plugin_info = it->second;
    
    // 濉厖缁勪欢淇℃伅
    info->name = std::filesystem::path(dll_path).stem().string();
    info->version = std::to_string(HIWORD(plugin_info.version)) + "." + 
                   std::to_string(LOWORD(plugin_info.version));
    info->description = "foobar2000鎻掍欢";
    info->author = "Unknown";
    info->file_path = dll_path;
    info->guid = "{" + info->name + "-plugin-0000-0000-000000000000}";
    info->type = component_type::unknown;
    info->is_loaded = true;
    info->is_enabled = true;
    info->load_order = 1000;
    info->dependencies = "";
    
    return S_OK;
}

HRESULT fb2k_plugin_loader_impl::CheckPluginDependencies(const char* dll_path, bool* satisfied) {
    if (!dll_path || !satisfied) return E_POINTER;
    
    std::lock_guard<std::mutex> lock(plugins_mutex_);
    
    auto it = loaded_plugins_.find(dll_path);
    if (it == loaded_plugins_.end()) {
        *satisfied = false;
        return S_OK;
    }
    
    *satisfied = verify_plugin_dependencies(it->second);
    return S_OK;
}

HRESULT fb2k_plugin_loader_impl::GetPluginDependencies(const char* dll_path, char*** dependencies, DWORD* count) {
    if (!dll_path || !count) return E_POINTER;
    
    std::lock_guard<std::mutex> lock(plugins_mutex_);
    
    auto it = loaded_plugins_.find(dll_path);
    if (it == loaded_plugins_.end()) {
        *count = 0;
        return S_OK;
    }
    
    const plugin_info& info = it->second;
    *count = static_cast<DWORD>(info.dependencies.size());
    
    if (!dependencies) return S_OK;
    
    // 杩欓噷搴旇鍒嗛厤鍐呭瓨骞惰繑鍥炰緷璧栧垪琛?
    return S_OK;
}

HRESULT fb2k_plugin_loader_impl::VerifyPluginSignature(const char* dll_path, bool* verified) {
    if (!dll_path || !verified) return E_POINTER;
    
    // 绠€鍖栧疄鐜帮細鎬绘槸楠岃瘉閫氳繃
    *verified = true;
    return S_OK;
}

HRESULT fb2k_plugin_loader_impl::ScanPluginForMalware(const char* dll_path, bool* clean) {
    if (!dll_path || !clean) return E_POINTER;
    
    // 绠€鍖栧疄鐜帮細鎬绘槸瀹夊叏
    *clean = true;
    return S_OK;
}

HRESULT fb2k_plugin_loader_impl::CheckPluginCompatibility(const char* dll_path, bool* compatible) {
    if (!dll_path || !compatible) return E_POINTER;
    
    std::lock_guard<std::mutex> lock(plugins_mutex_);
    
    auto it = loaded_plugins_.find(dll_path);
    if (it == loaded_plugins_.end()) {
        *compatible = false;
        return S_OK;
    }
    
    *compatible = it->second.compatible;
    return S_OK;
}

HRESULT fb2k_plugin_loader_impl::GetPluginVersion(const char* dll_path, DWORD* version) {
    if (!dll_path || !version) return E_POINTER;
    
    std::lock_guard<std::mutex> lock(plugins_mutex_);
    
    auto it = loaded_plugins_.find(dll_path);
    if (it == loaded_plugins_.end()) {
        return E_FAIL;
    }
    
    *version = it->second.version;
    return S_OK;
}

HRESULT fb2k_plugin_loader_impl::GetRequiredAPIVersion(const char* dll_path, DWORD* api_version) {
    if (!dll_path || !api_version) return E_POINTER;
    
    std::lock_guard<std::mutex> lock(plugins_mutex_);
    
    auto it = loaded_plugins_.find(dll_path);
    if (it == loaded_plugins_.end()) {
        return E_FAIL;
    }
    
    *api_version = it->second.api_version;
    return S_OK;
}

// 绉佹湁杈呭姪鏂规硶
bool fb2k_plugin_loader_impl::load_plugin_internal(const std::string& dll_path, plugin_info& info) {
    // 鍔犺浇DLL
    HMODULE handle = LoadLibraryA(dll_path.c_str());
    if (!handle) {
        std::cerr << "[FB2K Plugin] 鍔犺浇DLL澶辫触: " << dll_path << std::endl;
        return false;
    }
    
    info.path = dll_path;
    info.handle = handle;
    
    // 鑾峰彇鎻掍欢淇℃伅
    FB2KGetPluginInfoFunc get_info_func = (FB2KGetPluginInfoFunc)GetProcAddress(handle, "FB2KGetPluginInfo");
    if (get_info_func) {
        get_info_func(&info.version, &info.api_version, nullptr);
    }
    
    // 璋冪敤鎻掍欢鍒濆鍖栧嚱鏁?
    FB2KInitPluginFunc init_func = (FB2KInitPluginFunc)GetProcAddress(handle, "FB2KInitPlugin");
    if (init_func) {
        HRESULT hr = init_func();
        if (FAILED(hr)) {
            std::cerr << "[FB2K Plugin] 鎻掍欢鍒濆鍖栧け璐? " << dll_path << std::endl;
            FreeLibrary(handle);
            return false;
        }
    }
    
    // 鎻愬彇缁勪欢淇℃伅
    extract_component_info(dll_path, info);
    
    // 妫€鏌ュ吋瀹规€у拰渚濊禆鍏崇郴
    info.compatible = check_plugin_compatibility(info);
    info.verified = verify_plugin_dependencies(info);
    
    return true;
}

bool fb2k_plugin_loader_impl::unload_plugin_internal(const std::string& dll_path) {
    std::lock_guard<std::mutex> lock(plugins_mutex_);
    
    auto it = loaded_plugins_.find(dll_path);
    if (it == loaded_plugins_.end()) {
        return false;
    }
    
    plugin_info& info = it->second;
    
    // 璋冪敤鎻掍欢閫€鍑哄嚱鏁?
    FB2KQuitPluginFunc quit_func = (FB2KQuitPluginFunc)GetProcAddress(info.handle, "FB2KQuitPlugin");
    if (quit_func) {
        quit_func();
    }
    
    // 鍗歌浇DLL
    FreeLibrary(info.handle);
    
    loaded_plugins_.erase(it);
    
    std::cout << "[FB2K Plugin] 鎻掍欢鍗歌浇鎴愬姛: " << dll_path << std::endl;
    return true;
}

bool fb2k_plugin_loader_impl::verify_plugin_dependencies(const plugin_info& info) {
    // 妫€鏌ヤ緷璧栫殑绯荤粺搴?
    for (const std::string& dep : info.dependencies) {
        HMODULE dep_handle = GetModuleHandleA(dep.c_str());
        if (!dep_handle) {
            // 灏濊瘯鍔犺浇渚濊禆
            dep_handle = LoadLibraryA(dep.c_str());
            if (!dep_handle) {
                return false;
            }
        }
    }
    return true;
}

bool fb2k_plugin_loader_impl::check_plugin_compatibility(const plugin_info& info) {
    // 妫€鏌PI鐗堟湰鍏煎鎬?
    DWORD current_api_version = 0x00010000; // 1.0.0.0
    return info.api_version <= current_api_version;
}

bool fb2k_plugin_loader_impl::extract_component_info(const std::string& dll_path, plugin_info& info) {
    HMODULE handle = info.handle;
    if (!handle) return false;
    
    // 鑾峰彇缁勪欢鏁伴噺
    FB2KGetComponentCountFunc get_count_func = (FB2KGetComponentCountFunc)GetProcAddress(handle, "FB2KGetComponentCount");
    if (!get_count_func) return false;
    
    DWORD count = 0;
    if (FAILED(get_count_func(&count))) return false;
    
    // 鑾峰彇缁勪欢淇℃伅
    FB2KGetComponentInfoFunc get_info_func = (FB2KGetComponentInfoFunc)GetProcAddress(handle, "FB2KGetComponentInfo");
    if (!get_info_func) return false;
    
    for (DWORD i = 0; i < count; i++) {
        char* guid = nullptr;
        char* name = nullptr;
        char* version = nullptr;
        component_type type = component_type::unknown;
        
        if (SUCCEEDED(get_info_func(i, &guid, &name, &version, &type))) {
            if (guid) {
                info.component_guids.push_back(guid);
            }
            // 閲婃斁瀛楃涓插唴瀛橈紙鍋囪浣跨敤CoTaskMemAlloc鍒嗛厤锛?
            if (guid) CoTaskMemFree(guid);
            if (name) CoTaskMemFree(name);
            if (version) CoTaskMemFree(version);
        }
    }
    
    return true;
}

// 鍏ㄥ眬缁勪欢绯荤粺鍒濆鍖?
void initialize_fb2k_component_system() {
    std::cout << "[FB2K Component] 鍒濆鍖栫粍浠剁郴缁?.." << std::endl;
    
    // 鍒涘缓缁勪欢绠＄悊鍣?
    auto* manager = new fb2k_component_manager_impl();
    auto* provider = fb2k_service_provider_impl::get_instance();
    if (provider) {
        provider->RegisterService(IID_IFB2KComponentManager, manager);
        manager->Release();
    }
    
    // 鍒涘缓鎻掍欢鍔犺浇鍣?
    auto* loader = new fb2k_plugin_loader_impl();
    if (provider) {
        provider->RegisterService(IID_IFB2KPluginLoader, loader);
        loader->Release();
    }
    
    // 鍒濆鍖栫粍浠剁鐞嗗櫒
    if (manager) {
        manager->Initialize();
    }
    
    std::cout << "[FB2K Component] 缁勪欢绯荤粺鍒濆鍖栧畬鎴? << std::endl;
}

// 鍏ㄥ眬缁勪欢绯荤粺娓呯悊
void shutdown_fb2k_component_system() {
    std::cout << "[FB2K Component] 鍏抽棴缁勪欢绯荤粺..." << std::endl;
    
    auto* provider = fb2k_service_provider_impl::get_instance();
    if (provider) {
        provider->UnregisterService(IID_IFB2KComponentManager);
        provider->UnregisterService(IID_IFB2KPluginLoader);
    }
    
    std::cout << "[FB2K Component] 缁勪欢绯荤粺宸插叧闂? << std::endl;
}

} // namespace fb2k