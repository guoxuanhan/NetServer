#include <sstream>
#include "eventloopthread.h"

EventLoopThread::EventLoopThread() :
    th_(), threadid_(-1), threadname_("IO Thread"), loop_(NULL)
{

}

EventLoopThread::~EventLoopThread()
{
    //线程结束时清理
    std::cout<< "Clean up the EventLoopThread id:" << std::this_thread::get_id() << std::endl;
    //停止IO线程运行
    loop_->Quit();
    //清理IO线程防止内存泄漏，因为pthread_create会calloc
    th_.join();
}

//获取当前线程运行的loop
EventLoop* EventLoopThread::GetLoop()
{
    return loop_;
}

//启动线程
void EventLoopThread::Start()
{
    //create thread
    th_ = std::thread(&EventLoopThread::ThreadFunc, this);
}

//线程执行的函数
void EventLoopThread::ThreadFunc()
{
    EventLoop loop;
    loop_ = &loop;

    threadid_ = std::this_thread::get_id();
    std::stringstream sin;
    sin << threadid_;
    threadname_ += sin.str();
    std::cout<< "in the thread: " << threadname_ << std::endl;

    try
    {
        loop_->loop();
    } catch(std::bad_alloc& bad)
    {
        std::cerr << "Bad_alloc caught in EventLoopThread::ThreadFunc loop: " << bad.what() << std::endl;
    }
}
