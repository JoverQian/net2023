#include "time.h"

/**
 * @brief 计算两个时间结构体之间的微秒差值。
 *
 * 用于计算两个时间结构体之间的微秒差值。它接受两个时间结构体作为输入参数，
 * 分别表示起始时间和结束时间，然后计算它们之间的微秒差值并返回结果。
 *
 * @param start_time 指向起始时间结构体的指针。
 * @param finish_time 指向结束时间结构体的指针。
 * @return 返回微秒级别的时间差值。
 */
long timeval_usecdiff(struct timeval *start_time,struct timeval *finish_time){
  long usec;
  usec=(finish_time->tv_sec - start_time->tv_sec)*1000000;
  usec+=(finish_time->tv_usec- start_time->tv_usec);
  return usec;
}

/**
 * @brief 计算超时时间。
 *
 * 用于计算超时时间，在当前时间的基础上增加0.1秒（100,000微秒），
 * 并处理微秒和秒的进位。计算结果将被存储在传入的 `timeout` 结构体中。
 *
 * @param timeout 指向 timeval 结构体的指针，用于存储计算后的超时时间。
 */
void calculate_timeout(struct timeval * timeout){
    gettimeofday(timeout,NULL);
    timeout->tv_usec+=100000; //0.1s
    if (timeout->tv_usec>=1000000){
        timeout->tv_usec-=1000000; //1s
        timeout->tv_sec+=1;
    }
}