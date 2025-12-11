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

// 组件管理器实现
fb2k_component_manager_impl::fb2k_component_manager_impl() {
    std::cout << "[FB2K Component] 创建组件管理器" << std::endl;
}

fb2k_component_manager_impl::~fb2k_component_manager_impl() {
    std::cout << "[FB2K Component] 销毁组件管理器" << std::endl;
    UnloadAllComponents();
}

HRESULT fb2k_component_manager_impl::do_initialize() {
    std::cout << "[FB2K Component] 初始化组件管理器" << std::endl;
    
    // 创建插件加载器
    plugin_loader_ = std::make_unique<fb2k_plugin_loader_impl>();
    
    // 设置默认组件目录
    char buffer[MAX_PATH];
    if (GetModuleFileNameA(nullptr, buffer, MAX_PATH) > 0) {
        std::string exe_path = buffer;
        size_t last_slash = exe_path.find_last_of("\\/");
        if (last_slash != std::string::npos) {
            components_directory_ = exe_path.substr(0, last_slash) + "\\components";
        }
    }
    
    // 扫描默认组件目录
    if (!components_directory_.empty() && std::filesystem::exists(components_directory_)) {
        ScanComponents(components_directory_.c_str());
    }
    
    return S_OK;
}

HRESULT fb2k_component_manager_impl::do_shutdown() {
    std::cout << "[FB2K Component] 关闭组件管理器" << std::endl;
    UnloadAllComponents();
    plugin_loader_.reset();
    return S_OK;
}

HRESULT fb2k_component_manager_impl::ScanComponents(const char* directory) {
    if (!directory) return E_POINTER;
    
    std::string dir_path = directory;
    if (!std::filesystem::exists(dir_path)) {
        add_error("组件目录不存在: " + dir_path);
        return E_FAIL;
    }
    
    std::cout << "[FB2K Component] 扫描组件目录: " << dir_path << std::endl;
    
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
    
    // 这里应该分配内存并填充组件信息
    // 简化实现：直接返回数量
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
    
    // 先卸载
    if (!unload_component_internal(guid)) {
        return E_FAIL;
    }
    
    // 再重新加载
    return load_component_file(file_path) ? S_OK : E_FAIL;
}

HRESULT fb2k_component_manager_impl::LoadAllComponents() {
    std::lock_guard<std::mutex> lock(components_mutex_);
    
    std::cout << "[FB2K Component] 加载所有组件..." << std::endl;
    
    bool all_success = true;
    std::vector<std::string> load_order;
    
    // 按加载顺序排序
    for (const auto& [guid, entry] : components_) {
        if (!entry.info.is_loaded && entry.info.is_enabled) {
            load_order.push_back(guid);
        }
    }
    
    // 按加载顺序排序
    std::sort(load_order.begin(), load_order.end(), 
        [this](const std::string& a, const std::string& b) {
            return components_[a].info.load_order < components_[b].info.load_order;
        });
    
    // 解析依赖关系
    if (!resolve_dependencies()) {
        add_error("依赖关系解析失败");
        all_success = false;
    }
    
    // 加载组件
    for (const std::string& guid : load_order) {
        auto& entry = components_[guid];
        
        if (!entry.dependency_satisfied) {
            std::cout << "[FB2K Component] 跳过加载（依赖未满足）: " << entry.info.name << std::endl;
            continue;
        }
        
        std::cout << "[FB2K Component] 加载组件: " << entry.info.name << std::endl;
        
        // 这里应该实际加载组件
        // 简化实现：标记为已加载
        entry.info.is_loaded = true;
        
        // 调用组件的初始化
        if (entry.component) {
            IFB2KInitQuit* init_quit = nullptr;
            if (SUCCEEDED(entry.component->QueryInterface(IID_IFB2KInitQuit, (void**)&init_quit))) {
                init_quit->OnInit();
                init_quit->Release();
            }
        }
    }
    
    // 调用所有组件的OnSystemInit
    for (auto& [guid, entry] : components_) {
        if (entry.info.is_loaded && entry.component) {
            IFB2KInitQuit* init_quit = nullptr;
            if (SUCCEEDED(entry.component->QueryInterface(IID_IFB2KInitQuit, (void**)&init_quit))) {
                init_quit->OnSystemInit();
                init_quit->Release();
            }
        }
    }
    
    std::cout << "[FB2K Component] 组件加载完成" << std::endl;
    return all_success ? S_OK : S_FALSE;
}

