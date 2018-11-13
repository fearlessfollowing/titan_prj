/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2 Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: fifo.cpp
** 功能描述: 输入管理器（用于处理按键事件）
**
**
**
** 作     者: Skymixos
** 版     本: V1.0
** 日     期: 2018年05月04日
** 修改记录:
** V1.0			Skymixos		2018-05-04		创建文件，添加注释
******************************************************************************************************/

#include <sys/stat.h>
#include <future>
#include <vector>
#include <common/include_common.h>
#include <trans/fifo.h>
#include <trans/fifo_event.h>
#include <util/ARHandler.h>
#include <util/ARMessage.h>
#include <common/common.h>
#include <util/bytes_int_convert.h>
#include <hw/MenuUI.h>

#include <hw/ins_gpio.h>

#include <system_properties.h>
#include <sys/VolumeManager.h>

#include <prop_cfg.h>
#include <sstream>
#include <json/value.h>
#include <json/json.h>

#include <log/log_wrapper.h>


using namespace std;

#if 0
static QR_STRUCT mQRInfo[] = {
	100, {{7,4,4,3,1,0}, {6,6,5,0,0,1}, {6,6,6}}
};
#endif

#undef  TAG 
#define TAG "fifo"

static sp<fifo> mpFIFO = nullptr;

#if 0
static const int qr_array_len = 4;

static const RES_INFO mResInfos[] = {
	//sti
	{7680, {7680, 3840}},//0
	{5760, {5760, 2880}},
	{4096, {4096, 2048}},
	{3840, {3840, 1920}},
	{2880, {2880, 1440}},
	{1920, {1920, 960}}, //5
	{1440, {1440, 720}},
	//org
	{
	 4000, {3000, 3000}
	},
	{
	 3840, {2160, 2160}
	},
	{
	 2560, {1440, 1440}
	},
	{
	 1920, {1080, 1080} //10
	},
	{
	 3200, {2400, 2400}
	},
	{
	 2160, {1620, 1620}
	},
	{
	 1920, {1440, 1440}
	},
	{1280,{960,960}},
	{2160,{1080,1080}} //15
};
#endif


static sp<fifo> gSysTranObj = NULL;
static std::mutex gSysTranMutex;
static Mutex gSendLock;


sp<fifo>& fifo::getSysTranObj()
{
    unique_lock<mutex> lock(gSysTranMutex);
    if (gSysTranObj != NULL) {
        return gSysTranObj;
    } else {
        gSysTranObj = sp<fifo> (new fifo());
        CHECK_NE(gSysTranObj, nullptr);
    }
    return gSysTranObj;
}


void init_fifo()
{
    fifo::getSysTranObj();
}


void fifo::sendUiMessage(sp<ARMessage>& msg)
{
    mOLEDHandle->postUiMessage(msg);
}


class my_handler : public ARHandler {
public:
    my_handler(fifo *source) : mHandler(source) {
    }

    virtual ~my_handler() override {
    }

    virtual void handleMessage(const sp<ARMessage> &msg) override {
        mHandler->handleMessage(msg);
    }

private:
    fifo *mHandler;
};


fifo::fifo()
{
    init();
}


fifo::~fifo()
{
    deinit();
}

void fifo::init()
{
    make_fifo();
 
    // init_thread();

    // //set in the end
    // notify = obtainMessage(MSG_UI_KEY);

    mOLEDHandle = (sp<MenuUI>)(new MenuUI()); //oled_handler::getSysUiObj(notify);

    #if 1
    th_read_fifo_ = thread([this] { read_fifo_thread(); });
    #endif

    LOGDBG(TAG, "fifo::init() ... OK");
}


void fifo::start_all()
{

}

void fifo::stop_all(bool delay)
{
    LOGDBG(TAG, "stop_all");

    if (!bReadThread) {
        bReadThread = true;
        if (th_read_fifo_.joinable()) {
            write_exit_for_read();
            th_read_fifo_.join();
        }
    }
    mOLEDHandle = nullptr;

    LOGDBG(TAG, "stop_all3");
}

sp<ARMessage> fifo::obtainMessage(uint32_t what)
{
    return mHandler->obtainMessage(what);
}

void fifo::sendExit()
{
    if (!bExit) {
        bExit = true;
        if (th_msg_.joinable()) {
            if (bWFifoStop) {
                // unblock fifo write open while fifo read not happen
                int fd = open(FIFO_TO_CLIENT, O_RDONLY);
                CHECK_NE(fd, -1);
                close(fd);
            }
            obtainMessage(MSG_EXIT)->post();
            th_msg_.join();
        } else {
            LOGERR(TAG, " th_msg_ not joinable ");
        }
    }
}


/*************************************************************************
** 方法名称: write_fifo
** 方法功能: 发送数据到给osc
** 入口参数: 
**		iEvent - 事件类型
**		str - 数据
** 返 回 值: 无 
** 调 用: 
**
*************************************************************************/
void fifo::write_fifo(int iEvent, const char *str)
{

    AutoMutex _l(gSendLock);

    char data[4096] = {0x00};
    int total = FIFO_HEAD_LEN;
    int write_len;
    int len = 0;

    get_write_fd();

    int_to_bytes(data, iEvent);

    if (str != nullptr) {
        len = strlen(str);
#if 1
        if (len > (int)((sizeof(data) - FIFO_HEAD_LEN))) {
            LOGERR(TAG, "fifo len exceed (%d %d)", len, (sizeof(data) - FIFO_HEAD_LEN));
            len = (sizeof(data) - FIFO_HEAD_LEN);
        }
		
        int_to_bytes(&data[FIFO_DATA_LEN_OFF], len);
        memcpy((void *)&data[FIFO_HEAD_LEN],str,len);
#else
        int_to_bytes(&data[FIFO_DATA_LEN_OFF], len);
        snprintf(&data[FIFO_HEAD_LEN], sizeof(data) - FIFO_HEAD_LEN, str, len);
#endif
        total += len;
    } else {
        int_to_bytes(&data[FIFO_DATA_LEN_OFF], len);
    }
	
    write_len = write(write_fd, data, total);
    if (write_len != total) {
        LOGERR(TAG, "write fifo len %d but total is %d\n", write_len, total);
        if (write_len == -1) {
            LOGERR(TAG, "write fifo broken");
            close_write_fd();
        }
    } 
}



void fifo::postTranMessage(sp<ARMessage>& msg)
{
    msg->setHandler(mHandler);
    msg->post();
}


/*************************************************************************
** 方法名称: handleMessage
** 方法功能: FIFO交互线程消息处理
** 入口参数: 
**      msg - 消息指针
** 返回值: 无
** 调用:
** 改动: 让消息循环只处理来自UI线程的消息 - 2018年9月5日
*************************************************************************/
void fifo::handleMessage(const sp<ARMessage> &msg)
{
    uint32_t what = msg->what();

	{
        if (MSG_EXIT == what) {		/* 线程退出消息 */
            mLooper->quit();
            close_write_fd();
        } 
    }
}


