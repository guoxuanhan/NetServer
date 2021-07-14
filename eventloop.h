#pragma once
#ifndef EVENTLOOP_H
#define EVENTLOOP_H
#include <iostream>
#include <functional>
#include <vector>
#include <thread>
#include <mutex>
#include "poller.h"
#include "channel.h"

class EventLoop
{
public:
    //任务类型
    typedef std::function<void()> Functor;
    //事件列表类型
    typedef std::vector<Channel*> ChannelList;
public:
    EventLoop();
    ~EventLoop();
    //执行事件循环
    void loop();
    //添加事件
    void AddChannelToPoller(Channel* pchannel);
    //移除事件
    void RemoveChannelToPoller(Channel* pchannel);
    //修改事件
    void UpdateChannelToPoller(Channel* pchannel);
    //退出事件循环
    void Quit();
    //获取loop所在的线程id
    std::thread::id GetThreadId() const;
    //唤醒loop
    void WakeUp();
    //唤醒loop后的读回调
    void HandleRead();
    //唤醒loop后的错误处理回调
    void HandleError();
    //向任务队列添加任务
    void AddTask(Functor functor);
    //执行任务队列的任务
    void ExecuteTask();
private:
    //任务队列
    std::vector<Functor> functorlist_;
    //所有事件，暂时不用
    ChannelList channellist_;
    //活跃事件
    ChannelList activechannellist_;
    //epoll操作封装
    Poller poller_;
    //运行状态
    bool quit_;
    //loop所在的线程id
    std::thread::id tid_;
    //保护任务队列的互斥量
    std::mutex mutex_;
    //跨线程唤醒fd
    int wakeupfd_;
    //用于唤醒当前loop的事件
    Channel wakeupchannel_;
};

#endif // EVENTLOOP_H
