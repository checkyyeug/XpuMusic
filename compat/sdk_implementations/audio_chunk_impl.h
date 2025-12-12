/**
 * @file audio_chunk_impl.h
 * @brief audio_chunk 鎺ュ彛瀹炵幇
 * @date 2025-12-09
 * 
 * audio_chunk 鏄?foobar2000 闊抽澶勭悊绠￠亾鐨勬牳蹇冩暟鎹粨鏋勩€? * 瀹冨皢闊抽鏁版嵁鍜屾牸寮忎俊鎭竴璧峰皝瑁咃紝渚夸簬 DSP 澶勭悊銆? */

#pragma once

#include "common_includes.h"
#include "audio_chunk_interface.h"
#include "audio_sample.h"
#include "file_info_types.h"
#include <memory>
#include <vector>
#include <cstring>

namespace foobar2000_sdk {

/**
 * @class audio_chunk_impl
 * @brief audio_chunk 鎺ュ彛鐨勫畬鏁村疄鐜? * 
 * 鍏抽敭鐗规€э細
 * - 鍙皟鏁村ぇ灏忕殑闊抽鏁版嵁缂撳啿鍖? * - 澶氱闊抽鏍煎紡鏀寔锛圥CM int/float锛? * - 閫氶亾閲嶆槧灏勫拰鍒犻櫎
 * - 閲囨牱鐜囪浆鎹紙浣跨敤 DSP锛? * - 澧炵泭搴旂敤鍜屽嘲鍊兼壂鎻? */
class audio_chunk_impl : public audio_chunk_interface {
private:
    /**
     * @struct buffer_t
     * @brief 鍐呴儴闊抽鏁版嵁缂撳啿鍖?     */
    struct buffer_t {
        std::vector<audio_sample> data;  // 浣跨敤 vector 绠＄悊鍐呭瓨
        uint32_t sample_count;           // 闊抽鏍锋湰鐨勫抚鏁?        uint32_t channels;               // 闊抽閫氶亾鏁?        uint32_t sample_rate;            // 閲囨牱鐜?        
        buffer_t() : sample_count(0), channels(0), sample_rate(0) {}
        
        /**
         * @brief 璁剧疆缂撳啿鍖哄ぇ灏?         * @param new_size 鏂扮殑澶у皬锛堟牱鏈暟锛?         */
        void resize(size_t new_size) {
            data.resize(new_size);
            sample_count = static_cast<uint32_t>(new_size / (channels > 0 ? channels : 1));
        }
        
        /**
         * @brief 璁剧疆鏍煎紡淇℃伅
         */
        void set_format(uint32_t rate, uint32_t ch) {
            sample_rate = rate;
            channels = ch;
        }
    };
    
    buffer_t buffer_;  // 闊抽鏁版嵁缂撳啿鍖?
public:
    /**
     * @brief 鏋勯€犲嚱鏁?     */
    audio_chunk_impl();
    
    /**
     * @brief 鏋愭瀯鍑芥暟
     */
    ~audio_chunk_impl() override = default;
    
    /**
     * @brief 璁剧疆闊抽鏁版嵁锛堟繁鎷疯礉锛?     * @param p_data 闊抽鏍锋湰鏁版嵁锛堜氦閿欐牸寮忥級
     * @param p_sample_count 鏍锋湰甯ф暟锛堟瘡涓€氶亾锛?     * @param p_channels 閫氶亾鏁?     * @param p_sample_rate 閲囨牱鐜?     */
    void set_data(const audio_sample* p_data, size_t p_sample_count, 
                  uint32_t p_channels, uint32_t p_sample_rate) override;
    
    /**
     * @brief 璁剧疆闊抽鏁版嵁锛堢Щ鍔ㄨ涔夛級
     * @param p_data 鎸囧悜鏍锋湰鏁版嵁鐨勬寚閽堬紙灏嗚鎺ョ锛?     * @param p_sample_count 鏍锋湰甯ф暟
     * @param p_channels 閫氶亾鏁?     * @param p_sample_rate 閲囨牱鐜?     * @param p_data_free 閲婃斁鍥炶皟
     */
    void set_data(const audio_sample* p_data, size_t p_sample_count,
                  uint32_t p_channels, uint32_t p_sample_rate, 
                  void (* p_data_free)(audio_sample*)) override;
    