/*************************************************************************
** 方法名称: init_thread
** 方法功能: 初始化通信线程(FiFo)
** 入口参数: 
** 返 回 值: 无 
** 调 用: 
**
*************************************************************************/
void fifo::init_thread()
{
    std::promise<bool> pr;
    std::future<bool> reply = pr.get_future();
    th_msg_ = thread([this, &pr] {
                       mLooper = sp<ARLooper>(new ARLooper());
                       mHandler = sp<ARHandler>(new my_handler(this));
                       mHandler->registerTo(mLooper);
                       pr.set_value(true);
                       mLooper->run();
                   });
}



#define RECV_MSG(n) case n: return #n
const char *getRecvMsgName(int iMessage)
{
    switch (iMessage) {
        RECV_MSG(CMD_OLED_DISP_TYPE);
        RECV_MSG(CMD_OLED_SET_SN);
        RECV_MSG(CMD_OLED_SYNC_INIT);
        RECV_MSG(CMD_OLED_DISP_TYPE_ERR);
        RECV_MSG(CMD_CONFIG_WIFI);
        RECV_MSG(ACTION_CALIBRATION);
        RECV_MSG(CMD_WEB_UI_TF_CHANGED);
        RECV_MSG(CMD_WEB_UI_TF_NOTIFY);
        RECV_MSG(CMD_WEB_UI_TF_FORMAT);
        RECV_MSG(CMD_WEB_UI_QUERY_LEFT_INFO);
        RECV_MSG(CMD_WEB_UI_GPS_STATE_CHANGE);
        RECV_MSG(CMD_WEB_UI_SHUT_DOWN);

    default: return "Unkown Message Type";
    }    
}


#define DISPLAY_TYPE(n) case n: return #n
const char *getDispType(int iType)
{
    switch (iType) {
        DISPLAY_TYPE(START_RECING);
        DISPLAY_TYPE(START_REC_SUC);
        DISPLAY_TYPE(START_REC_FAIL);
        DISPLAY_TYPE(STOP_RECING);
        DISPLAY_TYPE(STOP_REC_SUC);

        DISPLAY_TYPE(STOP_REC_FAIL);
        DISPLAY_TYPE(CAPTURE);
        DISPLAY_TYPE(CAPTURE_SUC);
        DISPLAY_TYPE(CAPTURE_FAIL);
        DISPLAY_TYPE(COMPOSE_PIC);

        DISPLAY_TYPE(COMPOSE_PIC_FAIL);
        DISPLAY_TYPE(COMPOSE_PIC_SUC);
        DISPLAY_TYPE(COMPOSE_VIDEO);
        DISPLAY_TYPE(COMPOSE_VIDEO_FAIL);
        DISPLAY_TYPE(COMPOSE_VIDEO_SUC);

        DISPLAY_TYPE(STRAT_LIVING);
        DISPLAY_TYPE(START_LIVE_SUC);
        DISPLAY_TYPE(START_LIVE_FAIL);
        DISPLAY_TYPE(STOP_LIVING);
        DISPLAY_TYPE(STOP_LIVE_SUC);


        DISPLAY_TYPE(STOP_LIVE_FAIL);
        DISPLAY_TYPE(PIC_ORG_FINISH);
        DISPLAY_TYPE(START_LIVE_CONNECTING);
        DISPLAY_TYPE(START_CALIBRATIONING);
        DISPLAY_TYPE(CALIBRATION_SUC);

        DISPLAY_TYPE(CALIBRATION_FAIL);
        DISPLAY_TYPE(START_PREVIEWING);
        DISPLAY_TYPE(START_PREVIEW_SUC);
        DISPLAY_TYPE(START_PREVIEW_FAIL);
        DISPLAY_TYPE(STOP_PREVIEWING);

        DISPLAY_TYPE(STOP_PREVIEW_SUC);
        DISPLAY_TYPE(STOP_PREVIEW_FAIL);
        DISPLAY_TYPE(START_QRING);
        DISPLAY_TYPE(START_QR_SUC);
        DISPLAY_TYPE(START_QR_FAIL);

        DISPLAY_TYPE(STOP_QRING);
        DISPLAY_TYPE(STOP_QR_SUC);
        DISPLAY_TYPE(STOP_QR_FAIL);
        DISPLAY_TYPE(QR_FINISH_CORRECT);
        DISPLAY_TYPE(QR_FINISH_ERROR);

        DISPLAY_TYPE(CAPTURE_ORG_SUC);
        DISPLAY_TYPE(CALIBRATION_ORG_SUC);
        DISPLAY_TYPE(SET_CUS_PARAM);
        DISPLAY_TYPE(QR_FINISH_UNRECOGNIZE);
        DISPLAY_TYPE(TIMELPASE_COUNT);

        DISPLAY_TYPE(START_NOISE_SUC);
        DISPLAY_TYPE(START_NOISE_FAIL);
        DISPLAY_TYPE(START_NOISE);
        DISPLAY_TYPE(START_LOW_BAT_SUC);
        DISPLAY_TYPE(START_LOW_BAT_FAIL);

        DISPLAY_TYPE(LIVE_REC_OVER);
        DISPLAY_TYPE(SET_SYS_SETTING);
        DISPLAY_TYPE(STITCH_PROGRESS);
        DISPLAY_TYPE(START_BLC);
        DISPLAY_TYPE(STOP_BLC);

        DISPLAY_TYPE(START_GYRO);
        DISPLAY_TYPE(START_GYRO_SUC);
        DISPLAY_TYPE(START_GYRO_FAIL);
        DISPLAY_TYPE(SPEED_TEST_SUC);
        DISPLAY_TYPE(SPEED_TEST_FAIL);

        DISPLAY_TYPE(SPEED_START);
        DISPLAY_TYPE(SYNC_REC_AND_PREVIEW);
        DISPLAY_TYPE(SYNC_PIC_CAPTURE_AND_PREVIEW);
        DISPLAY_TYPE(SYNC_PIC_STITCH_AND_PREVIEW);
        DISPLAY_TYPE(SYNC_LIVE_AND_PREVIEW);

        DISPLAY_TYPE(SYNC_LIVE_CONNECT_AND_PREVIEW);
        DISPLAY_TYPE(START_STA_WIFI_FAIL);
        DISPLAY_TYPE(STOP_STA_WIFI_FAIL);
        DISPLAY_TYPE(START_AP_WIFI_FAIL);
        DISPLAY_TYPE(STOP_AP_WIFI_FAIL);

        DISPLAY_TYPE(START_QUERY_STORAGE);
        DISPLAY_TYPE(START_QUERY_STORAGE_SUC);
        DISPLAY_TYPE(START_QUERY_STORAGE_FAIL);
        DISPLAY_TYPE(START_BPC);
        DISPLAY_TYPE(STOP_BPC);

        DISPLAY_TYPE(ENTER_UDISK_MODE);
        DISPLAY_TYPE(EXIT_UDISK_MODE);
        DISPLAY_TYPE(EXIT_UDISK_DONE);
        DISPLAY_TYPE(RESET_ALL_CFG);
        DISPLAY_TYPE(MAX_TYPE);

    default: 
        return "Display Type";
    }    
}


