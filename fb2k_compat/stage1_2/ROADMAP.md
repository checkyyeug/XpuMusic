# 阶段1.2：功能扩展路线图

## 🎯 阶段目标
实现完整的音频处理链路：Input → DSP → Output，支持真实foobar2000的DSP效果和音频输出组件。

## 📅 时间安排：2-3周（2025年12月11日 - 2026年1月1日）

## 📋 核心任务

### Week 1: DSP组件支持 (优先级：🔴 高)
- [ ] 实现DSP预设接口 (`dsp_preset`)
- [ ] 实现DSP音频块接口 (`audio_chunk`)
- [ ] 实现DSP链管理器
- [ ] 集成foo_dsp_std组件测试
- [ ] 支持基础DSP效果（均衡器、音量等）

### Week 2: Output组件支持 (优先级：🔴 高)
- [ ] 实现音频输出设备接口 (`output_device`)
- [ ] 实现音频块数据传递
- [ ] 集成foo_out_wasapi组件测试
- [ ] 支持多种输出格式（PCM、浮点等）
- [ ] 实现缓冲管理

### Week 3: 配置系统 + 集成测试 (优先级：🔴 高)
- [ ] 实现cfg_var配置系统
- [ ] 实现config_io配置持久化
- [ ] 完整音频链路测试
- [ ] 性能基准测试
- [ ] 错误处理和稳定性优化

## 🔧 技术架构

### 音频处理链路
```
音频文件 → InputDecoder → [DSP1] → [DSP2] → ... → OutputDevice → 音频输出
     ↑         ↑           ↑        ↑           ↑         ↑
  文件系统   解码器     DSP效果   DSP效果    输出设备   音频驱动
```

### 核心接口层次
```
ServiceBase (服务基类)
    ├── InputDecoder (输入解码 - 已完成)
    ├── DSPPreset (DSP预设 - 新增)
    ├── AudioChunk (音频块 - 新增)
    ├── OutputDevice (输出设备 - 新增)
    └── ConfigVar (配置变量 - 新增)
```

### 数据流
```
音频数据流：解码器 → DSP链 → 输出设备
配置流：配置文件 → cfg_var系统 → 各组件
控制流：播放控制 → 状态管理 → 组件协调
```

## 🎯 详细实现计划

### 第1周：DSP组件支持

#### Day 1-2: 音频块接口 (audio_chunk)
```cpp
class audio_chunk {
    // 音频数据容器
    virtual float* get_data() = 0;
    virtual size_t get_sample_count() = 0;
    virtual uint32_t get_sample_rate() = 0;
    virtual uint32_t get_channels() = 0;
    virtual uint32_t get_channel_config() = 0;
    virtual double get_duration() = 0;
    
    // 数据操作
    virtual void set_data(const float* data, size_t samples, 
                         uint32_t channels, uint32_t sample_rate) = 0;
    virtual void copy(const audio_chunk& source) = 0;
    virtual void reset() = 0;
};
```

#### Day 3-4: DSP预设接口 (dsp_preset)
```cpp
class dsp_preset {
    // DSP配置管理
    virtual void reset() = 0;
    virtual bool is_valid() const = 0;
    virtual void copy(const dsp_preset& source) = 0;
    virtual const char* get_name() const = 0;
    virtual void set_name(const char* name) = 0;
};

class dsp_preset_impl : public dsp_preset {
    // 具体实现
    std::string name_;
    std::map<std::string, std::string> params_;
};
```

#### Day 5-7: DSP效果器接口 (dsp)
```cpp
class dsp : public service_base {
    // DSP效果器基类
    virtual bool instantiate(audio_chunk& chunk, uint32_t sample_rate, 
                            uint32_t channels) = 0;
    virtual void run(audio_chunk& chunk, abort_callback& abort) = 0;
    virtual void reset() = 0;
    virtual bool need_track_change_mark() const = 0;
    virtual double get_latency() const = 0;
};

class dsp_chain {
    // DSP链管理器
    std::vector<service_ptr_t<dsp>> effects_;
    void run_chain(audio_chunk& chunk, abort_callback& abort);
};
```

