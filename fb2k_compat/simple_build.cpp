// 简化版构建 - 单文件测试程序
// 包含所有功能在一个文件中，避免复杂的构建配置

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <unknwn.h>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <filesystem>

namespace fs = std::filesystem;

// 简化GUID定义
#ifdef __uuidof
#undef __uuidof
#endif
#define __uuidof(x) IID_##x

// 生成GUID宏
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    EXTERN_C const GUID DECLSPEC_SELECTANY name \
        = { l, w1, w2, { b1, b2, b3, b4, b5, b6, b7, b8 } }

// 接口GUID
DEFINE_GUID(IID_IUnknown, 0x00000000, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);
DEFINE_GUID(IID_FileInfo, 0x12345678, 0x1234, 0x1234, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0);
DEFINE_GUID(IID_AbortCallback, 0x23456789, 0x2345, 0x2345, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x01);
DEFINE_GUID(IID_InputDecoder, 0x3456789A, 0x3456, 0x3456, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x12);
DEFINE_GUID(CLSID_InputDecoderService, 0x456789AB, 0x4567, 0x4567, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x01, 0x23);

// 前向声明
class FileInfo;
class AbortCallback;
class InputDecoder;

// 基础类型
struct AudioInfo {
    uint32_t sample_rate = 44100;
    uint32_t channels = 2;
    uint32_t bitrate = 0;
    double length = 0.0;
};

struct FileStats {
    uint64_t size = 0;
    uint64_t timestamp = 0;
};

// 简化COM接口
class ComObject : public IUnknown {
private:
    ULONG ref_count_ = 1;
    
public:
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) {
        if(!ppvObject) return E_POINTER;
        *ppvObject = nullptr;
        
        if(IsEqualGUID(riid, IID_IUnknown)) {
            *ppvObject = static_cast<IUnknown*>(this);
        } else {
            return QueryInterfaceImpl(riid, ppvObject);
        }
        
        AddRef();
        return S_OK;
    }
    
    virtual HRESULT QueryInterfaceImpl(REFIID riid, void** ppvObject) {
        return E_NOINTERFACE;
    }
    
    virtual ULONG STDMETHODCALLTYPE AddRef() {
        return ++ref_count_;
    }
    
    virtual ULONG STDMETHODCALLTYPE Release() {
        ULONG count = --ref_count_;
        if(count == 0) {
            delete this;
        }
        return count;
    }
};

// 文件信息接口
class FileInfo : public ComObject {
public:
    virtual void Reset() = 0;
    virtual const char* MetaGet(const char* name, size_t index) const = 0;
    virtual size_t MetaGetCount(const char* name) const = 0;
    virtual void MetaSet(const char* name, const char* value) = 0;
    virtual double GetLength() const = 0;
    virtual void SetLength(double length) = 0;
    virtual const AudioInfo& GetAudioInfo() const = 0;
    virtual void SetAudioInfo(const AudioInfo& info) = 0;
    virtual const FileStats& GetFileStats() const = 0;
    virtual void SetFileStats(const FileStats& stats) = 0;
    
    HRESULT QueryInterfaceImpl(REFIID riid, void** ppvObject) override {
        if(IsEqualGUID(riid, IID_FileInfo)) {
            *ppvObject = static_cast<FileInfo*>(this);
            return S_OK;
        }
        return E_NOINTERFACE;
    }
};

// 中止回调接口
class AbortCallback : public ComObject {
public:
    virtual bool IsAborting() const = 0;
    
    HRESULT QueryInterfaceImpl(REFIID riid, void** ppvObject) override {
        if(IsEqualGUID(riid, IID_AbortCallback)) {
            *ppvObject = static_cast<AbortCallback*>(this);
            return S_OK;
        }
        return E_NOINTERFACE;
    }
};

// 输入解码器接口
class InputDecoder : public ComObject {
public:
    virtual bool Open(const char* path, FileInfo& info, AbortCallback& abort) = 0;
    virtual int Decode(float* buffer, int samples, AbortCallback& abort) = 0;
    virtual void Seek(double seconds, AbortCallback& abort) = 0;
    virtual bool CanSeek() = 0;
    virtual void Close() = 0;
    virtual bool IsOurPath(const char* path) = 0;
    virtual const char* GetName() = 0;
    
