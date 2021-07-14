// ref: ucontext-人人都可以实现的简单协程库
// link: https://blog.csdn.net/qq910894904/article/details/41911175
//
// g++ -g -O2 -std=c++11 coroutine_test.cpp coroutine.cpp -o coroutine_test

// ucontext例子
/*
#include <ucontext.h>
#include <unistd.h>

#include <iostream>

using namespace std;

int main() {
	ucontext_t context;

	getcontext(&context);
	cout << "hello" << endl;
	sleep(1);
	setcontext(&context);
	return 0;
}
*/

/*
#include <ucontext.h>
#include <stdio.h>
 
void func1(void * arg)
{
    puts("1");
    puts("11");
    puts("111");
    puts("1111"); 
}

void context_test()
{
    char stack[1024];
    ucontext_t child,main;
 
    getcontext(&child); //获取当前上下文
    child.uc_stack.ss_sp = stack;//指定栈空间
    child.uc_stack.ss_size = sizeof(stack);//指定栈空间大小
    child.uc_stack.ss_flags = 0;
    child.uc_link = &main;//设置后继上下文
	child.uc_link = NULL;//设置后继上下文

	puts("makecontext");
    makecontext(&child,(void (*)(void))func1,0);//修改上下文指向func1函数
 
	puts("swapcontext");
	swapcontext(&main,&child);//切换到child上下文，保存当前上下文到main
    puts("main");//如果设置了后继上下文，func1函数指向完后会返回此处

}
 
int main()
{
    context_test(); 
    return 0;
}
*/

#include <time.h>
#include <stdio.h>

#include <iostream>

#include "./coroutine.h"

using namespace std;

void cofunc1(schedule_t *ps, void *arg) {
	cout << "func1...1" << endl;
	cout << "func1...2" << endl;
	return ;
}

void cofunc2(schedule_t *ps, void *arg) {
	cout << "func2...1" << endl;
	ps->uthread_yield();
	cout << "func2...2" << endl;
	return ;
}

void cofunc3(schedule_t *ps, void *arg) {
	cout << "func3...1" << endl;
	ps->uthread_yield();
	cout << "func3...2" << endl;
	return ;
}

//功能测试
void schedule_main() {
	schedule_t sche;
	void *arg;
	int id1 = sche.uthread_create(std::bind(cofunc2, &sche, arg));
	int id2 = sche.uthread_create(std::bind(cofunc3, &sche, arg));
	while(!sche.schedule_finished()) {
		sche.uthread_resume(id1);
		sche.uthread_resume(id2);
	}
	cout << "schedule_main end" << endl;
}

//测试性能
void coroutine_switch_performance_test_func(schedule_t *ps, int times) {
	int i = 0;	
	//百万次上下文切换测试
	while(i < 1000000 * times) {
		ps->uthread_yield();
		++i;
	}
}

void coroutine_performance_test() {
	schedule_t sche;
	int times = 10;
	int id = sche.uthread_create(std::bind(coroutine_switch_performance_test_func, &sche, times));
	clock_t start, end;
	start = clock();
	while(!sche.schedule_finished()) {
		sche.uthread_resume(id);
	}
	end = clock();
	printf("total %d times context switch\ntake Time：%lf ms\navg time: %lf us\n", 
		times*1000000, 
		(double)(end - start)/1000,
		(double)(end - start)/(times*1000));
}

int main() {	
	schedule_main();

	coroutine_performance_test();

	return 0;
}
