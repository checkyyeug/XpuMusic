// 绠€鍖栫増鏋勫缓 - 鍗曟枃浠舵祴璇曠▼搴?
// 鍖呭惈鎵€鏈夊姛鑳藉湪涓€涓枃浠朵腑锛岄伩鍏嶅鏉傜殑鏋勫缓閰嶇疆

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

// 绠€鍖朑UID瀹氫箟
#ifdef __uuidof
#undef __uuidof
#endif
#define __uuidof(x) IID_##x

// 鐢熸垚GUID瀹?
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    EXTERN_C const GUID DECLSPEC_SELECTANY name \
        = { l, w1, w2, { b1, b2, b3, b4, b5, b6, b7, b8 } }

// 鎺ュ彛GUID
DEFINE_GUID(IID_IUnknown, 0x00000000, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);
DEFINE_GUID(IID_FileInfo, 0x12345678, 0x1234, 0x1234, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0);
DEFINE_GUID(IID_AbortCallback, 0x23456789, 0x2345, 0x2345, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x01);
DEFINE_GUID(IID_InputDecoder, 0x3456789A, 0x3456, 0x3456, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x12);
DEFINE_GUID(CLSID_InputDecoderService, 0x456789AB, 0x4567, 0x4567, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x01, 0x23);

// 鍓嶅悜澹版槑
class FileInfo;
class AbortCallback;
class InputDecoder;

// 鍩虹绫诲瀷
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

// 绠€鍖朇OM鎺ュ彛
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

// 鏂囦欢淇℃伅鎺ュ彛
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

// 涓鍥炶皟鎺ュ彛
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

// 杈撳叆瑙ｇ爜鍣ㄦ帴鍙?
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

// 鏂囦欢淇℃伅瀹炵幇
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

// 鍝戜腑姝㈠洖璋?
class AbortCallbackDummy : public AbortCallback {
public:
    bool IsAborting() const override { return false; }
};

// 妯℃嫙瑙ｇ爜鍣紙鐢ㄤ簬娴嬭瘯锛?
class MockDecoder : public InputDecoder {
private:
    bool is_open_ = false;
    std::string current_path_;
    AudioInfo audio_info_;
    double position_ = 0.0;
    
public:
    bool Open(const char* path, FileInfo& info, AbortCallback& abort) override {
        if(!path) return false;
        
        // 妯℃嫙鏂囦欢鎵撳紑
        current_path_ = path;
        is_open_ = true;
        position_ = 0.0;
        
        // 璁剧疆闊抽淇℃伅锛堟ā鎷燂級
        audio_info_.sample_rate = 44100;
        audio_info_.channels = 2;
        audio_info_.bitrate = 128;
        audio_info_.length = 180.0; // 3鍒嗛挓
        
        // 璁剧疆鏂囦欢淇℃伅
        info.SetLength(audio_info_.length);
        info.SetAudioInfo(audio_info_);
        info.MetaSet("title", "Test Audio");
        info.MetaSet("artist", "Mock Decoder");
        
        std::cout << "[MockDecoder] Opened: " << path << std::endl;
        return true;
    }
    
    int Decode(float* buffer, int samples, AbortCallback& abort) override {
        if(!is_open_ || !buffer) return 0;
        
        // 鐢熸垚姝ｅ鸡娉㈡祴璇曢煶棰?
        const float frequency = 440.0f; // A4闊崇
        const float amplitude = 0.5f;
        
        for(int i = 0; i < samples; i++) {
            float time = (position_ + float(i) / audio_info_.sample_rate);
            float value = amplitude * sin(2.0f * 3.14159f * frequency * time);
            
            // 绔嬩綋澹?
            buffer[i * 2] = value;      // 宸﹀０閬?
            buffer[i * 2 + 1] = value;  // 鍙冲０閬?
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
        // 鏀寔甯歌闊抽鏍煎紡
        if(!path) return false;
        std::string ext = fs::path(path).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext == ".wav" || ext == ".mp3" || ext == ".flac" || ext == ".ape";
    }
    
    const char* GetName() override { return "Mock Decoder (fb2k鍏煎娴嬭瘯)"; }
};

// 涓绘満绫?
class MiniHost {
private:
    std::vector<std::unique_ptr<InputDecoder>> decoders_;
    
public:
    bool Initialize() {
        std::cout << "[MiniHost] 鍒濆鍖?.." << std::endl;
        
        // 娣诲姞妯℃嫙瑙ｇ爜鍣?
        auto mock_decoder = std::make_unique<MockDecoder>();
        decoders_.push_back(std::move(mock_decoder));
        
        std::cout << "[MiniHost] 鍒濆鍖栧畬鎴愶紝瑙ｇ爜鍣ㄦ暟閲? " << decoders_.size() << std::endl;
        return true;
    }
    