#if 0

void fifo::handleQrContent(sp<DISP_TYPE>& mDispType, cJSON* root, cJSON *subNode)
{
	int iArraySize = cJSON_GetArraySize(subNode);
	int qr_version;
	int qr_index = -1;
	int qr_action_index;
	int sti_res;
	cJSON *child = nullptr;

	subNode = subNode->child;
	CHECK_EQ(subNode->type, cJSON_Number);
	qr_version = subNode->valueint;
	
	for (u32 i = 0; i < sizeof(mQRInfo) / sizeof(mQRInfo[0]); i++) {
		if (qr_version == mQRInfo[i].version) {
			qr_index = i;
			break;
		}
	}
								
	LOGDBG(TAG, "qr version %d array size is %d qr_index %d", qr_version, iArraySize, qr_index);
	if (qr_index != -1) {
		subNode = subNode->next;
		CHECK_EQ(subNode->type, cJSON_Number);
		mDispType->qr_type = subNode->valueint;
		
		LOGDBG(TAG,"qr type %d", mDispType->qr_type);
		qr_action_index = (mDispType->qr_type - ACTION_PIC);
		LOGDBG(TAG, "qr action index %d iArraySize %d "
				"mQRInfo[qr_index].astQRInfo[qr_action_index].qr_size %d ",
				qr_action_index,
				iArraySize,
				mQRInfo[qr_index].astQRInfo[qr_action_index].qr_size);
									
		if (iArraySize == mQRInfo[qr_index].astQRInfo[qr_action_index].qr_size) {
			int org_res;
			mDispType->mAct = sp<ACTION_INFO>(new ACTION_INFO());
			subNode = subNode->next;
			CHECK_EQ(subNode->type, cJSON_Number);
			mDispType->mAct->size_per_act = subNode->valueint;

			LOGDBG(TAG, "size per act %d", mDispType->mAct->size_per_act);
			//org
			subNode = subNode->next;
			CHECK_EQ(subNode->type, cJSON_Array);
                                        
			iArraySize = cJSON_GetArraySize(subNode);
			if (iArraySize > 0) {
				CHECK_EQ(iArraySize, mQRInfo[qr_index].astQRInfo[qr_action_index].org_size);
				child = subNode->child;
				CHECK_EQ(child->type, cJSON_Number);
				
				org_res = child->valueint;
				mDispType->mAct->stOrgInfo.w = mResInfos[org_res].w;

				// org 3d or pano h is same 170721
				mDispType->mAct->stOrgInfo.h = mResInfos[org_res].h[0];

				// skip mime
				child = child->next;
				CHECK_EQ(child->type, cJSON_Number);
				mDispType->mAct->stOrgInfo.mime = child->valueint;

				switch (mDispType->qr_type) {
					// old [1, Predicate, [resolution, mime, saveOrigin, delay], [resolution, mime, mode, algorithm]]
					// new [v,1, Predicate, [resolution, mime, saveOrigin, delay], [resolution, mime, mode, algorithm], [enable, count, step], [enable, count]]
					case ACTION_PIC:
						child = child->next;
						CHECK_EQ(child->type, cJSON_Number);
						
						mDispType->mAct->stOrgInfo.save_org = child->valueint;

						child = child->next;
						CHECK_EQ(child->type, cJSON_Number);
						mDispType->mAct->delay = child->valueint;
						break;

					// [resolution, mime, framerate, originBitrate,saveOrigin]
					case ACTION_VIDEO:
						child = child->next;
						CHECK_EQ(child->type, cJSON_Number);
						
						mDispType->mAct->stOrgInfo.stOrgAct.mOrgV.org_fr = child->valueint;

						child = child->next;
						CHECK_EQ(child->type, cJSON_Number);
						
						mDispType->mAct->stOrgInfo.stOrgAct.mOrgV.org_br = child->valueint;

						child = child->next;
						CHECK_EQ(child->type, cJSON_Number);
						mDispType->mAct->stOrgInfo.save_org = child->valueint;

						child = child->next;
						CHECK_EQ(child->type, cJSON_Number);
						
						mDispType->mAct->stOrgInfo.stOrgAct.mOrgV.logMode = child->valueint;
						break;
						
					// [resolution, mime, framerate, originBitrate,saveOrigin]
					case ACTION_LIVE:
						child = child->next;
						CHECK_EQ(child->type, cJSON_Number);
						mDispType->mAct->stOrgInfo.stOrgAct.mOrgL.org_fr = child->valueint;

						child = child->next;
						CHECK_EQ(child->type, cJSON_Number);
						
						mDispType->mAct->stOrgInfo.stOrgAct.mOrgL.org_br = child->valueint;

						child = child->next;
						CHECK_EQ(child->type, cJSON_Number);
						mDispType->mAct->stOrgInfo.save_org = child->valueint;

						child = child->next;
						CHECK_EQ(child->type, cJSON_Number);
						mDispType->mAct->stOrgInfo.stOrgAct.mOrgL.logMode = child->valueint;
						break;
						
					SWITCH_DEF_ERROR(mDispType->qr_type)
				}
			} else {
				LOGWARN(TAG, "no org mDispType->qr_type %d", mDispType->qr_type);
			}

			//sti
			subNode = subNode->next;
			CHECK_EQ(subNode->type, cJSON_Array);
			iArraySize = cJSON_GetArraySize(subNode);
			switch (mDispType->qr_type) {
				//[resolution, mime, mode, algorithm]]
				case ACTION_PIC:
					if (iArraySize > 0) {
						CHECK_EQ(iArraySize, mQRInfo[qr_index].astQRInfo[qr_action_index].stich_size);
						child = subNode->child;
						CHECK_EQ(child->type, cJSON_Number);
						
						sti_res = child->valueint;

						//skip mime
						child = child->next;
						CHECK_EQ(child->type, cJSON_Number);
						mDispType->mAct->stStiInfo.mime = child->valueint;

						child = child->next;
						CHECK_EQ(child->type, cJSON_Number);
						
						mDispType->mAct->mode = child->valueint;

						mDispType->mAct->stStiInfo.w = mResInfos[sti_res].w;
						mDispType->mAct->stStiInfo.h = mResInfos[sti_res].h[mDispType->mAct->mode];

						child = child->next;
						CHECK_EQ(child->type, cJSON_Number);
						
						mDispType->mAct->stStiInfo.stich_mode = child->valueint;
						LOGDBG(TAG, "2qr pic info %d,[%d,%d,%d,%d,%d],[%d,%d,%d,%d,%d]",
                                                          mDispType->mAct->size_per_act,
                                                          mDispType->mAct->stOrgInfo.w,
                                                          mDispType->mAct->stOrgInfo.h,
                                                          mDispType->mAct->stOrgInfo.mime,
                                                          mDispType->mAct->stOrgInfo.save_org,
                                                          mDispType->mAct->delay,
                                                          mDispType->mAct->stStiInfo.w,
                                                          mDispType->mAct->stStiInfo.h,
                                                          mDispType->mAct->stStiInfo.mime,
                                                          mDispType->mAct->mode,
                                                          mDispType->mAct->stStiInfo.stich_mode);
					} else {
						mDispType->mAct->stStiInfo.stich_mode = STITCH_OFF;
						LOGDBG(TAG, "3qr pic org %d,[%d,%d,%d,%d,%d]",
                                                          mDispType->mAct->size_per_act,
                                                          mDispType->mAct->stOrgInfo.w,
                                                          mDispType->mAct->stOrgInfo.h,
                                                          mDispType->mAct->stOrgInfo.mime,
                                                          mDispType->mAct->stOrgInfo.save_org,
														mDispType->mAct->delay);
					}
					//[count, min_ev,max_ev] -- HDR
					subNode = subNode->next;
					iArraySize = cJSON_GetArraySize(subNode);
					if (iArraySize > 0) {
						CHECK_EQ(iArraySize, mQRInfo[qr_index].astQRInfo[qr_action_index].hdr_size);
						
						child = subNode->child;
						CHECK_EQ(child->type, cJSON_Number);
						
						mDispType->mAct->stOrgInfo.stOrgAct.mOrgP.hdr_count = child->valueint;

						child = child->next;
						CHECK_EQ(child->type, cJSON_Number);
						
						mDispType->mAct->stOrgInfo.stOrgAct.mOrgP.min_ev = child->valueint;

						child = child->next;
						CHECK_EQ(child->type, cJSON_Number);
						
						mDispType->mAct->stOrgInfo.stOrgAct.mOrgP.max_ev = child->valueint;

						LOGDBG(TAG,"hdr %d %d %d",
											mDispType->mAct->stOrgInfo.stOrgAct.mOrgP.hdr_count,
											mDispType->mAct->stOrgInfo.stOrgAct.mOrgP.min_ev,
											mDispType->mAct->stOrgInfo.stOrgAct.mOrgP.max_ev);
					}
						
					subNode = subNode->next;
					//[count] -- Burst
					iArraySize = cJSON_GetArraySize(subNode);
					if (iArraySize > 0) {
						CHECK_EQ(iArraySize, mQRInfo[qr_index].astQRInfo[qr_action_index].burst_size);
						child = subNode->child;
						CHECK_EQ(child->type, cJSON_Number);
							
						mDispType->mAct->stOrgInfo.stOrgAct.mOrgP.burst_count = child->valueint;
						LOGDBG(TAG, "burst count %d",
										mDispType->mAct->stOrgInfo.stOrgAct.mOrgP.burst_count);
					}
					break;

				case ACTION_VIDEO:
					// [resolution, mime, mode, framerate, stichBitrate]]
					if (iArraySize > 0) {
						CHECK_EQ(iArraySize, mQRInfo[qr_index].astQRInfo[qr_action_index].stich_size);
						child = subNode->child;
						CHECK_EQ(child->type, cJSON_Number);
						
						sti_res = child->valueint;
						
						//skip mime
						child = child->next;
						CHECK_EQ(child->type, cJSON_Number);
						mDispType->mAct->stStiInfo.mime = child->valueint;

						child = child->next;
						CHECK_EQ(child->type, cJSON_Number);
						
						mDispType->mAct->mode = child->valueint;

						mDispType->mAct->stStiInfo.w = mResInfos[sti_res].w;
						mDispType->mAct->stStiInfo.h = mResInfos[sti_res].h[ mDispType->mAct->mode];

						child = child->next;
						CHECK_EQ(child->type, cJSON_Number);
						
						mDispType->mAct->stStiInfo.stStiAct.mStiV.sti_fr = child->valueint;

						child = child->next;
						CHECK_EQ(child->type, cJSON_Number);
						
						mDispType->mAct->stStiInfo.stStiAct.mStiV.sti_br = child->valueint;

						//force to stitch normal
						mDispType->mAct->stStiInfo.stich_mode = STITCH_NORMAL;
						LOGDBG(TAG, "qr video sti info [%d,%d,%d,%d,%d,%d]",
                                                          mDispType->mAct->stStiInfo.w,
                                                          mDispType->mAct->stStiInfo.h,
                                                          mDispType->mAct->stStiInfo.mime,
                                                          mDispType->mAct->mode,
                                                          mDispType->mAct->stStiInfo.stStiAct.mStiV.sti_fr,
                                                          mDispType->mAct->stStiInfo.stStiAct.mStiV.sti_br);
					} else {
						mDispType->mAct->stStiInfo.stich_mode = STITCH_OFF;
						LOGDBG(TAG, "stich off");
					}
					
					LOGDBG(TAG, "qr vid org info %d,[%d,%d,%d,%d,%d,%d]",
                                                mDispType->mAct->size_per_act,
                                                mDispType->mAct->stOrgInfo.w,
                                                mDispType->mAct->stOrgInfo.h,
                                                mDispType->mAct->stOrgInfo.mime,
                                                mDispType->mAct->stOrgInfo.save_org,
                                                mDispType->mAct->stOrgInfo.stOrgAct.mOrgV.org_fr,
                                                mDispType->mAct->stOrgInfo.stOrgAct.mOrgV.org_br);

					subNode = subNode->next;
					
					//[interval] -- timelapse
					iArraySize = cJSON_GetArraySize(subNode);
					if (iArraySize > 0) {
						CHECK_EQ(iArraySize, mQRInfo[qr_index].astQRInfo[qr_action_index].timelap_size);
						child = subNode->child;
						CHECK_EQ(child->type, cJSON_Number);
						
						mDispType->mAct->stOrgInfo.stOrgAct.mOrgV.tim_lap_int = child->valueint *1000;
						LOGDBG(TAG,"tim_lap_int  %d", mDispType->mAct->stOrgInfo.stOrgAct.mOrgV.tim_lap_int);
						if (mDispType->mAct->size_per_act == 0) {
							mDispType->mAct->size_per_act = 10;
						}
					}
					break;
					
				// [resolution, mime, mode, framerate, stichBitrate, hdmiState]
				case ACTION_LIVE:
					// iArraySize = cJSON_GetArraySize(subNode);
					CHECK_EQ(iArraySize, mQRInfo[qr_index].astQRInfo[qr_action_index].stich_size);

					child = subNode->child;
					CHECK_EQ(child->type, cJSON_Number);
					
					sti_res = child->valueint;
					//skip mime
					child = child->next;
					CHECK_EQ(child->type, cJSON_Number);
					
					mDispType->mAct->stStiInfo.mime = child->valueint;

					child = child->next;
					CHECK_EQ(child->type, cJSON_Number);
					
					mDispType->mAct->mode = child->valueint;

					child = child->next;
					CHECK_EQ(child->type, cJSON_Number);
					mDispType->mAct->stStiInfo.stStiAct.mStiL.sti_fr = child->valueint;

					child = child->next;
					CHECK_EQ(child->type, cJSON_Number);
					
					mDispType->mAct->stStiInfo.stStiAct.mStiL.sti_br = child->valueint;

					child = child->next;
					CHECK_EQ(child->type, cJSON_Number);
					
					mDispType->mAct->stStiInfo.stStiAct.mStiL.hdmi_on = child->valueint;

					LOGDBG(TAG, "qr live org info [%d,%d,%d,%d,%d]",
								mDispType->mAct->stOrgInfo.w,
								mDispType->mAct->stOrgInfo.h,
								mDispType->mAct->stOrgInfo.mime,
								mDispType->mAct->stOrgInfo.stOrgAct.mOrgL.org_fr,
								mDispType->mAct->stOrgInfo.stOrgAct.mOrgL.org_br);

					//for url
					subNode = subNode->next;
					CHECK_EQ(subNode->type, cJSON_String);
					if (strlen(subNode->valuestring) > 0) {
						snprintf(mDispType->mAct->stStiInfo.stStiAct.mStiL.url,
									sizeof(mDispType->mAct->stStiInfo.stStiAct.mStiL.url),
									"%s",
									subNode->valuestring);
					}
					
					subNode = cJSON_GetObjectItem(root, "proExtra");
					if (subNode) {
						child = cJSON_GetObjectItem(subNode, "saveStitch");
						if (child) {
							if (SAVE_OFF == child->valueint) {
								mDispType->mAct->stStiInfo.stStiAct.mStiL.file_save = 0;
							} else {
								mDispType->mAct->stStiInfo.stStiAct.mStiL.file_save = 1;
							}
						}
						
						child = cJSON_GetObjectItem(subNode, "format");
						if (child) {
							snprintf(mDispType->mAct->stStiInfo.stStiAct.mStiL.format,sizeof(mDispType->mAct->stStiInfo.stStiAct.mStiL.format),
                                                                 "%s",child->valuestring);
						}
						child = cJSON_GetObjectItem(subNode, "map");
						if (child) {
							mDispType->mAct->stStiInfo.stich_mode = get_sti_mode(child->valuestring);
						} else {
							//force to stitch normal
							mDispType->mAct->stStiInfo.stich_mode = STITCH_NORMAL;
						}
					} else {
						//force to stitch normal
						mDispType->mAct->stStiInfo.stich_mode = STITCH_NORMAL;
					}
					
					if (mDispType->mAct->stStiInfo.stich_mode == STITCH_CUBE) {
						mDispType->mAct->stStiInfo.h = mResInfos[sti_res].h[mDispType->mAct->mode];
						mDispType->mAct->stStiInfo.w = mDispType->mAct->stStiInfo.h*3/2;
					} else {
						mDispType->mAct->stStiInfo.h = mResInfos[sti_res].h[mDispType->mAct->mode];
						mDispType->mAct->stStiInfo.w = mResInfos[sti_res].w;
					}
					
					LOGDBG(TAG, "qr live info [%d,%d,%d,%d,%d,%d %d %d] url %s",
								mDispType->mAct->stStiInfo.w,
								mDispType->mAct->stStiInfo.h,
								mDispType->mAct->stStiInfo.mime,
								mDispType->mAct->mode,
								mDispType->mAct->stStiInfo.stStiAct.mStiL.sti_fr,
								mDispType->mAct->stStiInfo.stStiAct.mStiL.sti_br,
                                mDispType->mAct->stStiInfo.stStiAct.mStiL.hdmi_on,
                                mDispType->mAct->stStiInfo.stich_mode,
								mDispType->mAct->stStiInfo.stStiAct.mStiL.url);
                    break;
				SWITCH_DEF_ERROR(mDispType->qr_type)
			}
		} else {
			mDispType->type = QR_FINISH_UNRECOGNIZE;
		}
	} else {
		mDispType->type = QR_FINISH_UNRECOGNIZE;
	}
    
}

