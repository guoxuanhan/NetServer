#pragma once
#ifndef TIMERMANAGER_H
#define TIMERMANAGER_H
#include <functional>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <thread>
#include "timer.h"

/*
    TimerManager类:定时器管理类，基于时间轮实现。
    增加删除O(1)，执行可能复杂度高些，slot多的话可以降低链表长度
*/
class TimerManager
{
public:
    typedef std::function<void()> CallBack;
public:
    //单例模式，懒汉
    static TimerManager* GetTimerManagerInstance();
    //添加定时任务
    void AddTimer(Timer* ptimer);
    //删除定时任务
    void RemoveTimer(Timer* ptimer);
    //调整定时任务
    void AdjustTimer(Timer* ptimer);
    //开启线程，定时器启动
    void Start();
    //暂停定时器
    void Stop();
public:
    //时间轮操作内部函数
    //检查超时任务
    void CheckTimer();
    //线程实际执行的函数
    void CheckTick();
    //计算定时器参数
    void CalculateTimer(Timer* ptimer);
    //添加定时器到时间轮中
    void AddTimerToTimeWheel(Timer* ptimer);
    //从时间轮中移除定时器
    void RemoveTimerFromTimeWheel(Timer* ptimer);
    //从时间轮中修改定时器
    void AdjustTimerToWheel(Timer* ptimer);
public:
    //垃圾回收，程序结束的时候析构TimerManager
    class GC
    {
        public:
            ~GC()
            {
                if(ptimermanager_ != nullptr)
                    delete ptimermanager_;
            }
    };
private:
    TimerManager();
    ~TimerManager();
private:
    static TimerManager *ptimermanager_;
    static std::mutex mutex_;
    static GC gc;
private:
    //时间轮相关成员
    //当前slot
    int currentslot;
    //每个slot的时间间隔,ms
    static const int slotinterval;
    //slot总数
    static const int slotnum;
    //时间轮结构
    std::vector<Timer*> timewheel;
    //时间轮互斥量
    std::mutex timewheelmutex_;
    //时间轮运行状态
    bool running_;
    //定时器线程
    std::thread th_;
};

#endif // TIMERMANAGER_H
