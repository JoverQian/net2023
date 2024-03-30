#ifndef __CRC_H__
#define __CRC_H__

#include <stdlib.h>
#include <stdint.h>


/**
 * @brief 计算使用CCITT CRC-16算法的循环冗余校验码（FCS）。
 *
 * 这个函数接受一个指向数据缓冲区的指针和数据的长度，并使用CCITT CRC-16算法来计算数据的循环冗余校验码（FCS）。
 *
 * @param p 指向数据缓冲区的指针。
 * @param len 数据的长度（以字节为单位）。
 *
 * @return uint16_t 返回计算得到的CRC-16校验值（FCS）。
 */
uint16_t crc16_ccitt(uint8_t* p,size_t len);


#endif