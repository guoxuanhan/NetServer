#include <iostream>
#include <stdio.h>//perror
#include <stdlib.h>//exit
#include <unistd.h>//close
#include <errno.h>
#include "poller.h"

#define EVENTNUM    4096   //最大触发事件数量
#define TIMEOUT     1000   //epoll_wait超时时间设置


Poller::Poller() :
    pollfd_(-1), eventlist_(EVENTNUM), channelmap_(), mutex_()
{
    pollfd_ = epoll_create(256);
    if(pollfd_ == -1)
    {
        perror("epoll_create error!");
        exit(1);
    }
    std::cout<< "epoll_create: " << pollfd_ << std::endl;
}

Poller::~Poller()
{
    close(pollfd_);
}

//等待IO事件， epoll_wait封装
void Poller::poll(ChannelList& activechannellist)
{
    int timeout = TIMEOUT;
    int nfds = epoll_wait(pollfd_, &*eventlist_.begin(), (int)eventlist_.capacity(), timeout);
    if(nfds == -1)
    {
        perror("epoll wait error!");
    }
    //std::cout<< "event num: " << nfds << std::endl;
    for(int i = 0; i < nfds; i++)
    {
        int events = eventlist_[i].events;
        Channel* pchannel = (Channel*)eventlist_[i].data.ptr;
        int fd = pchannel->GetFd();

        std::map<int, Channel*>::const_iterator iter;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            iter = channelmap_.find(fd);
        }
        if(iter != channelmap_.end())
        {
            pchannel->SetEvents(events);
            activechannellist.push_back(pchannel);
        } else
        {
            std::cout<< "not find channel!" << std::endl;
        }
    }
    if(nfds == (int)eventlist_.capacity())
    {
        std::cout<< "resize:" << nfds << std::endl;
        eventlist_.resize(nfds * 2);
    }
}

//添加事件，EPOLL_CTL_ADD
void Poller::AddChannel(Channel* pchannel)
{
    int fd = pchannel->GetFd();
    struct epoll_event ev;
    ev.events = pchannel->GetEvents();
    //data是联合体
    //ev.data.fd == fd
    ev.data.ptr = pchannel;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        channelmap_[fd] = pchannel;
    }

    if(epoll_ctl(pollfd_, EPOLL_CTL_ADD, fd, &ev) == -1)
    {
        perror("epoll add error!");
        exit(1);
    }
    std::cout<< "Add Channel!" << std::endl;
}

//移除事件，EPOLL_CTL_DEL
void Poller::RemoveChannel(Channel* pchannel)
{
    int fd = pchannel->GetFd();
    struct epoll_event ev;
    ev.events = pchannel->GetEvents();
    ev.data.ptr = pchannel;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        channelmap_.erase(fd);
    }

    if(epoll_ctl(pollfd_, EPOLL_CTL_DEL, fd, &ev) == -1)
    {
        perror("epoll del error!");
        exit(1);
    }
    std::cout<< "Remove Channel!" << std::endl;
}

//修改事件，EPOLL_CTL_MOD
void Poller::UpdateChannel(Channel* pchannel)
{
    int fd = pchannel->GetFd();
    struct epoll_event ev;
    ev.events = pchannel->GetEvents();
    ev.data.ptr = pchannel;

    if(epoll_ctl(pollfd_, EPOLL_CTL_MOD, fd, &ev) == -1)
    {
        perror("epoll update error!");
        exit(1);
    }
    std::cout<< "Update Channel!" << std::endl;
}
