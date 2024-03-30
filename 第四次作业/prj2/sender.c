#include "sender.h"   // sender.h

//滑动窗口——选择重传
//发送方对重复的ack丢弃

void init_sender(Sender * sender, int id)
{
    //TODO(SOLVED): You should fill in this function as necessary
    sender->send_id = id; 
    sender->input_cmdlist_head = NULL; 
    sender->input_framelist_head = NULL; 
    sender->num_swp=glb_receivers_array_length; //有多少个接收者就有多少个滑动窗口
    sender->swp=(SSWP*)malloc(sender->num_swp*sizeof(SSWP));
    for (int i=0;i<sender->num_swp;++i){ //初始化每个滑动窗口
        init_sender_slide_window(&(sender->swp[i]));
    }
}

struct timeval * sender_get_next_expiring_timeval(Sender * sender)
{
    //TODO(SOLVED): You should fill in this function so that it returns the next timeout that should occur
    int index_i=0,index_j=0;
    for (int i=0;i<sender->num_swp;++i){
        for (int j=0;j<SWP_WINDOW_SIZE;++j){
            if ( sender->swp[i].expiring_timeval[j].tv_sec==0 && sender->swp[i].expiring_timeval[j].tv_sec==0 ){
                continue; //跳过还没有开始计时的
            }
            if (sender->swp[i].expiring_timeval[j].tv_sec < sender->swp[index_i].expiring_timeval[index_j].tv_sec){ //先比较秒
                index_i=i;
                index_j=j;
            }
            else if (sender->swp[i].expiring_timeval[j].tv_sec == sender->swp[index_i].expiring_timeval[index_j].tv_sec){ //如果秒一样大
                if (sender->swp[i].expiring_timeval[j].tv_usec < sender->swp[index_i].expiring_timeval[index_j].tv_usec){ //比较微秒
                    index_i=i;
                    index_j=j;
                }
            }
        }
    }

    //如果都没有开始计时，就返回NULL
    if ( sender->swp[index_i].expiring_timeval[index_j].tv_sec==0 && sender->swp[index_i].expiring_timeval[index_j].tv_sec==0 ) return NULL;
    return &(sender->swp[index_i].expiring_timeval[index_j]);
}


void handle_incoming_acks(Sender * sender,
                          LLnode ** outgoing_frames_head_ptr)
{
    //TODO(SOLVED): Suggested steps for handling incoming ACKs
    //    1) Dequeue the ACK from the sender->input_framelist_head
    //    2) Convert the char * buffer to a Frame data type
    //    3) Check whether the frame is corrupted
    //    4) Check whether the frame is for this sender
    //    5) Do sliding window protocol for sender/receiver pair  

    while(ll_get_length(sender->input_framelist_head) > 0){
        LLnode* ll_input_frame_node=ll_pop_node(&sender->input_framelist_head);
        Frame* frame=ll_input_frame_node->value;
        if (is_corrupted(frame)) goto CleanUpHandleIncomingAcksWhile; //检查到帧损坏跳转到清理处
        if (frame->dst!=sender->send_id) goto CleanUpHandleIncomingAcksWhile; //不是发送给这个sender的跳转到清理处
        ack_frame_in_sender_slide_window(&(sender->swp[frame->src]),frame->ack); //移动滑动窗口
        
    CleanUpHandleIncomingAcksWhile: //清理资源
        if (NULL!=frame) free(frame);
        if (NULL!=ll_input_frame_node) free(ll_input_frame_node);
    }
}


