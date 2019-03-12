/*
 * external/libfmrec_1388/fm_wav.h
 * (C) Copyright 2014-2018
 * Fortemedia, Inc. <www.fortemedia.com>
 * Author: LiFu <fuli@fortemedia.com>
 *
 * This program is dynamic library which privode interface to 
 * convert pcm data file to wav file .
 */
#ifndef __FM_WAV_H__
#define __FM_WAV_H__

#define ID_RIFF 0x46464952
#define ID_WAVE 0x45564157
#define ID_FMT  0x20746d66
#define ID_DATA 0x61746164

#define FORMAT_PCM 1

typedef unsigned int    uint32_t;
typedef unsigned short  uint16_t;

typedef struct wav_header_t {
    uint32_t riff_id;
    uint32_t riff_sz;
    uint32_t riff_fmt;
    uint32_t fmt_id;
    uint32_t fmt_sz;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    uint32_t data_id;
    uint32_t data_sz;
} wav_header;

#endif // __FM_WAV_H__
