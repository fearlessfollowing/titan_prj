
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
#include <json/value.h>
#include <json/json.h>
 
using namespace std;


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
	char* json = getfileAll("parseJson.json");
	parseJSON(json);
	printf("===============================\n");

	/*组装Json串*/
	createJSON();
	getchar();
	
    return 1;
}
