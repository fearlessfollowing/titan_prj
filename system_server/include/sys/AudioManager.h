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
#include <vector>


/** 
 * WAVE文件格式分析
 * RIFF块           - 8字节（RIFF + Length）
 * 文件格式类型“WAVE” - 4字节
 * fmt块
 * fact块（可选，压缩编码格式要含有该块）
 * 数据块
 */

//需要从wav文件中读取的三个参数
typedef struct {
    snd_pcm_format_t    format;
    unsigned int        channels;
    unsigned int        rate;
} HWParams; 

/*
 * 包括RIFF块和WAVE标识
 */
typedef struct {
    unsigned int    magic;          /* Offset: 00H 大写字符串"RIFF",标明该文件为有效的 RIFF 格式文档 */
    unsigned int    length;         /* Offset: 04H 从下一个字段首地址开始到文件末尾的总字节数。该字段的数值加 8 为当前文件的实际长度 */
    unsigned int    type;           /* Offset: 08H 所有 WAV 格式的文件此处为字符串"WAVE",标明该文件是 WAV 格式文件 */
} WaveHeader;



typedef struct {
    unsigned short  format;         /* Offset: 14H 编码格式代码，常见的 WAV 文件使用 PCM 脉冲编码调制格式,该数值通常为 1 */
    unsigned short  channels;       /* Offset: 16H 单声道为 1,立体声或双声道为 2 */
    unsigned int    sample_fq;      /* Offset: 18H 每个声道单位时间采样次数。常用的采样频率有 11025, 22050 和 44100 kHz */
    unsigned int    byte_p_sec;     /* Offset: 1CH 该数值为:声道数×采样频率×每样本的数据位数/8。播放软件利用此值可以估计缓冲区的大小 */
    unsigned short  byte_p_spl;     /* Offset: 20H 采样帧大小。该数值为:声道数×位数/8。播放软件需要一次处理多个该值大小的字节数据,用该数值调整缓冲区 */
    unsigned short  bit_p_spl;      /* Offset: 22H 存储每个采样值所用的二进制数位数。常见的位数有 4、8、12、16、24、32  */
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

// #define ENABLE_CACHE_AUDIO_FILE

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

    void        initSpeaker();      /* 初始化扬声器 */
    void        initRecorder();     /* 录音设备初始化 */

    void        deinit();

#ifdef ENABLE_CACHE_AUDIO_FILE
    
    /* 加载所有的wav素材到Cache中 */
    int         loadRes2Cache(const char* fileName);

    std::vector<std::shared_ptr<struct stMapItem>> audioRes;
#endif

    int         checkWavFile(const char* pBuf, HWParams* hw_params);

    std::string             defaultPlayDev;
    std::string             mAudioResPath;
    static AudioManager*    sInstance;
    bool                    mInited = false;
    snd_pcm_t*              mHandle;
    snd_pcm_uframes_t       periodsize;
    snd_pcm_uframes_t       mFrames;
    int                     mSizeUnit;

};


#endif  /* _AUDIO_MANAGER_H_ */