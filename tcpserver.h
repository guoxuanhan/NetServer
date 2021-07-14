#pragma once
#ifndef TCPSERVER_H
#define TCPSERVER_H
#include <functional>
#include <string>
#include <map>
#include <mutex>
#include "socket.h"
#include "channel.h"
#include "eventloop.h"
#include "tcpconnection.h"
#include "eventloopthreadpool.h"

#define MAXCONNECTION 20000

class TcpServer
{
public:
    //TcpConnection智能指针类型
    typedef std::shared_ptr<TcpConnection> spTcpConnection;

    //回调函数类型
    typedef std::function<void(const spTcpConnection&, std::string&)> MessageCallback;
    typedef std::function<void(const spTcpConnection&)> Callback;

    TcpServer(EventLoop* loop, const int port, const int threadnum = 0);
    ~TcpServer();

    //启动ＴＣＰ服务器
    void Start();

    //业务函数注册
    //注册新连接回调函数
    void SetNewConnCallback(const Callback &cb)
    {
        newconnectioncallback_ = cb;
    }

    //注册数据回调函数
    void SetMessageCallback(const MessageCallback &cb)
    {
        messagecallback_ = cb;
    }

    //注册数据发送完成回调函数
    void SetSendCompleteCallback(const Callback &cb)
    {
        sendcompletecallback_ = cb;
    }

    //注册连接关闭回调函数
    void SetCloseCallback(const Callback &cb)
    {
        closecallback_ = cb;
    }

    //注册连接异常回调函数
    void SetErrorCallback(const Callback &cb)
    {
        errorcallback_ = cb;
    }


private:
    //服务器socket
    Socket serversocket_;

    //主loop
    EventLoop *loop_;

    //服务器事件
    Channel serverchannel_;

    //连接数量统计
    int conncount_;

    //TCP连接表
    std::map<int, std::shared_ptr<TcpConnection>> tcpconnlist_;

    //保护TCP连接表的互斥量
    std::mutex mutex_;

    //IO线程池
    EventLoopThreadPool eventloopthreadpool;

    //业务接口回调函数
    Callback newconnectioncallback_;
    MessageCallback messagecallback_;
    Callback sendcompletecallback_;
    Callback closecallback_;
    Callback errorcallback_;

    //服务器对新连接连接处理的函数
    void OnNewConnection();

    //移除TCP连接函数
    void RemoveConnection(const std::shared_ptr<TcpConnection> sptcpconnection);

    //服务器socket的异常处理函数
    void OnConnectionError();
};


#endif // TCPSERVER_H
