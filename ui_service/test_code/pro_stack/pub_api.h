//
// Created by vans on 30/8/17.
//

#ifndef PROJECT_PUB_API_H
#define PROJECT_PUB_API_H
class pub_api
{
public:
    pub_api()
    {
        init();
    }

    ~pub_api()
    {
        deinit();
    }
public:
    void init();
    void deinit();
};
#endif //PROJECT_PUB_API_H
