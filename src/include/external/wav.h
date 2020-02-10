/*
 * file:		include/external/wav.h
 * auther:		Jason Hu
 * time:		2020/1/26
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _EXTERNAL_WAV_H
#define _EXTERNAL_WAV_H

#include <lib/stdint.h>
#include <lib/stddef.h>

typedef struct WavRIFF {
    /* chunk "riff" */
    char chunkID[4];   /* "RIFF" */
    /* sub-chunk-size */
    uint32_t chunkSize; /* 36 + Subchunk2Size */
    /* sub-chunk-data */
    char format[4];    /* "WAVE" */
} WavRIFF_t;

typedef struct WavFMT {
    /* sub-chunk "fmt" */
    char subchunk1ID[4];   /* "fmt " */
    /* sub-chunk-size */
    uint32_t subchunk1Size; /* 16 for PCM */
    /* sub-chunk-data */
    uint16_t audioFormat;   /* PCM = 1*/
    uint16_t numChannels;   /* Mono = 1, Stereo = 2, etc. */
    uint32_t sampleRate;    /* 8000, 44100, etc. */
    uint32_t byteRate;  /* = SampleRate * NumChannels * BitsPerSample/8 */
    uint16_t blockAlign;    /* = NumChannels * BitsPerSample/8 */
    uint16_t bitsPerSample; /* 8bits, 16bits, etc. */
} WavFMT_t;
/*
typedef struct WavDataBlock {
    char data[0];
} DataBlock_t;
*/
typedef struct WavData {
    /* sub-chunk "data" */
    char subchunk2ID[4];   /* "data" */
    /* sub-chunk-size */
    uint32_t subchunk2Size; /* data size */
    /* sub-chunk-data */
    //DataBlock_t block;
} WavData_t;

typedef struct WavFotmat {
    WavRIFF_t riff;
    WavFMT_t fmt;
    WavData_t data;
} Wav_t;


#endif  /* _EXTERNAL_WAV_H */
