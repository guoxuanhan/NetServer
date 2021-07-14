#include <iostream>
#include <sys/epoll.h>
#include "channel.h"

Channel::Channel() :
    fd_(-1)
{

}

Channel::~Channel()
{

}

//设置文件描述符
void Channel::SetFd(int fd)
{
    fd_ = fd;
}

//获取文件描述符
int Channel::GetFd() const
{
    return fd_;
}
//设置触发事件
void Channel::SetEvents(uint32_t events)
{
    events_ = events;
}

//获取触发事件
uint32_t Channel::GetEvents() const
{
    return events_;
}
//事件分发处理
void Channel::HandleEvent()
{
    if(events_ & EPOLLRDHUP)
    {
        //对方异常关闭事件
        std::cout<< "Event EPOLLRDHUP!" << std::endl;
        closehandler_();
    } else if(events_ & (EPOLLIN | EPOLLPRI))
    {
        //读事件，对端有数据或者正常关闭
        std::cout<< "Event EPOLLIN!" << std::endl;
        readhandler_();
    } else if(events_ & EPOLLOUT)
    {
        //写事件
        std::cout << "Event EPOLLOUT!" << std::endl;
        writehandler_();
    } else
    {
        //连接错误
        std::cout << "Event EPOLLERR!" << std::endl;
        errorhandler_();
    }
}

//设置读事件回调
void Channel::SetReadHandle(const Callback& cb)
{
    //TODO: 提高效率，可以使用move语句，这里暂时还是存在一次拷贝
    readhandler_ = cb;
}

//设置写事件回调
void Channel::SetWriteHandle(const Callback& cb)
{
    writehandler_ = cb;
}

//设置错误事件回调
void Channel::SetErrorHandle(const Callback& cb)
{
    errorhandler_ = cb;
}

//设置关闭事件回调
void Channel::SetCloseHandle(const Callback& cb)
{
    closehandler_ = cb;
}