    HRESULT QueryInterfaceImpl(REFIID riid, void** ppvObject) override {
        if(IsEqualGUID(riid, IID_InputDecoder)) {
            *ppvObject = static_cast<InputDecoder*>(this);
            return S_OK;
        }
        return E_NOINTERFACE;
    }
};

// 文件信息实现
class FileInfoImpl : public FileInfo {
private:
    std::unordered_map<std::string, std::vector<std::string>> metadata_;
    AudioInfo audio_info_;
    FileStats file_stats_;
    double length_ = 0.0;
    
public:
    void Reset() override {
        metadata_.clear();
        audio_info_ = {};
        file_stats_ = {};
        length_ = 0.0;
    }
    
    const char* MetaGet(const char* name, size_t index) const override {
        auto it = metadata_.find(name ? name : "");
        if(it != metadata_.end() && index < it->second.size()) {
            return it->second[index].c_str();
        }
        return nullptr;
    }
    
    size_t MetaGetCount(const char* name) const override {
        auto it = metadata_.find(name ? name : "");
        return it != metadata_.end() ? it->second.size() : 0;
    }
    
    void MetaSet(const char* name, const char* value) override {
        if(name && value) {
            metadata_[name] = {value};
        }
    }
    
    double GetLength() const override { return length_; }
    void SetLength(double length) override { length_ = length; }
    
    const AudioInfo& GetAudioInfo() const override { return audio_info_; }
    void SetAudioInfo(const AudioInfo& info) override { audio_info_ = info; }
    
    const FileStats& GetFileStats() const override { return file_stats_; }
    void SetFileStats(const FileStats& stats) override { file_stats_ = stats; }
};

// 哑中止回调
class AbortCallbackDummy : public AbortCallback {
public:
    bool IsAborting() const override { return false; }
};

// 模拟解码器（用于测试）
class MockDecoder : public InputDecoder {
private:
    bool is_open_ = false;
    std::string current_path_;
    AudioInfo audio_info_;
    double position_ = 0.0;
    
public:
    bool Open(const char* path, FileInfo& info, AbortCallback& abort) override {
        if(!path) return false;
        
        // 模拟文件打开
        current_path_ = path;
        is_open_ = true;
        position_ = 0.0;
        
        // 设置音频信息（模拟）
        audio_info_.sample_rate = 44100;
        audio_info_.channels = 2;
        audio_info_.bitrate = 128;
        audio_info_.length = 180.0; // 3分钟
        
        // 设置文件信息
        info.SetLength(audio_info_.length);
        info.SetAudioInfo(audio_info_);
        info.MetaSet("title", "Test Audio");
        info.MetaSet("artist", "Mock Decoder");
        
        std::cout << "[MockDecoder] Opened: " << path << std::endl;
        return true;
    }
    
    int Decode(float* buffer, int samples, AbortCallback& abort) override {
        if(!is_open_ || !buffer) return 0;
        
        // 生成正弦波测试音频
        const float frequency = 440.0f; // A4音符
        const float amplitude = 0.5f;
        
        for(int i = 0; i < samples; i++) {
            float time = (position_ + float(i) / audio_info_.sample_rate);
            float value = amplitude * sin(2.0f * 3.14159f * frequency * time);
            
            // 立体声
            buffer[i * 2] = value;      // 左声道
            buffer[i * 2 + 1] = value;  // 右声道
        }
        
        position_ += float(samples) / audio_info_.sample_rate;
        
        std::cout << "[MockDecoder] Decoded " << samples << " samples at position " << position_ << std::endl;
        return samples;
    }
    
    void Seek(double seconds, AbortCallback& abort) override {
        position_ = seconds;
        std::cout << "[MockDecoder] Seek to: " << seconds << " seconds" << std::endl;
    }
    
    bool CanSeek() override { return true; }
    
    void Close() override {
        is_open_ = false;
        current_path_.clear();
        position_ = 0.0;
        std::cout << "[MockDecoder] Closed" << std::endl;
    }
    
