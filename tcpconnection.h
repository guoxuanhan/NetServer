#pragma once
#ifndef TCPCONNECTION_H
#define TCPCONNECTION_H
#include <functional>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <memory>
#include "channel.h"
#include "eventloop.h"

/*
    TcpConnection类：客户端连接的抽象表示
*/
class TcpConnection : public std::enable_shared_from_this<TcpConnection>
{
public:
    //TcpConnection智能指针
    typedef std::shared_ptr<TcpConnection> spTcpConnection;

    //回调函数类型
    typedef std::function<void(const spTcpConnection&)> Callback;
    typedef std::function<void(const spTcpConnection&, std::string&)> MessageCallback;
    //typedef std::function<void()> TaskCallback;.

    TcpConnection(EventLoop *loop, int fd, const struct sockaddr_in &clientaddr);
    ~TcpConnection();

    //获取当前连接的fd
    int fd() const
    { return fd_; }

    //获取当前连接所属的loop
    EventLoop* GetLoop() const { return loop_; }

    //添加本连接对应的事件到loop
    void AddChannelToLoop();

    //发送数据的函数
    void Send(const std::string &s);

    //在当前IO线程发送数据函数
    void SendInLoop();

    //主动清理连接
    void Shutdown();

    //在当前IO线程清理连接函数
    void ShutdownInLoop();

    //可读事件回调
    void HandleRead();
    //可写事件回调
    void HandleWrite();
    //错误事件回调
    void HandleError();
    //连接关闭事件回调
    void HandleClose();

    //设置收到数据回调函数
    void SetMessaeCallback(const MessageCallback &cb)
    {
        messagecallback_ = cb;
    }

    //设置发送完数据的回调函数
    void SetSendCompleteCallback(const Callback &cb)
    {
        sendcompletecallback_ = cb;
    }

    //设置连接关闭的回调函数
    void SetCloseCallback(const Callback &cb)
    {
        closecallback_ = cb;
    }

    //设置连接异常的回调函数
    void SetErrorCallback(const Callback &cb)
    {
        errorcallback_ = cb;
    }

    //设置连接清理函数
    void SetConnectionCleanUp(const Callback &cb)
    {
        connectioncleanup_ = cb;
    }

    //设置异步处理标志，开启工作线程池的时候使用
    void SetAsyncProcessing(const bool asyncprocessing)
    {
        asyncprocessing_ = asyncprocessing;
    }

private:
    //当前连接所在的loop
    EventLoop *loop_;

    //当前连接的事件
    std::unique_ptr<Channel> spchannel_;

    //文件描述符
    int fd_;

    //对端地址
    struct sockaddr_in clientaddr_;

    //半关闭标志位
    bool halfclose_;

    //连接已关闭标志位
    bool disconnected_;

    //异步调用标志位,当工作任务交给线程池时，置为true，任务完成回调时置为false
    bool asyncprocessing_;

    //读写缓冲
    std::string bufferin_;
    std::string bufferout_;

    //各种回调函数
    MessageCallback messagecallback_;
    Callback sendcompletecallback_;
    Callback closecallback_;
    Callback errorcallback_;
    Callback connectioncleanup_;
};

#endif // TCPCONNECTION_H