    InputDecoder* CreateDecoderForPath(const std::string& path) {
        std::cout << "[MiniHost] 涓鸿矾寰勫垱寤鸿В鐮佸櫒: " << path << std::endl;
        
        for(const auto& decoder : decoders_) {
            if(decoder->IsOurPath(path.c_str())) {
                std::cout << "[MiniHost] 鎵惧埌鍖归厤鐨勮В鐮佸櫒: " << decoder->GetName() << std::endl;
                return decoder.get();
            }
        }
        
        std::cout << "[MiniHost] 鏈壘鍒板尮閰嶇殑瑙ｇ爜鍣? << std::endl;
        return nullptr;
    }
    
    bool TestDecode(const std::string& audio_file) {
        std::cout << std::endl << "=== 瑙ｇ爜娴嬭瘯寮€濮?===" << std::endl;
        std::cout << "闊抽鏂囦欢: " << audio_file << std::endl;
        
        // 鍒涘缓瑙ｇ爜鍣?
        auto* decoder = CreateDecoderForPath(audio_file);
        if(!decoder) {
            std::cerr << "閿欒: 鏈壘鍒版敮鎸佹鏍煎紡鐨勮В鐮佸櫒" << std::endl;
            return false;
        }
        
        // 鍒涘缓鏂囦欢淇℃伅鍜屼腑姝㈠洖璋?
        auto file_info = std::make_unique<FileInfoImpl>();
        auto abort_cb = std::make_unique<AbortCallbackDummy>();
        
        // 鎵撳紑鏂囦欢
        std::cout << std::endl << "姝ｅ湪鎵撳紑鏂囦欢..." << std::endl;
        if(!decoder->Open(audio_file.c_str(), *file_info, *abort_cb)) {
            std::cerr << "閿欒: 鏃犳硶鎵撳紑鏂囦欢" << std::endl;
            return false;
        }
        
        // 鏄剧ず鏂囦欢淇℃伅
        std::cout << std::endl << "鏂囦欢淇℃伅:" << std::endl;
        std::cout << "  闀垮害: " << file_info->GetLength() << " 绉? << std::endl;
        auto& audio_info = file_info->GetAudioInfo();
        std::cout << "  閲囨牱鐜? " << audio_info.sample_rate << " Hz" << std::endl;
        std::cout << "  澹伴亾鏁? " << audio_info.channels << std::endl;
        std::cout << "  姣旂壒鐜? " << audio_info.bitrate << " kbps" << std::endl;
        
        const char* title = file_info->MetaGet("title", 0);
        const char* artist = file_info->MetaGet("artist", 0);
        if(title) std::cout << "  鏍囬: " << title << std::endl;
        if(artist) std::cout << "  鑹烘湳瀹? " << artist << std::endl;
        
        // 娴嬭瘯瑙ｇ爜
        std::cout << std::endl << "寮€濮嬭В鐮佹祴璇?.." << std::endl;
        const int test_samples = 1024;
        std::vector<float> buffer(test_samples * audio_info.channels);
        
        int total_decoded = 0;
        const int max_iterations = 10; // 闄愬埗娴嬭瘯杞暟
        
        for(int i = 0; i < max_iterations; i++) {
            int decoded = decoder->Decode(buffer.data(), test_samples, *abort_cb);
            if(decoded <= 0) {
                std::cout << "瑙ｇ爜缁撴潫锛屾€诲叡瑙ｇ爜 " << total_decoded << " 涓噰鏍? << std::endl;
                break;
            }
            
            total_decoded += decoded;
            
            // 鏄剧ず杩涘害
            double progress = (double)total_decoded / audio_info.sample_rate;
            std::cout << "  杩涘害: " << std::fixed << std::setprecision(2) << progress << " 绉? << std::endl;
            
            // 妫€鏌ラ煶棰戞暟鎹紙绠€鍗曢獙璇侊級
            float max_amplitude = 0.0f;
            for(int j = 0; j < decoded * audio_info.channels; j++) {
                max_amplitude = std::max(max_amplitude, std::abs(buffer[j]));
            }
            std::cout << "  鏈€澶ф尟骞? " << max_amplitude << std::endl;
        }
        
        // 娴嬭瘯璺宠浆
        if(decoder->CanSeek()) {
            std::cout << std::endl << "娴嬭瘯璺宠浆鍔熻兘..." << std::endl;
            decoder->Seek(1.0, *abort_cb); // 璺宠浆鍒?绉?
        }
        
        // 鍏抽棴瑙ｇ爜鍣?
        std::cout << std::endl << "鍏抽棴瑙ｇ爜鍣?.." << std::endl;
        decoder->Close();
        
        std::cout << std::endl << "=== 瑙ｇ爜娴嬭瘯瀹屾垚 ===" << std::endl;
        std::cout << "鎬昏В鐮侀噰鏍锋暟: " << total_decoded << std::endl;
        std::cout << "娴嬭瘯鏃堕暱: " << (double)total_decoded / audio_info.sample_rate << " 绉? << std::endl;
        
        return true;
    }
};

// 娴嬭瘯闊抽鏂囦欢鐢熸垚
void CreateTestWav(const std::string& filename) {
    std::cout << "鍒涘缓娴嬭瘯闊抽鏂囦欢: " << filename << std::endl;
    
    // 绠€鍗曠殑WAV鏂囦欢澶?+ 姝ｅ鸡娉㈡暟鎹?
    struct WavHeader {
        char riff[4] = {'R', 'I', 'F', 'F'};
        uint32_t size = 36 + 44100 * 2 * 2; // 1绉掞紝16bit锛岀珛浣撳０
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
    
    // 鐢熸垚1绉掓寮︽尝
    const int samples = 44100;
    const float frequency = 440.0f; // A4
    const float amplitude = 0.5f;
    
    for(int i = 0; i < samples; i++) {
        float time = (float)i / 44100.0f;
        float value = amplitude * sin(2.0f * 3.14159f * frequency * time);
        
        // 杞崲涓?6bit鏁存暟
        int16_t sample = static_cast<int16_t>(value * 32767.0f);
        
        // 绔嬩綋澹?
        file.write(reinterpret_cast<const char*>(&sample), sizeof(sample));
        file.write(reinterpret_cast<const char*>(&sample), sizeof(sample));
    }
    
    file.close();
    std::cout << "娴嬭瘯闊抽鏂囦欢鍒涘缓瀹屾垚" << std::endl;
}

// 涓诲嚱鏁?
int main(int argc, char* argv[]) {
    std::cout << "=== foobar2000 鍏煎灞傞樁娈?娴嬭瘯 ===" << std::endl;
    std::cout << "绠€鍖栫増鍗曟枃浠舵祴璇曠▼搴? << std::endl;
    std::cout << "=====================================" << std::endl << std::endl;
    
    // 鍒濆鍖朇OM
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if(FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        std::cerr << "COM鍒濆鍖栧け璐? 0x" << std::hex << hr << std::endl;
        return 1;
    }
    
    // 鍒涘缓涓绘満
    MiniHost host;
    if(!host.Initialize()) {
        std::cerr << "涓绘満鍒濆鍖栧け璐? << std::endl;
        CoUninitialize();
        return 1;
    }
    
    // 鍒涘缓鎴栨煡鎵炬祴璇曟枃浠?
    std::string test_file = "test_sine.wav";
    if(!fs::exists(test_file)) {
        std::cout << "鏈壘鍒版祴璇曟枃浠讹紝鍒涘缓娴嬭瘯闊抽..." << std::endl;
        CreateTestWav(test_file);
    }
    
    if(!fs::exists(test_file)) {
        std::cerr << "鏃犳硶鍒涘缓娴嬭瘯鏂囦欢" << std::endl;
        CoUninitialize();
        return 1;
    }
    
    std::cout << std::endl << "浣跨敤娴嬭瘯鏂囦欢: " << test_file << std::endl;
    
    // 杩愯瑙ｇ爜娴嬭瘯
    bool success = host.TestDecode(test_file);
    
    if(success) {
        std::cout << std::endl << "鉁?娴嬭瘯鎴愬姛! foobar2000 鍏煎灞傛鏋跺伐浣滄甯搞€? << std::endl;
        std::cout << "铏界劧浣跨敤鐨勬槸妯℃嫙瑙ｇ爜鍣紝浣嗘帴鍙ｆ灦鏋勯獙璇侀€氳繃銆? << std::endl;
    } else {
        std::cout << std::endl << "鉂?娴嬭瘯澶辫触锛岃妫€鏌ラ敊璇俊鎭€? << std::endl;
    }
    
    CoUninitialize();
    return success ? 0 : 1;
}