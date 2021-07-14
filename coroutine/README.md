# coroutine
简易协程实现--目前版本基于ucontext.h

## Introduction 
基于ucontext.h实现简单的协程，每个协程都有各自的栈空间，缺点是比较耗内存。  
采用共享栈模式可以改进这缺点，但是会引入栈内存拷贝的一些性能开销。  
做了简单的功能测试和上下文切换性能测试。  

## Build and Run
* 进入目录，执行：
  $ g++ -O2 -std=c++11 coroutine_test.cpp coroutine.cpp -o coroutine_test
* 运行：
  $ ./coroutine_test

## Simple Performance Test
1千万次上下文切换，耗时6.563ms，平均656.3us。

## TODO
有空继续研究下ucontext的汇编实现或者其他开源的实现(boost coroutine、[libco](https://github.com/Tencent/libco)、[libgo](https://github.com/yyzybb537/libgo))，还有C++20的coroutine

## Other Blog
[ucontext-人人都可以实现的简单协程库](https://blog.csdn.net/qq910894904/article/details/41911175)  
[C++20 Coroutine 性能测试 (附带和libcopp/libco/libgo/goroutine/linux ucontext对比)](https://owent.net/2019/1911.html)  
