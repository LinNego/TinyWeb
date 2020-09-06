# TinyWeb

TinyWeb采用Proactor模式来设计的一个小型的Web服务器，服务器是基于事件驱动，并且预线程化。使用epoll进行监听，采用C++语言编写，在linux平台下运行，封装了linux下的一些线程同步的类型，诸如mutex，condition_varible，和sem。主线程只负责接受I/O，工作线程负责处理逻辑。目前只完成了GET方法，并且请求只有一个页面。

**2020.9.7**

第一次提交的是一个小demo。