    bool IsOurPath(const char* path) override {
        // 支持常见音频格式
        if(!path) return false;
        std::string ext = fs::path(path).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext == ".wav" || ext == ".mp3" || ext == ".flac" || ext == ".ape";
    }
    
    const char* GetName() override { return "Mock Decoder (fb2k兼容测试)"; }
};

// 主机类
class MiniHost {
private:
    std::vector<std::unique_ptr<InputDecoder>> decoders_;
    
public:
    bool Initialize() {
        std::cout << "[MiniHost] 初始化..." << std::endl;
        
        // 添加模拟解码器
        auto mock_decoder = std::make_unique<MockDecoder>();
        decoders_.push_back(std::move(mock_decoder));
        
        std::cout << "[MiniHost] 初始化完成，解码器数量: " << decoders_.size() << std::endl;
        return true;
    }
    
    InputDecoder* CreateDecoderForPath(const std::string& path) {
        std::cout << "[MiniHost] 为路径创建解码器: " << path << std::endl;
        
        for(const auto& decoder : decoders_) {
            if(decoder->IsOurPath(path.c_str())) {
                std::cout << "[MiniHost] 找到匹配的解码器: " << decoder->GetName() << std::endl;
                return decoder.get();
            }
        }
        
        std::cout << "[MiniHost] 未找到匹配的解码器" << std::endl;
        return nullptr;
    }
    
    bool TestDecode(const std::string& audio_file) {
        std::cout << std::endl << "=== 解码测试开始 ===" << std::endl;
        std::cout << "音频文件: " << audio_file << std::endl;
        
        // 创建解码器
        auto* decoder = CreateDecoderForPath(audio_file);
        if(!decoder) {
            std::cerr << "错误: 未找到支持此格式的解码器" << std::endl;
            return false;
        }
        
        // 创建文件信息和中止回调
        auto file_info = std::make_unique<FileInfoImpl>();
        auto abort_cb = std::make_unique<AbortCallbackDummy>();
        
        // 打开文件
        std::cout << std::endl << "正在打开文件..." << std::endl;
        if(!decoder->Open(audio_file.c_str(), *file_info, *abort_cb)) {
            std::cerr << "错误: 无法打开文件" << std::endl;
            return false;
        }
        
        // 显示文件信息
        std::cout << std::endl << "文件信息:" << std::endl;
        std::cout << "  长度: " << file_info->GetLength() << " 秒" << std::endl;
        auto& audio_info = file_info->GetAudioInfo();
        std::cout << "  采样率: " << audio_info.sample_rate << " Hz" << std::endl;
        std::cout << "  声道数: " << audio_info.channels << std::endl;
        std::cout << "  比特率: " << audio_info.bitrate << " kbps" << std::endl;
        
        const char* title = file_info->MetaGet("title", 0);
        const char* artist = file_info->MetaGet("artist", 0);
        if(title) std::cout << "  标题: " << title << std::endl;
        if(artist) std::cout << "  艺术家: " << artist << std::endl;
        
        // 测试解码
        std::cout << std::endl << "开始解码测试..." << std::endl;
        const int test_samples = 1024;
        std::vector<float> buffer(test_samples * audio_info.channels);
        
        int total_decoded = 0;
        const int max_iterations = 10; // 限制测试轮数
        
        for(int i = 0; i < max_iterations; i++) {
            int decoded = decoder->Decode(buffer.data(), test_samples, *abort_cb);
            if(decoded <= 0) {
                std::cout << "解码结束，总共解码 " << total_decoded << " 个采样" << std::endl;
                break;
            }
            
            total_decoded += decoded;
            
            // 显示进度
            double progress = (double)total_decoded / audio_info.sample_rate;
            std::cout << "  进度: " << std::fixed << std::setprecision(2) << progress << " 秒" << std::endl;
            
            // 检查音频数据（简单验证）
            float max_amplitude = 0.0f;
            for(int j = 0; j < decoded * audio_info.channels; j++) {
                max_amplitude = std::max(max_amplitude, std::abs(buffer[j]));
            }
            std::cout << "  最大振幅: " << max_amplitude << std::endl;
        }
        
        // 测试跳转
        if(decoder->CanSeek()) {
            std::cout << std::endl << "测试跳转功能..." << std::endl;
            decoder->Seek(1.0, *abort_cb); // 跳转到1秒
        }
        
        // 关闭解码器
        std::cout << std::endl << "关闭解码器..." << std::endl;
        decoder->Close();
        
        std::cout << std::endl << "=== 解码测试完成 ===" << std::endl;
        std::cout << "总解码采样数: " << total_decoded << std::endl;
        std::cout << "测试时长: " << (double)total_decoded / audio_info.sample_rate << " 秒" << std::endl;
        
        return true;
    }
};