### 第2周：Output组件支持

#### Day 8-9: 输出设备接口 (output_device)
```cpp
class output_device : public service_base {
    // 输出设备基类
    virtual bool open(uint32_t sample_rate, uint32_t channels, 
                     abort_callback& abort) = 0;
    virtual void close(abort_callback& abort) = 0;
    virtual void process_chunk(audio_chunk& chunk, abort_callback& abort) = 0;
    virtual void flush(abort_callback& abort) = 0;
    virtual double get_latency() const = 0;
    virtual bool can_update_format() const = 0;
};
```

#### Day 10-11: WASAPI输出实现
```cpp
class output_wasapi : public output_device {
    // WASAPI具体实现
    IMMDevice* device_;
    IAudioClient* audio_client_;
    IAudioRenderClient* render_client_;
    
    bool open(uint32_t sample_rate, uint32_t channels, abort_callback& abort) override;
    void process_chunk(audio_chunk& chunk, abort_callback& abort) override;
};
```

#### Day 12-14: 缓冲管理和格式转换
```cpp
class audio_buffer {
    // 音频缓冲管理
    std::vector<float> buffer_;
    size_t write_pos_;
    size_t read_pos_;
    
    void write(const float* data, size_t samples);
    size_t read(float* data, size_t samples);
    bool is_empty() const;
};

class format_converter {
    // 音频格式转换
    static void convert_float_to_int16(const float* src, int16_t* dst, size_t samples);
    static void convert_int16_to_float(const int16_t* src, float* dst, size_t samples);
};
```

### 第3周：配置系统 + 集成测试

#### Day 15-17: 配置系统 (cfg_var)
```cpp
class cfg_var {
    // 配置变量基类
    virtual void get_data(stream_writer& stream) const = 0;
    virtual void set_data(stream_reader& stream, size_t size) = 0;
    virtual void reset() = 0;
    virtual bool is_default() const = 0;
};

template<typename T>
class cfg_var_t : public cfg_var {
    // 模板化的配置变量
    T value_;
    T default_value_;
    
public:
    const T& get() const { return value_; }
    void set(const T& value) { value_ = value; }
    void reset() { value_ = default_value_; }
};

class cfg_var_manager {
    // 配置变量管理器
    std::map<std::string, std::unique_ptr<cfg_var>> vars_;
    
    void load_config(const std::string& path);
    void save_config(const std::string& path);
};
```

#### Day 18-19: 播放控制接口
```cpp
class playback_control : public service_base {
    // 播放控制核心接口
    virtual bool is_playing() const = 0;
    virtual bool is_paused() const = 0;
    virtual double get_position() const = 0;
    virtual void set_position(double position) = 0;
    virtual double get_length() const = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void pause(bool pause_state) = 0;
};

class playback_engine {
    // 播放引擎实现
    service_ptr_t<input_decoder> decoder_;
    std::unique_ptr<dsp_chain> dsp_chain_;
    service_ptr_t<output_device> output_;
    
    void process_audio();
    void handle_state_changes();
};
```

#### Day 20-21: 集成测试和优化
- 完整音频链路测试
- 性能基准测试
- 错误处理和稳定性优化

## 🧪 测试策略

### 组件测试
```cpp
// DSP测试
TEST(dsp_test, basic_equalizer) {
    auto dsp = create_dsp_equalizer();
    audio_chunk chunk(1024, 2, 44100);
    
    // 生成测试音频
    generate_test_signal(chunk, 1000.0f);  // 1kHz正弦波
    
    // 应用DSP
    dsp->run(chunk, abort_callback_dummy());
    
    // 验证输出
    verify_frequency_response(chunk, 1000.0f);
}

// Output测试
TEST(output_test, wasapi_basic) {
    auto output = create_output_wasapi();
    
    EXPECT_TRUE(output->open(44100, 2, abort_callback_dummy()));
    
    audio_chunk chunk(1024, 2, 44100);
    generate_test_signal(chunk, 440.0f);  // A4音符
    
    output->process_chunk(chunk, abort_callback_dummy());
    
    output->close(abort_callback_dummy());
}
```