    /**
     * @brief 杩藉姞闊抽鏁版嵁鍒板綋鍓嶇紦鍐插尯
     * @param p_data 瑕佽拷鍔犵殑鏍锋湰
     * @param p_sample_count 鏍锋湰甯ф暟
     * @param p_channels 閫氶亾鏁帮紙蹇呴』鍖归厤褰撳墠鏍煎紡锛?     * @param p_sample_rate 閲囨牱鐜囷紙蹇呴』鍖归厤褰撳墠鏍煎紡锛?     */
    void data_append(const audio_sample* p_data, size_t p_sample_count) override;
    
    /**
     * @brief 淇濈暀缂撳啿鍖虹┖闂?     * @param p_sample_count 瑕佷繚鐣欑殑鏍锋湰甯ф暟
     */
    void data_pad(size_t p_sample_count) override;
    
    /**
     * @brief 鑾峰彇闊抽鏍锋湰鏁版嵁锛坈onst 鐗堟湰锛?     * @return 鎸囧悜鏍锋湰鏁版嵁鐨勬寚閽堬紙浜ら敊鏍煎紡锛歀RLRLR...锛?     */
    audio_sample* get_data() override { return buffer_.data.data(); }
    
    /**
     * @brief 鑾峰彇闊抽鏍锋湰鏁版嵁锛堥潪 const 鐗堟湰锛?     * @return 鎸囧悜鏍锋湰鏁版嵁鐨勬寚閽?     */
    const audio_sample* get_data() const override { return buffer_.data.data(); }
    
    /**
     * @brief 鑾峰彇鏍锋湰甯ф暟锛堟瘡涓€氶亾锛?     * @return 鏍锋湰甯ф暟
     */
    uint32_t get_sample_count() const override { return buffer_.sample_count; }
    
    /**
     * @brief 鑾峰彇閫氶亾鏁?     * @return 閫氶亾鏁?     */
    uint32_t get_channels() const override { return buffer_.channels; }
    
    /**
     * @brief 鑾峰彇閲囨牱鐜?     * @return 閲囨牱鐜囷紙Hz锛?     */
    uint32_t get_sample_rate() const override { return buffer_.sample_rate; }
    
    /**
     * @brief 鑾峰彇闊抽鏁版嵁鎬诲ぇ灏忥紙浠ユ牱鏈负鍗曚綅锛?     * @return 鎵€鏈夐€氶亾鐨勬€绘牱鏈暟
     */
    uint32_t get_data_size() const override { 
        return buffer_.sample_count * buffer_.channels; 
    }
    
    /**
     * @brief 鑾峰彇闊抽鏁版嵁澶у皬锛堜互瀛楄妭涓哄崟浣嶏級
     * @return 瀛楄妭澶у皬
     */
    size_t get_data_bytes() const override { 
        return buffer_.data.size() * sizeof(audio_sample);
    }
    
    /**
     * @brief 鑾峰彇鐗瑰畾閫氶亾鐨勬暟鎹寚閽?     * @param p_channel 閫氶亾绱㈠紩锛? 寮€濮嬶級
     * @return 鎸囧悜璇ラ€氶亾鏁版嵁鐨勬寚閽?     */
    audio_sample* get_channel_data(uint32_t p_channel) override;
    
    /**
     * @brief 鑾峰彇鐗瑰畾閫氶亾鐨勬暟鎹寚閽堬紙const 鐗堟湰锛?     * @param p_channel 閫氶亾绱㈠紩
     * @return 鎸囧悜璇ラ€氶亾鏁版嵁鐨勬寚閽?     */
    const audio_sample* get_channel_data(uint32_t p_channel) const override;
    
    /**
     * @brief 璁剧疆闊抽鏍煎紡淇℃伅锛堜笉淇敼鏁版嵁锛?     */
    void set_sample_rate(uint32_t p_sample_rate) override { 
        buffer_.sample_rate = p_sample_rate; 
    }
    
    /**
     * @brief 璁剧疆閫氶亾鏁?     * @param p_channels 閫氶亾鏁?     * @param p_preserve_data 鏄惁涓洪€氶亾鍒犻櫎淇濈暀鏁版嵁
     */
    void set_channels(uint32_t p_channels, bool p_preserve_data) override;
    
    /**
     * @brief 璁剧疆閲囨牱鐜囷紙resample 妯″紡锛?     * @param p_sample_rate 鏂伴噰鏍风巼
     * @param p_mode resample 妯″紡
     */
    void set_sample_rate(uint32_t p_sample_rate, t_resample_mode p_mode) override;
    
