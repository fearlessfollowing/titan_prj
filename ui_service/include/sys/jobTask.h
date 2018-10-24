#ifndef _JOB_TASK_H_
#define _JOB_TASK_H_

#include <sys/ins_types.h>
#include <mutex>
#include <mutex>
#include <common/sp.h>
#include <string>

typedef  void (*pJobLoop)(void* arg);


class JobTask {
public:
    JobTask(const char* jobName, pJobLoop jobFunc);
    ~JobTask()

    void                startJob();
    void                stopJob();

    void                suspendJob();
    void                wakeupJob();

private:
    std::string         mTaskName;              /* 任务的名称 */
    bool                mLoopExit;              /* 退出标志 */
    pJobLoop            mJobFunc;               /* 任务的执行体 */
    u32                 mLoopTimes;             /* 循环体执行的次数 */
    pthread_t           mThreadId;  

};


#endif  /* _JOB_TASK_H_ */