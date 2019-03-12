/*
 * external/libfmrec_1388/fm_wav.c
 * (C) Copyright 2014-2018
 * Fortemedia, Inc. <www.fortemedia.com>
 * Author: LiFu <fuli@fortemedia.com>
 *
 * This program is dynamic library which privode interface to 
 * convert pcm data file to wav file .
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h> 
#include <sys/stat.h>
#include <signal.h>
#include "fm_wav.h"
#include <unistd.h>
//struct wav_header header;

int fm_wav_config_header(int fd,
						 unsigned int sample_rate,
						 unsigned int bits_per_sample,
						 unsigned int channels,
						 wav_header *header)
{
    header->riff_id 		= ID_RIFF;
    header->riff_sz 		= 0;
    header->riff_fmt 		= ID_WAVE;
    header->fmt_id 			= ID_FMT;
    header->fmt_sz 			= 16;
    header->audio_format 	= FORMAT_PCM;
    header->num_channels 	= channels;
    header->sample_rate 	= sample_rate;
    header->bits_per_sample = bits_per_sample;
    header->byte_rate 		= (header->bits_per_sample / 8) * channels * sample_rate;
    header->block_align 	= channels * (header->bits_per_sample / 8);
    header->data_id 		= ID_DATA;
    
	write(fd, header, sizeof(wav_header));

    return 0;
}

unsigned int fm_wav_write_data(int fd, char *buffer, int size)
{
	if (write(fd, buffer, size) != size) {
		fprintf(stderr,"%s: Error writing sample\n", __func__);
		return -1;
	}

    return 0;
}

int fm_wav_write_header(int fd, int read_total, int extraChunkSize, wav_header header)
{
	header.data_sz = read_total;
    header.riff_sz = header.data_sz + sizeof(header) - 8 + extraChunkSize;

    lseek(fd, 0, SEEK_SET);
    write(fd, &header, sizeof(wav_header));

    return 0;
}

