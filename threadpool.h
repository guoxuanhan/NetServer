#pragma once
#ifndef THREADPOOL_H
#define THREADPOOL_H
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <functional>

/*
    ThreadPool类，简易线程池实现。
    表示worker线程，执行通用任务线程

    使用的同步原语：
    pthread_mutex_t mutex_l;//互斥锁
    pthread_cond_t condtion_l;//条件变量

    使用的系统调用：
    pthread_mutex_init();
    pthread_cond_init();
    pthread_create(&thread_[i],NULL,threadFunc,this);
    pthread_mutex_lock();
    pthread_mutex_unlock();
    pthread_cond_signal();
    pthread_cond_wait();
    pthread_cond_broadcast();
    pthread_join();
    pthread_mutex_destory();
    pthread_cond_destory();
*/
class ThreadPool
{
public:
    //线程池任务类型
    typedef std::function<void()> Task;
public:
    ThreadPool(int threadnum = 0);
    ~ThreadPool();
    //启动线程池
    void Start();
    //暂停线程池
    void Stop();
    //添加任务
    void AddTask(Task task);
    //线程池执行的函数
    void ThreadFunc();
    //获取线程数量
    int GetThreadNum();
private:
    //运行状态
    bool started_;
    //线程数量
    int threadnum_;
    //线程列表
    std::vector<std::thread*> threadlist_;
    //任务队列
    std::queue<Task> taskqueue_;
    //任务队列互斥锁
    std::mutex mutex_;
    //任务队列同步的条件变量
    std::condition_variable condition_;
};


#endif // THREADPOOL_H
