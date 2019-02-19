#include <vector>
#include <stdio.h>

using namespace std;

typedef struct stTest {
    int a;
    char b;
    int (*ptest)();
} TestItem;

typedef unsigned int u32;

static int test_proc()
{

}

TestItem aa = {
    10,
    'a',
    test_proc,
};

TestItem bb = {
    11,
    'b',
    test_proc,
};

TestItem * gItems[] = {
    &aa,
    &bb,
};

vector<TestItem*> vecItems;

int main()
{
    vecItems.clear();
    TestItem* tmp = NULL;
    int size = sizeof(gItems) / sizeof(gItems[0]);
    printf("size of gItems = %d\n", size);
    for (u32 i = 0; i < size; i++) {
        vecItems.push_back(gItems[i]);
    }

    for (u32 i = 0; i < vecItems.size(); i ++) {
        tmp = vecItems.at(i);
        printf("name [%d]\n", tmp->a);
    }
    return 0;
}