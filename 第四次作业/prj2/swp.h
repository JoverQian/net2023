#ifndef __SWP_H__
#define __SWP_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include "list.h"
#include "crc.h"
#include "time.h"
#include "memory.h"

//Remember, your frame can be AT MOST 64 bytes!
#define MAX_FRAME_SIZE 64 //帧的大小（单位字节）

#define FRAME_SRC_SIZE  1 //帧中源地址字段大小（单位字节）
#define FRAME_DST_SIZE  1 //帧中目的地址字段大小（单位字节）
#define FRAME_SEQ_SIZE  1 //帧中帧序号字段大小（单位字节）
#define FRAME_ACK_SIZE  1 //帧中帧确认号字段大小（单位字节）
#define FRAME_FLAG_SIZE 1 //帧中标志字段大小（单位字节）
#define FRAME_FCS_SIZE  2 //帧中循环冗余余数字段大小（单位字节）
#define FRAME_PAYLOAD_SIZE MAX_FRAME_SIZE - FRAME_SRC_SIZE - FRAME_DST_SIZE - FRAME_SEQ_SIZE - FRAME_ACK_SIZE - FRAME_FLAG_SIZE - FRAME_FCS_SIZE //帧中数据字段大小（单位字节）

#define FRAME_FLAG_SEG_MIDDLE 0x00 //帧中标志字段中的段中
#define FRAME_FLAG_SEG_HEAD 0x01 //帧中标志字段中的段首
#define FRAME_FLAG_SEG_TAIL 0x02 //帧中标志字段中的段尾
#define FRAME_FLAG_SEG_DONT 0x03 //帧中标志字段中的不需要分段

#define FRAME_MAX_SEQ (uint64_t)pow(2,FRAME_SEQ_SIZE*8) //帧中序号的最大大小

struct Frame_t
{
    uint8_t  src; //源地址
    uint8_t  dst; //目的地址
    uint8_t  seq; //帧序号
    uint8_t  ack; //帧确认号
    uint8_t  flag; //最后两位用来表示分段情况
                    /*  0x00 段中
                        0x01 段首
                        0x02 段尾
                        0x03 不需要分段   */
    char     data[FRAME_PAYLOAD_SIZE]; //数据部分
    uint16_t fcs; //循环冗余余数
};
typedef struct Frame_t Frame; //帧


#define SWP_WINDOW_SIZE 8 //滑动窗口的大小

struct SenderSlideWindowProtocol_t
{
    uint8_t left; //滑动窗口的左边的序号
    Frame   frame[SWP_WINDOW_SIZE]; //缓存帧
    uint8_t is_ack[SWP_WINDOW_SIZE]; //记录该位置的帧是否已收到确认帧
    struct timeval expiring_timeval[SWP_WINDOW_SIZE]; //记录该位置的帧啥时候超时

    LLnode* output_framelist_head; //待发送帧链表

    uint8_t first_empty_frame_index; //记录第一个空位的索引
};
typedef struct SenderSlideWindowProtocol_t SSWP; //发送者的滑动窗口

struct ReceiverSlideWindowProtocol_t
{
    uint8_t left; //滑动窗口的左边的序号
    Frame frame[SWP_WINDOW_SIZE]; //缓存帧
    uint8_t has_frame[SWP_WINDOW_SIZE]; //记录该位置是否收到帧
};
typedef struct ReceiverSlideWindowProtocol_t RSWP; //接收者的滑动窗口


/**
 * @brief 初始化发送方滑动窗口（Sender Slide Window）结构体。
 *
 * 用于初始化发送方滑动窗口结构体（SSWP），包括设置左边界、初始化帧数据、确认标志、超时时间等相关字段，
 * 并将输出帧列表头指针和第一个空闲帧索引初始化为空和零。
 *
 * @param swp 指向发送方滑动窗口结构体的指针。
 */
void init_sender_slide_window(SSWP* swp);

