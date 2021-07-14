#include "tcpserver.h"
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <memory>

void Setnonblocking(int fd);

TcpServer::TcpServer(EventLoop* loop, const int port, const int threadnum)
    : serversocket_(),
    loop_(loop),
    serverchannel_(),
    conncount_(0),
    eventloopthreadpool(loop, threadnum)
{
    //serversocket_.SetSocketOption();
    serversocket_.SetReuseAddr();
    serversocket_.BindAddress(port);
    serversocket_.Listen();
    serversocket_.Setnonblocking();

    serverchannel_.SetFd(serversocket_.fd());
    serverchannel_.SetReadHandle(std::bind(&TcpServer::OnNewConnection, this));
    serverchannel_.SetErrorHandle(std::bind(&TcpServer::OnConnectionError, this));

}

TcpServer::~TcpServer()
{

}

void TcpServer::Start()
{
    eventloopthreadpool.Start();

    serverchannel_.SetEvents(EPOLLIN | EPOLLET);
    loop_->AddChannelToPoller(&serverchannel_);
}

//新TCP连接处理，核心功能，业务功能注册，任务分发
void TcpServer::OnNewConnection()
{
    //循环调用accept，获取所有的建立好连接的客户端fd
    struct sockaddr_in clientaddr;
    int clientfd;
    while( (clientfd = serversocket_.Accept(clientaddr)) > 0)
    {
        std::cout << "New client from IP:" << inet_ntoa(clientaddr.sin_addr)
            << ":" << ntohs(clientaddr.sin_port) << std::endl;

        if(++conncount_ >= MAXCONNECTION)
        {
            close(clientfd);
            continue;
        }
        Setnonblocking(clientfd);

        //选择IO线程loop
        EventLoop *loop = eventloopthreadpool.GetNextLoop();

        //创建连接，注册业务函数
        std::shared_ptr<TcpConnection> sptcpconnection = std::make_shared<TcpConnection>(loop, clientfd, clientaddr);
        sptcpconnection->SetMessaeCallback(messagecallback_);
        sptcpconnection->SetSendCompleteCallback(sendcompletecallback_);
        sptcpconnection->SetCloseCallback(closecallback_);
        sptcpconnection->SetErrorCallback(errorcallback_);
        sptcpconnection->SetConnectionCleanUp(std::bind(&TcpServer::RemoveConnection, this, std::placeholders::_1));
        {
            std::lock_guard<std::mutex> lock(mutex_);
            tcpconnlist_[clientfd] = sptcpconnection;
        }


        newconnectioncallback_(sptcpconnection);
        //Bug，应该把事件添加的操作放到最后,否则bug segement fault,导致HandleMessage中的phttpsession==NULL
        //总之就是做好一切准备工作再添加事件到epoll！！！
        sptcpconnection->AddChannelToLoop();
    }
}

//连接清理,bugfix:这里应该由主loop来执行，投递回主线程删除 OR 多线程加锁删除
void TcpServer::RemoveConnection(std::shared_ptr<TcpConnection> sptcpconnection)
{
    std::lock_guard<std::mutex> lock(mutex_);
    --conncount_;
    //std::cout << "clean up connection, conncount is" << conncount_ << std::endl;
    tcpconnlist_.erase(sptcpconnection->fd());
}

void TcpServer::OnConnectionError()
{
    std::cout << "UNKNOWN EVENT" << std::endl;
    serversocket_.Close();
}

void Setnonblocking(int fd)
{
    int opts = fcntl(fd, F_GETFL);
    if (opts < 0)
    {
        perror("fcntl(fd,GETFL)");
        exit(1);
    }
    if (fcntl(fd, F_SETFL, opts | O_NONBLOCK) < 0)
    {
        perror("fcntl(fd,SETFL,opts)");
        exit(1);
    }
}
