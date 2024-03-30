#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/**
 * @brief 将指定内存区域向左逻辑位移指定偏移量。
 *
 * 这个函数接受一个指向内存区域的指针，将该区域的数据向左逻辑位移指定的偏移量。
 *
 * @param p 指向内存区域的指针。
 * @param size 内存区域的大小（以字节为单位）。
 * @param offset 逻辑位移的偏移量，必须小于 8*size。
 */
void logical_left_shift_memory(uint8_t* p, size_t size, size_t offset);

#endif