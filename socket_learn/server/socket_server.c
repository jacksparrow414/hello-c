#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

#include <netinet/in.h>

#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>

#include "socket_server.h"
#include "../utils/socket_utils.h"

#define PORT 5555

// 代码来自于 https://sourceware.org/glibc/manual/latest/html_node/Server-Example.html
int read_from_client(int client_socket)
{
    char buffer[BUFFER_SIZE];
    int bytes_read = read(client_socket, buffer, BUFFER_SIZE);
    if (bytes_read < 0)
    {
        perror("read");
        exit(EXIT_FAILURE);
    }
    else if (bytes_read == 0)
    {
        return -1;
    }
    fprintf(stderr, "Server: got message: `%s'\n", buffer);
    return 0;
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
        while (1)
        {
            if (read_from_client(new_client_socket) < 0)
            {
                close(new_client_socket);
                break;
            }
        }
    }
}
