#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include <netinet/in.h>

#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "socket_server.h"
#include "../utils/socket_utils.h"

#define MESSAGE "I have received your message"

#if defined(__linux__)
#include <sys/epoll.h>
#define USE_EPOLL 1
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#include <sys/event.h>
#define USE_KQUEUE 1
#elif defined(_WIN64)
#include <winsock2.h>
#define USE_SELECT 1
#else
#error "Unsupported platform"
#endif

#define MAX_EVENTS 10

struct client_info
{
    int client_socket;
    int using_non_blocking_io;
};

void handle_request(int client_socket)
{
    while (1)
    {
        if (read_from_client(client_socket) <= 0)
        {
            write(client_socket, MESSAGE, strlen(MESSAGE) + 1);
            close(client_socket);
            printf("close client socket\n");
            break;
        }
    }
}

void *handle_request_for_threads(void *client_socket)
{
    int client_fd = *(int *)client_socket;
    free(client_socket);
    printf("thread is %lu\n", (unsigned long)pthread_self());
    handle_request(client_fd);
    return NULL;
}

/*
    Linux系统编程 63.1 整体概览
    这种轮询通常是我们不希望看到的。如果轮询的频率不高，那么应用程序响应I/O事件的延时可能会达到无法接受的程度。
    换句话说，在一个紧凑的循环中做轮询就是在浪费CPU
*/
void *handle_request_for_threads_using_non_block_io(void *arg)
{
    struct client_info *client = (struct client_info *)arg;
    int client_fd = client->client_socket;
    int using_non_blocking_io = client->using_non_blocking_io;
    printf("i/o mode: %d\n", using_non_blocking_io);
    free(client);
    if (using_non_blocking_io)
    {
        while (1)
        {
            int result = read_from_client(client_fd);
            if (result <= 0)
            {
                close(client_fd);
                printf("close client socket\n");
                break;
            }
            else if (result == READ_AGAIN)
            {
                print_time();
                printf("no data now, please read again after 3 seconds\n");
                // 为了充分利用CPU，休眠这段时间，实际上可以让线程去做点其他事情
                // do something else
                sleep(3);
            }
        }
    }
    else
    {
        printf("non-blocking io\n");
    }
    return NULL;
}

/*
    Linux系统编程 63.4.1 创建epoll实例
    这个文件描述符在其他几个epoll系统调用中用来表示epoll实例。当这个文件描述符不再需要时，应该通过close()来关闭
    【当所有与epoll实例相关的文件描述符都被关闭时，实例被销毁，相关的资源都返还给系统】。（多个文件描述符可能引用到相同的epoll实例，这是由于调用了fork()或者dup()这样类似的函数所致。）

    这句话和server_that_can_process_requests_concurrently_using_child_process方法上有关文件描述符的注释形成呼应
*/
int server_that_can_process_requests_concurrently_using_io_multiplexing_by_event_poll()
{
    int epfd = epoll_create1(0);
    if (epfd == -1)
    {
        perror("epoll_create");
        exit(1);
    }
    int server_socket = bind_server_socket_to_port_and_listen();
    set_nonblocking(server_socket);
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = server_socket;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, server_socket, &ev) == -1)
    {
        perror("epoll_ctl");
        exit(1);
    }
    struct epoll_event evlist[MAX_EVENTS];
    while (1)
    {
        int nready = epoll_wait(epfd, evlist, MAX_EVENTS, -1);
        if (nready == -1)
        {
            perror("epoll_wait");
            exit(1);
        }
        printf("nready = %d\n", nready);
        for (int i = 0; i < nready; i++)
        {
            int fd = evlist[i].data.fd;
            if (fd == server_socket)
            {
                int new_client_socket = accept(server_socket, NULL, NULL);
                if (new_client_socket < 0)
                {
                    perror("accept");
                    exit(EXIT_FAILURE);
                }
                printf("new epoll client connected\n");
                set_nonblocking(new_client_socket);
                struct epoll_event client_ev;
                client_ev.events = EPOLLIN;
                client_ev.data.fd = new_client_socket;
                if (epoll_ctl(epfd, EPOLL_CTL_ADD, new_client_socket, &client_ev) == -1)
                {
                    perror("epoll_ctl");
                    exit(1);
                }
            }
            else if (evlist[i].events & (EPOLLIN | EPOLLHUP | EPOLLERR | EPOLLRDHUP))
            {
                printf("fd %d is ready to read\n", fd);
                int result = read_from_client(fd);
                printf("result %d\n", result);
                if (result <= 0)
                {
                    // epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                    printf("fd %d is closed\n", fd);
                    // 调用close后内核会自动把fd从epoll中删除，无需显示调用epoll_ctl
                    close(fd);
                }
                else if (result == READ_AGAIN)
                {
                    print_time();
                    printf("no connection now, please connect after 3 seconds\n");
                    continue;
                }
            }
        }
    }
    close(server_socket);
    close(epfd);
    return 0;
}

