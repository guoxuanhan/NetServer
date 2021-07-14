#pragma once
#ifndef POLLER_H
#define POLLER_H
#include <vector>
#include <mutex>
#include <map>
#include <sys/epoll.h>
#include "channel.h"

/*
    Poller类：对epoll的封装
*/
class Poller
{
public:
    //事件指针数组类型
    typedef std::vector<Channel*> ChannelList;
public:
    Poller();
    ~Poller();
    //等待IO事件， epoll_wait封装
    void poll(ChannelList& activechannellist);
    //添加事件，EPOLL_CTL_ADD
    void AddChannel(Channel* pchannel);
    //移除事件，EPOLL_CTL_DEL
    void RemoveChannel(Channel* pchannel);
    //修改事件，EPOLL_CTL_MOD
    void UpdateChannel(Channel* pchannel);
public:
    //epoll实例
    int pollfd_;
    //events数组，用于传递给epollwait
    std::vector<struct epoll_event> eventlist_;
    //事件表
    std::map<int, Channel*> channelmap_;
    //保护事件表的互斥量
    std::mutex mutex_;
};

#endif // POLLER_H
