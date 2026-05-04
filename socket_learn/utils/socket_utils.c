/*
    https://sourceware.org/glibc/manual/latest/html_node/Internet-Address-Formats.html
    The data types for representing socket addresses in the Internet namespace are defined in the header file netinet/in.h
    Linux系统编程 第59.4节 Internet socket地址
*/
#include <netinet/in.h>

/*
    使用宏，对于不同的平台使用不同的头文件

    不同操作系统对应的宏见
    https://stackoverflow.com/questions/142508/how-do-i-check-os-with-a-preprocessor-directive
    https://sourceforge.net/p/predef/wiki/OperatingSystems/
*/
#ifdef _WIN64
// winsock.h和winsock2.h应该使用哪个? https://stackoverflow.com/questions/14094457/winsock-h-winsock2-h-which-to-use
#include <winsock2.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <sys/socket.h>
#endif

// close函数
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>

#include <arpa/inet.h>

#include <fcntl.h>
#include <errno.h>
#include <time.h>

#include "socket_utils.h"

#define SERVER_PORT 18080
#define SERVER_IP "127.0.0.1"

//  make a server socket or a client socket.
int make_socket()
{
    int sockfd;
    /*
        This is the data type used to represent socket addresses in the Internet namespace. It has the following members
        https://sourceware.org/glibc/manual/latest/html_node/Internet-Address-Formats.html

        A corresponding symbolic name starting with ‘AF_’ designates the address format for that namespace
        AF repsresents Address Formats
        AF_INET IPv4 Internet protocols

        关于使用AF_还是PF_, 见Linux系统编程 第56.1 概述, 作者都使用AF,所以这里也使用AF

        SOCK_STREAM 可靠性传输协议
        SOCK_DGRAM 不可靠传输协议

        For each combination of style and namespace there is a default protocol, which you can request by specifying 0 as the protocol number. And that’s what you should normally do—use the default

        https://sourceware.org/glibc/manual/latest/html_node/Creating-a-Socket.html

        创建套接字见Linux系统编程 第56.2 创建Socket
    */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    /*
        socket默认是阻塞式的
        Linux系统编程 5.9 非阻塞式I/O
        因为无法通过 open()来获取管道和套接字的文件描述符，所以要启用非阻塞标志，就必须使用5.3节所述fcntl()的F_SETFL命令

        https://snapshots.sourceware.org/glibc/trunk/2025-02-03_13-41_1738590061/manual/html_node/Operating-Modes.html
        The bit that enables nonblocking mode for the file. If this bit is set, read requests on the file can return immediately with a failure status if there is no input immediately available, instead of blocking.
        Likewise, write requests can also return immediately with a failure status if the output can’t be written immediately
    */
    int flags = fcntl(sockfd, F_GETFL);
    if (flags == -1)
    {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }
    if (!(flags & O_NONBLOCK))
    {
        printf("socket is blocking I/O\n");
    }
    return sockfd;
}

/*
    Linux系统编程 5.3 打开文件的状态标志
    为了修改打开文件的状态标志，可以使用fcntl()的F_GETFL命令来获取当前标志的副本，然后修改需要变更的比特位，
    最后再次调用fcntl()函数的F_SETFL命令来更新此状态标志。因此，为了添加O_APPEND标志，可以编写如下代码

    https://sourceware.org/glibc/manual/2.25/html_node/Getting-File-Status-Flags.html
    The normal return value from fcntl with this command is an unspecified value other than -1, which indicates an error.
*/
int set_nonblocking(int sockfd)
{
    int flags = fcntl(sockfd, F_GETFL);
    if (flags == -1)
    {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }
    flags |= O_NONBLOCK;
    if (fcntl(sockfd, F_SETFL, flags) == -1)
    {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }
    printf("socket is nonblocking I/O now\n");
    return 0;
}

