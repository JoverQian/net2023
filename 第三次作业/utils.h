#ifndef UTILS_H
#define UTILS_H

#include <memory>
#include <queue>
#include <mutex>
#include <thread>
#include <functional>
#include <future>
#include <type_traits>
#include <atomic>
#include <semaphore.h>
#include <iostream>

namespace utils
{

/*------------Definition of MessageQueue--------------*/
template<class T>
class MessageQueue
{
public:
    MessageQueue(const unsigned int& max_msg_num=4096){
        sem_init(&(this->empty_semaphore),0,max_msg_num);
        sem_init(&(this->full_semaphore),0,0);
    }
    ~MessageQueue(){
        sem_destroy(&(this->empty_semaphore));
        sem_destroy(&(this->full_semaphore));
    }

    bool push(const T& msg, const __time_t& timeout_seconds){
        // 获取当前时间，并计算超时时间
        struct timespec abs_timeout;
        clock_gettime(CLOCK_REALTIME, &abs_timeout);
        abs_timeout.tv_sec += timeout_seconds;

        auto res=sem_timedwait(&(this->empty_semaphore),&(abs_timeout));
        if (-1==res && ETIMEDOUT==errno ) return false; //超时
        if (-1==res) throw std::runtime_error("error in MessageQueue::push"); //未知错误
        {
            std::lock_guard<std::mutex> lock(this->mtx); //获得队列的互斥锁
            this->que.emplace(msg);
        }
        sem_post(&(this->full_semaphore));
        return true;
    }
    void push(const T& msg){
        auto res=sem_wait(&(this->empty_semaphore));
        if (-1==res) throw std::runtime_error("error in MessageQueue::push"); //未知错误
        {
            std::lock_guard<std::mutex> lock(this->mtx); //获得队列的互斥锁
            this->que.emplace(msg);
        }
        sem_post(&(this->full_semaphore));
    }
    bool pull(T& msg, const __time_t& timeout_seconds){
        // 获取当前时间，并计算超时时间
        struct timespec abs_timeout;
        clock_gettime(CLOCK_REALTIME, &abs_timeout);
        abs_timeout.tv_sec += timeout_seconds;

        auto res=sem_timedwait(&(this->full_semaphore),&(abs_timeout));
        if (-1==res && ETIMEDOUT==errno ) return false; //超时
        if (-1==res) throw std::runtime_error("error in MessageQueue::pull"); //未知错误
        {
            std::lock_guard<std::mutex> lock(this->mtx); //获得队列的互斥锁
            msg=this->que.front();
            this->que.pop();
        }
        sem_post(&(this->empty_semaphore));
        return true;
    }
    void pull(T& msg){
        auto res=sem_wait(&(this->full_semaphore));
        if (-1==res) throw std::runtime_error("error in MessageQueue::pull"); //未知错误
        {
            std::lock_guard<std::mutex> lock(this->mtx); //获得队列的互斥锁
            msg=this->que.front();
            this->que.pop();
        }
        sem_post(&(this->empty_semaphore));
    }

private:
    std::queue<T> que;

    std::mutex mtx;
    sem_t empty_semaphore;
    sem_t full_semaphore;
};

/*------------Definition of ThreadPool--------------*/
class ThreadPool{
public:
    ThreadPool(const size_t& num=5):stop(false){
        for (size_t i=0;i<num;++i){
            this->workers.emplace_back(std::bind(&ThreadPool::workerThread,this));
        }
    }
    ~ThreadPool(){
        this->stop.store(true);
        for (std::thread& worker:this->workers){
            worker.join();
        }
    }

    template <typename Func, typename... Args>
    auto addTask(Func&& func, Args&&... args) -> std::future<decltype(func(args...))>{
        auto sp_task = std::make_shared<std::packaged_task<decltype(func(args...))()>>( //将函数包装为一个异步任务
            std::bind(std::forward<Func>(func), std::forward<Args>(args)...)  //将func与它所有参数绑定，得到一个签名为decltype(func(args...))()的可调用对象
        );

        std::future<decltype(func(args...))> result = sp_task->get_future(); //延时存放函数的返回结果

        this->sp_tasks.push(std::make_shared<std::function<void()>>([sp_task]() { (*sp_task)(); }));

        return result;
    }

private:
    void workerThread(){
        while(false==this->stop.load()){
            try{
                std::shared_ptr<std::function<void()>> sp_task;
                if (this->sp_tasks.pull(sp_task,5)) (*sp_task)(); //拉取任务并执行
            }
            catch(const std::exception& e){
                std::cerr << e.what() << " in ThreadPool::workerThread\n";
            }
        }
    }

private:
    std::vector<std::thread> workers;
    MessageQueue<std::shared_ptr<std::function<void()>>> sp_tasks;
    std::atomic_bool stop;
};

} // namespace utils



#endif // UTILS_H