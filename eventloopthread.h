#pragma once
#ifndef EVENTLOOPTHREAD_H
#define EVENTLOOPTHREAD_H
#include <istream>
#include <string>
#include <thread>
#include "eventloop.h"

/*
    EventLoopThread类：表示IO线程执行特定任务的，线程池的是通用任务线程
*/
class EventLoopThread
{
public:
    EventLoopThread();
    ~EventLoopThread();
    //获取当前线程运行的loop
    EventLoop* GetLoop();
    //启动线程
    void Start();
    //线程执行的函数
    void ThreadFunc();
private:
    //线程成员
    std::thread th_;
    //线程id
    std::thread::id threadid_;
    //线程名
    std::string threadname_;
    //线程运行的loop循环
    EventLoop* loop_;
};

#endif // EVENTLOOPTHREAD_H