HRESULT fb2k_component_manager_impl::UnloadAllComponents() {
    std::lock_guard<std::mutex> lock(components_mutex_);
    
    std::cout << "[FB2K Component] 卸载所有组件..." << std::endl;
    
    // 反向卸载（按加载顺序的逆序）
    std::vector<std::string> unload_order;
    for (const auto& [guid, entry] : components_) {
        if (entry.info.is_loaded) {
            unload_order.push_back(guid);
        }
    }
    
    // 按加载顺序的逆序排序
    std::sort(unload_order.begin(), unload_order.end(),
        [this](const std::string& a, const std::string& b) {
            return components_[a].info.load_order > components_[b].info.load_order;
        });
    
    // 调用所有组件的OnSystemQuit
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
    
    // 卸载组件
    for (const std::string& guid : unload_order) {
        auto& entry = components_[guid];
        
        std::cout << "[FB2K Component] 卸载组件: " << entry.info.name << std::endl;
        
        // 调用组件的退出
        if (entry.component) {
            IFB2KInitQuit* init_quit = nullptr;
            if (SUCCEEDED(entry.component->QueryInterface(IID_IFB2KInitQuit, (void**)&init_quit))) {
                init_quit->OnQuit();
                init_quit->Release();
            }
            
            entry.component->Release();
            entry.component = nullptr;
        }
        
        // 卸载DLL
        if (entry.dll_handle) {
            FreeLibrary(entry.dll_handle);
            entry.dll_handle = nullptr;
        }
        
        entry.info.is_loaded = false;
    }
    
    std::cout << "[FB2K Component] 组件卸载完成" << std::endl;
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
    
    std::cout << "[FB2K Component] " << (enable ? "启用" : "禁用") << "组件: " << it->second.info.name << std::endl;
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
    
    // 这里应该分配内存并返回组件数组
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
    
    // 返回最后一条错误消息
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
    
    // 这里应该分配内存并返回错误日志
    return S_OK;
}

// 私有辅助方法
bool fb2k_component_manager_impl::scan_component_directory(const std::string& directory) {
    try {
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                std::string file_path = entry.path().string();
                std::string extension = entry.path().extension().string();
                
                // 检查文件扩展名
                std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
                
                if (extension == ".dll" || extension == ".fb2k-component") {
                    load_component_file(file_path);
                }
            }
        }
        return true;
    } catch (const std::exception& e) {
        add_error("扫描组件目录失败: " + std::string(e.what()));
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
        
        // 检查是否已存在
        if (components_.find(info.guid) != components_.end()) {
            std::cout << "[FB2K Component] 组件已存在，跳过: " << info.name << std::endl;
            return false;
        }
        
        component_entry entry;
        entry.info = info;
        entry.component = nullptr;
        entry.dll_handle = nullptr;
        entry.dependency_satisfied = false;
        
        // 解析依赖关系
        if (!info.dependencies.empty()) {
            std::stringstream ss(info.dependencies);
            std::string dep;
            while (std::getline(ss, dep, ',')) {
                // 去除空格
                dep.erase(0, dep.find_first_not_of(" \t"));
                dep.erase(dep.find_last_not_of(" \t") + 1);
                if (!dep.empty()) {
                    entry.dependencies.push_back(dep);
                }
            }
        }
        
        components_[info.guid] = entry;
        
        std::cout << "[FB2K Component] 发现组件: " << info.name << " (" << info.guid << ")" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        add_error("加载组件文件失败: " + std::string(e.what()));
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
        // 调用组件的退出
        if (entry.component) {
            IFB2KInitQuit* init_quit = nullptr;
            if (SUCCEEDED(entry.component->QueryInterface(IID_IFB2KInitQuit, (void**)&init_quit))) {
                init_quit->OnQuit();
                init_quit->Release();
            }
            
            entry.component->Release();
            entry.component = nullptr;
        }
        
        // 卸载DLL
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
                    std::cout << "[FB2K Component] 依赖关系满足: " << entry.info.name << std::endl;
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
    std::cerr << "[FB2K Component] 错误: " << error << std::endl;
}