void handle_input_cmds(Sender * sender,
                       LLnode ** outgoing_frames_head_ptr)
{
    //TODO(SOLVED): Suggested steps for handling input cmd
    //    1) Dequeue the Cmd from sender->input_cmdlist_head
    //    2) Convert to Frame
    //    3) Set up the frame according to the sliding window protocol
    //    4) Compute CRC and add CRC to Frame

    while(ll_get_length(sender->input_cmdlist_head)>0){ //先将用户输入消息放入滑动窗口的待发送帧链表中
        LLnode * ll_input_cmd_node = ll_pop_node(&sender->input_cmdlist_head); //从cmdlist中拿到一个用户输入的消息
        Cmd * outgoing_cmd = (Cmd *) ll_input_cmd_node->value;
        free(ll_input_cmd_node);

        size_t msg_length = strlen(outgoing_cmd->message)+1;
        if (msg_length>FRAME_PAYLOAD_SIZE){ //消息的长度大于帧负载的大小，需要分段
            size_t offset=0; //偏移量
            Frame* outgoing_frame=(Frame*)malloc (sizeof(Frame)); //分配一个帧
            memset(outgoing_frame,0,sizeof(Frame));
            outgoing_frame->src=outgoing_cmd->src_id; //设置源地址
            outgoing_frame->dst=outgoing_cmd->dst_id; //设置目的地址
            outgoing_frame->flag=FRAME_FLAG_SEG_HEAD; //设置段头
            memcpy(outgoing_frame->data, outgoing_cmd->message+offset,sizeof(outgoing_frame->data)); //将消息拷贝到负载中
            append_output_frame_to_sender_slide_window(&(sender->swp[outgoing_frame->dst]),outgoing_frame); //先将帧加入到swp的待发送帧链表中
            offset+=sizeof(outgoing_frame->data); //偏移量增加
            while(offset+sizeof(outgoing_frame->data) < msg_length){
                outgoing_frame=(Frame*)malloc (sizeof(Frame)); //分配一个帧
                memset(outgoing_frame,0,sizeof(Frame));
                outgoing_frame->src=outgoing_cmd->src_id;
                outgoing_frame->dst=outgoing_cmd->dst_id;
                outgoing_frame->flag=FRAME_FLAG_SEG_MIDDLE; //设置段中
                memcpy(outgoing_frame->data, outgoing_cmd->message+offset,sizeof(outgoing_frame->data));
                append_output_frame_to_sender_slide_window(&(sender->swp[outgoing_frame->dst]),outgoing_frame);
                offset+=sizeof(outgoing_frame->data);
            }
            outgoing_frame=(Frame*)malloc (sizeof(Frame)); //分配一个帧
            memset(outgoing_frame,0,sizeof(Frame));
            outgoing_frame->src=outgoing_cmd->src_id;
            outgoing_frame->dst=outgoing_cmd->dst_id;
            outgoing_frame->flag=FRAME_FLAG_SEG_TAIL; //设置段尾
            memcpy(outgoing_frame->data, outgoing_cmd->message+offset,msg_length-offset);
            append_output_frame_to_sender_slide_window(&(sender->swp[outgoing_frame->dst]),outgoing_frame);
        }
        else { //一个帧就可以装下
            Frame* outgoing_frame=(Frame*)malloc (sizeof(Frame)); //分配一个帧
            memset(outgoing_frame,0,sizeof(Frame));
            outgoing_frame->src=outgoing_cmd->src_id; //设置源地址
            outgoing_frame->dst=outgoing_cmd->dst_id; //设置目的地址
            outgoing_frame->flag=FRAME_FLAG_SEG_DONT; //设置不需要分段
            memcpy(outgoing_frame->data, outgoing_cmd->message,sizeof(outgoing_frame->data)); //将消息拷贝到负载中
            append_output_frame_to_sender_slide_window(&(sender->swp[outgoing_frame->dst]),outgoing_frame);
        }
        free(outgoing_cmd->message);
        free(outgoing_cmd);
    }

    for (int i=0;i<sender->num_swp;++i){ //发送每个滑动窗口的可发送帧
        send_output_frame_from_sender_slide_window(&(sender->swp[i]),outgoing_frames_head_ptr);
    }
}


void handle_timedout_frames(Sender * sender,
                            LLnode ** outgoing_frames_head_ptr)
{
    //TODO(SOLVED): Suggested steps for handling timed out datagrams
    //    1) Iterate through the sliding window protocol information you maintain for each receiver
    //    2) Locate frames that are timed out and add them to the outgoing frames
    //    3) Update the next timeout field on the outgoing frames
    for (int i=0;i<sender->num_swp;++i){
        resend_timeout_frame_from_sender_slide_window(&(sender->swp[i]),outgoing_frames_head_ptr);
    }
}