#endif


void fifo::handleSetting(sp<struct _disp_type_>& mDispType, Json::Value& reqNode)
{
    // cJSON *child = nullptr;
    mDispType->mSysSetting = sp<SYS_SETTING>(new SYS_SETTING());

    memset(mDispType->mSysSetting.get(), -1, sizeof(SYS_SETTING));

    if (reqNode["flicker"].isInt()) {
        mDispType->mSysSetting->flicker = reqNode["flicker"].asInt();
    }

    if (reqNode["speaker"].isInt()) {
        mDispType->mSysSetting->speaker = reqNode["speaker"].asInt();
    }

    if (reqNode["led_on"].isInt()) {
        mDispType->mSysSetting->led_on = reqNode["led_on"].asInt();
    }

    if (reqNode["fan_on"].isInt()) {
        mDispType->mSysSetting->fan_on = reqNode["fan_on"].asInt();
    }

    if (reqNode["aud_on"].isInt()) {
        mDispType->mSysSetting->aud_on = reqNode["aud_on"].asInt();
    }

    if (reqNode["aud_spatial"].isInt()) {
        mDispType->mSysSetting->aud_spatial = reqNode["aud_spatial"].asInt();
    }

    if (reqNode["set_logo"].isInt()) {
        mDispType->mSysSetting->set_logo = reqNode["set_logo"].asInt();
    }

    if (reqNode["gyro_on"].isInt()) {
        mDispType->mSysSetting->gyro_on = reqNode["gyro_on"].asInt();
    }

    if (reqNode["video_fragment"].isInt()) {
        mDispType->mSysSetting->video_fragment = reqNode["video_fragment"].asInt();
    }

    LOGDBG(TAG, "%d %d %d %d %d %d %d %d %d",
                mDispType->mSysSetting->flicker,
                mDispType->mSysSetting->speaker,
                mDispType->mSysSetting->led_on,
                mDispType->mSysSetting->fan_on,
                mDispType->mSysSetting->aud_on,
                mDispType->mSysSetting->aud_spatial,
                mDispType->mSysSetting->set_logo,
                mDispType->mSysSetting->gyro_on,
                mDispType->mSysSetting->video_fragment);    
}