    /**
     * @brief 杞崲鏍锋湰鏍煎紡锛坒loat -> int, int -> float锛?     * @param p_target_format 鐩爣鏍煎紡
     */
    void convert(t_sample_point_format p_target_format) override;
    
    /**
     * @brief 搴旂敤澧炵泭鍒版墍鏈夋牱鏈?     * @param p_gain 绾挎€у鐩婂€硷紙1.0 = 鏃犲彉鍖栵級
     */
    void scale(const audio_sample& p_gain) override;
    
    /**
     * @brief 搴旂敤澶氫釜澧炵泭锛堟瘡涓€氶亾涓嶅悓锛?     * @param p_gain 姣忎釜閫氶亾鐨勫鐩婃暟缁?     */
    void scale(const audio_sample* p_gain) override;
    
    /**
     * @brief 璁＄畻闊抽鏁版嵁鐨勫嘲鍊肩數骞?     * @param p_peak 杈撳嚭宄板€兼暟缁勶紙澶у皬 = 閫氶亾鏁帮級
     */
    void calculate_peak(audio_sample* p_peak) const override;
    
    /**
     * @brief 鍒犻櫎鎸囧畾閫氶亾
     * @param p_channel 瑕佸垹闄ょ殑閫氶亾绱㈠紩
     */
    void remove_channel(uint32_t p_channel) override;
    
    /**
     * @brief 澶嶅埗鐗瑰畾閫氶亾鍒版柊缂撳啿鍖?     * @param p_channel 婧愰€氶亾绱㈠紩
     * @param p_target 鐩爣 audio_chunk
     */
    void copy_channel_to(uint32_t p_channel, audio_chunk_interface& p_target) const override;
    
    /**
     * @brief 浠庡彟涓€涓?chunk 澶嶅埗鐗瑰畾閫氶亾
     * @param p_channel 婧愰€氶亾绱㈠紩
     * @param p_source 婧?audio_chunk
     */
    void copy_channel_from(uint32_t p_channel, const audio_chunk_interface& p_source) override;
    
    /**
     * @brief 浠庡彟涓€涓?chunk 杩藉姞鎵€鏈夐€氶亾
     * @param p_source 婧?audio_chunk
     */
    void duplicate(const audio_chunk_interface& p_source) override;
    
    /**
     * @brief 浠庡彟涓€涓?chunk 娣烽煶
     * @param p_source 瑕佹贩鍏ョ殑婧?     * @param p_count 瑕佹贩闊崇殑鏍锋湰鏁?     */
    void combine(const audio_chunk_interface& p_source, size_t p_count) override;
    
    /**
     * @brief 娣辨嫹璐濆彟涓€涓?chunk 鐨勬墍鏈夋暟鎹?     * @param p_source 婧?audio_chunk
     */
    void copy(const audio_chunk_interface& p_source) override;
    
    /**
     * @brief 浠庡彟涓€涓?chunk 澶嶅埗鍏冩暟鎹紙鏍煎紡銆佺姸鎬侊級
     * @param p_source 婧?audio_chunk
     */
    void copy_meta(const audio_chunk_interface& p_source) override;
    
    /**
     * @brief 閲嶇疆鎵€鏈夋暟鎹紙娓呯┖缂撳啿鍖猴級
     */
    void reset() override;
    
    /**
     * @brief 鍙嶅悜閬嶅巻鏌ユ壘鏍锋湰
     * @param p_start 璧峰浣嶇疆
     * @return 鏈夋晥鐨勫抚浣嶇疆鎴?0
     */
    uint32_t find_peaks(uint32_t p_start) const override;
    
    /**
     * @brief 浜ゆ崲涓や釜 chunk 鐨勫唴瀹?     * @param p_other 瑕佷氦鎹㈢殑 chunk
     */
    void swap(audio_chunk_interface& p_other) override;
    
    /**
     * @brief 娴嬭瘯涓や釜 chunk 鏄惁鏁版嵁鐩哥瓑
     * @param p_other 瑕佹瘮杈冪殑 chunk
     * @return true 濡傛灉鏁版嵁鐩哥瓑
     */
    bool audio_data_equals(const audio_chunk_interface& p_other) const override;
};

// 杈呭姪鍑芥暟锛氬垱寤?audio_chunk
std::unique_ptr<audio_chunk_impl> audio_chunk_create();

} // namespace foobar2000_sdk
