#include <iostream>
#include <memory>
#include <sys/ins_types.h>
#include <json/value.h>
#include <json/json.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>


void printJson(Json::Value& json)
{
    std::ostringstream osOutput;  

    std::string resultStr = "";
    Json::StreamWriterBuilder builder;

    // builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

	writer->write(json, &osOutput);
    resultStr = osOutput.str();

	printf("%s\n", resultStr.c_str());

}

std::string convJson2String(Json::Value& json)
{
    std::ostringstream osOutput;  

    std::string resultStr = "";
    Json::StreamWriterBuilder builder;

    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

	writer->write(json, &osOutput);
    resultStr = osOutput.str();
	return resultStr;
}


int getUptime()
{
	int iUptime = 1000;
	FILE* fp = fopen("/proc/uptime", "r");
	if (fp) {
		char buf[512] = {0};
		char num[512] = {0};
		int i = 0;

		fgets(buf, 512, fp);
		while (buf[i] != '.') {
			num[i] = buf[i];
			i++;
		}

		iUptime = atoi(num);
		fclose(fp);
	}
	return iUptime;
}



std::string extraAbsDirFromUri(std::string fileUrl)
{
	const char *delim = "/";
	std::vector<std::string> vUris;
	char cUri[1024] = {0};

	strncpy(cUri, fileUrl.c_str(), (fileUrl.length() > 1024)? 1024: fileUrl.length());
    char* p = strtok(cUri, delim);
    while(p) {
		std::string tmpStr = p;
		vUris.push_back(tmpStr);
        p = strtok(NULL, delim);
    }

	if (vUris.size() < 3) {
		return "none";
	} else {
		std::string result = "/";
		u32 i = 2;
		for (i = 2; i < vUris.size() - 1; i++) {
			result += vUris.at(i);
			
			if (i != vUris.size() - 2)
				result += "/";
		}
		return result;
	}
}


bool endWith(const std::string &str, const std::string &tail) 
{
	return str.compare(str.size() - tail.size(), tail.size(), tail) == 0;
}
 
bool startWith(const std::string &str, const std::string &head) 
{
	return str.compare(0, head.size(), head) == 0;
}