// 测试音频文件生成
void CreateTestWav(const std::string& filename) {
    std::cout << "创建测试音频文件: " << filename << std::endl;
    
    // 简单的WAV文件头 + 正弦波数据
    struct WavHeader {
        char riff[4] = {'R', 'I', 'F', 'F'};
        uint32_t size = 36 + 44100 * 2 * 2; // 1秒，16bit，立体声
        char wave[4] = {'W', 'A', 'V', 'E'};
        char fmt[4] = {'f', 'm', 't', ' '};
        uint32_t fmt_size = 16;
        uint16_t audio_format = 1; // PCM
        uint16_t channels = 2;
        uint32_t sample_rate = 44100;
        uint32_t byte_rate = 44100 * 2 * 2;
        uint16_t block_align = 4;
        uint16_t bits_per_sample = 16;
        char data[4] = {'d', 'a', 't', 'a'};
        uint32_t data_size = 44100 * 2 * 2;
    };
    
    WavHeader header;
    std::ofstream file(filename, std::ios::binary);
    if(!file) return;
    
    file.write(reinterpret_cast<const char*>(&header), sizeof(header));
    
    // 生成1秒正弦波
    const int samples = 44100;
    const float frequency = 440.0f; // A4
    const float amplitude = 0.5f;
    
    for(int i = 0; i < samples; i++) {
        float time = (float)i / 44100.0f;
        float value = amplitude * sin(2.0f * 3.14159f * frequency * time);
        
        // 转换为16bit整数
        int16_t sample = static_cast<int16_t>(value * 32767.0f);
        
        // 立体声
        file.write(reinterpret_cast<const char*>(&sample), sizeof(sample));
        file.write(reinterpret_cast<const char*>(&sample), sizeof(sample));
    }
    
    file.close();
    std::cout << "测试音频文件创建完成" << std::endl;
}

// 主函数
int main(int argc, char* argv[]) {
    std::cout << "=== foobar2000 兼容层阶段1测试 ===" << std::endl;
    std::cout << "简化版单文件测试程序" << std::endl;
    std::cout << "=====================================" << std::endl << std::endl;
    
    // 初始化COM
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if(FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        std::cerr << "COM初始化失败: 0x" << std::hex << hr << std::endl;
        return 1;
    }
    
    // 创建主机
    MiniHost host;
    if(!host.Initialize()) {
        std::cerr << "主机初始化失败" << std::endl;
        CoUninitialize();
        return 1;
    }
    
    // 创建或查找测试文件
    std::string test_file = "test_sine.wav";
    if(!fs::exists(test_file)) {
        std::cout << "未找到测试文件，创建测试音频..." << std::endl;
        CreateTestWav(test_file);
    }
    
    if(!fs::exists(test_file)) {
        std::cerr << "无法创建测试文件" << std::endl;
        CoUninitialize();
        return 1;
    }
    
    std::cout << std::endl << "使用测试文件: " << test_file << std::endl;
    
    // 运行解码测试
    bool success = host.TestDecode(test_file);
    
    if(success) {
        std::cout << std::endl << "✅ 测试成功! foobar2000 兼容层框架工作正常。" << std::endl;
        std::cout << "虽然使用的是模拟解码器，但接口架构验证通过。" << std::endl;
    } else {
        std::cout << std::endl << "❌ 测试失败，请检查错误信息。" << std::endl;
    }
    
    CoUninitialize();
    return success ? 0 : 1;
}