#ifndef _OSC_UTILS_H_
#define _OSC_UTILS_H_

#include <iostream>
#include <memory>

void printJson(Json::Value& json);
std::string convJson2String(Json::Value& json);
int getUptime();
std::string extraAbsDirFromUri(std::string fileUrl);
bool endWith(const std::string &str, const std::string &tail);
bool startWith(const std::string &str, const std::string &head);

#endif  /* _OSC_UTILS_H_ */