/*
 * 处理来自HTTP的请求
 */
void fifo::handleReqFormHttp(sp<DISP_TYPE>& mDispType, Json::Value& reqNode)
{
	
    if (reqNode["action"].isNull() == false) {
        /* 设置Customer时，使用该字段来区分是拍照,录像，直播 */
        mDispType->qr_type = reqNode["action"].asInt();
    }

    if (reqNode["param"].isNull() == false) {
        mDispType->jsonArg = reqNode["param"];
    }
						
	switch (mDispType->type) {
		case START_LIVE_SUC: {	/* 16, 启动录像成功 */
            LOGDBG(TAG, "Client control Live");
        	mDispType->control_act = ACTION_LIVE;
			break;
        }
										
		case CAPTURE: {			/* 拍照 */
            LOGDBG(TAG, "Client control Capture");
			mDispType->control_act = ACTION_PIC;
			break;
        }
										
		case START_REC_SUC:	{	/* 1, 启动录像成功 */
            LOGDBG(TAG, "Client control Video");
			mDispType->control_act = ACTION_VIDEO;
			break;
        }
											
		case SET_CUS_PARAM:	{	/* 46, 设置自定义参数 */
            LOGDBG(TAG, "Client control Set Customer");
			mDispType->control_act = CONTROL_SET_CUSTOM;
			break;
        }
										
		SWITCH_DEF_ERROR(mDispType->type);
	}	   
}

