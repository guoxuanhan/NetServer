#include <iostream>
#include <sys/eventfd.h>
#include <unistd.h>
#include <stdlib.h>
#include "eventloop.h"

//参照muduo，实现跨线程唤醒
int CreateEventFd()
{
    int evtfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if(evtfd < 0)
    {
        std::cout<< "Failed in eventfd!" << std::endl;
        exit(1);
    }
    return evtfd;
}

EventLoop::EventLoop() :
    functorlist_(), channellist_(), activechannellist_(), poller_(),
    quit_(true), tid_(std::this_thread::get_id()), mutex_(),
    wakeupfd_(CreateEventFd()), wakeupchannel_()
{
    wakeupchannel_.SetFd(wakeupfd_);
    wakeupchannel_.SetEvents(EPOLLIN | EPOLLET);
    wakeupchannel_.SetReadHandle(std::bind(&EventLoop::HandleRead, this));
    wakeupchannel_.SetErrorHandle(std::bind(&EventLoop::HandleError, this));
    AddChannelToPoller(&wakeupchannel_);
}

EventLoop::~EventLoop()
{
    close(wakeupfd_);
}

//执行事件循环
void EventLoop::loop()
{
    quit_ = false;
    while(!quit_)
    {
        poller_.poll(activechannellist_);
        for(Channel* pchannel : activechannellist_)
        {
            //处理事件
            pchannel->HandleEvent();
        }
        activechannellist_.clear();
        ExecuteTask();
    }
}

//添加事件
void EventLoop::AddChannelToPoller(Channel* pchannel)
{
    poller_.AddChannel(pchannel);
}

//移除事件
void EventLoop::RemoveChannelToPoller(Channel* pchannel)
{
    poller_.RemoveChannel(pchannel);
}

//修改事件
void EventLoop::UpdateChannelToPoller(Channel* pchannel)
{
    poller_.UpdateChannel(pchannel);
}

//退出事件循环
void EventLoop::Quit()
{
    quit_ = true;
}

//获取loop所在的线程id
std::thread::id EventLoop::GetThreadId() const
{
    return tid_;
}
//唤醒loop
void EventLoop::WakeUp()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupfd_, (char*)&one, sizeof(one));
}

//唤醒loop后的读回调
void EventLoop::HandleRead()
{
    uint64_t one = 1;
    ssize_t n = read(wakeupfd_, &one, sizeof(one));
}

//唤醒loop后的错误处理回调
void EventLoop::HandleError()
{

}

//向任务队列添加任务
void EventLoop::AddTask(Functor functor)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        functorlist_.push_back(functor);
    }
    //跨线程唤醒，worker线程唤醒IO线程
    WakeUp();
}

//执行任务队列的任务
void EventLoop::ExecuteTask()
{
    std::vector<Functor> functorlist;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        functorlist.swap(functorlist_);
    }
    for(Functor & functor : functorlist)
    {
        functor();
    }
    functorlist.clear();
}
