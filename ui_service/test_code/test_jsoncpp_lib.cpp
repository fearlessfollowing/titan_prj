
/********************************************************
Copyright (C), 2016-2017,
FileName: 	main
Author: 	woniu201
Email: 		wangpengfei.201@163.com
Created: 	2017/09/06
Description:use jsoncpp src , not use dll, but i also provide dll and lib.
********************************************************/
 
#include <stdio.h>
#include <string>
#include <memory>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <json/value.h>
#include <json/writer.h>
#include <json/json.h>



#if 0
// origin = jpeg / jpeg+raw

take_pic_8k_3d_of.json
{"name": "camera._takePicture", "parameters": {"delay": 0, "origin": {"mime": "jpeg", "saveOrigin": true, "width": 4000, "height": 3000, "storage_loc": 0}, "stiching": {"mode": "3d_top_left", "height": 7680, "width": 7680, "mime": "jpeg", "algorithm": "opticalFlow"}}} 

take_pic_8k_of.json
{"name": "camera._takePicture", "parameters": {"delay": 0, "origin": {"mime": "jpeg", "saveOrigin": true, "width": 4000, "height": 3000, "storage_loc": 0}, "stiching": {"mode": "pano", "height": 3840, "width": 7680, "mime": "jpeg", "algorithm": "opticalFlow"}}}

take_pic_8k.json
{"name": "camera._takePicture", "parameters": {"delay": 0, "origin": {"mime": "jpeg", "saveOrigin": true, "width": 4000, "height": 3000, "storage_loc": 0}}}

take_pic_aeb.json
{"name": "camera._takePicture", "parameters": {"delay": 0, "hdr": {"enable": true, "count": 9, "min_ev": -10, "max_ev": 10}, "origin": {"mime": "raw+jpeg", "saveOrigin": true, "width": 4000, "height": 3000, "storage_loc": 0}}} 

take_pic_burst.json
{"name": "camera._takePicture", "parameters": {"delay": 0, "burst": {"enable": true, "count": 10}, "origin": {"mime": "raw+jpeg", "saveOrigin": true, "width": 4000, "height": 3000, "storage_loc": 0}}}

take_pic_customer.json
{"name": "camera._takePicture", "parameters": {"delay": 0, "origin": {"mime": "jpeg", "saveOrigin": true, "width": 4000, "height": 3000, "storage_loc": 0}, "stiching": {"mode": "3d_top_left", "height": 7680, "width": 7680, "mime": "jpeg", "algorithm": "opticalFlow"}}} 

8k_30f_3d
{"name":"camera._startRecording","parameters":{"origin":{"mime":"h264","framerate":30,"bitrate":120000,"saveOrigin":true,"width":3840,"height":2880,"storage_loc":1}}}


g++ --std=c++11 test_jsoncpp_lib.cpp -I ../include/ -L ../ -ljsoncpp


amixer cset -c tegrasndt186ref name="I2S1 Mux" 20
amixer cset -c tegrasndt186ref name="MIXER1-1 Mux" 1
amixer cset -c tegrasndt186ref name="Adder1 RX1" 1
amixer cset -c tegrasndt186ref name="Mixer Enable" 1
amixer cset -c tegrasndt186ref name="ADMAIF1 Mux" 11
amixer cset -c tegrasndt186ref name="x Int Spk Switch" 1
amixer cset -c tegrasndt186ref name="x Speaker Playback Volume" 100
amixer cset -c tegrasndt186ref name="x Headphone Jack Switch" 0
amixer sset -c tegrasndt186ref 'MIXER1-1 Mux' 'ADMAIF1'
amixer sset -c tegrasndt186ref 'I2S1 Mux' 'MIXER1-1'
amixer cset -c 1 name="x Speaker Playback Volume" 100  ## reg-01: 8080

#endif


/*
 * 1.生成json对象并保存到文件
 */
void genDefaultCfg()
{
    Json::Value rootCfg;
    Json::Value modeSelectCfg;
    Json::Value sysSetCfg;
    Json::Value sysWifiCfg;

    modeSelectCfg["pic_mode"] = 0;
    modeSelectCfg["video_mode"] = 0;
    modeSelectCfg["live_mode"] = 0;


    sysSetCfg["dhcp"]           = 1;
    sysSetCfg["flicker"]        = 0;
    sysSetCfg["hdr"]            = 0;
    sysSetCfg["raw"]            = 0;
    sysSetCfg["aeb"]            = 0;        // AEB3
    sysSetCfg["ph_delay"]       = 1;        // 5S
    sysSetCfg["aeb"]            = 0;        // AEB3
    sysSetCfg["ph_delay"]       = 1;        // 5S
    sysSetCfg["speaker"]        = 1;        // Speaker: On
    sysSetCfg["light_on"]       = 1;        // LED: On
    sysSetCfg["aud_on"]         = 1;        // Audio: On
    sysSetCfg["aud_spatial"]    = 1;        // Spatial Audio: On
    sysSetCfg["flow_state"]     = 1;        // FlowState: Off
    sysSetCfg["gyro_on"]        = 1;        // Gyro: On
    sysSetCfg["fan_on"]         = 0;        // Fan: On
    sysSetCfg["set_logo"]       = 0;        // Logo: On
    sysSetCfg["video_fragment"] = 0;        // Video Fragment: On


	sysWifiCfg["ssid"] = "Insta360";
	sysWifiCfg["passwd"] = "888888";

    rootCfg["mode_select"] = modeSelectCfg;
    rootCfg["sys_setting"] = sysSetCfg;
    rootCfg["wifi_cfg"] = sysWifiCfg;

#if 1

    Json::StreamWriterBuilder builder; 
    // builder.settings_["indentation"] = ""; 
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter()); 
    std::ofstream ofs;
	ofs.open("./def_cfg.json");
    writer->write(rootCfg, &ofs);
	ofs.close();

#else 
    Json::StyledWriter sw;
	std::cout << sw.write(rootCfg) << std::endl;
    
    std::ofstream os;
	os.open(DEF_CFG_PARAM_FILE);
	os << sw.write(rootCfg);
	os.close();
#endif

}


bool loadCfgFormFile(Json::Value& root, const char* pFile)
{
    Json::CharReaderBuilder builder;
    builder["collectComments"] = false;
    JSONCPP_STRING errs;

    std::ifstream ifs;  
    ifs.open(pFile, std::ios::binary); 

	/*
	 * 成功返回true
	 */
    if (parseFromStream(builder, ifs, &root, &errs)) {
        fprintf(stderr, "parse [%s] success\n", pFile);
    }  
}

int main()
{
	Json::Value root;

	printf("jsoncpp test .....\n");
    // genDefaultCfg();
	

	loadCfgFormFile(root, "./def_cfg.json");

	Json::StreamWriterBuilder builder; 
	// builder.settings_["indentation"] = ""; 
	std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter()); 
	writer->write(root, &std::cout);

	if (root["mode_select"].isMember("live_mode")) {
		printf("live_mode member is exist\n");
	}

	return 0;
}
