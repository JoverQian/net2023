#include "memory.h"

/**
 * @param p 指向内存区域的指针。
 * @param size 内存区域的大小（以字节为单位）。
 * @param offset 逻辑位移的偏移量，必须小于 8*size。
 */
void logical_left_shift_memory(uint8_t* p, size_t size, size_t offset){
    size_t left_size=size-offset; //需要拷贝的字节数
    uint8_t* buffer=(uint8_t*)malloc(left_size); //分配需要位移的空间
    memcpy(buffer,p+offset,left_size); //拷贝需要拷贝的
    memcpy(p,buffer,left_size); //将buffer拷贝到p中
    memset(p+left_size,0,offset); //将p中剩余的部分置0
    free(buffer);
}