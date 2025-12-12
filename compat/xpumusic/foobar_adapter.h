/**
 * @file foobar_adapter.h
 * @brief XpuMusic鎻掍欢鍏煎閫傞厤鍣?
 * @date 2025-12-10
 */

#pragma once

#include "../../sdk/xpumusic_plugin_sdk.h"
#include "../xpumusic_sdk/foobar2000.h"
#include <memory>
#include <vector>
#include <unordered_map>

namespace xpumusic::compat {

/**
 * @brief XpuMusic瑙ｇ爜鍣ㄥ埌XpuMusic鎺ュ彛鐨勯€傞厤鍣?
 */
class FoobarDecoderAdapter : public IAudioDecoder {
private:
    xpumusic::service_ptr_t<xpumusic::input_decoder> foobar_decoder_;
    std::string file_path_;
    AudioFormat format_;
    int64_t total_samples_;

public:
    explicit FoobarDecoderAdapter(xpumusic::input_decoder* decoder);
    ~FoobarDecoderAdapter() override = default;

    // IPlugin鎺ュ彛
    bool initialize() override;
    void finalize() override;
    PluginInfo get_info() const override;
    PluginState get_state() const override;
    void set_state(PluginState state) override;
    std::string get_last_error() const override;

    // IAudioDecoder鎺ュ彛
    bool can_decode(const std::string& file_path) override;
    std::vector<std::string> get_supported_extensions() override;
    bool open(const std::string& file_path) override;
    int decode(AudioBuffer& buffer, int max_frames) override;
    bool seek(int64_t sample_pos) override;
    void close() override;
    AudioFormat get_format() const override;
    int64_t get_length() const override;
    double get_duration() const override;
    std::vector<MetadataItem> get_metadata() override;
    std::string get_metadata_value(const std::string& key) override;
    int64_t get_position() const override;
    bool is_eof() const override;
};

/**
 * @brief XpuMusic鎻掍欢鍖呰鍣?
 */
class FoobarPluginWrapper {
private:
    std::string plugin_path_;
    void* library_handle_;
    std::vector<std::unique_ptr<IPluginFactory>> adapter_factories_;

public:
    FoobarPluginWrapper();
    ~FoobarPluginWrapper();

    // 鍔犺浇XpuMusic鎻掍欢
    bool load_plugin(const std::string& path);

    // 鑾峰彇閫傞厤鍚庣殑瑙ｇ爜鍣?
    std::vector<std::unique_ptr<IAudioDecoder>> get_decoders();

    // 鑾峰彇鎻掍欢淇℃伅
    std::vector<PluginInfo> get_plugin_info() const;

private:
    // 鏋氫妇鎻掍欢鎻愪緵鐨勬湇鍔?
    void enumerate_services();

    // 鍒涘缓瑙ｇ爜鍣ㄩ€傞厤鍣?
    void create_decoder_adapter(const xpumusic::GUID& guid,
                                const std::string& name);
};

/**
 * @brief foobar瑙ｇ爜鍣ㄩ€傞厤鍣ㄥ伐鍘?
 */
class FoobarDecoderFactory : public ITypedPluginFactory<IAudioDecoder> {
private:
    std::function<std::unique_ptr<IAudioDecoder>()> creator_;

public:
    explicit FoobarDecoderFactory(std::function<std::unique_ptr<IAudioDecoder>()> creator)
        : creator_(std::move(creator)) {}

    std::unique_ptr<IAudioDecoder> create_typed() override {
        return creator_();
    }

    PluginInfo get_info() const override;
};

} // namespace xpumusic::compat