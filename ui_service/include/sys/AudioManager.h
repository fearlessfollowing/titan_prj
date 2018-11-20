/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2/Titan Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: AudioManager.h
** 功能描述: 音频服务(负责Speaker的管理及提示文件播放)
**
**
**
** 作     者: Skymixos
** 版     本: V1.0
** 日     期: 2018年11月19日
** 修改记录:
** V1.0			Skymixos			2018-11-19		创建文件
******************************************************************************************************/
#ifndef _AUDIO_MANAGER_H_
#define _AUDIO_MANAGER_H_

#include <alsa/asoundlib.h>
#include <memory>
#include <string>


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



struct stMapItem;

#define DEFAULT_PALY_DEVICE     "speaker"

#define ENABLE_CACHE_AUDIO_FILE

/*
 * 1.构造时打开Speaker设备
 * 2.提供播放指定文件的接口
 */
class AudioManager {
public:
            ~AudioManager();

    static AudioManager*	Instance();

    bool    playWav(std::string fileName, std::string palyDev = DEFAULT_PALY_DEVICE);


private:
                AudioManager();
    void        init();
    void        deinit();

#ifdef ENABLE_CACHE_AUDIO_FILE
    
    /* 加载所有的wav素材到Cache中 */
    int         loadRes2Cache(const char* fileName);

    std::vector<std::shared_ptr<struct stMapItem>> audioRes;
#endif

    std::string defaultPlayDev;

    static AudioManager* sInstance;
};


#endif  /* _AUDIO_MANAGER_H_ */