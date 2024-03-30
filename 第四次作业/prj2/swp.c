#include "swp.h"

/**
 * @brief 初始化发送方滑动窗口（Sender Slide Window）结构体。
 *
 * 用于初始化发送方滑动窗口结构体（SSWP），包括设置左边界、初始化帧数据、确认标志、超时时间等相关字段，
 * 并将输出帧列表头指针和第一个空闲帧索引初始化为空和零。
 *
 * @param swp 指向发送方滑动窗口结构体的指针。
 */
void init_sender_slide_window(SSWP* swp){
    swp->left=0;
    memset(swp->frame,0,sizeof(swp->frame));
    memset(swp->is_ack,0,sizeof(swp->is_ack));
    memset(swp->expiring_timeval,0,sizeof(swp->expiring_timeval));
    swp->output_framelist_head=NULL;
    swp->first_empty_frame_index=0;
}

/**
 * @brief 在发送方滑动窗口（Sender Slide Window）中确认接收到的帧。
 *
 * 用于确认接收到的帧，并在滑动窗口内移动滑动窗口以维护滑动窗口的状态。
 *
 * @param swp 指向发送方滑动窗口结构体的指针。
 * @param ack 确认号，表示已接收的帧的序号。
 */
void ack_frame_in_sender_slide_window(SSWP* swp,uint8_t ack){
    uint8_t index=(ack - swp->left + FRAME_MAX_SEQ ) % FRAME_MAX_SEQ; //将ack号映射到滑动窗口中
    if (index >= SWP_WINDOW_SIZE) return; //ack号不在窗口内
    swp->is_ack[index]=1; //将is_ack设置为1
    
    uint8_t offset=0;
    for (uint8_t i=0;i<swp->first_empty_frame_index;++i){ //计算滑动窗口的滑动量offset
        if (0==swp->is_ack[i]) break;
        ++offset;
    }
    if (0==offset) return; //不需要移动滑动窗口

    swp->left=(swp->left+offset)%FRAME_MAX_SEQ; //更新left的值

    logical_left_shift_memory((uint8_t*)swp->frame,sizeof(swp->frame),offset*sizeof(Frame)); //将frame数组左移offset个单位
    logical_left_shift_memory((uint8_t*)swp->is_ack,sizeof(swp->is_ack),offset*sizeof(uint8_t)); //将is_ack数组左移offset个单位
    logical_left_shift_memory((uint8_t*)swp->expiring_timeval,sizeof(swp->expiring_timeval),offset*sizeof(struct timeval)); //将expiring_timeval数组左移offset个单位

    swp->first_empty_frame_index-=offset; //更新first_empty_frame_index的值
}

/**
 * @brief 将输出帧追加到发送方滑动窗口（Sender Slide Window）的输出帧列表中。
 *
 * 用于将一个输出帧追加到发送方滑动窗口的输出帧列表中。
 *
 * @param swp 指向发送方滑动窗口结构体的指针。
 * @param frame 指向要追加的输出帧的指针。
 */
void append_output_frame_to_sender_slide_window(SSWP* swp,Frame* frame){
    ll_append_node(&(swp->output_framelist_head),frame);
}


/**
 * @brief 发送帧。
 *
 * 用于发送帧。
 *
 * @param swp 指向发送方滑动窗口结构体的指针。
 * @param outgoing_frames_head_ptr 输出帧列表的头指针的地址。
 */
void send_output_frame_from_sender_slide_window(SSWP* swp,LLnode ** outgoing_frames_head_ptr){
    while(swp->first_empty_frame_index<SWP_WINDOW_SIZE){ //把滑动窗口的空位都用掉
        if (!ll_get_length(swp->output_framelist_head)) break; //该滑动窗口待发送帧链表是空的，break
        LLnode* ll_output_frame_node=ll_pop_node(&(swp->output_framelist_head));
        Frame* frame=ll_output_frame_node->value;

        frame->seq=(swp->left+swp->first_empty_frame_index)%FRAME_MAX_SEQ; //通过left取得frame的seq
        set_fcs_frame(frame); //计算frame的fcs
        memcpy(&(swp->frame[swp->first_empty_frame_index]),frame,sizeof(Frame)); //将frame拷贝到滑动窗口中
        swp->is_ack[swp->first_empty_frame_index]=0; //设置is_ack为0
        calculate_timeout(&(swp->expiring_timeval[swp->first_empty_frame_index])); //为该frame设置超时时间
        ++swp->first_empty_frame_index; //记录新的空位
        ll_append_node(outgoing_frames_head_ptr,frame); //发送该帧

        free(ll_output_frame_node);
    }
}

/**
 * @brief 重新发送发送方滑动窗口（Sender Slide Window）中超时的帧。
 *
 * 用于在发送方滑动窗口中找到并重新发送已发送但尚未被确认的超时帧。
 * 它检查每个帧的超时状态，如果发现某个帧已经超时，则将该帧重新发送，并重新设置计时器。
 *
 * @param swp 指向发送方滑动窗口结构体的指针。
 * @param outgoing_frames_head_ptr 传输层输出帧列表的头指针的地址。
 */