/*
    https://sourceware.org/glibc/manual/2.25/html_node/Waiting-for-I_002fO.html
    A better solution is to use the select function.
    This blocks the program until input or output is ready on a specified set of file descriptors, or until a timer expires, whichever comes first

    Linux系统编程 60.4 并发型服务器的其他设计方案
    在设计单进程服务器时，服务器进程必须做一些通常由内核来处理的调度任务。
    在每个客户端一个服务器进程地解决方案中，我们可以依靠内核来确保每个服务器进程（从而也确保了客户端）能公平地访问到服务器主机的资源。
    但当我们用单个服务器进程来处理多个客户端时，服务器进程必须自行确保一个或多个客户端不会霸占服务器，从而使其他的客户端处于饥饿状态

    Linux系统编程 63.2.1 select()系统调用
    FD_ZERO()将fdset所指向的集合初始化为空。
    FD_SET()将文件描述符fd添加到由fdset所指向的集合中。
    FD_CLR()将文件描述符fd从fdset所指向的集合中移除。
    如果文件描述符fd是fdset所指向的集合中的成员，FD_ISSET()返回true
    FD_SET是1个有16个元素的数组，元素数据类型为unsigned long,该类型在Linux系统+64位机器上 占8字节，64位，所以每个元素为64位，这样每1位都可以按照下列条件来判断文件描述符是否在这个位图中
        fd_set = unsigned long fds_bits[16]
          └── 16个元素 × 64位 = 1024位 总容量
        第一步：fd / 64  →  确定在哪个数组元素（哪个槽）
        第二步：fd % 64  →  确定在该元素的哪一位（槽内偏移）

    select是如何知道一个文件描述符已就绪呢？
    来自claude的回答
        select() 的就绪判断发生在内核态，它直接检查每个 fd 对应的内核数据结构状态，而不是真的去调用 I/O 函数试探
        每种类型的 fd，内核都维护着对应的数据结构，就绪判断就是直接读这些结构的字段,如tcp_poll,pipe_poll
        socket 可读？  → 检查 接收缓冲区 sk_receive_queue 是否非空
        socket 可写？  → 检查 发送缓冲区 剩余空间是否足够
        监听socket？   → 检查 accept队列 是否非空
        管道可读？     → 检查 管道缓冲区 是否有数据，或写端是否已关闭

        O_NONBLOCK 的意义：保护 select() 返回之后的那段时间
        select() 判断就绪是一个时间点的快照，但从 select() 返回到你真正调用 read()/accept() 之间，有一段时间差，状态可能在这段时间内发生变化。
            场景一：多进程/线程竞争同一个 fd
            进程A                进程B                 内核
            |                    |                    |
            | select()返回可读   | select()返回可读   |（同一个socket）
            |                    |                    |
            | [准备调用read()]   |                    |
            |                    | read() ──────────→ | 数据被B读走了
            |                    | ← 返回数据         |
            |                    |                    |
            | read() ──────────→ |                    | 缓冲区已空
            |                    |                    |
            没有O_NONBLOCK → 永久阻塞 ❌
            有O_NONBLOCK   → 返回EAGAIN，继续处理其他事情 ✅

            这个回答也提到这种场景 https://stackoverflow.com/questions/12625224/how-is-select-alerted-to-an-fd-becoming-ready#comment17025226_12626119
*/
int server_that_can_process_requests_concurrently_using_io_multiplexing_by_select()
{
    int server_socket = bind_server_socket_to_port_and_listen();
    set_nonblocking(server_socket);
    fd_set active_fd_set, read_fd_set;
    FD_ZERO(&active_fd_set);
    FD_SET(server_socket, &active_fd_set);
    int i;
    int max_fd = 0;
    if (server_socket >= max_fd)
    {
        max_fd = server_socket + 1;
    }
    while (1)
    {
        read_fd_set = active_fd_set;
        /*
            The timeout specifies the maximum time to wait.
            If you pass a null pointer for this argument, it means to block indefinitely until one of the file descriptors is ready.

            Linux系统编程 63.2.1 select()系统调用
            返回一个正整数表示有1个或多个文件描述符已达到就绪态。返回值表示处于就绪态的文件描述符个数。
            在这种情况下，每个返回的文件描述符集合都需要检查（通过 FD_ISSET()），以此找出发生的 I/O 事件是什么

            参数nfds必须设为比3个文件描述符集合中所包含的最大文件描述符号还要大1。
            该参数让select()变得更有效率，因为此时内核就不用去检查大于这个值的文件描述符号是否属于这些文件描述符集合

            readfds是用来检测输入是否就绪的文件描述符集合
            writefds是用来检测输出是否就绪的文件描述符集合
            exceptfds是用来检测异常情况是否发生的文件描述符集合
        */
        int active_fd_num = select(max_fd, &read_fd_set, NULL, NULL, NULL);
        if (active_fd_num < 0)
        {
            perror("select");
            exit(EXIT_FAILURE);
        }
        printf("select() returns %d\n", active_fd_num);
        for (i = 0; i < max_fd; i++)
        {
            if (FD_ISSET(i, &read_fd_set))
            {
                printf("fd %d is ready\n", i);
                if (i == server_socket)
                {
                    int new_client_socket = accept(server_socket, NULL, NULL);
                    if (new_client_socket < 0)
                    {
                        perror("accept");
                        exit(EXIT_FAILURE);
                    }
                    printf("new client connected\n");
                    set_nonblocking(new_client_socket);
                    FD_SET(new_client_socket, &active_fd_set);
                    if (new_client_socket >= max_fd)
                    {
                        max_fd = new_client_socket + 1;
                    }
                }
                else
                {
                    int result = read_from_client(i);
                    if (result <= 0)
                    {
                        close(i);
                        FD_CLR(i, &active_fd_set);
                    }
                    else if (result == READ_AGAIN)
                    {
                        print_time();
                        printf("no connection now, please connect after 3 seconds\n");
                        continue;
                    }
                }
            }
        }
    }
}

