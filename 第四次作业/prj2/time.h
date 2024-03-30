#ifndef __TIME_H__
#define __TIME_H__

#include <stdlib.h>
#include <sys/time.h>

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
long timeval_usecdiff(struct timeval *, struct timeval *);

/**
 * @brief 计算超时时间。
 *
 * 用于计算超时时间，在当前时间的基础上增加0.1秒（100,000微秒），
 * 并处理微秒和秒的进位。计算结果将被存储在传入的 `timeout` 结构体中。
 *
 * @param timeout 指向 timeval 结构体的指针，用于存储计算后的超时时间。
 */
void calculate_timeout(struct timeval * timeout);

#endif