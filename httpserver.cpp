#include "httpserver.h"
#include <iostream>
#include <functional>
#include <memory>
#include "timermanager.h"

HttpServer::HttpServer(EventLoop *loop, const int port, const int iothreadnum, const int workerthreadnum)
    : tcpserver_(loop, port, iothreadnum),
    threadpool_(workerthreadnum)
{
    tcpserver_.SetNewConnCallback(std::bind(&HttpServer::HandleNewConnection, this, std::placeholders::_1));
    tcpserver_.SetMessageCallback(std::bind(&HttpServer::HandleMessage, this, std::placeholders::_1, std::placeholders::_2));
    tcpserver_.SetSendCompleteCallback(std::bind(&HttpServer::HandleSendComplete, this, std::placeholders::_1));
    tcpserver_.SetCloseCallback(std::bind(&HttpServer::HandleClose, this, std::placeholders::_1));
    tcpserver_.SetErrorCallback(std::bind(&HttpServer::HandleError, this, std::placeholders::_1));

    threadpool_.Start(); //工作线程池启动
    TimerManager::GetTimerManagerInstance()->Start(); //HttpServer定时器管理器启动
}

HttpServer::~HttpServer()
{

}

void HttpServer::HandleNewConnection(const spTcpConnection& sptcpconn)
{
    //std::string msg(s);
    std::shared_ptr<HttpSession> sphttpsession = std::make_shared<HttpSession>(); //创建应用层会话
    spTimer sptimer = std::make_shared<Timer>(5000, Timer::TimerType::TIMER_ONCE, std::bind(&TcpConnection::Shutdown, sptcpconn));
    sptimer->Start();
    //可以优化成无锁，放入conn里面就行
    {
        std::lock_guard <std::mutex> lock(mutex_);
        httpsessionnlist_[sptcpconn] = sphttpsession;
        timerlist_[sptcpconn] = sptimer;
    }
}

void HttpServer::HandleMessage(const spTcpConnection& sptcpconn, std::string &msg)
{
    std::shared_ptr<HttpSession> sphttpsession;
    spTimer sptimer;
    {
        std::lock_guard <std::mutex> lock(mutex_);
        sphttpsession = httpsessionnlist_[sptcpconn];
        sptimer = timerlist_[sptcpconn];
    }
    sptimer->Adjust(5000, Timer::TimerType::TIMER_ONCE, std::bind(&TcpConnection::Shutdown, sptcpconn));

    if(threadpool_.GetThreadNum() > 0)
    {
        //Bug20190320：多线程下，对象的声明周期管理问题，就像在这里，向线程池传入了phttpsession和ptcpconn，怎么知道其指向对象是否已经析构了呢？
        //所以，需要采用智能指针来管理对象,替换原始指针
        //线程池处理业务，处理完后投递回本IO线程执行send
        //std::cout << "threadpool_.AddTask" << std::endl;
        HttpRequestContext httprequestcontext;
        std::string responsecontext;
        bool result = sphttpsession->PraseHttpRequest(msg, httprequestcontext);
        if(result == false)
        {
            sphttpsession->HttpError(400, "Bad request", httprequestcontext, responsecontext); //请求报文解析错误，报400
            sptcpconn->Send(responsecontext);
            return;
        }

        sptcpconn->SetAsyncProcessing(true);
        threadpool_.AddTask([=]() {
            //HttpRequestContext req = httprequestcontext;
            std::string responsemsg;
            sphttpsession->HttpProcess(httprequestcontext, responsemsg);

            sptcpconn->Send(responsemsg); //任务已经处理完成，执行跨线程调度，即回调

            if(!sphttpsession->KeepAlive())
            {
                //短连接，可以告诉框架层数据发完就可以关掉TCP连接，不过这里注释掉，还是交给客户端主动关闭吧
                //sptcpconn->HandleClose();
            }
        });
    }
    else
    {
        //没有开启业务线程池，业务计算直接在IO线程执行
        HttpRequestContext httprequestcontext;
        std::string responsecontext;
        bool result = sphttpsession->PraseHttpRequest(msg, httprequestcontext);
        if(result == false)
        {
            sphttpsession->HttpError(400, "Bad request", httprequestcontext, responsecontext); //请求报文解析错误，报400
            sptcpconn->Send(responsecontext);
            return;
        }

        sphttpsession->HttpProcess(httprequestcontext, responsecontext);

        sptcpconn->Send(responsecontext);

        if(!sphttpsession->KeepAlive())
        {
            //短连接，可以告诉框架层数据发完就可以关掉TCP连接，不过这里注释掉，还是交给客户端主动关闭吧
            //sptcpconn->HandleClose();
        }
    }
}

void HttpServer::HandleSendComplete(const spTcpConnection& sptcpconn)
{

}

void HttpServer::HandleClose(const spTcpConnection& sptcpconn)
{
    {
        std::lock_guard <std::mutex> lock(mutex_);
        httpsessionnlist_.erase(sptcpconn);
        timerlist_.erase(sptcpconn);
    }
}

void HttpServer::HandleError(const spTcpConnection& sptcpconn)
{
    {
        std::lock_guard <std::mutex> lock(mutex_);
        httpsessionnlist_.erase(sptcpconn);
        timerlist_.erase(sptcpconn);
    }
}

void HttpServer::Start()
{
    tcpserver_.Start();
}