void resend_timeout_frame_from_sender_slide_window(SSWP* swp,LLnode ** outgoing_frames_head_ptr){
    struct timeval curr_time; //当前时间
    for (int i=0;i<swp->first_empty_frame_index;++i){ //在已发送的且未被确认的帧中找到超时的帧
        if (swp->is_ack[i]) continue; //已确认
        gettimeofday(&curr_time, NULL); //获取当前时间
        if (timeval_usecdiff(&curr_time,&(swp->expiring_timeval[i]))>0) continue; //未超时

        //将超时的帧重新发送
        Frame* frame=(Frame*)malloc(sizeof(Frame));
        memcpy(frame,&(swp->frame[i]),sizeof(Frame));
        ll_append_node(outgoing_frames_head_ptr,frame);

        calculate_timeout(&(swp->expiring_timeval[i])); //重新设置计时器
    }
}

/**
 * @brief 初始化接收方滑动窗口（Receiver Slide Window）结构体。
 *
 * 用于初始化接收方滑动窗口结构体（RSWP），包括设置左边界、初始化帧数据以及帧存在标志等相关字段。
 *
 * @param swp 指向接收方滑动窗口结构体的指针。
 */
void init_receiver_slide_window(RSWP* swp){
    swp->left=0;
    memset(swp->frame,0,sizeof(swp->frame));
    memset(swp->has_frame,0,sizeof(swp->has_frame));
}

/**
 * @brief 将接收到的帧放入接收方滑动窗口（Receiver Slide Window）并交付给上一层。
 *
 * 用于将接收到的帧放入接收方滑动窗口中，并检查是否可以交付一片连续的帧给上一层。
 * 如果发现一片连续的帧，它们将被交付给上一层，并滑动窗口将相应地更新。
 *
 * @param swp 指向接收方滑动窗口结构体的指针。
 * @param frame 指向接收到的帧的指针。
 */
void put_frame_in_receiver_slide_window(RSWP* swp, Frame* frame){
    uint8_t index=(frame->seq - swp->left + FRAME_MAX_SEQ ) % FRAME_MAX_SEQ; //将seq号映射到滑动窗口中
    if (index>=SWP_WINDOW_SIZE) return; //不在滑动窗口内，不移动窗口
    if (swp->has_frame[index]) return; //已经接收过的frame，不移动窗口
    
    //将frame拷贝到滑动窗口中
    memcpy(&(swp->frame[index]),frame,sizeof(Frame));
    swp->has_frame[index]=1;

    uint8_t offset=0;
    for (uint8_t i=0;i<SWP_WINDOW_SIZE;++i){ //找到一片连续的frame，并将其交付给上一层
        if (0==swp->has_frame[i]) break;
        ++offset;

        Frame *out_frame=(Frame*)malloc(sizeof(Frame));
        memcpy(out_frame,&(swp->frame[i]),sizeof(Frame));
        deliver_frame_to_upper(out_frame); //将frame交付给上一层
    }
    if (0==offset) return; //不需要移动滑动窗口

    swp->left=(swp->left+offset)%FRAME_MAX_SEQ; //更新left的值

    logical_left_shift_memory((uint8_t*)swp->frame,sizeof(swp->frame),offset*sizeof(Frame)); //将frame数组左移offset个单位
    logical_left_shift_memory((uint8_t*)swp->has_frame,sizeof(swp->has_frame),offset*sizeof(uint8_t)); //将has_frame数组左移offset个单位
}

/**
 * @brief 将接收到的帧数据交付给上一层处理。
 *
 * 用于将接收到的帧数据交付给上一层处理，根据帧标志（flag）的不同分别处理段首、段中、段尾或不分段的情况，
 * 并在标准输出上打印相应的信息。
 *
 * @param frame 指向接收到的帧的指针。
 */
void deliver_frame_to_upper(Frame* frame){
    if (0x01==(frame->flag&0x03)) { //段首
        printf("<RECV-%d>:[",frame->dst);
        for (uint8_t i=0;i<FRAME_PAYLOAD_SIZE;++i) printf("%c",frame->data[i]);
    }
    else if (0x00==(frame->flag&0x03)){ //段中
        for (uint8_t i=0;i<FRAME_PAYLOAD_SIZE;++i) printf("%c",frame->data[i]);
    }
    else if (0x02==(frame->flag&0x03)){ //段尾
        printf("%s",frame->data);
        printf("]\n");
    }
    else { //不分段
        printf("<RECV-%d>:[%s]\n", frame->dst, frame->data);
    }
    free(frame);
}

/**
 * @brief 为帧计算并设置循环冗余校验码（FCS）字段。
 *
 * 用于计算并设置帧的循环冗余校验码（FCS）字段，以确保数据的完整性。
 * 它将帧除去FCS字段的部分作为输入，计算出FCS值，并将其设置到帧的FCS字段中。
 *
 * @param frame 指向帧结构体的指针。
 */
void set_fcs_frame(Frame* frame){
    frame->fcs=crc16_ccitt((uint8_t*)frame,sizeof(Frame)-sizeof(frame->fcs)); //计算非fcs字段的fcs
}

/**
 * @brief 检查帧是否损坏。
 *
 * 用于检查接收到的帧是否损坏。它计算整个帧的循环冗余校验码（FCS），
 * 如果计算出的FCS值为0，说明帧没有损坏，否则认为帧损坏。
 *
 * @param frame 指向帧结构体的指针。
 * @return 如果帧没有损坏，返回1；否则返回0。
 */
uint8_t is_corrupted(Frame* frame){
    return 0!=crc16_ccitt((uint8_t*)frame,sizeof(Frame)); //计算整个字段的fcs，等于0就说明没损坏
}