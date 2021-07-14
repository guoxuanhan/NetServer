#pragma once
#ifndef ECHOSERVER_H
#define ECHOSERVER_H
#include <string>
#include "tcpserver.h"
#include "eventloop.h"
#include "tcpconnection.h"
#include "timer.h"

/*
    EchoServer类：回声服务器
*/
class EchoServer
{
public:
    typedef std::shared_ptr<TcpConnection> spTcpConnection;
    typedef std::shared_ptr<Timer> spTimer;

    EchoServer(EventLoop* loop, const int port, const int threadnum);
    ~EchoServer();

    //启动服务
    void Start();

private:
    /* data */
    //业务函数
    void HandleNewConnection(const spTcpConnection& sptcpconn);
    void HandleMessage(const spTcpConnection &sptcpconn, std::string &s);
    void HandleSendComplete(const spTcpConnection& sptcpconn);
    void HandleClose(const spTcpConnection& sptcpconn);
    void HandleError(const spTcpConnection& sptcpconn);

    TcpServer tcpserver_;
};

#endif // ECHOSERVER_H
