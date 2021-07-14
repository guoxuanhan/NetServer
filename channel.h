#pragma once
#ifndef CHANNEL_H
#define CHANNEL_H
#include <functional>

/*
    Channel类：fd和事件的封装。表示每一个客户端连接的通道

    EPOLLIN：文件描述符可读（包括对端socket正常关闭）
    EPOLLOUT：文件描述符可写
    EPOLLPRI：文件描述符由紧急的数据可读
    EPOLLERR：文件描述符发生错误
    EPOLLHUP：文件描述符被挂断
    EPOLLET：将epoll设置为边缘触发（edge triggered）模式，这是相对于水平触发（level triggered）来说的
    EPOLLONESHOTL：只监听一次事件，当监听完这次事件后，如果还需要继续监听这个socket，需要再次把这个socket加入到epoll队列
*/
class Channel
{
public:
    //回调函数类型
    typedef std::function<void()> Callback;
public:
    Channel();
    ~Channel();
    //设置文件描述符
    void SetFd(int fd);
    //获取文件描述符
    int GetFd() const;
    //设置触发事件
    void SetEvents(uint32_t events);
    //获取触发事件
    uint32_t GetEvents() const;
    //事件分发处理
    void HandleEvent();
    //设置读事件回调
    void SetReadHandle(const Callback& cb);
    //设置写事件回调
    void SetWriteHandle(const Callback& cb);
    //设置错误事件回调
    void SetErrorHandle(const Callback& cb);
    //设置关闭事件回调
    void SetCloseHandle(const Callback& cb);
private:
    //文件描述符
    int fd_;
    //事件，一般情况下为epoll events
    uint32_t events_;
    //事件触发时执行的函数，在tcp conn中注册
    Callback readhandler_;
    Callback writehandler_;
    Callback errorhandler_;
    Callback closehandler_;
};

#endif // CHANNEL_H