void fifo::handleGpsStateChange(Json::Value& queryJson)
{
    int iGpstate;
    if (queryJson.isMember("state")) {
        iGpstate = queryJson["state"].asInt();
        mOLEDHandle->sendUpdateGpsState(iGpstate);
    } 
}

void fifo::handleShutdownMachine(Json::Value& queryJson)
{
    LOGDBG(TAG, "Recv Shut down machine message ...");
    mOLEDHandle->sendShutdown();
}


void fifo::handleQueryLeftInfo(Json::Value& queryJson)
{
    u32 uLeft = 0;

    Json::StreamWriterBuilder builder;

    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    std::ostringstream osOutput;  

    string sendDataStr;
    VolumeManager* vm = VolumeManager::Instance();

    Json::Value rootNode;

    /* 
     * 1.拍照
     * 2.录像/直播存片
     * 录像分为普通录像和timelapse
     */
    if (queryJson.isMember("name")) {
        if (!strcmp(queryJson["name"].asCString(), "camera._takePicture") ) {
            uLeft = vm->calcTakepicLefNum(queryJson, false);
        } else if (!strcmp(queryJson["name"].asCString(), "camera._startRecording")) {
            uLeft = vm->calcTakeRecLefSec(queryJson);
        } else if (!strcmp(queryJson["name"].asCString(), "camera._startLive")) {
            uLeft = vm->calcTakeLiveRecLefSec(queryJson);
        }
    } else {
        uLeft = 0;
    }

    LOGDBG(TAG, "-------- handleQueryLeftInfo");

    rootNode["left"] = uLeft;    
	writer->write(rootNode, &osOutput);
    sendDataStr = osOutput.str();


    write_fifo(EVENT_QUERY_LEFT, sendDataStr.c_str());
}


