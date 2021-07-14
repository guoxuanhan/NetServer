#pragma once
#ifndef HTTPSERVER_H
#define HTTPSERVER_H
#include <string>
#include <mutex>
#include <map>
#include "eventloop.h"
#include "tcpserver.h"
#include "tcpconnection.h"
#include "httpsession.h"
#include "threadpool.h"
#include "timer.h"

class HttpServer
{
public:
    //tcp连接的智能指针类型
    typedef std::shared_ptr<TcpConnection> spTcpConnection;

    //定时器的智能指针类型
    typedef std::shared_ptr<Timer> spTimer;

    HttpServer(EventLoop *loop, const int port, const int iothreadnum, const int workerthreadnum);
    ~HttpServer();

    //启动Http服务器
    void Start();

private:
    //新连接回调函数
    void HandleNewConnection(const spTcpConnection& sptcpconn);

    //数据接收回调函数
    void HandleMessage(const spTcpConnection &sptcpconn, std::string &msg);

    //数据发送完成回调函数
    void HandleSendComplete(const spTcpConnection& sptcpconn);

    //连接关闭回调函数
    void HandleClose(const spTcpConnection& sptcpconn);

    //连接异常回调函数
    void HandleError(const spTcpConnection& sptcpconn);

    //bugfix:声明顺序调整，map、mutex放到最后析构
    //管理Http会话
    std::map<spTcpConnection, std::shared_ptr<HttpSession> > httpsessionnlist_;

    //管理定时器,维护活跃连接
    std::map<spTcpConnection, spTimer> timerlist_;

    //保护以上两个map的互斥量
    std::mutex mutex_;

    //tcp服务器
    TcpServer tcpserver_;

    //工作线程池
    ThreadPool threadpool_;
};

#endif // HTTPSERVER_H
