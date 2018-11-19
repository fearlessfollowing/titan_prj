//#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>
#include <stdlib.h>
#include <stdio.h>


//需要从wav文件中读取的三个参数
typedef struct {
    snd_pcm_format_t    format;
    unsigned int        channels;
    unsigned int        rate;
} HWParams; 

//下面这四个结构体是为了分析wav头的
typedef struct {
    unsigned int    magic;          /* Offset: 00H 大写字符串"RIFF",标明该文件为有效的 RIFF 格式文档 */
    unsigned int    length;         /* Offset: 04H 从下一个字段首地址开始到文件末尾的总字节数。该字段的数值加 8 为当前文件的实际长度 */
    unsigned int    type;           /* Offset: 08H 所有 WAV 格式的文件此处为字符串"WAVE",标明该文件是 WAV 格式文件 */
} WaveHeader;

typedef struct {
    unsigned short  format;         /* Offset: 0CH 小写字符串,"fmt " see WAV_FMT_* */
    unsigned short  channels;
    unsigned int    sample_fq;     /* frequence of sample */
    unsigned int    byte_p_sec;
    unsigned short  byte_p_spl;  /* samplesize; 1 or 2 bytes */
    unsigned short  bit_p_spl;   /* 8, 12 or 16 bit */
} WaveFmtBody;


typedef struct {
    WaveFmtBody     format;
    unsigned short  ext_size;
    unsigned short  bit_p_spl;
    unsigned int    channel_mask;
    unsigned short  guid_format; /* WAV_FMT_* */
    unsigned char   guid_tag[14]; /* WAV_GUID_TAG */
} WaveFmtExtensibleBody;


typedef struct {
    unsigned int    type;       /* 'data' */
    unsigned int    length;     /* samplecount */
} WaveChunkHeader;

#define COMPOSE_ID(a,b,c,d) ((a) | ((b)<<8) | ((c)<<16) | ((d)<<24))
#define WAV_RIFF COMPOSE_ID('R','I','F','F')
#define WAV_WAVE COMPOSE_ID('W','A','V','E')
#define WAV_FMT COMPOSE_ID('f','m','t',' ')
#define WAV_DATA COMPOSE_ID('d','a','t','a')


int check_wavfile(int fd, HWParams* hw_params)
{
    int ret;
    int i, len;
    WaveHeader* header;
    WaveFmtBody* fmt;
    WaveChunkHeader* chunk_header;
    unsigned char* pbuf = (unsigned char*)malloc(128); 
    if (NULL == pbuf) {
        printf("pbuf malloc error");
        return -1;
    }

    //1. check Wave Header
    len = sizeof(WaveHeader);
    if ((ret = read(fd, pbuf, len)) != len) {
        printf("read error");
        return -1;
    }

    header = (WaveHeader*)pbuf;
    if ((header->magic != WAV_RIFF) || (header->type != WAV_WAVE)) {
        printf("not a wav file");
        return -1;
    }

    //2. check Wave Fmt
    len = sizeof(WaveChunkHeader)+sizeof(WaveFmtBody);
    if ((ret=read(fd, pbuf, len)) != len) {
        printf("read error");
        return -1;
    }

    chunk_header = (WaveChunkHeader*)pbuf;
    if (chunk_header->type!=WAV_FMT) {
        printf("fmt body error");
        return -1;
    }

    fmt = (WaveFmtBody*)(pbuf+sizeof(WaveChunkHeader));
    if (fmt->format != 0x0001) {   //WAV_FMT_PCM
        printf("format is not pcm");
        return -1;
    }

    printf("format=0x%x, channels=0x%x,sample_fq=%d,byte_p_sec=%d,byte_p_sample=%d,bit_p_sample=%d",
            fmt->format, fmt->channels,fmt->sample_fq, fmt->byte_p_sec,
            fmt->byte_p_spl, fmt->bit_p_spl);
    
    //copy params
    hw_params->channels = fmt->channels;
    hw_params->rate = fmt->sample_fq;
    
    switch (fmt->bit_p_spl) {
        case 8:
            hw_params->format = SND_PCM_FORMAT_U8;
            break;
        case 16:
            hw_params->format = SND_PCM_FORMAT_S16_LE;
            break;
        default:
            printf("FIXME: add more format");
            break;
    }
    
    //3. check data chunk
    len = sizeof(WaveChunkHeader);
    if( (ret=read(fd, pbuf, len)) != len) {
        printf("read error");
        return -1;
    }

    chunk_header = (WaveChunkHeader*)pbuf;
    if (chunk_header->type != WAV_DATA) {
        printf("not data chunk");
        return -1;
    }

    printf("pcm_data_size=0x%x",chunk_header->length);

    free(pbuf);
    pbuf = NULL;
    return -1;
}


int main(int argc, char *argv[])
{
    int i, fd;
    int ret, dir, size;
    unsigned int val, val2;
    char* buffer;
    snd_pcm_t* handle;
    snd_pcm_hw_params_t* params;
    snd_pcm_uframes_t periodsize;
    snd_pcm_uframes_t frames;
    HWParams hw_params;

    if (argc < 2) {
        printf("usage ./play ");
        return -1;
    }

    fd = open(argv[1], O_RDWR);
    if (fd < 0) {
        printf("file open error");
        return -1;
    }

    check_wavfile(fd, &hw_params);   //从wav头中分析出的参数，保存在hw_param中

    //1. 打开alsa
    if ( (ret = snd_pcm_open(&handle, "hw:1,0", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        printf("open pcm device error:%s", snd_strerror(ret));
        return -1;
    }

    //2. 给参数分配空间,并用hw_param(从wav头中分析出的参数)初始化
    snd_pcm_hw_params_alloca(&params);
    
    snd_pcm_hw_params_any(handle, params);

    snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);

    snd_pcm_hw_params_set_format(handle, params, hw_params.format); 
    snd_pcm_hw_params_set_channels(handle, params, hw_params.channels); //last param get from wav file
    
    printf("hw_params: format=%d, channels=%d", hw_params.format, hw_params.channels);

    val = 44100; 
    snd_pcm_hw_params_set_rate_near(handle,params, &val, &dir);

    frames = 32*4; 
    snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);
    
    //3. set param to driver
    if ((ret = snd_pcm_hw_params(handle, params)) < 0) {
        printf("set hw params error:%s", snd_strerror(ret));
        return -1;
    }

    snd_pcm_hw_params_get_period_size(params, &frames, &dir);
    size = frames*4; //2byte/smaple, 2 channels
    buffer = (char*)malloc(size);

    snd_pcm_hw_params_get_period_time(params, &val, &dir);
    
    while (1) {
        ret = read(fd, buffer, size);                //3.从wav文件中读取数据
        if (ret == 0) {
            printf("end of file");
            return 0;
        } else if (ret != size) {
            printf("short read");
        }

        ret = snd_pcm_writei(handle, buffer, frames);   //4.将读取数据写到driver中进行播放
        if (ret == -EPIPE) {
            //printf("-EPIPE");
            snd_pcm_prepare(handle);
        }
    }

    snd_pcm_drain(handle);
    snd_pcm_close(handle);
    free(buffer);

    return EXIT_SUCCESS;
}
