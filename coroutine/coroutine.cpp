// reference: ucontext-人人都可以实现的简单协程库
// link: https://blog.csdn.net/qq910894904/article/details/41911175
//
#include "./coroutine.h"
#include <iostream>

using namespace std;

int schedule_t::uthread_create(Task task) {
	int i = 0;
	for(i = 0; i < threads.size(); ++i) {
		if(threads[i].state == FREE) {
			break;
		}
	}

	if(i == threads.size())
		threads.push_back(uthread_t());
	
	uthread_t *puth = &threads[i];
    puth->psche = this;
	puth->state = RUNNABLE;
    puth->task = std::move(task);

	getcontext(&puth->ctx);
	puth->ctx.uc_stack.ss_sp = puth->stack;
	puth->ctx.uc_stack.ss_size = DEFAULT_STACK_SIZE;
	puth->ctx.uc_stack.ss_flags = 0;
	puth->ctx.uc_link = &mainctx;

	//回调形式的函数，必须传参数this，且为static
	makecontext(&puth->ctx, (void (*)(void))(&schedule_t::uthread_func), 1, this);
	
	//running_thread = i;
	//swapcontext(&mainctx, &puth->ctx);
	
	return i;
}


void schedule_t::uthread_yield() {
	if(running_thread != -1) {
		uthread_t *puth = &threads[running_thread];
		puth->state = SUSPEND;
		running_thread = -1;

		//保存当前协程的上下文，之后恢复后继续执行
		swapcontext(&puth->ctx, &mainctx);
	}
}


void schedule_t::uthread_resume(int id) {
	if(id < 0 || id >= threads.size()) {
		return ;
	}
	if(id == running_thread) {
		return ;
	}
	
	switch (threads[id].state)
	{
		case SUSPEND:
			running_thread = id;
			threads[id].state = RUNNING;
			swapcontext(&mainctx, &threads[id].ctx);
			break;
		case RUNNABLE:
			//创建协程时没有立即执行，可以用这里来执行
			running_thread = id;
			swapcontext(&mainctx, &threads[id].ctx);
		default:
			break;
	}
}


int schedule_t::schedule_finished() const {
	if(running_thread != -1) {
		return 0;
	} else {
		for(int i = 0; i < threads.size(); ++i) {
			if(threads[i].state != FREE) {
				return 0;
			}
		}
	}
	return 1;
}

// 必须传arg,不然swapcontext执行是不会有this，导致段错误
void schedule_t::uthread_func(void *arg) {
    schedule_t *ps = (schedule_t*)(arg);
	int id = ps->running_thread;
	if(id != -1) {
		uthread_t *puth = &ps->threads[id];
		puth->state = RUNNING;
		//puth->cofunc();
		puth->task();
		puth = &ps->threads[id]; //bugfix 没有这一句会段错误或者死循环，因为
		//这里可能会由于threads 以2倍扩展分配新空间，导致puth指向旧的内存空间，然后swapcontext段错误
		puth->state = FREE;
		ps->running_thread = -1;
	}
}
