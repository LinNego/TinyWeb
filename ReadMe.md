# TinyWebServer

一个运行在linux平台下的基于非阻塞I/O和事件驱动的轻量级Web服务器。

1. 使用预线程化+非阻塞I/O+epoll(ET模式和LT模式) + 事件处理(同步I/O模拟的Procator和Reactor模式)
2. 支持解析GET和POST请求
3. 访问服务器数据库达到注册和登陆的功能
4. 实现同步/异步的日志系统

