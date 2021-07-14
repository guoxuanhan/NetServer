#pragma once
#ifndef _MY_SOCKET_H
#define _MY_SOCKET_H
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/*
    服务器socket类：封装socket描述符及相关初始化操作
*/
class Socket
{
public:
    Socket(/* args */);
    ~Socket();
    //获取fd
    int fd() const { return serverfd_; }
    //socket设置
    void SetSocketOption();
    //设置地址重用
    void SetReuseAddr();
    //设置非阻塞
    void Setnonblocking();
    //绑定地址
    bool BindAddress(int serverport);
    //开启监听
    bool Listen();
    //accept获取连接
    int Accept(struct sockaddr_in &clientaddr);
    //关闭服务器fd
    bool Close();
private:
    //服务器socket文件描述符
    int serverfd_;
};

#endif // SOCKET_H
