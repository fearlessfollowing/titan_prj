//
// Created by vans on 17-3-24.
//
#include "../include_common.h"
#include "../sp.h"
#include "../sig_util.h"
#include "../cJSON.h"
#include "../check.h"

int main(int argc, char **argv)
{
    printf("json test\n");

    registerSig(default_signal_handler);
    signal(SIGPIPE, pipe_signal_handler);

    const char * test_str = "{\"content\": [1, 60, [6, 0, 0, 3], [0, 0, 1, 0]]}";
    cJSON *root = cJSON_Parse(test_str);

    cJSON *subNode = cJSON_GetObjectItem(root, "content");
    if(root)
    {
        cJSON *child;
        int iArraySize = cJSON_GetArraySize(subNode);
        printf(" array size is %d \n",iArraySize);
        CHECK_EQ(iArraySize,4);
        subNode = subNode->child;
        CHECK_NE(subNode,nullptr);
        CHECK_EQ(subNode->type,cJSON_Number);
        printf("qr type %d\n", subNode->valueint);
        subNode = subNode->next;
        CHECK_NE(subNode,nullptr);
        CHECK_EQ(subNode->type,cJSON_Number);
        printf("size %d\n", subNode->valueint);

        subNode = subNode->next;
        CHECK_NE(subNode,nullptr);
        CHECK_EQ(subNode->type,cJSON_Array);

        child = subNode->child;
        CHECK_NE(child,nullptr);
        CHECK_EQ(child->type,cJSON_Number);
        printf("val0 %d\n", child->valueint);

        child = child->next;
        CHECK_NE(child,nullptr);
        CHECK_EQ(child->type,cJSON_Number);
        printf("val1 %d\n", child->valueint);

        child = child->next;
        CHECK_NE(child,nullptr);
        CHECK_EQ(child->type,cJSON_Number);
        printf("val2 %d\n", child->valueint);

        child = child->next;
        CHECK_NE(child,nullptr);
        CHECK_EQ(child->type,cJSON_Number);
        printf("val3 %d\n", child->valueint);

        subNode = subNode->next;
        CHECK_NE(subNode,nullptr);
        CHECK_EQ(subNode->type,cJSON_Array);

        child = subNode->child;
        CHECK_NE(child,nullptr);
        CHECK_EQ(child->type,cJSON_Number);
        printf("val4 %d\n", child->valueint);

        child = child->next;
        CHECK_NE(child,nullptr);
        CHECK_EQ(child->type,cJSON_Number);
        printf("val5 %d\n", child->valueint);

        child = child->next;
        CHECK_NE(child,nullptr);
        CHECK_EQ(child->type,cJSON_Number);
        printf("val6 %d\n", child->valueint);

        child = child->next;
        CHECK_NE(child,nullptr);
        CHECK_EQ(child->type,cJSON_Number);
        printf("val7 %d\n", child->valueint);

        if (0 != root)
        {
            cJSON_Delete(root);
        }
    }
    else
    {
        printf("root null\n");
    }
}