void * run_sender(void * input_sender)
{    
    struct timespec   time_spec; //精度更高，用来唤醒线程
    struct timeval    curr_timeval; //配合本地的接口来使用
    const int WAIT_SEC_TIME = 0;
    const long WAIT_USEC_TIME = 100000;
    Sender * sender = (Sender *) input_sender; 
    struct timeval * expiring_timeval;
    long sleep_usec_time, sleep_sec_time;
    
    //This incomplete sender thread, at a high level, loops as follows:
    //1. Determine the next time the thread should wake up
    //2. Grab the mutex protecting the input_cmd/inframe queues
    //3. Dequeues messages from the input queue and adds them to the outgoing_frames list
    //4. Releases the lock
    //5. Sends out the messages

    pthread_cond_init(&sender->buffer_cv, NULL);
    pthread_mutex_init(&sender->buffer_mutex, NULL);

    while(1)
    {    
        LLnode* outgoing_framelist_head=NULL; //发送帧链表(包括超时的帧和窗口中新加入的帧)

        //Get the current time 获取到当前时间保存到curr_timeval中
        gettimeofday(&curr_timeval, 
                     NULL);

        //time_spec is a data structure used to specify when the thread should wake up
        //The time is specified as an ABSOLUTE (meaning, conceptually, you specify 9/23/2010 @ 1pm, wakeup)
        //将timeval时间转为timespec时间
        time_spec.tv_sec  = curr_timeval.tv_sec;
        time_spec.tv_nsec = curr_timeval.tv_usec * 1000;

        //Check for the next event we should handle
        //从滑动窗口中找出最快过期的时间
        expiring_timeval = sender_get_next_expiring_timeval(sender);

        //Perform full on timeout
        if (expiring_timeval == NULL) //还没有设置超时就等待0.1秒
        {
            time_spec.tv_sec += WAIT_SEC_TIME;
            time_spec.tv_nsec += WAIT_USEC_TIME * 1000;
        }
        else //否则等待时间为过期时间减去当前时间
        {
            //Take the difference between the next event and the current time
            sleep_usec_time = timeval_usecdiff(&curr_timeval,
                                               expiring_timeval);

            //Sleep if the difference is positive
            if (sleep_usec_time > 0)
            {
                sleep_sec_time = sleep_usec_time/1000000;
                sleep_usec_time = sleep_usec_time % 1000000;   
                time_spec.tv_sec += sleep_sec_time;
                time_spec.tv_nsec += sleep_usec_time*1000;
            }   
        }

        //Check to make sure we didn't "overflow" the nanosecond field
        if (time_spec.tv_nsec >= 1000000000)
        {
            time_spec.tv_sec++;
            time_spec.tv_nsec -= 1000000000;
        }

        
        //*****************************************************************************************
        //NOTE: Anything that involves dequeing from the input frames or input commands should go 
        //      between the mutex lock and unlock, because other threads CAN/WILL access these structures
        //*****************************************************************************************
        pthread_mutex_lock(&sender->buffer_mutex);

        //Check whether anything has arrived
        int input_cmd_length = ll_get_length(sender->input_cmdlist_head);
        int inframe_queue_length = ll_get_length(sender->input_framelist_head);
        
        //Nothing (cmd nor incoming frame) has arrived, so do a timed wait on the sender's condition variable (releases lock)
        //A signal on the condition variable will wakeup the thread and reaquire the lock
        if (input_cmd_length == 0 &&
            inframe_queue_length == 0) //如果什么消息都没有就先等待time_spec
        {
            
            pthread_cond_timedwait(&sender->buffer_cv, 
                                   &sender->buffer_mutex,
                                   &time_spec);
        }
        //Implement this 处理来自接收者的ack
        handle_incoming_acks(sender,
                             &outgoing_framelist_head); 

        //Implement this 处理用户输入的消息
        handle_input_cmds(sender,
                          &outgoing_framelist_head); 

        pthread_mutex_unlock(&sender->buffer_mutex);


        //Implement this 处理滑动窗口中超时的帧
        handle_timedout_frames(sender,
                               &outgoing_framelist_head);

        //CHANGE THIS AT YOUR OWN RISK!
        //Send out all the frames
        int ll_outgoing_frame_length = ll_get_length(outgoing_framelist_head);
        
        while(ll_outgoing_frame_length > 0)
        {
            LLnode * ll_outframe_node = ll_pop_node(&outgoing_framelist_head);
            char * char_buf = (char *)  ll_outframe_node->value;

            //Don't worry about freeing the char_buf, the following function does that
            send_msg_to_receivers(char_buf);

            //Free up the ll_outframe_node
            free(ll_outframe_node);

            ll_outgoing_frame_length = ll_get_length(outgoing_framelist_head);
        }
    }
    pthread_exit(NULL);
    return 0;
}