void fifo::parseAndDispatchRecMsg(int iMsgType, Json::Value& jsonData)
{
    // Json::FastWriter writer;
    // string data = writer.write(jsonData);
    // LOGDBG(TAG, "=============>> Recv Message type[%s], data[%s]", getRecvMsgName(iMsgType), data.c_str());

    switch (iMsgType) {
        case CMD_OLED_DISP_TYPE: {	/* 通信UI线程显示指定UI */
        
            sp<DISP_TYPE> mDispType = (sp<DISP_TYPE>)(new DISP_TYPE());
            if (jsonData["type"].isNull()) {
            } else {
                mDispType->type = jsonData["type"].asInt();
                LOGDBG(TAG, "----------->> Display Type: %s", getDispType(mDispType->type));                
            }

            mDispType->mSysSetting = nullptr;
            mDispType->mStichProgress = nullptr;
            mDispType->mAct = nullptr;
            mDispType->control_act = -1;
            mDispType->tl_count  = -1;
            mDispType->qr_type  = -1;

            if (jsonData["content"].isNull() == false) {
                LOGDBG(TAG, "Qr Function Not implement now ..");
                // handleQrContent(mDispType, root, subNode);
            } else if (jsonData["req"].isNull() == false) {
                handleReqFormHttp(mDispType, jsonData["req"]);
            } else if (jsonData["sys_setting"].isNull() == false) {
                handleSetting(mDispType, jsonData["sys_setting"]);
            } else if (jsonData["tl_count"].isNull() == false) {
                mDispType->tl_count = jsonData["tl_count"].asInt();
            } else {
                // LOGERR(TAG, "---------Unkown Error");
            }

            mOLEDHandle->send_disp_str(mDispType);
            break;
        }

        case CMD_WEB_UI_QUERY_LEFT_INFO: {  /* 查询剩余量信息 */
            LOGDBG(TAG, "Query Left Info now....");
            handleQueryLeftInfo(jsonData);
            break;
        }

        case CMD_WEB_UI_GPS_STATE_CHANGE: {
            LOGDBG(TAG, "Gps State change now....");
            handleGpsStateChange(jsonData);
            break;
        }

        case CMD_WEB_UI_SHUT_DOWN: {
            LOGDBG(TAG, "shut down machine ....");
            handleShutdownMachine(jsonData);
            break;
        }


        case CMD_OLED_SET_SN: {
            sp<SYS_INFO> mSysInfo = sp<SYS_INFO>(new SYS_INFO());
            
            if (jsonData["sn"].isString()) {
                snprintf(mSysInfo->sn, sizeof(mSysInfo->sn), "%s", jsonData["sn"].asCString());    
                LOGDBG(TAG, "Recv SN: %s", mSysInfo->sn);
            }

            if (jsonData["uuid"].isString()) {
                snprintf(mSysInfo->uuid, sizeof(mSysInfo->uuid), "%s", jsonData["uuid"].asCString());    
                LOGDBG(TAG, "Recv SN: %s", mSysInfo->uuid);
            }
            mOLEDHandle->send_sys_info(mSysInfo);
            break;
        }


        case CMD_OLED_SYNC_INIT: {	/* 给UI发送同步信息: state, a_v, h_v, c_v */

            sp<SYNC_INIT_INFO> mSyncInfo = sp<SYNC_INIT_INFO>(new SYNC_INIT_INFO());
            
            LOGDBG(TAG, "----------> CMD_OLED_SYNC_INIT");
            LOGDBG(TAG, "state: %d", jsonData["state"].asInt());
            LOGDBG(TAG, "a_v: %s ", jsonData["a_v"].asCString());
            LOGDBG(TAG, "h_v: %s ", jsonData["h_v"].asCString());
            LOGDBG(TAG, "c_v: %s ", jsonData["c_v"].asCString());


            if (jsonData.isMember("state")) {
                mSyncInfo->state = jsonData["state"].asInt();
            } else {
                mSyncInfo->state = 0;
            }
            
            if (jsonData.isMember("a_v")) {
                snprintf(mSyncInfo->a_v, sizeof(mSyncInfo->a_v), "%s", jsonData["a_v"].asCString());
            }            
            
            if (jsonData.isMember("h_v")) {
                snprintf(mSyncInfo->h_v, sizeof(mSyncInfo->h_v), "%s", jsonData["h_v"].asCString());
            }                

            if (jsonData.isMember("c_v")) {
                snprintf(mSyncInfo->c_v, sizeof(mSyncInfo->c_v), "%s", jsonData["c_v"].asCString());
            }                
            mOLEDHandle->send_sync_init_info(mSyncInfo);
            break;
        }    
   

        case CMD_OLED_DISP_TYPE_ERR: {	/* 给UI发送显示错误信息:  错误类型和错误码 */
            sp<ERR_TYPE_INFO> mInfo = sp<ERR_TYPE_INFO>(new ERR_TYPE_INFO());

            if (jsonData["type"].isNull() == false) {
                mInfo->type = jsonData["type"].asInt();
            }    

            if (jsonData["err_code"].isNull() == false) {
                mInfo->err_code = jsonData["err_code"].asInt();
            }    
            mOLEDHandle->send_disp_err(mInfo);
            break;
        }

        case CMD_WEB_UI_TF_CHANGED: {   /* 暂时每次只能解析一张卡的变化 */  

            LOGDBG(TAG, "[%s:%d] Get Tfcard Changed....");      

            std::vector<sp<Volume>> storageList;
            
            storageList.clear();

            /*  
             * {'module': {'storage_total': 61024, 'storage_left': 47748, 'pro_suc': 1, 'index': 1}}
             */
            if (jsonData["module"].isNull() == false) {
                sp<Volume> tmpVol = (sp<Volume>)(new Volume());

                if (jsonData["module"]["index"].isInt()) {
                    tmpVol->iIndex = jsonData["module"]["index"].asInt();
                }

                if (jsonData["module"]["storage_total"].isInt()) {
                    tmpVol->uTotal = jsonData["module"]["storage_total"].asInt();
                }

                if (jsonData["module"]["storage_left"].isInt()) {
                    tmpVol->uAvail = jsonData["module"]["storage_left"].asInt();
                }

                snprintf(tmpVol->cVolName, sizeof(tmpVol->cVolName), "mSD%d", tmpVol->iIndex);
                storageList.push_back(tmpVol);

                /* 直接将消息丢入UI线程的消息队列中 */
                mOLEDHandle->sendTfStateChanged(storageList);                
            } else {
                LOGDBG(TAG, "[%s:%d] get module json node[module] failed");                               
            }
            break;
        }

#if 0
        case CMD_WEB_UI_TF_NOTIFY: {    /* 查询TF卡的状态 */
            LOGDBG(TAG, "[%s:%d] get notify form server for TF info");

            bool bResult = false;
            // char cStoragePath[64] = {0};
            // int iModuleArray = 0;
            std::vector<sp<Volume>> storageList;
            
            storageList.clear();

            if ( (jsonData["state"].isNull() == false) && (jsonData["results"].isNull() == false)) {
                if (!strcmp(jsonData["state"].asCString(), "done")) {
                    if (jsonData["results"]["module"].isArray()) {
                        for (u32 i = 0; i < jsonData["results"]["module"].size(); i++) {
                            sp<Volume> tmpVol = (sp<Volume>)(new Volume());
                            if (jsonData["results"]["module"][i]["index"].isInt()) {
                                tmpVol->iIndex = jsonData["results"]["module"][i]["index"].asInt();
                            }

                            if (jsonData["results"]["module"][i]["storage_total"].isInt()) {
                                tmpVol->uTotal = jsonData["results"]["module"][i]["storage_total"].asInt();
                            }

                            if (jsonData["results"]["module"][i]["storage_left"].isInt()) {
                                tmpVol->uAvail = jsonData["results"]["module"][i]["storage_left"].asInt();
                            }

                            if (jsonData["results"]["module"][i]["pro_suc"].isInt()) {
                                tmpVol->iSpeedTest = jsonData["results"]["module"][i]["pro_suc"].asInt();
                            }

                            /* 类型为"SD"
                            * 外部TF卡的命名规则
                            * 名称: "tf-1","tf-2","tf-3"....
                            */
                            sprintf(tmpVol->cVolName, "mSD%d", tmpVol->iIndex);
                            #if 0
                            LOGDBG(TAG, "TF card node[%s] info index[%d], total space[%d]M, left space[%d], speed[%d]",
                                        tmpVol->cVolName, 
                                        tmpVol->iIndex, tmpVol->uTotal, tmpVol->uAvail, tmpVol->iSpeedTest);

                            #endif

                            storageList.push_back(tmpVol);

                        }
                        bResult = true; 
                    } else {
                        LOGERR(TAG, "module not array, what's wrong");
                    }
                }
            } else {
                LOGERR(TAG, "state node not exist!");
            }

            mOLEDHandle->updateTfStorageInfo(bResult, storageList);
            break;
        }
#endif        


        case CMD_WEB_UI_TF_FORMAT: {    /* 格式化结果 */
            LOGDBG(TAG, "Get Notify(mSD Format Info)");

            sp<Volume> tmpVolume = (sp<Volume>)(new Volume());
            std::vector<sp<Volume>> storageList;

            if (jsonData["state"].isNull()) {
                LOGDBG(TAG, "CMD_WEB_UI_TF_FORMAT Protocal Err, no 'state'");
                storageList.push_back(tmpVolume); 
            } else {
                
                if (!strcmp(jsonData["state"].asCString(), "done")) { /* 格式化成功 */
                    /* do nothind */
                } else {    /* 格式化失败: TODO - 传递格式化失败的设备号(需要camerad处理) */
                    storageList.push_back(tmpVolume); 
                }                
            }
            /* 直接将消息丢入UI线程的消息队列中 */
            mOLEDHandle->notifyTfcardFormatResult(storageList);
            break;

        }


        case CMD_WEB_UI_TEST_SPEED_RES: {

            LOGDBG(TAG, "Return Speed Test Result");

            std::vector<sp<Volume>> storageList;
            sp<Volume> tmpVol = NULL;


            storageList.clear();

            if (jsonData["local"].isNull() == false) {
                tmpVol = (sp<Volume>)(new Volume());
                tmpVol->iType = VOLUME_TYPE_NV;
                tmpVol->iSpeedTest = jsonData["local"].asInt();
                LOGDBG(TAG, "Local Device Test Speed Result: %d", tmpVol->iSpeedTest);
                storageList.push_back(tmpVol);
            }

            if (jsonData["module"].isNull() == false) {
                if (jsonData["module"].isArray()) {
                    for (u32 i = 0; i < jsonData["module"].size(); i++) {
                        tmpVol = (sp<Volume>)(new Volume());

                        tmpVol->iType       = VOLUME_TYPE_MODULE;
                        tmpVol->iIndex      = jsonData["module"][i]["index"].asInt();
                        tmpVol->iSpeedTest  = jsonData["module"][i]["result"].asInt();

                        /* 类型为"SD"
                        * 外部TF卡的命名规则
                        * 名称: "tf-1","tf-2","tf-3"....
                        */
                        snprintf(tmpVol->cVolName, sizeof(tmpVol->cVolName), "mSD%d", tmpVol->iIndex);
                        LOGDBG(TAG, "mSD card node[%s] info index[%d], speed[%d]",
                                     tmpVol->cVolName,  tmpVol->iIndex, tmpVol->iSpeedTest);

                        storageList.push_back(tmpVol);
                    }

                } else {
                    LOGERR(TAG, "node module not array!!");
                }
                
            }

            mOLEDHandle->sendSpeedTestResult(storageList);
            break;
        }


        case CMD_WEB_UI_SWITCH_MOUNT_MODE: {
            LOGDBG(TAG, "Switch Mount Mode");
            VolumeManager* vm = VolumeManager::Instance();

            if (jsonData.isMember("parameters")) {
                if (jsonData["parameters"].isMember("mode")) {
                       if (!strcmp(jsonData["parameters"]["mode"].asCString(), "ro")) {
                           LOGDBG(TAG, "Change mount mode to ReadOnly");
                           vm->changeMountMethod("ro");
                       } else if (!strcmp(jsonData["parameters"]["mode"].asCString(), "rw")) {
                           LOGDBG(TAG, "Change mount mode to Read-Write");
                           vm->changeMountMethod("rw");
                       }
                } else {
                    LOGERR(TAG, "not Member mode");
                }
            } else {
                LOGDBG(TAG, "Invalid Arguments");
            }
            break;
        }

        default: 
            break;

    }
}



