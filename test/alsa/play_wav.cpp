#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <alsa/asoundlib.h>
#ifndef WORD
#define WORD unsigned short
#endif

#ifndef DWORD
#define DWORD unsigned int
#endif
    
struct RIFF_HEADER
{
    char szRiffID[4];  // 'R','I','F','F'
    DWORD dwRiffSize;
    char szRiffFormat[4]; // 'W','A','V','E'
};

struct WAVE_FORMAT
{
    WORD wFormatTag;
    WORD wChannels;
    DWORD dwSamplesPerSec;
    DWORD dwAvgBytesPerSec;
    WORD wBlockAlign;
    WORD wBitsPerSample;
};

struct FMT_BLOCK
{
    char  szFmtID[4]; // 'f','m','t',' '
    DWORD  dwFmtSize;
    struct WAVE_FORMAT wavFormat;
};

struct DATA_BLOCK
{
    char szDataID[4]; // 'd','a','t','a'
    DWORD dwDataSize;
};

void read_wav(unsigned char *wav_buf, int *fs, int *channels, int *bits_per_sample, int *wav_size, int *file_size)
{
    struct RIFF_HEADER *headblk;
    struct FMT_BLOCK   *fmtblk;
    struct DATA_BLOCK  *datblk;

    headblk = (struct RIFF_HEADER *) wav_buf;
    fmtblk  = (struct FMT_BLOCK *) &headblk[1];
    datblk  = (struct DATA_BLOCK *) &fmtblk[1];
    
    *file_size = headblk->dwRiffSize;

    //采样频率
    *fs         = fmtblk->wavFormat.dwSamplesPerSec;

    //通道数
    *channels  = fmtblk->wavFormat.wChannels;
    
    *wav_size  = datblk->dwDataSize;
    //采样bit数 16 24
    *bits_per_sample = fmtblk->wavFormat.wBitsPerSample;
}

int main(int argc, char ** argv)
{
    int fs, channels, bits_per_sample, wav_size, file_size;
    unsigned char *wav_buf;
    unsigned char *audio_buf;
    int fd;
    struct stat stat;
    
    int size;
    snd_pcm_t *playback_handle;
    snd_pcm_hw_params_t *hw_params;//硬件信息和PCM流配置
    
    snd_pcm_uframes_t chunk_size = 0;
    unsigned int rate;
    snd_pcm_format_t format;

    
    fd = open(argv[1], O_RDONLY);
    fstat(fd, &stat);
    wav_buf = (unsigned char*)mmap(NULL, stat.st_size, PROT_READ, MAP_SHARED, fd, 0);
    size = stat.st_size;

    read_wav(wav_buf, &fs, &channels, &bits_per_sample, &wav_size, &file_size);
    printf("wav format: fs = %d, channels = %d, bits_per_sample = %d, wav_size = %d file_size = %d\n\r", fs, channels, bits_per_sample, wav_size, file_size);
    
    //真实wav 跳过头部
    audio_buf = wav_buf + sizeof(struct RIFF_HEADER) + sizeof(struct FMT_BLOCK) + sizeof(struct DATA_BLOCK);
    
    rate = fs;
    //初始化声卡
    //1. 打开PCM，最后一个参数为0意味着标准配置
    if (0 > snd_pcm_open(&playback_handle, "default", SND_PCM_STREAM_PLAYBACK, 0)) 
    {
        printf("snd_pcm_open err\n");
        return -1;
    }
    //2. 分配snd_pcm_hw_params_t结构体
    if(0 > snd_pcm_hw_params_malloc (&hw_params))
    {
        printf("snd_pcm_hw_params_malloc err\n");
        return -1;
    }
    //3. 初始化hw_params
    if(0 > snd_pcm_hw_params_any (playback_handle, hw_params))
    {
        printf("snd_pcm_hw_params_any err\n");
        return -1;
    }
    //4. 初始化访问权限
    if (0 > snd_pcm_hw_params_set_access (playback_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) 
    {
        printf("snd_pcm_hw_params_any err\n");
        return -1;
    }
    //5. 初始化采样格式SND_PCM_FORMAT_U8,8位
    if(8 == bits_per_sample)
    {
        if (0 > snd_pcm_hw_params_set_format (playback_handle, hw_params, SND_PCM_FORMAT_U8)) 
        {
            printf("snd_pcm_hw_params_set_format err\n");
            return -1;
        }
        format = SND_PCM_FORMAT_U8;
    }
    
    if(16 == bits_per_sample)
    {
        if (0 > snd_pcm_hw_params_set_format (playback_handle, hw_params, SND_PCM_FORMAT_S16_LE)) 
        {
            printf("snd_pcm_hw_params_set_format err\n");
            return -1;
        }
        format = SND_PCM_FORMAT_S16_LE;
    }
    //6. 设置采样率
    if (0 > snd_pcm_hw_params_set_rate_near (playback_handle, hw_params, &rate, 0)) 
    {
        printf("snd_pcm_hw_params_set_rate_near err\n");
        return -1;
    }
    //7. 设置通道数量
    if (0 > snd_pcm_hw_params_set_channels(playback_handle, hw_params, 2)) 
    {
        printf("snd_pcm_hw_params_set_channels err\n");
        return -1;
    }
    
    //8. 设置hw_params    
    if (0 > snd_pcm_hw_params (playback_handle, hw_params)) 
    {
        printf("snd_pcm_hw_params err\n");
        return -1;
    }

    snd_pcm_hw_params_get_period_size(hw_params, &chunk_size, 0);
    
    snd_pcm_hw_params_free (hw_params);
    if (0 > snd_pcm_prepare (playback_handle)) 
    {
        printf("snd_pcm_prepare err\n");
        return -1;
    }
    //chunk_size = 4096;
    printf("chunk_size:%d \n", chunk_size);
    while (size > 0)
    {
        snd_pcm_writei(playback_handle, audio_buf, chunk_size);
        audio_buf += chunk_size * 4; //16位 双声道
        size -= chunk_size * 4;
    }

    printf("play ok \n");
    snd_pcm_close(playback_handle);
    return 0;
}