
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
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <json/value.h>
#include <json/writer.h>
#include <json/json.h>
 
using namespace std;


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

#endif

/************************************
@ Brief:		read file
@ Author:		woniu201 
@ Created: 		2017/09/06
@ Return:		file data  
************************************/
char *getfileAll(char *fname)
{
	FILE *fp;
	char *str;
	char txt[1000];
	int filesize;
	if ((fp=fopen(fname,"r"))==NULL) {
		printf("open file %s fail \n",fname);
		return NULL;
	}
 
	/*
	获取文件的大小
	ftell函数功能:得到流式文件的当前读写位置,其返回值是当前读写位置偏离文件头部的字节数.
	*/
	fseek(fp,0,SEEK_END); 
	filesize = ftell(fp);
 
	str=(char *)malloc(filesize);
	str[0]=0;
 
	rewind(fp);
	while((fgets(txt,1000,fp))!=NULL){
		strcat(str,txt);
	}
	fclose(fp);
	return str;
}
 
/************************************
@ Brief:		write file
@ Author:		woniu201 
@ Created: 		2017/09/06
@ Return:		    
************************************/
int writefileAll(char* fname,const char* data)
{
	FILE *fp;
	if ((fp=fopen(fname, "w")) == NULL)
	{
		printf("open file %s fail \n", fname);
		return 1;
	}
	
	fprintf(fp, "%s", data);
	fclose(fp);
	
	return 0;
}
 
/************************************
@ Brief:		parse json data
@ Author:		woniu201 
@ Created: 		2017/09/06
@ Return:		    
************************************/
int parseJSON(const char* jsonstr)
{
	Json::Reader reader;
	Json::Value  resp;
 
	if (!reader.parse(jsonstr, resp, false))
	{
		printf("bad json format!\n");
		return 1;
	}
	#if 0
	int result = resp["Result"].asInt();
	string resultMessage = resp["ResultMessage"].asString();
	printf("Result=%d; ResultMessage=%s\n", result, resultMessage.c_str());
 
	Json::Value & resultValue = resp["ResultValue"];
	for (int i=0; i<resultValue.size(); i++)
	{
		Json::Value subJson = resultValue[i];
		string cpuRatio = subJson["cpuRatio"].asString();
		string serverIp = subJson["serverIp"].asString();
		string conNum = subJson["conNum"].asString();
		string websocketPort = subJson["websocketPort"].asString();
		string mqttPort = subJson["mqttPort"].asString();
		string ts = subJson["TS"].asString();
		printf("cpuRatio=%s; serverIp=%s; conNum=%s; websocketPort=%s; mqttPort=%s; ts=%s\n",cpuRatio.c_str(), serverIp.c_str(),
			conNum.c_str(), websocketPort.c_str(), mqttPort.c_str(), ts.c_str());
	}
	#else 
	printf("parser [%s] success", jsonstr);
	#endif

	return 0;
}


int ReadJsonFromFile(const char* filename)  
{  
    Json::Reader reader;// 解析json用Json::Reader   
    Json::Value root; // Json::Value是一种很重要的类型，可以代表任意类型。如int, string, object, array         

    std::ifstream is;  
    is.open (filename, std::ios::binary);    
    if (reader.parse(is, root, false)) {

		printf("Parse file [%s] OK\n", filename);
		#if 0  
        std::string code;  
        if (!root["files"].isNull())  // 访问节点，Access an object value by name, create a null member if it does not exist.  
            code = root["uploadid"].asString();  
        
        code = root.get("uploadid", "null").asString();// 访问节点，Return the member named key if it exist, defaultValue otherwise.    

        int file_size = root["files"].size();  // 得到"files"的数组个数  
        for(int i = 0; i < file_size; ++i) {  	// 遍历数组  
            Json::Value val_image = root["files"][i]["images"];  
            int image_size = val_image.size();  
            for(int j = 0; j < image_size; ++j)  
            {  
                std::string type = val_image[j]["type"].asString();  
                std::string url  = val_image[j]["url"].asString(); 
                printf("type : %s, url : %s \n", type.c_str(), url.c_str());
            }  
        }  
		#else
		Json::FastWriter writer;

		string jsonstr = writer.write(root);
		printf("%s\n", jsonstr.c_str());		
		#endif

    }  else {
		printf("Parse file [%s] failed\n", filename);
	}
    is.close();  

    return 0;  
} 


/************************************
@ Brief:		create json data
@ Author:		woniu201 
@ Created: 		2017/09/06
@ Return:		    
************************************/
int createJSON()
{
	Json::Value req;
	req["Result"] = 1;
	req["ResultMessage"] = "200";

	Json::Value	object1;
	object1["cpuRatio"] = "4.04";
	object1["serverIp"] = "42.159.116.104";
	object1["conNum"] = "1";
	object1["websocketPort"] = "0";
	object1["mqttPort"] = "8883";
	object1["TS"] = "1504665880572";
	
    Json::Value	object2;
	object2["cpuRatio"] = "2.04";
	object2["serverIp"] = "42.159.122.251";
	object2["conNum"] = "2";
	object2["websocketPort"] = "0";
	object2["mqttPort"] = "8883";
	object2["TS"] = "1504665896981";
	
    Json::Value jarray;
	jarray.append(object1);
	jarray.append(object2);
	
    req["ResultValue"] = jarray;
	Json::FastWriter writer;

	string jsonstr = writer.write(req);
	printf("%s\n", jsonstr.c_str());

	writefileAll("createJson.json", jsonstr.c_str());
	return 0;
}

int main()
{
	/*读取Json串，解析Json串*/
	char* json = "{\"name\": \"camera._takePicture\", \"parameters\": {\"delay\": 0, \"origin\": {\"mime\": \"jpeg\", \"saveOrigin\": true, \"width\": 4000, \"height\": 3000, \"storage_loc\": 0}, \"stiching\": {\"mode\": \"3d_top_left\", \"height\": 7680, \"width\": 7680, \"mime\": \"jpeg\", \"algorithm\": \"opticalFlow\"}}}";

	//getfileAll("parseJson.json");
	parseJSON(json);

	ReadJsonFromFile("./take_pic.json");
	printf("===============================\n");

	/*组装Json串*/
	createJSON();
	getchar();
	
    return 1;
}
