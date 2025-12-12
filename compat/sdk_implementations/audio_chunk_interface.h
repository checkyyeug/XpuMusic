/**
 * @file audio_chunk_interface.h
 * @brief audio_chunk 鎶借薄鎺ュ彛瀹氫箟
 * @date 2025-12-09
 */

#pragma once

#include "common_includes.h"
#include <memory>

namespace foobar2000_sdk {

// 鍓嶅悜澹版槑
typedef float audio_sample;

// 閲囨牱鐐规牸寮?enum t_sample_point_format {
    sample_point_format_float32 = 0,
    sample_point_format_int16 = 1,
    sample_point_format_int32 = 2,
    sample_point_format_int24 = 3,
};

// 閲嶉噰鏍锋ā寮?enum t_resample_mode {
    resample_disabled = 0,
    resample_direct = 1,
    resample_preserve_length = 2,
};

/**
 * @class audio_chunk_interface
 * @brief audio_chunk 鎶借薄鎺ュ彛
 */
class audio_chunk_interface {
public:
    virtual ~audio_chunk_interface() = default;
    
    /**
     * @brief 璁剧疆闊抽鏁版嵁锛堟繁鎷疯礉锛?     * @param p_data 闊抽鏍锋湰鏁版嵁锛堜氦閿欐牸寮忥級
     * @param p_sample_count 鏍锋湰甯ф暟锛堟瘡涓€氶亾锛?     * @param p_channels 閫氶亾鏁?     * @param p_sample_rate 閲囨牱鐜?     */
    virtual void set_data(const audio_sample* p_data, size_t p_sample_count, 
                          uint32_t p_channels, uint32_t p_sample_rate) = 0;
    
    /**
     * @brief 璁剧疆闊抽鏁版嵁锛堢Щ鍔ㄨ涔夛級
     * @param p_data 鎸囧悜鏍锋湰鏁版嵁鐨勬寚閽堬紙灏嗚鎺ョ锛?     * @param p_sample_count 鏍锋湰甯ф暟
     * @param p_channels 閫氶亾鏁?     * @param p_sample_rate 閲囨牱鐜?     * @param p_data_free 閲婃斁鍥炶皟
     */
    virtual void set_data(const audio_sample* p_data, size_t p_sample_count,
                          uint32_t p_channels, uint32_t p_sample_rate, 
                          void (* p_data_free)(audio_sample*)) = 0;
    
    /**
     * @brief 杩藉姞闊抽鏁版嵁鍒板綋鍓嶇紦鍐插尯
     * @param p_data 瑕佽拷鍔犵殑鏍锋湰
     * @param p_sample_count 鏍锋湰甯ф暟
     * @param p_channels 閫氶亾鏁帮紙蹇呴』鍖归厤褰撳墠鏍煎紡锛?     * @param p_sample_rate 閲囨牱鐜囷紙蹇呴』鍖归厤褰撳墠鏍煎紡锛?     */
    virtual void data_append(const audio_sample* p_data, size_t p_sample_count) = 0;
    
    /**
     * @brief 淇濈暀缂撳啿鍖虹┖闂?     * @param p_sample_count 瑕佷繚鐣欑殑鏍锋湰甯ф暟
     */
    virtual void data_pad(size_t p_sample_count) = 0;
    
    /**
     * @brief 鑾峰彇闊抽鏍锋湰鏁版嵁锛坈onst 鐗堟湰锛?     * @return 鎸囧悜鏍锋湰鏁版嵁鐨勬寚閽堬紙浜ら敊鏍煎紡锛歀RLRLR...锛?     */
    virtual const audio_sample* get_data() const = 0;
    
    /**
     * @brief 鑾峰彇闊抽鏍锋湰鏁版嵁锛堥潪 const 鐗堟湰锛?     * @return 鎸囧悜鏍锋湰鏁版嵁鐨勬寚閽?     */
    virtual audio_sample* get_data() = 0;
    
    /**
     * @brief 鑾峰彇鏍锋湰甯ф暟锛堟瘡涓€氶亾锛?     * @return 鏍锋湰甯ф暟
     */
    virtual uint32_t get_sample_count() const = 0;
    
    /**
     * @brief 鑾峰彇閫氶亾鏁?     * @return 閫氶亾鏁?     */
    virtual uint32_t get_channels() const = 0;
    
    /**
     * @brief 鑾峰彇閲囨牱鐜?     * @return 閲囨牱鐜囷紙Hz锛?     */
    virtual uint32_t get_sample_rate() const = 0;
    
    /**
     * @brief 鑾峰彇闊抽鏁版嵁鎬诲ぇ灏忥紙浠ユ牱鏈负鍗曚綅锛?     * @return 鎵€鏈夐€氶亾鐨勬€绘牱鏈暟
     */
    virtual uint32_t get_data_size() const = 0;
    
    /**
     * @brief 鑾峰彇闊抽鏁版嵁澶у皬锛堜互瀛楄妭涓哄崟浣嶏級
     * @return 瀛楄妭澶у皬
     */
    virtual size_t get_data_bytes() const = 0;
    
    /**
     * @brief 鑾峰彇鐗瑰畾閫氶亾鐨勬暟鎹寚閽?     * @param p_channel 閫氶亾绱㈠紩锛? 寮€濮嬶級
     * @return 鎸囧悜璇ラ€氶亾鏁版嵁鐨勬寚閽?     */
    virtual audio_sample* get_channel_data(uint32_t p_channel) = 0;
    
