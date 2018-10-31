//
// Created by vans on 16-12-2.
//

#ifndef INC_360PRO_SERVICE_MSG_UTIL_H
#define INC_360PRO_SERVICE_MSG_UTIL_H
#include <thread>
#include <chrono>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

class msg_util
{
public:
	static void sleep_ms(uint32_t uiMS)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(uiMS));
//		usleep(uiMS *1000);
//		for(int i = 0; i< 100;i++)
//			;
	}

    static void sleep_us(uint32_t uiUS)
    {
        std::this_thread::sleep_for(std::chrono::microseconds(uiUS));
//		usleep(uiMS *1000);
//		for(int i = 0; i< 100;i++)
//			;
    }

    static long get_cur_time_us()
    {
        struct timeval tv;
        gettimeofday(&tv,NULL);
        return tv.tv_sec * 1000 * 1000 + tv.tv_usec;
    }

    static long get_cur_time_ms()
    {
        struct timeval tv;
        gettimeofday(&tv,NULL);
        return tv.tv_sec * 1000 + tv.tv_usec / 1000;
    }

    static void get_cur_time_by_gettime()
    {
//        struct time t;
//        gettime(&t);
//        printf("The current time is: %2d:%02d:%02d.%02d/n",
//               t.ti_hour, t.ti_min, t.ti_sec, t.ti_hund);
//        return 0;
    }

    static void get_sys_time_by_chrono()
	{
		using std::chrono::system_clock;

		std::chrono::duration<int,std::ratio<60*60*24> > one_day (1);

		system_clock::time_point today = system_clock::now();


		std::time_t tt;

		tt = system_clock::to_time_t ( today );
		printf( "today is: %s\n", ctime(&tt));

//		system_clock::time_point tomorrow = today + one_day;
//		tt = system_clock::to_time_t ( tomorrow );
//        printf( "tomorrow will be: %s\n",ctime(&tt));

//        std::chrono::steady_clock time =  std::steady_clock::now();

//        tt = steady_clock::to_time_t ( tomorrow );
//        printf( "steady now  be: %s\n",ctime(&tt));
	}

	static void get_sys_time(char *sys_time,int len )
	{
		time_t t;                       //时间结构或者对象
		t=time(NULL);                     //获取当前系统的日历时间
#if 0
        struct tm *local,*ptr; //定义tm结构指针存储时间信息
		//通过time()函数来获得日历时间（Calendar Time），
		//其原型为：time_t time(time_t * timer);
		local=localtime(&t);//localtime()函数是将日历时间转化为本地时间
		printf("Local hour is: %d\n",local->tm_hour);//输出tm结构体的时间成员
		printf("UTC hour is: %d\n",local->tm_hour);
		//local=gmtime(&t);
		//gmtime()函数是将日历时间转化为世界标准时间（即格林尼治时间），
		//并返回一个tm结构体来保存这个时间
		ptr=gmtime(&t);//将日历时间转化为世界标准时间
		printf("The UTC time is %s\n",asctime(ptr)); //格式化输出世界标准时间
#endif
//		printf("The local time is %s\n",ctime(&t));//输出本地时间
		snprintf(sys_time,len,"%s",ctime(&t));
	}
};

#endif //INC_360PRO_SERVICE_MSG_UTIL_H