/*
    测试: (sleep 15; echo "hello"; sleep 15) | nc 127.0.0.1 18080
    这种服务器类型有致命的问题：
        当1万个连接到来时，accept接受新连接后，还要并行创建1万个线程处理请求。而创建线程是很耗时的，而且创建线程需要时间。
        此时如果第10001个连接到来时，第10000个线程还没有创建完成，那么第10001个线程就会等待，等待时间为前10000个线程创建完成所耗的时间。
        线程池+提前创建好线程倒是可以解决这个问题

    Linux系统编程 60.4 并发型服务器的其他设计方案
    服务器程序在启动阶段（即在任何客户端请求到来之前）就立刻预先创建好一定数量的子进程（或线程），而不是针对每个客户端来创建一个新的子进程（或线程）
*/
int server_that_can_process_requests_concurrently_using_thread_and_non_blocking_io()
{
    int server_socket = bind_server_socket_to_port_and_listen();
    set_nonblocking(server_socket);
    while (1)
    {
        int new_client_socket = accept(server_socket, NULL, NULL);
        if (new_client_socket < 0)
        {
            if (errno == EAGAIN)
            {
                print_time();
                printf("no connection now, please connect after 3 seconds\n");
                sleep(3);
                continue;
            }
            else
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }
        }
        set_nonblocking(new_client_socket);
        pthread_t client_thread;
        struct client_info *client = malloc(sizeof(struct client_info));
        client->client_socket = new_client_socket;
        client->using_non_blocking_io = 1;
        int create_result = pthread_create(&client_thread, NULL, handle_request_for_threads_using_non_block_io, (void *)client);
        if (create_result != 0)
        {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
        pthread_detach(client_thread);
    }
}
/*
    并发型服务器: 服务器可以同时处理多个客户端，使用线程实现
*/
int server_that_can_process_requests_concurrently_using_thread()
{
    int server_socket = bind_server_socket_to_port_and_listen();
    while (1)
    {
        int new_client_socket = accept(server_socket, NULL, NULL);
        if (new_client_socket < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        /*
            必须把new_client_socket传递给子线程且以指针的形式，如果直接传&new_client_socket，
            但是假设此时子线程还创建完还没拿到CPU的执行权，此时主线程又accept了另外一个客户端，此时子线程再拿这个new_client_socket对于的内存地址数据,拿的可能是第二个客户端的socket
        */
        pthread_t client_thread;
        int *pclient = malloc(sizeof(int));
        if (pclient == NULL)
        {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        /*
            这里还有一种不用malloc的方式，使用 (void *)(intptr_t)new_client_socket
            Linux系统编程 29.3 创建线程
            通过审慎的类型强制转换，arg甚至可以传递int类型的值
            说的应该就是这种方式，在指针函数中使用
            int client_fd = (int)(intptr_t)arg;
            转回来
        */
        *pclient = new_client_socket;
        int create_result = pthread_create(&client_thread, NULL, handle_request_for_threads, pclient);
        if (create_result != 0)
        {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
        /*
            Linux系统编程 29.7 线程分离
            有时，程序员并不关心线程的返回状态，【只是希望系统在线程终止时能够自动清理并移除之】
            在这种情况下，可以调用pthread_detach()并向thread参数传入指定线程的标识符，将该线程标记为处于分离（detached）状态
        */
        pthread_detach(client_thread);
    }
}

/*
    并发型服务器: 服务器可以同时处理多个客户端，使用子进程实现
    Linux系统编程 60.1 迭代型和并发型服务器

    关于文件描述符的一些前置知识：

    Linux系统编程 5.4　文件描述符和打开文件之间的关系
    内核对于打开的文件会维护3张表，分别是
    1. 【进程级】的文件描述符表，针对每个进程，内核为其维护打开文件的描述符，主要列为 【文件描述符标志+文件指针】，该指针指向第2项
    2. 【系统级】的打开文件表，open file description table/open file table, 主要列为【当前文件偏移量+打开文件时所使用的标志+文件访问模式+与信号驱动I/O相关的设置+i-node指针】，该指针指向第3项
        2.1 系统级打开文件表还有一个关键列为文件描述符的引用计数，【只有当引用计数为0时，内核才会进行真正【关闭】操作，释放资源等】，
        这个引用计数目前没找到书籍佐证，但是网上很多博客都提到了这个引用计数和内核关闭，【释放资源】相关
        https://www.cs.cmu.edu/afs/cs/academic/class/15213-f15/www/lectures/16-io.pptx CSAPP 课程课件也提到了这个引用计数
        Child’s table same as parent’s, and +1 to each refcnt

        还有例如：https://chessman7.substack.com/p/fork-and-file-descriptors-the-unix 博客中提到
        Only when all file descriptors pointing to an open file description are closed does the kernel actually close the fil

    3. 【文件系统级】的i-node表，每个文件系统都会为驻留其上的所有文件建立一个i-node表

    创建进程失败时，可能的原因：
    Linux系统编程 24.2 和 36.3
    1. RLIMIT_NOFILE限制规定了一个数值，该数值等于一个进程能够分配的最大文件描述符数量加1
    2. 还存在一个系统级别的限制，它规定了系统中所有进程能够打开的文件数量
    3. RLIMIT_NPROC限制规定了调用进程的真实用户ID下最多能够创建的进程数量。试图（fork()、vfork()和clone()）超出这个限制会得到EAGAIN错误

    关于引用计数为0时的行为 > https://claude.ai/chat/0c74635f-6692-469c-bd81-1031edcc10a4 还有server_that_can_process_requests_concurrently_using_io_multiplexing_by_event_poll方法上的注释形成呼应
    用户调用 close(fd)
    │
    ├── 第一层（每次 close() 都会发生）
    │       └── 将 fd 从该进程的文件描述符表中移除
    │           引用计数 -1
    │
    └── 第二层（仅当引用计数归零时才发生）
            └── 内核释放底层资源
                （刷盘 / 发FIN / 释放内存等）

    普通文件
    close() → 引用计数归零
            │
            ├── 1. 刷新页缓存（page cache）中的脏页写回磁盘
            │        （如果有 O_SYNC 或调用了 fsync()，早已写回）
            │
            ├── 2. 释放该进程持有的文件锁（flock / fcntl lock）
            │
            ├── 3. 释放内核中的 struct file 对象
            │
            └── 4. inode 引用计数 -1
                    │
                    └── 如果 inode 引用计数也归零（文件已被unlink且无进程打开）
                            └── 释放 inode，磁盘块被标记为空闲

    内核和TCP关闭连接时的行为：
    根据上述前置知识，只有文件描述符的引用计数为0时，内核才会进行真正【关闭】操作，释放资源等。对应在TCP关闭连接时的行为：
    如果是主动关闭连接的一方，发送第1个FIN包；
    如果是被动接受关闭连接的那一方，才会从当前的CLOSE-WAIT状态进入到下一阶段
    close() → 引用计数归零
            │
            ├── 1. 根据 SO_LINGER 选项决定行为
            │        ├── 默认（不设置）：后台异步完成关闭，close() 立即返回
            │        └── 设置了 SO_LINGER：阻塞等待数据发送完毕
            │
            ├── 2. 发送缓冲区中还未发出的数据 → 继续发完
            │
            ├── 3. 发送 FIN 包给对端（TCP 四次挥手开始）
            │        对端收到后，会回 ACK，然后对端也发 FIN
            │
            ├── 4. 等待 TIME_WAIT 状态结束（主动关闭方）
            │        默认 2*MSL（约 60~120 秒），防止最后一个 ACK 丢失
            │
            ├── 5. 释放发送/接收缓冲区内存
            │
            └── 6. 释放 struct socket、struct sock 等内核对象

    这里测试时偶然触发了个有意思的现象：看read_from_client注释
    服务器关闭里socket，由于引用计数降为0，所以它发送了FIN，客户端ACK了之后，客户端休眠了60秒，60秒之后再发送FIN，但是服务器此时是RST

    这是因为服务器在收到客户端ACK之后进入FIN_WAIT_2阶段，这个阶段是有超时限制的，这个限制刚好是60s，到60s后内核直接清理这个socket
    疑问：如果这60s内客户端有一直发数据，60s后还会RST吗？AI给的回答是依然会，如果不想出现这种情况应该使用shutdown.这一点还没有找到书籍或实验验证

*/
int server_that_can_process_requests_concurrently_using_child_process()
{
    int server_socket = bind_server_socket_to_port_and_listen();
    while (1)
    {
        /*
            如果不关心客户端的连接信息，则后面2个参数可以传NULL
            https://linux.die.net/man/2/accept
        */
        int new_client_socket = accept(server_socket, NULL, NULL);
        if (new_client_socket < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        printf("Server: new client connected.");
        /*
            Linux系统编程 24.2 创建新进程 fork()
            理解fork()的诀窍是，要意识到，完成对其调用后将存在两个进程，且每个进程都会从fork()的返回处继续执行

            执行fork()时，子进程会获得父进程所有文件描述符的副本。这些副本的创建方式类似于dup()，这也意味着父、子进程中对应的描述符均指向相同的打开文件句柄（即 open file description table/open file table）

            基于上述知识和方法注释，就可以明白
            1. 为什么父进程要关闭new_client_socket？因为父进程不关心连接后的事情，它只负责监听连接，所以要把new_client_socket关闭以使得new_client_socket文件描述符的引用计数减1
                1.1 如果父进程不关闭new_client_socket文件描述符，那么内核不会真正发起关闭TCP连接动作，因为引用计数不为0
                1.2 同时由于它不关闭，那么它持有的文件描述符数量会一直增加，从而导致进程数量超过系统限制. Linux系统编程 36.3 RLIMIT_NOFILE 和 RLIMIT_NPROC的说明
            2. 为什么子进程要关闭server_socket？因为子进程要处理连接不处理监听连接，所以要把server_socket关闭以使得server_socket文件描述符的引用计数减1
        */
        switch (fork())
        {
        case -1:
            close(new_client_socket);
            break;
        case 0:
            close(server_socket);
            handle_request(new_client_socket);
            _exit(0);
        default:
            close(new_client_socket);
            break;
        }
    }
}

/*
    迭代型服务器: 服务器每次只处理一个客户端，只有当完全处理完一个客户端的请求后才去处理下一个客户端
    Linux系统编程 60.1 迭代型和并发型服务器
*/
int server_that_can_only_process_requests_iteratively()
{
    int server_socket = bind_server_socket_to_port_and_listen();
    while (1)
    {
        // 更通用的结构应该使用sockaddr_storage 见 Linux系统编程 59.4 sockaddr_storage结构
        struct sockaddr_in client_socket;
        socklen_t addr_len = sizeof(client_socket);
        printf("server is ready for accept connection......\n");
        /*
             https://sourceware.org/glibc/manual/latest/html_node/Accepting-Connections.html
             Accepting a connection does not make socket part of the connection.
             Instead, it creates a new socket which becomes connected.
             The normal return value of accept is the file descriptor for the new socket
        */
        int new_client_socket = accept(server_socket, (struct sockaddr *)&client_socket, &addr_len);
        if (new_client_socket < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        fprintf(stderr,
                "Server: connect from host %s, port %hd.\n",
                inet_ntoa(client_socket.sin_addr),
                ntohs(client_socket.sin_port));
        handle_request(new_client_socket);
    }
}