int bind_server_socket_to_port_and_listen()
{
    int sockfd = make_socket();
    // internet socket地址 见Linux系统编程 第59.4节 Internet socket地址
    struct sockaddr_in server_addr_info;
    server_addr_info.sin_family = AF_INET;
    // 关于为什么要使用该函数?

    /*
       1. 见 https://sourceware.org/glibc/manual/latest/html_node/Byte-Order.html
       2. 见 Linux系统编程 第59.2 网络字节序
       IP地址和端口号是整数值。在将这些值在网络中传递时碰到的一个问题是不同类型的计算机的硬件结构会以不同的顺序来存储一个多字节整数的字节，即大端存储和小端存储。为了统一，网络协议规定了一个标准的字节顺序，称为网络字节序，使用大端存储
       使用一些特定的函数可以将一个整数转为标准的网络字节序，然后把转换后的整数写入套接字地址结构中 传递出去，这样无论发送方和接收方的计算机使用什么样的字节顺序，网络协议都能正确地解释这些整数值
       htons 全称 host to network short, 将一个16位整数从主机字节序转换为网络字节序

       这里为什么使用htons而不是htonl? 因为端口号是16位整数, 而IP地址是32位整数
       引用自Linux系统编程 第59.2 网络字节序 > sin_port和sin_addr字段是端口号和IP地址，它们都是网络字节序的。in_port_t和in_addr_t数据类型是无符号整型，其长度分别为16位和32位
    */
    server_addr_info.sin_port = htons(SERVER_PORT);
    /*
       见Linux系统编程 58.5 IP地址 > 常量 INADDR_ANY是所谓的IPV4通配地址 还有 第59.2 网络字节序 后半段也提到了它

       This data type is used in certain contexts to contain an IPv4 Internet host address.
       It has just one field, named s_addr, which records the host address number as an uint32_t
    */
    server_addr_info.sin_addr.s_addr = INADDR_ANY;

    /*
        https://sourceware.org/glibc/manual/latest/html_node/Setting-Address.html
        A socket newly created with the socket function has no address.
        Other processes can find it for communication only if you give it an address.
        We call this binding the address to the socket, and the way to do it is with the bind function

        该回答来自于grok
        第二个参数的类型是 const struct sockaddr *，这是一个通用地址结构的指针，用于兼容不同协议族的地址（如 IPv4 的 sockaddr_in 或 IPv6 的 sockaddr_in6）。
        假设 server_addr 的类型是 struct sockaddr_in（这是常见的 IPv4 地址结构），那么 &server_addr 的类型是 struct sockaddr_in *。
        直接将 &server_addr 作为第二个参数传递会引发类型不匹配的编译错误（incompatible pointer types），
        因为 struct sockaddr_in * 和 const struct sockaddr * 是不同的类型，尽管它们在内存布局上兼容。
        为了解决这个问题，需要显式地将 &server_addr 强制类型转换为 (struct sockaddr *)，这样才能匹配 bind 函数的期望类型。这是一种常见的类型转换实践，确保代码在不同协议族下灵活，同时避免编译警告或错误。

        还有 Linux系统编程 第56.4 通用socket地址结构：struct sockaddr

        https://sourceware.org/glibc/manual/latest/html_node/Address-Formats.html
    */
    if (bind(sockfd, (struct sockaddr *)&server_addr_info, sizeof(server_addr_info)) < 0)
    {
        perror("bind");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    /*
        监听连接 表示这个socket可以接受连接请求 https://sourceware.org/glibc/manual/latest/html_node/Listening.html
        Now let us consider what the server process must do to accept connections on a socket.
        First it must use the listen function to enable connection requests on the socket,
        and then accept each incoming connection with a call to accept if (listen(server_socket, 5) < 0)

        关于backlog的解释,见 Linux系统编程 56.5.1 > SUSv3规定实现应该通过在<sys/socket.h>中定义SOMAXCONN常量来发布这个限制

        https://sourceware.org/glibc/manual/latest/html_node/Accepting-Connections.html#index-accepting-connections
        If connection requests arrive from clients faster than the server can act upon them,
        the queue can fill up and additional requests are refused with an ECONNREFUSED error.
         You can specify the maximum length of this queue as an argument to the listen function

         这个等待队列是TCP3次握手之后的等待队列，如果TCP3次握手成功，队列还是满的，那么内核可能把该连接丢弃，反应在客户端就是连接建立成功，但是发送数据超时

         下列流程图来自claude > https://claude.ai/chat/bad1e6b6-9f6a-4c40-8ef0-83b60018f5cb
         客户端 SYN 到达
            ↓
        ┌─────────────────────┐
        │  SYN 队列           │  ← 半连接队列 (incomplete)
        │  (SYN_RCVD 状态)    │    收到 SYN → 发 SYN-ACK → 等 ACK
        └─────────────────────┘
            ↓  收到第三次握手的 ACK
        ┌─────────────────────┐
        │  Accept 队列         │  ← 全连接队列 (complete)
        │  (ESTABLISHED 状态)  │    listen() 的 backlog 限制的是这个！
        └─────────────────────┘
            ↓
            accept()

        完整流程图

        Client          Kernel (Server)         App
        |                  |                   |
        |──── SYN ────────→|                   |
        |                  | 放入 SYN 队列      |
        |←── SYN-ACK ──────|                   |
        |                  |                   |
        |──── ACK ────────→|                   |
        |                  | Accept 队列满了？  |
        |                  |    ├── 否 → 放入 Accept 队列 → 等待 accept()
        |                  |    └── 是 → 丢弃 ACK (或发 RST)
        |                  |                   |
        |  （客户端以为连接建立了！）           |
        |  （但服务端 accept 队列没有它）       |
    */
    if (listen(sockfd, SOMAXCONN) < 0)
    {
        perror("listen");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

int bind_client_socket_to_port_and_connect()
{
    int sockfd = make_socket();
    /*
        这里要设置服务器的IP和服务器端口.
        通常客户端socket的端口是OS随机分配的,如果想要使用指定端口，使用bind即可. bind函数的定义就是为socket绑定IP和端口.

        inet_pton函数的作用是把点分十进制的IP地址转换成网络字节序的二进制IP地址.
        见 Linux系统编程 59.6 > net_pton()和inet_ntop()函数
        https://sourceware.org/glibc/manual/2.25/html_node/Host-Address-Functions.html#index-inet_005fnetof
    */
    struct sockaddr_in server_addr_info;
    server_addr_info.sin_family = AF_INET;
    server_addr_info.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr_info.sin_addr);
    /*
        https://sourceware.org/glibc/manual/latest/html_node/Connecting.html
    */
    if (connect(sockfd, (struct sockaddr *)&server_addr_info, sizeof(server_addr_info)) < 0)
    {
        perror("connect");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

/*
    代码来自于 https://sourceware.org/glibc/manual/latest/html_node/Server-Example.html

    关于read函数的说明
    https://sourceware.org/glibc/manual/2.25/html_node/I_002fO-Primitives.html#index-read
    In case of an error, read returns -1.
    Compatibility Note: Most versions of BSD Unix use a different error code for this: EWOULDBLOCK.
    In the GNU C Library, EWOULDBLOCK is an alias for EAGAIN, so it doesn’t matter which name you use

    Linux系统编程 5.9 非阻塞I/O
    若I/O系统调用未能立即完成，则可能会只传输部分数据，或者系统调用失败，并返回EAGAIN或EWOULDBLOCK错误

    Linux系统编程 63.1 整体概览
    如果在打开文件时设定了O_NONBLOCK标志，会以非阻塞方式打开文件。如果I/O系统调用不能立刻完成，则会返回错误而不是阻塞进程

    Linux系统编程 3.4 处理来自系统调用和库函数的错误
    系统调用失败时，会将全局整形变量 errno 设置为一个正值，以标识具体的错误。
    程序应包含 <errno.h>头文件，该文件提供了对 errno 的声明，以及一组针对各种错误编号而定义的常量
*/
int read_from_client(int client_socket)
{
    char buffer[BUFFER_SIZE];
    int bytes_read = read(client_socket, buffer, BUFFER_SIZE);

    if (bytes_read == -1)
    {
        if (errno == EAGAIN)
        {
            return READ_AGAIN;
        }
        else
        {
            perror("read");
            exit(EXIT_FAILURE);
        }
    }
    else if (bytes_read == 0)
    {
        return -1;
    }
    fprintf(stdout, "got message: '%s'\n", buffer);
    // 打开注释观察FIN_WAIT_2 超时RST现象，看server_that_can_process_requests_concurrently_using_child_process注释
    // sleep(60);
    return 0;
}

/*
    https://sourceware.org/glibc/manual/2.25/html_node/Time-Functions-Example.html
*/
int print_time()
{
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm_now);
    printf("当前时间: %s\n", buf);
    return 0;
}
