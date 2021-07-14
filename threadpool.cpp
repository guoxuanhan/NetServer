#include "threadpool.h"
#include <deque>
#include <unistd.h>

ThreadPool::ThreadPool(int threadnum) :
    started_(false), threadnum_(threadnum), threadlist_(), taskqueue_(),
    mutex_(), condition_()
{

}

ThreadPool::~ThreadPool()
{
    std::cout<< "Clean up the ThreadPool!" << std::endl;
    Stop();
    for(int i = 0; i < threadnum_; i++)
    {
        threadlist_[i]->join();
    }
    for(int i = 0; i < threadnum_; i++)
    {
        delete threadlist_[i];
    }
    threadlist_.clear();
}

//启动线程池
void ThreadPool::Start()
{
    if(threadnum_ > 0)
    {
        started_ = true;
        for(int i = 0; i < threadnum_; i++)
        {
            std::thread *pthread = new std::thread(&ThreadPool::ThreadFunc, this);
            threadlist_.push_back(pthread);
        }
    } else
    {
        std::cout<< "ThreadPool: threadnum < 0!" << std::endl;
    }
}

//暂停线程池
void ThreadPool::Stop()
{
    started_ = false;
    condition_.notify_all();
}

//添加任务
void ThreadPool::AddTask(Task task)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        taskqueue_.push(task);
    }
    //依次唤醒等待队列的线程
    condition_.notify_one();
}

//线程池执行的函数
void ThreadPool::ThreadFunc()
{
    std::thread::id tid = std::this_thread::get_id();
    std::stringstream sin;
    sin << tid;
    std::cout<< "Worker thread is running! tid: " << tid << std::endl;
    Task task;
    while(started_)
    {
        task = NULL;
        {
            std::unique_lock<std::mutex> lock(mutex_);//unique_lock支持解锁又上锁的情况
            while(taskqueue_.empty() && started_)
            {
                condition_.wait(lock);
            }
            if(!started_)
            {
                break;
            }
            std::cout<< "Thread wake up! tid: " << tid << std::endl;
            std::cout<< "Taskqueue size: " << taskqueue_.size() << std::endl;
            task = taskqueue_.front();
            taskqueue_.pop();
        }
        if(task)
        {
            try
            {
                task();//task()中IO过程可以使用协程优化，让出CPU资源
            } catch(std::bad_alloc& bad)
            {
                std::cerr << "Bad_alloc caught in ThreadPool::ThreadFunc task: " << bad.what() << std::endl;
                while(true);
            }
        }
    }
}

//获取线程数量
int ThreadPool::GetThreadNum()
{
    return threadnum_;
}
