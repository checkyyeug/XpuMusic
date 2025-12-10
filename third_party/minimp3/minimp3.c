#define MINIMP3_IMPLEMENTATION
#define MINIMP3_FLOAT_OUTPUT
#include "minimp3.h"

// Stub implementations for minimp3 functions
int mp3dec_init(mp3dec_t *dec) {
    (void)dec;
    return 0;
}

int mp3dec_decode_frame(mp3dec_t *dec, const uint8_t *buf, int buf_size, short *pcm, mp3d_frame_t *info) {
    (void)dec; (void)buf; (void)buf_size; (void)pcm; (void)info;
    return 0;
}

int mp3dec_decode_frame_float(mp3dec_t *dec, const uint8_t *buf, int buf_size, float *pcm, mp3d_frame_t *info) {
    (void)dec; (void)buf; (void)buf_size; (void)pcm; (void)info;
    // In a real implementation, this would decode to float samples
    // For now, just return 0 (no samples decoded)
    return 0;
}

int mp3dec_load(mp3dec_t *dec, const char *file_name, uint8_t *buf, uint32_t *buf_size, mp3dec_file_info_t *info) {
    (void)dec; (void)file_name; (void)buf; (void)buf_size; (void)info;
    // Stub implementation - return error
    return -1;
}