void fb2k_component_manager_impl::clear_error_log_internal() {
    std::lock_guard<std::mutex> lock(error_mutex_);
    error_log_.clear();
}

component_type fb2k_component_manager_impl::detect_component_type(const std::string& file_path) {
    // 根据文件名和路径推断组件类型
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
    // 简化实现：从文件名提取基本信息
    std::filesystem::path path(file_path);
    
    info.file_path = file_path;
    info.name = path.stem().string();
    info.version = "1.0.0";
    info.description = "foobar2000组件";
    info.author = "Unknown";
    info.guid = "{" + info.name + "-0000-0000-0000-000000000000}";
    info.type = detect_component_type(file_path);
    info.is_loaded = false;
    info.is_enabled = true;
    info.load_order = 1000;
    info.dependencies = "";
    info.last_modified = std::filesystem::last_write_time(path);
    info.file_size = std::filesystem::file_size(path);
    
    // 尝试从DLL获取版本信息
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
            
            // 获取产品名称和描述
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

// 插件加载器实现
fb2k_plugin_loader_impl::fb2k_plugin_loader_impl() {
    std::cout << "[FB2K Plugin] 创建插件加载器" << std::endl;
}

fb2k_plugin_loader_impl::~fb2k_plugin_loader_impl() {
    std::cout << "[FB2K Plugin] 销毁插件加载器" << std::endl;
    
    // 卸载所有插件
    std::lock_guard<std::mutex> lock(plugins_mutex_);
    for (auto& [path, info] : loaded_plugins_) {
        if (info.handle) {
            // 调用插件退出函数
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
    
    // 检查是否已加载
    auto it = loaded_plugins_.find(dll_path);
    if (it != loaded_plugins_.end()) {
        return S_OK; // 已加载
    }
    
    plugin_info info;
    if (!load_plugin_internal(dll_path, info)) {
        return E_FAIL;
    }
    
    loaded_plugins_[dll_path] = info;
    std::cout << "[FB2K Plugin] 插件加载成功: " << dll_path << std::endl;
    
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
    
    // 这里应该创建组件实例并返回
    // 简化实现
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
    
    // 填充组件信息
    info->name = std::filesystem::path(dll_path).stem().string();
    info->version = std::to_string(HIWORD(plugin_info.version)) + "." + 
                   std::to_string(LOWORD(plugin_info.version));
    info->description = "foobar2000插件";
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
    
    // 这里应该分配内存并返回依赖列表
    return S_OK;
}

HRESULT fb2k_plugin_loader_impl::VerifyPluginSignature(const char* dll_path, bool* verified) {
    if (!dll_path || !verified) return E_POINTER;
    
    // 简化实现：总是验证通过
    *verified = true;
    return S_OK;
}

HRESULT fb2k_plugin_loader_impl::ScanPluginForMalware(const char* dll_path, bool* clean) {
    if (!dll_path || !clean) return E_POINTER;
    
    // 简化实现：总是安全
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

// 私有辅助方法
bool fb2k_plugin_loader_impl::load_plugin_internal(const std::string& dll_path, plugin_info& info) {
    // 加载DLL
    HMODULE handle = LoadLibraryA(dll_path.c_str());
    if (!handle) {
        std::cerr << "[FB2K Plugin] 加载DLL失败: " << dll_path << std::endl;
        return false;
    }
    
    info.path = dll_path;
    info.handle = handle;
    
    // 获取插件信息
    FB2KGetPluginInfoFunc get_info_func = (FB2KGetPluginInfoFunc)GetProcAddress(handle, "FB2KGetPluginInfo");
    if (get_info_func) {
        get_info_func(&info.version, &info.api_version, nullptr);
    }
    
    // 调用插件初始化函数
    FB2KInitPluginFunc init_func = (FB2KInitPluginFunc)GetProcAddress(handle, "FB2KInitPlugin");
    if (init_func) {
        HRESULT hr = init_func();
        if (FAILED(hr)) {
            std::cerr << "[FB2K Plugin] 插件初始化失败: " << dll_path << std::endl;
            FreeLibrary(handle);
            return false;
        }
    }
    
    // 提取组件信息
    extract_component_info(dll_path, info);
    
    // 检查兼容性和依赖关系
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
    
    // 调用插件退出函数
    FB2KQuitPluginFunc quit_func = (FB2KQuitPluginFunc)GetProcAddress(info.handle, "FB2KQuitPlugin");
    if (quit_func) {
        quit_func();
    }
    
    // 卸载DLL
    FreeLibrary(info.handle);
    
    loaded_plugins_.erase(it);
    
    std::cout << "[FB2K Plugin] 插件卸载成功: " << dll_path << std::endl;
    return true;
}

bool fb2k_plugin_loader_impl::verify_plugin_dependencies(const plugin_info& info) {
    // 检查依赖的系统库
    for (const std::string& dep : info.dependencies) {
        HMODULE dep_handle = GetModuleHandleA(dep.c_str());
        if (!dep_handle) {
            // 尝试加载依赖
            dep_handle = LoadLibraryA(dep.c_str());
            if (!dep_handle) {
                return false;
            }
        }
    }
    return true;
}

bool fb2k_plugin_loader_impl::check_plugin_compatibility(const plugin_info& info) {
    // 检查API版本兼容性
    DWORD current_api_version = 0x00010000; // 1.0.0.0
    return info.api_version <= current_api_version;
}

bool fb2k_plugin_loader_impl::extract_component_info(const std::string& dll_path, plugin_info& info) {
    HMODULE handle = info.handle;
    if (!handle) return false;
    
    // 获取组件数量
    FB2KGetComponentCountFunc get_count_func = (FB2KGetComponentCountFunc)GetProcAddress(handle, "FB2KGetComponentCount");
    if (!get_count_func) return false;
    
    DWORD count = 0;
    if (FAILED(get_count_func(&count))) return false;
    
    // 获取组件信息
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
            // 释放字符串内存（假设使用CoTaskMemAlloc分配）
            if (guid) CoTaskMemFree(guid);
            if (name) CoTaskMemFree(name);
            if (version) CoTaskMemFree(version);
        }
    }
    
    return true;
}

// 全局组件系统初始化
void initialize_fb2k_component_system() {
    std::cout << "[FB2K Component] 初始化组件系统..." << std::endl;
    
    // 创建组件管理器
    auto* manager = new fb2k_component_manager_impl();
    auto* provider = fb2k_service_provider_impl::get_instance();
    if (provider) {
        provider->RegisterService(IID_IFB2KComponentManager, manager);
        manager->Release();
    }
    
    // 创建插件加载器
    auto* loader = new fb2k_plugin_loader_impl();
    if (provider) {
        provider->RegisterService(IID_IFB2KPluginLoader, loader);
        loader->Release();
    }
    
    // 初始化组件管理器
    if (manager) {
        manager->Initialize();
    }
    
    std::cout << "[FB2K Component] 组件系统初始化完成" << std::endl;
}

// 全局组件系统清理
void shutdown_fb2k_component_system() {
    std::cout << "[FB2K Component] 关闭组件系统..." << std::endl;
    
    auto* provider = fb2k_service_provider_impl::get_instance();
    if (provider) {
        provider->UnregisterService(IID_IFB2KComponentManager);
        provider->UnregisterService(IID_IFB2KPluginLoader);
    }
    
    std::cout << "[FB2K Component] 组件系统已关闭" << std::endl;
}

} // namespace fb2k