/*
 * minimp3 - mini MP3 decoder
 * Copyright (c) 2017-2023, Alex Lohman
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * For more information, please visit:
 * https://github.com/lieff/minimp3
 */

#ifndef MINIMP3_H
#define MINIMP3_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Version info */
#define MINIMP3_VERSION "0.6.36"

/* Compile-time configuration */
#ifndef MINIMP3_MAX_SAMPLES_PER_FRAME
#define MINIMP3_MAX_SAMPLES_PER_FRAME 1152
#endif

#ifndef MINIMP3_MAX_FRAME_SYNC_MATCHES
#define MINIMP3_MAX_FRAME_SYNC_MATCHES 10
#endif

/* Flags */
#define MINIMP3_FLAG_ID3V2_LIB  0x00000001
#define MINIMP3_FLAG_ALLOW_MONO_STEREO_TRANSITION 0x00000002
#define MINIMP3_FLAG_WAVEFORMATEX 0x00000004

/* Error codes */
#define MP3D_E_NONE  0
#define MP3D_E_MEMORY 1
#define MP3D_E_PARAM 2
#define MP3D_E_DECODE 3

/* Sample formats */
enum {
    MP3D_SAMPLE_INT16 = 0,
    MP3D_SAMPLE_INT32,
    MP3D_SAMPLE_FLOAT,
    MP3D_SAMPLE_COUNT
};

/* Structures */
typedef struct {
    const uint8_t *buffer;
    size_t size;
    size_t file_offset;
    void *file;
    void *read_user_data;
    size_t (*read)(void *buf, size_t size, void *user_data);
} mp3dec_io_t;

typedef struct {
    float *samples;
    size_t channels;
    size_t hz;
    int layer;
    int bitrate_kbps;
} mp3d_frame_t;

typedef struct {
    size_t *samples;
    size_t sample_rate;
    size_t channels;
    size_t total_samples;
    float *buffer;
    size_t buffer_samples;
    int is_float;
    void *io;
    mp3dec_io_t *io_data;
} mp3dec_file_info_t;

typedef struct {
    uint32_t channels;
    uint32_t hz;
    int layer;
    int avg_bitrate_kbps;
} mp3dec_t;

typedef struct {
    void *is;
    uint64_t cur;
    uint64_t file_size;
    uint32_t flags;
    uint32_t padding[3];
    uint32_t frames;
    uint32_t delay;
    uint32_t layer;
    uint32_t channels;
    uint32_t hz;
    uint32_t bitrate_kbps;
    uint32_t bitrate_frame_bytes;
    uint64_t samples;
    uint32_t buffer_samples;
    uint32_t eof;
    uint32_t to_skip;
    uint32_t current_sample;
    uint8_t *buffer;
    uint8_t *file_buffer;
    uint8_t *file_buffer_size;
    uint8_t *file_buffer_pos;
    uint32_t free_format_bytes;
    uint32_t last_frame;
    uint32_t free_format_frame_bytes;
    uint8_t has_sync;
    uint8_t md5[16];
    void *tag;
    int vbr_tag_found;
    int is_id3v1;
} mp3dec_ex_t;

/* Functions */
int mp3dec_load(mp3dec_t *dec, const char *file_name, uint8_t *buf, uint32_t *buf_size, mp3dec_file_info_t *info);
int mp3dec_load_buf(mp3dec_t *dec, const uint8_t *buf, size_t buf_size, mp3dec_file_info_t *info);
int mp3dec_load_info(mp3dec_t *dec, mp3dec_file_info_t *info, const char *file_name, uint8_t *buf, uint32_t *buf_size);
int mp3dec_load_cb(mp3dec_t *dec, mp3dec_io_t *io, uint8_t *buf, uint32_t *buf_size, mp3dec_file_info_t *info);

int mp3dec_init(mp3dec_t *dec);
int mp3dec_decode_frame(mp3dec_t *dec, const uint8_t *buf, int buf_size, short *pcm, mp3d_frame_t *info);
int mp3dec_decode_frame32(mp3dec_t *dec, const uint8_t *buf, int buf_size, int *pcm, mp3d_frame_t *info);
int mp3dec_decode_frame_float(mp3dec_t *dec, const uint8_t *buf, int buf_size, float *pcm, mp3d_frame_t *info);


int mp3dec_ex_open_buf(mp3dec_ex_t *dec, const uint8_t *buf, size_t buf_size, int flags);
int mp3dec_ex_open_cb(mp3dec_ex_t *dec, mp3dec_io_t *io, int flags);
void mp3dec_ex_close(mp3dec_ex_t *dec);
int mp3dec_ex_seek(mp3dec_ex_t *dec, uint64_t position);
int mp3dec_ex_read_frame(mp3dec_ex_t *dec, float *buf, mp3d_frame_t *frame_info, int max_samples);
int mp3dec_ex_read(mp3dec_ex_t *dec, float *buf, int buf_samples);

int mp3dec_detect_buf(const uint8_t *buf, size_t buf_size);
int mp3dec_detect_cb(mp3dec_io_t *io);
int mp3dec_scan_file(const char *file_name, uint32_t *max_frames, int *sample_rate, int *channels, int *layer, int *avg_bitrate_kbps);
int mp3dec_scan_buf(const uint8_t *buf, size_t buf_size, uint32_t *max_frames, int *sample_rate, int *channels, int *layer, int *avg_bitrate_kbps);
int mp3dec_scan_cb(mp3dec_io_t *io, uint32_t *max_frames, int *sample_rate, int *channels, int *layer, int *avg_bitrate_kbps);

int mp3dec_skip_id3v1(const uint8_t *buf, size_t buf_size);
int mp3dec_skip_id3v2(const uint8_t *buf, size_t buf_size, size_t *tag_size, int *has_id3v1);
int mp3dec_get_aac_buffer(const uint8_t *buf, size_t buf_size);
int mp3dec_get_mp3_buffer_size(const uint8_t *buf, size_t buf_size);

#define MP3D_SEEK_TO_SAMPLE 0x00000001

#define MP3D_E_FILE (-1)
#define MP3D_E_BUF太小 (-2)
#define MP3D_E_INVALID (-3)

#ifdef __cplusplus
}
#endif

#endif /* MINIMP3_H */