/*************************************************************************
** 方法名称: read_fifo_thread
** 方法功能: 读取来自osc的消息
** 入口参数: 
** 返 回 值: 无 
** 调     用: 
** 注: 读线程得到的消息最终会投递到交互线程的消息队列中统一处理
*************************************************************************/
void fifo::read_fifo_thread()
{
    char buf[1024] = {0};
    char result[1024] = {0}; 
    int error_times = 0;

    while (true) {

        memset(buf, 0, sizeof(buf));
        memset(result, 0, sizeof(result));

        get_read_fd();	/* 获取FIFO读端的fd */

		/* 首先读取8字节的头部 */
        int len = read(read_fd, buf, FIFO_HEAD_LEN);
        if (len != FIFO_HEAD_LEN) {	/* 头部读取错误 */
            LOGWARN(TAG, "ReadFifoThread: read fifo head mismatch(rec[%d] act[%d])", len, FIFO_HEAD_LEN);
            if (++error_times >= 3) {
                LOGERR(TAG, ">> read fifo broken?");
                close_read_fd();
            }
        } else {
            int msg_what = bytes_to_int(buf);	/* 前4字节代表消息类型: what */
            if (msg_what == CMD_EXIT) {	/* 如果是退出消息 */
                break;
            } else {
				
				/* 头部的后4字节代表本次数据传输的长度 */
                int content_len = bytes_to_int(&buf[FIFO_DATA_LEN_OFF]);
                CHECK_NE(content_len, 0);

				/* 读取传输的数据 */
                len = read(read_fd, &buf[FIFO_HEAD_LEN], content_len);

				if (len != content_len) {	/* 读取的数据长度不一致 */
                    LOGWARN(TAG, "3read fifo content mismatch(%d %d)", len, content_len);
                    if (++error_times >= 3) {
                        LOGERR(TAG, " 2read fifo broken? ");
                        close_read_fd();
                    }
                } else {

                    Json::CharReaderBuilder builder;
                    builder["collectComments"] = false;
                    JSONCPP_STRING errs;
                    Json::Value rootJson;

                    Json::CharReader* reader = builder.newCharReader();
                    LOGDBG(TAG, "FIFO Recv: %s", &buf[FIFO_HEAD_LEN]);

                    if (!reader->parse(&buf[FIFO_HEAD_LEN], &buf[FIFO_HEAD_LEN + content_len], &rootJson, &errs)) {
                        LOGERR(TAG, "parse json format failed");
                        continue;
                    }

                    // Json::Reader reader;
	                // if (!reader.parse(&buf[FIFO_HEAD_LEN], rootJson, false)) {
		            //     LOGERR(TAG, "bad json format!");
		            //     continue;
	                // }
                    parseAndDispatchRecMsg(msg_what, rootJson);                  
                }
            }
        }
    }
    close_read_fd();
}



void fifo::write_exit_for_read()
{
    char buf[32];
    memset(buf, 0, sizeof(buf));
    int cmd = CMD_EXIT;
    int_to_bytes(buf, cmd);

    int fd = open(FIFO_FROM_CLIENT, O_WRONLY);
    CHECK_NE(fd, -1);

    write(fd, buf, FIFO_HEAD_LEN);
    close(fd);
}

void fifo::deinit()
{
    LOGDBG(TAG, "deinit");
    stop_all();
    LOGDBG(TAG, "deinit2");
    sendExit();
    LOGDBG(TAG, "deinit3");
}

void fifo::close_read_fd()
{
    if (read_fd != -1) {
        close(read_fd);
        read_fd = -1;
    }
}

void fifo::close_write_fd()
{
    if (write_fd != -1) {
        close(write_fd);
        write_fd = -1;
    }
}

int fifo::get_read_fd()
{
    if (read_fd == -1) {
        read_fd = open(FIFO_FROM_CLIENT, O_RDONLY);
        CHECK_NE(read_fd, -1);
    }
    return read_fd;
}

int fifo::get_write_fd()
{
    if (write_fd == -1) {

        bWFifoStop = true;
        write_fd = open(FIFO_TO_CLIENT, O_WRONLY);
        CHECK_NE(write_fd, -1);
        bWFifoStop = false;
    }
    return write_fd;
}

int fifo::make_fifo()
{
    if (access(FIFO_FROM_CLIENT, F_OK) == -1) {
        if (mkfifo(FIFO_FROM_CLIENT, 0777)) {
            LOGDBG(TAG, "make fifo:%s fail", FIFO_FROM_CLIENT);
            return INS_ERR;
        }
    }

    if (access(FIFO_TO_CLIENT, F_OK) == -1) {
        if (mkfifo(FIFO_TO_CLIENT, 0777)) {
            LOGDBG(TAG, "make fifo:%s fail", FIFO_TO_CLIENT);
            return INS_ERR;
        }
    }
    LOGDBG(TAG, "---> make fifo ok!!");
    return INS_OK;
}
