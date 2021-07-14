#include "tcpconnection.h"
#include <stdio.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>

#define BUFSIZE 4096

int recvn(int fd, std::string &bufferin);
int sendn(int fd, std::string &bufferout);

TcpConnection::TcpConnection(EventLoop *loop, int fd, const struct sockaddr_in &clientaddr)
    : loop_(loop),
    spchannel_(new Channel()),
    fd_(fd),
    clientaddr_(clientaddr),
    halfclose_(false),
    disconnected_(false),
    asyncprocessing_(false),
    bufferin_(),
    bufferout_()
{
    //注册事件执行函数
    spchannel_->SetFd(fd_);
    spchannel_->SetEvents(EPOLLIN | EPOLLET);
    spchannel_->SetReadHandle(std::bind(&TcpConnection::HandleRead, this));
    spchannel_->SetWriteHandle(std::bind(&TcpConnection::HandleWrite, this));
    spchannel_->SetCloseHandle(std::bind(&TcpConnection::HandleClose, this));
    spchannel_->SetErrorHandle(std::bind(&TcpConnection::HandleError, this));
}

TcpConnection::~TcpConnection()
{
    //多线程下，加入loop的任务队列？不用，因为已经在当前loop线程
    //移除事件,析构成员变量
    loop_->RemoveChannelToPoller(spchannel_.get());
    close(fd_);
}

void TcpConnection::AddChannelToLoop()
{
    //bug segement fault
    //https://blog.csdn.net/littlefang/article/details/37922113
    //多线程下，加入loop的任务队列
    //主线程直接执行
    //loop_->AddChannelToPoller(pchannel_);
    loop_->AddTask(std::bind(&EventLoop::AddChannelToPoller, loop_, spchannel_.get()));

}

void TcpConnection::Send(const std::string &s)
{
    //std::cout << "TcpConnection::Send" << std::endl;
    bufferout_ += s; //跨线程消息投递成功
    //判断当前线程是不是Loop IO线程
    if(loop_->GetThreadId() == std::this_thread::get_id())
    {
        SendInLoop();
    }
    else
    {
        asyncprocessing_ = false; //异步调用结束
        //std::cout << "asyncprocessing_" << std::endl;
        loop_->AddTask(std::bind(&TcpConnection::SendInLoop, shared_from_this()));//跨线程调用,加入IO线程的任务队列，唤醒
    }
}

void TcpConnection::SendInLoop()
{
    //bufferout_ += s;//copy一次
    if(disconnected_)
    {
        return;
    }
    int result = sendn(fd_, bufferout_);
    if(result > 0)
    {
        uint32_t events = spchannel_->GetEvents();
        if(bufferout_.size() > 0)
        {
            //缓冲区满了，数据没发完，就设置EPOLLOUT事件触发
            spchannel_->SetEvents(events | EPOLLOUT);
            loop_->UpdateChannelToPoller(spchannel_.get());
        }
        else
        {
            //数据已发完
            spchannel_->SetEvents(events & (~EPOLLOUT));
            sendcompletecallback_(shared_from_this());
            //20190217
            if(halfclose_)
                HandleClose();
        }
    }
    else if(result < 0)
    {
        HandleError();
    }
    else
    {
        HandleClose();
    }
}


void TcpConnection::Shutdown()
{
    if(loop_->GetThreadId() == std::this_thread::get_id())
    {
        ShutdownInLoop();
    }
    else
    {
        //不是IO线程，则是跨线程调用,加入IO线程的任务队列，唤醒
        loop_->AddTask(std::bind(&TcpConnection::ShutdownInLoop, shared_from_this()));
    }
}

void TcpConnection::ShutdownInLoop()
{
    if(disconnected_) {return;}
    std::cout << "shutdown" << std::endl;
    closecallback_(shared_from_this()); //应用层清理连接回调
    loop_->AddTask(std::bind(connectioncleanup_, shared_from_this())); //自己不能清理自己，交给loop执行，Tcpserver清理TcpConnection
    disconnected_ = true;
}

void TcpConnection::HandleRead()
{
    //接收数据，写入缓冲区
    int result = recvn(fd_, bufferin_);

    //业务回调,可以利用工作线程池处理，投递任务
    if(result > 0)
    {
        messagecallback_(shared_from_this(), bufferin_);//可以用右值引用优化
        //bufferin_.clear();
    }
    else if(result == 0)
    {
        HandleClose();
    }
    else
    {
        HandleError();
    }
}

void TcpConnection::HandleWrite()
{
    int result = sendn(fd_, bufferout_);
    if(result > 0)
    {
        uint32_t events = spchannel_->GetEvents();
        if(bufferout_.size() > 0)
        {
            //缓冲区满了，数据没发完，就设置EPOLLOUT事件触发
            spchannel_->SetEvents(events | EPOLLOUT);
            loop_->UpdateChannelToPoller(spchannel_.get());
        }
        else
        {
            //数据已发完
            spchannel_->SetEvents(events & (~EPOLLOUT));
            sendcompletecallback_(shared_from_this());
            //发送完毕，如果是半关闭状态，则可以close了
            if(halfclose_)
                HandleClose();
        }
    }
    else if(result < 0)
    {
        HandleError();
    }
    else
    {
        HandleClose();
    }
}