    /**
     * @brief 鑾峰彇鐗瑰畾閫氶亾鐨勬暟鎹寚閽堬紙const 鐗堟湰锛?     * @param p_channel 閫氶亾绱㈠紩
     * @return 鎸囧悜璇ラ€氶亾鏁版嵁鐨勬寚閽?     */
    virtual const audio_sample* get_channel_data(uint32_t p_channel) const = 0;
    
    /**
     * @brief 璁剧疆闊抽鏍煎紡淇℃伅锛堜笉淇敼鏁版嵁锛?     */
    virtual void set_sample_rate(uint32_t p_sample_rate) = 0;
    
    /**
     * @brief 璁剧疆閫氶亾鏁?     * @param p_channels 閫氶亾鏁?     * @param p_preserve_data 鏄惁涓洪€氶亾鍒犻櫎淇濈暀鏁版嵁
     */
    virtual void set_channels(uint32_t p_channels, bool p_preserve_data) = 0;
    
    /**
     * @brief 璁剧疆閲囨牱鐜囷紙resample 妯″紡锛?     * @param p_sample_rate 鏂伴噰鏍风巼
     * @param p_mode resample 妯″紡
     */
    virtual void set_sample_rate(uint32_t p_sample_rate, t_resample_mode p_mode) = 0;
    
    /**
     * @brief 杞崲鏍锋湰鏍煎紡锛坒loat -> int, int -> float锛?     * @param p_target_format 鐩爣鏍煎紡
     */
    virtual void convert(t_sample_point_format p_target_format) = 0;
    
    /**
     * @brief 搴旂敤澧炵泭鍒版墍鏈夋牱鏈?     * @param p_gain 绾挎€у鐩婂€硷紙1.0 = 鏃犲彉鍖栵級
     */
    virtual void scale(const audio_sample& p_gain) = 0;
    
    /**
     * @brief 搴旂敤澶氫釜澧炵泭锛堟瘡涓€氶亾涓嶅悓锛?     * @param p_gain 姣忎釜閫氶亾鐨勫鐩婃暟缁?     */
    virtual void scale(const audio_sample* p_gain) = 0;
    
    /**
     * @brief 璁＄畻闊抽鏁版嵁鐨勫嘲鍊肩數骞?     * @param p_peak 杈撳嚭宄板€兼暟缁勶紙澶у皬 = 閫氶亾鏁帮級
     */
    virtual void calculate_peak(audio_sample* p_peak) const = 0;
    
    /**
     * @brief 鍒犻櫎鎸囧畾閫氶亾
     * @param p_channel 瑕佸垹闄ょ殑閫氶亾绱㈠紩
     */
    virtual void remove_channel(uint32_t p_channel) = 0;
    
    /**
     * @brief 澶嶅埗鐗瑰畾閫氶亾鍒版柊缂撳啿鍖?     * @param p_channel 婧愰€氶亾绱㈠紩
     * @param p_target 鐩爣 audio_chunk
     */
    virtual void copy_channel_to(uint32_t p_channel, audio_chunk_interface& p_target) const = 0;
    
    /**
     * @brief 浠庡彟涓€涓?chunk 澶嶅埗鐗瑰畾閫氶亾
     * @param p_channel 婧愰€氶亾绱㈠紩
     * @param p_source 婧?audio_chunk
     */
    virtual void copy_channel_from(uint32_t p_channel, const audio_chunk_interface& p_source) = 0;
    
    /**
     * @brief 浠庡彟涓€涓?chunk 澶嶅埗鎵€鏈夐€氶亾
     * @param p_source 婧?audio_chunk
     */
    virtual void duplicate(const audio_chunk_interface& p_source) = 0;
    
    /**
     * @brief 浠庡彟涓€涓?chunk 娣烽煶
     * @param p_source 瑕佹贩鍏ョ殑婧?     * @param p_count 瑕佹贩闊崇殑鏍锋湰鏁?     */
    virtual void combine(const audio_chunk_interface& p_source, size_t p_count) = 0;
    
    /**
     * @brief 娣辨嫹璐濆彟涓€涓?chunk 鐨勬墍鏈夋暟鎹?     * @param p_source 婧?audio_chunk
     */
    virtual void copy(const audio_chunk_interface& p_source) = 0;
    
    /**
     * @brief 浠庡彟涓€涓?chunk 澶嶅埗鍏冩暟鎹紙鏍煎紡銆佺姸鎬侊級
     * @param p_source 婧?audio_chunk
     */
    virtual void copy_meta(const audio_chunk_interface& p_source) = 0;
    
    /**
     * @brief 閲嶇疆鎵€鏈夋暟鎹紙娓呯┖缂撳啿鍖猴級
     */
    virtual void reset() = 0;
    
    /**
     * @brief 鍙嶅悜閬嶅巻鏌ユ壘鏍锋湰
     * @param p_start 璧峰浣嶇疆
     * @return 鏈夋晥鐨勫抚浣嶇疆鎴?0
     */
    virtual uint32_t find_peaks(uint32_t p_start) const = 0;
    
    /**
     * @brief 浜ゆ崲涓や釜 chunk 鐨勫唴瀹?     * @param p_other 瑕佷氦鎹㈢殑 chunk
     */
    virtual void swap(audio_chunk_interface& p_other) = 0;
    
    /**
     * @brief 娴嬭瘯涓や釜 chunk 鏄惁鏁版嵁鐩哥瓑
     * @param p_other 瑕佹瘮杈冪殑 chunk
     * @return true 濡傛灉鏁版嵁鐩哥瓑
     */
    virtual bool audio_data_equals(const audio_chunk_interface& p_other) const = 0;
};

} // namespace foobar2000_sdk
