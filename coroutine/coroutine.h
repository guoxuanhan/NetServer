// ref: ucontext-人人都可以实现的简单协程库
// link: https://blog.csdn.net/qq910894904/article/details/41911175
//
// 没有采用共享栈的方式，采用每个协程一个私有栈，消耗空间大，但是没有栈内存拷贝过程
// 共享栈的方式可以看云风的实现
//

/*
typedef struct ucontext
  {
    unsigned long int uc_flags;
    struct ucontext *uc_link;
    stack_t uc_stack;
    mcontext_t uc_mcontext;
    __sigset_t uc_sigmask;
    struct _libc_fpstate __fpregs_mem;
  } ucontext_t;

typedef struct sigaltstack
  {
    void *ss_sp;
    int ss_flags;
    size_t ss_size;
  } stack_t;

4个函数：
函数执行控制权转移到ucp
int setcontext(const ucontext_t *ucp);

将当前上下文保存到中ucp
int getcontext(ucontext_t *ucp);

修改上下文入口函数 用于将一个新函数和堆栈，绑定到指定context中
void makecontext(ucontext_t *ucp, void (*func)(), int argc, ...);

保存上下文到oucp 切换到ucp
int swapcontext(ucontext_t *oucp, ucontext_t *ucp);
*/

#include <ucontext.h>
#include <unistd.h>

#include <vector>
#include <functional>

//协程栈空间大小 8KB
#define DEFAULT_STACK_SIZE 1024*8

//协程任务类
typedef std::function<void()> Task;

//协程运行状态，FREE：执行完毕，RUNABLE：就绪，RUNNING：运行，SUSPEND：挂起
enum ThreadState {FREE, RUNNABLE, RUNNING, SUSPEND};

struct schedule_t;

struct uthread_t {
public:
  schedule_t *psche; //协程所属的调度器
	ucontext_t ctx; //协程上下文数据结构
  Task task; //协程任务，包装用户的函数
	enum ThreadState state; //FREE, RUNNABLE, RUNNING, SUSPEND
	char stack[DEFAULT_STACK_SIZE]; //协程私有栈空间

  //协程要执行的函数
  void cofunc() {
    task();
  }   

};

//协程数组类别
typedef std::vector<uthread_t> UThread_vector;
// 协程调度器
struct schedule_t {
private:
	ucontext_t mainctx; //main的上下文
	int running_thread; //正在运行的协程编号
	UThread_vector threads; //协程数组

public:
  schedule_t() : running_thread(-1) {}
  ~schedule_t() {}

  //创建协程，func是执行函数，加入协程数组中,暂时只创建不执行
  int uthread_create(Task task);

  //挂起正在执行的协程，切换到main
  void uthread_yield();

  //恢复编号id的协程
  void uthread_resume(int id);

  //判断所有协程是否执行完毕
  int schedule_finished() const;

  //协程函数入口
  static void uthread_func(void *arg);

};
