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

#include "socket_server.h"
#include "../utils/socket_utils.h"

#define MESSAGE "I have received your message"

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

int server_that_can_process_requests_concurrently()
{
    int server_socket = bind_server_socket_to_port_and_listen();
    //  https://sourceware.org/glibc/manual/2.25/html_node/Waiting-for-I_002fO.html
    // A better solution is to use the select function. This blocks the program until input or output is ready on a specified set of file descriptors,
    // or until a timer expires, whichever comes first
    fd_set active_fd_set, read_fd_set;
    FD_ZERO(&active_fd_set);
    FD_SET(server_socket, &active_fd_set);
    struct sockaddr_in client_socket;
    int i;
    socklen_t client_len;
    while (1)
    {
        read_fd_set = active_fd_set;
        if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0)
        {
            perror("select");
            exit(EXIT_FAILURE);
        }

        for (i = 0; i < FD_SETSIZE; i++)
        {
            if (FD_ISSET(i, &read_fd_set))
            {
                if (i == server_socket)
                {
                    /* connection request on original socket. */
                    int new_client_socket;
                    client_len = sizeof(client_socket);
                    // 接受连接 https://sourceware.org/glibc/manual/latest/html_node/Accepting-Connections.html
                    // accept 返回值是一个新的 socket 描述符
                    /*
                        Accepting a connection does not make socket part of the connection.
                        Instead, it creates a new socket which becomes connected.
                        The normal return value of accept is the file descriptor for the new socket
                    */
                    new_client_socket = accept(server_socket, (struct sockaddr *)&client_socket, &client_len);
                    if (new_client_socket < 0)
                    {
                        perror("accept");
                        exit(EXIT_FAILURE);
                    }
                    fprintf(stderr,
                            "Server: connect from host %s, port %hd.\n",
                            inet_ntoa(client_socket.sin_addr),
                            ntohs(client_socket.sin_port));
                    FD_SET(new_client_socket, &active_fd_set);
                }
                else
                {
                    /* data arriving on an already-connected socket. */
                    if (read_from_client(i) < 0)
                    {
                        close(i);
                        FD_CLR(i, &active_fd_set);
                    }
                }
            }
        }
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
        例如：https://chessman7.substack.com/p/fork-and-file-descriptors-the-unix 博客中提到
        Only when all file descriptors pointing to an open file description are closed does the kernel actually close the fil

    3. 【文件系统级】的i-node表，每个文件系统都会为驻留其上的所有文件建立一个i-node表

    创建进程失败时，可能的原因：
    Linux系统编程 24.2 和 36.3
    1. RLIMIT_NOFILE限制规定了一个数值，该数值等于一个进程能够分配的最大文件描述符数量加1
    2. 还存在一个系统级别的限制，它规定了系统中所有进程能够打开的文件数量
    3. RLIMIT_NPROC限制规定了调用进程的真实用户ID下最多能够创建的进程数量。试图（fork()、vfork()和clone()）超出这个限制会得到EAGAIN错误

    关于引用计数为0时的行为 > https://claude.ai/chat/0c74635f-6692-469c-bd81-1031edcc10a4
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