/**
 * @brief 在发送方滑动窗口（Sender Slide Window）中确认接收到的帧。
 *
 * 用于确认接收到的帧，并在滑动窗口内移动滑动窗口以维护滑动窗口的状态。
 *
 * @param swp 指向发送方滑动窗口结构体的指针。
 * @param ack 确认号，表示已接收的帧的序号。
 */
void ack_frame_in_sender_slide_window(SSWP* swp,uint8_t ack);

/**
 * @brief 将输出帧追加到发送方滑动窗口（Sender Slide Window）的输出帧列表中。
 *
 * 用于将一个输出帧追加到发送方滑动窗口的输出帧列表中。
 *
 * @param swp 指向发送方滑动窗口结构体的指针。
 * @param frame 指向要追加的输出帧的指针。
 */
void append_output_frame_to_sender_slide_window(SSWP* swp,Frame* frame);

/**
 * @brief 发送帧。
 *
 * 用于发送帧。
 *
 * @param swp 指向发送方滑动窗口结构体的指针。
 * @param outgoing_frames_head_ptr 输出帧列表的头指针的地址。
 */
void send_output_frame_from_sender_slide_window(SSWP* swp,LLnode ** outgoing_frames_head_ptr);

/**
 * @brief 重新发送发送方滑动窗口（Sender Slide Window）中超时的帧。
 *
 * 用于在发送方滑动窗口中找到并重新发送已发送但尚未被确认的超时帧。
 * 它检查每个帧的超时状态，如果发现某个帧已经超时，则将该帧重新发送，并重新设置计时器。
 *
 * @param swp 指向发送方滑动窗口结构体的指针。
 * @param outgoing_frames_head_ptr 传输层输出帧列表的头指针的地址。
 */
void resend_timeout_frame_from_sender_slide_window(SSWP* swp,LLnode ** outgoing_frames_head_ptr);

/**
 * @brief 初始化接收方滑动窗口（Receiver Slide Window）结构体。
 *
 * 用于初始化接收方滑动窗口结构体（RSWP），包括设置左边界、初始化帧数据以及帧存在标志等相关字段。
 *
 * @param swp 指向接收方滑动窗口结构体的指针。
 */
void init_receiver_slide_window(RSWP* swp);

/**
 * @brief 将接收到的帧放入接收方滑动窗口（Receiver Slide Window）并交付给上一层。
 *
 * 用于将接收到的帧放入接收方滑动窗口中，并检查是否可以交付一片连续的帧给上一层。
 * 如果发现一片连续的帧，它们将被交付给上一层，并滑动窗口将相应地更新。
 *
 * @param swp 指向接收方滑动窗口结构体的指针。
 * @param frame 指向接收到的帧的指针。
 */
void put_frame_in_receiver_slide_window(RSWP* swp, Frame* frame);

/**
 * @brief 将接收到的帧数据交付给上一层处理。
 *
 * 用于将接收到的帧数据交付给上一层处理，根据帧标志（flag）的不同分别处理段首、段中、段尾或不分段的情况，
 * 并在标准输出上打印相应的信息。
 *
 * @param frame 指向接收到的帧的指针。
 */
void deliver_frame_to_upper(Frame* frame);

/**
 * @brief 为帧计算并设置循环冗余校验码（FCS）字段。
 *
 * 用于计算并设置帧的循环冗余校验码（FCS）字段，以确保数据的完整性。
 * 它将帧除去FCS字段的部分作为输入，计算出FCS值，并将其设置到帧的FCS字段中。
 *
 * @param frame 指向帧结构体的指针。
 */
void set_fcs_frame(Frame* frame);

/**
 * @brief 检查帧是否损坏。
 *
 * 用于检查接收到的帧是否损坏。它计算整个帧的循环冗余校验码（FCS），
 * 如果计算出的FCS值为0，说明帧没有损坏，否则认为帧损坏。
 *
 * @param frame 指向帧结构体的指针。
 * @return 如果帧没有损坏，返回1；否则返回0。
 */
uint8_t is_corrupted(Frame* frame);


#endif