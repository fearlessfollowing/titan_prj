/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2/Titan Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: AudioManager.cpp
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

#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/statfs.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <mutex>
#include <vector>
#include <util/msg_util.h>
#include <sys/ins_types.h>
#include <util/util.h>
#include <prop_cfg.h>
#include <system_properties.h>
#include <log/log_wrapper.h>
#include <sys/AudioManager.h>


#undef  TAG
#define TAG "AudioManager"

#define DEFAULT_AUDIO_RES_PATH  "/home/nvidia/insta360/wav/"
#define PROP_AUDIO_RES_PATH     "sys.audio_path"


typedef struct stMapItem {

    std::string             fileName;       /* wav对应的文件名 */
    int                     iFd;
    void*                   mapPtr;         /* mmap得到的指针 */
    int                     iMapLen;

    stMapItem(const char* fName, int fd, void* map_data, int iLen) {
        fileName = fName;
        iFd = fd;
        mapPtr = map_data;
        iMapLen = iLen;
    }

    ~stMapItem() {
        munmap(mapPtr, iMapLen);
        if (iFd) close(iFd);
    }

} AudioMapItem;


static std::mutex   gInstanceLock;
AudioManager* AudioManager::sInstance = NULL;

AudioManager* AudioManager::Instance()
{
	std::unique_lock<std::mutex> lock(gInstanceLock);    
    if (!sInstance)
        sInstance = new AudioManager();
    return sInstance;

}

AudioManager::AudioManager()
{
    init();
}



AudioManager::~AudioManager()
{
    deinit();    
}


bool AudioManager::playWav(std::string fileName, std::string playDev)
{
    bool bResult = false;
    bool bFound = false;
    int ret;
    snd_pcm_t* handle;
    snd_pcm_hw_params_t* params;
    snd_pcm_uframes_t periodsize;
    snd_pcm_uframes_t frames;
    HWParams hw_params;
    int iSizeUnit;
    unsigned int iPeroid;

    LOGDBG(TAG, "---> play device[%s], file[%s]", playDev.c_str(), fileName.c_str());

    std::shared_ptr<AudioMapItem> tmpPtr = nullptr;
    for (u32 i = 0; i < audioRes.size(); i++) {
        tmpPtr = audioRes.at(i);
        if (tmpPtr && tmpPtr->fileName.find(fileName)) {
            bFound = true;
            break;
        }
    }    

    if (bFound) {
        // 1. 打开alsa
        if ((ret = snd_pcm_open(&handle, playDev.c_str(), SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
            LOGERR(TAG, "open pcm device error: %s", snd_strerror(ret));
            goto err_out;
        }

        // 2. 给参数分配空间,并用hw_param(从wav头中分析出的参数)初始化
        if (snd_pcm_hw_params_malloc(&params) < 0) {
            LOGERR(TAG, "snd_pcm_hw_params_malloc err");
            goto err_alloc;
        }

        if (snd_pcm_hw_params_any(handle, params) < 0) {
            LOGERR(TAG, "snd_pcm_hw_params_any err");
            goto err_alloc;
        }

        snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
        
        /* 设置音频数据格式 */
        snd_pcm_hw_params_set_format(handle, params, hw_params.format); 
        
        /* 设置采样通道 */
        snd_pcm_hw_params_set_channels(handle, params, hw_params.channels); //last param get from wav file
        
        /* 设置采样率 */
        snd_pcm_hw_params_set_rate_near(handle, params, &hw_params.rate, 0);

        LOGDBG(TAG, "hw_params: format=%d, channels=%d, rate = %d", hw_params.format, hw_params.channels, hw_params.rate);

        // 3. set param to driver
        if (snd_pcm_hw_params(handle, params) < 0) {
            LOGERR(TAG, "snd_pcm_hw_params error");
            goto err_alloc;
        }        

        snd_pcm_hw_params_get_period_size(params, &frames, 0);
        iSizeUnit = frames* 4;
        snd_pcm_hw_params_get_period_time(params, &iPeroid, 0); 
        LOGDBG(TAG, "period time = %d", iPeroid);

        unsigned char* pStart = static_cast<unsigned char*>(tmpPtr->mapPtr);
        unsigned char* pEnd = static_cast<unsigned char*>(tmpPtr->mapPtr) + tmpPtr->iMapLen;
        
        while (pStart < pEnd) {
            snd_pcm_writei(handle, pStart, frames);
            pStart += iSizeUnit;
        }

    } else {
        LOGERR(TAG, "play wav[%s] not cached!!", fileName.c_str());
    }

    return true;


err_alloc:
    snd_pcm_drain(handle);
    snd_pcm_close(handle);

err_out:
    return bResult;
}


#ifdef ENABLE_CACHE_AUDIO_FILE

/* 加载所有的wav素材到Cache中 */
int AudioManager::loadRes2Cache(const char* fileName)
{
    int iRet = -1;
    
    /* mmap */
    if (access(fileName, F_OK) == 0) {
        int fd = open(fileName, O_RDONLY);
        if (fd > 0) {
            struct stat stat;
            fstat(fd, &stat);
            void* map_data = mmap(NULL, stat.st_size, PROT_READ, MAP_SHARED, fd, 0);
            if (map_data != MAP_FAILED) {
                std::shared_ptr<AudioMapItem> audioPtr = std::make_shared<AudioMapItem>(fileName, fd, map_data, stat.st_size);
                audioRes.push_back(audioPtr);
                iRet = 0;
            }
        }
    }
    return iRet;
}
#endif


void AudioManager::init()
{
    /* 选择播放设备 */
    defaultPlayDev = DEFAULT_PALY_DEVICE;

    /* 预加载wav文件(mmap) */
#ifdef ENABLE_CACHE_AUDIO_FILE

    const char* pAudioResPath = NULL;
    std::string audioPath;
    DIR *dir;
    struct dirent *de;
    char devname[512] = {0};
    char *filename;

    pAudioResPath = property_get(PROP_AUDIO_RES_PATH);
    if (NULL == pAudioResPath) {
        audioPath = DEFAULT_AUDIO_RES_PATH;
    } else {
        audioPath = pAudioResPath;
    }
    LOGDBG(TAG, "audio res path: [%s]", audioPath);
    
    dir = opendir(audioPath.c_str());
    if (dir == NULL) {
        LOGERR(TAG, "---> open dir[%s] error", audioPath.c_str());
        return;
    }

    strcpy(devname, audioPath.c_str());
    filename = devname + strlen(audioPath.c_str());
    *filename++ = '/';

    while ((de = readdir(dir))) {
        if (de->d_name[0] == '.' && (de->d_name[1] == '\0' || (de->d_name[1] == '.' && de->d_name[2] == '\0')))
            continue;

        strcpy(filename, de->d_name);
        loadRes2Cache(filename);        /* Cache音频资源 */
    }
    closedir(dir);
#endif
}

void AudioManager::deinit()
{

}