void TcpConnection::HandleError()
{
    if(disconnected_) {return;}
    errorcallback_(shared_from_this());
    //loop_->RemoveChannelToPoller(pchannel_);
    //连接标记为清理
    //task添加
    loop_->AddTask(std::bind(connectioncleanup_, shared_from_this())); //自己不能清理自己，交给loop执行，Tcpserver清理
    disconnected_ = true;
}

//对端关闭连接,有两种，一种close，另一种是shutdown(半关闭)，但服务器并不清楚是哪一种，只能按照最保险的方式来，即发完数据再close
void TcpConnection::HandleClose()
{
    //移除事件
    //loop_->RemoveChannelToPoller(pchannel_);
    //连接标记为清理
    //task添加
    //loop_->AddTask(connectioncleanup_);
    //closecallback_(this);

    //20190217 优雅关闭，发完数据再关闭
    if(disconnected_) {return;}
    if(bufferout_.size() > 0 || bufferin_.size() > 0 || asyncprocessing_)
    {
        //如果还有数据待发送，则先发完,设置半关闭标志位
        halfclose_ = true;
        //还有数据刚刚才收到，但同时又收到FIN
        if(bufferin_.size() > 0)
        {
            messagecallback_(shared_from_this(), bufferin_);
        }
    }
    else
    {
        loop_->AddTask(std::bind(connectioncleanup_, shared_from_this()));
        closecallback_(shared_from_this());
        disconnected_ = true;
    }
}

int recvn(int fd, std::string &bufferin)
{
    int nbyte = 0;
    int readsum = 0;
    char buffer[BUFSIZE];
    for(;;)
    {
        //nbyte = recv(fd, buffer, BUFSIZE, 0);
        nbyte = read(fd, buffer, BUFSIZE);

        if (nbyte > 0)
        {
            bufferin.append(buffer, nbyte);//效率较低，2次拷贝
            readsum += nbyte;
            if(nbyte < BUFSIZE)
                return readsum;//读优化，减小一次读调用，因为一次调用耗时10+us
            else
                continue;
        }
        else if (nbyte < 0)//异常
        {
            if (errno == EAGAIN)//系统缓冲区未有数据，非阻塞返回
            {
                //std::cout << "EAGAIN,系统缓冲区未有数据，非阻塞返回" << std::endl;
                return readsum;
            }
            else if (errno == EINTR)
            {
                std::cout << "errno == EINTR" << std::endl;
                continue;
            }
            else
            {
                //可能是RST
                perror("recv error");
                //std::cout << "recv error" << std::endl;
                return -1;
            }
        }
        else//返回0，客户端关闭socket，FIN
        {
            //std::cout << "client close the Socket" << std::endl;
            return 0;
        }
    }
}

int sendn(int fd, std::string &bufferout)
{
    ssize_t nbyte = 0;
    int sendsum = 0;
    //char buffer[BUFSIZE+1];
    size_t length = 0;
    //length = bufferout.copy(buffer, BUFSIZE, 0);
    //buffer[length] = '\0';
    // if(bufferout.size() >= BUFSIZE)
    // {
    // 	length =  BUFSIZE;
    // }
    // else
    // {
    // 	length =  bufferout.size();
    // }
    //无拷贝优化
    length = bufferout.size();
    if(length >= BUFSIZE)
    {
        length = BUFSIZE;
    }
    for (;;)
    {
        //nbyte = send(fd, buffer, length, 0);
        //nbyte = send(fd, bufferout.c_str(), length, 0);
        nbyte = write(fd, bufferout.c_str(), length);
        if (nbyte > 0)
        {
            sendsum += nbyte;
            bufferout.erase(0, nbyte);
            //length = bufferout.copy(buffer, BUFSIZE, 0);
            //buffer[length] = '\0';
            length = bufferout.size();
            if(length >= BUFSIZE)
            {
                length = BUFSIZE;
            }
            if (length == 0 )
            {
                return sendsum;
            }
        }
        else if (nbyte < 0)//异常
        {
            if (errno == EAGAIN)//系统缓冲区满，非阻塞返回
            {
                std::cout << "write errno == EAGAIN,not finish!" << std::endl;
                return sendsum;
            }
            else if (errno == EINTR)
            {
                std::cout << "write errno == EINTR" << std::endl;
                continue;
            }
            else if (errno == EPIPE)
            {
                //客户端已经close，并发了RST，继续wirte会报EPIPE，返回0，表示close
                perror("write error");
                std::cout << "write errno == client send RST" << std::endl;
                return -1;
            }
            else
            {
                perror("write error");//Connection reset by peer
                std::cout << "write error, unknow error" << std::endl;
                return -1;
            }
        }
        else//返回0
        {
            //应该不会返回0
            //std::cout << "client close the Socket!" << std::endl;
            return 0;
        }
    }
}