### 集成测试
```cpp
TEST(integration_test, complete_audio_chain) {
    // 创建完整音频链路
    auto input = create_input_decoder("test.mp3");
    auto dsp_chain = create_dsp_chain();
    auto output = create_output_wasapi();
    
    // 配置DSP链路
    dsp_chain->add_effect(create_dsp_equalizer());
    dsp_chain->add_effect(create_dsp_volume());
    
    // 打开音频链路
    ASSERT_TRUE(input->open("test.mp3", file_info, abort));
    ASSERT_TRUE(output->open(44100, 2, abort));
    
    // 处理音频数据
    audio_chunk chunk;
    while(input->decode(chunk.get_data(), 1024, abort) > 0) {
        dsp_chain->run(chunk, abort);
        output->process_chunk(chunk, abort);
    }
    
    // 清理
    input->close();
    output->close(abort);
}
```

## 📊 性能目标

### DSP性能
- DSP处理延迟: < 1ms
- CPU占用: < 5% (单效果)
- 内存使用: < 10MB (DSP链)

### Output性能
- 输出延迟: < 10ms (WASAPI)
- 缓冲大小: 可配置 (默认10-50ms)
- 格式转换: 零拷贝优化

### 整体性能
- 完整链路延迟: < 20ms
- CPU总占用: < 15% (播放+DSP)
- 内存总使用: < 50MB

## 🔍 调试和诊断

### 调试工具
- **音频分析器**: 频谱分析、波形显示
- **性能分析器**: CPU/内存使用率
- **延迟测量器**: 链路延迟测试
- **日志系统**: 详细运行日志

### 诊断功能
- **组件验证**: 自动检查组件有效性
- **格式支持**: 显示支持的音频格式
- **错误定位**: 精确定位问题组件
- **性能分析**: 瓶颈识别和优化建议

## 🎯 成功标准

### 必须实现 (MVP)
- ✅ 支持foo_dsp_std基础效果
- ✅ 支持foo_out_wasapi音频输出
- ✅ 完整的Input→DSP→Output链路
- ✅ 基础配置持久化
- ✅ 性能达到目标要求

### 期望实现 (Nice to have)
- 🎯 支持5+种DSP效果组件
- 🎯 支持3+种输出设备
- 🎯 高级配置界面
- 🎯 实时性能监控
- 🎯 插件热加载

## 🚀 技术风险与缓解

### 主要风险
1. **组件兼容性** - 某些组件可能有特殊依赖
   - *缓解*: 分步骤测试，优先支持主流组件
   
2. **性能瓶颈** - DSP链可能引入延迟
   - *缓解*: 零拷贝优化，异步处理
   
3. **配置复杂性** - fb2k配置系统复杂
   - *缓解*: 逐步实现，优先核心功能

### 技术挑战
1. **WASAPI独占模式** - 需要精细的缓冲管理
2. **DSP实时处理** - 需要高效的算法实现
3. **多线程协调** - 播放、DSP、输出的线程同步

---

## 🎯 下一步行动

### 立即开始 (今天)
1. **创建阶段1.2目录结构**
2. **实现audio_chunk接口基础**
3. **开始DSP预设接口设计**

### 本周目标
- ✅ 完成audio_chunk和dsp_preset接口
- ✅ 实现基础DSP链管理器
- ✅ 集成第一个DSP效果测试

### 成功指标
- 🎯 能够加载并运行foo_dsp_std
- 🎯 音频处理链路工作正常
- 🎯 性能达到预期要求

---

**🚀 让我们开始创造音频处理的新历史！** 

**阶段1.2目标**: 实现完整的Input→DSP→Output音频处理链路！