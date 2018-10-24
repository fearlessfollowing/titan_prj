#include <memory>
#include <iostream>
#include <string>
#include <json/value.h>
#include <json/json.h>
#include <sstream>
#include <stdio.h>

using namespace std;

int main(int argc, char* argv[])
{
    Json::Value root;
    Json::Value param;
    std::string resultStr = "";
    ostringstream os;

    param["origin"] = "w=1920, h=2440";
    param["audio"] = 94;

    root["name"] = "camera._connect";
    root["parameters"] = param;

    Json::StreamWriterBuilder builder;
    builder.settings_["indentation"] = "";
    
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    // writer->write(root, &std::cout);

    writer->write(root, &os);
    resultStr = os.str();
    printf("len = %lu", resultStr.length());

    printf("%s\n", resultStr.c_str());
    
    return 0;
}